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
#include "class_library.h"
#include "libviewfrm.h"


void WinEDA_ViewlibFrame::ReCreateHToolbar()
{
    int  ii;
    CMP_LIBRARY* lib;
    LIB_COMPONENT* component = NULL;
    CMP_LIB_ENTRY* entry = NULL;
    bool asdeMorgan = false;

    if( m_HToolBar  == NULL )
    {
        m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR,
                                         TRUE );
        SetToolBar( m_HToolBar );

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
        m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                             wxBitmap( zoom_in_xpm ),
                             _( "Zoom in" ) );

        m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                             wxBitmap( zoom_out_xpm ),
                             _( "Zoom out" ) );

        m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                             wxBitmap( zoom_redraw_xpm ),
                             _( "Redraw view" ) );

        m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                             wxBitmap( zoom_auto_xpm ),
                             _( "Best zoom" ) );

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
        m_HToolBar->EnableTool( ID_LIBVIEW_VIEWDOC, FALSE );

        if( m_Semaphore )   // The lib browser is called from a "load component" command
        {
            m_HToolBar->AddSeparator();
            m_HToolBar->AddTool( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC,
                                 wxEmptyString,
                                 wxBitmap( export_xpm ),
                                 _( "Insert component in schematic" ) );
        }

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_HToolBar->Realize();
    }

    if( (g_CurrentViewLibraryName != wxEmptyString)
        && (g_CurrentViewComponentName != wxEmptyString) )
    {
        lib = CMP_LIBRARY::FindLibrary( g_CurrentViewLibraryName );

        if( lib != NULL )
        {
            component = lib->FindComponent( g_CurrentViewComponentName );

            if( component && component->HasConversion() )
                asdeMorgan = true;

            entry = lib->FindEntry( g_CurrentViewComponentName );
        }
    }

    // Must be AFTER Realize():
    m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT,
                            (g_ViewConvert <= 1) ? TRUE : FALSE );
    m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT,
                            (g_ViewConvert >= 2) ? TRUE : FALSE );
    m_HToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, asdeMorgan );
    m_HToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, asdeMorgan );

    int jj = 1;
    if( component )
        jj = MAX( component->m_UnitCount, 1 );
    SelpartBox->Clear();
    for( ii = 0; ii < jj; ii++ )
    {
        wxString msg;
        msg.Printf( _( "Part %c" ), 'A' + ii );
        SelpartBox->Append( msg );
    }

    SelpartBox->SetSelection( 0 );
    SelpartBox->Enable( component && component->HasConversion() );

    m_HToolBar->EnableTool( ID_LIBVIEW_VIEWDOC,
                            entry && ( entry->m_DocFile != wxEmptyString ) );
}


void WinEDA_ViewlibFrame::ReCreateVToolbar()
{
}
