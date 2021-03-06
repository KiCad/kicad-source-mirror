/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>
#include <iterator>
#include <pcb_base_frame.h>
#include <reporter.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <track.h>
#include <zone.h>
#include <pcb_marker.h>
#include <pcb_group.h>
#include <pcb_target.h>
#include <core/arraydim.h>
#include <core/kicad_algo.h>
#include <connectivity/connectivity_data.h>
#include <kicad_string.h>
#include <pgm_base.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <ratsnest/ratsnest_data.h>
#include <tool/selection_conditions.h>
#include <convert_drawsegment_list_to_polygon.h>

// This is an odd place for this, but CvPcb won't link if it's in board_item.cpp like I first
// tried it.
wxPoint BOARD_ITEM::ZeroOffset( 0, 0 );


BOARD::BOARD() :
        BOARD_ITEM_CONTAINER( (BOARD_ITEM*) NULL, PCB_T ),
        m_boardUse( BOARD_USE::NORMAL ),
        m_timeStamp( 1 ),
        m_paper( PAGE_INFO::A4 ),
        m_project( nullptr ),
        m_designSettings( new BOARD_DESIGN_SETTINGS( nullptr, "board.design_settings" ) ),
        m_NetInfo( this ),
        m_LegacyDesignSettingsLoaded( false ),
        m_LegacyCopperEdgeClearanceLoaded( false ),
        m_LegacyNetclassesLoaded( false )
{
    // we have not loaded a board yet, assume latest until then.
    m_fileFormatVersionAtLoad = LEGACY_BOARD_FILE_VERSION;

    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        m_layers[layer].m_name = GetStandardLayerName( ToLAYER_ID( layer ) );

        if( IsCopperLayer( layer ) )
            m_layers[layer].m_type = LT_SIGNAL;
        else
            m_layers[layer].m_type = LT_UNDEFINED;
    }

    BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();

    // Initialize default netclass.
    NETCLASS* defaultClass = bds.GetDefault();
    defaultClass->SetDescription( _( "This is the default net class." ) );

    bds.UseCustomTrackViaSize( false );

    // Initialize ratsnest
    m_connectivity.reset( new CONNECTIVITY_DATA() );

    // Set flag bits on these that will only be cleared if these are loaded from a legacy file
    m_LegacyVisibleLayers.reset().set( Rescue );
    m_LegacyVisibleItems.reset().set( GAL_LAYER_INDEX( GAL_LAYER_ID_BITMASK_END ) );
}


BOARD::~BOARD()
{
    // Clean up the owned elements
    DeleteMARKERs();

    for( ZONE* zone : m_zones )
        delete zone;

    m_zones.clear();

    for( FOOTPRINT* footprint : m_footprints )
        delete footprint;

    m_footprints.clear();

    for( TRACK* t : m_tracks )
        delete t;

    m_tracks.clear();

    for( BOARD_ITEM* d : m_drawings )
        delete d;

    m_drawings.clear();

    for( PCB_GROUP* g : m_groups )
        delete g;

    m_groups.clear();
}


void BOARD::BuildConnectivity()
{
    GetConnectivity()->Build( this );
}


void BOARD::SetProject( PROJECT* aProject )
{
    if( m_project )
        ClearProject();

    m_project = aProject;

    if( aProject )
    {
        PROJECT_FILE& project = aProject->GetProjectFile();

        // Link the design settings object to the project file
        project.m_BoardSettings = &GetDesignSettings();

        // Set parent, which also will load the values from JSON stored in the project if we don't
        // have legacy design settings loaded already
        project.m_BoardSettings->SetParent( &project, !m_LegacyDesignSettingsLoaded );

        // The DesignSettings' netclasses pointer will be pointing to its internal netclasses
        // list at this point. If we loaded anything into it from a legacy board file then we
        // want to transfer it over to the project netclasses list.
        if( m_LegacyNetclassesLoaded )
            project.NetSettings().m_NetClasses = GetDesignSettings().GetNetClasses();

        // Now update the DesignSettings' netclass pointer to point into the project.
        GetDesignSettings().SetNetClasses( &project.NetSettings().m_NetClasses );
    }
}


void BOARD::ClearProject()
{
    if( !m_project )
        return;

    PROJECT_FILE& project = m_project->GetProjectFile();

    // Owned by the BOARD
    if( project.m_BoardSettings )
    {
        project.ReleaseNestedSettings( project.m_BoardSettings );
        project.m_BoardSettings = nullptr;
    }

    GetDesignSettings().SetParent( nullptr );
    m_project = nullptr;
}


std::vector<PCB_MARKER*> BOARD::ResolveDRCExclusions()
{
    for( PCB_MARKER* marker : GetBoard()->Markers() )
    {
        auto i = m_designSettings->m_DrcExclusions.find( marker->Serialize() );

        if( i != m_designSettings->m_DrcExclusions.end() )
        {
            marker->SetExcluded( true );
            m_designSettings->m_DrcExclusions.erase( i );
        }
    }

    std::vector<PCB_MARKER*> newMarkers;

    for( const wxString& exclusionData : m_designSettings->m_DrcExclusions )
    {
        PCB_MARKER* marker = PCB_MARKER::Deserialize( exclusionData );

        if( marker )
        {
            marker->SetExcluded( true );
            newMarkers.push_back( marker );
        }
    }

    m_designSettings->m_DrcExclusions.clear();

    return newMarkers;
}


bool BOARD::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( GetTitleBlock().TextVarResolver( token, m_project ) )
    {
        return true;
    }
    else if( m_properties.count( *token ) )
    {
        *token = m_properties.at( *token );
        return true;
    }

    return false;
}


wxPoint BOARD::GetPosition() const
{
    return ZeroOffset;
}


void BOARD::SetPosition( const wxPoint& aPos )
{
    wxLogWarning( wxT( "This should not be called on the BOARD object") );
}


void BOARD::Move( const wxPoint& aMoveVector )        // overload
{
    // @todo : anything like this elsewhere?  maybe put into GENERAL_COLLECTOR class.
    static const KICAD_T top_level_board_stuff[] = {
        PCB_MARKER_T,
        PCB_TEXT_T,
        PCB_SHAPE_T,
        PCB_DIM_ALIGNED_T,
        PCB_DIM_ORTHOGONAL_T,
        PCB_DIM_CENTER_T,
        PCB_DIM_LEADER_T,
        PCB_TARGET_T,
        PCB_VIA_T,
        PCB_TRACE_T,
        PCB_ARC_T,
        //        PCB_PAD_T,            Can't be at board level
        //        PCB_FP_TEXT_T,        Can't be at board level
        //        PCB_FP_SHAPE_T,       Can't be at board level
        //        PCB_FP_ZONE_T,        Can't be at board level
        PCB_FOOTPRINT_T,
        PCB_ZONE_T,
        EOT
    };

    INSPECTOR_FUNC inspector = [&] ( EDA_ITEM* item, void* testData )
    {
        BOARD_ITEM* brd_item = (BOARD_ITEM*) item;

        // aMoveVector was snapshotted, don't need "data".
        brd_item->Move( aMoveVector );

        return SEARCH_RESULT::CONTINUE;
    };

    Visit( inspector, NULL, top_level_board_stuff );
}


