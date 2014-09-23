/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file tool_viewlib.cpp
 * @brief Build the toolbars for the library browser.
 */

#include <fctsys.h>
#include <macros.h>
#include <eeschema_id.h>

#include <general.h>
#include <hotkeys.h>
#include <class_library.h>
#include <viewlib_frame.h>
#include <dialog_helpers.h>


void LIB_VIEW_FRAME::ReCreateHToolbar()
{
    wxString    msg;
    LIB_ALIAS*  entry = NULL;
    bool        asdeMorgan = false;
    LIB_PART*   part = NULL;

    if( m_mainToolBar  == NULL )
    {
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

        // Set up toolbar
        m_mainToolBar->AddTool( ID_LIBVIEW_SELECT_LIB, wxEmptyString,
                                KiBitmap( library_xpm ),
                                _( "Select library to browse" ) );

        m_mainToolBar->AddTool( ID_LIBVIEW_SELECT_PART, wxEmptyString,
                                KiBitmap( add_component_xpm ),
                                _( "Select component to browse" ) );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
                                KiBitmap( lib_previous_xpm ),
                                _( "Display previous component" ) );

        m_mainToolBar->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
                                KiBitmap( lib_next_xpm ),
                                _( "Display next component" ) );

        m_mainToolBar->AddSeparator();
        msg = AddHotkeyName( _( "Zoom in" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_IN, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                                KiBitmap( zoom_in_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom out" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_OUT, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                                KiBitmap( zoom_out_xpm ), msg );

        msg = AddHotkeyName( _( "Redraw view" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_REDRAW, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                             KiBitmap( zoom_redraw_xpm ), msg );

        msg = AddHotkeyName( _( "Zoom auto" ), s_Viewlib_Hokeys_Descr,
                             HK_ZOOM_AUTO, IS_COMMENT );
        m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                                KiBitmap( zoom_fit_in_page_xpm ), msg );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
                                KiBitmap( morgan1_xpm ),
                                _( "Show as \"De Morgan\" normal part" ),
                                wxITEM_CHECK );

        m_mainToolBar->AddTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
                                KiBitmap( morgan2_xpm ),
                                _( "Show as \"De Morgan\" convert part" ),
                                wxITEM_CHECK );

        m_mainToolBar->AddSeparator();

        m_selpartBox = new wxComboBox( m_mainToolBar, ID_LIBVIEW_SELECT_PART_NUMBER,
                                       wxEmptyString, wxDefaultPosition,
                                       wxSize( 150, -1 ), 0, NULL, wxCB_READONLY );
        m_mainToolBar->AddControl( m_selpartBox );

        m_mainToolBar->AddSeparator();
        m_mainToolBar->AddTool( ID_LIBVIEW_VIEWDOC, wxEmptyString,
                                KiBitmap( datasheet_xpm ),
                                _( "View component documents" ) );
        m_mainToolBar->EnableTool( ID_LIBVIEW_VIEWDOC, false );

        if( IsModal() )
        {
            m_mainToolBar->AddSeparator();
            m_mainToolBar->AddTool( ID_LIBVIEW_CMP_EXPORT_TO_SCHEMATIC,
                                    wxEmptyString, KiBitmap( export_xpm ),
                                    _( "Insert component in schematic" ) );
        }

        // after adding the buttons to the toolbar, must call Realize() to
        // reflect the changes
        m_mainToolBar->Realize();
    }

    if( m_libraryName.size() && m_entryName.size() )
    {
        if( PART_LIB* lib = Prj().SchLibs()->FindLibrary( m_libraryName ) )
        {
            part = lib->FindPart( m_entryName );

            if( part && part->HasConversion() )
                asdeMorgan = true;

            entry = lib->FindEntry( m_entryName );
        }
    }

    // Must be AFTER Realize():
    m_mainToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, asdeMorgan );
    m_mainToolBar->EnableTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, asdeMorgan );

    if( asdeMorgan )
    {
        bool normal = m_convert <= 1;
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT,normal );
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, !normal );
    }
    else
    {
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, true  );
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, false );
     }

    int parts_count = 1;

    if( part )
        parts_count = std::max( part->GetUnitCount(), 1 );

    m_selpartBox->Clear();

    for( int ii = 0; ii < parts_count; ii++ )
    {
        wxString msg = wxString::Format( _( "Unit %c" ), 'A' + ii );
        m_selpartBox->Append( msg );
    }

    m_selpartBox->SetSelection( m_unit > 0 ? m_unit - 1 : 0 );
    m_selpartBox->Enable( parts_count > 1 );

    m_mainToolBar->EnableTool( ID_LIBVIEW_VIEWDOC, entry && !!entry->GetDocFileName() );

    m_mainToolBar->Refresh();
}


void LIB_VIEW_FRAME::ReCreateVToolbar()
{
}
