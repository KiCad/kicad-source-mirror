/**
 * @file class_board.cpp
 * @brief  BOARD class functions.
 */

#include <limits.h>
#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <pcbcommon.h>
#include <wxBasePcbFrame.h>

#include <pcbnew.h>
#include <colors_selection.h>

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
    m_NetClasses( this )
{
    // we have not loaded a board yet, assume latest until then.
    m_fileFormatVersionAtLoad = BOARD_FILE_VERSION;

    m_Status_Pcb    = 0;                    // Status word: bit 1 = calculate.
    SetColorsSettings( &g_ColorsSettings );
    m_NbNodes     = 0;                      // Number of connected pads.
    m_NbNoconnect = 0;                      // Number of unconnected nets.

    m_CurrentZoneContour = NULL;            // This ZONE_CONTAINER handle the
                                            // zone contour currently in progress

    BuildListOfNets();                      // prepare pad and netlist containers.

    for( int layer = 0; layer < NB_COPPER_LAYERS; ++layer )
    {
        m_Layer[layer].m_Name = GetDefaultLayerName( layer );
        m_Layer[layer].m_Type = LT_SIGNAL;
    }

    m_NetClasses.GetDefault()->SetDescription( _( "This is the default net class." ) );

    m_ViaSizeSelector    = 0;
    m_TrackWidthSelector = 0;

    /*  Dick 5-Feb-2012: this seems unnecessary.  I don't believe the comment
        near line 70 of class_netclass.cpp.  I stepped through with debugger.
        Perhaps something else is at work, it is not a constructor race.
    // Initialize default values in default netclass.
    */
    m_NetClasses.GetDefault()->SetParams();

    SetCurrentNetClass( m_NetClasses.GetDefault()->GetName() );
}


BOARD::~BOARD()
{
    /*  @todo
        NO!  this has nothing to do with a BOARD
        Do this in the UI, not in the storage container please.
    if( m_PcbFrame && m_PcbFrame->GetScreen() )
        m_PcbFrame->GetScreen()->ClearUndoRedoList();
    */

    while( m_ZoneDescriptorList.size() )
    {
        ZONE_CONTAINER* area_to_remove = m_ZoneDescriptorList[0];
        Delete( area_to_remove );
    }

    m_FullRatsnest.clear();

    m_LocalRatsnest.clear();

    DeleteMARKERs();
    DeleteZONEOutlines();

    delete m_CurrentZoneContour;
    m_CurrentZoneContour = NULL;
}


void BOARD::chainMarkedSegments( wxPoint aPosition, int aLayerMask, TRACK_PTRS* aList )
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
        if( GetPadFast( aPosition, aLayerMask ) != NULL )
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
            aLayerMask = via->ReturnMaskLayer();

            aList->push_back( via );
        }

        /* Now we search all segments connected to point aPosition
         *  if only 1 segment: this segment is candidate
         *  if > 1 segment:
         *      end of track (more than 2 segment connected at this location)
         */
        segment = m_Track; candidate = NULL;
        NbSegm  = 0;

        while( ( segment = ::GetTrace( segment, NULL, aPosition, aLayerMask ) ) != NULL )
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
            aLayerMask = candidate->ReturnMaskLayer();

            if( aPosition == candidate->m_Start )
            {
                aPosition = candidate->m_End;
            }
            else
            {
                aPosition = candidate->m_Start;
            }

            segment = m_Track; /* restart list of tracks to analyze */

            /* flag this item an push it in list of selected items */
            aList->push_back( candidate );
            candidate->SetState( BUSY, ON );
        }
        else
        {
            return;
        }
    }
}


void BOARD::PushHighLight()
{
    m_hightLightPrevious = m_hightLight;
}


void BOARD::PopHighLight()
{
    m_hightLight = m_hightLightPrevious;
    m_hightLightPrevious.Clear();
}


