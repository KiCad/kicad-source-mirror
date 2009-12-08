/****************************************************************/
/* tool_viewlib.cpp: Build the toolbars for the library browser */
/****************************************************************/

#include "fctsys.h"
#include "common.h"
#include "bitmaps.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "hotkeys.h"
#include "class_library.h"
#include "libviewfrm.h"


void WinEDA_ViewlibFrame::ReCreateHToolbar()
{
    int  ii; wxString msg;
    CMP_LIBRARY* lib;
    LIB_COMPONENT* component = NULL;
    CMP_LIB_ENTRY* entry = NULL;
    bool asdeMorgan = false;

    if( m_HToolBar  == NULL )
    {
        m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR,
                                         true );

#if !defined(KICAD_AUIMANAGER)
        SetToolBar( (wxToolBar*)m_HToolBar );
#endif

        // Set up toolbar
        m_HToolBar->AddTool( ID_LIBVIEW_SELECT_LIB, wxEmptyString,
                             wxBitmap( library_xpm ),
                             _( "Select library to browse" ) );

        m_HToolBar->AddTool( ID_LIBVIEW_SELECT_PART, wxEmptyString,
                             wxBitmap( add_component_xpm ),
                             _( "Select part to browse" ) );

        m_HToolBar->AddSeparator();
        m_HToolBar->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
                             wxBitmap( lib_previous_xpm ),
                             _( "Display previous part" ) );

        m_HToolBar->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
                             wxBitmap( lib_next_xpm ),
                             _( "Display next part" ) );

        m_HToolBar->AddSeparator();
        msg = AddHotkeyName( _( "Zoom in" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_IN );
        m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                             wxBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_OUT );
        m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                             wxBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_REDRAW );
        m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                             wxBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_AUTO );
        m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                             wxBitmap( zoom_auto_xpm ), msg );

        m_HToolBar->AddSeparator();
        m_HToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
                             wxBitmap( morgan1_xpm ),
                             _( "Show as \"De Morgan\" normal part" ),
                             wxITEM_CHECK );

        m_HToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
                             wxBitmap( morgan2_xpm ),
                             _( "Show as \"De Morgan\" convert part" ),
                             wxITEM_CHECK );

        m_HToolBar->AddSeparator();

        SelpartBox =
            new WinEDAChoiceBox( m_HToolBar, ID_LIBVIEW_SELECT_PART_NUMBER,
                                 wxDefaultPosition, wxSize( 150, -1 ) );
        m_HToolBar->AddControl( SelpartBox );

        m_HToolBar->AddSeparator();
        m_HToolBar->AddTool( ID_LIBVIEW_VIEWDOC, wxEmptyString,
                             wxBitmap( datasheet_xpm ),
                             _( "View component documents" ) );
        m_HToolBar->EnableTool( ID_LIBVIEW_VIEWDOC, false );

        if( m_Semaphore )
        {
            // The library browser is called from a "load component" command
            m_HToolBar->AddSeparator();
            m_HToolBar->AddTool( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC,
                                 wxEmptyString, wxBitmap( export_xpm ),
                                 _( "Insert component in schematic" ) );
        }

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_HToolBar->Realize();
    }

    if( (m_libraryName != wxEmptyString)
        && (m_entryName != wxEmptyString) )
    {
        lib = CMP_LIBRARY::FindLibrary( m_libraryName );

        if( lib != NULL )
        {
            component = lib->FindComponent( m_entryName );

            if( component && component->HasConversion() )
                asdeMorgan = true;

            entry = lib->FindEntry( m_entryName );
        }
    }

    // Must be AFTER Realize():
    if( asdeMorgan )
    {
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT,
                                (m_convert <= 1) ? true : false );
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
                                (m_convert >= 2) ? true : false );
        m_HToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, true );
        m_HToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, true );
    }
    else
    {
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, true  );
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, false );
        m_HToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, false );
        m_HToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, false );
    }


    int parts_count = 1;
    if( component )
        parts_count = MAX( component->GetPartCount(), 1 );
    SelpartBox->Clear();
    for( ii = 0; ii < parts_count; ii++ )
    {
        wxString msg;
        msg.Printf( _( "Part %c" ), 'A' + ii );
        SelpartBox->Append( msg );
    }

    SelpartBox->SetSelection( (m_unit > 0 ) ? m_unit - 1 : 0 );
    SelpartBox->Enable( parts_count > 1 );

    m_HToolBar->EnableTool( ID_LIBVIEW_VIEWDOC,
                            entry && ( entry->m_DocFile != wxEmptyString ) );
}


void WinEDA_ViewlibFrame::ReCreateVToolbar()
{
}
