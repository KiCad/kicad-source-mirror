/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 *
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <iterator>

#include <wx/log.h>

#include <drc/drc_rtree.h>
#include <board_design_settings.h>
#include <board_commit.h>
#include <board.h>
#include <core/arraydim.h>
#include <core/kicad_algo.h>
#include <connectivity/connectivity_data.h>
#include <convert_shape_list_to_polygon.h>
#include <footprint.h>
#include <lset.h>
#include <pcb_base_frame.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <pcb_target.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_dimension.h>
#include <pgm_base.h>
#include <pcbnew_settings.h>
#include <progress_reporter.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <ratsnest/ratsnest_data.h>
#include <reporter.h>
#include <tool/tool_manager.h>
#include <tool/selection_conditions.h>
#include <string_utils.h>
#include <core/thread_pool.h>
#include <zone.h>
#include <mutex>

// This is an odd place for this, but CvPcb won't link if it's in board_item.cpp like I first
// tried it.
VECTOR2I BOARD_ITEM::ZeroOffset( 0, 0 );


BOARD::BOARD() :
        BOARD_ITEM_CONTAINER( (BOARD_ITEM*) nullptr, PCB_T ),
        m_LegacyDesignSettingsLoaded( false ),
        m_LegacyCopperEdgeClearanceLoaded( false ),
        m_LegacyNetclassesLoaded( false ),
        m_boardUse( BOARD_USE::NORMAL ),
        m_timeStamp( 1 ),
        m_paper( PAGE_INFO::A4 ),
        m_project( nullptr ),
        m_userUnits( EDA_UNITS::MILLIMETRES ),
        m_designSettings( new BOARD_DESIGN_SETTINGS( nullptr, "board.design_settings" ) ),
        m_NetInfo( this )
{
    // A too small value do not allow connecting 2 shapes (i.e. segments) not exactly connected
    // A too large value do not allow safely connecting 2 shapes like very short segments.
    m_outlinesChainingEpsilon = pcbIUScale.mmToIU( DEFAULT_CHAINING_EPSILON_MM );

    m_DRCMaxClearance = 0;
    m_DRCMaxPhysicalClearance = 0;

    // we have not loaded a board yet, assume latest until then.
    m_fileFormatVersionAtLoad = LEGACY_BOARD_FILE_VERSION;

    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        m_layers[layer].m_name = GetStandardLayerName( ToLAYER_ID( layer ) );

        if( IsCopperLayer( layer ) )
            m_layers[layer].m_type = LT_SIGNAL;
        else if( layer >= User_1 && layer <= User_9 )
            m_layers[layer].m_type = LT_AUX;
        else
            m_layers[layer].m_type = LT_UNDEFINED;
    }

    recalcOpposites();

    // Creates a zone to show sloder mask bridges created by a min web value
    // it it just to show them
    m_SolderMaskBridges = new ZONE( this );
    m_SolderMaskBridges->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER );
    m_SolderMaskBridges->SetLayerSet( LSET().set( F_Mask ).set( B_Mask ) );
    int infinity = ( std::numeric_limits<int>::max() / 2 ) - pcbIUScale.mmToIU( 1 );
    m_SolderMaskBridges->Outline()->NewOutline();
    m_SolderMaskBridges->Outline()->Append( VECTOR2I( -infinity, -infinity ) );
    m_SolderMaskBridges->Outline()->Append( VECTOR2I( -infinity, +infinity ) );
    m_SolderMaskBridges->Outline()->Append( VECTOR2I( +infinity, +infinity ) );
    m_SolderMaskBridges->Outline()->Append( VECTOR2I( +infinity, -infinity ) );
    m_SolderMaskBridges->SetMinThickness( 0 );

    BOARD_DESIGN_SETTINGS& bds = GetDesignSettings();

    // Initialize default netclass.
    bds.m_NetSettings->m_DefaultNetClass = std::make_shared<NETCLASS>( NETCLASS::Default );
    bds.m_NetSettings->m_DefaultNetClass->SetDescription( _( "This is the default net class." ) );

    bds.UseCustomTrackViaSize( false );

    // Initialize ratsnest
    m_connectivity.reset( new CONNECTIVITY_DATA() );

    // Set flag bits on these that will only be cleared if these are loaded from a legacy file
    m_LegacyVisibleLayers.reset().set( Rescue );
    m_LegacyVisibleItems.reset().set( GAL_LAYER_INDEX( GAL_LAYER_ID_BITMASK_END ) );
}


BOARD::~BOARD()
{
    // Untangle group parents before doing any deleting
    for( PCB_GROUP* group : m_groups )
    {
        for( BOARD_ITEM* item : group->GetItems() )
            item->SetParentGroup( nullptr );
    }

    for( PCB_GENERATOR* generator : m_generators )
    {
        for( BOARD_ITEM* item : generator->GetItems() )
            item->SetParentGroup( nullptr );
    }

    m_itemByIdCache.clear();

    // Clean up the owned elements
    DeleteMARKERs();

    delete m_SolderMaskBridges;

    BOARD_ITEM_SET ownedItems = GetItemSet();

    m_zones.clear();
    m_footprints.clear();
    m_tracks.clear();
    m_drawings.clear();
    m_groups.clear();

    // Generators not currently returned by GetItemSet
    for( PCB_GENERATOR* g : m_generators )
        ownedItems.insert( g );

    m_generators.clear();

    // Delete the owned items after clearing the containers, because some item dtors
    // cause call chains that query the containers
    for( BOARD_ITEM* item : ownedItems )
        delete item;
}


bool BOARD::BuildConnectivity( PROGRESS_REPORTER* aReporter )
{
    if( !GetConnectivity()->Build( this, aReporter ) )
        return false;

    UpdateRatsnestExclusions();
    return true;
}


void BOARD::SetProject( PROJECT* aProject, bool aReferenceOnly )
{
    if( m_project )
        ClearProject();

    m_project = aProject;

    if( aProject && !aReferenceOnly )
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
        {
            std::shared_ptr<NET_SETTINGS>  legacySettings  = GetDesignSettings().m_NetSettings;
            std::shared_ptr<NET_SETTINGS>& projectSettings = project.NetSettings();

            projectSettings->m_DefaultNetClass = legacySettings->m_DefaultNetClass;
            projectSettings->m_NetClasses      = legacySettings->m_NetClasses;
            projectSettings->m_NetClassPatternAssignments =
                    std::move( legacySettings->m_NetClassPatternAssignments );
            projectSettings->m_NetClassPatternAssignmentCache.clear();
        }

        // Now update the DesignSettings' netclass pointer to point into the project.
        GetDesignSettings().m_NetSettings = project.NetSettings();
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

    GetDesignSettings().m_NetSettings = nullptr;
    GetDesignSettings().SetParent( nullptr );
    m_project = nullptr;
}


void BOARD::IncrementTimeStamp()
{
    m_timeStamp++;

    if( !m_IntersectsAreaCache.empty()
        || !m_EnclosedByAreaCache.empty()
        || !m_IntersectsCourtyardCache.empty()
        || !m_IntersectsFCourtyardCache.empty()
        || !m_IntersectsBCourtyardCache.empty()
        || !m_LayerExpressionCache.empty()
        || !m_ZoneBBoxCache.empty()
        || m_CopperItemRTreeCache
        || m_maxClearanceValue.has_value() )
    {
        std::unique_lock<std::shared_mutex> writeLock( m_CachesMutex );

        m_IntersectsAreaCache.clear();
        m_EnclosedByAreaCache.clear();
        m_IntersectsCourtyardCache.clear();
        m_IntersectsFCourtyardCache.clear();
        m_IntersectsBCourtyardCache.clear();
        m_LayerExpressionCache.clear();

        m_ZoneBBoxCache.clear();

        m_CopperItemRTreeCache = nullptr;

        // These are always regenerated before use, but still probably safer to clear them
        // while we're here.
        m_DRCMaxClearance = 0;
        m_DRCMaxPhysicalClearance = 0;
        m_DRCZones.clear();
        m_DRCCopperZones.clear();
        m_ZoneIsolatedIslandsMap.clear();
        m_CopperZoneRTreeCache.clear();

        m_maxClearanceValue.reset();
    }
}


