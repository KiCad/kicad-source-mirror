/**
 * @file class_board.cpp
 * @brief  BOARD class functions.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <limits.h>
#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <kicad_string.h>
#include <pcbcommon.h>
#include <wxBasePcbFrame.h>
#include <msgpanel.h>
#include <pcb_netlist.h>
#include <reporter.h>
#include <base_units.h>
#include <ratsnest_data.h>
#include <ratsnest_viewitem.h>
#include <worksheet_viewitem.h>

#include <pcbnew.h>
#include <colors_selection.h>
#include <collectors.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>


/* This is an odd place for this, but CvPcb won't link if it is
 *  in class_board_item.cpp like I first tried it.
 */
wxPoint BOARD_ITEM::ZeroOffset( 0, 0 );


BOARD::BOARD() :
    BOARD_ITEM( (BOARD_ITEM*) NULL, PCB_T ),
    m_NetInfo( this ),
    m_paper( PAGE_INFO::A4 )
{
    // we have not loaded a board yet, assume latest until then.
    m_fileFormatVersionAtLoad = LEGACY_BOARD_FILE_VERSION;

    m_Status_Pcb    = 0;                    // Status word: bit 1 = calculate.
    SetColorsSettings( &g_ColorsSettings );
    m_nodeCount     = 0;                    // Number of connected pads.
    m_unconnectedNetCount   = 0;            // Number of unconnected nets.

    m_CurrentZoneContour = NULL;            // This ZONE_CONTAINER handle the
                                            // zone contour currently in progress

    BuildListOfNets();                      // prepare pad and netlist containers.

    for( LAYER_NUM layer = FIRST_LAYER; layer < NB_LAYERS; ++layer )
    {
        m_Layer[layer].m_Name = GetStandardLayerName( layer );

        if( layer <= LAST_COPPER_LAYER )
            m_Layer[layer].m_Type = LT_SIGNAL;
        else
            m_Layer[layer].m_Type = LT_UNDEFINED;
    }

    NETCLASSPTR defaultClass = m_designSettings.GetDefault();
    defaultClass->SetDescription( _( "This is the default net class." ) );

    // Initialize default values in default netclass.
    defaultClass->SetParams( m_designSettings );
    m_designSettings.SetCurrentNetClass( defaultClass->GetName() );

    // Set sensible initial values for custom track width & via size
    m_designSettings.UseCustomTrackViaSize( false );
    m_designSettings.SetCustomTrackWidth( m_designSettings.GetCurrentTrackWidth() );
    m_designSettings.SetCustomViaSize( m_designSettings.GetCurrentViaSize() );
    m_designSettings.SetCustomViaDrill( m_designSettings.GetCurrentViaDrill() );

    // Initialize ratsnest
    m_ratsnest = new RN_DATA( this );
    m_ratsnestViewItem = new KIGFX::RATSNEST_VIEWITEM( m_ratsnest );

    // Initialize view item for displaying worksheet frame
    m_worksheetViewItem = new KIGFX::WORKSHEET_VIEWITEM( &m_paper, &m_titles );
    m_worksheetViewItem->SetFileName( std::string( m_fileName.mb_str() ) );
}


BOARD::~BOARD()
{
    while( m_ZoneDescriptorList.size() )
    {
        ZONE_CONTAINER* area_to_remove = m_ZoneDescriptorList[0];
        Delete( area_to_remove );
    }

    delete m_worksheetViewItem;
    delete m_ratsnestViewItem;
    delete m_ratsnest;

    m_FullRatsnest.clear();
    m_LocalRatsnest.clear();

    DeleteMARKERs();
    DeleteZONEOutlines();

    delete m_CurrentZoneContour;
    m_CurrentZoneContour = NULL;
}


const wxPoint& BOARD::GetPosition() const
{
    wxLogWarning( wxT( "This should not be called on the BOARD object") );

    return ZeroOffset;
}

void BOARD::SetPosition( const wxPoint& aPos )
{
    wxLogWarning( wxT( "This should not be called on the BOARD object") );
}


void BOARD::Move( const wxPoint& aMoveVector )        // overload
{
    // Implement 'interface INSPECTOR' which is only INSPECTOR::Inspect(),
    // here it does the moving.
    struct MOVER : public INSPECTOR
    {
        SEARCH_RESULT Inspect( EDA_ITEM* item, const void* data )
        {
            BOARD_ITEM*     brd_item = (BOARD_ITEM*) item;
            const wxPoint*  vector   = (const wxPoint*) data;

            brd_item->Move( *vector );

            return SEARCH_CONTINUE;
        }
    } inspector;

    // @todo : anything like this elsewhere?  maybe put into GENERAL_COLLECTOR class.
    static const KICAD_T top_level_board_stuff[] = {
        PCB_MARKER_T,
        PCB_TEXT_T,
        PCB_LINE_T,
        PCB_DIMENSION_T,
        PCB_TARGET_T,
        PCB_VIA_T,
        PCB_TRACE_T,
        //        PCB_PAD_T,
        //        PCB_MODULE_TEXT_T,
        PCB_MODULE_T,
        PCB_ZONE_AREA_T,
        EOT
    };

    // visit this BOARD with the above inspector, which moves all items.
    Visit( &inspector, &aMoveVector, top_level_board_stuff );
}


void BOARD::chainMarkedSegments( wxPoint aPosition, LAYER_MSK aLayerMask, TRACK_PTRS* aList )
{
    TRACK* segment;             // The current segment being analyzed.
    TRACK* via;                 // The via identified, eventually destroy
    TRACK* candidate;           // The end segment to destroy (or NULL = segment)
    int NbSegm;

    if( m_Track == NULL )
        return;

    /* Set the BUSY flag of all connected segments, first search starting at
     * aPosition.  The search ends when a pad is found (end of a track), a
     * segment end has more than one other segment end connected, or when no
     * connected item found.
     *
     * Vias are a special case because they must look for segments connected
     * on other layers and they change the layer mask.  They can be a track
     * end or not.  They will be analyzer later and vias on terminal points
     * of the track will be considered as part of this track if they do not
     * connect segments of an other track together and will be considered as
     * part of an other track when removing the via, the segments of that other
     * track are disconnected.
     */
    for( ; ; )
    {
        if( GetPad( aPosition, aLayerMask ) != NULL )
            return;

        /* Test for a via: a via changes the layer mask and can connect a lot
         * of segments at location aPosition. When found, the via is just
         * pushed in list.  Vias will be examined later, when all connected
         * segment are found and push in list.  This is because when a via
         * is found we do not know at this time the number of connected items
         * and we do not know if this via is on the track or finish the track
         */
        via = m_Track->GetVia( NULL, aPosition, aLayerMask );

        if( via )
        {
            aLayerMask = via->GetLayerMask();

            aList->push_back( via );
        }

        /* Now we search all segments connected to point aPosition
         *  if only 1 segment: this segment is candidate
         *  if > 1 segment:
         *      end of track (more than 2 segment connected at this location)
         */
        segment = m_Track; candidate = NULL;
        NbSegm  = 0;

        while( ( segment = ::GetTrack( segment, NULL, aPosition, aLayerMask ) ) != NULL )
        {
            if( segment->GetState( BUSY ) ) // already found and selected: skip it
            {
                segment = segment->Next();
                continue;
            }

            if( segment == via ) // just previously found: skip it
            {
                segment = segment->Next();
                continue;
            }

            NbSegm++;

            if( NbSegm == 1 ) /* First time we found a connected item: segment is candidate */
            {
                candidate = segment;
                segment = segment->Next();
            }
            else /* More than 1 segment connected -> this location is an end of the track */
            {
                return;
            }
        }

        if( candidate )      // A candidate is found: flag it an push it in list
        {
            /* Initialize parameters to search items connected to this
             * candidate:
             * we must analyze connections to its other end
             */
            aLayerMask = candidate->GetLayerMask();

            if( aPosition == candidate->GetStart() )
            {
                aPosition = candidate->GetEnd();
            }
            else
            {
                aPosition = candidate->GetStart();
            }

            segment = m_Track; /* restart list of tracks to analyze */

            /* flag this item an push it in list of selected items */
            aList->push_back( candidate );
            candidate->SetState( BUSY, true );
        }
        else
        {
            return;
        }
    }
}


void BOARD::PushHighLight()
{
    m_highLightPrevious = m_highLight;
}


void BOARD::PopHighLight()
{
    m_highLight = m_highLightPrevious;
    m_highLightPrevious.Clear();
}