bool BOARD::SetCurrentNetClass( const wxString& aNetClassName )
{
    NETCLASS* netClass = m_NetClasses.Find( aNetClassName );
    bool      lists_sizes_modified = false;

    // if not found (should not happen) use the default
    if( netClass == NULL )
        netClass = m_NetClasses.GetDefault();

    m_CurrentNetClassName = netClass->GetName();

    // Initialize others values:
    if( m_ViasDimensionsList.size() == 0 )
    {
        VIA_DIMENSION viadim;
        lists_sizes_modified = true;
        m_ViasDimensionsList.push_back( viadim );
    }

    if( m_TrackWidthList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_TrackWidthList.push_back( 0 );
    }

    /* note the m_ViasDimensionsList[0] and m_TrackWidthList[0] values
     * are always the Netclass values
     */
    if( m_ViasDimensionsList[0].m_Diameter != netClass->GetViaDiameter() )
        lists_sizes_modified = true;

    m_ViasDimensionsList[0].m_Diameter = netClass->GetViaDiameter();

    if( m_TrackWidthList[0] != netClass->GetTrackWidth() )
        lists_sizes_modified = true;

    m_TrackWidthList[0] = netClass->GetTrackWidth();

    if( m_ViaSizeSelector >= m_ViasDimensionsList.size() )
        m_ViaSizeSelector = m_ViasDimensionsList.size();

    if( m_TrackWidthSelector >= m_TrackWidthList.size() )
        m_TrackWidthSelector = m_TrackWidthList.size();

    return lists_sizes_modified;
}


int BOARD::GetBiggestClearanceValue()
{
    int clearance = m_NetClasses.GetDefault()->GetClearance();

    //Read list of Net Classes
    for( NETCLASSES::const_iterator nc = m_NetClasses.begin(); nc != m_NetClasses.end(); nc++ )
    {
        NETCLASS* netclass = nc->second;
        clearance = MAX( clearance, netclass->GetClearance() );
    }

    return clearance;
}


int BOARD::GetSmallestClearanceValue()
{
    int clearance = m_NetClasses.GetDefault()->GetClearance();

    //Read list of Net Classes
    for( NETCLASSES::const_iterator nc = m_NetClasses.begin(); nc != m_NetClasses.end(); nc++ )
    {
        NETCLASS* netclass = nc->second;
        clearance = MIN( clearance, netclass->GetClearance() );
    }

    return clearance;
}


int BOARD::GetCurrentMicroViaSize()
{
    NETCLASS* netclass = m_NetClasses.Find( m_CurrentNetClassName );

    return netclass->GetuViaDiameter();
}


int BOARD::GetCurrentMicroViaDrill()
{
    NETCLASS* netclass = m_NetClasses.Find( m_CurrentNetClassName );

    return netclass->GetuViaDrill();
}


wxString BOARD::GetLayerName( int aLayerIndex ) const
{
    if( !IsValidLayerIndex( aLayerIndex ) )
        return wxEmptyString;

    // copper layer names are stored in the BOARD.
    if( IsValidCopperLayerIndex( aLayerIndex ) && IsLayerEnabled( aLayerIndex ) )
    {
        // default names were set in BOARD::BOARD() but they may be
        // over-ridden by BOARD::SetLayerName()
        return m_Layer[aLayerIndex].m_Name;
    }

    return GetDefaultLayerName( aLayerIndex );
}


wxString BOARD::GetDefaultLayerName( int aLayerNumber )
{
    const wxChar* txt;

    // These are only default layer names.  For Pcbnew the copper names
    // may be over-ridden in the BOARD (*.brd) file.

    // Use a switch to explicitly show the mapping more clearly
    switch( aLayerNumber )
    {
    case LAYER_N_FRONT:         txt = _( "Front" );         break;
    case LAYER_N_2:             txt = _( "Inner2" );        break;
    case LAYER_N_3:             txt = _( "Inner3" );        break;
    case LAYER_N_4:             txt = _( "Inner4" );        break;
    case LAYER_N_5:             txt = _( "Inner5" );        break;
    case LAYER_N_6:             txt = _( "Inner6" );        break;
    case LAYER_N_7:             txt = _( "Inner7" );        break;
    case LAYER_N_8:             txt = _( "Inner8" );        break;
    case LAYER_N_9:             txt = _( "Inner9" );        break;
    case LAYER_N_10:            txt = _( "Inner10" );       break;
    case LAYER_N_11:            txt = _( "Inner11" );       break;
    case LAYER_N_12:            txt = _( "Inner12" );       break;
    case LAYER_N_13:            txt = _( "Inner13" );       break;
    case LAYER_N_14:            txt = _( "Inner14" );       break;
    case LAYER_N_15:            txt = _( "Inner15" );       break;
    case LAYER_N_BACK:          txt = _( "Back" );          break;
    case ADHESIVE_N_BACK:       txt = _( "Adhes_Back" );    break;
    case ADHESIVE_N_FRONT:      txt = _( "Adhes_Front" );   break;
    case SOLDERPASTE_N_BACK:    txt = _( "SoldP_Back" );    break;
    case SOLDERPASTE_N_FRONT:   txt = _( "SoldP_Front" );   break;
    case SILKSCREEN_N_BACK:     txt = _( "SilkS_Back" );    break;
    case SILKSCREEN_N_FRONT:    txt = _( "SilkS_Front" );   break;
    case SOLDERMASK_N_BACK:     txt = _( "Mask_Back" );     break;
    case SOLDERMASK_N_FRONT:    txt = _( "Mask_Front" );    break;
    case DRAW_N:                txt = _( "Drawings" );      break;
    case COMMENT_N:             txt = _( "Comments" );      break;
    case ECO1_N:                txt = _( "Eco1" );          break;
    case ECO2_N:                txt = _( "Eco2" );          break;
    case EDGE_N:                txt = _( "PCB_Edges" );     break;
    default:                    txt = _( "BAD INDEX" );     break;
    }

    return wxString( txt );
}