void BOARD::UpdateRatsnestExclusions()
{
    std::set<std::pair<KIID, KIID>> m_ratsnestExclusions;

    for( PCB_MARKER* marker : GetBoard()->Markers() )
    {
        if( marker->GetMarkerType() == MARKER_BASE::MARKER_RATSNEST && marker->IsExcluded() )
        {
            const std::shared_ptr<RC_ITEM>& rcItem = marker->GetRCItem();
            m_ratsnestExclusions.emplace( rcItem->GetMainItemID(), rcItem->GetAuxItemID() );
            m_ratsnestExclusions.emplace( rcItem->GetAuxItemID(), rcItem->GetMainItemID() );
        }
    }

    GetConnectivity()->RunOnUnconnectedEdges(
            [&]( CN_EDGE& aEdge )
            {
                if( aEdge.GetSourceNode() && aEdge.GetTargetNode()
                    && !aEdge.GetSourceNode()->Dirty() && !aEdge.GetTargetNode()->Dirty() )
                {
                    std::pair<KIID, KIID> ids = { aEdge.GetSourceNode()->Parent()->m_Uuid,
                                                  aEdge.GetTargetNode()->Parent()->m_Uuid };

                    aEdge.SetVisible( m_ratsnestExclusions.count( ids ) == 0 );
                }

                return true;
            } );
}


void BOARD::RecordDRCExclusions()
{
    m_designSettings->m_DrcExclusions.clear();
    m_designSettings->m_DrcExclusionComments.clear();

    for( PCB_MARKER* marker : m_markers )
    {
        if( marker->IsExcluded() )
        {
            wxString serialized = marker->SerializeToString();
            m_designSettings->m_DrcExclusions.insert( serialized );
            m_designSettings->m_DrcExclusionComments[ serialized ] = marker->GetComment();
        }
    }
}


std::vector<PCB_MARKER*> BOARD::ResolveDRCExclusions( bool aCreateMarkers )
{
    std::set<wxString>           exclusions = m_designSettings->m_DrcExclusions;
    std::map<wxString, wxString> comments = m_designSettings->m_DrcExclusionComments;

    m_designSettings->m_DrcExclusions.clear();
    m_designSettings->m_DrcExclusionComments.clear();

    for( PCB_MARKER* marker : GetBoard()->Markers() )
    {
        wxString                     serialized = marker->SerializeToString();
        std::set<wxString>::iterator it = exclusions.find( serialized );

        if( it != exclusions.end() )
        {
            marker->SetExcluded( true, comments[ serialized ] );

            // Exclusion still valid; store back to BOARD_DESIGN_SETTINGS
            m_designSettings->m_DrcExclusions.insert( serialized );
            m_designSettings->m_DrcExclusionComments[ serialized ] = comments[ serialized ];

            exclusions.erase( it );
        }
    }

    std::vector<PCB_MARKER*> newMarkers;

    if( aCreateMarkers )
    {
        for( const wxString& serialized : exclusions )
        {
            PCB_MARKER* marker = PCB_MARKER::DeserializeFromString( serialized );

            if( !marker )
                continue;

            // Check to see if items still exist
            for( const KIID& guid : marker->GetRCItem()->GetIDs() )
            {
                if( GetItem( guid ) == DELETED_BOARD_ITEM::GetInstance() )
                {
                    delete marker;
                    marker = nullptr;
                    break;
                }
            }

            if( marker )
            {
                marker->SetExcluded( true, comments[ serialized ] );
                newMarkers.push_back( marker );

                // Exclusion still valid; store back to BOARD_DESIGN_SETTINGS
                m_designSettings->m_DrcExclusions.insert( serialized );
                m_designSettings->m_DrcExclusionComments[ serialized ] = comments[ serialized ];
            }
        }
    }

    return newMarkers;
}


void BOARD::GetContextualTextVars( wxArrayString* aVars ) const
{
    auto add =
            [&]( const wxString& aVar )
            {
                if( !alg::contains( *aVars, aVar ) )
                    aVars->push_back( aVar );
            };

    add( wxT( "LAYER" ) );
    add( wxT( "FILENAME" ) );
    add( wxT( "FILEPATH" ) );
    add( wxT( "PROJECTNAME" ) );

    GetTitleBlock().GetContextualTextVars( aVars );

    if( GetProject() )
    {
        for( std::pair<wxString, wxString> entry : GetProject()->GetTextVars() )
            add( entry.first );
    }
}


bool BOARD::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( token->Contains( ':' ) )
    {
        wxString      remainder;
        wxString      ref = token->BeforeFirst( ':', &remainder );
        BOARD_ITEM*   refItem = GetItem( KIID( ref ) );

        if( refItem && refItem->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* refFP = static_cast<FOOTPRINT*>( refItem );

            if( refFP->ResolveTextVar( &remainder, aDepth + 1 ) )
            {
                *token = remainder;
                return true;
            }
        }
    }

    if( token->IsSameAs( wxT( "FILENAME" ) ) )
    {
        wxFileName fn( GetFileName() );
        *token = fn.GetFullName();
        return true;
    }
    else if( token->IsSameAs( wxT( "FILEPATH" ) ) )
    {
        wxFileName fn( GetFileName() );
        *token = fn.GetFullPath();
        return true;
    }
    else if( token->IsSameAs( wxT( "PROJECTNAME" ) ) && GetProject() )
    {
        *token = GetProject()->GetProjectName();
        return true;
    }

    wxString var = *token;

    if( m_properties.count( var ) )
    {
        *token = m_properties.at( var );
        return true;
    }
    else if( GetTitleBlock().TextVarResolver( token, m_project ) )
    {
        return true;
    }

    if( GetProject() && GetProject()->TextVarResolver( token ) )
        return true;

    return false;
}


VECTOR2I BOARD::GetPosition() const
{
    return ZeroOffset;
}


void BOARD::SetPosition( const VECTOR2I& aPos )
{
    wxLogWarning( wxT( "This should not be called on the BOARD object") );
}


void BOARD::Move( const VECTOR2I& aMoveVector ) // overload
{
    INSPECTOR_FUNC inspector =
            [&] ( EDA_ITEM* item, void* testData )
            {
                if( BOARD_ITEM* board_item = dynamic_cast<BOARD_ITEM*>( item ) )
                {
                    // aMoveVector was snapshotted, don't need "data".
                    // Only move the top level group
                    if( !board_item->GetParentGroup() && !board_item->GetParentFootprint() )
                        board_item->Move( aMoveVector );
                }

                return INSPECT_RESULT::CONTINUE;
            };

    Visit( inspector, nullptr, GENERAL_COLLECTOR::BoardLevelItems );
}


