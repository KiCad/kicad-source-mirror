/**
 * @file class_board.cpp
 * @brief  BOARD class functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fctsys.h>
#include <common.h>
#include <kicad_string.h>
#include <pcb_base_frame.h>
#include <msgpanel.h>
#include <reporter.h>
#include <ratsnest_data.h>
#include <ratsnest_viewitem.h>
#include <ws_proxy_view_item.h>
#include <pcbnew.h>
#include <collectors.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>
#include <class_drawsegment.h>
#include <class_pcb_target.h>
#include <connectivity/connectivity_data.h>
#include <pgm_base.h>
#include <pcbnew_settings.h>

/**
 * A singleton item of this class is returned for a weak reference that no longer exists.
 * Its sole purpose is to flag the item as having been deleted.
 */
class DELETED_BOARD_ITEM : public BOARD_ITEM
{
public:
    DELETED_BOARD_ITEM() :
        BOARD_ITEM( nullptr, NOT_USED )
    {}

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override
    {
        return _( "(Deleted Item)" );
    }

    wxString GetClass() const override
    {
        return wxT( "DELETED_BOARD_ITEM" );
    }

    // pure virtuals:
    void SetPosition( const wxPoint& ) override {}

#if defined(DEBUG)
    void Show( int , std::ostream&  ) const override {}
#endif
};

DELETED_BOARD_ITEM* g_DeletedItem = nullptr;


/* This is an odd place for this, but CvPcb won't link if it is
 *  in class_board_item.cpp like I first tried it.
 */
wxPoint BOARD_ITEM::ZeroOffset( 0, 0 );

// Dummy settings used to initialize the board.
// This is needed because some APIs that make use of BOARD without the context of a frame or
// application, and so the BOARD needs to store a valid pointer to a PCBNEW_SETTINGS even if
// one hasn't been provided by the application.
static PCBNEW_SETTINGS dummyGeneralSettings;

BOARD::BOARD() :
        BOARD_ITEM_CONTAINER( (BOARD_ITEM*) NULL, PCB_T ),
        m_paper( PAGE_INFO::A4 ),
        m_NetInfo( this ),
        m_project( nullptr )
{
    // we have not loaded a board yet, assume latest until then.
    m_fileFormatVersionAtLoad = LEGACY_BOARD_FILE_VERSION;

    m_generalSettings = &dummyGeneralSettings;

    m_CurrentZoneContour = NULL;            // This ZONE_CONTAINER handle the
                                            // zone contour currently in progress

    BuildListOfNets();                      // prepare pad and netlist containers.

    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        m_Layer[layer].m_name = GetStandardLayerName( ToLAYER_ID( layer ) );

        if( IsCopperLayer( layer ) )
            m_Layer[layer].m_type = LT_SIGNAL;
        else
            m_Layer[layer].m_type = LT_UNDEFINED;
    }

    // Initialize default netclass.
    NETCLASSPTR defaultClass = m_designSettings.GetDefault();
    defaultClass->SetDescription( _( "This is the default net class." ) );
    m_designSettings.SetCurrentNetClass( defaultClass->GetName() );

    // Set sensible initial values for custom track width & via size
    m_designSettings.UseCustomTrackViaSize( false );
    m_designSettings.SetCustomTrackWidth( m_designSettings.GetCurrentTrackWidth() );
    m_designSettings.SetCustomViaSize( m_designSettings.GetCurrentViaSize() );
    m_designSettings.SetCustomViaDrill( m_designSettings.GetCurrentViaDrill() );

    // Initialize ratsnest
    m_connectivity.reset( new CONNECTIVITY_DATA() );
}


BOARD::~BOARD()
{
    while( m_ZoneDescriptorList.size() )
    {
        ZONE_CONTAINER* area_to_remove = m_ZoneDescriptorList[0];
        Delete( area_to_remove );
    }

    // Clean up the owned elements
    DeleteMARKERs();
    DeleteZONEOutlines();

    // Delete the modules
    for( auto m : m_modules )
        delete m;

    m_modules.clear();

    // Delete the tracks
    for( auto t : m_tracks )
        delete t;

    m_tracks.clear();

    // Delete the drawings
    for (auto d : m_drawings )
        delete d;

    m_drawings.clear();

    delete m_CurrentZoneContour;
    m_CurrentZoneContour = NULL;
}


void BOARD::BuildConnectivity()
{
    GetConnectivity()->Build( this );
}


