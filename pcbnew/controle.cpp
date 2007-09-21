/********************************************************/
/* Routines generales de gestion des commandes usuelles */
/********************************************************/

/* controle.cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "id.h"
#include "protos.h"
#include "collectors.h"

#include "bitmaps.h"
#include "add_cotation.xpm"
#include "Add_Mires.xpm"
#include "Add_Zone.xpm"


/* Routines Locales : */

/* Variables Locales */

/****************************************/
void RemoteCommand( const char* cmdline )
/****************************************/

/* Read a remote command send by eeschema via a socket,
 *  port KICAD_PCB_PORT_SERVICE_NUMBER (currently 4242)
 */
{
    char             line[1024];
    wxString         msg;
    char*            idcmd;
    char*            text;
    WinEDA_PcbFrame* frame = EDA_Appl->m_PcbFrame;
    MODULE*          module = 0;

    strncpy( line, cmdline, sizeof(line) - 1 );
    msg = CONV_FROM_UTF8( line );

    idcmd = strtok( line, " \n\r" );
    text  = strtok( NULL, " \n\r" );
    if( (idcmd == NULL) || (text == NULL) )
        return;

    if( strcmp( idcmd, "$PART:" ) == 0 )
    {
        msg    = CONV_FROM_UTF8( text );
        
        module = ReturnModule( frame->m_Pcb, msg );
        
        msg.Printf( _( "Locate module %s %s" ), msg.GetData(),
                   module ? wxT( "Ok" ) : wxT( "not found" ) );
        
        frame->Affiche_Message( msg );
        if( module )
        {
            wxClientDC dc( frame->DrawPanel );

            frame->DrawPanel->PrepareGraphicContext( &dc );
            frame->DrawPanel->CursorOff( &dc );
            frame->GetScreen()->m_Curseur = module->m_Pos;
            frame->DrawPanel->CursorOn( &dc );
        }
    }

    if( idcmd && strcmp( idcmd, "$PIN:" ) == 0 )
    {
        wxString pinName, modName;
        D_PAD*   pad = NULL;
        int      netcode = -1;
        
        pinName = CONV_FROM_UTF8( text );
        
        text = strtok( NULL, " \n\r" );
        if( text && strcmp( text, "$PART:" ) == 0 )
            text = strtok( NULL, "\n\r" );

        wxClientDC dc( frame->DrawPanel );

        frame->DrawPanel->PrepareGraphicContext( &dc );

        modName = CONV_FROM_UTF8( text );
        module  = ReturnModule( frame->m_Pcb, modName );
        if( module )
            pad = ReturnPad( module, pinName );
        
        if( pad )
            netcode = pad->m_NetCode;
        
        if( netcode > 0 )    /* hightlighted the net selected net*/
        {
            if( g_HightLigt_Status )        /* erase the old hightlighted net */
                frame->Hight_Light( &dc );
            
            g_HightLigth_NetCode = netcode;
            frame->Hight_Light( &dc );      /* hightlighted the new one */
            
            frame->DrawPanel->CursorOff( &dc );
            frame->GetScreen()->m_Curseur = pad->m_Pos;
            frame->DrawPanel->CursorOn( &dc );
        }

        if( module == NULL )
            msg.Printf( _( "module %s not found" ), text );
        else if( pad == NULL )
            msg.Printf( _( "Pin %s (module %s) not found" ), pinName.GetData(), modName.GetData() );
        else
            msg.Printf( _( "Locate Pin %s (module %s)" ), pinName.GetData(), modName.GetData() );
        frame->Affiche_Message( msg );
    }
        
    if( module )    // if found, center the module on screen.
        frame->Recadre_Trace( false );
}