TRACKS BOARD::TracksInNet( int aNetCode )
{
    TRACKS ret;

    INSPECTOR_FUNC inspector = [aNetCode, &ret]( EDA_ITEM* item, void* testData )
                               {
                                   PCB_TRACK* t = static_cast<PCB_TRACK*>( item );

                                   if( t->GetNetCode() == aNetCode )
                                       ret.push_back( t );

                                   return INSPECT_RESULT::CONTINUE;
                               };

    // visit this BOARD's PCB_TRACKs and PCB_VIAs with above TRACK INSPECTOR which
    // appends all in aNetCode to ret.
    Visit( inspector, nullptr, GENERAL_COLLECTOR::Tracks );

    return ret;
}


bool BOARD::SetLayerDescr( PCB_LAYER_ID aIndex, const LAYER& aLayer )
{
    if( unsigned( aIndex ) < arrayDim( m_layers ) )
    {
        m_layers[ aIndex ] = aLayer;
        recalcOpposites();
        return true;
    }

    return false;
}


PCB_LAYER_ID BOARD::GetLayerID( const wxString& aLayerName ) const
{
    // Check the BOARD physical layer names.
    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if ( m_layers[ layer ].m_name == aLayerName || m_layers[ layer ].m_userName == aLayerName )
            return ToLAYER_ID( layer );
    }

    // Otherwise fall back to the system standard layer names for virtual layers.
    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
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
        recalcOpposites();
        return true;
    }

    return false;
}


LAYER_T BOARD::GetLayerType( PCB_LAYER_ID aLayer ) const
{
    if( IsLayerEnabled( aLayer ) )
        return m_layers[aLayer].m_type;

    if( aLayer >= User_1 && aLayer <= User_9 )
        return LT_AUX;
    else if( IsCopperLayer( aLayer ) )
        return LT_SIGNAL;
    else
        return LT_UNDEFINED;
}


bool BOARD::SetLayerType( PCB_LAYER_ID aLayer, LAYER_T aLayerType )
{
    if( IsLayerEnabled( aLayer ) )
    {
        m_layers[aLayer].m_type = aLayerType;
        recalcOpposites();
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
    case LT_AUX:    return "auxiliary";
    case LT_FRONT:  return "front";
    case LT_BACK:   return "back";
    }
}


LAYER_T LAYER::ParseType( const char* aType )
{
    if(      strcmp( aType, "signal" ) == 0 )    return LT_SIGNAL;
    else if( strcmp( aType, "power" ) == 0 )     return LT_POWER;
    else if( strcmp( aType, "mixed" ) == 0 )     return LT_MIXED;
    else if( strcmp( aType, "jumper" ) == 0 )    return LT_JUMPER;
    else if( strcmp( aType, "auxiliary" ) == 0 ) return LT_AUX;
    else if( strcmp( aType, "front" ) == 0 )     return LT_FRONT;
    else if( strcmp( aType, "back" ) == 0 )      return LT_BACK;
    else                                         return LT_UNDEFINED;
}


void BOARD::recalcOpposites()
{
    for( int layer = F_Cu; layer < User_9; ++layer )
        m_layers[layer].m_opposite = ::FlipLayer( ToLAYER_ID( layer ), GetCopperLayerCount() );

    // Match up similary-named front/back user layers
    for( int layer = User_1; layer <= User_9; ++layer )
    {
        if( m_layers[layer].m_opposite != layer )   // already paired
            continue;

        if( m_layers[layer].m_type != LT_FRONT )
            continue;

        wxString principalName = m_layers[layer].m_userName.AfterFirst( '.' );

        for( int ii = User_1; ii <= User_9; ++ii )
        {
            if( ii == layer )                      // can't pair with self
                continue;

            if( m_layers[ii].m_opposite != ii )    // already paired
                continue;

            if( m_layers[ii].m_type != LT_BACK )
                continue;

            wxString candidate = m_layers[ii].m_userName.AfterFirst( '.' );

            if( !candidate.IsEmpty() && candidate == principalName )
            {
                m_layers[layer].m_opposite = ii;
                m_layers[ii].m_opposite = layer;
                break;
            }
        }
    }

    // Match up non-custom-named consecutive front/back user layer pairs
    for( int layer = User_1; layer < User_9; ++layer )
    {
        int next = layer + 1;

        // ignore already-matched layers
        if( m_layers[layer].m_opposite != layer || m_layers[next].m_opposite != next )
            continue;

        // ignore layer pairs that aren't consecutive front/back
        if( m_layers[layer].m_type != LT_FRONT || m_layers[next].m_type != LT_BACK )
            continue;

        if( m_layers[layer].m_userName != m_layers[layer].m_name
                && m_layers[next].m_userName != m_layers[next].m_name )
        {
            m_layers[layer].m_opposite = next;
            m_layers[next].m_opposite = layer;
        }
    }
}


PCB_LAYER_ID BOARD::FlipLayer( PCB_LAYER_ID aLayer ) const
{
    return ToLAYER_ID( m_layers[aLayer].m_opposite );
}


int BOARD::GetCopperLayerCount() const
{
    return GetDesignSettings().GetCopperLayerCount();
}


void BOARD::SetCopperLayerCount( int aCount )
{
    GetDesignSettings().SetCopperLayerCount( aCount );
}


int BOARD::LayerDepth( PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer ) const
{
    if( aStartLayer > aEndLayer )
        std::swap( aStartLayer, aEndLayer );

    if( aEndLayer == B_Cu )
        aEndLayer = ToLAYER_ID( F_Cu + GetCopperLayerCount() - 1 );

    return aEndLayer - aStartLayer;
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


bool BOARD::IsLayerEnabled( PCB_LAYER_ID aLayer ) const
{
    return GetDesignSettings().IsLayerEnabled( aLayer );
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

        for( PCB_TRACK* track : Tracks() )
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
    case F_Cu: return IsElementVisible( LAYER_FOOTPRINTS_FR );
    case B_Cu: return IsElementVisible( LAYER_FOOTPRINTS_BK );
    default:   wxFAIL_MSG( wxT( "BOARD::IsModuleLayerVisible(): bad layer" ) ); return true;
    }
}



BOARD_DESIGN_SETTINGS& BOARD::GetDesignSettings() const
{
    return *m_designSettings;
}


int BOARD::GetMaxClearanceValue() const
{
    if( !m_maxClearanceValue.has_value() )
    {
        std::unique_lock<std::shared_mutex> writeLock( m_CachesMutex );

        int worstClearance = m_designSettings->GetBiggestClearanceValue();

        for( ZONE* zone : m_zones )
            worstClearance = std::max( worstClearance, zone->GetLocalClearance().value() );

        for( FOOTPRINT* footprint : m_footprints )
        {
            for( PAD* pad : footprint->Pads() )
            {
                std::optional<int> override = pad->GetClearanceOverrides( nullptr );

                if( override.has_value() )
                    worstClearance = std::max( worstClearance, override.value() );
            }

            for( ZONE* zone : footprint->Zones() )
                worstClearance = std::max( worstClearance, zone->GetLocalClearance().value() );
        }

        m_maxClearanceValue = worstClearance;
    }

    return m_maxClearanceValue.value_or( 0 );
};


void BOARD::CacheTriangulation( PROGRESS_REPORTER* aReporter, const std::vector<ZONE*>& aZones )
{
    std::vector<ZONE*> zones = aZones;

    if( zones.empty() )
        zones = m_zones;

    if( zones.empty() )
        return;

    if( aReporter )
        aReporter->Report( _( "Tessellating copper zones..." ) );

    thread_pool& tp = GetKiCadThreadPool();
    std::vector<std::future<size_t>> returns;

    returns.reserve( zones.size() );

    auto cache_zones = [aReporter]( ZONE* aZone ) -> size_t
            {
                if( aReporter && aReporter->IsCancelled() )
                    return 0;

                aZone->CacheTriangulation();

                if( aReporter )
                    aReporter->AdvanceProgress();

                return 1;
            };

    for( ZONE* zone : zones )
        returns.emplace_back( tp.submit( cache_zones, zone ) );

    // Finalize the triangulation threads
    for( const std::future<size_t>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            if( aReporter )
                aReporter->KeepRefreshing();

            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }
}


