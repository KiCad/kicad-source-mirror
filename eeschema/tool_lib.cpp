/******************/
/*  tool_lib.cpp  */
/******************/

#include "fctsys.h"
#include "common.h"
#include "hotkeys.h"
#include "bitmaps.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


extern int ExportPartId;
extern int ImportPartId;
extern int CreateNewLibAndSavePartId;


void WinEDA_LibeditFrame::ReCreateVToolbar()
{
    if( m_VToolBar != NULL )
        return;

    m_VToolBar = new WinEDA_Toolbar( TOOLBAR_TOOL, this, ID_V_TOOLBAR, false );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_SELECT_BUTT, wxEmptyString,
                         wxBitmap( cursor_xpm ),
                         _( "Deselect current tool" ), wxITEM_CHECK );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_LIBEDIT_PIN_BUTT, wxEmptyString,
                         wxBitmap( pin_xpm ),
                         _( "Add pin" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_TEXT_BUTT, wxEmptyString,
                         wxBitmap( add_text_xpm ),
                         _( "Add graphic text" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_RECT_BUTT, wxEmptyString,
                         wxBitmap( add_rectangle_xpm ),
                         _( "Add rectangle" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_CIRCLE_BUTT, wxEmptyString,
                         wxBitmap( add_circle_xpm ),
                         _( "Add circle" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_ARC_BUTT, wxEmptyString,
                         wxBitmap( add_arc_xpm ),
                         _( "Add arc" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_LINE_BUTT, wxEmptyString,
                         wxBitmap( add_polygon_xpm ),
                         _( "Add lines and polygons" ), wxITEM_CHECK  );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_LIBEDIT_ANCHOR_ITEM_BUTT, wxEmptyString,
                         wxBitmap( anchor_xpm ),
                         _( "Move part anchor" ), wxITEM_CHECK  );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_LIBEDIT_IMPORT_BODY_BUTT, wxEmptyString,
                         wxBitmap( import_xpm ),
                         _( "Import existing drawings" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_EXPORT_BODY_BUTT, wxEmptyString,
                         wxBitmap( export_xpm ),
                         _( "Export current drawing" ), wxITEM_CHECK  );

    m_VToolBar->AddSeparator();
    m_VToolBar->AddTool( ID_LIBEDIT_DELETE_ITEM_BUTT, wxEmptyString,
                         wxBitmap( delete_body_xpm ),
                         _( "Delete items" ), wxITEM_CHECK  );

    m_VToolBar->Realize();
}


void WinEDA_LibeditFrame::ReCreateHToolbar()
{
    wxString msg;

    // Create the toolbar if not exists
    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, true );
#if !defined(KICAD_AUIMANAGER)
    SetToolBar( (wxToolBar*)m_HToolBar );
#endif
    // Set up toolbar
    m_HToolBar->AddTool( ID_LIBEDIT_SAVE_CURRENT_LIB, wxEmptyString,
                         wxBitmap( save_library_xpm ),
                         _( "Save current library to disk" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_SELECT_CURRENT_LIB, wxEmptyString,
                         wxBitmap( library_xpm ),
                         _( "Select working library" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_DELETE_PART, wxEmptyString,
                         wxBitmap( delete_xpm ),
                         _( "Delete component in current library" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_NEW_PART, wxEmptyString,
                         wxBitmap( new_component_xpm ),
                         _( "New component" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_SELECT_PART, wxEmptyString,
                         wxBitmap( add_component_xpm ),
                         _( "Select component to edit" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_SAVE_CURRENT_PART, wxEmptyString,
                         wxBitmap( save_part_in_mem_xpm ),
                         _( "Update current component in current library" ) );

    m_HToolBar->AddTool( ImportPartId, wxEmptyString, wxBitmap( import_xpm ),
                         _( "Import component" ) );

    m_HToolBar->AddTool( ExportPartId, wxEmptyString, wxBitmap( export_xpm ),
                         _( "Export component" ) );

    m_HToolBar->AddTool( CreateNewLibAndSavePartId, wxEmptyString,
                         wxBitmap( new_library_xpm ),
                         _( "Save current component to new library" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Undo last command" ), s_Schematic_Hokeys_Descr,
                         HK_UNDO );
    m_HToolBar->AddTool( wxID_UNDO, wxEmptyString, wxBitmap( undo_xpm ),
                         msg );
    msg = AddHotkeyName( _( "Redo the last command" ), s_Schematic_Hokeys_Descr,
                         HK_REDO );
    m_HToolBar->AddTool( wxID_REDO, wxEmptyString, wxBitmap( redo_xpm ),
                         msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_PART, wxEmptyString,
                         wxBitmap( part_properties_xpm ),
                         _( "Edit component properties" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, wxEmptyString,
                         wxBitmap( add_text_xpm ),
                         _( "Add and remove fields and edit field properties" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_CHECK_PART, wxEmptyString,
                         wxBitmap( erc_xpm ),
                         _( "Test for duplicate pins and off grid pins" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), s_Libedit_Hokeys_Descr, HK_ZOOM_IN );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, wxBitmap( zoom_in_xpm ),
                         msg );

    msg = AddHotkeyName( _( "Zoom out" ), s_Libedit_Hokeys_Descr, HK_ZOOM_OUT );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, wxBitmap( zoom_out_xpm ),
                         msg );

    msg = AddHotkeyName( _( "Redraw view" ), s_Libedit_Hokeys_Descr,
                         HK_ZOOM_REDRAW );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), s_Libedit_Hokeys_Descr, HK_ZOOM_AUTO );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                         wxBitmap( zoom_auto_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
                         wxBitmap( morgan1_xpm ),
                         _( "Show as \"De Morgan\" normal part" ),
                         wxITEM_CHECK );
    m_HToolBar->AddTool( ID_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
                         wxBitmap( morgan2_xpm ),
                         _( "Show as \"De Morgan\" convert part" ),
                         wxITEM_CHECK );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_VIEW_DOC, wxEmptyString,
                         wxBitmap( datasheet_xpm ), _( "Edit document file" ) );

    m_HToolBar->AddSeparator();
    m_SelpartBox = new WinEDAChoiceBox( m_HToolBar,
                                        ID_LIBEDIT_SELECT_PART_NUMBER,
                                        wxDefaultPosition,
                                        wxSize( LISTBOX_WIDTH, -1 ) );
    m_HToolBar->AddControl( m_SelpartBox );

    m_SelAliasBox = new WinEDAChoiceBox( m_HToolBar, ID_LIBEDIT_SELECT_ALIAS,
                                         wxDefaultPosition,
                                         wxSize( LISTBOX_WIDTH, -1 ) );
    m_HToolBar->AddControl( m_SelAliasBox );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, wxEmptyString,
                         wxBitmap( pin2pin_xpm ),
                         _( "Edit pins part per part ( Use carefully!)" ),
                         wxITEM_CHECK );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}