TRACKS BOARD::TracksInNet( int aNetCode )
{
    TRACKS ret;

    INSPECTOR_FUNC inspector = [aNetCode, &ret]( EDA_ITEM* item, void* testData )
                               {
                                   TRACK* t = static_cast<TRACK*>( item );

                                   if( t->GetNetCode() == aNetCode )
                                       ret.push_back( t );

                                   return SEARCH_RESULT::CONTINUE;
                               };

    // visit this BOARD's TRACKs and VIAs with above TRACK INSPECTOR which
    // appends all in aNetCode to ret.
    Visit( inspector, NULL, GENERAL_COLLECTOR::Tracks );

    return ret;
}


bool BOARD::SetLayerDescr( PCB_LAYER_ID aIndex, const LAYER& aLayer )
{
    if( unsigned( aIndex ) < arrayDim( m_layers ) )
    {
        m_layers[ aIndex ] = aLayer;
        return true;
    }

    return false;
}


const PCB_LAYER_ID BOARD::GetLayerID( const wxString& aLayerName ) const
{

    // Check the BOARD physical layer names.
    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if ( ( m_layers[ layer ].m_name == aLayerName )
                || ( m_layers[ layer ].m_userName == aLayerName ) )
            return ToLAYER_ID( layer );
    }

    // Otherwise fall back to the system standard layer names for virtual layers.
    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( GetStandardLayerName( ToLAYER_ID( layer ) ) == aLayerName )
            return ToLAYER_ID( layer );
    }

    return UNDEFINED_LAYER;
}


const wxString BOARD::GetLayerName( PCB_LAYER_ID aLayer ) const
{
    // All layer names are stored in the BOARD.
    if( IsLayerEnabled( aLayer ) )
    {
        // Standard names were set in BOARD::BOARD() but they may be over-ridden by
        // BOARD::SetLayerName().  For copper layers, return the user defined layer name,
        // if it was set.  Otherwise return the Standard English layer name.
        if( !m_layers[aLayer].m_userName.IsEmpty() )
            return m_layers[aLayer].m_userName;
    }

    return GetStandardLayerName( aLayer );
}


bool BOARD::SetLayerName( PCB_LAYER_ID aLayer, const wxString& aLayerName )
{
    wxCHECK( !aLayerName.IsEmpty(), false );

    // no quote chars in the name allowed
    if( aLayerName.Find( wxChar( '"' ) ) != wxNOT_FOUND )
        return false;

    if( IsLayerEnabled( aLayer ) )
    {
        m_layers[aLayer].m_userName = aLayerName;
        return true;
    }

    return false;
}


LAYER_T BOARD::GetLayerType( PCB_LAYER_ID aLayer ) const
{
    if( !IsCopperLayer( aLayer ) )
        return LT_SIGNAL;

    //@@IMB: The original test was broken due to the discontinuity
    // in the layer sequence.
    if( IsLayerEnabled( aLayer ) )
        return m_layers[aLayer].m_type;

    return LT_SIGNAL;
}


bool BOARD::SetLayerType( PCB_LAYER_ID aLayer, LAYER_T aLayerType )
{
    if( !IsCopperLayer( aLayer ) )
        return false;

    //@@IMB: The original test was broken due to the discontinuity
    // in the layer sequence.
    if( IsLayerEnabled( aLayer ) )
    {
        m_layers[aLayer].m_type = aLayerType;
        return true;
    }

    return false;
}


const char* LAYER::ShowType( LAYER_T aType )
{
    switch( aType )
    {
    default:
    case LT_SIGNAL: return "signal";
    case LT_POWER:  return "power";
    case LT_MIXED:  return "mixed";
    case LT_JUMPER: return "jumper";
    }
}


LAYER_T LAYER::ParseType( const char* aType )
{
    if( strcmp( aType, "signal" ) == 0 )
        return LT_SIGNAL;
    else if( strcmp( aType, "power" ) == 0 )
        return LT_POWER;
    else if( strcmp( aType, "mixed" ) == 0 )
        return LT_MIXED;
    else if( strcmp( aType, "jumper" ) == 0 )
        return LT_JUMPER;
    else
        return LT_UNDEFINED;
}


int BOARD::GetCopperLayerCount() const
{
    return GetDesignSettings().GetCopperLayerCount();
}


void BOARD::SetCopperLayerCount( int aCount )
{
    GetDesignSettings().SetCopperLayerCount( aCount );
}


LSET BOARD::GetEnabledLayers() const
{
    return GetDesignSettings().GetEnabledLayers();
}


bool BOARD::IsLayerVisible( PCB_LAYER_ID aLayer ) const
{
    // If there is no project, assume layer is visible always
    return GetDesignSettings().IsLayerEnabled( aLayer )
           && ( !m_project || m_project->GetLocalSettings().m_VisibleLayers[aLayer] );
}


LSET BOARD::GetVisibleLayers() const
{
    return m_project ? m_project->GetLocalSettings().m_VisibleLayers : LSET::AllLayersMask();
}


void BOARD::SetEnabledLayers( LSET aLayerSet )
{
    GetDesignSettings().SetEnabledLayers( aLayerSet );
}


void BOARD::SetVisibleLayers( LSET aLayerSet )
{
    if( m_project )
        m_project->GetLocalSettings().m_VisibleLayers = aLayerSet;
}


void BOARD::SetVisibleElements( const GAL_SET& aSet )
{
    // Call SetElementVisibility for each item
    // to ensure specific calculations that can be needed by some items,
    // just changing the visibility flags could be not sufficient.
    for( size_t i = 0; i < aSet.size(); i++ )
        SetElementVisibility( GAL_LAYER_ID_START + static_cast<int>( i ), aSet[i] );
}


void BOARD::SetVisibleAlls()
{
    SetVisibleLayers( LSET().set() );

    // Call SetElementVisibility for each item,
    // to ensure specific calculations that can be needed by some items
    for( GAL_LAYER_ID ii = GAL_LAYER_ID_START; ii < GAL_LAYER_ID_BITMASK_END; ++ii )
        SetElementVisibility( ii, true );
}


GAL_SET BOARD::GetVisibleElements() const
{
    return m_project ? m_project->GetLocalSettings().m_VisibleItems : GAL_SET().set();
}


bool BOARD::IsElementVisible( GAL_LAYER_ID aLayer ) const
{
    return !m_project || m_project->GetLocalSettings().m_VisibleItems[aLayer - GAL_LAYER_ID_START];
}


