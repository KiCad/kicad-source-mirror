/***
 * @file menubarmodedit.cpp
 * Module editor menu bar.
 ***/
#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"

#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"

/* Create the menubar for the module editor */
void WinEDA_ModuleEditFrame::ReCreateMenuBar()
{
    wxMenuBar*  menuBar = GetMenuBar();
    wxMenuItem* item;

    if( menuBar )
        return;

        menuBar = new wxMenuBar();

        /* File menu */
        wxMenu* fileMenu = new wxMenu;

        /* New module */
        item = new wxMenuItem( fileMenu,
                              ID_MODEDIT_NEW_MODULE,
                              _( "New Module" ),
                              _( "Create new module" ) );
        item->SetBitmap( new_footprint_xpm );
        fileMenu->Append( item );

        /* Open submenu */
        wxMenu* openSubmenu = new wxMenu;

        /* from File */
        item = new wxMenuItem( openSubmenu,
                              ID_MODEDIT_IMPORT_PART,
                              _( "from File (Import)" ),
                              _( "Import a footprint from an existing file" ) );
        item->SetBitmap( import_module_xpm );
        openSubmenu->Append( item );

        /* from Library */
        item = new wxMenuItem( openSubmenu,
                              ID_MODEDIT_LOAD_MODULE,
                              _( "Load from Library" ),
                              _( "Open a footprint module from a Library" ) );
        item->SetBitmap( module_xpm );
        openSubmenu->Append( item );

        /* from current Board */
        item = new wxMenuItem( openSubmenu,
                              ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                              _( "Load from current Board" ),
                              _( "Load a footprint module from the current loaded board" ) );
        item->SetBitmap( load_module_board_xpm );
        openSubmenu->Append( item );

        /* Append openSubmenu to fileMenu */
        ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, openSubmenu, -1,
                                            _( "&Load Module" ), _( "Load a footprint module" ), open_xpm );

        /* Save module */
        item = new wxMenuItem( fileMenu, ID_MODEDIT_SAVE_LIBMODULE,
                              _( "&Save Module in Current Lib" ),
                              _( "Save Module in working library" ) );
        item->SetBitmap( save_library_xpm );
        fileMenu->Append( item );

        item = new wxMenuItem( fileMenu,
                              ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                              _( "&Save Module in a New Lib" ),
                              _( "Create new library and save current module" ) );
        item->SetBitmap( new_library_xpm );
        fileMenu->Append( item );

        item = new wxMenuItem( fileMenu,
                              ID_MODEDIT_EXPORT_PART,
                              _( "&Export module" ),
                              _( "Save the current loaded module to a file on the harddisk" ) );
        item->SetBitmap( export_module_xpm );
        fileMenu->Append( item );

        /* Separator */
        fileMenu->AppendSeparator();

        /* Print */
        item = new wxMenuItem( fileMenu, wxID_PRINT, _( "&Print" ),
                              _( "Print the current module" ) );
        item->SetBitmap( plot_xpm );
        fileMenu->Append( item );

        /* Separator */
        fileMenu->AppendSeparator();

        /* Close editor */
        item = new wxMenuItem( fileMenu, wxID_EXIT, _( "Close" ),
                              _( "Close the footprint editor" ) );
        item->SetBitmap( exit_xpm );
        fileMenu->Append( item );


        /* Edit menu */
        wxMenu* editMenu = new wxMenu;

        /* Undo */
        item = new wxMenuItem( editMenu,
                              wxID_UNDO,
                              _( "Undo" ),
                              _( "Undo last edit" ) );
        item->SetBitmap( undo_xpm );
        editMenu->Append( item );

        /* Redo */
        item = new wxMenuItem( editMenu,
                              wxID_REDO,
                              _( "Redo" ),
                              _( "Redo the last undo action" ) );
        item->SetBitmap( redo_xpm );
        editMenu->Append( item );

        /* Delete items */
        item = new wxMenuItem( editMenu,
                              ID_MODEDIT_DELETE_ITEM_BUTT,
                              _( "Delete" ),
                              _( "Delete objects with the eraser" ) );
        item->SetBitmap( delete_body_xpm );
        editMenu->Append( item );

        /* Separator */
        editMenu->AppendSeparator();

        /* Properties */
        item = new wxMenuItem( editMenu,
                              ID_MODEDIT_EDIT_MODULE_PROPERTIES,
                              _( "Properties" ),
                              _( "Edit module properties" ) );
        item->SetBitmap( module_options_xpm );
        editMenu->Append( item );

        /* Dimensions submenu */
        wxMenu* dimensions_Submenu = new wxMenu;

        /* Sizes and Widths */
        item = new wxMenuItem( dimensions_Submenu,
                              ID_PCB_DRAWINGS_WIDTHS_SETUP,
                              _( "Sizes and Widths" ),
                              _( "Adjust width for texts and drawings" ) );
        item->SetBitmap( options_text_xpm );
        dimensions_Submenu->Append( item );


        /* Pad settings */
        item = new wxMenuItem( dimensions_Submenu,
                              ID_MODEDIT_PAD_SETTINGS,
                              _( "Pad settings" ),
                              _( "Edit the settings for new pads" ) );
        item->SetBitmap( options_pad_xpm );
        dimensions_Submenu->Append( item );

        /* User Grid Size */
        item = new wxMenuItem( dimensions_Submenu,
                              ID_PCB_USER_GRID_SETUP,
                              _( "User Grid Size" ),
                              _( "Adjust user grid" ) );
        item->SetBitmap( grid_xpm );
        dimensions_Submenu->Append( item );

        /* Append dimensions_Submenu to editMenu */
        ADD_MENUITEM_WITH_HELP_AND_SUBMENU( editMenu,
                                            dimensions_Submenu, -1,
                                            _( "&Dimensions" ),
                                            _( "Edit dimensions preferences" ),
                                            add_dimension_xpm );


        /* View menu */
        wxMenu* viewMenu = new wxMenu;

        /* Zoom In */
        item = new wxMenuItem( viewMenu,
                              ID_ZOOM_IN,
                              _( "Zoom In" ),
                              _( "Zoom in on the module" ) );
        item->SetBitmap( zoom_in_xpm );
        viewMenu->Append( item );

        /* Zoom Out */
        item = new wxMenuItem( viewMenu,
                              ID_ZOOM_OUT,
                              _( "Zoom Out" ),
                              _( "Zoom out on the module" ) );
        item->SetBitmap( zoom_out_xpm );
        viewMenu->Append( item );

        /* Fit on Screen */
        item = new wxMenuItem( viewMenu,
                              ID_ZOOM_PAGE,
                              _( "Fit on Screen" ),
                              _( "Zoom and fit the module in the window" ) );
        item->SetBitmap( zoom_auto_xpm );
        viewMenu->Append( item );

        /* Separator */
        viewMenu->AppendSeparator();

        /* Redraw */
        item = new wxMenuItem( viewMenu,
                              ID_ZOOM_REDRAW,
                              _( "Redraw" ),
                              _( "Redraw the window's viewport" ) );
        item->SetBitmap( zoom_redraw_xpm );
        viewMenu->Append( item );

        /* 3D Viewer */
        item = new wxMenuItem( viewMenu,
                              ID_MENU_PCB_SHOW_3D_FRAME,
                              _( "3D View" ),
                              _( "Show board in 3D viewer" ) );
        item->SetBitmap( show_3d_xpm );
        viewMenu->Append( item );


        /* Place menu */
        wxMenu* placeMenu = new wxMenu;

        /* Pad */
        item = new wxMenuItem( placeMenu,
                              ID_MODEDIT_ADD_PAD,
                              _( "Pad" ),
                              _( "Add Pads" ) );
        item->SetBitmap( pad_xpm );
        placeMenu->Append( item );

        /* Separator */
        placeMenu->AppendSeparator();

        /* Circle */
        item = new wxMenuItem( placeMenu,
                              ID_PCB_CIRCLE_BUTT,
                              _( "Circle" ),
                              _( "Add graphic circle" ) );
        item->SetBitmap( add_circle_xpm );
        placeMenu->Append( item );


        /* Line or Polygon */
        item = new wxMenuItem( placeMenu,
                              ID_PCB_ADD_LINE_BUTT,
                              _( "Line or Polygon" ),
                              _( "Add graphic line or polygon" ) );
        item->SetBitmap( add_polygon_xpm );
        placeMenu->Append( item );

        /* Arc */
        item = new wxMenuItem( placeMenu,
                              ID_PCB_ARC_BUTT,
                              _( "Arc" ),
                              _( "Add graphic arc" ) );
        item->SetBitmap( add_arc_xpm );
        placeMenu->Append( item );

        /* Text */
        item = new wxMenuItem( placeMenu,
                              ID_PCB_ADD_TEXT_BUTT,
                              _( "Text" ),
                              _( "Add graphic text" ) );
        item->SetBitmap( add_text_xpm );
        placeMenu->Append( item );

        /* Anchor */
        placeMenu->AppendSeparator();
        item = new wxMenuItem( placeMenu,
                              ID_MODEDIT_PLACE_ANCHOR,
                              _( "Anchor" ),
                              _( "Place the footprint module reference anchor" ) );
        item->SetBitmap( anchor_xpm );
        placeMenu->Append( item );


        /* Help menu */
        wxMenu* helpMenu = new wxMenu;

        /* Contents */
        item = new wxMenuItem( helpMenu,
                              ID_GENERAL_HELP,
                              _( "&Contents" ),
                              _( "Open the PCBNew pdf manual" ) );
        item->SetBitmap( online_help_xpm );
        helpMenu->Append( item );

        /* About PCBNew */
        item = new wxMenuItem( helpMenu,
                              ID_KICAD_ABOUT,
                              _( "&About PCBNew" ),
                              _( "About PCBNew PCB designer" ) );
        item->SetBitmap( info_xpm );
        helpMenu->Append( item );


        /* Append all the menu's to the menubar */
        menuBar->Append( fileMenu, _( "&File" ) );
        menuBar->Append( editMenu, _( "&Edit" ) );
        menuBar->Append( viewMenu, _( "&View" ) );
        menuBar->Append( placeMenu, _( "&Place" ) );
        menuBar->Append( helpMenu, _( "&Help" ) );

        /* Associate the menu bar with the frame */
        SetMenuBar( menuBar );
}