bool BOARD::SetLayer( LAYER_NUM aIndex, const LAYER& aLayer )
{
    if( aIndex < NB_COPPER_LAYERS )
    {
        m_Layer[ aIndex ] = aLayer;
        return true;
    }

    return false;
}


wxString BOARD::GetLayerName( LAYER_NUM aLayer ) const
{
    if( !IsPcbLayer( aLayer ) )
        return wxEmptyString;

    // All layer names are stored in the BOARD.
    if( IsLayerEnabled( aLayer ) )
    {
        // Standard names were set in BOARD::BOARD() but they may be
        // over-ridden by BOARD::SetLayerName().
        // For copper layers, return the actual copper layer name,
        // otherwise return the Standard English layer name.
        if( IsCopperLayer( aLayer ) )
            return m_Layer[aLayer].m_Name;
    }

    return GetStandardLayerName( aLayer );
}


wxString BOARD::GetStandardLayerName( LAYER_NUM aLayerNumber )
{
    const wxChar* txt;

    // These are only default layer names.  For Pcbnew the copper names
    // may be over-ridden in the BOARD (*.brd) file.

    // Use a switch to explicitly show the mapping more clearly
    switch( aLayerNumber )
    {
    case LAYER_N_FRONT:         txt = wxT( "F.Cu" );            break;
    case LAYER_N_2:             txt = wxT( "Inner1.Cu" );       break;
    case LAYER_N_3:             txt = wxT( "Inner2.Cu" );       break;
    case LAYER_N_4:             txt = wxT( "Inner3.Cu" );       break;
    case LAYER_N_5:             txt = wxT( "Inner4.Cu" );       break;
    case LAYER_N_6:             txt = wxT( "Inner5.Cu" );       break;
    case LAYER_N_7:             txt = wxT( "Inner6.Cu" );       break;
    case LAYER_N_8:             txt = wxT( "Inner7.Cu" );       break;
    case LAYER_N_9:             txt = wxT( "Inner8.Cu" );       break;
    case LAYER_N_10:            txt = wxT( "Inner9.Cu" );       break;
    case LAYER_N_11:            txt = wxT( "Inner10.Cu" );      break;
    case LAYER_N_12:            txt = wxT( "Inner11.Cu" );      break;
    case LAYER_N_13:            txt = wxT( "Inner12.Cu" );      break;
    case LAYER_N_14:            txt = wxT( "Inner13.Cu" );      break;
    case LAYER_N_15:            txt = wxT( "Inner14.Cu" );      break;
    case LAYER_N_BACK:          txt = wxT( "B.Cu" );            break;
    case ADHESIVE_N_BACK:       txt = wxT( "B.Adhes" );         break;
    case ADHESIVE_N_FRONT:      txt = wxT( "F.Adhes" );         break;
    case SOLDERPASTE_N_BACK:    txt = wxT( "B.Paste" );         break;
    case SOLDERPASTE_N_FRONT:   txt = wxT( "F.Paste" );         break;
    case SILKSCREEN_N_BACK:     txt = wxT( "B.SilkS" );         break;
    case SILKSCREEN_N_FRONT:    txt = wxT( "F.SilkS" );         break;
    case SOLDERMASK_N_BACK:     txt = wxT( "B.Mask" );          break;
    case SOLDERMASK_N_FRONT:    txt = wxT( "F.Mask" );          break;
    case DRAW_N:                txt = wxT( "Dwgs.User" );       break;
    case COMMENT_N:             txt = wxT( "Cmts.User" );       break;
    case ECO1_N:                txt = wxT( "Eco1.User" );       break;
    case ECO2_N:                txt = wxT( "Eco2.User" );       break;
    case EDGE_N:                txt = wxT( "Edge.Cuts" );       break;
    default:                    txt = wxT( "BAD_INDEX" );       break;
    }

    return txt;     // wxString constructed once here
}


bool BOARD::SetLayerName( LAYER_NUM aLayer, const wxString& aLayerName )
{
    if( !IsCopperLayer( aLayer ) )
        return false;

    if( aLayerName == wxEmptyString || aLayerName.Len() > 20 )
        return false;

    // no quote chars in the name allowed
    if( aLayerName.Find( wxChar( '"' ) ) != wxNOT_FOUND )
        return false;

    wxString NameTemp = aLayerName;

    // replace any spaces with underscores before we do any comparing
    NameTemp.Replace( wxT( " " ), wxT( "_" ) );

    if( IsLayerEnabled( aLayer ) )
    {
        for( LAYER_NUM i = FIRST_COPPER_LAYER; i < NB_COPPER_LAYERS; ++i )
        {
            if( i != aLayer && IsLayerEnabled( i ) && NameTemp == m_Layer[i].m_Name )
                return false;
        }

        m_Layer[aLayer].m_Name = NameTemp;

        return true;
    }

    return false;
}


LAYER_T BOARD::GetLayerType( LAYER_NUM aLayer ) const
{
    if( !IsCopperLayer( aLayer ) )
        return LT_SIGNAL;

    //@@IMB: The original test was broken due to the discontinuity
    // in the layer sequence.
    if( IsLayerEnabled( aLayer ) )
        return m_Layer[aLayer].m_Type;

    return LT_SIGNAL;
}