void BOARD::SetElementVisibility( GAL_LAYER_ID aLayer, bool isEnabled )
{
    if( m_project )
        m_project->GetLocalSettings().m_VisibleItems.set( aLayer - GAL_LAYER_ID_START, isEnabled );

    switch( aLayer )
    {
    case LAYER_RATSNEST:
    {
        // because we have a tool to show/hide ratsnest relative to a pad or a footprint
        // so the hide/show option is a per item selection

        for( TRACK* track : Tracks() )
            track->SetLocalRatsnestVisible( isEnabled );

        for( FOOTPRINT* footprint : Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
                pad->SetLocalRatsnestVisible( isEnabled );
        }

        for( ZONE* zone : Zones() )
            zone->SetLocalRatsnestVisible( isEnabled );

        break;
    }

    default:
        ;
    }
}


bool BOARD::IsFootprintLayerVisible( PCB_LAYER_ID aLayer ) const
{
    switch( aLayer )
    {
    case F_Cu:
        return IsElementVisible( LAYER_MOD_FR );

    case B_Cu:
        return IsElementVisible( LAYER_MOD_BK );

    default:
        wxFAIL_MSG( wxT( "BOARD::IsModuleLayerVisible() param error: bad layer" ) );
        return true;
    }
}


void BOARD::Add( BOARD_ITEM* aBoardItem, ADD_MODE aMode )
{
    if( aBoardItem == NULL )
    {
        wxFAIL_MSG( wxT( "BOARD::Add() param error: aBoardItem NULL" ) );
        return;
    }

    switch( aBoardItem->Type() )
    {
    case PCB_NETINFO_T:
        m_NetInfo.AppendNet( (NETINFO_ITEM*) aBoardItem );
        break;

    // this one uses a vector
    case PCB_MARKER_T:
        m_markers.push_back( (PCB_MARKER*) aBoardItem );
        break;

    // this one uses a vector
    case PCB_GROUP_T:
        m_groups.push_back( (PCB_GROUP*) aBoardItem );
        break;

    // this one uses a vector
    case PCB_ZONE_T:
        m_zones.push_back( (ZONE*) aBoardItem );
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
    case PCB_ARC_T:

        // N.B. This inserts a small memory leak as we lose the
        if( !IsCopperLayer( aBoardItem->GetLayer() ) )
        {
            wxFAIL_MSG( wxT( "BOARD::Add() Cannot place Track on non-copper layer" ) );
            return;
        }

        if( aMode == ADD_MODE::APPEND || aMode == ADD_MODE::BULK_APPEND )
            m_tracks.push_back( static_cast<TRACK*>( aBoardItem ) );
        else
            m_tracks.push_front( static_cast<TRACK*>( aBoardItem ) );

        break;

    case PCB_FOOTPRINT_T:
        if( aMode == ADD_MODE::APPEND || aMode == ADD_MODE::BULK_APPEND )
            m_footprints.push_back( static_cast<FOOTPRINT*>( aBoardItem ) );
        else
            m_footprints.push_front( static_cast<FOOTPRINT*>( aBoardItem ) );

        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_SHAPE_T:
    case PCB_TEXT_T:
    case PCB_TARGET_T:
        if( aMode == ADD_MODE::APPEND || aMode == ADD_MODE::BULK_APPEND )
            m_drawings.push_back( aBoardItem );
        else
            m_drawings.push_front( aBoardItem );

        break;

    // other types may use linked list
    default:
        {
            wxString msg;
            msg.Printf( wxT( "BOARD::Add() needs work: BOARD_ITEM type (%d) not handled" ),
                        aBoardItem->Type() );
            wxFAIL_MSG( msg );
            return;
        }
        break;
    }

    aBoardItem->SetParent( this );
    aBoardItem->ClearEditFlags();
    m_connectivity->Add( aBoardItem );

    if( aMode != ADD_MODE::BULK_INSERT && aMode != ADD_MODE::BULK_APPEND )
        InvokeListeners( &BOARD_LISTENER::OnBoardItemAdded, *this, aBoardItem );
}


void BOARD::FinalizeBulkAdd( std::vector<BOARD_ITEM*>& aNewItems )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardItemsAdded, *this, aNewItems );
}


void BOARD::FinalizeBulkRemove( std::vector<BOARD_ITEM*>& aRemovedItems )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardItemsRemoved, *this, aRemovedItems );
}


void BOARD::Remove( BOARD_ITEM* aBoardItem, REMOVE_MODE aRemoveMode )
{
    // find these calls and fix them!  Don't send me no stinking' NULL.
    wxASSERT( aBoardItem );

    switch( aBoardItem->Type() )
    {
    case PCB_NETINFO_T:
    {
        NETINFO_ITEM* item = (NETINFO_ITEM*) aBoardItem;
        m_NetInfo.RemoveNet( item );
        break;
    }

    case PCB_MARKER_T:
        m_markers.erase( std::remove_if( m_markers.begin(), m_markers.end(),
                                         [aBoardItem]( BOARD_ITEM* aItem )
                                         {
                                             return aItem == aBoardItem;
                                         } ) );
        break;

    case PCB_GROUP_T:
        m_groups.erase( std::remove_if( m_groups.begin(), m_groups.end(),
                                        [aBoardItem]( BOARD_ITEM* aItem )
                                        {
                                            return aItem == aBoardItem;
                                        } ) );
        break;

    case PCB_ZONE_T:
        m_zones.erase( std::remove_if( m_zones.begin(), m_zones.end(),
                                       [aBoardItem]( BOARD_ITEM* aItem )
                                       {
                                           return aItem == aBoardItem;
                                       } ) );
        break;

    case PCB_FOOTPRINT_T:
        m_footprints.erase( std::remove_if( m_footprints.begin(), m_footprints.end(),
                                            [aBoardItem]( BOARD_ITEM* aItem )
                                            {
                                                return aItem == aBoardItem;
                                            } ) );
        break;

    case PCB_TRACE_T:
    case PCB_ARC_T:
    case PCB_VIA_T:
        m_tracks.erase( std::remove_if( m_tracks.begin(), m_tracks.end(),
                                        [aBoardItem]( BOARD_ITEM* aItem )
                                        {
                                            return aItem == aBoardItem;
                                        } ) );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_SHAPE_T:
    case PCB_TEXT_T:
    case PCB_TARGET_T:
        m_drawings.erase( std::remove_if( m_drawings.begin(), m_drawings.end(),
                                          [aBoardItem](BOARD_ITEM* aItem)
                                          {
                                              return aItem == aBoardItem;
                                          } ) );
        break;

    // other types may use linked list
    default:
        wxFAIL_MSG( wxT( "BOARD::Remove() needs more ::Type() support" ) );
    }

    aBoardItem->SetFlags( STRUCT_DELETED );

    PCB_GROUP* parentGroup = aBoardItem->GetParentGroup();

    if( parentGroup && !( parentGroup->GetFlags() & STRUCT_DELETED ) )
        parentGroup->RemoveItem( aBoardItem );

    m_connectivity->Remove( aBoardItem );

    if( aRemoveMode != REMOVE_MODE::BULK )
        InvokeListeners( &BOARD_LISTENER::OnBoardItemRemoved, *this, aBoardItem );
}