void BOARD::Add( BOARD_ITEM* aBoardItem, ADD_MODE aMode, bool aSkipConnectivity )
{
    if( aBoardItem == nullptr )
    {
        wxFAIL_MSG( wxT( "BOARD::Add() param error: aBoardItem nullptr" ) );
        return;
    }

    m_itemByIdCache.insert( { aBoardItem->m_Uuid, aBoardItem } );

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
    case PCB_GENERATOR_T:
        m_generators.push_back( (PCB_GENERATOR*) aBoardItem );
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
            m_tracks.push_back( static_cast<PCB_TRACK*>( aBoardItem ) );
        else
            m_tracks.push_front( static_cast<PCB_TRACK*>( aBoardItem ) );

        break;

    case PCB_FOOTPRINT_T:
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aBoardItem );

        if( aMode == ADD_MODE::APPEND || aMode == ADD_MODE::BULK_APPEND )
            m_footprints.push_back( footprint );
        else
            m_footprints.push_front( footprint );

        footprint->RunOnChildren( [&]( BOARD_ITEM* aChild )
                                  {
                                      m_itemByIdCache.insert( { aChild->m_Uuid, aChild } );
                                  } );
        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_SHAPE_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TARGET_T:
    {
        if( aMode == ADD_MODE::APPEND || aMode == ADD_MODE::BULK_APPEND )
            m_drawings.push_back( aBoardItem );
        else
            m_drawings.push_front( aBoardItem );

        if( aBoardItem->Type() == PCB_TABLE_T )
        {
            PCB_TABLE* table = static_cast<PCB_TABLE*>( aBoardItem );

            table->RunOnChildren( [&]( BOARD_ITEM* aChild )
                                  {
                                      m_itemByIdCache.insert( { aChild->m_Uuid, aChild } );
                                  } );
        }

        break;
    }

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

    if( !aSkipConnectivity )
        m_connectivity->Add( aBoardItem );

    if( aMode != ADD_MODE::BULK_INSERT && aMode != ADD_MODE::BULK_APPEND )
    {
        InvokeListeners( &BOARD_LISTENER::OnBoardItemAdded, *this, aBoardItem );
    }
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
    // find these calls and fix them!  Don't send me no stinking' nullptr.
    wxASSERT( aBoardItem );

    m_itemByIdCache.erase( aBoardItem->m_Uuid );

    switch( aBoardItem->Type() )
    {
    case PCB_NETINFO_T:
    {
        NETINFO_ITEM* netItem = static_cast<NETINFO_ITEM*>( aBoardItem );
        NETINFO_ITEM* unconnected = m_NetInfo.GetNetItem( NETINFO_LIST::UNCONNECTED );

        for( BOARD_CONNECTED_ITEM* boardItem : AllConnectedItems() )
        {
            if( boardItem->GetNet() == netItem )
                boardItem->SetNet( unconnected );
        }

        m_NetInfo.RemoveNet( netItem );
        break;
    }

    case PCB_MARKER_T:
        alg::delete_matching( m_markers, aBoardItem );
        break;

    case PCB_GROUP_T:
        alg::delete_matching( m_groups, aBoardItem );
        break;

    case PCB_ZONE_T:
        alg::delete_matching( m_zones, aBoardItem );
        break;

    case PCB_GENERATOR_T:
        alg::delete_matching( m_generators, aBoardItem );
        break;

    case PCB_FOOTPRINT_T:
    {
        alg::delete_matching( m_footprints, aBoardItem );
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aBoardItem );

        footprint->RunOnChildren( [&]( BOARD_ITEM* aChild )
                                  {
                                      m_itemByIdCache.erase( aChild->m_Uuid );
                                  } );

        break;
    }

    case PCB_TRACE_T:
    case PCB_ARC_T:
    case PCB_VIA_T:
        alg::delete_matching( m_tracks, aBoardItem );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_SHAPE_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TARGET_T:
    {
        alg::delete_matching( m_drawings, aBoardItem );

        if( aBoardItem->Type() == PCB_TABLE_T )
        {
            PCB_TABLE* table = static_cast<PCB_TABLE*>( aBoardItem );

            table->RunOnChildren( [&]( BOARD_ITEM* aChild )
                                  {
                                      m_itemByIdCache.erase( aChild->m_Uuid );
                                  } );
        }

        break;
    }

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


void BOARD::RemoveAll( std::initializer_list<KICAD_T> aTypes )
{
    std::vector<BOARD_ITEM*> removed;

    for( const KICAD_T& type : aTypes )
    {
        switch( type )
        {
        case PCB_NETINFO_T:
            for( NETINFO_ITEM* item : m_NetInfo )
                removed.emplace_back( item );

            m_NetInfo.clear();
            break;

        case PCB_MARKER_T:
            std::copy( m_markers.begin(), m_markers.end(), std::back_inserter( removed ) );
            m_markers.clear();
            break;

        case PCB_GROUP_T:
            std::copy( m_groups.begin(), m_groups.end(), std::back_inserter( removed ) );
            m_groups.clear();
            break;

        case PCB_ZONE_T:
            std::copy( m_zones.begin(), m_zones.end(), std::back_inserter( removed ) );
            m_zones.clear();
            break;

        case PCB_GENERATOR_T:
            std::copy( m_generators.begin(), m_generators.end(), std::back_inserter( removed ) );
            m_generators.clear();
            break;

        case PCB_FOOTPRINT_T:
            std::copy( m_footprints.begin(), m_footprints.end(), std::back_inserter( removed ) );
            m_footprints.clear();
            break;

        case PCB_TRACE_T:
            std::copy( m_tracks.begin(), m_tracks.end(), std::back_inserter( removed ) );
            m_tracks.clear();
            break;

        case PCB_ARC_T:
        case PCB_VIA_T:
            wxFAIL_MSG( wxT( "Use PCB_TRACE_T to remove all tracks, arcs, and vias" ) );
            break;

        case PCB_SHAPE_T:
            std::copy( m_drawings.begin(), m_drawings.end(), std::back_inserter( removed ) );
            m_drawings.clear();
            break;

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
        case PCB_REFERENCE_IMAGE_T:
        case PCB_FIELD_T:
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_TABLE_T:
        case PCB_TARGET_T:
            wxFAIL_MSG( wxT( "Use PCB_SHAPE_T to remove all graphics and text" ) );
            break;

        default:
            wxFAIL_MSG( wxT( "BOARD::RemoveAll() needs more ::Type() support" ) );
        }
    }

    for( BOARD_ITEM* item : removed )
        m_itemByIdCache.erase( item->m_Uuid );

    FinalizeBulkRemove( removed );
}


wxString BOARD::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "PCB" ) );
}


void BOARD::UpdateUserUnits( BOARD_ITEM* aItem, KIGFX::VIEW* aView )
{
    INSPECTOR_FUNC inspector =
            [&]( EDA_ITEM* descendant, void* aTestData )
            {
                PCB_DIMENSION_BASE* dimension = static_cast<PCB_DIMENSION_BASE*>( descendant );

                if( dimension->GetUnitsMode() == DIM_UNITS_MODE::AUTOMATIC )
                {
                    dimension->UpdateUnits();

                    if( aView )
                        aView->Update( dimension );
                }

                return INSPECT_RESULT::CONTINUE;
            };

    aItem->Visit( inspector, nullptr, { PCB_DIM_ALIGNED_T,
                                        PCB_DIM_LEADER_T,
                                        PCB_DIM_ORTHOGONAL_T,
                                        PCB_DIM_CENTER_T,
                                        PCB_DIM_RADIAL_T } );
}