const wxPoint BOARD::GetPosition() const
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
        PCB_LINE_T,
        PCB_DIMENSION_T,
        PCB_TARGET_T,
        PCB_VIA_T,
        PCB_TRACE_T,
        PCB_ARC_T,
        //        PCB_PAD_T,            Can't be at board level
        //        PCB_MODULE_TEXT_T,    Can't be at board level
        PCB_MODULE_T,
        PCB_ZONE_AREA_T,
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

    INSPECTOR_FUNC inspector = [aNetCode,&ret] ( EDA_ITEM* item, void* testData )
    {
        TRACK*  t = (TRACK*) item;

        if( t->GetNetCode() == aNetCode )
            ret.push_back( t );

        return SEARCH_RESULT::CONTINUE;
    };

    // visit this BOARD's TRACKs and VIAs with above TRACK INSPECTOR which
    // appends all in aNetCode to ret.
    Visit( inspector, NULL, GENERAL_COLLECTOR::Tracks  );

    return ret;
}


bool BOARD::SetLayerDescr( PCB_LAYER_ID aIndex, const LAYER& aLayer )
{
    if( unsigned( aIndex ) < arrayDim( m_Layer ) )
    {
        m_Layer[ aIndex ] = aLayer;
        return true;
    }

    return false;
}


const PCB_LAYER_ID BOARD::GetLayerID( const wxString& aLayerName ) const
{

    // Look for the BOARD specific copper layer names
    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if ( IsCopperLayer( layer ) && ( m_Layer[ layer ].m_name == aLayerName ) )
        {
            return ToLAYER_ID( layer );
        }
    }

    // Otherwise fall back to the system standard layer names
    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( GetStandardLayerName( ToLAYER_ID( layer ) ) == aLayerName )
        {
            return ToLAYER_ID( layer );
        }
    }

    return UNDEFINED_LAYER;
}

const wxString BOARD::GetLayerName( PCB_LAYER_ID aLayer ) const
{
    // All layer names are stored in the BOARD.
    if( IsLayerEnabled( aLayer ) )
    {
        // Standard names were set in BOARD::BOARD() but they may be
        // over-ridden by BOARD::SetLayerName().
        // For copper layers, return the actual copper layer name,
        // otherwise return the Standard English layer name.
        if( IsCopperLayer( aLayer ) )
            return m_Layer[aLayer].m_name;
    }

    return GetStandardLayerName( aLayer );
}

bool BOARD::SetLayerName( PCB_LAYER_ID aLayer, const wxString& aLayerName )
{
    if( !IsCopperLayer( aLayer ) )
        return false;

    if( aLayerName == wxEmptyString )
        return false;

    // no quote chars in the name allowed
    if( aLayerName.Find( wxChar( '"' ) ) != wxNOT_FOUND )
        return false;

    wxString nameTemp = aLayerName;

    // replace any spaces with underscores before we do any comparing
    nameTemp.Replace( wxT( " " ), wxT( "_" ) );

    if( IsLayerEnabled( aLayer ) )
    {
        m_Layer[aLayer].m_name = nameTemp;
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
        return m_Layer[aLayer].m_type;

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
        m_Layer[aLayer].m_type = aLayerType;
        return true;
    }

    return false;
}


const char* LAYER::ShowType( LAYER_T aType )
{
    const char* cp;

    switch( aType )
    {
    default:
    case LT_SIGNAL:
        cp = "signal";
        break;

    case LT_POWER:
        cp = "power";
        break;

    case LT_MIXED:
        cp = "mixed";
        break;

    case LT_JUMPER:
        cp = "jumper";
        break;
    }

    return cp;
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
    return m_designSettings.GetCopperLayerCount();
}


void BOARD::SetCopperLayerCount( int aCount )
{
    m_designSettings.SetCopperLayerCount( aCount );
}


LSET BOARD::GetEnabledLayers() const
{
    return m_designSettings.GetEnabledLayers();
}


LSET BOARD::GetVisibleLayers() const
{
    return m_designSettings.GetVisibleLayers();
}


void BOARD::SetEnabledLayers( LSET aLayerSet )
{
    m_designSettings.SetEnabledLayers( aLayerSet );
}


void BOARD::SetVisibleLayers( LSET aLayerSet )
{
    m_designSettings.SetVisibleLayers( aLayerSet );
}