wxString BOARD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "PCB" ) );
}


void BOARD::DeleteMARKERs()
{
    // the vector does not know how to delete the PCB_MARKER, it holds pointers
    for( PCB_MARKER* marker : m_markers )
        delete marker;

    m_markers.clear();
}


void BOARD::DeleteMARKERs( bool aWarningsAndErrors, bool aExclusions )
{
    // Deleting lots of items from a vector can be very slow.  Copy remaining items instead.
    MARKERS remaining;

    for( PCB_MARKER* marker : m_markers )
    {
        if( ( marker->IsExcluded() && aExclusions )
                || ( !marker->IsExcluded() && aWarningsAndErrors ) )
        {
            delete marker;
        }
        else
        {
            remaining.push_back( marker );
        }
    }

    m_markers = remaining;
}


BOARD_ITEM* BOARD::GetItem( const KIID& aID ) const
{
    if( aID == niluuid )
        return nullptr;

    for( TRACK* track : Tracks() )
    {
        if( track->m_Uuid == aID )
            return track;
    }

    for( FOOTPRINT* footprint : Footprints() )
    {
        if( footprint->m_Uuid == aID )
            return footprint;

        for( PAD* pad : footprint->Pads() )
        {
            if( pad->m_Uuid == aID )
                return pad;
        }

        if( footprint->Reference().m_Uuid == aID )
            return &footprint->Reference();

        if( footprint->Value().m_Uuid == aID )
            return &footprint->Value();

        for( BOARD_ITEM* drawing : footprint->GraphicalItems() )
        {
            if( drawing->m_Uuid == aID )
                return drawing;
        }

        for( BOARD_ITEM* zone : footprint->Zones() )
        {
            if( zone->m_Uuid == aID )
                return zone;
        }

        for( PCB_GROUP* group : footprint->Groups() )
        {
            if( group->m_Uuid == aID )
                return group;
        }
    }

    for( ZONE* zone : Zones() )
    {
        if( zone->m_Uuid == aID )
            return zone;
    }

    for( BOARD_ITEM* drawing : Drawings() )
    {
        if( drawing->m_Uuid == aID )
            return drawing;
    }

    for( PCB_MARKER* marker : m_markers )
    {
        if( marker->m_Uuid == aID )
            return marker;
    }

    for( PCB_GROUP* group : m_groups )
    {
        if( group->m_Uuid == aID )
            return group;
    }

    if( m_Uuid == aID )
        return const_cast<BOARD*>( this );

    // Not found; weak reference has been deleted.
    return DELETED_BOARD_ITEM::GetInstance();
}


void BOARD::FillItemMap( std::map<KIID, EDA_ITEM*>& aMap )
{
    // the board itself
    aMap[ m_Uuid ] = this;

    for( TRACK* track : Tracks() )
        aMap[ track->m_Uuid ] = track;

    for( FOOTPRINT* footprint : Footprints() )
    {
        aMap[ footprint->m_Uuid ] = footprint;

        for( PAD* pad : footprint->Pads() )
            aMap[ pad->m_Uuid ] = pad;

        aMap[ footprint->Reference().m_Uuid ] = &footprint->Reference();
        aMap[ footprint->Value().m_Uuid ] = &footprint->Value();

        for( BOARD_ITEM* drawing : footprint->GraphicalItems() )
            aMap[ drawing->m_Uuid ] = drawing;
    }

    for( ZONE* zone : Zones() )
        aMap[ zone->m_Uuid ] = zone;

    for( BOARD_ITEM* drawing : Drawings() )
        aMap[ drawing->m_Uuid ] = drawing;

    for( PCB_MARKER* marker : m_markers )
        aMap[ marker->m_Uuid ] = marker;

    for( PCB_GROUP* group : m_groups )
        aMap[ group->m_Uuid ] = group;
}


wxString BOARD::ConvertCrossReferencesToKIIDs( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                wxString remainder;
                wxString ref = token.BeforeFirst( ':', &remainder );

                for( const FOOTPRINT* footprint : Footprints() )
                {
                    if( footprint->GetReference().CmpNoCase( ref ) == 0 )
                    {
                        wxString test( remainder );

                        if( footprint->ResolveTextVar( &test ) )
                            token = footprint->m_Uuid.AsString() + ":" + remainder;

                        break;
                    }
                }
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


wxString BOARD::ConvertKIIDsToCrossReferences( const wxString& aSource ) const
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( aSource[i] == '$' && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            wxString token;
            bool     isCrossRef = false;

            for( i = i + 2; i < sourceLen; ++i )
            {
                if( aSource[i] == '}' )
                    break;

                if( aSource[i] == ':' )
                    isCrossRef = true;

                token.append( aSource[i] );
            }

            if( isCrossRef )
            {
                wxString      remainder;
                wxString      ref = token.BeforeFirst( ':', &remainder );
                BOARD_ITEM*   refItem = GetItem( KIID( ref ) );

                if( refItem && refItem->Type() == PCB_FOOTPRINT_T )
                    token = static_cast<FOOTPRINT*>( refItem )->GetReference() + ":" + remainder;
            }

            newbuf.append( "${" + token + "}" );
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


unsigned BOARD::GetNodesCount( int aNet ) const
{
    unsigned retval = 0;

    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( ( aNet == -1 && pad->GetNetCode() > 0 ) || aNet == pad->GetNetCode() )
                retval++;
        }
    }

    return retval;
}


unsigned BOARD::GetUnconnectedNetCount() const
{
    return m_connectivity->GetUnconnectedCount();
}


EDA_RECT BOARD::ComputeBoundingBox( bool aBoardEdgesOnly ) const
{
    EDA_RECT area;
    LSET     visible = GetVisibleLayers();
    bool     showInvisibleText = IsElementVisible( LAYER_MOD_TEXT_INVISIBLE )
                                 && PgmOrNull() && !PgmOrNull()->m_Printing;

    if( aBoardEdgesOnly )
        visible.set( Edge_Cuts );

    // Check shapes, dimensions, texts, and fiducials
    for( BOARD_ITEM* item : m_drawings )
    {
        if( aBoardEdgesOnly && ( item->GetLayer() != Edge_Cuts || item->Type() != PCB_SHAPE_T ) )
            continue;

        if( ( item->GetLayerSet() & visible ).any() )
            area.Merge( item->GetBoundingBox() );
    }

    // Check footprints
    for( FOOTPRINT* footprint : m_footprints )
    {
        if( !( footprint->GetLayerSet() & visible ).any() )
            continue;

        if( aBoardEdgesOnly )
        {
            for( const BOARD_ITEM* edge : footprint->GraphicalItems() )
            {
                if( edge->GetLayer() == Edge_Cuts  && edge->Type() == PCB_FP_SHAPE_T )
                    area.Merge( edge->GetBoundingBox() );
            }
        }
        else
        {
            area.Merge( footprint->GetBoundingBox( true, showInvisibleText ) );
        }
    }

    if( !aBoardEdgesOnly )
    {
        // Check tracks
        for( TRACK* track : m_tracks )
        {
            if( ( track->GetLayerSet() & visible ).any() )
                area.Merge( track->GetBoundingBox() );
        }

        // Check zones
        for( ZONE* aZone : m_zones )
        {
            if( ( aZone->GetLayerSet() & visible ).any() )
                area.Merge( aZone->GetBoundingBox() );
        }
    }

    return area;
}