bool BOARD::SetLayerType( LAYER_NUM aLayer, LAYER_T aLayerType )
{
    if( !IsCopperLayer( aLayer ) )
        return false;

    //@@IMB: The original test was broken due to the discontinuity
    // in the layer sequence.
    if( IsLayerEnabled( aLayer ) )
    {
        m_Layer[aLayer].m_Type = aLayerType;
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


LAYER_MSK BOARD::GetEnabledLayers() const
{
    return m_designSettings.GetEnabledLayers();
}


LAYER_MSK BOARD::GetVisibleLayers() const
{
    return m_designSettings.GetVisibleLayers();
}


void BOARD::SetEnabledLayers( LAYER_MSK aLayerMask )
{
    m_designSettings.SetEnabledLayers( aLayerMask );
}


void BOARD::SetVisibleLayers( LAYER_MSK aLayerMask )
{
    m_designSettings.SetVisibleLayers( aLayerMask );
}


void BOARD::SetVisibleElements( int aMask )
{
    // Call SetElementVisibility for each item
    // to ensure specific calculations that can be needed by some items,
    // just changing the visibility flags could be not sufficient.
    for( int ii = 0; ii < PCB_VISIBLE( END_PCB_VISIBLE_LIST ); ii++ )
    {
        int item_mask = 1 << ii;
        SetElementVisibility( ii, aMask & item_mask );
    }
}


void BOARD::SetVisibleAlls()
{
    SetVisibleLayers( FULL_LAYERS );

    // Call SetElementVisibility for each item,
    // to ensure specific calculations that can be needed by some items
    for( int ii = 0; ii < PCB_VISIBLE(END_PCB_VISIBLE_LIST); ii++ )
        SetElementVisibility( ii, true );
}


int BOARD::GetVisibleElements() const
{
    return m_designSettings.GetVisibleElements();
}


bool BOARD::IsElementVisible( int aPCB_VISIBLE ) const
{
    return m_designSettings.IsElementVisible( aPCB_VISIBLE );
}


void BOARD::SetElementVisibility( int aPCB_VISIBLE, bool isEnabled )
{
    m_designSettings.SetElementVisibility( aPCB_VISIBLE, isEnabled );

    switch( aPCB_VISIBLE )
    {
    case RATSNEST_VISIBLE:

        // we must clear or set the CH_VISIBLE flags to hide/show ratsnest
        // because we have a tool to show/hide ratsnest relative to a pad or a module
        // so the hide/show option is a per item selection
        if( IsElementVisible( RATSNEST_VISIBLE ) )
        {
            for( unsigned ii = 0; ii < GetRatsnestsCount(); ii++ )
                m_FullRatsnest[ii].m_Status |= CH_VISIBLE;
        }
        else
        {
            for( unsigned ii = 0; ii < GetRatsnestsCount(); ii++ )
                m_FullRatsnest[ii].m_Status &= ~CH_VISIBLE;
        }
        break;

    default:
        ;
    }
}


EDA_COLOR_T BOARD::GetVisibleElementColor( int aPCB_VISIBLE )
{
    EDA_COLOR_T color = UNSPECIFIED_COLOR;

    switch( aPCB_VISIBLE )
    {
    case NON_PLATED_VISIBLE:
    case VIA_THROUGH_VISIBLE:
    case VIA_MICROVIA_VISIBLE:
    case VIA_BBLIND_VISIBLE:
    case MOD_TEXT_FR_VISIBLE:
    case MOD_TEXT_BK_VISIBLE:
    case MOD_TEXT_INVISIBLE:
    case ANCHOR_VISIBLE:
    case PAD_FR_VISIBLE:
    case PAD_BK_VISIBLE:
    case RATSNEST_VISIBLE:
    case GRID_VISIBLE:
        color = GetColorsSettings()->GetItemColor( aPCB_VISIBLE );
        break;

    default:
        wxLogDebug( wxT( "BOARD::GetVisibleElementColor(): bad arg %d" ), aPCB_VISIBLE );
    }

    return color;
}


void BOARD::SetVisibleElementColor( int aPCB_VISIBLE, EDA_COLOR_T aColor )
{
    switch( aPCB_VISIBLE )
    {
    case NON_PLATED_VISIBLE:
    case VIA_THROUGH_VISIBLE:
    case VIA_MICROVIA_VISIBLE:
    case VIA_BBLIND_VISIBLE:
    case MOD_TEXT_FR_VISIBLE:
    case MOD_TEXT_BK_VISIBLE:
    case MOD_TEXT_INVISIBLE:
    case ANCHOR_VISIBLE:
    case PAD_FR_VISIBLE:
    case PAD_BK_VISIBLE:
    case GRID_VISIBLE:
    case RATSNEST_VISIBLE:
        GetColorsSettings()->SetItemColor( aPCB_VISIBLE, aColor );
        break;

    default:
        wxLogDebug( wxT( "BOARD::SetVisibleElementColor(): bad arg %d" ), aPCB_VISIBLE );
    }
}


void BOARD::SetLayerColor( LAYER_NUM aLayer, EDA_COLOR_T aColor )
{
    GetColorsSettings()->SetLayerColor( aLayer, aColor );
}


EDA_COLOR_T BOARD::GetLayerColor( LAYER_NUM aLayer ) const
{
    return GetColorsSettings()->GetLayerColor( aLayer );
}


bool BOARD::IsModuleLayerVisible( LAYER_NUM layer )
{
    switch( layer )
    {
    case LAYER_N_FRONT:
        return IsElementVisible( PCB_VISIBLE(MOD_FR_VISIBLE) );

    case LAYER_N_BACK:
        return IsElementVisible( PCB_VISIBLE(MOD_BK_VISIBLE) );

    default:
        wxFAIL_MSG( wxT( "BOARD::IsModuleLayerVisible() param error: bad layer" ) );
        return true;
    }
}


void BOARD::Add( BOARD_ITEM* aBoardItem, int aControl )
{
    if( aBoardItem == NULL )
    {
        wxFAIL_MSG( wxT( "BOARD::Add() param error: aBoardItem NULL" ) );
        return;
    }

    switch( aBoardItem->Type() )
    {
    // this one uses a vector
    case PCB_MARKER_T:
        aBoardItem->SetParent( this );
        m_markers.push_back( (MARKER_PCB*) aBoardItem );
        break;

    // this one uses a vector
    case PCB_ZONE_AREA_T:
        aBoardItem->SetParent( this );
        m_ZoneDescriptorList.push_back( (ZONE_CONTAINER*) aBoardItem );
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
        TRACK* insertAid;
        insertAid = ( (TRACK*) aBoardItem )->GetBestInsertPoint( this );
        m_Track.Insert( (TRACK*) aBoardItem, insertAid );
        break;

    case PCB_ZONE_T:
        if( aControl & ADD_APPEND )
            m_Zone.PushBack( (SEGZONE*) aBoardItem );
        else
            m_Zone.PushFront( (SEGZONE*) aBoardItem );

        aBoardItem->SetParent( this );
        break;

    case PCB_MODULE_T:
        if( aControl & ADD_APPEND )
            m_Modules.PushBack( (MODULE*) aBoardItem );
        else
            m_Modules.PushFront( (MODULE*) aBoardItem );

        aBoardItem->SetParent( this );

        // Because the list of pads has changed, reset the status
        // This indicate the list of pad and nets must be recalculated before use
        m_Status_Pcb = 0;
        break;

    case PCB_DIMENSION_T:
    case PCB_LINE_T:
    case PCB_TEXT_T:
    case PCB_MODULE_EDGE_T:
    case PCB_TARGET_T:
        if( aControl & ADD_APPEND )
            m_Drawings.PushBack( aBoardItem );
        else
            m_Drawings.PushFront( aBoardItem );

        aBoardItem->SetParent( this );
        break;

    // other types may use linked list
    default:
        {
            wxString msg;
            msg.Printf( wxT( "BOARD::Add() needs work: BOARD_ITEM type (%d) not handled" ),
                        aBoardItem->Type() );
            wxFAIL_MSG( msg );
        }
        break;
    }

    m_ratsnest->Add( aBoardItem );
}


BOARD_ITEM* BOARD::Remove( BOARD_ITEM* aBoardItem )
{
    // find these calls and fix them!  Don't send me no stinking' NULL.
    wxASSERT( aBoardItem );

    switch( aBoardItem->Type() )
    {
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
        m_Modules.Remove( (MODULE*) aBoardItem );
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
        m_Track.Remove( (TRACK*) aBoardItem );
        break;

    case PCB_ZONE_T:
        m_Zone.Remove( (SEGZONE*) aBoardItem );
        break;

    case PCB_DIMENSION_T:
    case PCB_LINE_T:
    case PCB_TEXT_T:
    case PCB_MODULE_EDGE_T:
    case PCB_TARGET_T:
        m_Drawings.Remove( aBoardItem );
        break;

    // other types may use linked list
    default:
        wxFAIL_MSG( wxT( "BOARD::Remove() needs more ::Type() support" ) );
    }

    m_ratsnest->Remove( aBoardItem );

    return aBoardItem;
}


void BOARD::DeleteMARKERs()
{
    // the vector does not know how to delete the MARKER_PCB, it holds pointers
    for( unsigned i = 0; i<m_markers.size(); ++i )
        delete m_markers[i];

    m_markers.clear();
}


void BOARD::DeleteZONEOutlines()
{
    // the vector does not know how to delete the ZONE Outlines, it holds
    // pointers
    for( unsigned i = 0; i<m_ZoneDescriptorList.size(); ++i )
        delete m_ZoneDescriptorList[i];

    m_ZoneDescriptorList.clear();
}


int BOARD::GetNumSegmTrack() const
{
    return m_Track.GetCount();
}


int BOARD::GetNumSegmZone() const
{
    return m_Zone.GetCount();
}


unsigned BOARD::GetNodesCount() const
{
    return m_nodeCount;
}


EDA_RECT BOARD::ComputeBoundingBox( bool aBoardEdgesOnly )
{
    bool hasItems = false;
    EDA_RECT area;

    // Check segments, dimensions, texts, and fiducials
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( aBoardEdgesOnly && (item->Type() != PCB_LINE_T || item->GetLayer() != EDGE_N ) )
            continue;

        if( !hasItems )
            area = item->GetBoundingBox();
        else
            area.Merge( item->GetBoundingBox() );

        hasItems = true;
    }

    if( !aBoardEdgesOnly )
    {
        // Check modules
        for( MODULE* module = m_Modules; module; module = module->Next() )
        {
            if( !hasItems )
                area = module->GetBoundingBox();
            else
                area.Merge( module->GetBoundingBox() );

            hasItems = true;
        }

        // Check tracks
        for( TRACK* track = m_Track; track; track = track->Next() )
        {
            if( !hasItems )
                area = track->GetBoundingBox();
            else
                area.Merge( track->GetBoundingBox() );

            hasItems = true;
        }

        // Check segment zones
        for( TRACK* track = m_Zone; track; track = track->Next() )
        {
            if( !hasItems )
                area = track->GetBoundingBox();
            else
                area.Merge( track->GetBoundingBox() );

            hasItems = true;
        }

        // Check polygonal zones
        for( unsigned int i = 0; i < m_ZoneDescriptorList.size(); i++ )
        {
            ZONE_CONTAINER* aZone = m_ZoneDescriptorList[i];

            if( !hasItems )
                area = aZone->GetBoundingBox();
            else
                area.Merge( aZone->GetBoundingBox() );

            area.Merge( aZone->GetBoundingBox() );
            hasItems = true;
        }
    }

    m_BoundingBox = area;   // save for BOARD::GetBoundingBox()

    return area;
}


// virtual, see pcbstruct.h
void BOARD::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString txt;
    int      viasCount = 0;
    int      trackSegmentsCount = 0;

    for( BOARD_ITEM* item = m_Track; item; item = item->Next() )
    {
        if( item->Type() == PCB_VIA_T )
            viasCount++;
        else
            trackSegmentsCount++;
    }

    txt.Printf( wxT( "%d" ), GetPadCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Pads" ), txt, DARKGREEN ) );

    txt.Printf( wxT( "%d" ), viasCount );
    aList.push_back( MSG_PANEL_ITEM( _( "Vias" ), txt, DARKGREEN ) );

    txt.Printf( wxT( "%d" ), trackSegmentsCount );
    aList.push_back( MSG_PANEL_ITEM( _( "trackSegm" ), txt, DARKGREEN ) );

    txt.Printf( wxT( "%d" ), GetNodesCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Nodes" ), txt, DARKCYAN ) );

    txt.Printf( wxT( "%d" ), m_NetInfo.GetNetCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Nets" ), txt, RED ) );

    /* These parameters are known only if the full ratsnest is available,
     *  so, display them only if this is the case
     */
    if( (m_Status_Pcb & NET_CODES_OK) )
    {
        txt.Printf( wxT( "%d" ), GetRatsnestsCount() );
        aList.push_back( MSG_PANEL_ITEM( _( "Links" ), txt, DARKGREEN ) );

        txt.Printf( wxT( "%d" ), GetRatsnestsCount() - GetUnconnectedNetCount() );
        aList.push_back( MSG_PANEL_ITEM( _( "Connect" ), txt, DARKGREEN ) );

        txt.Printf( wxT( "%d" ), GetUnconnectedNetCount() );
        aList.push_back( MSG_PANEL_ITEM( _( "Unconnected" ), txt, BLUE ) );
    }
}


// virtual, see pcbstruct.h
SEARCH_RESULT BOARD::Visit( INSPECTOR* inspector, const void* testData,
                            const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_CONTINUE;
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
            result = inspector->Inspect( this, testData );  // inspect me
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
            result = IterateForward( m_Modules, inspector, testData, p );

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
            result = IterateForward( m_Drawings, inspector, testData, p );

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
            result = IterateForward( m_Track, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_VIA_T:
                case PCB_TRACE_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

#else
        case PCB_VIA_T:
            result = IterateForward( m_Track, inspector, testData, p );
            ++p;
            break;

        case PCB_TRACE_T:
            result = IterateForward( m_Track, inspector, testData, p );
            ++p;
            break;
#endif

        case PCB_MARKER_T:

            // MARKER_PCBS are in the m_markers std::vector
            for( unsigned i = 0; i<m_markers.size(); ++i )
            {
                result = m_markers[i]->Visit( inspector, testData, p );

                if( result == SEARCH_QUIT )
                    break;
            }

            ++p;
            break;

        case PCB_ZONE_AREA_T:

            // PCB_ZONE_AREA_T are in the m_ZoneDescriptorList std::vector
            for( unsigned i = 0; i< m_ZoneDescriptorList.size(); ++i )
            {
                result = m_ZoneDescriptorList[i]->Visit( inspector, testData, p );

                if( result == SEARCH_QUIT )
                    break;
            }

            ++p;
            break;

        case PCB_ZONE_T:
            result = IterateForward( m_Zone, inspector, testData, p );
            ++p;
            break;

        default:        // catch EOT or ANY OTHER type here and return.
            done = true;
            break;
        }

        if( result == SEARCH_QUIT )
            break;
    }

    return result;
}


/*  now using PcbGeneralLocateAndDisplay(), but this remains a useful example
 *   of how the INSPECTOR can be used in a lightweight way.
 *  // see pcbstruct.h
 *  BOARD_ITEM* BOARD::FindPadOrModule( const wxPoint& refPos, LAYER_NUM layer )
 *  {
 *   class PadOrModule : public INSPECTOR
 *   {
 *   public:
 *       BOARD_ITEM*         found;
 *       LAYER_NUM           layer;
 *       int                 layer_mask;
 *
 *       PadOrModule( LAYER_NUM alayer ) :
 *           found(0), layer(alayer), layer_mask( g_TabOneLayerMask[alayer] )
 *       {}
 *
 *       SEARCH_RESULT Inspect( EDA_ITEM* testItem, const void* testData
 * )
 *       {
 *           BOARD_ITEM*     item   = (BOARD_ITEM*) testItem;
 *           const wxPoint&  refPos = *(const wxPoint*) testData;
 *
 *           if( item->Type() == PCB_PAD_T )
 *           {
 *               D_PAD*  pad = (D_PAD*) item;
 *               if( pad->HitTest( refPos ) )
 *               {
 *                   if( layer_mask & pad->GetLayerMask() )
 *                   {
 *                       found = item;
 *                       return SEARCH_QUIT;
 *                   }
 *                   else if( !found )
 *                   {
 *                       MODULE* parent = (MODULE*) pad->m_Parent;
 *                       if( IsModuleLayerVisible( parent->GetLayer() ) )
 *                           found = item;
 *                   }
 *               }
 *           }
 *
 *           else if( item->Type() == PCB_MODULE_T )
 *           {
 *               MODULE* module = (MODULE*) item;
 *
 *               // consider only visible modules
 *               if( IsModuleLayerVisible( module->GetLayer() ) )
 *               {
 *                   if( module->HitTest( refPos ) )
 *                   {
 *                       if( layer == module->GetLayer() )
 *                       {
 *                           found = item;
 *                           return SEARCH_QUIT;
 *                       }
 *
 *                       // layer mismatch, save in case we don't find a
 *                       // future layer match hit.
 *                       if( !found )
 *                           found = item;
 *                   }
 *               }
 *           }
 *           return SEARCH_CONTINUE;
 *       }
 *   };
 *
 *   PadOrModule inspector( layer );
 *
 *   // search only for PADs first, then MODULES, and preferably a layer match
 *   static const KICAD_T scanTypes[] = { PCB_PAD_T, PCB_MODULE_T, EOT };
 *
 *   // visit this BOARD with the above inspector
 *   Visit( &inspector, &refPos, scanTypes );
 *
 *   return inspector.found;
 *  }
 */


NETINFO_ITEM* BOARD::FindNet( int aNetcode ) const
{
    // the first valid netcode is 1 and the last is m_NetInfo.GetCount()-1.
    // zero is reserved for "no connection" and is not used.
    // NULL is returned for non valid netcodes
    NETINFO_ITEM* net = m_NetInfo.GetNetItem( aNetcode );

    return net;
}


NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname ) const
{
    return m_NetInfo.GetNetItem( aNetname );
}


MODULE* BOARD::FindModuleByReference( const wxString& aReference ) const
{
    struct FINDER : public INSPECTOR
    {
        MODULE* found;

        FINDER() : found( 0 )  {}

        // implement interface INSPECTOR
        SEARCH_RESULT Inspect( EDA_ITEM* item, const void* data )
        {
            MODULE*         module = (MODULE*) item;
            const wxString& ref    = *(const wxString*) data;

            if( ref == module->GetReference() )
            {
                found = module;
                return SEARCH_QUIT;
            }

            return SEARCH_CONTINUE;
        }
    } inspector;

    // search only for MODULES
    static const KICAD_T scanTypes[] = { PCB_MODULE_T, EOT };

    // visit this BOARD with the above inspector
    BOARD* nonconstMe = (BOARD*) this;
    nonconstMe->Visit( &inspector, &aReference, scanTypes );

    return inspector.found;
}


MODULE* BOARD::FindModule( const wxString& aRefOrTimeStamp, bool aSearchByTimeStamp ) const
{
    if( aSearchByTimeStamp )
    {
        for( MODULE* module = m_Modules;  module;  module = module->Next() )
        {
            if( aRefOrTimeStamp.CmpNoCase( module->GetPath() ) == 0 )
                return module;
        }
    }
    else
    {

#if 0   // case independent compare, why?
        for( MODULE* module = m_Modules;  module;  module = module->Next() )
        {
            if( aRefOrTimeStamp.CmpNoCase( module->GetReference() ) == 0 )
                return module;
        }
#else
        return FindModuleByReference( aRefOrTimeStamp );
#endif

    }

    return NULL;
}


// Sort nets by decreasing pad count
static bool s_SortByNodes( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
{
    return b->GetNodesCount() < a->GetNodesCount();
}


int BOARD::SortedNetnamesList( wxArrayString& aNames, bool aSortbyPadsCount )
{
    if( m_NetInfo.GetNetCount() == 0 )
        return 0;

    // Build the list
    std::vector <NETINFO_ITEM*> netBuffer;

    netBuffer.reserve( m_NetInfo.GetNetCount() );

    for( NETINFO_LIST::iterator net( m_NetInfo.begin() ), netEnd( m_NetInfo.end() );
                net != netEnd; ++net )
    {
        if( net->GetNet() > 0 )
            netBuffer.push_back( *net );
    }

    // sort the list
    if( aSortbyPadsCount )
        sort( netBuffer.begin(), netBuffer.end(), s_SortByNodes );

    for( unsigned ii = 0; ii <  netBuffer.size(); ii++ )
        aNames.Add( netBuffer[ii]->GetNetname() );

    return netBuffer.size();
}


void BOARD::RedrawAreasOutlines( EDA_DRAW_PANEL* panel, wxDC* aDC, GR_DRAWMODE aDrawMode, LAYER_NUM aLayer )
{
    if( !aDC )
        return;

    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = GetArea( ii );

        if( (aLayer < 0) || ( aLayer == edge_zone->GetLayer() ) )
            edge_zone->Draw( panel, aDC, aDrawMode );
    }
}


void BOARD::RedrawFilledAreas( EDA_DRAW_PANEL* panel, wxDC* aDC, GR_DRAWMODE aDrawMode, LAYER_NUM aLayer )
{
    if( !aDC )
        return;

    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = GetArea( ii );

        if( (aLayer < 0) || ( aLayer == edge_zone->GetLayer() ) )
            edge_zone->DrawFilledArea( panel, aDC, aDrawMode );
    }
}