// @todo: move this to proper source file.
wxString BOARD_ITEM::MenuText( const BOARD* aPcb ) const
{
    wxString            text;
    const BOARD_ITEM*   item = this;
    EQUIPOT*            net;

    switch( item->Type() )
    {
    case PCB_EQUIPOT_STRUCT_TYPE:
        text << _("Net") << ((EQUIPOT*)item)->m_Netname << wxT(" ") << ((EQUIPOT*)item)->m_NetCode;
        break;
        
    case TYPEMODULE:
        text << _("Footprint") << wxT(" ") << ((MODULE*)item)->GetReference();
		text  << wxT(" (") << ReturnPcbLayerName( item->m_Layer )  << wxT(")");
        break;
        
    case TYPEPAD:
        text << _("Pad") << wxT(" ") << ((D_PAD*)item)->ReturnStringPadName() << _(" of ") 
//            << GetParent()->MenuText( aPcb );
            << ((MODULE*)GetParent())->GetReference();
        break;
        
    case TYPEDRAWSEGMENT:
        text << _("Pcb Graphic") << _(" on ") << ReturnPcbLayerName( item->GetLayer() );  // @todo: extend text
        break;
        
    case TYPETEXTE:
        text << _("Pcb Text") << wxT(" ");;
        if( ((TEXTE_PCB*)item)->m_Text.Len() < 12 )
            text << ((TEXTE_PCB*)item)->m_Text;
        else
            text += ((TEXTE_PCB*)item)->m_Text.Left( 10 ) + wxT( ".." );
        break;
        
    case TYPETEXTEMODULE:
        switch( ((TEXTE_MODULE*)item)->m_Type )
        {
        case TEXT_is_REFERENCE:
            text << _( "Reference" ) << wxT( " " ) << ((TEXTE_MODULE*)item)->m_Text;
            break;
    
        case TEXT_is_VALUE:
            text << _( "Value" ) << wxT( " " ) << ((TEXTE_MODULE*)item)->m_Text << _(" of ") 
//                << GetParent()->MenuText( aPcb );
                << ((MODULE*)GetParent())->GetReference(); 
            break;
    
        default:    // wrap this one in quotes:
            text << _( "Text" ) << wxT( " \"" ) << ((TEXTE_MODULE*)item)->m_Text << wxT("\"") <<  _(" of ") 
//                << GetParent()->MenuText( aPcb );
                << ((MODULE*)GetParent())->GetReference();
            break;
        }
        break;
        
    case TYPEEDGEMODULE:
        text << _("Graphic") << wxT( " " );
        const wxChar* cp;
        switch( ((EDGE_MODULE*)item)->m_Shape )
        {
        case S_SEGMENT:     cp = _("Line");             break;
        case S_RECT:        cp = _("Rect");             break;
        case S_ARC:         cp = _("Arc");              break;
        case S_CIRCLE:      cp = _("Circle");           break;
        /* used in Gerbview: */            
        case S_ARC_RECT:    cp = wxT("arc_rect");       break;
        case S_SPOT_OVALE:  cp = wxT("spot_oval");      break;
        case S_SPOT_CIRCLE: cp = wxT("spot_circle");    break;
        case S_SPOT_RECT:   cp = wxT("spot_rect");      break;
        case S_POLYGON:     cp = wxT("polygon");        break;

        default:            cp = wxT("??EDGE??");       break;
        }
        text << *cp << _(" of ") 
//              << GetParent()->MenuText( aPcb );
                << ((MODULE*)GetParent())->GetReference();
        break;
        
    case TYPETRACK:
        text << _("Track") << wxT(" ") << ((TRACK*)item)->m_NetCode;
        net = aPcb->FindNet( ((TRACK*)item)->m_NetCode );
        if( net )
        {
            text << wxT( " [" ) << net->m_Netname << wxT("]");
        }
        text << _(" on ") << ReturnPcbLayerName( item->GetLayer() );
        break;
        
    case TYPEZONE:
        text << _("Zone") << _(" on ") << ReturnPcbLayerName( item->GetLayer() );
        break;
        
    case TYPEVIA:
        text << _("Via") << wxT(" ") << ((SEGVIA*)item)->m_NetCode;
        net = aPcb->FindNet( ((TRACK*)item)->m_NetCode );
        if( net )
        {
            text << wxT( " [" ) << net->m_Netname << wxT("]");
        }
        break;
        
    case TYPEMARQUEUR:
        text << _("Marker");
        break;
        
    case TYPECOTATION:
        text << _("Dimension");     // @todo: extend text
        break;
        
    case TYPEMIRE:
        text << _("Mire");          // @todo: extend text,  Mire is not an english word!
        break;
        
    case TYPEEDGEZONE:
        text << _("Edge Zone") << _(" on ") << ReturnPcbLayerName( item->GetLayer() );  // @todo: extend text
        break;
        
    default:
        text << item->GetClass() << wxT(" Unexpected item type: BUG!!");
        break;
    }

    return text;    
}