void BOARD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString txt;
    int      viasCount = 0;
    int      trackSegmentsCount = 0;

    for( TRACK* item : m_tracks )
    {
        if( item->Type() == PCB_VIA_T )
            viasCount++;
        else
            trackSegmentsCount++;
    }

    txt.Printf( wxT( "%d" ), GetPadCount() );
    aList.emplace_back( _( "Pads" ), txt );

    txt.Printf( wxT( "%d" ), viasCount );
    aList.emplace_back( _( "Vias" ), txt );

    txt.Printf( wxT( "%d" ), trackSegmentsCount );
    aList.emplace_back( _( "Track Segments" ), txt );

    txt.Printf( wxT( "%d" ), GetNodesCount() );
    aList.emplace_back( _( "Nodes" ), txt );

    txt.Printf( wxT( "%d" ), m_NetInfo.GetNetCount() - 1 /* Don't include "No Net" in count */ );
    aList.emplace_back( _( "Nets" ), txt );

    txt.Printf( wxT( "%d" ), GetConnectivity()->GetUnconnectedCount() );
    aList.emplace_back( _( "Unrouted" ), txt );
}


SEARCH_RESULT BOARD::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_RESULT::CONTINUE;
    const KICAD_T* p      = scanTypes;
    bool           done   = false;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    while( !done )
    {
        stype = *p;

        switch( stype )
        {
        case PCB_T:
            result = inspector( this, testData );  // inspect me
            // skip over any types handled in the above call.
            ++p;
            break;

        /*
         * Instances of the requested KICAD_T live in a list, either one that I manage, or one
         * that my footprints manage.  If it's a type managed by class FOOTPRINT, then simply
         * pass it on to each footprint's Visit() function via IterateForward( m_footprints, ... ).
         */

        case PCB_FOOTPRINT_T:
        case PCB_PAD_T:
        case PCB_FP_TEXT_T:
        case PCB_FP_SHAPE_T:
        case PCB_FP_ZONE_T:

            // this calls FOOTPRINT::Visit() on each footprint.
            result = IterateForward<FOOTPRINT*>( m_footprints, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_FOOTPRINT_T:
                case PCB_PAD_T:
                case PCB_FP_TEXT_T:
                case PCB_FP_SHAPE_T:
                case PCB_FP_ZONE_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        case PCB_SHAPE_T:
        case PCB_TEXT_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
        case PCB_TARGET_T:
            result = IterateForward<BOARD_ITEM*>( m_drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_SHAPE_T:
                case PCB_TEXT_T:
                case PCB_DIM_ALIGNED_T:
                case PCB_DIM_CENTER_T:
                case PCB_DIM_ORTHOGONAL_T:
                case PCB_DIM_LEADER_T:
                case PCB_TARGET_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        case PCB_VIA_T:
            result = IterateForward<TRACK*>( m_tracks, inspector, testData, p );
            ++p;
            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
            result = IterateForward<TRACK*>( m_tracks, inspector, testData, p );
            ++p;
            break;

        case PCB_MARKER_T:
            for( PCB_MARKER* marker : m_markers )
            {
                result = marker->Visit( inspector, testData, p );

                if( result == SEARCH_RESULT::QUIT )
                    break;
            }

            ++p;
            break;

        case PCB_ZONE_T:
            for( ZONE* zone : m_zones)
            {
                result = zone->Visit( inspector, testData, p );

                if( result == SEARCH_RESULT::QUIT )
                    break;
            }

            ++p;
            break;

        case PCB_GROUP_T:
            result = IterateForward<PCB_GROUP*>( m_groups, inspector, testData, p );
            ++p;
            break;

        default:        // catch EOT or ANY OTHER type here and return.
            done = true;
            break;
        }

        if( result == SEARCH_RESULT::QUIT )
            break;
    }

    return result;
}


NETINFO_ITEM* BOARD::FindNet( int aNetcode ) const
{
    // the first valid netcode is 1 and the last is m_NetInfo.GetCount()-1.
    // zero is reserved for "no connection" and is not actually a net.
    // NULL is returned for non valid netcodes

    wxASSERT( m_NetInfo.GetNetCount() > 0 );

    if( aNetcode == NETINFO_LIST::UNCONNECTED && m_NetInfo.GetNetCount() == 0 )
        return NETINFO_LIST::OrphanedItem();
    else
        return m_NetInfo.GetNetItem( aNetcode );
}


NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname ) const
{
    return m_NetInfo.GetNetItem( aNetname );
}


FOOTPRINT* BOARD::FindFootprintByReference( const wxString& aReference ) const
{
    for( FOOTPRINT* footprint : m_footprints )
    {
        if( aReference == footprint->GetReference() )
            return footprint;
    }

    return nullptr;
}


FOOTPRINT* BOARD::FindFootprintByPath( const KIID_PATH& aPath ) const
{
    for( FOOTPRINT* footprint : m_footprints )
    {
        if( footprint->GetPath() == aPath )
            return footprint;
    }

    return nullptr;
}


// The pad count for each netcode, stored in a buffer for a fast access.
// This is needed by the sort function sortNetsByNodes()
static std::vector<int> padCountListByNet;