bool BOARD::SetLayerName( int aLayerIndex, const wxString& aLayerName )
{
    if( !IsValidCopperLayerIndex( aLayerIndex ) )
        return false;

    if( aLayerName == wxEmptyString  || aLayerName.Len() > 20 )
        return false;

    // no quote chars in the name allowed
    if( aLayerName.Find( wxChar( '"' ) ) != wxNOT_FOUND )
        return false;

    wxString NameTemp = aLayerName;

    // replace any spaces with underscores before we do any comparing
    NameTemp.Replace( wxT( " " ), wxT( "_" ) );

    if( IsLayerEnabled( aLayerIndex ) )
    {
        for( int i = 0; i < NB_COPPER_LAYERS; i++ )
        {
            if( i != aLayerIndex && IsLayerEnabled( i ) && NameTemp == m_Layer[i].m_Name )
                return false;
        }

        m_Layer[aLayerIndex].m_Name = NameTemp;

        return true;
    }

    return false;
}


LAYER_T BOARD::GetLayerType( int aLayerIndex ) const
{
    if( !IsValidCopperLayerIndex( aLayerIndex ) )
        return LT_SIGNAL;

    //@@IMB: The original test was broken due to the discontinuity
    // in the layer sequence.
    if( IsLayerEnabled( aLayerIndex ) )
        return m_Layer[aLayerIndex].m_Type;

    return LT_SIGNAL;
}


