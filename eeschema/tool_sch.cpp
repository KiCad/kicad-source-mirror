/*****************************************************/
/*	toolsch.cpp; vreate toolbars for schematic frame */
/*****************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "hotkeys.h"

#define BITMAP wxBitmap

#include "bitmaps.h" /* general bitmaps */

/* Specific bitmaps */
#include "cvpcb.xpm"
#include "Hierarchy_Nav.xpm"
#include "Hierarchy_cursor.xpm"
#include "library_browse.xpm"
#include "libedit.xpm"
#include "Lines90.xpm"
#include "Hidden_Pin.xpm"

#include "id.h"


/**************************************************************/
void WinEDA_SchematicFrame::ReCreateHToolbar()
/**************************************************************/

/* Create  the main Horizontal Toolbar for the schematic editor
 */
{
    if( m_HToolBar != NULL )
        return;

    wxString msg;
    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
    SetToolBar( m_HToolBar );

    // Set up toolbar
    m_HToolBar->AddTool( ID_NEW_PROJECT, wxEmptyString, BITMAP( new_xpm ),
                        _( "New schematic project" ) );

    m_HToolBar->AddTool( ID_LOAD_PROJECT, wxEmptyString, BITMAP( open_xpm ),
                        _( "Open schematic project" ) );

    m_HToolBar->AddTool( ID_SAVE_PROJECT, wxEmptyString, BITMAP( save_project_xpm ),
                        _( "Save schematic project" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_SHEET_SET, wxEmptyString, BITMAP( sheetset_xpm ),
                        _( "page settings (size, texts)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TO_LIBRARY, wxEmptyString, BITMAP( libedit_xpm ),
                        _( "go to library editor" ) );

    m_HToolBar->AddTool( ID_TO_LIBVIEW, wxEmptyString, BITMAP( library_browse_xpm ),
                        _( "go to library browse" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_HIERARCHY, wxEmptyString, BITMAP( hierarchy_nav_xpm ),
                        _( "Schematic Hierarchy Navigator" ) );

    m_HToolBar->AddSeparator();

    m_HToolBar->AddTool( wxID_CUT, wxEmptyString, BITMAP( cut_button ),
                        _( "Cut selected item" ) );

    m_HToolBar->AddTool( wxID_COPY, wxEmptyString, BITMAP( copy_button ),
                        _( "Copy selected item" ) );

    m_HToolBar->AddTool( wxID_PASTE, wxEmptyString, BITMAP( paste_xpm ),
                        _( "Paste" ) );

    m_HToolBar->AddSeparator();
	msg = AddHotkeyName( _( "Undo last edition" ), s_Schematic_Hokeys_Descr, HK_UNDO );
    m_HToolBar->AddTool( ID_SCHEMATIC_UNDO, wxEmptyString, BITMAP( undo_xpm ), msg );

	msg = AddHotkeyName( _( "Redo the last undo command" ), s_Schematic_Hokeys_Descr, HK_REDO );
    m_HToolBar->AddTool( ID_SCHEMATIC_REDO, wxEmptyString, BITMAP( redo_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GEN_PRINT, wxEmptyString, BITMAP( print_button ),
                        _( "Print schematic" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_TO_CVPCB, wxEmptyString, BITMAP( cvpcb_xpm ),
                        _( "Run Cvpcb" ) );

    m_HToolBar->AddTool( ID_TO_PCB, wxEmptyString, BITMAP( pcbnew_xpm ),
                        _( "Run Pcbnew" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "zoom +" ), s_Schematic_Hokeys_Descr, HK_ZOOM_IN );
    m_HToolBar->AddTool( ID_ZOOM_IN_BUTT, wxEmptyString, BITMAP( zoom_in_xpm ),
                         msg );

    msg = AddHotkeyName( _( "zoom -" ), s_Schematic_Hokeys_Descr, HK_ZOOM_OUT );
    m_HToolBar->AddTool( ID_ZOOM_OUT_BUTT, wxEmptyString, BITMAP( zoom_out_xpm ),
                         msg );

    msg = AddHotkeyName( _( "redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW_BUTT, wxEmptyString, BITMAP( repaint_xpm ),
                         msg );

    m_HToolBar->AddTool( ID_ZOOM_PAGE_BUTT, wxEmptyString, BITMAP( zoom_optimal_xpm ),
                        _( "auto zoom" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_FIND_ITEMS, wxEmptyString, BITMAP( find_xpm ),
                        _( "Find components and texts" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GET_NETLIST, wxEmptyString, BITMAP( netlist_xpm ),
                        _( "Netlist generation" ) );

    m_HToolBar->AddTool( ID_GET_ANNOTATE, wxEmptyString, BITMAP( annotate_xpm ),
                        _( "Schematic Annotation" ) );

    m_HToolBar->AddTool( ID_GET_ERC, wxEmptyString, BITMAP( erc_xpm ),
                        _( "Schematic Electric Rules Check" ) );

    m_HToolBar->AddTool( ID_GET_TOOLS, wxEmptyString, BITMAP( tools_xpm ),
                        _( "Bill of material and/or Crossreferences" ) );


    // after adding the tools to the toolbar, must call Realize() to reflect the changes
    m_HToolBar->Realize();
    SetToolbars();
}


/*************************************************/
void WinEDA_SchematicFrame::ReCreateVToolbar()
/*************************************************/

/* Create Vertical Right Toolbar
 */
{
    if( m_VToolBar )
        return;
    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT, wxEmptyString,
                         BITMAP( cursor_xpm ), wxEmptyString, wxITEM_CHECK );
    m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );

    m_VToolBar->AddTool( ID_HIERARCHY_PUSH_POP_BUTT, wxEmptyString,
                         BITMAP( hierarchy_cursor_xpm ),
                         _( "Hierarchy Push/Pop" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_COMPONENT_BUTT, wxEmptyString,
                         BITMAP( add_component_xpm ),
                         _( "Place the component" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PLACE_POWER_BUTT, wxEmptyString,
                         BITMAP( add_power_xpm ),
                         _( "Place the power port" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_WIRE_BUTT, wxEmptyString,
                         BITMAP( add_line_xpm ),
                         _( "Place the wire" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_BUS_BUTT, wxEmptyString,
                         BITMAP( add_bus_xpm ),
                         _( "Place the bus" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_WIRETOBUS_ENTRY_BUTT, wxEmptyString,
                         BITMAP( add_line2bus_xpm ),
                         _( "Place the wire to bus entry" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_BUSTOBUS_ENTRY_BUTT, wxEmptyString,
                         BITMAP( add_bus2bus_xpm ),
                         _( "Place the bus to bus entry" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_NOCONN_BUTT, wxEmptyString,
                         BITMAP( noconn_button ),
                         _( "Place the no connect flag" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_LABEL_BUTT, wxEmptyString,
                         BITMAP( add_line_label_xpm ),
                         _( "Place the net name" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_GLABEL_BUTT, wxEmptyString,
                         BITMAP( add_glabel_xpm ),
                         _( "Place the global label.\nWarning: all global labels with the same name are connected in whole hierarchy" ),
						 wxITEM_CHECK );
	
    m_VToolBar->AddTool( ID_JUNCTION_BUTT, wxEmptyString,
                         BITMAP( add_junction_xpm ),
                         _( "Place the junction" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
	m_VToolBar->AddTool( ID_HIERLABEL_BUTT, wxEmptyString,
						 BITMAP( add_hierarchical_label_xpm ),
								 _( "Place the hierarchical label. This label will be seen as a pin sheet in the sheet symbol" ),
								 wxITEM_CHECK );

    m_VToolBar->AddTool( ID_SHEET_SYMBOL_BUTT, wxEmptyString,
                         BITMAP( add_hierarchical_subsheet_xpm ),
                         _( "Place the hierarchical sheet" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_IMPORT_GLABEL_BUTT, wxEmptyString,
                         BITMAP( import_hierarchical_label_xpm ),
                         _( "Place the pin sheet (imported hierarchical label from sheet)" ),
						 wxITEM_CHECK );

    m_VToolBar->AddTool( ID_SHEET_LABEL_BUTT, wxEmptyString,
                         BITMAP( add_hierar_pin_xpm ),
                         _( "Place the hierachical pin to sheet" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_LINE_COMMENT_BUTT, wxEmptyString,
                         BITMAP( add_dashed_line_xpm ),
                         _( "Place the graphic line or polygon" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_TEXT_COMMENT_BUTT, wxEmptyString,
                         BITMAP( add_text_xpm ),
                         _( "Place the graphic text (comment)" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_SCHEMATIC_DELETE_ITEM_BUTT, wxEmptyString,
                         BITMAP( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK );

    m_VToolBar->Realize();
    SetToolbars();
}


/****************************************************************/
void WinEDA_SchematicFrame::ReCreateOptToolbar()
/****************************************************************/

/* Create Vertical Left Toolbar (Option Toolbar)
 */
{
    if( m_OptionsToolBar )
        return;

    // creation du tool bar options
    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               BITMAP( grid_xpm ),
                               _( "Display Grid OFF" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               BITMAP( unit_inch_xpm ),
                               _( "Units = Inch" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               BITMAP( unit_mm_xpm ),
                               _( "Units = mm" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               BITMAP( cursor_shape_xpm ),
                               _( "Change Cursor Shape" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_HIDDEN_PINS, wxEmptyString,
                               BITMAP( hidden_pin_xpm ),
                               _( "Show Hidden Pins" ), wxITEM_CHECK );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_BUS_WIRES_ORIENT, wxEmptyString,
                               BITMAP( lines90_xpm ),
                               _( "HV orientation for Wires and Bus" ), wxITEM_CHECK );

    m_OptionsToolBar->Realize();

    SetToolbars();
}


/*******************************************************************************************/
void WinEDA_SchematicFrame::OnSelectOptionToolbar( wxCommandEvent& event )
/*******************************************************************************************/
{
    if( DrawPanel == NULL )
        return;

    int        id = event.GetId();
    wxClientDC dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_GRID:
        g_ShowGrid = m_Draw_Grid = m_OptionsToolBar->GetToolState( id );
        ReDrawPanel();
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_MM:
        g_UnitMetric = MILLIMETRE;
        Affiche_Status_Box();        /* Reaffichage des coord curseur */
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_INCH:
        g_UnitMetric = INCHES;
        Affiche_Status_Box();        /* Reaffichage des coord curseur */
        break;

    case ID_TB_OPTIONS_SELECT_CURSOR:
        DrawPanel->CursorOff( &dc );
        g_CursorShape = m_OptionsToolBar->GetToolState( id );
        DrawPanel->CursorOn( &dc );
        break;

    case ID_TB_OPTIONS_HIDDEN_PINS:
        g_ShowAllPins = m_OptionsToolBar->GetToolState( id );
        DrawPanel->ReDraw( &dc, TRUE );
        break;

    case ID_TB_OPTIONS_BUS_WIRES_ORIENT:
        g_HVLines = m_OptionsToolBar->GetToolState( id );
        break;

    default:
        DisplayError( this, wxT( "OnSelectOptionToolbar() error" ) );
        break;
    }

    SetToolbars();
}