// Sort nets by decreasing pad count.
// For same pad count, sort by alphabetic names
static bool sortNetsByNodes( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
{
    int countA = padCountListByNet[ a->GetNetCode() ];
    int countB = padCountListByNet[ b->GetNetCode() ];

    if( countA == countB )
        return a->GetNetname() < b->GetNetname();
    else
        return countB < countA;
}


// Sort nets by alphabetic names
static bool sortNetsByNames( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
{
    return a->GetNetname() < b->GetNetname();
}


int BOARD::SortedNetnamesList( wxArrayString& aNames, bool aSortbyPadsCount )
{
    if( m_NetInfo.GetNetCount() == 0 )
        return 0;

    // Build the list
    std::vector <NETINFO_ITEM*> netBuffer;

    netBuffer.reserve( m_NetInfo.GetNetCount() );
    int max_netcode = 0;

    for( NETINFO_ITEM* net : m_NetInfo )
    {
        int netcode = net->GetNetCode();

        if( netcode > 0 && net->IsCurrent() )
        {
            netBuffer.push_back( net );
            max_netcode = std::max( netcode, max_netcode);
        }
    }

    // sort the list
    if( aSortbyPadsCount )
    {
        // Build the pad count by net:
        padCountListByNet.clear();
        std::vector<PAD*> pads = GetPads();

        padCountListByNet.assign( max_netcode + 1, 0 );

        for( PAD* pad : pads )
        {
            int netCode = pad->GetNetCode();

            if( netCode >= 0 )
                padCountListByNet[ netCode ]++;
        }

        sort( netBuffer.begin(), netBuffer.end(), sortNetsByNodes );
    }
    else
    {
        sort( netBuffer.begin(), netBuffer.end(), sortNetsByNames );
    }

    for( NETINFO_ITEM* net : netBuffer )
        aNames.Add( UnescapeString( net->GetNetname() ) );

    return netBuffer.size();
}


std::vector<wxString> BOARD::GetNetClassAssignmentCandidates() const
{
    std::vector<wxString> names;

    for( const NETINFO_ITEM* net : m_NetInfo )
    {
        if( !net->GetNetname().IsEmpty() )
            names.emplace_back( net->GetNetname() );
    }

    return names;
}


void BOARD::SynchronizeProperties()
{
    if( m_project )
        SetProperties( m_project->GetTextVars() );
}


void BOARD::SynchronizeNetsAndNetClasses()
{
    if( !m_project )
        return;

    NET_SETTINGS* netSettings     = m_project->GetProjectFile().m_NetSettings.get();
    NETCLASSES&   netClasses      = netSettings->m_NetClasses;
    NETCLASSPTR   defaultNetClass = netClasses.GetDefault();

    for( NETINFO_ITEM* net : m_NetInfo )
    {
        const wxString& netname = net->GetNetname();
        const wxString& netclassName = netSettings->GetNetclassName( netname );

        net->SetNetClass( netClasses.Find( netclassName ) );
    }

    BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();

    // Set initial values for custom track width & via size to match the default
    // netclass settings
    bds.UseCustomTrackViaSize( false );
    bds.SetCustomTrackWidth( defaultNetClass->GetTrackWidth() );
    bds.SetCustomViaSize( defaultNetClass->GetViaDiameter() );
    bds.SetCustomViaDrill( defaultNetClass->GetViaDrill() );
    bds.SetCustomDiffPairWidth( defaultNetClass->GetDiffPairWidth() );
    bds.SetCustomDiffPairGap( defaultNetClass->GetDiffPairGap() );
    bds.SetCustomDiffPairViaGap( defaultNetClass->GetDiffPairViaGap() );

    InvokeListeners( &BOARD_LISTENER::OnBoardNetSettingsChanged, *this );
}


int BOARD::SetAreasNetCodesFromNetNames()
{
    int error_count = 0;

    for( ZONE* zone : Zones() )
    {
        if( !zone->IsOnCopperLayer() )
        {
            zone->SetNetCode( NETINFO_LIST::UNCONNECTED );
            continue;
        }

        if( zone->GetNetCode() != 0 )      // i.e. if this zone is connected to a net
        {
            const NETINFO_ITEM* net = zone->GetNet();

            if( net )
            {
                zone->SetNetCode( net->GetNetCode() );
            }
            else
            {
                error_count++;

                // keep Net Name and set m_NetCode to -1 : error flag.
                zone->SetNetCode( -1 );
            }
        }
    }

    return error_count;
}


PAD* BOARD::GetPad( const wxPoint& aPosition, LSET aLayerSet ) const
{
    if( !aLayerSet.any() )
        aLayerSet = LSET::AllCuMask();

    for( FOOTPRINT* footprint : m_footprints )
    {
        PAD* pad = NULL;

        if( footprint->HitTest( aPosition ) )
            pad = footprint->GetPad( aPosition, aLayerSet );

        if( pad )
            return pad;
    }

    return NULL;
}


PAD* BOARD::GetPad( const TRACK* aTrace, ENDPOINT_T aEndPoint ) const
{
    const wxPoint& aPosition = aTrace->GetEndPoint( aEndPoint );

    LSET lset( aTrace->GetLayer() );

    return GetPad( aPosition, lset );
}


PAD* BOARD::GetPadFast( const wxPoint& aPosition, LSET aLayerSet ) const
{
    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
        if( pad->GetPosition() != aPosition )
            continue;

        // Pad found, it must be on the correct layer
        if( ( pad->GetLayerSet() & aLayerSet ).any() )
            return pad;
        }
    }

    return nullptr;
}


PAD* BOARD::GetPad( std::vector<PAD*>& aPadList, const wxPoint& aPosition, LSET aLayerSet ) const
{
    // Search aPadList for aPosition
    // aPadList is sorted by X then Y values, and a fast binary search is used
    int idxmax = aPadList.size() - 1;

    int delta = aPadList.size();

    int idx = 0;        // Starting index is the beginning of list

    while( delta )
    {
        // Calculate half size of remaining interval to test.
        // Ensure the computed value is not truncated (too small)
        if( (delta & 1) && ( delta > 1 ) )
            delta++;

        delta /= 2;

        PAD* pad = aPadList[idx];

        if( pad->GetPosition() == aPosition )       // candidate found
        {
            // The pad must match the layer mask:
            if( ( aLayerSet & pad->GetLayerSet() ).any() )
                return pad;

            // More than one pad can be at aPosition
            // search for a pad at aPosition that matched this mask

            // search next
            for( int ii = idx+1; ii <= idxmax; ii++ )
            {
                pad = aPadList[ii];

                if( pad->GetPosition() != aPosition )
                    break;

                if( ( aLayerSet & pad->GetLayerSet() ).any() )
                    return pad;
            }
            // search previous
            for( int ii = idx - 1 ;ii >=0; ii-- )
            {
                pad = aPadList[ii];

                if( pad->GetPosition() != aPosition )
                    break;

                if( ( aLayerSet & pad->GetLayerSet() ).any() )
                    return pad;
            }

            // Not found:
            return 0;
        }

        if( pad->GetPosition().x == aPosition.x )       // Must search considering Y coordinate
        {
            if( pad->GetPosition().y < aPosition.y )    // Must search after this item
            {
                idx += delta;

                if( idx > idxmax )
                    idx = idxmax;
            }
            else // Must search before this item
            {
                idx -= delta;

                if( idx < 0 )
                    idx = 0;
            }
        }
        else if( pad->GetPosition().x < aPosition.x ) // Must search after this item
        {
            idx += delta;

            if( idx > idxmax )
                idx = idxmax;
        }
        else // Must search before this item
        {
            idx -= delta;

            if( idx < 0 )
                idx = 0;
        }
    }

    return NULL;
}


/**
 * Used by #GetSortedPadListByXCoord to sort a pad list by X coordinate value.
 *
 * This function is used to build ordered pads lists
 */
