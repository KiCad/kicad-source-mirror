/*******************************************/
/* class_board.cpp - BOARD class functions */
/*******************************************/
#include <limits.h>
#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"


/* This is an odd place for this, but cvpcb won't link if it is
 *  in class_board_item.cpp like I first tried it.
 */
wxPoint BOARD_ITEM::ZeroOffset( 0, 0 );


// Current design settings (used also to read configs):
BOARD_DESIGN_SETTINGS boardDesignSettings;


BOARD::BOARD( EDA_ITEM* parent, PCB_BASE_FRAME* frame ) :
    BOARD_ITEM( (BOARD_ITEM*)parent, TYPE_PCB ),
    m_NetClasses( this )
{
    m_PcbFrame = frame;
    m_Status_Pcb    = 0;                    // Status word: bit 1 = calculate.
    SetBoardDesignSettings(&boardDesignSettings);
    SetColorsSettings(&g_ColorsSettings);
    m_NbNodes     = 0;                      // Number of connected pads.
    m_NbNoconnect = 0;                      // Number of unconnected nets.

    m_CurrentZoneContour = NULL;            // This ZONE_CONTAINER handle the
                                            // zone contour currently in
                                            // progress
    m_NetInfo = new NETINFO_LIST( this );   // handle nets info list (name,
                                            // design constraints ..
    m_NetInfo->BuildListOfNets();           // prepare pads and nets lists containers.

    for( int layer = 0; layer < NB_COPPER_LAYERS; ++layer )
    {
        m_Layer[layer].m_Name = GetDefaultLayerName( layer );
        m_Layer[layer].m_Type = LT_SIGNAL;
    }

    // Initial parameters for the default NETCLASS come from the global
    // preferences
    // within g_DesignSettings via the NETCLASS() constructor.
    // Should user eventually load a board from a disk file, then these
    // defaults
    // will get overwritten during load.
    m_NetClasses.GetDefault()->SetDescription( _( "This is the default net class." ) );
    m_ViaSizeSelector    = 0;
    m_TrackWidthSelector = 0;

    // Initialize default values.
    SetCurrentNetClass( m_NetClasses.GetDefault()->GetName() );
}


BOARD::~BOARD()
{
    if( m_PcbFrame->GetScreen() )
        m_PcbFrame->GetScreen()->ClearUndoRedoList();

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

    delete m_NetInfo;
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

    // These are only default layer names.  For PCBNEW, the copper names
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
    return GetBoardDesignSettings()->GetCopperLayerCount();
}

void BOARD::SetCopperLayerCount( int aCount )
{
    GetBoardDesignSettings()->SetCopperLayerCount( aCount );
}


int BOARD::GetEnabledLayers() const
{
    return GetBoardDesignSettings()->GetEnabledLayers();
}


int BOARD::GetVisibleLayers() const
{
    return GetBoardDesignSettings()->GetVisibleLayers();
}


void BOARD::SetEnabledLayers( int aLayerMask )
{
    GetBoardDesignSettings()->SetEnabledLayers( aLayerMask );
}