// @todo: move this to proper source file.
const char** BOARD_ITEM::MenuIcon() const
{
    char**              xpm;
    const BOARD_ITEM*   item = this;
    
    switch( item->Type() )
    {
    case PCB_EQUIPOT_STRUCT_TYPE:
        xpm = module_xpm;   // @todo: use net icon 
        break;
        
    case TYPEMODULE:
        xpm = module_xpm;
        break;
        
    case TYPEPAD:
        xpm = pad_xpm;
        break;
        
    case TYPEDRAWSEGMENT:
        xpm = module_xpm;   // @todo: use draw segment icon & expand on text
        break;
        
    case TYPETEXTE:
        xpm = add_text_xpm;
        break;
        
    case TYPETEXTEMODULE:
        xpm = footprint_text_xpm;
        break;
        
    case TYPEEDGEMODULE:
        xpm = show_mod_edge_xpm;
        break;
        
    case TYPETRACK:
        xpm = showtrack_xpm;
        break;
        
    case TYPEZONE:
        xpm = add_zone_xpm;
        break;
        
    case TYPEVIA:
        xpm = showtrack_xpm;        // @todo: use via specific xpm
        break;
        
    case TYPEMARQUEUR:
        xpm = pad_xpm;              // @todo: create and use marker xpm
        break;
        
    case TYPECOTATION:
        xpm = add_cotation_xpm;
        break;
        
    case TYPEMIRE:
        xpm = add_mires_xpm; 
        break;
        
    case TYPEEDGEZONE:
        xpm = show_mod_edge_xpm;    // @todo: pcb edge xpm
        break;
        
    default:
        xpm = 0;
        break;
    }
    
    return (const char**) xpm;
}


/**
 * Function AllAreModulesAndReturnSmallestIfSo
 * tests that all items in the collection are MODULEs and if so, returns the
 * smallest MODULE.
 * @return BOARD_ITEM* - The smallest or NULL.
 */ 
static BOARD_ITEM* AllAreModulesAndReturnSmallestIfSo( GENERAL_COLLECTOR* aCollector )
{
    int count = aCollector->GetCount();
    
    for( int i=0; i<count;  ++i )
    {
        if(  (*aCollector)[i]->Type() != TYPEMODULE )
            return NULL;
    }
    
    // all are modules, now find smallest MODULE

    int minDim = 0x7FFFFFFF;
    int minNdx = 0;
    
    for( int i=0;  i<count;  ++i )
    {
        MODULE* module = (MODULE*) (*aCollector)[i];
        
        int  lx = module->m_BoundaryBox.GetWidth();
        int  ly = module->m_BoundaryBox.GetHeight();
        
        int  lmin = MIN( lx, ly );
        
        if( lmin <= minDim )
        {
            minDim = lmin;
            minNdx = i;
        }
    }
    
    return (*aCollector)[minNdx];
}