bool sortPadsByXthenYCoord( PAD* const & aLH, PAD* const & aRH )
{
    if( aLH->GetPosition().x == aRH->GetPosition().x )
        return aLH->GetPosition().y < aRH->GetPosition().y;

    return aLH->GetPosition().x < aRH->GetPosition().x;
}


void BOARD::GetSortedPadListByXthenYCoord( std::vector<PAD*>& aVector, int aNetCode ) const
{
    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads( ) )
        {
            if( aNetCode < 0 || pad->GetNetCode() == aNetCode )
                aVector.push_back( pad );
        }
    }

    std::sort( aVector.begin(), aVector.end(), sortPadsByXthenYCoord );
}


void BOARD::PadDelete( PAD* aPad )
{
    GetConnectivity()->Remove( aPad );

    InvokeListeners( &BOARD_LISTENER::OnBoardItemRemoved, *this, aPad );

    aPad->DeleteStructure();
}


std::tuple<int, double, double> BOARD::GetTrackLength( const TRACK& aTrack ) const
{
    int    count = 0;
    double length = 0.0;
    double package_length = 0.0;

    constexpr KICAD_T types[] = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T, EOT };
    auto              connectivity = GetBoard()->GetConnectivity();

    for( BOARD_CONNECTED_ITEM* item : connectivity->GetConnectedItems(
                 static_cast<const BOARD_CONNECTED_ITEM*>( &aTrack ), types ) )
    {
        count++;

        if( TRACK* track = dyn_cast<TRACK*>( item ) )
        {
            bool inPad = false;

            for( auto pad_it : connectivity->GetConnectedPads( item ) )
            {
                PAD* pad = static_cast<PAD*>( pad_it );

                if( pad->HitTest( track->GetStart(), track->GetWidth() / 2 )
                        && pad->HitTest( track->GetEnd(), track->GetWidth() / 2 ) )
                {
                    inPad = true;
                    break;
                }
            }

            if( !inPad )
                length += track->GetLength();
        }
        else if( PAD* pad = dyn_cast<PAD*>( item ) )
        {
            package_length += pad->GetPadToDieLength();
        }
    }

    return std::make_tuple( count, length, package_length );
}


FOOTPRINT* BOARD::GetFootprint( const wxPoint& aPosition, PCB_LAYER_ID aActiveLayer,
                                bool aVisibleOnly, bool aIgnoreLocked ) const
{
    FOOTPRINT* footprint     = NULL;
    FOOTPRINT* alt_footprint = NULL;
    int        min_dim       = 0x7FFFFFFF;
    int        alt_min_dim   = 0x7FFFFFFF;
    bool       current_layer_back = IsBackLayer( aActiveLayer );

    for( FOOTPRINT* candidate : m_footprints )
    {
        // is the ref point within the footprint's bounds?
        if( !candidate->HitTest( aPosition ) )
            continue;

        // if caller wants to ignore locked footprints, and this one is locked, skip it.
        if( aIgnoreLocked && candidate->IsLocked() )
            continue;

        PCB_LAYER_ID layer = candidate->GetLayer();

        // Filter non visible footprints if requested
        if( !aVisibleOnly || IsFootprintLayerVisible( layer ) )
        {
            EDA_RECT bb = candidate->GetBoundingBox( false, false );

            int offx = bb.GetX() + bb.GetWidth() / 2;
            int offy = bb.GetY() + bb.GetHeight() / 2;

            // off x & offy point to the middle of the box.
            int dist = ( aPosition.x - offx ) * ( aPosition.x - offx ) +
                       ( aPosition.y - offy ) * ( aPosition.y - offy );

            if( current_layer_back == IsBackLayer( layer ) )
            {
                if( dist <= min_dim )
                {
                    // better footprint shown on the active side
                    footprint = candidate;
                    min_dim = dist;
                }
            }
            else if( aVisibleOnly && IsFootprintLayerVisible( layer ) )
            {
                if( dist <= alt_min_dim )
                {
                    // better footprint shown on the other side
                    alt_footprint = candidate;
                    alt_min_dim = dist;
                }
            }
        }
    }

    if( footprint )
        return footprint;

    if( alt_footprint)
        return alt_footprint;

    return NULL;
}


std::list<ZONE*> BOARD::GetZoneList( bool aIncludeZonesInFootprints ) const
{
    std::list<ZONE*> zones;

    for( ZONE* zone : Zones() )
        zones.push_back( zone );

    if( aIncludeZonesInFootprints )
    {
        for( FOOTPRINT* footprint : m_footprints )
        {
            for( FP_ZONE* zone : footprint->Zones() )
                zones.push_back( zone );
        }
    }

    return zones;
}


ZONE* BOARD::AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode, PCB_LAYER_ID aLayer,
                      wxPoint aStartPointPosition, ZONE_BORDER_DISPLAY_STYLE aHatch )
{
    ZONE* new_area = new ZONE( this );

    new_area->SetNetCode( aNetcode );
    new_area->SetLayer( aLayer );

    m_zones.push_back( new_area );

    new_area->SetHatchStyle( (ZONE_BORDER_DISPLAY_STYLE) aHatch );

    // Add the first corner to the new zone
    new_area->AppendCorner( aStartPointPosition, -1 );

    if( aNewZonesList )
    {
        ITEM_PICKER picker( nullptr, new_area, UNDO_REDO::NEWITEM );
        aNewZonesList->PushItem( picker );
    }

    return new_area;
}


bool BOARD::NormalizeAreaPolygon( PICKED_ITEMS_LIST * aNewZonesList, ZONE* aCurrArea )
{
    // mark all areas as unmodified except this one, if modified
    for( ZONE* zone : m_zones )
        zone->SetLocalFlags( 0 );

    aCurrArea->SetLocalFlags( 1 );

    if( aCurrArea->Outline()->IsSelfIntersecting() )
    {
        aCurrArea->UnHatchBorder();

        // Normalize copied area and store resulting number of polygons
        int n_poly = aCurrArea->Outline()->NormalizeAreaOutlines();

        // If clipping has created some polygons, we must add these new copper areas.
        if( n_poly > 1 )
        {
            ZONE* NewArea;

            // Move the newly created polygons to new areas, removing them from the current area
            for( int ip = 1; ip < n_poly; ip++ )
            {
                // Create new copper area and copy poly into it
                SHAPE_POLY_SET* new_p = new SHAPE_POLY_SET( aCurrArea->Outline()->UnitSet( ip ) );
                NewArea = AddArea( aNewZonesList, aCurrArea->GetNetCode(), aCurrArea->GetLayer(),
                                   wxPoint(0, 0), aCurrArea->GetHatchStyle() );

                // remove the poly that was automatically created for the new area
                // and replace it with a poly from NormalizeAreaOutlines
                delete NewArea->Outline();
                NewArea->SetOutline( new_p );
                NewArea->HatchBorder();
                NewArea->SetLocalFlags( 1 );
            }

            SHAPE_POLY_SET* new_p = new SHAPE_POLY_SET( aCurrArea->Outline()->UnitSet( 0 ) );
            delete aCurrArea->Outline();
            aCurrArea->SetOutline( new_p );
        }
    }

    aCurrArea->HatchBorder();

    return true;
}


