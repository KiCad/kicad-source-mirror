/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
 * @file modview.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <3d_viewer.h>
#include <pcbcommon.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <modview_frame.h>
#include <wildcards_and_files_ext.h>


#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


void FOOTPRINT_VIEWER_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxString   msg;

    switch( event.GetId() )
    {
    case ID_MODVIEW_NEXT:
        SelectAndViewFootprint( NEXT_PART );
        break;

    case ID_MODVIEW_PREVIOUS:
        SelectAndViewFootprint( PREVIOUS_PART );
        break;

    default:
        msg << wxT( "FOOTPRINT_VIEWER_FRAME::Process_Special_Functions error: id = " )
            << event.GetId();
        wxMessageBox( msg );
        break;
    }
}


void FOOTPRINT_VIEWER_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool FOOTPRINT_VIEWER_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


/* Displays the name of the current opened library in the caption */
void FOOTPRINT_VIEWER_FRAME::DisplayLibInfos()
{
    wxString     msg;

    msg = _( "Library Browser" );
    msg << wxT( " [" );

    if( ! m_libraryName.IsEmpty() )
        msg << m_libraryName;
    else
        msg += _( "no library selected" );

    msg << wxT( "]" );

    SetTitle( msg );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentLibrary( wxCommandEvent& event )
{
    wxString msg;

    if( g_LibraryNames.GetCount() == 0 )
        return;

    wxArrayString headers;
    headers.Add( wxT( "Library" ) );
    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < g_LibraryNames.GetCount(); i++ )
    {
        wxArrayString item;
        item.Add( g_LibraryNames[i] );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Select Current Library:" ),
                         headers, itemsToDisplay, m_libraryName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    if( m_libraryName == dlg.GetTextSelection() )
        return;

    m_libraryName = dlg.GetTextSelection();
    m_footprintName.Empty();
    DisplayLibInfos();
    ReCreateFootprintList();

    int id = m_LibList->FindString( m_libraryName );

    if( id >= 0 )
        m_LibList->SetSelection( id );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint( wxCommandEvent& event )
{
    PCB_EDIT_FRAME* parent = (PCB_EDIT_FRAME*) GetParent();
    wxString        libname = m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension;
    MODULE*         oldmodule = GetBoard()->m_Modules;
    MODULE*         module = LoadModuleFromLibrary( libname, parent->GetFootprintLibraryTable(),
                                                    false );

    if( module )
    {
        module->SetPosition( wxPoint( 0, 0 ) );

        // Only one footprint allowed: remove the previous footprint (if exists)
        if( oldmodule )
        {
            GetBoard()->Remove( oldmodule );
            delete oldmodule;
        }

        m_footprintName = FROM_UTF8( module->GetFPID().GetFootprintName().c_str() );
        module->ClearFlags();
        SetCurItem( NULL );

        Zoom_Automatique( false );
        m_canvas->Refresh();
        Update3D_Frame();
        m_FootprintList->SetStringSelection( m_footprintName );
   }
}


const wxString FOOTPRINT_VIEWER_FRAME::GetSelectedLibraryFullName( void )
{
    wxString fullname = m_libraryName + wxT( "." ) + LegacyFootprintLibPathExtension;
    return fullname;
}


/* Routine to view one selected library content. */
void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( int aMode )
{
    if( m_libraryName.IsEmpty() )
        return;

    int selection = m_FootprintList->FindString( m_footprintName );

    if( aMode == NEXT_PART )
    {
        if( selection != wxNOT_FOUND && selection < (int)m_FootprintList->GetCount()-1 )
            selection++;
    }

    if( aMode == PREVIOUS_PART )
    {
        if( selection != wxNOT_FOUND && selection > 0)
            selection--;
    }

    if( selection != wxNOT_FOUND )
    {
        m_footprintName = m_FootprintList->GetString( selection );
        SetCurItem( NULL );
        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();
        MODULE* footprint = GetModuleLibrary( GetSelectedLibraryFullName(), m_footprintName, true );

        if( footprint )
            GetBoard()->Add( footprint, ADD_APPEND );

        Update3D_Frame();
    }

    DisplayLibInfos();
    Zoom_Automatique( false );
    m_canvas->Refresh( );
}


void FOOTPRINT_VIEWER_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    m_canvas->DrawBackGround( DC );
    GetBoard()->Draw( m_canvas, DC, GR_COPY );

    MODULE* module = GetBoard()->m_Modules;

    m_canvas->DrawCrossHair( DC );

    ClearMsgPanel();

    if( module )
        SetMsgPanel( module );
}