/***********************************************************************/
BOARD_ITEM* WinEDA_BasePcbFrame::PcbGeneralLocateAndDisplay()
/***********************************************************************/
{
    BOARD_ITEM*     item;
    
    GENERAL_COLLECTORS_GUIDE  guide = GetCollectorsGuide();

    // Assign to scanList the proper item types desired based on tool type.
    // May need to pass a hot key code to this function to support hot keys too.
    
    const KICAD_T*  scanList;
    
    if( m_ID_current_state == 0 )
    {
        switch( m_HTOOL_current_state )
        {
        case ID_TOOLBARH_PCB_AUTOPLACE:
            scanList = GENERAL_COLLECTOR::ModuleItems;
            break;
            
        default:
            scanList = GENERAL_COLLECTOR::AllBoardItems;
            break;
        }
    }
    else
    {
        switch( m_ID_current_state )
        {
        case ID_PCB_SHOW_1_RATSNEST_BUTT:
            scanList = GENERAL_COLLECTOR::PadsOrModules;
            break;            

        case ID_TRACK_BUTT:
            scanList = GENERAL_COLLECTOR::Tracks;
            break;            

        case ID_COMPONENT_BUTT:
            scanList = GENERAL_COLLECTOR::ModuleItems;
            break;
            
        default:
            scanList = GENERAL_COLLECTOR::AllBoardItems;
        }
    }

    m_Collector->Collect( m_Pcb, scanList, GetScreen()->RefPos(true), guide );

    /* debugging: print out the collected items, showing their priority order too.
    for( unsigned i=0; i<m_Collector->GetCount();  ++i )
       (*m_Collector)[i]->Show( 0, std::cout ); 
    */

    if( m_Collector->GetCount() <= 1 )  
    {
        item = (*m_Collector)[0];
        SetCurItem( item );        
    }
    
    // If the count is 2, and first item is a pad or moduletext, and the 2nd item is its parent module:
    else if( m_Collector->GetCount() == 2 && 
        ( (*m_Collector)[0]->Type() == TYPEPAD || (*m_Collector)[0]->Type() == TYPETEXTEMODULE) && 
        (*m_Collector)[1]->Type() == TYPEMODULE && (*m_Collector)[0]->GetParent()==(*m_Collector)[1] )
    {
        item = (*m_Collector)[0];
        SetCurItem( item );        
    }
    
    // if all are modules, find the smallest one amoung the primary choices
    else if( (item = AllAreModulesAndReturnSmallestIfSo(m_Collector) ) != NULL )
    {
        SetCurItem( item );        
    }
    
    else    // show a popup menu
    {
        wxMenu  itemMenu;

        itemMenu.SetTitle( _("Selection Clarification") );  // does this work? not under Linux!
        
        int limit = MIN( MAX_ITEMS_IN_PICKER, m_Collector->GetCount() );

        for( int i=0;  i<limit;  ++i )
        {
            wxString        text;
            const char**    xpm;
            
            item = (*m_Collector)[i];

            text = item->MenuText( m_Pcb );
            xpm  = item->MenuIcon();
            
            ADD_MENUITEM( &itemMenu, ID_POPUP_PCB_ITEM_SELECTION_START+i, text, xpm );
        }
        
        DrawPanel->m_IgnoreMouseEvents = TRUE;

        // this menu's handler is void WinEDA_BasePcbFrame::ProcessItemSelection()
        // and it calls SetCurItem() which in turn calls Display_Infos() on the item.
        PopupMenu( &itemMenu );
        
        DrawPanel->MouseToCursorSchema();
        
        DrawPanel->m_IgnoreMouseEvents = FALSE;
        
        // The function ProcessItemSelection() has set the current item, return it.
        item = GetCurItem();
    }
    
    return item;
    
/* old way:
    
    item = Locate( CURSEUR_OFF_GRILLE, GetScreen()->m_Active_Layer );
    if( item == NULL )
        item = Locate( CURSEUR_OFF_GRILLE, -1 );
    return item;
*/    
}


