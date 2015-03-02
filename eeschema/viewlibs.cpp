/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file viewlibs.cpp
 */

#include <fctsys.h>
#include <kiway.h>
#include <gr_basic.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <class_sch_screen.h>

#include <general.h>
#include <viewlib_frame.h>
#include <eeschema_id.h>
#include <class_library.h>
#include <dialog_helpers.h>


#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


void LIB_VIEW_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxString   msg;
    LIB_ALIAS* entry;
    int        ii, id = event.GetId();

    switch( id )
    {
    case ID_LIBVIEW_SELECT_LIB:
        SelectCurrentLibrary();
        break;

    case ID_LIBVIEW_SELECT_PART:
        SelectAndViewLibraryPart( NEW_PART );
        break;

    case ID_LIBVIEW_NEXT:
        SelectAndViewLibraryPart( NEXT_PART );
        break;

    case ID_LIBVIEW_PREVIOUS:
        SelectAndViewLibraryPart( PREVIOUS_PART );
        break;

    case ID_LIBVIEW_VIEWDOC:
        entry = Prj().SchLibs()->FindLibraryEntry( m_entryName, m_libraryName );

        if( entry && !entry->GetDocFileName().IsEmpty() )
        {
            SEARCH_STACK* lib_search = Prj().SchSearchS();

            GetAssociatedDocument( this, entry->GetDocFileName(), lib_search );
        }
        break;

    case ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT:
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, true );
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, false );
        m_convert = 1;
        m_canvas->Refresh();
        break;

    case ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT:
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, false );
        m_mainToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, true );
        m_convert = 2;
        m_canvas->Refresh();
        break;

    case ID_LIBVIEW_SELECT_PART_NUMBER:
        ii = m_selpartBox->GetCurrentSelection();
        if( ii < 0 )
            return;
        m_unit = ii + 1;
        m_canvas->Refresh();
        break;

    default:
        msg << wxT( "LIB_VIEW_FRAME::Process_Special_Functions error: id = " ) << id;
        DisplayError( this, msg );
        break;
    }
}


void LIB_VIEW_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool LIB_VIEW_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


void LIB_VIEW_FRAME::DisplayLibInfos()
{
    PART_LIBS*  libs = Prj().SchLibs();

    if( libs )
    {
        PART_LIB* lib = libs->FindLibrary( m_libraryName );

        wxString     msg = _( "Library Browser" );

        msg += wxT( " [" );

        if( lib )
            msg += lib->GetFullFileName();
        else
            msg += _( "no library selected" );

        msg += wxT( "]" );

        SetTitle( msg );
    }
}


void LIB_VIEW_FRAME::SelectCurrentLibrary()
{
    PART_LIB* Lib;

    Lib = SelectLibraryFromList();

    if( Lib )
    {
        m_entryName.Empty();
        m_libraryName = Lib->GetName();
        DisplayLibInfos();

        if( m_libList )
        {
            ReCreateListCmp();
            m_canvas->Refresh();
            DisplayLibInfos();
            ReCreateHToolbar();
            int id = m_libList->FindString( m_libraryName.GetData() );

            if( id >= 0 )
                m_libList->SetSelection( id );
        }
    }
}


void LIB_VIEW_FRAME::SelectAndViewLibraryPart( int option )
{
    if( m_libraryName.IsEmpty() )
        SelectCurrentLibrary();

    if( m_libraryName.IsEmpty() )
        return;

    if( PART_LIBS* libs = Prj().SchLibs() )
    {
        if( PART_LIB* lib = libs->FindLibrary( m_libraryName ) )
        {
            if( m_entryName.IsEmpty() || option == NEW_PART )
            {
                ViewOneLibraryContent( lib, NEW_PART );
                return;
            }

            if( lib->FindEntry( m_entryName ) )
            {
                if( option == NEXT_PART )
                    ViewOneLibraryContent( lib, NEXT_PART );

                if( option == PREVIOUS_PART )
                    ViewOneLibraryContent( lib, PREVIOUS_PART );
            }
        }
    }
}


void LIB_VIEW_FRAME::ViewOneLibraryContent( PART_LIB* Lib, int Flag )
{
    int        NumOfParts = 0;

    if( Lib )
        NumOfParts = Lib->GetCount();

    if( NumOfParts == 0 )
    {
        DisplayError( this, wxT( "No library or library is empty!" ) );
        return;
    }

    LIB_ALIAS* entry;
    wxString   CmpName;

    if( Flag == NEW_PART )
        DisplayListComponentsInLib( Lib, CmpName, m_entryName );

    if( Flag == NEXT_PART )
    {
        entry = Lib->GetNextEntry( m_entryName );

        if( entry )
            CmpName = entry->GetName();
    }

    if( Flag == PREVIOUS_PART )
    {
        entry = Lib->GetPreviousEntry( m_entryName );

        if( entry )
            CmpName = entry->GetName();
    }

    m_unit    = 1;
    m_convert = 1;

    entry = Lib->FindEntry( CmpName );
    m_entryName = CmpName;
    DisplayLibInfos();
    Zoom_Automatique( false );
    m_canvas->Refresh( );

    if( m_cmpList )
    {
        int id = m_cmpList->FindString( m_entryName.GetData() );
        if( id >= 0 )
            m_cmpList->SetSelection( id );
    }

    ReCreateHToolbar();
}


void LIB_VIEW_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    LIB_ALIAS* entry = Prj().SchLibs()->FindLibraryEntry( m_entryName, m_libraryName );

    if( !entry )
        return;

    LIB_PART* part = entry->GetPart();

    if( !part )
        return;

    wxString    msg;
    wxString    tmp;

    m_canvas->DrawBackGround( DC );

    if( !entry->IsRoot() )
    {
        // Temporarily change the name field text to reflect the alias name.
        msg = entry->GetName();
        tmp = part->GetName();

        part->SetName( msg );

        if( m_unit < 1 )
            m_unit = 1;

        if( m_convert < 1 )
            m_convert = 1;
    }
    else
        msg = _( "None" );

    part->Draw( m_canvas, DC, wxPoint( 0, 0 ), m_unit, m_convert, GR_DEFAULT_DRAWMODE );

    // Redraw the cursor
    m_canvas->DrawCrossHair( DC );

    if( !tmp.IsEmpty() )
        part->SetName( tmp );

    ClearMsgPanel();
    AppendMsgPanel( _( "Part" ), part->GetName(), BLUE, 6 );
    AppendMsgPanel( _( "Alias" ), msg, RED, 6 );
    AppendMsgPanel( _( "Description" ), entry->GetDescription(), CYAN, 6 );
    AppendMsgPanel( _( "Key words" ), entry->GetKeyWords(), DARKDARKGRAY );
}