bool BOARD::SetLayerType( int aLayerIndex, LAYER_T aLayerType )
{
    if( !IsValidCopperLayerIndex( aLayerIndex ) )
        return false;

    //@@IMB: The original test was broken due to the discontinuity
    // in the layer sequence.
    if( IsLayerEnabled( aLayerIndex ) )
    {
        m_Layer[aLayerIndex].m_Type = aLayerType;
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
        return LAYER_T( -1 );
}

int BOARD::GetCopperLayerCount() const
{
    return m_designSettings.GetCopperLayerCount();
}

void BOARD::SetCopperLayerCount( int aCount )
{
    m_designSettings.SetCopperLayerCount( aCount );
}


int BOARD::GetEnabledLayers() const
{
    return m_designSettings.GetEnabledLayers();
}


int BOARD::GetVisibleLayers() const
{
    return m_designSettings.GetVisibleLayers();
}


void BOARD::SetEnabledLayers( int aLayerMask )
{
    m_designSettings.SetEnabledLayers( aLayerMask );
}


void BOARD::SetVisibleLayers( int aLayerMask )
{
    m_designSettings.SetVisibleLayers( aLayerMask );
}


// these are not tidy, since there are PCB_VISIBLEs that are not stored in the bitmap.

void BOARD::SetVisibleElements( int aMask )
{
    /* Call SetElementVisibility for each item,
     * to ensure specific calculations that can be needed by some items
     * just change the visibility flags could be not sufficient
     */
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

        // we must clear or set the CH_VISIBLE flags to hide/show ratsnet
        // because we have a tool to show hide ratsnest relative to a pad or a module
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


int BOARD::GetVisibleElementColor( int aPCB_VISIBLE )
{
    int color = -1;

    switch( aPCB_VISIBLE )
    {
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


void BOARD::SetVisibleElementColor( int aPCB_VISIBLE, int aColor )
{
    switch( aPCB_VISIBLE )
    {
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


void BOARD::SetLayerColor( int aLayer, int aColor )
{
    GetColorsSettings()->SetLayerColor( aLayer, aColor );
}


int BOARD::GetLayerColor( int aLayer )
{
    return GetColorsSettings()->GetLayerColor( aLayer );
}


bool BOARD::IsModuleLayerVisible( int layer )
{
    if( layer==LAYER_N_FRONT )
        return IsElementVisible( PCB_VISIBLE(MOD_FR_VISIBLE) );
    else if( layer==LAYER_N_BACK )
        return IsElementVisible( PCB_VISIBLE(MOD_BK_VISIBLE) );
    else
        return true;
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


unsigned BOARD::GetNoconnectCount() const
{
    return m_NbNoconnect;
}


unsigned BOARD::GetNodesCount() const
{
    return m_NbNodes;
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
void BOARD::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString txt;

    frame->ClearMsgPanel();

    int viasCount = 0;
    int trackSegmentsCount = 0;

    for( BOARD_ITEM* item = m_Track; item; item = item->Next() )
    {
        if( item->Type() == PCB_VIA_T )
            viasCount++;
        else
            trackSegmentsCount++;
    }

    txt.Printf( wxT( "%d" ), GetPadCount() );
    frame->AppendMsgPanel( _( "Pads" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), viasCount );
    frame->AppendMsgPanel( _( "Vias" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), trackSegmentsCount );
    frame->AppendMsgPanel( _( "trackSegm" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), GetNodesCount() );
    frame->AppendMsgPanel( _( "Nodes" ), txt, DARKCYAN );

    txt.Printf( wxT( "%d" ), m_NetInfo.GetNetCount() );
    frame->AppendMsgPanel( _( "Nets" ), txt, RED );

    /* These parameters are known only if the full ratsnest is available,
     *  so, display them only if this is the case
     */
    if( (m_Status_Pcb & NET_CODES_OK) )
    {
        txt.Printf( wxT( "%d" ), GetRatsnestsCount() );
        frame->AppendMsgPanel( _( "Links" ), txt, DARKGREEN );

        txt.Printf( wxT( "%d" ), GetRatsnestsCount() - GetNoconnectCount() );
        frame->AppendMsgPanel( _( "Connect" ), txt, DARKGREEN );

        txt.Printf( wxT( "%d" ), GetNoconnectCount() );
        frame->AppendMsgPanel( _( "Unconnected" ), txt, BLUE );
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
        // But we are not using separate lists for TRACKs and SEGVIAs, because
        // items are ordered (sorted) in the linked
        // list by netcode AND by physical distance:
        // when created, if a track or via is connected to an existing track or
        // via, it is put in linked list after this existing track or via
        // So usually, connected tracks or vias are grouped in this list
        // So the algorithm (used in ratsnest computations) which computes the
        // track connectivity is faster (more than 100 time regarding to
        // a non ordered list) because when it searches for a connexion, first
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
 *  BOARD_ITEM* BOARD::FindPadOrModule( const wxPoint& refPos, int layer )
 *  {
 *   class PadOrModule : public INSPECTOR
 *   {
 *   public:
 *       BOARD_ITEM*         found;
 *       int                 layer;
 *       int                 layer_mask;
 *
 *       PadOrModule( int alayer ) :
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
 *                   if( layer_mask & pad->m_layerMask )
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

#if defined(DEBUG)
    if( net )     // item can be NULL if anetcode is not valid
    {
        if( aNetcode != net->GetNet() )
        {
            printf( "FindNet() anetcode %d != GetNet() %d (net: %s)\n",
                    aNetcode, net->GetNet(), TO_UTF8( net->GetNetname() ) );
        }
    }
#endif

    return net;
}


NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname ) const
{
    // the first valid netcode is 1.
    // zero is reserved for "no connection" and is not used.
    if( aNetname.IsEmpty() )
        return NULL;

    int ncount = m_NetInfo.GetNetCount();

    // Search for a netname = aNetname
#if 0

    // Use a sequential search: easy to understand, but slow
    for( int ii = 1; ii < ncount; ii++ )
    {
        NETINFO_ITEM* item = m_NetInfo.GetNetItem( ii );

        if( item && item->GetNetname() == aNetname )
        {
            return item;
        }
    }

#else

    // Use a fast binary search,
    // this is possible because Nets are alphabetically ordered in list
    // see NETINFO_LIST::BuildListOfNets() and
    // NETINFO_LIST::Build_Pads_Full_List()
    int imax  = ncount - 1;
    int index = imax;

    while( ncount > 0 )
    {
        int ii = ncount;
        ncount >>= 1;

        if( (ii & 1) && ( ii > 1 ) )
            ncount++;

        NETINFO_ITEM* item = m_NetInfo.GetNetItem( index );

        if( item == NULL )
            return NULL;

        int           icmp = item->GetNetname().Cmp( aNetname );

        if( icmp == 0 ) // found !
        {
            return item;
        }

        if( icmp < 0 ) // must search after item
        {
            index += ncount;

            if( index > imax )
                index = imax;

            continue;
        }

        if( icmp > 0 ) // must search before item
        {
            index -= ncount;

            if( index < 1 )
                index = 1;

            continue;
        }
    }

#endif
    return NULL;
}


MODULE* BOARD::FindModuleByReference( const wxString& aReference ) const
{
    struct FindModule : public INSPECTOR
    {
        MODULE* found;
        FindModule() : found( 0 )  {}

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


// Sort nets by decreasing pad count
static bool s_SortByNodes( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
{
    return b->GetNodesCount() < a->GetNodesCount();
}


int BOARD::ReturnSortedNetnamesList( wxArrayString& aNames, bool aSortbyPadsCount )
{
    if( m_NetInfo.GetNetCount() == 0 )
        return 0;

    // Build the list
    std::vector <NETINFO_ITEM*> netBuffer;

    netBuffer.reserve( m_NetInfo.GetNetCount() );

    for( unsigned ii = 1; ii < m_NetInfo.GetNetCount(); ii++ )
    {
        if( m_NetInfo.GetNetItem( ii )->GetNet() > 0 )
            netBuffer.push_back( m_NetInfo.GetNetItem( ii ) );
    }

    // sort the list
    if( aSortbyPadsCount )
        sort( netBuffer.begin(), netBuffer.end(), s_SortByNodes );

    for( unsigned ii = 0; ii <  netBuffer.size(); ii++ )
        aNames.Add( netBuffer[ii]->GetNetname() );

    return netBuffer.size();
}


void BOARD::RedrawAreasOutlines( EDA_DRAW_PANEL* panel, wxDC* aDC, int aDrawMode, int aLayer )
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


void BOARD::RedrawFilledAreas( EDA_DRAW_PANEL* panel, wxDC* aDC, int aDrawMode, int aLayer )
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
                                                int            aStartLayer,
                                                int            aEndLayer )
{
    if( aEndLayer < 0 )
        aEndLayer = aStartLayer;

    if( aEndLayer <  aStartLayer )
        EXCHG( aEndLayer, aStartLayer );

    for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
    {
        ZONE_CONTAINER* area  = m_ZoneDescriptorList[ia];
        int             layer = area->GetLayer();

        if( (layer < aStartLayer) || (layer > aEndLayer) )
            continue;

        // In locate functions we must skip tagged items with BUSY flag set.
        if( area->GetState( BUSY ) )
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
            GetArea( ii )->SetNet( 0 );
            continue;
        }

        if( GetArea( ii )->GetNet() != 0 )      // i.e. if this zone is connected to a net
        {
            const NETINFO_ITEM* net = FindNet( GetArea( ii )->m_Netname );

            if( net )
            {
                GetArea( ii )->SetNet( net->GetNet() );
            }
            else
            {
                error_count++;

                // keep Net Name and set m_NetCode to -1 : error flag.
                GetArea( ii )->SetNet( -1 );
            }
        }
    }

    return error_count;
}


TRACK* BOARD::GetViaByPosition( const wxPoint& aPosition, int aLayerMask )
{
    TRACK* track;

    for( track = m_Track;  track; track = track->Next() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        if( track->m_Start != aPosition )
            continue;

        if( track->GetState( BUSY | IS_DELETED ) )
            continue;

        if( aLayerMask < 0 )
            break;

        if( track->IsOnLayer( aLayerMask ) )
            break;
    }

    return track;
}


D_PAD* BOARD::GetPad( const wxPoint& aPosition, int aLayerMask )
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


D_PAD* BOARD::GetPad( TRACK* aTrace, int aEndPoint )
{
    D_PAD*  pad = NULL;
    wxPoint aPosition;

    int     aLayerMask = GetLayerMask( aTrace->GetLayer() );

    if( aEndPoint == START )
    {
        aPosition = aTrace->m_Start;
    }
    else
    {
        aPosition = aTrace->m_End;
    }

    for( MODULE* module = m_Modules;  module;  module = module->Next() )
    {
        pad = module->GetPad( aPosition, aLayerMask );

        if( pad != NULL )
            break;
    }

    return pad;
}


D_PAD* BOARD::GetPadFast( const wxPoint& aPosition, int aLayerMask )
{
    for( unsigned i=0; i<GetPadCount();  ++i )
    {
        D_PAD* pad = m_NetInfo.GetPad(i);

        if( pad->m_Pos != aPosition )
            continue;

        /* Pad found, it must be on the correct layer */
        if( pad->m_layerMask & aLayerMask )
            return pad;
    }

    return NULL;
}


D_PAD* BOARD::GetPad( std::vector<D_PAD*>& aPadList, const wxPoint& aPosition, int aLayerMask )
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

        if( pad->m_Pos == aPosition )       // candidate found
        {
            // The pad must match the layer mask:
            if( (aLayerMask & pad->m_layerMask) != 0 )
                return pad;

            // More than one pad can be at aPosition
            // search for a pad at aPosition that matched this mask

            // search next
            for( int ii = idx+1; ii <= idxmax; ii++ )
            {
                pad = aPadList[ii];
                if( pad->m_Pos != aPosition )
                    break;
                if( (aLayerMask & pad->m_layerMask) != 0 )
                    return pad;
            }
            // search previous
            for(  int ii = idx-1 ;ii >=0; ii-- )
            {
                pad = aPadList[ii];
                if( pad->m_Pos != aPosition )
                    break;
                if( (aLayerMask & pad->m_layerMask) != 0 )
                    return pad;
            }

            // Not found:
            return 0;
        }

        if( pad->m_Pos.x == aPosition.x )   // Must search considering Y coordinate
        {
            if(pad->m_Pos.y < aPosition.y)  // Must search after this item
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
        else if( pad->m_Pos.x < aPosition.x ) // Must search after this item
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
 */
static bool sortPadsByXthenYCoord( D_PAD* const & ref, D_PAD* const & comp )
{
    if( ref->m_Pos.x == comp->m_Pos.x )
        return ref->m_Pos.y < comp->m_Pos.y;
    return ref->m_Pos.x < comp->m_Pos.x;
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


TRACK* BOARD::GetTrace( TRACK* aTrace, const wxPoint& aPosition, int aLayerMask )
{
    for( TRACK* track = aTrace;   track;  track =  track->Next() )
    {
        int layer = track->GetLayer();

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


TRACK* BOARD::MarkTrace( TRACK* aTrace,
                         int*   aCount,
                         int*   aTraceLength,
                         int*   aDieLength,
                         bool   aReorder )
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
        track->SetState( BUSY, OFF );

    /* Set flags of the initial track segment */
    aTrace->SetState( BUSY, ON );
    int layerMask = aTrace->ReturnMaskLayer();

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
        Segm1 = ::GetTrace( m_Track, NULL, aTrace->m_Start, layerMask );

        if( Segm1 )
        {
            Segm2 = ::GetTrace( Segm1->Next(), NULL, aTrace->m_Start, layerMask );
        }

        if( Segm2 )
        {
            Segm3 = ::GetTrace( Segm2->Next(), NULL, aTrace->m_Start, layerMask );
        }

        if( Segm3 ) // More than 2 segments are connected to this via. the track" is only this via
        {
            if( aCount )
                *aCount = 1;

            return aTrace;
        }

        if( Segm1 ) // search for others segments connected to the initial segment start point
        {
            layerMask = Segm1->ReturnMaskLayer();
            chainMarkedSegments( aTrace->m_Start, layerMask, &trackList );
        }

        if( Segm2 ) // search for others segments connected to the initial segment end point
        {
            layerMask = Segm2->ReturnMaskLayer();
            chainMarkedSegments( aTrace->m_Start, layerMask, &trackList );
        }
    }
    else    // mark the chain using both ends of the initial segment
    {
        chainMarkedSegments( aTrace->m_Start, layerMask, &trackList );
        chainMarkedSegments( aTrace->m_End, layerMask, &trackList );
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

        via->SetState( BUSY, ON );  // Try to flag it. the flag will be cleared later if needed

        layerMask = via->ReturnMaskLayer();

        TRACK* track = ::GetTrace( m_Track, NULL, via->m_Start, layerMask );

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
        int layer = track->GetLayer();

        while( ( track = ::GetTrace( track->Next(), NULL, via->m_Start, layerMask ) ) != NULL )
        {
            if( layer != track->GetLayer() )
            {
                // The via connects segments of an other track: it is removed
                // from list because it is member of an other track
                via->SetState( BUSY, OFF );
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
    double lenDie = 0;

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

                if( aDieLength ) // Add now length die.
                {
                    // In fact only 2 pads (maximum) will be taken in account:
                    // that are on each end of the track, if any
                    if( track->GetState( BEGIN_ONPAD ) )
                    {
                        D_PAD * pad = (D_PAD *) track->start;
                        lenDie += (double) pad->m_LengthDie;
                    }

                    if( track->GetState( END_ONPAD ) )
                    {
                        D_PAD * pad = (D_PAD *) track->end;
                        lenDie += (double) pad->m_LengthDie;
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
                track->SetState( BUSY, OFF );
                full_len += track->GetLength();

                // Add now length die.
                // In fact only 2 pads (maximum) will be taken in account:
                // that are on each end of the track, if any
                if( track->GetState( BEGIN_ONPAD ) )
                {
                    D_PAD * pad = (D_PAD *) track->start;
                    lenDie += (double) pad->m_LengthDie;
                }

                if( track->GetState( END_ONPAD ) )
                {
                    D_PAD * pad = (D_PAD *) track->end;
                    lenDie += (double) pad->m_LengthDie;
                }
            }
        }
    }

    if( aTraceLength )
        *aTraceLength = wxRound( full_len );

    if( aDieLength )
        *aDieLength = wxRound( lenDie );

    if( aCount )
        *aCount = NbSegmBusy;

    return firstTrack;
}


MODULE* BOARD::GetFootprint( const wxPoint& aPosition, int aActiveLayer,
                             bool aVisibleOnly, bool aIgnoreLocked )
{
    MODULE* pt_module;
    MODULE* module      = NULL;
    MODULE* Altmodule   = NULL;
    int     min_dim     = 0x7FFFFFFF;
    int     alt_min_dim = 0x7FFFFFFF;
    int     layer;

    for( pt_module = m_Modules;  pt_module;  pt_module = (MODULE*) pt_module->Next() )
    {
        // is the ref point within the module's bounds?
        if( !pt_module->HitTest( aPosition ) )
            continue;

        // if caller wants to ignore locked modules, and this one is locked, skip it.
        if( aIgnoreLocked && pt_module->IsLocked() )
            continue;

        /* Calculate priority: the priority is given to the layer of the
         * module and the copper layer if the module layer is indelible,
         * adhesive copper, a layer if cmp module layer is indelible,
         * adhesive component.
         */
        layer = pt_module->GetLayer();

        if( layer==ADHESIVE_N_BACK || layer==SILKSCREEN_N_BACK )
            layer = LAYER_N_BACK;
        else if( layer==ADHESIVE_N_FRONT || layer==SILKSCREEN_N_FRONT )
            layer = LAYER_N_FRONT;

        /* Test of minimum size to choosing the best candidate. */

        EDA_RECT bb = pt_module->GetFootPrintRect();
        int offx = bb.GetX() + bb.GetWidth() / 2;
        int offy = bb.GetY() + bb.GetHeight() / 2;

        //off x & offy point to the middle of the box.
        int dist = abs( aPosition.x - offx ) + abs( aPosition.y - offy );

        //int dist = MIN(lx, ly);  // to pick the smallest module (kinda
        // screwy with same-sized modules -- this is bad!)

        if( aActiveLayer == layer )
        {
            if( dist <= min_dim )
            {
                /* better footprint shown on the active layer */
                module  = pt_module;
                min_dim = dist;
            }
        }
        else if( aVisibleOnly && IsModuleLayerVisible( layer ) )
        {
            if( dist <= alt_min_dim )
            {
                /* better footprint shown on other layers */
                Altmodule   = pt_module;
                alt_min_dim = dist;
            }
        }
    }

    if( module )
    {
        return module;
    }

    if( Altmodule )
    {
        return Altmodule;
    }

    return NULL;
}


BOARD_CONNECTED_ITEM* BOARD::GetLockPoint( const wxPoint& aPosition, int aLayerMask )
{
    for( MODULE* module = m_Modules; module; module = module->Next() )
    {
        D_PAD* pad = module->GetPad( aPosition, aLayerMask );

        if( pad )
            return pad;
    }

    /* No pad has been located so check for a segment of the trace. */
    TRACK* segment = ::GetTrace( m_Track, NULL, aPosition, aLayerMask );

    if( segment == NULL )
        segment = GetTrace( m_Track, aPosition, aLayerMask );

    return segment;
}


TRACK* BOARD::CreateLockPoint( wxPoint& aPosition, TRACK* aSegment, PICKED_ITEMS_LIST* aList )
{
    /* creates an intermediate point on aSegment and break it into two segments
     * at aPosition.
     * The new segment starts from aPosition and ends at the end point of
     * aSegment. The original segment now ends at aPosition.
     */
    if( aSegment->m_Start == aPosition || aSegment->m_End == aPosition )
        return NULL;

    /* A via is a good lock point */
    if( aSegment->Type() == PCB_VIA_T )
    {
        aPosition = aSegment->m_Start;
        return aSegment;
    }

    // Calculation coordinate of intermediate point relative to the start point of aSegment
     wxPoint delta = aSegment->m_End - aSegment->m_Start;

    // calculate coordinates of aPosition relative to aSegment->m_Start
    wxPoint lockPoint = aPosition - aSegment->m_Start;

    // lockPoint must be on aSegment:
    // Ensure lockPoint.y/lockPoint.y = delta.y/delta.x
    if( delta.x == 0 )
        lockPoint.x = 0;         /* horizontal segment*/
    else
        lockPoint.y = wxRound( ( (double)lockPoint.x * delta.y ) / delta.x );

    /* Create the intermediate point (that is to say creation of a new
     * segment, beginning at the intermediate point.
     */
    lockPoint += aSegment->m_Start;

    TRACK* newTrack = (TRACK*)aSegment->Clone();
    // The new segment begins at the new point,
    newTrack->m_Start = lockPoint;
    newTrack->start = aSegment;
    newTrack->SetState( BEGIN_ONPAD, OFF );

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
    aSegment->m_End = lockPoint;
    aSegment->end = newTrack;
    aSegment->SetState( END_ONPAD, OFF );

    D_PAD * pad = GetPad( newTrack, START );

    if ( pad )
    {
        newTrack->start = pad;
        newTrack->SetState( BEGIN_ONPAD, ON );
        aSegment->end = pad;
        aSegment->SetState( END_ONPAD, ON );
    }

    aPosition = lockPoint;
    return newTrack;
}


#if defined(DEBUG)

void BOARD::Show( int nestLevel, std::ostream& os ) const
{
    BOARD_ITEM* p;

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";

    // specialization of the output:
    NestedSpace( nestLevel + 1, os ) << "<modules>\n";
    p = m_Modules;
    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );
    NestedSpace( nestLevel + 1, os ) << "</modules>\n";

    NestedSpace( nestLevel + 1, os ) << "<pdrawings>\n";
    p = m_Drawings;
    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );
    NestedSpace( nestLevel + 1, os ) << "</pdrawings>\n";

    NestedSpace( nestLevel + 1, os ) << "<tracks>\n";
    p = m_Track;
    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );
    NestedSpace( nestLevel + 1, os ) << "</tracks>\n";

    NestedSpace( nestLevel + 1, os ) << "<zones>\n";
    p = m_Zone;
    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );
    NestedSpace( nestLevel + 1, os ) << "</zones>\n";

    NestedSpace( nestLevel+1, os ) << "<zone_containers>\n";
    for( ZONE_CONTAINERS::const_iterator it = m_ZoneDescriptorList.begin();
                it != m_ZoneDescriptorList.end();  ++it )
        (*it)->Show( nestLevel+2, os );

    NestedSpace( nestLevel+1, os ) << "</zone_containers>\n";

    p = (BOARD_ITEM*) m_Son;
    for( ; p; p = p->Next() )
    {
        p->Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