void BOARD::DeleteMARKERs()
{
    // the vector does not know how to delete the PCB_MARKER, it holds pointers
    for( PCB_MARKER* marker : m_markers )
    {
        // We also must clear the cache
        m_itemByIdCache.erase( marker->m_Uuid );
        delete marker;
    }

    m_markers.clear();
}


void BOARD::DeleteMARKERs( bool aWarningsAndErrors, bool aExclusions )
{
    // Deleting lots of items from a vector can be very slow.  Copy remaining items instead.
    MARKERS remaining;

    for( PCB_MARKER* marker : m_markers )
    {
        if( ( marker->GetSeverity() == RPT_SEVERITY_EXCLUSION && aExclusions )
                || ( marker->GetSeverity() != RPT_SEVERITY_EXCLUSION && aWarningsAndErrors ) )
        {
            // We also must clear the cache
            m_itemByIdCache.erase( marker->m_Uuid );
            delete marker;
        }
        else
        {
            remaining.push_back( marker );
        }
    }

    m_markers = remaining;
}


void BOARD::DeleteAllFootprints()
{
    for( FOOTPRINT* footprint : m_footprints )
    {
        m_itemByIdCache.erase( footprint->m_Uuid );
        delete footprint;
    }

    m_footprints.clear();
    IncrementTimeStamp();
}


BOARD_ITEM* BOARD::GetItem( const KIID& aID ) const
{
    if( aID == niluuid )
        return nullptr;

    if( m_itemByIdCache.count( aID ) )
        return m_itemByIdCache.at( aID );

    for( PCB_TRACK* track : Tracks() )
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

        for( PCB_FIELD* field : footprint->Fields() )
        {
            if( field->m_Uuid == aID )
                return field;
        }

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
        if( drawing->Type() == PCB_TABLE_T )
        {
            for( PCB_TABLECELL* cell : static_cast<PCB_TABLE*>( drawing )->GetCells() )
            {
                if( cell->m_Uuid == aID )
                    return drawing;
            }
        }

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

    for( PCB_GENERATOR* generator : m_generators )
    {
        if( generator->m_Uuid == aID )
            return generator;
    }


    for( NETINFO_ITEM* netInfo : m_NetInfo )
    {
        if( netInfo->m_Uuid == aID )
            return netInfo;
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

    for( PCB_TRACK* track : Tracks() )
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

    for( PCB_GENERATOR* generator : m_generators )
        aMap[ generator->m_Uuid ] = generator;
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
                            token = footprint->m_Uuid.AsString() + wxT( ":" ) + remainder;

                        break;
                    }
                }
            }

            newbuf.append( wxT( "${" ) + token + wxT( "}" ) );
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
                {
                    token = static_cast<FOOTPRINT*>( refItem )->GetReference() + wxT( ":" )
                                                                               + remainder;
                }
            }

            newbuf.append( wxT( "${" ) + token + wxT( "}" ) );
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


BOX2I BOARD::ComputeBoundingBox( bool aBoardEdgesOnly, bool aIncludeHiddenText ) const
{
    BOX2I bbox;
    LSET  visible = GetVisibleLayers();

    // If the board is just showing a footprint, we want all footprint layers included in the
    // bounding box
    if( IsFootprintHolder() )
        visible.set();

    if( aBoardEdgesOnly )
        visible.set( Edge_Cuts );

    // Check shapes, dimensions, texts, and fiducials
    for( BOARD_ITEM* item : m_drawings )
    {
        if( aBoardEdgesOnly && ( item->GetLayer() != Edge_Cuts || item->Type() != PCB_SHAPE_T ) )
            continue;

        if( ( item->GetLayerSet() & visible ).any() )
            bbox.Merge( item->GetBoundingBox() );
    }

    // Check footprints
    for( FOOTPRINT* footprint : m_footprints )
    {
        if( aBoardEdgesOnly )
        {
            for( const BOARD_ITEM* edge : footprint->GraphicalItems() )
            {
                if( edge->GetLayer() == Edge_Cuts && edge->Type() == PCB_SHAPE_T )
                    bbox.Merge( edge->GetBoundingBox() );
            }
        }
        else if( ( footprint->GetLayerSet() & visible ).any() )
        {
            bbox.Merge( footprint->GetBoundingBox( true, aIncludeHiddenText ) );
        }
    }

    if( !aBoardEdgesOnly )
    {
        // Check tracks
        for( PCB_TRACK* track : m_tracks )
        {
            if( ( track->GetLayerSet() & visible ).any() )
                bbox.Merge( track->GetBoundingBox() );
        }

        // Check zones
        for( ZONE* aZone : m_zones )
        {
            if( ( aZone->GetLayerSet() & visible ).any() )
                bbox.Merge( aZone->GetBoundingBox() );
        }
    }

    return bbox;
}


void BOARD::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    int           padCount = 0;
    int           viaCount = 0;
    int           trackSegmentCount = 0;
    std::set<int> netCodes;
    int           unconnected = GetConnectivity()->GetUnconnectedCount( true );

    for( PCB_TRACK* item : m_tracks )
    {
        if( item->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackSegmentCount++;

        if( item->GetNetCode() > 0 )
            netCodes.insert( item->GetNetCode() );
    }

    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            padCount++;

            if( pad->GetNetCode() > 0 )
                netCodes.insert( pad->GetNetCode() );
        }
    }

    aList.emplace_back( _( "Pads" ), wxString::Format( wxT( "%d" ), padCount ) );
    aList.emplace_back( _( "Vias" ), wxString::Format( wxT( "%d" ), viaCount ) );
    aList.emplace_back( _( "Track Segments" ), wxString::Format( wxT( "%d" ), trackSegmentCount ) );
    aList.emplace_back( _( "Nets" ), wxString::Format( wxT( "%d" ), (int) netCodes.size() ) );
    aList.emplace_back( _( "Unrouted" ), wxString::Format( wxT( "%d" ), unconnected ) );
}