void BOARD::SetVisibleElements( int aMask )
{
    // Call SetElementVisibility for each item
    // to ensure specific calculations that can be needed by some items,
    // just changing the visibility flags could be not sufficient.
    for( GAL_LAYER_ID ii = GAL_LAYER_ID_START; ii < GAL_LAYER_ID_BITMASK_END; ++ii )
    {
        int item_mask = 1 << GAL_LAYER_INDEX( ii );
        SetElementVisibility( ii, aMask & item_mask );
    }
}


void BOARD::SetVisibleAlls()
{
    SetVisibleLayers( LSET().set() );

    // Call SetElementVisibility for each item,
    // to ensure specific calculations that can be needed by some items
    for( GAL_LAYER_ID ii = GAL_LAYER_ID_START; ii < GAL_LAYER_ID_BITMASK_END; ++ii )
        SetElementVisibility( ii, true );
}


int BOARD::GetVisibleElements() const
{
    return m_designSettings.GetVisibleElements();
}


bool BOARD::IsElementVisible( GAL_LAYER_ID aLayer ) const
{
    return m_designSettings.IsElementVisible( aLayer );
}


void BOARD::SetElementVisibility( GAL_LAYER_ID aLayer, bool isEnabled )
{
    m_designSettings.SetElementVisibility( aLayer, isEnabled );

    switch( aLayer )
    {
    case LAYER_RATSNEST:
    {
        // because we have a tool to show/hide ratsnest relative to a pad or a module
        // so the hide/show option is a per item selection

        for( auto track : Tracks() )
            track->SetLocalRatsnestVisible( isEnabled );

        for( auto mod : Modules() )
        {
            for( auto pad : mod->Pads() )
                pad->SetLocalRatsnestVisible( isEnabled );
        }

        for( int i = 0; i<GetAreaCount(); i++ )
        {
            auto zone = GetArea( i );
            zone->SetLocalRatsnestVisible( isEnabled );
        }

        break;
    }

    default:
        ;
    }
}


bool BOARD::IsModuleLayerVisible( PCB_LAYER_ID aLayer )
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
        m_markers.push_back( (MARKER_PCB*) aBoardItem );
        break;

    // this one uses a vector
    case PCB_ZONE_AREA_T:
        m_ZoneDescriptorList.push_back( (ZONE_CONTAINER*) aBoardItem );
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

        if( aMode == ADD_MODE::APPEND )
            m_tracks.push_back( static_cast<TRACK*>( aBoardItem ) );
        else
            m_tracks.push_front( static_cast<TRACK*>( aBoardItem ) );

        break;

    case PCB_MODULE_T:
        if( aMode == ADD_MODE::APPEND )
            m_modules.push_back( (MODULE*) aBoardItem );
        else
            m_modules.push_front( (MODULE*) aBoardItem );

        break;

    case PCB_BARCODE_T:
    case PCB_DIMENSION_T:
    case PCB_LINE_T:
    case PCB_TEXT_T:
    case PCB_TARGET_T:
        if( aMode == ADD_MODE::APPEND )
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

    InvokeListeners( &BOARD_LISTENER::OnBoardItemAdded, *this, aBoardItem );
}


void BOARD::Remove( BOARD_ITEM* aBoardItem )
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

        // find the item in the vector, then remove it
        for( unsigned i = 0; i<m_markers.size(); ++i )
        {
            if( m_markers[i] == (MARKER_PCB*) aBoardItem )
            {
                m_markers.erase( m_markers.begin() + i );
                break;
            }
        }

        break;

    case PCB_ZONE_AREA_T:    // this one uses a vector
        // find the item in the vector, then delete then erase it.
        for( unsigned i = 0; i<m_ZoneDescriptorList.size(); ++i )
        {
            if( m_ZoneDescriptorList[i] == (ZONE_CONTAINER*) aBoardItem )
            {
                m_ZoneDescriptorList.erase( m_ZoneDescriptorList.begin() + i );
                break;
            }
        }
        break;

    case PCB_MODULE_T:
        m_modules.erase( std::remove_if( m_modules.begin(), m_modules.end(),
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

    case PCB_DIMENSION_T:
    case PCB_LINE_T:
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

    m_connectivity->Remove( aBoardItem );

    InvokeListeners( &BOARD_LISTENER::OnBoardItemRemoved, *this, aBoardItem );
}


wxString BOARD::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "PCB" ) );
}


void BOARD::DeleteMARKERs()
{
    // the vector does not know how to delete the MARKER_PCB, it holds pointers
    for( MARKER_PCB* marker : m_markers )
        delete marker;

    m_markers.clear();
}