ZONE_CONTAINER* BOARD::HitTestForAnyFilledArea( const wxPoint& aRefPos,
                                                LAYER_NUM      aStartLayer,
                                                LAYER_NUM      aEndLayer,
                                                int aNetCode )
{
    if( aEndLayer < 0 )
        aEndLayer = aStartLayer;

    if( aEndLayer <  aStartLayer )
        EXCHG( aEndLayer, aStartLayer );

    for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
    {
        ZONE_CONTAINER* area  = m_ZoneDescriptorList[ia];
        LAYER_NUM       layer = area->GetLayer();

        if( (layer < aStartLayer) || (layer > aEndLayer) )
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


int BOARD::SetAreasNetCodesFromNetNames( void )
{
    int error_count = 0;

    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        if( !GetArea( ii )->IsOnCopperLayer() )
        {
            GetArea( ii )->SetNetCode( NETINFO_LIST::UNCONNECTED );
            continue;
        }

        if( GetArea( ii )->GetNetCode() != 0 )      // i.e. if this zone is connected to a net
        {
            const NETINFO_ITEM* net = GetArea( ii )->GetNet();

            if( net )
            {
                GetArea( ii )->SetNetCode( net->GetNet() );
            }
            else
            {
                error_count++;

                // keep Net Name and set m_NetCode to -1 : error flag.
                GetArea( ii )->SetNetCode( -1 );
            }
        }
    }

    return error_count;
}


VIA* BOARD::GetViaByPosition( const wxPoint& aPosition, LAYER_NUM aLayer) const
{
    for( VIA *via = GetFirstVia( m_Track); via; via = GetFirstVia( via->Next() ) )
    {
        if( (via->GetStart() == aPosition) &&
                (via->GetState( BUSY | IS_DELETED ) == 0) &&
                ((aLayer == UNDEFINED_LAYER) || (via->IsOnLayer( aLayer ))) )
            return via;
    }

    return NULL;
}


D_PAD* BOARD::GetPad( const wxPoint& aPosition, LAYER_MSK aLayerMask )
{
    D_PAD* pad = NULL;

    for( MODULE* module = m_Modules;  module && ( pad == NULL );  module = module->Next() )
    {
        if( aLayerMask )
            pad = module->GetPad( aPosition, aLayerMask );
        else
            pad = module->GetPad( aPosition, ALL_LAYERS );
    }

    return pad;
}


D_PAD* BOARD::GetPad( TRACK* aTrace, ENDPOINT_T aEndPoint )
{
    D_PAD*  pad = NULL;
    const wxPoint &aPosition = aTrace->GetEndPoint( aEndPoint );

    LAYER_MSK aLayerMask = GetLayerMask( aTrace->GetLayer() );

    for( MODULE* module = m_Modules;  module;  module = module->Next() )
    {
        pad = module->GetPad( aPosition, aLayerMask );

        if( pad != NULL )
            break;
    }

    return pad;
}


D_PAD* BOARD::GetPadFast( const wxPoint& aPosition, LAYER_MSK aLayerMask )
{
    for( unsigned i=0; i<GetPadCount();  ++i )
    {
        D_PAD* pad = m_NetInfo.GetPad(i);

        if( pad->GetPosition() != aPosition )
            continue;

        /* Pad found, it must be on the correct layer */
        if( pad->GetLayerMask() & aLayerMask )
            return pad;
    }

    return NULL;
}


D_PAD* BOARD::GetPad( std::vector<D_PAD*>& aPadList, const wxPoint& aPosition, LAYER_MSK aLayerMask )
{
    // Search the aPoint coordinates in aPadList
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
            if( aLayerMask & pad->GetLayerMask())
                return pad;

            // More than one pad can be at aPosition
            // search for a pad at aPosition that matched this mask

            // search next
            for( int ii = idx+1; ii <= idxmax; ii++ )
            {
                pad = aPadList[ii];

                if( pad->GetPosition() != aPosition )
                    break;

                if( (aLayerMask & pad->GetLayerMask()) != 0 )
                    return pad;
            }
            // search previous
            for(  int ii = idx-1 ;ii >=0; ii-- )
            {
                pad = aPadList[ii];

                if( pad->GetPosition() != aPosition )
                    break;

                if( (aLayerMask & pad->GetLayerMask()) != 0 )
                    return pad;
            }

            // Not found:
            return 0;
        }

        if( pad->GetPosition().x == aPosition.x )   // Must search considering Y coordinate
        {
            if(pad->GetPosition().y < aPosition.y)  // Must search after this item
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
    if( aNetCode < 0 )
    {
        aVector.insert( aVector.end(), m_NetInfo.m_PadsFullList.begin(),
                        m_NetInfo.m_PadsFullList.end() );
    }
    else
    {
        const NETINFO_ITEM* net = m_NetInfo.GetNetItem( aNetCode );
        if( net )
        {
            aVector.insert( aVector.end(), net->m_PadInNetList.begin(),
                            net->m_PadInNetList.end() );
        }
    }

    sort( aVector.begin(), aVector.end(), sortPadsByXthenYCoord );
}


TRACK* BOARD::GetTrack( TRACK* aTrace, const wxPoint& aPosition,
        LAYER_MSK aLayerMask ) const
{
    for( TRACK* track = aTrace; track; track = track->Next() )
    {
        LAYER_NUM layer = track->GetLayer();

        if( track->GetState( BUSY | IS_DELETED ) )
            continue;

        if( m_designSettings.IsLayerVisible( layer ) == false )
            continue;

        if( track->Type() == PCB_VIA_T )    /* VIA encountered. */
        {
            if( track->HitTest( aPosition ) )
                return track;
        }
        else
        {
            if( (GetLayerMask( layer ) & aLayerMask) == 0 )
                continue;   /* Segments on different layers. */

            if( track->HitTest( aPosition ) )
                return track;
        }
    }

    return NULL;
}


TRACK* BOARD::MarkTrace( TRACK*  aTrace, int* aCount,
                         double* aTraceLength, double* aPadToDieLength,
                         bool    aReorder )
{
    int        NbSegmBusy;

    TRACK_PTRS trackList;

    if( aCount )
        *aCount = 0;

    if( aTraceLength )
        *aTraceLength = 0;

    if( aTrace == NULL )
        return NULL;

    // Ensure the flag BUSY of all tracks of the board is cleared
    // because we use it to mark segments of the track
    for( TRACK* track = m_Track; track; track = track->Next() )
        track->SetState( BUSY, false );

    /* Set flags of the initial track segment */
    aTrace->SetState( BUSY, true );
    LAYER_MSK layerMask = aTrace->GetLayerMask();

    trackList.push_back( aTrace );

    /* Examine the initial track segment : if it is really a segment, this is
     * easy.
     *  If it is a via, one must search for connected segments.
     *  If <=2, this via connect 2 segments (or is connected to only one
     *  segment) and this via and these 2 segments are a part of a track.
     *  If > 2 only this via is flagged (the track has only this via)
     */
    if( aTrace->Type() == PCB_VIA_T )
    {
        TRACK* Segm1, * Segm2 = NULL, * Segm3 = NULL;
        Segm1 = ::GetTrack( m_Track, NULL, aTrace->GetStart(), layerMask );

        if( Segm1 )
        {
            Segm2 = ::GetTrack( Segm1->Next(), NULL, aTrace->GetStart(), layerMask );
        }

        if( Segm2 )
        {
            Segm3 = ::GetTrack( Segm2->Next(), NULL, aTrace->GetStart(), layerMask );
        }

        if( Segm3 ) // More than 2 segments are connected to this via. the track" is only this via
        {
            if( aCount )
                *aCount = 1;

            return aTrace;
        }

        if( Segm1 ) // search for others segments connected to the initial segment start point
        {
            layerMask = Segm1->GetLayerMask();
            chainMarkedSegments( aTrace->GetStart(), layerMask, &trackList );
        }

        if( Segm2 ) // search for others segments connected to the initial segment end point
        {
            layerMask = Segm2->GetLayerMask();
            chainMarkedSegments( aTrace->GetStart(), layerMask, &trackList );
        }
    }
    else    // mark the chain using both ends of the initial segment
    {
        chainMarkedSegments( aTrace->GetStart(), layerMask, &trackList );
        chainMarkedSegments( aTrace->GetEnd(), layerMask, &trackList );
    }

    // Now examine selected vias and flag them if they are on the track
    // If a via is connected to only one or 2 segments, it is flagged (is on the track)
    // If a via is connected to more than 2 segments, it is a track end, and it
    // is removed from the list
    // go through the list backwards.
    for( int i = trackList.size() - 1;  i>=0;  --i )
    {
        TRACK* via = trackList[i];

        if( via->Type() != PCB_VIA_T )
            continue;

        if( via == aTrace )
            continue;

        via->SetState( BUSY, true );  // Try to flag it. the flag will be cleared later if needed

        layerMask = via->GetLayerMask();

        TRACK* track = ::GetTrack( m_Track, NULL, via->GetStart(), layerMask );

        // GetTrace does not consider tracks flagged BUSY.
        // So if no connected track found, this via is on the current track
        // only: keep it
        if( track == NULL )
            continue;

        /* If a track is found, this via connects also others segments of an
         * other track.  This case happens when the vias ends the selected
         * track but must we consider this via is on the selected track, or
         * on an other track.
         * (this is important when selecting a track for deletion: must this
         * via be deleted or not?)
         * We consider here this via on the track if others segment connected
         * to this via remain connected when removing this via.
         * We search for all others segment connected together:
         * if there are on the same layer, the via is on the selected track
         * if there are on different layers, the via is on an other track
         */
        LAYER_NUM layer = track->GetLayer();

        while( ( track = ::GetTrack( track->Next(), NULL, via->GetStart(), layerMask ) ) != NULL )
        {
            if( layer != track->GetLayer() )
            {
                // The via connects segments of an other track: it is removed
                // from list because it is member of an other track
                via->SetState( BUSY, false );
                break;
            }
        }
    }

    /* Rearrange the track list in order to have flagged segments linked
     * from firstTrack so the NbSegmBusy segments are consecutive segments
     * in list, the first item in the full track list is firstTrack, and
     * the NbSegmBusy-1 next items (NbSegmBusy when including firstTrack)
     * are the flagged segments
     */
    NbSegmBusy = 0;
    TRACK* firstTrack;

    for( firstTrack = m_Track; firstTrack; firstTrack = firstTrack->Next() )
    {
        // Search for the first flagged BUSY segments
        if( firstTrack->GetState( BUSY ) )
        {
            NbSegmBusy = 1;
            break;
        }
    }

    if( firstTrack == NULL )
        return NULL;

    double full_len = 0;
    double lenPadToDie = 0;

    if( aReorder )
    {
        DLIST<TRACK>* list = (DLIST<TRACK>*)firstTrack->GetList();
        wxASSERT( list );

        /* Rearrange the chain starting at firstTrack
         * All others flagged items are moved from their position to the end
         * of the flagged list
         */
        TRACK* next;

        for( TRACK* track = firstTrack->Next(); track; track = next )
        {
            next = track->Next();

            if( track->GetState( BUSY ) )   // move it!
            {
                NbSegmBusy++;
                track->UnLink();
                list->Insert( track, firstTrack->Next() );

                if( aTraceLength )
                    full_len += track->GetLength();

                if( aPadToDieLength ) // Add now length die.
                {
                    // In fact only 2 pads (maximum) will be taken in account:
                    // that are on each end of the track, if any
                    if( track->GetState( BEGIN_ONPAD ) )
                    {
                        D_PAD * pad = (D_PAD *) track->start;
                        lenPadToDie += (double) pad->GetPadToDieLength();
                    }

                    if( track->GetState( END_ONPAD ) )
                    {
                        D_PAD * pad = (D_PAD *) track->end;
                        lenPadToDie += (double) pad->GetPadToDieLength();
                    }
                }
            }
        }
    }
    else if( aTraceLength )
    {
        NbSegmBusy = 0;

        for( TRACK* track = firstTrack; track; track = track->Next() )
        {
            if( track->GetState( BUSY ) )
            {
                NbSegmBusy++;
                track->SetState( BUSY, false );
                full_len += track->GetLength();

                // Add now length die.
                // In fact only 2 pads (maximum) will be taken in account:
                // that are on each end of the track, if any
                if( track->GetState( BEGIN_ONPAD ) )
                {
                    D_PAD * pad = (D_PAD *) track->start;
                    lenPadToDie += (double) pad->GetPadToDieLength();
                }

                if( track->GetState( END_ONPAD ) )
                {
                    D_PAD * pad = (D_PAD *) track->end;
                    lenPadToDie += (double) pad->GetPadToDieLength();
                }
            }
        }
    }

    if( aTraceLength )
        *aTraceLength = KiROUND( full_len );

    if( aPadToDieLength )
        *aPadToDieLength = KiROUND( lenPadToDie );

    if( aCount )
        *aCount = NbSegmBusy;

    return firstTrack;
}


MODULE* BOARD::GetFootprint( const wxPoint& aPosition, LAYER_NUM aActiveLayer,
                             bool aVisibleOnly, bool aIgnoreLocked )
{
    MODULE* pt_module;
    MODULE* module      = NULL;
    MODULE* alt_module  = NULL;
    int     min_dim     = 0x7FFFFFFF;
    int     alt_min_dim = 0x7FFFFFFF;
    bool    current_layer_back = IsBackLayer( aActiveLayer );

    for( pt_module = m_Modules;  pt_module;  pt_module = (MODULE*) pt_module->Next() )
    {
        // is the ref point within the module's bounds?
        if( !pt_module->HitTest( aPosition ) )
            continue;

        // if caller wants to ignore locked modules, and this one is locked, skip it.
        if( aIgnoreLocked && pt_module->IsLocked() )
            continue;

        LAYER_NUM layer = pt_module->GetLayer();

        // Filter non visible modules if requested
        if( (!aVisibleOnly) || IsModuleLayerVisible( layer ) )
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


BOARD_CONNECTED_ITEM* BOARD::GetLockPoint( const wxPoint& aPosition, LAYER_MSK aLayerMask )
{
    for( MODULE* module = m_Modules; module; module = module->Next() )
    {
        D_PAD* pad = module->GetPad( aPosition, aLayerMask );

        if( pad )
            return pad;
    }

    /* No pad has been located so check for a segment of the trace. */
    TRACK* segment = ::GetTrack( m_Track, NULL, aPosition, aLayerMask );

    if( segment == NULL )
        segment = GetTrack( m_Track, aPosition, aLayerMask );

    return segment;
}


TRACK* BOARD::CreateLockPoint( wxPoint& aPosition, TRACK* aSegment, PICKED_ITEMS_LIST* aList )
{
    /* creates an intermediate point on aSegment and break it into two segments
     * at aPosition.
     * The new segment starts from aPosition and ends at the end point of
     * aSegment. The original segment now ends at aPosition.
     */
    if( aSegment->GetStart() == aPosition || aSegment->GetEnd() == aPosition )
        return NULL;

    /* A via is a good lock point */
    if( aSegment->Type() == PCB_VIA_T )
    {
        aPosition = aSegment->GetStart();
        return aSegment;
    }

    // Calculation coordinate of intermediate point relative to the start point of aSegment
     wxPoint delta = aSegment->GetEnd() - aSegment->GetStart();

    // calculate coordinates of aPosition relative to aSegment->GetStart()
    wxPoint lockPoint = aPosition - aSegment->GetStart();

    // lockPoint must be on aSegment:
    // Ensure lockPoint.y/lockPoint.y = delta.y/delta.x
    if( delta.x == 0 )
        lockPoint.x = 0;         /* horizontal segment*/
    else
        lockPoint.y = KiROUND( ( (double)lockPoint.x * delta.y ) / delta.x );

    /* Create the intermediate point (that is to say creation of a new
     * segment, beginning at the intermediate point.
     */
    lockPoint += aSegment->GetStart();

    TRACK* newTrack = (TRACK*)aSegment->Clone();
    // The new segment begins at the new point,
    newTrack->SetStart(lockPoint);
    newTrack->start = aSegment;
    newTrack->SetState( BEGIN_ONPAD, false );

    DLIST<TRACK>* list = (DLIST<TRACK>*)aSegment->GetList();
    wxASSERT( list );
    list->Insert( newTrack, aSegment->Next() );

    if( aList )
    {
        // Prepare the undo command for the now track segment
        ITEM_PICKER picker( newTrack, UR_NEW );
        aList->PushItem( picker );
        // Prepare the undo command for the old track segment
        // before modifications
        picker.SetItem( aSegment );
        picker.SetStatus( UR_CHANGED );
        picker.SetLink( aSegment->Clone() );
        aList->PushItem( picker );
    }

    // Old track segment now ends at new point.
    aSegment->SetEnd(lockPoint);
    aSegment->end = newTrack;
    aSegment->SetState( END_ONPAD, false );

    D_PAD * pad = GetPad( newTrack, ENDPOINT_START );

    if ( pad )
    {
        newTrack->start = pad;
        newTrack->SetState( BEGIN_ONPAD, true );
        aSegment->end = pad;
        aSegment->SetState( END_ONPAD, true );
    }

    aPosition = lockPoint;
    return newTrack;
}


ZONE_CONTAINER* BOARD::AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode,
                                LAYER_NUM aLayer, wxPoint aStartPointPosition, int aHatch )
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


ZONE_CONTAINER* BOARD::InsertArea( int netcode, int iarea, LAYER_NUM layer, int x, int y, int hatch )
{
    ZONE_CONTAINER* new_area = new ZONE_CONTAINER( this );

    new_area->SetNetCode( netcode );
    new_area->SetLayer( layer );
    new_area->SetTimeStamp( GetNewTimeStamp() );

    if( iarea < (int) ( m_ZoneDescriptorList.size() - 1 ) )
        m_ZoneDescriptorList.insert( m_ZoneDescriptorList.begin() + iarea + 1, new_area );
    else
        m_ZoneDescriptorList.push_back( new_area );

    new_area->Outline()->Start( layer, x, y, hatch );
    return new_area;
}


bool BOARD::NormalizeAreaPolygon( PICKED_ITEMS_LIST * aNewZonesList, ZONE_CONTAINER* aCurrArea )
{
    CPolyLine* curr_polygon = aCurrArea->Outline();

    // mark all areas as unmodified except this one, if modified
    for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
        m_ZoneDescriptorList[ia]->SetLocalFlags( 0 );

    aCurrArea->SetLocalFlags( 1 );

    if( curr_polygon->IsPolygonSelfIntersecting() )
    {
        std::vector<CPolyLine*>* pa = new std::vector<CPolyLine*>;
        curr_polygon->UnHatch();
        int n_poly = aCurrArea->Outline()->NormalizeAreaOutlines( pa );

        // If clipping has created some polygons, we must add these new copper areas.
        if( n_poly > 1 )
        {
            ZONE_CONTAINER* NewArea;

            for( int ip = 1; ip < n_poly; ip++ )
            {
                // create new copper area and copy poly into it
                CPolyLine* new_p = (*pa)[ip - 1];
                NewArea = AddArea( aNewZonesList, aCurrArea->GetNetCode(), aCurrArea->GetLayer(),
                                   wxPoint(0, 0), CPolyLine::NO_HATCH );

                // remove the poly that was automatically created for the new area
                // and replace it with a poly from NormalizeAreaOutlines
                delete NewArea->Outline();
                NewArea->SetOutline( new_p );
                NewArea->Outline()->Hatch();
                NewArea->SetLocalFlags( 1 );
            }
        }

        delete pa;
    }

    curr_polygon->Hatch();

    return true;
}


void BOARD::ReplaceNetlist( NETLIST& aNetlist, bool aDeleteSinglePadNets,
                            REPORTER* aReporter )
{
    unsigned       i;
    wxPoint        bestPosition;
    wxString       msg;
    D_PAD*         pad;
    MODULE*        footprint;
    COMPONENT_NET  net;

    if( !IsEmpty() )
    {
        // Position new components below any existing board features.
        EDA_RECT bbbox = ComputeBoundingBox( true );

        if( bbbox.GetWidth() || bbbox.GetHeight() )
        {
            bestPosition.x = bbbox.Centre().x;
            bestPosition.y = bbbox.GetBottom() + Millimeter2iu( 10 );
        }
    }
    else
    {
        // Position new components in the center of the page when the board is empty.
        wxSize pageSize = m_paper.GetSizeIU();

        bestPosition.x = pageSize.GetWidth() / 2;
        bestPosition.y = pageSize.GetHeight() / 2;
    }

    m_Status_Pcb = 0;

    for( i = 0;  i < aNetlist.GetCount();  i++ )
    {
        COMPONENT* component = aNetlist.GetComponent( i );

        if( aReporter && aReporter->ReportAll() )
        {
#if defined(DEBUG)
            if( component->GetReference() == wxT( "D2" ) )
            {
                int breakhere = 1;
                (void) breakhere;
            }
#endif

            msg.Printf( _( "Checking netlist component footprint \"%s:%s:%s\".\n" ),
                        GetChars( component->GetReference() ),
                        GetChars( component->GetTimeStamp() ),
                        GetChars( component->GetFPID().Format() ) );
            aReporter->Report( msg );
        }

        if( aNetlist.IsFindByTimeStamp() )
            footprint = FindModule( aNetlist.GetComponent( i )->GetTimeStamp(), true );
        else
            footprint = FindModule( aNetlist.GetComponent( i )->GetReference() );

        if( footprint == NULL )        // A new footprint.
        {
            if( aReporter )
            {
                if( component->GetModule() != NULL )
                {
                    msg.Printf( _( "Adding new component \"%s:%s\" footprint \"%s\".\n" ),
                                GetChars( component->GetReference() ),
                                GetChars( component->GetTimeStamp() ),
                                GetChars( component->GetFPID().Format() ) );

                    if( aReporter->ReportWarnings() )
                        aReporter->Report( msg );
                }
                else
                {
                    msg.Printf( _( "Cannot add new component \"%s:%s\" due to missing "
                                   "footprint \"%s\".\n" ),
                                GetChars( component->GetReference() ),
                                GetChars( component->GetTimeStamp() ),
                                GetChars( component->GetFPID().Format() ) );

                    if( aReporter->ReportErrors() )
                        aReporter->Report( msg );
                }
            }

            if( !aNetlist.IsDryRun() && (component->GetModule() != NULL) )
            {
                // Owned by NETLIST, can only copy it.
                footprint = new MODULE( *component->GetModule() );
                footprint->SetParent( this );
                footprint->SetPosition( bestPosition );
                footprint->SetTimeStamp( GetNewTimeStamp() );
                Add( footprint, ADD_APPEND );
            }
        }
        else                           // An existing footprint.
        {
            // Test for footprint change.
            if( !component->GetFPID().empty() &&
                footprint->GetFPID() != component->GetFPID() )
            {
                if( aNetlist.GetReplaceFootprints() )
                {
                    if( aReporter )
                    {
                        if( component->GetModule() != NULL )
                        {
                            msg.Printf( _( "Replacing component \"%s:%s\" footprint \"%s\" with "
                                           "\"%s\".\n" ),
                                        GetChars( footprint->GetReference() ),
                                        GetChars( footprint->GetPath() ),
                                        GetChars( footprint->GetFPID().Format() ),
                                        GetChars( component->GetFPID().Format() ) );

                            if( aReporter->ReportWarnings() )
                                aReporter->Report( msg );
                        }
                        else
                        {
                            msg.Printf( _( "Cannot replace component \"%s:%s\" due to missing "
                                           "footprint \"%s\".\n" ),
                                        GetChars( footprint->GetReference() ),
                                        GetChars( footprint->GetPath() ),
                                        GetChars( component->GetFPID().Format() ) );

                            if( aReporter->ReportErrors() )
                                aReporter->Report( msg );
                        }
                    }

                    if( !aNetlist.IsDryRun() && (component->GetModule() != NULL) )
                    {
                        wxASSERT( footprint != NULL );
                        MODULE* newFootprint = new MODULE( *component->GetModule() );

                        if( aNetlist.IsFindByTimeStamp() )
                            newFootprint->SetReference( footprint->GetReference() );
                        else
                            newFootprint->SetPath( footprint->GetPath() );

                        footprint->CopyNetlistSettings( newFootprint );
                        Remove( footprint );
                        Add( newFootprint, ADD_APPEND );
                        footprint = newFootprint;
                    }
                }
            }

            // Test for reference designator field change.
            if( footprint->GetReference() != component->GetReference() )
            {
                if( aReporter && aReporter->ReportWarnings())
                {
                    msg.Printf( _( "Changing footprint \"%s:%s\" reference to \"%s\".\n" ),
                                GetChars( footprint->GetReference() ),
                                GetChars( footprint->GetPath() ),
                                GetChars( component->GetReference() ) );
                    aReporter->Report( msg );
                }

                if( !aNetlist.IsDryRun() )
                    footprint->SetReference( component->GetReference() );
            }

            // Test for value field change.
            if( footprint->GetValue() != component->GetValue() )
            {
                if( aReporter && aReporter->ReportAll() )
                {
                    msg.Printf( _( "Changing footprint \"%s:%s\" value from \"%s\" to \"%s\".\n" ),
                                GetChars( footprint->GetReference() ),
                                GetChars( footprint->GetPath() ),
                                GetChars( footprint->GetValue() ),
                                GetChars( component->GetValue() ) );
                    aReporter->Report( msg );
                }

                if( !aNetlist.IsDryRun() )
                    footprint->SetValue( component->GetValue() );
            }

            // Test for time stamp change.
            if( footprint->GetPath() != component->GetTimeStamp() )
            {
                if( aReporter && aReporter->ReportWarnings() )
                {
                    msg.Printf( _( "Changing footprint path \"%s:%s\" to \"%s\".\n" ),
                                GetChars( footprint->GetReference() ),
                                GetChars( footprint->GetPath() ),
                                GetChars( component->GetTimeStamp() ) );
                    aReporter->Report( msg );
                }

                if( !aNetlist.IsDryRun() )
                    footprint->SetPath( component->GetTimeStamp() );
            }
        }

        if( footprint == NULL )
            continue;

        // At this point, the component footprint is updated.  Now update the nets.
        for( pad = footprint->Pads();  pad;  pad = pad->Next() )
        {
            net = component->GetNet( pad->GetPadName() );

            if( !net.IsValid() )                // Footprint pad had no net.
            {
                if( !pad->GetNetname().IsEmpty() )
                {
                    if( aReporter && aReporter->ReportAll() )
                    {
                        msg.Printf( _( "Clearing component \"%s:%s\" pin \"%s\" net name.\n" ),
                                    GetChars( footprint->GetReference() ),
                                    GetChars( footprint->GetPath() ),
                                    GetChars( pad->GetPadName() ) );
                        aReporter->Report( msg );
                    }

                    if( !aNetlist.IsDryRun() )
                        pad->SetNetCode( NETINFO_LIST::UNCONNECTED );
                }
            }
            else                                 // Footprint pad has a net.
            {
                if( net.GetNetName() != pad->GetNetname() )
                {
                    if( aReporter && aReporter->ReportAll() )
                    {
                        msg.Printf( _( "Changing component \"%s:%s\" pin \"%s\" net name from "
                                       "\"%s\" to \"%s\".\n" ),
                                    GetChars( footprint->GetReference() ),
                                    GetChars( footprint->GetPath() ),
                                    GetChars( pad->GetPadName() ),
                                    GetChars( pad->GetNetname() ),
                                    GetChars( net.GetNetName() ) );
                        aReporter->Report( msg );
                    }

                    if( !aNetlist.IsDryRun() )
                    {
                        NETINFO_ITEM* netinfo = FindNet( net.GetNetName() );
                        if( netinfo == NULL )
                        {
                            // It is a new net, we have to add it
                            netinfo = new NETINFO_ITEM( this, net.GetNetName() );
                            m_NetInfo.AppendNet( netinfo );
                        }

                        pad->SetNetCode( netinfo->GetNet() );
                    }
                }
            }
        }
    }

    // Remove all components not in the netlist.
    if( aNetlist.GetDeleteExtraFootprints() )
    {
        MODULE* nextModule;
        const COMPONENT* component;

        for( MODULE* module = m_Modules;  module != NULL;  module = nextModule )
        {
            nextModule = module->Next();

            if( module->IsLocked() )
                continue;

            if( aNetlist.IsFindByTimeStamp() )
                component = aNetlist.GetComponentByTimeStamp( module->GetPath() );
            else
                component = aNetlist.GetComponentByReference( module->GetReference() );

            if( component == NULL )
            {
                if( aReporter && aReporter->ReportWarnings() )
                {
                    msg.Printf( _( "Removing footprint \"%s:%s\".\n" ),
                                GetChars( module->GetReference() ),
                                GetChars( module->GetPath() ) );
                    aReporter->Report( msg );
                }

                if( !aNetlist.IsDryRun() )
                    module->DeleteStructure();
            }
        }
    }

    // If needed, remove the single pad nets:
    if( aDeleteSinglePadNets && !aNetlist.IsDryRun() )
    {
        BuildListOfNets();

        std::vector<D_PAD*> padlist = GetPads();

        // padlist is the list of pads, sorted by netname.
        int         count = 0;
        wxString    netname;
        D_PAD*      pad = NULL;
        D_PAD*      previouspad = NULL;

        for( unsigned ii = 0; ii < padlist.size(); ii++ )
        {
            pad = padlist[ii];

            if( pad->GetNetname().IsEmpty() )
                continue;

            if( netname != pad->GetNetname() )  // End of net
            {
                if( previouspad && count == 1 )
                {
                    if( aReporter && aReporter->ReportAll() )
                    {
                        msg.Printf( _( "Remove single pad net \"%s\" on \"%s\" pad '%s'\n" ),
                                    GetChars( previouspad->GetNetname() ),
                                    GetChars( previouspad->GetParent()->GetReference() ),
                                    GetChars( previouspad->GetPadName() ) );
                        aReporter->Report( msg );
                    }

                    previouspad->SetNetCode( NETINFO_LIST::UNCONNECTED );
                }
                netname = pad->GetNetname();
                count = 1;
            }
            else
                count++;

            previouspad = pad;
        }

        // Examine last pad
        if( pad && count == 1 )
            pad->SetNetCode( NETINFO_LIST::UNCONNECTED );
    }

    // Last step: Some tests:
    // verify all pads found in netlist:
    // They should exist in footprints, otherwise the footprint is wrong
    // note also references or time stamps are updated, so we use only
    // the reference to find a footprint
    if( aReporter && aReporter->ReportErrors() )
    {
        wxString padname;
        for( i = 0; i < aNetlist.GetCount(); i++ )
        {
            const COMPONENT* component = aNetlist.GetComponent( i );
            MODULE* footprint = FindModuleByReference( component->GetReference() );

            if( footprint == NULL )    // It can be missing in partial designs
                continue;

            // Explore all pins/pads in component
            for( unsigned jj = 0; jj < component->GetNetCount(); jj++ )
            {
                net = component->GetNet( jj );
                padname = net.GetPinName();

                if( footprint->FindPadByName( padname ) )
                    continue;   // OK, pad found

                // not found: bad footprint, report error
                msg.Printf( _( "*** Error: Component '%s' pad '%s' not found in footprint '%s' ***\n" ),
                            GetChars( component->GetReference() ),
                            GetChars( padname ),
                            GetChars( footprint->GetFPID().Format() ) );
                aReporter->Report( msg );
            }
        }
    }
}

/* Extracts the board outlines and build a closed polygon
 * from lines, arcs and circle items on edge cut layer
 * Any closed outline inside the main outline is a hole
 * All contours should be closed, i.e. are valid vertices for a closed polygon
 * return true if success, false if a contour is not valid
 */
#include <specctra.h>
bool BOARD::GetBoardPolygonOutlines( CPOLYGONS_LIST& aOutlines,
                                     CPOLYGONS_LIST& aHoles,
                                     wxString* aErrorText )
{
    // the SPECCTRA_DB function to extract board outlines:
    SPECCTRA_DB dummy;
    return dummy.GetBoardPolygonOutlines( this, aOutlines,
                                          aHoles, aErrorText );
}