bool BOARD::GetBoardPolygonOutlines( SHAPE_POLY_SET& aOutlines,
                                     OUTLINE_ERROR_HANDLER* aErrorHandler )
{
    int chainingEpsilon = Millimeter2iu( 0.02 );  // max dist from one endPt to next startPt

    bool success = BuildBoardPolygonOutlines( this, aOutlines, GetDesignSettings().m_MaxError,
                                              chainingEpsilon, aErrorHandler );

    // Make polygon strictly simple to avoid issues (especially in 3D viewer)
    aOutlines.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    return success;
}


const std::vector<PAD*> BOARD::GetPads() const
{
    std::vector<PAD*> allPads;

    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            allPads.push_back( pad );
    }

    return allPads;
}


unsigned BOARD::GetPadCount() const
{
    unsigned retval = 0;

    for( FOOTPRINT* footprint : Footprints() )
        retval += footprint->Pads().size();

    return retval;
}


const std::vector<BOARD_CONNECTED_ITEM*> BOARD::AllConnectedItems()
{
    std::vector<BOARD_CONNECTED_ITEM*> items;

    for( TRACK* track : Tracks() )
        items.push_back( track );

    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            items.push_back( pad );
    }

    for( ZONE* zone : Zones() )
        items.push_back( zone );

    return items;
}


void BOARD::ClearAllNetCodes()
{
    for( BOARD_CONNECTED_ITEM* item : AllConnectedItems() )
        item->SetNetCode( 0 );
}


void BOARD::MapNets( const BOARD* aDestBoard )
{
    for( BOARD_CONNECTED_ITEM* item : AllConnectedItems() )
    {
        NETINFO_ITEM* netInfo = aDestBoard->FindNet( item->GetNetname() );

        if( netInfo )
            item->SetNet( netInfo );
        else
            item->SetNetCode( 0 );
    }
}


void BOARD::SanitizeNetcodes()
{
    for ( BOARD_CONNECTED_ITEM* item : AllConnectedItems() )
    {
        if( FindNet( item->GetNetCode() ) == nullptr )
            item->SetNetCode( NETINFO_LIST::ORPHANED );
    }
}


void BOARD::AddListener( BOARD_LISTENER* aListener )
{
    if( !alg::contains( m_listeners, aListener ) )
        m_listeners.push_back( aListener );
}


void BOARD::RemoveListener( BOARD_LISTENER* aListener )
{
    auto i = std::find( m_listeners.begin(), m_listeners.end(), aListener );

    if( i != m_listeners.end() )
    {
        std::iter_swap( i, m_listeners.end() - 1 );
        m_listeners.pop_back();
    }
}


void BOARD::OnItemChanged( BOARD_ITEM* aItem )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardItemChanged, *this, aItem );
}


void BOARD::OnItemsChanged( std::vector<BOARD_ITEM*>& aItems )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardItemsChanged, *this, aItems );
}


void BOARD::ResetNetHighLight()
{
    m_highLight.Clear();
    m_highLightPrevious.Clear();

    InvokeListeners( &BOARD_LISTENER::OnBoardHighlightNetChanged, *this );
}


void BOARD::SetHighLightNet( int aNetCode, bool aMulti )
{
    if( !m_highLight.m_netCodes.count( aNetCode ) )
    {
        if( !aMulti )
            m_highLight.m_netCodes.clear();

        m_highLight.m_netCodes.insert( aNetCode );
        InvokeListeners( &BOARD_LISTENER::OnBoardHighlightNetChanged, *this );
    }
}


void BOARD::HighLightON( bool aValue )
{
    if( m_highLight.m_highLightOn != aValue )
    {
        m_highLight.m_highLightOn = aValue;
        InvokeListeners( &BOARD_LISTENER::OnBoardHighlightNetChanged, *this );
    }
}


wxString BOARD::GroupsSanityCheck( bool repair )
{
    if( repair )
    {
        while( GroupsSanityCheckInternal( repair ) != wxEmptyString )
        {};

        return wxEmptyString;
    }
    return GroupsSanityCheckInternal( repair );
}


wxString BOARD::GroupsSanityCheckInternal( bool repair )
{
    // Cycle detection
    //
    // Each group has at most one parent group.
    // So we start at group 0 and traverse the parent chain, marking groups seen along the way.
    // If we ever see a group that we've already marked, that's a cycle.
    // If we reach the end of the chain, we know all groups in that chain are not part of any cycle.
    //
    // Algorithm below is linear in the # of groups because each group is visited only once.
    // There may be extra time taken due to the container access calls and iterators.
    //
    // Groups we know are cycle free
    std::unordered_set<PCB_GROUP*> knownCycleFreeGroups;
    // Groups in the current chain we're exploring.
    std::unordered_set<PCB_GROUP*> currentChainGroups;
    // Groups we haven't checked yet.
    std::unordered_set<PCB_GROUP*> toCheckGroups;

    // Initialize set of groups to check that could participate in a cycle.
    for( PCB_GROUP* group : Groups() )
        toCheckGroups.insert( group);

    while( !toCheckGroups.empty() )
    {
        currentChainGroups.clear();
        PCB_GROUP* group = *toCheckGroups.begin();

        while( true )
        {
            if( currentChainGroups.find( group ) != currentChainGroups.end() )
            {
                if( repair )
                    Remove( group );

                return "Cycle detected in group membership";
            }
            else if( knownCycleFreeGroups.find( group ) != knownCycleFreeGroups.end() )
            {
                // Parent is a group we know does not lead to a cycle
                break;
            }

            currentChainGroups.insert( group );
            // We haven't visited currIdx yet, so it must be in toCheckGroups
            toCheckGroups.erase( group );

            group = group->GetParentGroup();

            if( !group )
            {
                // end of chain and no cycles found in this chain
                break;
            }
        }

        // No cycles found in chain, so add it to set of groups we know don't participate
        // in a cycle.
        knownCycleFreeGroups.insert( currentChainGroups.begin(), currentChainGroups.end() );
    }

    // Success
    return "";
}


BOARD::GroupLegalOpsField BOARD::GroupLegalOps( const PCB_SELECTION& selection ) const
{
    bool hasGroup = false;
    bool hasMember = false;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_GROUP_T )
            hasGroup = true;

        if( item->GetParent() && item->GetParent()->Type() == PCB_GROUP_T )
            hasMember = true;
    }

    GroupLegalOpsField legalOps;

    legalOps.create      = true;
    legalOps.removeItems = hasMember;
    legalOps.ungroup     = hasGroup;
    legalOps.enter       = hasGroup && selection.Size() == 1;

    return legalOps;
}