void BOARD::DeleteZONEOutlines()
{
    // the vector does not know how to delete the ZONE Outlines, it holds pointers
    for( ZONE_CONTAINER* zone : m_ZoneDescriptorList )
        delete zone;

    m_ZoneDescriptorList.clear();
}


BOARD_ITEM* BOARD::GetItem( const KIID& aID )
{
    if( aID == niluuid )
        return nullptr;

    for( TRACK* track : Tracks() )
        if( track->m_Uuid == aID )
            return track;

    for( MODULE* module : Modules() )
    {
        if( module->m_Uuid == aID )
            return module;

        for( D_PAD* pad : module->Pads() )
            if( pad->m_Uuid == aID )
                return pad;

        if( module->Reference().m_Uuid == aID )
            return &module->Reference();

        if( module->Value().m_Uuid == aID )
            return &module->Value();

        for( BOARD_ITEM* drawing : module->GraphicalItems() )
            if( drawing->m_Uuid == aID )
                return drawing;
    }

    for( ZONE_CONTAINER* zone : Zones() )
        if( zone->m_Uuid == aID )
            return zone;

    for( BOARD_ITEM* drawing : Drawings() )
        if( drawing->m_Uuid == aID )
            return drawing;

    for( MARKER_PCB* marker : m_markers )
        if( marker->m_Uuid == aID )
            return marker;

    // Not found; weak reference has been deleted.
    if( !g_DeletedItem )
        g_DeletedItem = new DELETED_BOARD_ITEM();

    return g_DeletedItem;
}


void BOARD::FillItemMap( std::map<KIID, EDA_ITEM*>& aMap )
{
    for( TRACK* track : Tracks() )
        aMap[ track->m_Uuid ] = track;

    for( MODULE* module : Modules() )
    {
        aMap[ module->m_Uuid ] = module;

        for( D_PAD* pad : module->Pads() )
            aMap[ pad->m_Uuid ] = pad;

        aMap[ module->Reference().m_Uuid ] = &module->Reference();
        aMap[ module->Value().m_Uuid ] = &module->Value();

        for( BOARD_ITEM* drawing : module->GraphicalItems() )
            aMap[ drawing->m_Uuid ] = drawing;
    }

    for( ZONE_CONTAINER* zone : Zones() )
        aMap[ zone->m_Uuid ] = zone;

    for( BOARD_ITEM* drawing : Drawings() )
        aMap[ drawing->m_Uuid ] = drawing;

    for( MARKER_PCB* marker : m_markers )
        aMap[ marker->m_Uuid ] = marker;
}


