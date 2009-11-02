/*****************************************************************/
/* tool_modeit.cpp: construction du menu de l'editeur de modules */
/*****************************************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "protos.h"

#include "bitmaps.h"

#include "pcbnew_id.h"

#include "hotkeys.h"

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif

/***************************************************/
void WinEDA_ModuleEditFrame::ReCreateHToolbar()
/***************************************************/
/* Create the main horizontal toolbar for the footprint editor */
{
    if( m_HToolBar  != NULL )
        return;

    wxString msg;

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
#if !KICAD_AUIMANAGER
    SetToolBar( (wxToolBar*) m_HToolBar );
#endif
    // Set up toolbar
    m_HToolBar->AddTool( ID_MODEDIT_SELECT_CURRENT_LIB, wxEmptyString,
                         wxBitmap( open_library_xpm ),
                         _( "Select working library" ) );

    m_HToolBar->AddTool( ID_MODEDIT_SAVE_LIBMODULE, wxEmptyString,
                         wxBitmap( save_library_xpm ),
                         _( "Save Module in working library" ) );

    m_HToolBar->AddTool( ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                         wxEmptyString,
                         wxBitmap( new_library_xpm ),
                         _( "Create new library and save current module" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_DELETE_PART, wxEmptyString,
                         wxBitmap( delete_xpm ),
                         _( "Delete part in current library" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_NEW_MODULE, wxEmptyString,
                         wxBitmap( new_footprint_xpm ),
                         _( "New Module" ) );

    m_HToolBar->AddTool( ID_MODEDIT_LOAD_MODULE, wxEmptyString,
                         wxBitmap( module_xpm ),
                         _( "Load module from lib" ) );

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
    m_HToolBar->AddTool( ID_MODEDIT_IMPORT_PART, wxEmptyString,
                         wxBitmap( import_module_xpm ),
                         _( "import module" ) );

    m_HToolBar->AddTool( ID_MODEDIT_EXPORT_PART, wxEmptyString,
                         wxBitmap( export_module_xpm ),
                         _( "export module" ) );


    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_UNDO, wxEmptyString, wxBitmap( undo_xpm ),
                         _( "Undo last edition" ) );
    m_HToolBar->AddTool( ID_MODEDIT_REDO, wxEmptyString, wxBitmap( redo_xpm ),
                         _( "Redo the last undo command" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_EDIT_MODULE_PROPERTIES, wxEmptyString,
                         wxBitmap( module_options_xpm ),
                         _( "Module Properties" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_GEN_PRINT, wxEmptyString, wxBitmap( print_button ),
                         _( "Print Module" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), s_Module_Editor_Hokeys_Descr,
                         HK_ZOOM_IN );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                         wxBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), s_Module_Editor_Hokeys_Descr,
                         HK_ZOOM_OUT );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                         wxBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( _( "Redraw view" ), s_Module_Editor_Hokeys_Descr,
                         HK_ZOOM_REDRAW );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), s_Module_Editor_Hokeys_Descr,
                         HK_ZOOM_AUTO );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                         wxBitmap( zoom_auto_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_PAD_SETTINGS, wxEmptyString,
                         wxBitmap( options_pad_xpm ),
                         _( "Pad Settings" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MODEDIT_CHECK, wxEmptyString,
                         wxBitmap( module_check_xpm ),
                         _( "Module Check" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
    SetToolbars();
}


/********************************************************/
void WinEDA_ModuleEditFrame::ReCreateVToolbar()
/********************************************************/
{
    if( m_VToolBar )
        return;

    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, FALSE );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT, wxEmptyString,
                         wxBitmap( cursor_xpm ), wxEmptyString, wxITEM_CHECK );
    m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_ADD_PAD, wxEmptyString,
                         wxBitmap( pad_xpm ),
                         _( "Add Pads" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_PCB_ADD_LINE_BUTT, wxEmptyString,
                         wxBitmap( add_polygon_xpm ),
                         _( "Add graphic line or polygon" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_CIRCLE_BUTT, wxEmptyString,
                         wxBitmap( add_circle_xpm ),
                         _( "Add graphic circle" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ARC_BUTT, wxEmptyString,
                         wxBitmap( add_arc_xpm ),
                         _( "Add graphic arc" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_PCB_ADD_TEXT_BUTT, wxEmptyString,
                         wxBitmap( add_text_xpm ),
                         _( "Add Text" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_PLACE_ANCHOR, wxEmptyString,
                         wxBitmap( anchor_xpm ),
                         _( "Place anchor" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_MODEDIT_DELETE_ITEM_BUTT, wxEmptyString,
                         wxBitmap( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK );

    m_VToolBar->Realize();

    SetToolbars();
}


/*********************************************************/
void WinEDA_ModuleEditFrame::ReCreateOptToolbar()
/*********************************************************/
{
    if( m_OptionsToolBar )
        return;

    // creation du tool bar options
    m_OptionsToolBar = new WinEDA_Toolbar( TOOLBAR_OPTION, this,
                                           ID_OPT_TOOLBAR, FALSE );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                                wxBitmap( grid_xpm ),
                               _( "Display Grid OFF" ), wxITEM_CHECK );
    m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID, m_Draw_Grid );

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
                               _( "Change Cursor Shape" ) );

    m_OptionsToolBar->AddSeparator();
    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               wxBitmap( pad_sketch_xpm ),
                               _( "Show Pads Sketch" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
                               wxEmptyString,
                               wxBitmap( text_sketch_xpm ),
                               _( "Show Texts Sketch" ) );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
                               wxEmptyString,
                               wxBitmap( show_mod_edge_xpm ),
                               _( "Show Edges Sketch" ) );

    m_OptionsToolBar->Realize();

    SetToolbars();
}


/*********************************************************/
void WinEDA_ModuleEditFrame::ReCreateAuxiliaryToolbar()
/*********************************************************/
{
    size_t   i;
    wxString msg;

    if( m_AuxiliaryToolBar == NULL )
    {
        m_AuxiliaryToolBar = new WinEDA_Toolbar( TOOLBAR_AUX, this,
                                                 ID_AUX_TOOLBAR, TRUE );

        // Set up toolbar
        m_AuxiliaryToolBar->AddSeparator();

        // Boite de selection du pas de grille
        m_SelGridBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                            ID_ON_GRID_SELECT,
                                            wxPoint( -1, -1 ),
                                            wxSize( LISTBOX_WIDTH, -1 ) );
        m_AuxiliaryToolBar->AddControl( m_SelGridBox );

        // Boite de selection du Zoom
        m_AuxiliaryToolBar->AddSeparator();
        m_SelZoomBox = new WinEDAChoiceBox( m_AuxiliaryToolBar,
                                            ID_ON_ZOOM_SELECT,
                                            wxPoint( -1, -1 ),
                                            wxSize( LISTBOX_WIDTH, -1 ) );
        msg = _( "Auto" );
        m_SelZoomBox->Append( msg );
        for( int i = 0; i < (int)GetScreen()->m_ZoomList.GetCount(); i++ )
        {
            msg = _( "Zoom " );
            if ( GetScreen()->m_ZoomList[i] % GetScreen()->m_ZoomScalar == 0 )
                msg << GetScreen()->m_ZoomList[i] / GetScreen()->m_ZoomScalar;
            else
            {
                wxString value;
                value.Printf( wxT( "%.1f" ),
                              (float)GetScreen()->m_ZoomList[i] /
                              GetScreen()->m_ZoomScalar );
                msg += value;
            }
            m_SelZoomBox->Append( msg );
        }

        m_AuxiliaryToolBar->AddControl( m_SelZoomBox );

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_AuxiliaryToolBar->Realize();
    }

    // mise a jour des affichages
    m_SelGridBox->Clear();
    for( i = 0; i < GetScreen()->m_GridList.GetCount(); i++ )
    {
        double value = To_User_Unit( g_UnitMetric,
                                     GetScreen()->m_GridList[i].m_Size.x,
                                     PCB_INTERNAL_UNIT );
        if( GetScreen()->m_GridList[i].m_Id != ID_POPUP_GRID_USER )
        {
            if( g_UnitMetric == INCHES )
                msg.Printf( _( "Grid %.1f" ), value * 1000 );
            else
                msg.Printf( _( "Grid %.3f" ), value );
        }
        else
        {
            msg = _( "User Grid" );
        }

        m_SelGridBox->Append( msg, (void*) &GetScreen()->m_GridList[i].m_Id );

        if( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId ==
            GetScreen()->m_GridList[i].m_Id )
            m_SelGridBox->SetSelection( i );
    }

    SetToolbars();
}