INSPECT_RESULT BOARD::Visit( INSPECTOR inspector, void* testData,
                             const std::vector<KICAD_T>& scanTypes )
{
#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    bool footprintsScanned = false;
    bool drawingsScanned = false;
    bool tracksScanned = false;

    for( KICAD_T scanType : scanTypes )
    {
        switch( scanType )
        {
        case PCB_T:
            if( inspector( this, testData ) == INSPECT_RESULT::QUIT )
                return INSPECT_RESULT::QUIT;

            break;

        /*
         * Instances of the requested KICAD_T live in a list, either one that I manage, or one
         * that my footprints manage.  If it's a type managed by class FOOTPRINT, then simply
         * pass it on to each footprint's Visit() function via IterateForward( m_footprints, ... ).
         */

        case PCB_FOOTPRINT_T:
        case PCB_PAD_T:
        case PCB_SHAPE_T:
        case PCB_REFERENCE_IMAGE_T:
        case PCB_FIELD_T:
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_TABLE_T:
        case PCB_TABLECELL_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
        case PCB_TARGET_T:
            if( !footprintsScanned )
            {
                if( IterateForward<FOOTPRINT*>( m_footprints, inspector, testData, scanTypes )
                        == INSPECT_RESULT::QUIT )
                {
                    return INSPECT_RESULT::QUIT;
                }

                footprintsScanned = true;
            }

            if( !drawingsScanned )
            {
                if( IterateForward<BOARD_ITEM*>( m_drawings, inspector, testData, scanTypes )
                        == INSPECT_RESULT::QUIT )
                {
                    return INSPECT_RESULT::QUIT;
                }

                drawingsScanned = true;
            }

            break;

        case PCB_VIA_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
            if( !tracksScanned )
            {
                if( IterateForward<PCB_TRACK*>( m_tracks, inspector, testData, scanTypes )
                        == INSPECT_RESULT::QUIT )
                {
                    return INSPECT_RESULT::QUIT;
                }

                tracksScanned = true;
            }

            break;

        case PCB_MARKER_T:
            for( PCB_MARKER* marker : m_markers )
            {
                if( marker->Visit( inspector, testData, { scanType } ) == INSPECT_RESULT::QUIT )
                    return INSPECT_RESULT::QUIT;
            }

            break;

        case PCB_ZONE_T:
            if( !footprintsScanned )
            {
                if( IterateForward<FOOTPRINT*>( m_footprints, inspector, testData, scanTypes )
                        == INSPECT_RESULT::QUIT )
                {
                    return INSPECT_RESULT::QUIT;
                }

                footprintsScanned = true;
            }

            for( ZONE* zone : m_zones)
            {
                if( zone->Visit( inspector, testData, { scanType } ) == INSPECT_RESULT::QUIT )
                    return INSPECT_RESULT::QUIT;
            }

            break;

        case PCB_GENERATOR_T:
            if( !footprintsScanned )
            {
                if( IterateForward<FOOTPRINT*>( m_footprints, inspector, testData, scanTypes )
                    == INSPECT_RESULT::QUIT )
                {
                    return INSPECT_RESULT::QUIT;
                }

                footprintsScanned = true;
            }

            if( IterateForward<PCB_GENERATOR*>( m_generators, inspector, testData, { scanType } )
                == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }

            break;

        case PCB_GROUP_T:
            if( IterateForward<PCB_GROUP*>( m_groups, inspector, testData, { scanType } )
                    == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }

            break;

        default:
            break;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


NETINFO_ITEM* BOARD::FindNet( int aNetcode ) const
{
    // the first valid netcode is 1 and the last is m_NetInfo.GetCount()-1.
    // zero is reserved for "no connection" and is not actually a net.
    // nullptr is returned for non valid netcodes

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


int BOARD::MatchDpSuffix( const wxString& aNetName, wxString& aComplementNet )
{
    int rv = 0;
    int count = 0;

    for( auto it = aNetName.rbegin(); it != aNetName.rend() && rv == 0; ++it, ++count )
    {
        int ch = *it;

        if( ( ch >= '0' && ch <= '9' ) || ch == '_' )
        {
            continue;
        }
        else if( ch == '+' )
        {
            aComplementNet = wxT( "-" );
            rv = 1;
        }
        else if( ch == '-' )
        {
            aComplementNet = wxT( "+" );
            rv = -1;
        }
        else if( ch == 'N' )
        {
            aComplementNet = wxT( "P" );
            rv = -1;
        }
        else if ( ch == 'P' )
        {
            aComplementNet = wxT( "N" );
            rv = 1;
        }
        else
        {
            break;
        }
    }

    if( rv != 0 && count >= 1 )
    {
        aComplementNet = aNetName.Left( aNetName.length() - count )
                            + aComplementNet
                            + aNetName.Right( count - 1 );
    }

    return rv;
}


NETINFO_ITEM* BOARD::DpCoupledNet( const NETINFO_ITEM* aNet )
{
    if( aNet )
    {
        wxString refName = aNet->GetNetname();
        wxString coupledNetName;

        if( MatchDpSuffix( refName, coupledNetName ) )
            return FindNet( coupledNetName );
    }

    return nullptr;
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


std::set<wxString> BOARD::GetNetClassAssignmentCandidates() const
{
    std::set<wxString> names;

    for( const NETINFO_ITEM* net : m_NetInfo )
    {
        if( !net->GetNetname().IsEmpty() )
            names.insert( net->GetNetname() );
    }

    return names;
}


void BOARD::SynchronizeProperties()
{
    wxCHECK( m_project, /*void*/ );

    if( !m_project->IsNullProject() )
        SetProperties( m_project->GetTextVars() );
}


void BOARD::SynchronizeNetsAndNetClasses( bool aResetTrackAndViaSizes )
{
    if( !m_project )
        return;

    BOARD_DESIGN_SETTINGS&     bds = GetDesignSettings();
    std::shared_ptr<NETCLASS>& defaultNetClass = bds.m_NetSettings->m_DefaultNetClass;

    for( NETINFO_ITEM* net : m_NetInfo )
        net->SetNetClass( bds.m_NetSettings->GetEffectiveNetClass( net->GetNetname() ) );

    if( aResetTrackAndViaSizes )
    {
        // Set initial values for custom track width & via size to match the default
        // netclass settings
        bds.UseCustomTrackViaSize( false );
        bds.SetCustomTrackWidth( defaultNetClass->GetTrackWidth() );
        bds.SetCustomViaSize( defaultNetClass->GetViaDiameter() );
        bds.SetCustomViaDrill( defaultNetClass->GetViaDrill() );
        bds.SetCustomDiffPairWidth( defaultNetClass->GetDiffPairWidth() );
        bds.SetCustomDiffPairGap( defaultNetClass->GetDiffPairGap() );
        bds.SetCustomDiffPairViaGap( defaultNetClass->GetDiffPairViaGap() );
    }

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


PAD* BOARD::GetPad( const VECTOR2I& aPosition, LSET aLayerSet ) const
{
    if( !aLayerSet.any() )
        aLayerSet = LSET::AllCuMask();

    for( FOOTPRINT* footprint : m_footprints )
    {
        PAD* pad = nullptr;

        if( footprint->HitTest( aPosition ) )
            pad = footprint->GetPad( aPosition, aLayerSet );

        if( pad )
            return pad;
    }

    return nullptr;
}


PAD* BOARD::GetPad( const PCB_TRACK* aTrace, ENDPOINT_T aEndPoint ) const
{
    const VECTOR2I& aPosition = aTrace->GetEndPoint( aEndPoint );

    LSET lset( aTrace->GetLayer() );

    return GetPad( aPosition, lset );
}


PAD* BOARD::GetPadFast( const VECTOR2I& aPosition, LSET aLayerSet ) const
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


PAD* BOARD::GetPad( std::vector<PAD*>& aPadList, const VECTOR2I& aPosition, LSET aLayerSet ) const
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
            return nullptr;
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

    return nullptr;
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


BOARD_STACKUP BOARD::GetStackupOrDefault() const
{
    if( GetDesignSettings().m_HasStackup )
        return GetDesignSettings().GetStackupDescriptor();

    BOARD_STACKUP stackup;
    stackup.BuildDefaultStackupList( &GetDesignSettings(), GetCopperLayerCount() );
    return stackup;
}


std::tuple<int, double, double> BOARD::GetTrackLength( const PCB_TRACK& aTrack ) const
{
    int    count = 0;
    double length = 0.0;
    double package_length = 0.0;

    auto              connectivity = GetBoard()->GetConnectivity();
    BOARD_STACKUP&    stackup      = GetDesignSettings().GetStackupDescriptor();
    bool              useHeight    = GetDesignSettings().m_UseHeightForLengthCalcs;

    static const std::vector<KICAD_T> baseConnectedTypes = { PCB_TRACE_T,
                                                             PCB_ARC_T,
                                                             PCB_VIA_T,
                                                             PCB_PAD_T };

    for( BOARD_CONNECTED_ITEM* item : connectivity->GetConnectedItems( &aTrack, baseConnectedTypes ) )
    {
        count++;

        if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
        {
            if( track->Type() == PCB_VIA_T && useHeight )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( track );
                length += stackup.GetLayerDistance( via->TopLayer(), via->BottomLayer() );
                continue;
            }
            else if( track->Type() == PCB_ARC_T )
            {
                // Note: we don't apply the clip-to-pad optimization if an arc ends in a pad
                // Room for future improvement.
                length += track->GetLength();
                continue;
            }

            bool   inPad = false;
            SEG    trackSeg( track->GetStart(), track->GetEnd() );
            double segLen      = trackSeg.Length();
            double segInPadLen = 0;

            for( auto pad_it : connectivity->GetConnectedPads( item ) )
            {
                PAD* pad = static_cast<PAD*>( pad_it );

                bool hitStart = pad->HitTest( track->GetStart(), track->GetWidth() / 2 );
                bool hitEnd   = pad->HitTest( track->GetEnd(), track->GetWidth() / 2 );

                if( hitStart && hitEnd )
                {
                    inPad = true;
                    break;
                }
                else if( hitStart || hitEnd )
                {
                    VECTOR2I loc;

                    // We may not collide even if we passed the bounding-box hit test
                    if( pad->GetEffectivePolygon( ERROR_INSIDE )->Collide( trackSeg, 0, nullptr, &loc ) )
                    {
                        // Part 1: length of the seg to the intersection with the pad poly
                        if( hitStart )
                            trackSeg.A = loc;
                        else
                            trackSeg.B = loc;

                        segLen = trackSeg.Length();

                        // Part 2: length from the intersection to the pad anchor
                        segInPadLen += ( loc - pad->GetPosition() ).EuclideanNorm();
                    }
                }
            }

            if( !inPad )
                length += segLen + segInPadLen;
        }
        else if( PAD* pad = dynamic_cast<PAD*>( item ) )
        {
            package_length += pad->GetPadToDieLength();
        }
    }

    return std::make_tuple( count, length, package_length );
}


FOOTPRINT* BOARD::GetFootprint( const VECTOR2I& aPosition, PCB_LAYER_ID aActiveLayer,
                                bool aVisibleOnly, bool aIgnoreLocked ) const
{
    FOOTPRINT* footprint     = nullptr;
    FOOTPRINT* alt_footprint = nullptr;
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
            BOX2I bb = candidate->GetBoundingBox( false, false );

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

    return nullptr;
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
            for( ZONE* zone : footprint->Zones() )
                zones.push_back( zone );
        }
    }

    return zones;
}


ZONE* BOARD::AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode, PCB_LAYER_ID aLayer,
                      VECTOR2I aStartPointPosition, ZONE_BORDER_DISPLAY_STYLE aHatch )
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


bool BOARD::GetBoardPolygonOutlines( SHAPE_POLY_SET& aOutlines,
                                     OUTLINE_ERROR_HANDLER* aErrorHandler,
                                     bool aAllowUseArcsInPolygons,
                                     bool aIncludeNPTHAsOutlines )
{
    // max dist from one endPt to next startPt: use the current value
    int chainingEpsilon = GetOutlinesChainingEpsilon();

    bool success = BuildBoardPolygonOutlines( this, aOutlines, GetDesignSettings().m_MaxError,
                                              chainingEpsilon, aErrorHandler,
                                              aAllowUseArcsInPolygons );

    // Now add NPTH oval holes as holes in outlines if required
    if( aIncludeNPTHAsOutlines )
    {
        for( FOOTPRINT* fp : Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetAttribute () != PAD_ATTRIB::NPTH )
                    continue;

                SHAPE_POLY_SET hole;
                pad->TransformHoleToPolygon( hole, 0, GetDesignSettings().m_MaxError, ERROR_INSIDE );

                // Add this pad hole to the main outline
                // But we can have more than one main outline (i.e. more than one board), so
                // search the right main outline i.e. the outline that contains the pad hole
                SHAPE_LINE_CHAIN& pad_hole = hole.Outline( 0 );
                const VECTOR2I holePt = pad_hole.CPoint( 0 );

                for( int jj = 0; jj < aOutlines.OutlineCount(); ++jj )
                {
                    if( aOutlines.Outline( jj ).PointInside( holePt ) )
                    {
                        aOutlines.AddHole( pad_hole, jj );
                        break;
                    }
                }
            }
        }
    }

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


const std::vector<BOARD_CONNECTED_ITEM*> BOARD::AllConnectedItems()
{
    std::vector<BOARD_CONNECTED_ITEM*> items;

    for( PCB_TRACK* track : Tracks() )
        items.push_back( track );

    for( FOOTPRINT* footprint : Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            items.push_back( pad );
    }

    for( ZONE* zone : Zones() )
        items.push_back( zone );

    for( BOARD_ITEM* item : Drawings() )
    {
        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            items.push_back( bci );
    }

    return items;
}


void BOARD::MapNets( BOARD* aDestBoard )
{
    for( BOARD_CONNECTED_ITEM* item : AllConnectedItems() )
    {
        NETINFO_ITEM* netInfo = aDestBoard->FindNet( item->GetNetname() );

        if( netInfo )
            item->SetNet( netInfo );
        else
        {
            NETINFO_ITEM* newNet = new NETINFO_ITEM( aDestBoard, item->GetNetname() );
            aDestBoard->Add( newNet );
            item->SetNet( newNet );

        }
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


void BOARD::RemoveAllListeners()
{
    m_listeners.clear();
}


void BOARD::OnItemChanged( BOARD_ITEM* aItem )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardItemChanged, *this, aItem );
}


void BOARD::OnItemsChanged( std::vector<BOARD_ITEM*>& aItems )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardItemsChanged, *this, aItems );
}


