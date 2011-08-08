/***********************************************/
/* tool_modeit.cpp: footprint editor toolbars. */
/***********************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "dialog_helpers.h"
#include "bitmaps.h"
#include "pcbnew_id.h"
#include "hotkeys.h"

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


/* Create the main horizontal toolbar for the footprint editor */
void WinEDA_ModuleEditFrame::ReCreateHToolbar()
{
    if( m_HToolBar  != NULL )
        return;

    wxString msg;

    m_HToolBar = new EDA_TOOLBAR( TOOLBAR_MAIN, this, ID_H_TOOLBAR, true );

    // Set up toolbar
    m_HToolBar->AddTool( ID_MODEDIT_SELECT_CURRENT_LIB, wxEmptyString,
                         wxBitmap( open_library_xpm ),
                         _( "Select active library" ) );

    m_HToolBar->AddTool( ID_MODEDIT_SAVE_LIBMODULE, wxEmptyString, wxBitmap( save_library_xpm ),
                         _( "Save module in active library" ) );

    m_HToolBar->AddTool( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, wxEmptyString,
                         wxBitmap( new_library_xpm ),
                         _( "Create new library and save current module" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_DELETE_PART, wxEmptyString, wxBitmap( delete_xpm ),
                         _( "Delete part from active library" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_NEW_MODULE, wxEmptyString, wxBitmap( new_footprint_xpm ),
                         _( "New module" ) );

    m_HToolBar->AddTool( ID_MODEDIT_LOAD_MODULE, wxEmptyString, wxBitmap( module_xpm ),
                         _( "Load module from library" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, wxEmptyString,
                         wxBitmap( load_module_board_xpm ),
                         _( "Load module from current board" ) );

    m_HToolBar->AddTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, wxEmptyString,
                         wxBitmap( update_module_board_xpm ),
                         _( "Update module in current board" ) );

    m_HToolBar->AddTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, wxEmptyString,
                         wxBitmap( insert_module_board_xpm ),
                         _( "Insert module into current board" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_IMPORT_PART, wxEmptyString, wxBitmap( import_module_xpm ),
                         _( "Import module" ) );

    m_HToolBar->AddTool( ID_MODEDIT_EXPORT_PART, wxEmptyString, wxBitmap( export_module_xpm ),
                         _( "Export module" ) );


    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_UNDO, wxEmptyString, wxBitmap( undo_xpm ),
                         _( "Undo last edition" ) );
    m_HToolBar->AddTool( wxID_REDO, wxEmptyString, wxBitmap( redo_xpm ),
                         _( "Redo the last undo command" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_EDIT_MODULE_PROPERTIES, wxEmptyString,
                         wxBitmap( module_options_xpm ),
                         _( "Module properties" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( wxID_PRINT, wxEmptyString, wxBitmap( print_button ),
                         _( "Print module" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_IN, false );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, wxBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_OUT, false );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, wxBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( _( "Redraw view" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_REDRAW, false );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, wxBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), g_Module_Editor_Hokeys_Descr, HK_ZOOM_AUTO, false );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, wxBitmap( zoom_fit_in_page_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_PAD_SETTINGS, wxEmptyString, wxBitmap( options_pad_xpm ),
                         _( "Pad settings" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_CHECK, wxEmptyString,
                         wxBitmap( module_check_xpm ),
                         _( "Check module" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


void WinEDA_ModuleEditFrame::ReCreateVToolbar()
{
    if( m_VToolBar )
        return;

    m_VToolBar = new EDA_TOOLBAR( TOOLBAR_TOOL, this, ID_V_TOOLBAR, false );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, wxBitmap( cursor_xpm ),
                         wxEmptyString, wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_PAD_TOOL, wxEmptyString, wxBitmap( pad_xpm ),
                         _( "Add pads" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_LINE_TOOL, wxEmptyString, wxBitmap( add_polygon_xpm ),
                         _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_MODEDIT_CIRCLE_TOOL, wxEmptyString, wxBitmap( add_circle_xpm ),
                         _( "Add graphic circle" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_MODEDIT_ARC_TOOL, wxEmptyString, wxBitmap( add_arc_xpm ),
                         _( "Add graphic arc" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_MODEDIT_TEXT_TOOL, wxEmptyString, wxBitmap( add_text_xpm ),
                         _( "Add Text" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_ANCHOR_TOOL, wxEmptyString, wxBitmap( anchor_xpm ),
                         _( "Place the footprint module reference anchor" ),
                         wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_DELETE_TOOL, wxEmptyString, wxBitmap( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_MODEDIT_PLACE_GRID_COORD, wxEmptyString,
                         wxBitmap( grid_select_axis_xpm ),
                         _( "Set the origin point for the grid" ),
                         wxITEM_CHECK );

    m_VToolBar->Realize();
}


void WinEDA_ModuleEditFrame::ReCreateOptToolbar()
{
    if( m_OptionsToolBar )
        return;

    // Create options tool bar.
    m_OptionsToolBar = new EDA_TOOLBAR( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, false );
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, wxBitmap( grid_xpm ),
                               _( "Hide grid" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               wxBitmap( polar_coord_xpm ),
                               _( "Display Polar Coord ON" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               wxBitmap( unit_inch_xpm ),
                               _( "Units in inches" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               wxBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               wxBitmap( cursor_shape_xpm ),
                               _( "Change Cursor Shape" ), wxITEM_CHECK  );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               wxBitmap( pad_sketch_xpm ),
                               _( "Show Pads Sketch" ), wxITEM_CHECK  );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, wxEmptyString,
                               wxBitmap( text_sketch_xpm ),
                               _( "Show Texts Sketch" ), wxITEM_CHECK  );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, wxEmptyString,
                               wxBitmap( show_mod_edge_xpm ),
                               _( "Show Edges Sketch" ), wxITEM_CHECK  );

    m_OptionsToolBar->Realize();
}


void WinEDA_ModuleEditFrame::ReCreateAuxiliaryToolbar()
{
    wxString msg;

    if( m_AuxiliaryToolBar )
        return;

    m_AuxiliaryToolBar = new EDA_TOOLBAR( TOOLBAR_AUX, this, ID_AUX_TOOLBAR, true );

    // Set up toolbar
    m_AuxiliaryToolBar->AddSeparator();

    // Grid selection choice box.
    m_SelGridBox = new wxComboBox( m_AuxiliaryToolBar,
                                   ID_ON_GRID_SELECT,
                                   wxEmptyString,
                                   wxPoint( -1, -1 ),
                                   wxSize( LISTBOX_WIDTH, -1 ),
                                   0, NULL, wxCB_READONLY );
    m_AuxiliaryToolBar->AddControl( m_SelGridBox );

    // Zoom selection choice box.
    m_AuxiliaryToolBar->AddSeparator();
    m_SelZoomBox = new wxComboBox( m_AuxiliaryToolBar,
                                   ID_ON_ZOOM_SELECT,
                                   wxEmptyString,
                                   wxPoint( -1, -1 ),
                                   wxSize( LISTBOX_WIDTH, -1 ),
                                   0, NULL, wxCB_READONLY );
    m_AuxiliaryToolBar->AddControl( m_SelZoomBox );

    // Update tool bar to reflect setting.
    updateGridSelectBox();
    updateZoomSelectBox();

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_AuxiliaryToolBar->Realize();
}