unsigned BOARD::GetNodesCount( int aNet )
{
    unsigned retval = 0;
    for( auto mod : Modules() )
    {
        for( auto pad : mod->Pads() )
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

    // Check segments, dimensions, texts, and fiducials
    for( auto item : m_drawings )
    {
        if( aBoardEdgesOnly && ( item->GetLayer() != Edge_Cuts ) )
            continue;

        if( ( item->GetLayerSet() & visible ).any() )
            area.Merge( item->GetBoundingBox() );
    }

    // Check modules
    for( auto module : m_modules )
    {
        if( !( module->GetLayerSet() & visible ).any() )
            continue;

        if( aBoardEdgesOnly )
        {
            for( const auto edge : module->GraphicalItems() )
            {
                if( edge->GetLayer() == Edge_Cuts )
                    area.Merge( edge->GetBoundingBox() );
            }
        }
        else
        {
            area.Merge( module->GetBoundingBox( showInvisibleText ) );
        }
    }

    if( !aBoardEdgesOnly )
    {
        // Check tracks
        for( auto track : m_tracks )
        {
            if( ( track->GetLayerSet() & visible ).any() )
                area.Merge( track->GetBoundingBox() );
        }

        // Check zones
        for( auto aZone : m_ZoneDescriptorList )
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

    for( auto item : m_tracks )
    {
        if( item->Type() == PCB_VIA_T )
            viasCount++;
        else
            trackSegmentsCount++;
    }

    txt.Printf( wxT( "%d" ), GetPadCount() );
    aList.emplace_back( _( "Pads" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), viasCount );
    aList.emplace_back( _( "Vias" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), trackSegmentsCount );
    aList.emplace_back( _( "Track Segments" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), GetNodesCount() );
    aList.emplace_back( _( "Nodes" ), txt, DARKCYAN );

    txt.Printf( wxT( "%d" ), m_NetInfo.GetNetCount() - 1 /* Don't include "No Net" in count */ );
    aList.emplace_back( _( "Nets" ), txt, RED );

    txt.Printf( wxT( "%d" ), GetConnectivity()->GetUnconnectedCount() );
    aList.emplace_back( _( "Unrouted" ), txt, BLUE );
}


SEARCH_RESULT BOARD::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_RESULT::CONTINUE;
    const KICAD_T* p    = scanTypes;
    bool           done = false;

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

        /*  Instances of the requested KICAD_T live in a list, either one
         *   that I manage, or that my modules manage.  If it's a type managed
         *   by class MODULE, then simply pass it on to each module's
         *   MODULE::Visit() function by way of the
         *   IterateForward( m_Modules, ... ) call.
         */

        case PCB_MODULE_T:
        case PCB_PAD_T:
        case PCB_MODULE_TEXT_T:
        case PCB_MODULE_EDGE_T:

            // this calls MODULE::Visit() on each module.
            result = IterateForward<MODULE*>( m_modules, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_MODULE_T:
                case PCB_PAD_T:
                case PCB_MODULE_TEXT_T:
                case PCB_MODULE_EDGE_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        case PCB_LINE_T:
        case PCB_TEXT_T:
        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
            result = IterateForward<BOARD_ITEM*>( m_drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_LINE_T:
                case PCB_TEXT_T:
                case PCB_DIMENSION_T:
                case PCB_TARGET_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            ;
            break;

#if 0   // both these are on same list, so we must scan it twice in order
        // to get VIA priority, using new #else code below.
        // But we are not using separate lists for TRACKs and VIA, because
        // items are ordered (sorted) in the linked
        // list by netcode AND by physical distance:
        // when created, if a track or via is connected to an existing track or
        // via, it is put in linked list after this existing track or via
        // So usually, connected tracks or vias are grouped in this list
        // So the algorithm (used in ratsnest computations) which computes the
        // track connectivity is faster (more than 100 time regarding to
        // a non ordered list) because when it searches for a connection, first
        // it tests the near (near in term of linked list) 50 items
        // from the current item (track or via) in test.
        // Usually, because of this sort, a connected item (if exists) is
        // found.
        // If not found (and only in this case) an exhaustive (and time
        // consuming) search is made, but this case is statistically rare.
        case PCB_VIA_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
            result = IterateForward( m_Track, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_VIA_T:
                case PCB_TRACE_T:
                case PCB_ARC_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

#else
        case PCB_VIA_T:
            result = IterateForward<TRACK*>( m_tracks, inspector, testData, p );
            ++p;
            break;

        case PCB_TRACE_T:
        case PCB_ARC_T:
            result = IterateForward<TRACK*>( m_tracks, inspector, testData, p );
            ++p;
            break;
#endif

        case PCB_MARKER_T:

            // MARKER_PCBS are in the m_markers std::vector
            for( unsigned i = 0; i<m_markers.size(); ++i )
            {
                result = m_markers[i]->Visit( inspector, testData, p );

                if( result == SEARCH_RESULT::QUIT )
                    break;
            }

            ++p;
            break;

        case PCB_ZONE_AREA_T:

            // PCB_ZONE_AREA_T are in the m_ZoneDescriptorList std::vector
            for( unsigned i = 0; i< m_ZoneDescriptorList.size(); ++i )
            {
                result = m_ZoneDescriptorList[i]->Visit( inspector, testData, p );

                if( result == SEARCH_RESULT::QUIT )
                    break;
            }

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


MODULE* BOARD::FindModuleByReference( const wxString& aReference ) const
{
    MODULE* found = nullptr;

    // search only for MODULES
    static const KICAD_T scanTypes[] = { PCB_MODULE_T, EOT };

    INSPECTOR_FUNC inspector = [&] ( EDA_ITEM* item, void* testData )
    {
        MODULE* module = (MODULE*) item;

        if( aReference == module->GetReference() )
        {
            found = module;
            return SEARCH_RESULT::QUIT;
        }

        return SEARCH_RESULT::CONTINUE;
    };

    // visit this BOARD with the above inspector
    BOARD* nonconstMe = (BOARD*) this;
    nonconstMe->Visit( inspector, NULL, scanTypes );

    return found;
}


MODULE* BOARD::FindModuleByPath( const KIID_PATH& aPath ) const
{
    for( MODULE* module : m_modules )
    {
        if( module->GetPath() == aPath )
            return module;
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
    int countA = padCountListByNet[a->GetNet()];
    int countB = padCountListByNet[b->GetNet()];

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
        auto netcode = net->GetNet();

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
        std::vector<D_PAD*> pads = GetPads();

        padCountListByNet.assign( max_netcode + 1, 0 );

        for( D_PAD* pad : pads )
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


ZONE_CONTAINER* BOARD::HitTestForAnyFilledArea( const wxPoint& aRefPos,
    PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer,  int aNetCode )
{
    if( aEndLayer < 0 )
        aEndLayer = aStartLayer;

    if( aEndLayer <  aStartLayer )
        std::swap( aEndLayer, aStartLayer );

    for( ZONE_CONTAINER* area : m_ZoneDescriptorList )
    {
        if( area->GetLayer() < aStartLayer || area->GetLayer() > aEndLayer )
            continue;

        // In locate functions we must skip tagged items with BUSY flag set.
        if( area->GetState( BUSY ) )
            continue;

        if( aNetCode >= 0 && area->GetNetCode() != aNetCode )
            continue;

        if( area->HitTestFilledArea( aRefPos ) )
            return area;
    }

    return NULL;
}


int BOARD::SetAreasNetCodesFromNetNames()
{
    int error_count = 0;

    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* it = GetArea( ii );

        if( !it->IsOnCopperLayer() )
        {
            it->SetNetCode( NETINFO_LIST::UNCONNECTED );
            continue;
        }

        if( it->GetNetCode() != 0 )      // i.e. if this zone is connected to a net
        {
            const NETINFO_ITEM* net = it->GetNet();

            if( net )
            {
                it->SetNetCode( net->GetNet() );
            }
            else
            {
                error_count++;

                // keep Net Name and set m_NetCode to -1 : error flag.
                it->SetNetCode( -1 );
            }
        }
    }

    return error_count;
}


D_PAD* BOARD::GetPad( const wxPoint& aPosition, LSET aLayerSet )
{
    if( !aLayerSet.any() )
        aLayerSet = LSET::AllCuMask();

    for( auto module : m_modules )
    {
        D_PAD* pad = NULL;

        if( module->HitTest( aPosition ) )
            pad = module->GetPad( aPosition, aLayerSet );

        if( pad )
            return pad;
    }

    return NULL;
}


D_PAD* BOARD::GetPad( TRACK* aTrace, ENDPOINT_T aEndPoint )
{
    const wxPoint& aPosition = aTrace->GetEndPoint( aEndPoint );

    LSET lset( aTrace->GetLayer() );

    return GetPad( aPosition, lset );
}


D_PAD* BOARD::GetPadFast( const wxPoint& aPosition, LSET aLayerSet )
{
    for( auto mod : Modules() )
    {
        for ( auto pad : mod->Pads() )
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


D_PAD* BOARD::GetPad( std::vector<D_PAD*>& aPadList, const wxPoint& aPosition, LSET aLayerSet )
{
    // Search aPadList for aPosition
    // aPadList is sorted by X then Y values, and a fast binary search is used
    int idxmax = aPadList.size()-1;

    int delta = aPadList.size();

    int idx = 0;        // Starting index is the beginning of list

    while( delta )
    {
        // Calculate half size of remaining interval to test.
        // Ensure the computed value is not truncated (too small)
        if( (delta & 1) && ( delta > 1 ) )
            delta++;

        delta /= 2;

        D_PAD* pad = aPadList[idx];

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
            for(  int ii = idx-1 ;ii >=0; ii-- )
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
 * Function SortPadsByXCoord
 * is used by GetSortedPadListByXCoord to Sort a pad list by x coordinate value.
 * This function is used to build ordered pads lists
 */
bool sortPadsByXthenYCoord( D_PAD* const & ref, D_PAD* const & comp )
{
    if( ref->GetPosition().x == comp->GetPosition().x )
        return ref->GetPosition().y < comp->GetPosition().y;
    return ref->GetPosition().x < comp->GetPosition().x;
}


void BOARD::GetSortedPadListByXthenYCoord( std::vector<D_PAD*>& aVector, int aNetCode )
{
    for ( auto mod : Modules() )
    {
        for ( auto pad : mod->Pads( ) )
        {
            if( aNetCode < 0 ||  pad->GetNetCode() == aNetCode )
            {
                aVector.push_back( pad );
            }
        }
    }

    std::sort( aVector.begin(), aVector.end(), sortPadsByXthenYCoord );
}


void BOARD::PadDelete( D_PAD* aPad )
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

    for( auto item : connectivity->GetConnectedItems(
                 static_cast<const BOARD_CONNECTED_ITEM*>( &aTrack ), types ) )
    {
        count++;

        if( auto track = dyn_cast<TRACK*>( item ) )
        {
            bool inPad = false;

            for( auto pad_it : connectivity->GetConnectedPads( item ) )
            {
                auto pad = static_cast<D_PAD*>( pad_it );

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
        else if( auto pad = dyn_cast<D_PAD*>( item ) )
            package_length += pad->GetPadToDieLength();
    }

    return std::make_tuple( count, length, package_length );
}


MODULE* BOARD::GetFootprint( const wxPoint& aPosition, PCB_LAYER_ID aActiveLayer,
                             bool aVisibleOnly, bool aIgnoreLocked )
{
    MODULE* module      = NULL;
    MODULE* alt_module  = NULL;
    int     min_dim     = 0x7FFFFFFF;
    int     alt_min_dim = 0x7FFFFFFF;
    bool    current_layer_back = IsBackLayer( aActiveLayer );

    for( auto pt_module : m_modules )
    {
        // is the ref point within the module's bounds?
        if( !pt_module->HitTest( aPosition ) )
            continue;

        // if caller wants to ignore locked modules, and this one is locked, skip it.
        if( aIgnoreLocked && pt_module->IsLocked() )
            continue;

        PCB_LAYER_ID layer = pt_module->GetLayer();

        // Filter non visible modules if requested
        if( !aVisibleOnly || IsModuleLayerVisible( layer ) )
        {
            EDA_RECT bb = pt_module->GetFootprintRect();

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
                    module  = pt_module;
                    min_dim = dist;
                }
            }
            else if( aVisibleOnly && IsModuleLayerVisible( layer ) )
            {
                if( dist <= alt_min_dim )
                {
                    // better footprint shown on the other side
                    alt_module  = pt_module;
                    alt_min_dim = dist;
                }
            }
        }
    }

    if( module )
    {
        return module;
    }

    if( alt_module)
    {
        return alt_module;
    }

    return NULL;
}

std::list<ZONE_CONTAINER*> BOARD::GetZoneList( bool aIncludeZonesInFootprints )
{
    std::list<ZONE_CONTAINER*> zones;

    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        zones.push_back( GetArea( ii ) );
    }

    if( aIncludeZonesInFootprints )
    {
        for( MODULE* mod : m_modules )
        {
            for( MODULE_ZONE_CONTAINER* zone : mod->Zones() )
            {
                zones.push_back( zone );
            }
        }
    }

    return zones;
}


ZONE_CONTAINER* BOARD::AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode, PCB_LAYER_ID aLayer,
        wxPoint aStartPointPosition, ZONE_HATCH_STYLE aHatch )
{
    ZONE_CONTAINER* new_area = InsertArea( aNetcode,
                                           m_ZoneDescriptorList.size( ) - 1,
                                           aLayer, aStartPointPosition.x,
                                           aStartPointPosition.y, aHatch );

    if( aNewZonesList )
    {
        ITEM_PICKER picker( new_area, UR_NEW );
        aNewZonesList->PushItem( picker );
    }

    return new_area;
}


void BOARD::RemoveArea( PICKED_ITEMS_LIST* aDeletedList, ZONE_CONTAINER* area_to_remove )
{
    if( area_to_remove == NULL )
        return;

    if( aDeletedList )
    {
        ITEM_PICKER picker( area_to_remove, UR_DELETED );
        aDeletedList->PushItem( picker );
        Remove( area_to_remove );   // remove from zone list, but does not delete it
    }
    else
    {
        Delete( area_to_remove );
    }
}


ZONE_CONTAINER* BOARD::InsertArea( int aNetcode, int aAreaIdx, PCB_LAYER_ID aLayer, int aCornerX,
        int aCornerY, ZONE_HATCH_STYLE aHatch )
{
    ZONE_CONTAINER* new_area = (ZONE_CONTAINER*) this->Duplicate();

    new_area->SetNetCode( aNetcode );
    new_area->SetLayer( aLayer );

    if( aAreaIdx < (int) ( m_ZoneDescriptorList.size() - 1 ) )
        m_ZoneDescriptorList.insert( m_ZoneDescriptorList.begin() + aAreaIdx + 1, new_area );
    else
        m_ZoneDescriptorList.push_back( new_area );

    new_area->SetHatchStyle( (ZONE_HATCH_STYLE) aHatch );

    // Add the first corner to the new zone
    new_area->AppendCorner( wxPoint( aCornerX, aCornerY ), -1 );

    return new_area;
}


bool BOARD::NormalizeAreaPolygon( PICKED_ITEMS_LIST * aNewZonesList, ZONE_CONTAINER* aCurrArea )
{
    // mark all areas as unmodified except this one, if modified
    for( ZONE_CONTAINER* zone : m_ZoneDescriptorList )
        zone->SetLocalFlags( 0 );

    aCurrArea->SetLocalFlags( 1 );

    if( aCurrArea->Outline()->IsSelfIntersecting() )
    {
        aCurrArea->UnHatch();

        // Normalize copied area and store resulting number of polygons
        int n_poly = aCurrArea->Outline()->NormalizeAreaOutlines();

        // If clipping has created some polygons, we must add these new copper areas.
        if( n_poly > 1 )
        {
            ZONE_CONTAINER* NewArea;

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
                NewArea->Hatch();
                NewArea->SetLocalFlags( 1 );
            }

            SHAPE_POLY_SET* new_p = new SHAPE_POLY_SET( aCurrArea->Outline()->UnitSet( 0 ) );
            delete aCurrArea->Outline();
            aCurrArea->SetOutline( new_p );
        }
    }

    aCurrArea->Hatch();

    return true;
}


/* Extracts the board outlines and build a closed polygon
 * from lines, arcs and circle items on edge cut layer
 * Any closed outline inside the main outline is a hole
 * All contours should be closed, i.e. are valid vertices for a closed polygon
 * return true if success, false if a contour is not valid
 */
extern bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                wxString* aErrorText, unsigned int aTolerance,
                                wxPoint* aErrorLocation = nullptr );


bool BOARD::GetBoardPolygonOutlines( SHAPE_POLY_SET& aOutlines, wxString* aErrorText, wxPoint* aErrorLocation )
{

    bool success = BuildBoardPolygonOutlines( this, aOutlines, aErrorText,
            GetDesignSettings().m_MaxError, aErrorLocation );

    // Make polygon strictly simple to avoid issues (especially in 3D viewer)
    aOutlines.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    return success;
}


const std::vector<D_PAD*> BOARD::GetPads()
{
    std::vector<D_PAD*> allPads;

    for( MODULE* mod : Modules() )
    {
        for( D_PAD* pad : mod->Pads() )
            allPads.push_back( pad );
    }

    return allPads;
}


unsigned BOARD::GetPadCount()
{
    unsigned retval = 0;

    for( auto mod : Modules() )
        retval += mod->Pads().size();

    return retval;
}


/**
 * Function GetPad
 * @return D_PAD* - at the \a aIndex
 */
D_PAD* BOARD::GetPad( unsigned aIndex ) const
{
    unsigned count = 0;

    for( auto mod : m_modules )
    {
        for( auto pad : mod->Pads() )
        {
            if( count == aIndex )
                return pad;

            count++;
        }
    }

    return nullptr;
}


const std::vector<BOARD_CONNECTED_ITEM*> BOARD::AllConnectedItems()
{
    std::vector<BOARD_CONNECTED_ITEM*> items;

    for( auto track : Tracks() )
    {
        items.push_back( track );
    }

    for( auto mod : Modules() )
    {
        for( auto pad : mod->Pads() )
        {
            items.push_back( pad );
        }
    }

    for( int i = 0; i<GetAreaCount(); i++ )
    {
        auto zone = GetArea( i );
        items.push_back( zone );
    }

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
            item->SetNetCode( netInfo->GetNet() );
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
    if( std::find( m_listeners.begin(), m_listeners.end(), aListener ) == m_listeners.end() )
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


void BOARD::ResetNetHighLight()
{
    m_highLight.Clear();
    m_highLightPrevious.Clear();

    InvokeListeners( &BOARD_LISTENER::OnBoardHighlightNetChanged, *this );
}


void BOARD::SetHighLightNet( int aNetCode )
{
    if( m_highLight.m_netCode != aNetCode )
    {
        m_highLight.m_netCode = aNetCode;
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