void BOARD::OnItemsCompositeUpdate( std::vector<BOARD_ITEM*>& aAddedItems,
                                    std::vector<BOARD_ITEM*>& aRemovedItems,
                                    std::vector<BOARD_ITEM*>& aChangedItems )
{
    InvokeListeners( &BOARD_LISTENER::OnBoardCompositeUpdate, *this, aAddedItems, aRemovedItems,
                     aChangedItems );
}


void BOARD::OnRatsnestChanged()
{
    InvokeListeners( &BOARD_LISTENER::OnBoardRatsnestChanged, *this );
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

    // Initialize set of groups and generators to check that could participate in a cycle.
    for( PCB_GROUP* group : Groups() )
        toCheckGroups.insert( group );

    for( PCB_GENERATOR* gen : Generators() )
        toCheckGroups.insert( gen );

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
        if( BOARD_ITEM* board_item = dynamic_cast<BOARD_ITEM*>( item ) )
        {
            if( board_item->Type() == PCB_GROUP_T )
                hasGroup = true;

            if( board_item->GetParentGroup() )
                hasMember = true;
        }
    }

    GroupLegalOpsField legalOps;

    legalOps.create      = true;
    legalOps.removeItems = hasMember;
    legalOps.ungroup     = hasGroup;

    return legalOps;
}


bool BOARD::cmp_items::operator() ( const BOARD_ITEM* a, const BOARD_ITEM* b ) const
{
    if( a->Type() != b->Type() )
        return a->Type() < b->Type();

    if( a->GetLayer() != b->GetLayer() )
        return a->GetLayer() < b->GetLayer();

    if( a->GetPosition().x != b->GetPosition().x )
        return a->GetPosition().x < b->GetPosition().x;

    if( a->GetPosition().y != b->GetPosition().y )
        return a->GetPosition().y < b->GetPosition().y;

    if( a->m_Uuid != b->m_Uuid )    // shopuld be always the case foer valid boards
        return a->m_Uuid < b->m_Uuid;

    return a < b;
}


