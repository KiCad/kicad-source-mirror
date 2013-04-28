/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file listboxes.cpp
 * @brief Implementation of class for displaying footprint list and component lists.
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <macros.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>
#include <cvpcb_id.h>


/******************************************************************************
* Basic class (from wxListView) to display component and footprint lists
* Not directly used: the 2 list boxes actually used are derived from it
******************************************************************************/

ITEMS_LISTBOX_BASE::ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                                        const wxPoint& aLocation, const wxSize& aSize,
                                        long aStyle) :
    wxListView( aParent, aId, aLocation, aSize, LISTB_STYLE | aStyle )
{
    InsertColumn( 0, wxEmptyString );
    SetColumnWidth( 0, wxLIST_AUTOSIZE );
}


ITEMS_LISTBOX_BASE::~ITEMS_LISTBOX_BASE()
{
}


/*
 * Adjust the column width to the entire available window width
 */
void ITEMS_LISTBOX_BASE::OnSize( wxSizeEvent& event )
{
    wxSize size  = GetClientSize();
    int    width = 0;

    SetColumnWidth( 0, std::max( width, size.x ) );

    event.Skip();
}


/*
 * Return an index for the selected item
 */
int ITEMS_LISTBOX_BASE::GetSelection()
{
    return GetFirstSelected();
}


CVPCB_MAINFRAME* ITEMS_LISTBOX_BASE::GetParent()
{
    return (CVPCB_MAINFRAME*) wxListView::GetParent();
}


/*
 * Create or update the schematic components list.
 */
void CVPCB_MAINFRAME::BuildCmpListBox()
{
    COMPONENT*  component;
    wxString    msg;
    wxSize      size( 10, 10 );
    wxFont      guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_ListCmp == NULL )
    {
        m_ListCmp = new COMPONENTS_LISTBOX( this, ID_CVPCB_COMPONENT_LIST,
                                            wxDefaultPosition, size,
                                            0, NULL );
        m_ListCmp->SetBackgroundColour( wxColour( 225, 255, 255 ) );
        m_ListCmp->SetForegroundColour( wxColour( 0, 0, 0 ) );
        m_ListCmp->SetFont( wxFont( guiFont.GetPointSize(),
                                    wxFONTFAMILY_MODERN,
                                    wxFONTSTYLE_NORMAL,
                                    wxFONTWEIGHT_NORMAL ) );
    }

    m_ListCmp->m_ComponentList.Clear();

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );

        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    GetChars( component->GetReference() ),
                    GetChars( component->GetValue() ),
                    GetChars( component->GetFootprintName() ) );
        m_ListCmp->m_ComponentList.Add( msg );
    }

    m_ListCmp->SetItemCount( m_ListCmp->m_ComponentList.Count() );
    m_ListCmp->SetSelection( 0, true );
}


/*
 * Create or update the footprint list.
 */
void CVPCB_MAINFRAME::BuildFOOTPRINTS_LISTBOX()
{
    wxString msg;
    wxSize   size( 10, 10 );
    wxFont   guiFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );

    if( m_FootprintList == NULL )
    {
        m_FootprintList = new FOOTPRINTS_LISTBOX( this, ID_CVPCB_FOOTPRINT_LIST,
                                                  wxDefaultPosition, size,
                                                  0, NULL );
        m_FootprintList->SetBackgroundColour( wxColour( 225, 255, 225 ) );
        m_FootprintList->SetForegroundColour( wxColour( 0, 0, 0 ) );
        m_FootprintList->SetFont( wxFont( guiFont.GetPointSize(),
                                          wxFONTFAMILY_MODERN,
                                          wxFONTSTYLE_NORMAL,
                                          wxFONTWEIGHT_NORMAL ) );
    }

    m_FootprintList->SetFootprintFullList( m_footprints );
    DisplayStatus();
}