void BOARD::SetVisibleLayers( int aLayerMask )
{
    GetBoardDesignSettings()->SetVisibleLayers( aLayerMask );
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

// these are not tidy, since there are PCB_VISIBLEs that are not stored in the bitmap.
void BOARD::SetVisibleAlls(  )
{
    SetVisibleLayers( FULL_LAYERS );

    /* Call SetElementVisibility for each item,
     * to ensure specific calculations that can be needed by some items
     */
    for( int ii = 0; ii < PCB_VISIBLE(END_PCB_VISIBLE_LIST); ii++ )
        SetElementVisibility( ii, true );
}


int BOARD::GetVisibleElements() const
{
    return GetBoardDesignSettings()->GetVisibleElements();
}


bool BOARD::IsElementVisible( int aPCB_VISIBLE ) const
{
    return GetBoardDesignSettings()->IsElementVisible( aPCB_VISIBLE );
}


void BOARD::SetElementVisibility( int aPCB_VISIBLE, bool isEnabled )
{
    switch( aPCB_VISIBLE )
    {
    case RATSNEST_VISIBLE:
        GetBoardDesignSettings()->SetElementVisibility( aPCB_VISIBLE, isEnabled );
        // we must clear or set the CH_VISIBLE flags to hide/show ratsnet
        // because we have a tool to show hide ratsnest relative to a pad or a module
        // so the hide/show option is a per item selection
        if( IsElementVisible(RATSNEST_VISIBLE) )
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
        GetBoardDesignSettings()->SetElementVisibility( aPCB_VISIBLE, isEnabled );
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



wxPoint& BOARD::GetPosition()
{
    static wxPoint dummy( 0, 0 );

    return dummy;   // a reference
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
    case TYPE_MARKER_PCB:
        aBoardItem->SetParent( this );
        m_markers.push_back( (MARKER_PCB*) aBoardItem );
        break;

    // this one uses a vector
    case TYPE_ZONE_CONTAINER:
        aBoardItem->SetParent( this );
        m_ZoneDescriptorList.push_back( (ZONE_CONTAINER*) aBoardItem );
        break;

    case TYPE_TRACK:
    case TYPE_VIA:
        TRACK* insertAid;
        insertAid = ( (TRACK*) aBoardItem )->GetBestInsertPoint( this );
        m_Track.Insert( (TRACK*) aBoardItem, insertAid );
        break;

    case TYPE_ZONE:
        if( aControl & ADD_APPEND )
            m_Zone.PushBack( (SEGZONE*) aBoardItem );
        else
            m_Zone.PushFront( (SEGZONE*) aBoardItem );
        aBoardItem->SetParent( this );
        break;

    case TYPE_MODULE:
        if( aControl & ADD_APPEND )
            m_Modules.PushBack( (MODULE*) aBoardItem );
        else
            m_Modules.PushFront( (MODULE*) aBoardItem );

        aBoardItem->SetParent( this );

        // Because the list of pads has changed, reset the status
        // This indicate the list of pad and nets must be recalculated before use
        m_Status_Pcb = 0;
        break;

    case TYPE_DIMENSION:
    case TYPE_DRAWSEGMENT:
    case TYPE_TEXTE:
    case TYPE_EDGE_MODULE:
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
    case TYPE_MARKER_PCB:

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

    case TYPE_ZONE_CONTAINER:    // this one uses a vector
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

    case TYPE_MODULE:
        m_Modules.Remove( (MODULE*) aBoardItem );
        break;

    case TYPE_TRACK:
    case TYPE_VIA:
        m_Track.Remove( (TRACK*) aBoardItem );
        break;

    case TYPE_ZONE:
        m_Zone.Remove( (SEGZONE*) aBoardItem );
        break;

    case TYPE_DIMENSION:
    case TYPE_DRAWSEGMENT:
    case TYPE_TEXTE:
    case TYPE_EDGE_MODULE:
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


int BOARD::GetNumSegmTrack()
{
    return m_Track.GetCount();
}


int BOARD::GetNumSegmZone()
{
    return m_Zone.GetCount();
}


unsigned BOARD::GetNoconnectCount()
{
    return m_NbNoconnect;
}


unsigned BOARD::GetNodesCount()
{
    return m_NbNodes;
}


bool BOARD::ComputeBoundingBox( bool aBoardEdgesOnly )
{
    bool hasItems = false;
    EDA_RECT area;

    // Check segments, dimensions, texts, and fiducials
    for( BOARD_ITEM* item = m_Drawings; item != NULL; item = item->Next() )
    {
        if( aBoardEdgesOnly && item->Type() != TYPE_DRAWSEGMENT && item->GetLayer() != EDGE_N )
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

    if( !hasItems && m_PcbFrame )
    {
        if( m_PcbFrame->m_Draw_Sheet_Ref )
        {
            area.SetOrigin( 0, 0 );
            area.SetEnd( m_PcbFrame->GetScreen()->ReturnPageSize().x,
                         m_PcbFrame->GetScreen()->ReturnPageSize().y );
        }
        else
        {
            area.SetOrigin( -m_PcbFrame->GetScreen()->ReturnPageSize().x / 2,
                            -m_PcbFrame->GetScreen()->ReturnPageSize().y / 2 );
            area.SetEnd( m_PcbFrame->GetScreen()->ReturnPageSize().x / 2,
                         m_PcbFrame->GetScreen()->ReturnPageSize().y / 2 );
        }
    }

    m_BoundaryBox = area;

    return hasItems;
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
        if( item->Type() == TYPE_VIA )
            viasCount++;
        else
            trackSegmentsCount++;
    }

    txt.Printf( wxT( "%d" ), GetPadsCount() );
    frame->AppendMsgPanel( _( "Pads" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), viasCount );
    frame->AppendMsgPanel( _( "Vias" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), trackSegmentsCount );
    frame->AppendMsgPanel( _( "trackSegm" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), GetNodesCount() );
    frame->AppendMsgPanel( _( "Nodes" ), txt, DARKCYAN );

    txt.Printf( wxT( "%d" ), m_NetInfo->GetCount() );
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
        case TYPE_PCB:
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

        case TYPE_MODULE:
        case TYPE_PAD:
        case TYPE_TEXTE_MODULE:
        case TYPE_EDGE_MODULE:

            // this calls MODULE::Visit() on each module.
            result = IterateForward( m_Modules, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case TYPE_MODULE:
                case TYPE_PAD:
                case TYPE_TEXTE_MODULE:
                case TYPE_EDGE_MODULE:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        case TYPE_DRAWSEGMENT:
        case TYPE_TEXTE:
        case TYPE_DIMENSION:
        case PCB_TARGET_T:
            result = IterateForward( m_Drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case TYPE_DRAWSEGMENT:
                case TYPE_TEXTE:
                case TYPE_DIMENSION:
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
        case TYPE_VIA:
        case TYPE_TRACK:
            result = IterateForward( m_Track, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case TYPE_VIA:
                case TYPE_TRACK:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

#else
        case TYPE_VIA:
            result = IterateForward( m_Track, inspector, testData, p );
            ++p;
            break;

        case TYPE_TRACK:
            result = IterateForward( m_Track, inspector, testData, p );
            ++p;
            break;
#endif

        case TYPE_MARKER_PCB:

            // MARKER_PCBS are in the m_markers std::vector
            for( unsigned i = 0; i<m_markers.size(); ++i )
            {
                result = m_markers[i]->Visit( inspector, testData, p );

                if( result == SEARCH_QUIT )
                    break;
            }

            ++p;
            break;

        case TYPE_ZONE_CONTAINER:

            // TYPE_ZONE_CONTAINER are in the m_ZoneDescriptorList std::vector
            for( unsigned i = 0; i< m_ZoneDescriptorList.size(); ++i )
            {
                result = m_ZoneDescriptorList[i]->Visit( inspector, testData, p );

                if( result == SEARCH_QUIT )
                    break;
            }

            ++p;
            break;

        case TYPE_ZONE:
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
 *           if( item->Type() == TYPE_PAD )
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
 *           else if( item->Type() == TYPE_MODULE )
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
 *   static const KICAD_T scanTypes[] = { TYPE_PAD, TYPE_MODULE, EOT };
 *
 *   // visit this BOARD with the above inspector
 *   Visit( &inspector, &refPos, scanTypes );
 *
 *   return inspector.found;
 *  }
 */


NETINFO_ITEM* BOARD::FindNet( int aNetcode ) const
{
    // the first valid netcode is 1 and the last is m_NetInfo->GetCount()-1.
    // zero is reserved for "no connection" and is not used.
    // NULL is returned for non valid netcodes
    NETINFO_ITEM* net = m_NetInfo->GetNetItem( aNetcode );

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

    int ncount = m_NetInfo->GetCount();

    // Search for a netname = aNetname
#if 0

    // Use a sequential search: easy to understand, but slow
    for( int ii = 1; ii < ncount; ii++ )
    {
        NETINFO_ITEM* item = m_NetInfo->GetNetItem( ii );

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

        NETINFO_ITEM* item = m_NetInfo->GetNetItem( index );

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
    static const KICAD_T scanTypes[] = { TYPE_MODULE, EOT };

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
    if( m_NetInfo->GetCount() == 0 )
        return 0;

    // Build the list
    std::vector <NETINFO_ITEM*> netBuffer;

    netBuffer.reserve( m_NetInfo->GetCount() );

    for( unsigned ii = 1; ii < m_NetInfo->GetCount(); ii++ )
    {
        if( m_NetInfo->GetNetItem( ii )->GetNet() > 0 )
            netBuffer.push_back( m_NetInfo->GetNetItem( ii ) );
    }

    // sort the list
    if( aSortbyPadsCount )
        sort( netBuffer.begin(), netBuffer.end(), s_SortByNodes );

    for( unsigned ii = 0; ii <  netBuffer.size(); ii++ )
        aNames.Add( netBuffer[ii]->GetNetname() );

    return netBuffer.size();
}


bool BOARD::Save( FILE* aFile ) const
{
    bool        rc = false;
    BOARD_ITEM* item;

    // save the nets
    for( unsigned ii = 0; ii < m_NetInfo->GetCount(); ii++ )
        if( !m_NetInfo->GetNetItem( ii )->Save( aFile ) )
            goto out;

    // Saved nets do not include netclass names, so save netclasses after nets.
    m_NetClasses.Save( aFile );

    // save the modules
    for( item = m_Modules; item; item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    for( item = m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_TEXTE:
        case TYPE_DRAWSEGMENT:
        case PCB_TARGET_T:
        case TYPE_DIMENSION:
            if( !item->Save( aFile ) )
                goto out;

            break;

        default:

            // future: throw exception here
#if defined(DEBUG)
            printf( "BOARD::Save() ignoring m_Drawings type %d\n", item->Type() );
#endif
            break;
        }
    }

    // do not save MARKER_PCBs, they can be regenerated easily

    // save the tracks & vias
    fprintf( aFile, "$TRACK\n" );

    for( item = m_Track; item; item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    fprintf( aFile, "$EndTRACK\n" );

    // save the zones
    fprintf( aFile, "$ZONE\n" );

    for( item = m_Zone; item; item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    fprintf( aFile, "$EndZONE\n" );

    // save the zone edges
    for( unsigned ii = 0; ii < m_ZoneDescriptorList.size(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_ZoneDescriptorList[ii];
        edge_zone->Save( aFile );
    }


    if( fprintf( aFile, "$EndBOARD\n" ) != sizeof("$EndBOARD\n") - 1 )
        goto out;

    rc = true;  // wrote all OK

out:
    return rc;
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
        if( track->Type() != TYPE_VIA )
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


#if defined(DEBUG)

void BOARD::Show( int nestLevel, std::ostream& os )
{
    BOARD_ITEM* p;

    // for now, make it look like XML:
    NestedSpace( nestLevel,
                 os ) << '<' << GetClass().Lower().mb_str() << ">\n";

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

    /*
     *  NestedSpace( nestLevel+1, os ) << "<zone_container>\n";
     *  for( ZONE_CONTAINERS::iterator i=m_ZoneDescriptorList.begin();
     *  i!=m_ZoneDescriptorList.end();  ++i )
     *   (*i)->Show( nestLevel+2, os );
     *  NestedSpace( nestLevel+1, os ) << "</zone_container>\n";
     */

    p = (BOARD_ITEM*) m_Son;

    for( ; p; p = p->Next() )
    {
        p->Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