bool BOARD::cmp_drawings::operator()( const BOARD_ITEM* aFirst,
                                      const BOARD_ITEM* aSecond ) const
{
    if( aFirst->Type() != aSecond->Type() )
        return aFirst->Type() < aSecond->Type();

    if( aFirst->GetLayer() != aSecond->GetLayer() )
        return aFirst->GetLayer() < aSecond->GetLayer();

    if( aFirst->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aFirst );
        const PCB_SHAPE* other = static_cast<const PCB_SHAPE*>( aSecond );
        return shape->Compare( other );
    }
    else if( aFirst->Type() == PCB_TEXT_T || aFirst->Type() == PCB_FIELD_T )
    {
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( aFirst );
        const PCB_TEXT* other = static_cast<const PCB_TEXT*>( aSecond );
        return text->Compare( other );
    }
    else if( aFirst->Type() == PCB_TEXTBOX_T )
    {
        const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( aFirst );
        const PCB_TEXTBOX* other = static_cast<const PCB_TEXTBOX*>( aSecond );

        return textbox->PCB_SHAPE::Compare( other ) && textbox->EDA_TEXT::Compare( other );
    }
    else if( aFirst->Type() == PCB_TABLE_T )
    {
        const PCB_TABLE* table = static_cast<const PCB_TABLE*>( aFirst );
        const PCB_TABLE* other = static_cast<const PCB_TABLE*>( aSecond );

        return PCB_TABLE::Compare( table, other );
    }

    return aFirst->m_Uuid < aSecond->m_Uuid;
}


void BOARD::ConvertBrdLayerToPolygonalContours( PCB_LAYER_ID aLayer,
                                                SHAPE_POLY_SET& aOutlines ) const
{
    int maxError = GetDesignSettings().m_MaxError;

    // convert tracks and vias:
    for( const PCB_TRACK* track : m_tracks )
    {
        if( !track->IsOnLayer( aLayer ) )
            continue;

        track->TransformShapeToPolygon( aOutlines, aLayer, 0, maxError, ERROR_INSIDE );
    }

    // convert pads and other copper items in footprints
    for( const FOOTPRINT* footprint : m_footprints )
    {
        footprint->TransformPadsToPolySet( aOutlines, aLayer, 0, maxError, ERROR_INSIDE );

        footprint->TransformFPShapesToPolySet( aOutlines, aLayer, 0, maxError, ERROR_INSIDE,
                                               true, /* include text */
                                               true, /* include shapes */
                                               false /* include private items */ );

        for( const ZONE* zone : footprint->Zones() )
        {
            if( zone->GetLayerSet().test( aLayer ) )
                zone->TransformSolidAreasShapesToPolygon( aLayer, aOutlines );
        }
    }

    // convert copper zones
    for( const ZONE* zone : Zones() )
    {
        if( zone->GetLayerSet().test( aLayer ) )
            zone->TransformSolidAreasShapesToPolygon( aLayer, aOutlines );
    }

    // convert graphic items on copper layers (texts)
    for( const BOARD_ITEM* item : m_drawings )
    {
        if( !item->IsOnLayer( aLayer ) )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );
            shape->TransformShapeToPolygon( aOutlines, aLayer, 0, maxError, ERROR_INSIDE );
            break;
        }

        case PCB_FIELD_T:
        case PCB_TEXT_T:
        {
            const PCB_TEXT* text = static_cast<const PCB_TEXT*>( item );
            text->TransformTextToPolySet( aOutlines, 0, maxError, ERROR_INSIDE );
            break;
        }

        case PCB_TEXTBOX_T:
        {
            const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( item );

            // border
            textbox->PCB_SHAPE::TransformShapeToPolygon( aOutlines, aLayer, 0, maxError,
                                                         ERROR_INSIDE );
            // text
            textbox->TransformTextToPolySet( aOutlines, 0, maxError, ERROR_INSIDE );
            break;
        }

        case PCB_TABLE_T:
        {
            const PCB_TABLE* table = static_cast<const PCB_TABLE*>( item );

            table->TransformShapeToPolygon( aOutlines, aLayer, 0, maxError, ERROR_INSIDE );
            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
        {
            const PCB_DIMENSION_BASE* dim = static_cast<const PCB_DIMENSION_BASE*>( item );

            dim->TransformShapeToPolygon( aOutlines, aLayer, 0, maxError, ERROR_INSIDE );
            dim->TransformTextToPolySet( aOutlines, 0, maxError, ERROR_INSIDE );
            break;
        }

        default:
            break;
        }
    }
}


const BOARD_ITEM_SET BOARD::GetItemSet()
{
    BOARD_ITEM_SET items;

    std::copy( m_tracks.begin(), m_tracks.end(), std::inserter( items, items.end() ) );
    std::copy( m_zones.begin(), m_zones.end(), std::inserter( items, items.end() ) );
    std::copy( m_footprints.begin(), m_footprints.end(), std::inserter( items, items.end() ) );
    std::copy( m_drawings.begin(), m_drawings.end(), std::inserter( items, items.end() ) );
    std::copy( m_markers.begin(), m_markers.end(), std::inserter( items, items.end() ) );
    std::copy( m_groups.begin(), m_groups.end(), std::inserter( items, items.end() ) );

    return items;
}


bool BOARD::operator==( const BOARD_ITEM& aItem ) const
{
    if( aItem.Type() != Type() )
        return false;

    const BOARD& other = static_cast<const BOARD&>( aItem );

    if( *m_designSettings != *other.m_designSettings )
        return false;

    if( m_NetInfo.GetNetCount() != other.m_NetInfo.GetNetCount() )
        return false;

    const NETNAMES_MAP& thisNetNames = m_NetInfo.NetsByName();
    const NETNAMES_MAP& otherNetNames = other.m_NetInfo.NetsByName();

    for( auto it1 = thisNetNames.begin(), it2 = otherNetNames.begin();
         it1 != thisNetNames.end() && it2 != otherNetNames.end(); ++it1, ++it2 )
    {
        // We only compare the names in order here, not the index values
        // as the index values are auto-generated and the names are not.
        if( it1->first != it2->first )
            return false;
    }

    if( m_properties.size() != other.m_properties.size() )
        return false;

    for( auto it1 = m_properties.begin(), it2 = other.m_properties.begin();
         it1 != m_properties.end() && it2 != other.m_properties.end(); ++it1, ++it2 )
    {
        if( *it1 != *it2 )
            return false;
    }

    if( m_paper.GetCustomHeightMils() != other.m_paper.GetCustomHeightMils() )
        return false;

    if( m_paper.GetCustomWidthMils() != other.m_paper.GetCustomWidthMils() )
        return false;

    if( m_paper.GetSizeMils() != other.m_paper.GetSizeMils() )
        return false;

    if( m_paper.GetPaperId() != other.m_paper.GetPaperId() )
        return false;

    if( m_paper.GetWxOrientation() != other.m_paper.GetWxOrientation() )
        return false;

    for( int ii = 0; !m_titles.GetComment( ii ).empty(); ++ii )
    {
        if( m_titles.GetComment( ii ) != other.m_titles.GetComment( ii ) )
            return false;
    }

    wxArrayString ourVars;
    m_titles.GetContextualTextVars( &ourVars );

    wxArrayString otherVars;
    other.m_titles.GetContextualTextVars( &otherVars );

    if( ourVars != otherVars )
        return false;

    return true;
}