/****************************************************************/
void WinEDA_BasePcbFrame::GeneralControle( wxDC* DC, wxPoint Mouse )
/*****************************************************************/
{
    wxSize  delta;
    int     zoom = GetScreen()->GetZoom();
    wxPoint curpos, oldpos;
    int     hotkey = 0;

    ActiveScreen = GetScreen();

    // Save the board after the time out :
    int CurrentTime = time( NULL );
    if( !GetScreen()->IsModify() || GetScreen()->IsSave() )
    {   
        /* If no change, reset the time out */
        g_SaveTime = CurrentTime;
    }

    if( (CurrentTime - g_SaveTime) > g_TimeOut )
    {
        wxString tmpFileName = GetScreen()->m_FileName;
        wxString filename    = g_SaveFileName + PcbExtBuffer;
        bool     flgmodify   = GetScreen()->IsModify();

        ( (WinEDA_PcbFrame*) this )->SavePcbFile( filename );

        if( flgmodify ) // Set the flags m_Modify cleared by SavePcbFile()
        {
            GetScreen()->SetModify();
            GetScreen()->SetSave(); // Set the flags m_FlagSave cleared by SetModify()
        }
        GetScreen()->m_FileName = tmpFileName;
        SetTitle( GetScreen()->m_FileName );
    }

    curpos = DrawPanel->CursorRealPosition( Mouse );
    oldpos = GetScreen()->m_Curseur;

    delta.x = (int) round( (double) GetScreen()->GetGrid().x / zoom );
    delta.y = (int) round( (double) GetScreen()->GetGrid().y / zoom );
    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case EDA_PANNING_UP_KEY:
        OnZoom( ID_ZOOM_PANNING_UP );
        curpos = m_CurrentScreen->m_Curseur;
        break;

    case EDA_PANNING_DOWN_KEY:
        OnZoom( ID_ZOOM_PANNING_DOWN );
        curpos = m_CurrentScreen->m_Curseur;
        break;

    case EDA_PANNING_LEFT_KEY:
        OnZoom( ID_ZOOM_PANNING_LEFT );
        curpos = m_CurrentScreen->m_Curseur;
        break;

    case EDA_PANNING_RIGHT_KEY:
        OnZoom( ID_ZOOM_PANNING_RIGHT );
        curpos = m_CurrentScreen->m_Curseur;
        break;

	case EDA_ZOOM_IN_FROM_MOUSE:
        OnZoom( ID_ZOOM_PLUS_KEY );
        oldpos = curpos = GetScreen()->m_Curseur;
        break;

	case EDA_ZOOM_OUT_FROM_MOUSE:
        OnZoom( ID_ZOOM_MOINS_KEY );
        oldpos = curpos = GetScreen()->m_Curseur;
        break;

	case EDA_ZOOM_CENTER_FROM_MOUSE:
        OnZoom( ID_ZOOM_CENTER_KEY );
        oldpos = curpos = GetScreen()->m_Curseur;
        break;

    case WXK_NUMPAD8:       /* Deplacement curseur vers le haut */
    case WXK_UP:
        Mouse.y -= delta.y;
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD2:       /* Deplacement curseur vers le bas */
    case WXK_DOWN:
        Mouse.y += delta.y;
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD4:       /* Deplacement curseur vers la gauche */
    case WXK_LEFT:
        Mouse.x -= delta.x;
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD6:      /* Deplacement curseur vers la droite */
    case WXK_RIGHT:
        Mouse.x += delta.x;
        DrawPanel->MouseTo( Mouse );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Put cursor in new position, according to the zoom keys (if any) */
    GetScreen()->m_Curseur = curpos;

    /* Put cursor on grid or a pad centre if requested
     * But if the tool DELETE is active the cursor is left off grid
     * this is better to reach items to delete off grid
     */
    D_PAD* pad;
    bool   keep_on_grid = TRUE;
    if( m_ID_current_state == ID_PCB_DELETE_ITEM_BUTT )
        keep_on_grid = FALSE;
    
    /* Cursor is left off grid if no block in progress and no moving object */
    if( GetScreen()->BlockLocate.m_State != STATE_NO_BLOCK )
        keep_on_grid = TRUE;
    
    EDA_BaseStruct* DrawStruct = GetScreen()->GetCurItem();
    if( DrawStruct && DrawStruct->m_Flags )
        keep_on_grid = TRUE;

    switch( g_MagneticPadOption )
    {
    case capture_cursor_in_track_tool:
    case capture_always:
        pad = Locate_Any_Pad( m_Pcb, CURSEUR_OFF_GRILLE, TRUE );
        if( (m_ID_current_state != ID_TRACK_BUTT )
           && (g_MagneticPadOption == capture_cursor_in_track_tool) )
            pad = NULL;
        if( keep_on_grid )
        {
            if( pad )       // Put cursor on the pad
                GetScreen()->m_Curseur = curpos = pad->m_Pos;
            else
                // Put cursor on grid
                PutOnGrid( &GetScreen()->m_Curseur );
        }
        break;

    case no_effect:
    default:

        // If we are not in delete function, put cursor on grid
        if( keep_on_grid )
            PutOnGrid( &GetScreen()->m_Curseur );
        break;
    }

    if( oldpos != GetScreen()->m_Curseur )
    {
        curpos = GetScreen()->m_Curseur;
        GetScreen()->m_Curseur = oldpos;
        DrawPanel->CursorOff( DC );

        GetScreen()->m_Curseur = curpos;
        DrawPanel->CursorOn( DC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
        }
    }

    if( hotkey )
    {
        OnHotKey( DC, hotkey, NULL );
    }

    if( GetScreen()->IsRefreshReq() )
    {
        RedrawActiveWindow( DC, TRUE );
    }

    SetToolbars();
    Affiche_Status_Box();    /* Affichage des coord curseur */
}
