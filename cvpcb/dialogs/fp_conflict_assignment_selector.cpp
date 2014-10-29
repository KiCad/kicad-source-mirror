/**
 * @file fp_conflict_assignment_selector.cpp
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2014 Kicad Developers, see CHANGELOG.TXT for contributors.
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

#include <common.h>

#include <fp_conflict_assignment_selector.h>


DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR( wxWindow* aParent )
        : DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE( aParent )
{
    m_listFp->AppendColumn( _( "Ref" ) );
    m_listFp->AppendColumn(  _( "Schematic assignment" ) );
    m_listFp->AppendColumn( wxT( "<=" ) );
    m_listFp->AppendColumn( wxT( "=>" ) );
    m_listFp->AppendColumn( _( "Cmp file assignment" ) );

    m_lineCount = 0;
}

void DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::Add( const wxString& aRef, const wxString& aFpSchName,
          const wxString& aFpCmpName )
{
    long idx = m_listFp->InsertItem(m_lineCount, aRef );

    m_listFp->SetItem(idx, COL_FPSCH, aFpSchName );
    m_listFp->SetItem(idx, COL_SELSCH, wxT("")  );
    m_listFp->SetItem(idx, COL_SELCMP, wxT("X") );
    m_listFp->SetItem(idx, COL_FPCMP, aFpCmpName );

    m_lineCount ++;
}

int DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::GetSelection( const wxString& aReference )
{
    // Find  Reference
    for( int ii = 0; ii < m_listFp->GetItemCount(); ii++ )
    {
        if( m_listFp->GetItemText( ii, COL_REF ) == aReference )
        {
            if( m_listFp->GetItemText( ii, COL_SELSCH ) != wxT("X") )
                return 1;

            return 0;
        }
    }

    return -1;
}

void DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::OnColumnClick( wxListEvent& event )
{
    // When clicking on the column title:
    // when it is the COL_SELCMP column, set all item choices to cmp file assigment.
    // when it is the COL_SELSCH column, set all item choices to schematic assigment.

    wxListItem item = event.GetItem();

    int column = event.GetColumn();
    int colclr, colset;

    switch( column )
    {
    case COL_SELSCH:
        colclr = COL_SELCMP;
        colset = COL_SELSCH;
        break;

    case COL_SELCMP:
        colclr = COL_SELSCH;
        colset = COL_SELCMP;
        break;

    default:
        return;
    }

    for( int i = 0; i < m_listFp->GetItemCount(); i++ )
    {
        m_listFp->SetItem( i, colclr, wxT("") );
        m_listFp->SetItem( i, colset, wxT("X") );
    }
}

void DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::OnItemClicked( wxMouseEvent& event )
{
    wxPoint pos = event.GetPosition();
    int flgs = wxLIST_HITTEST_ONITEMLABEL;
    long idx = m_listFp->HitTest( pos, flgs );

    // Try to find the column clicked (must be COL_SELCMP or COL_SELSCH)
    int colclr = -1, colset;
    int minpx = m_listFp->GetColumnWidth( 0 ) + m_listFp->GetColumnWidth( 1 );
    int maxpx = minpx + m_listFp->GetColumnWidth( 2 );

    if( pos.x > minpx && pos.x < maxpx )
    {
        colclr = COL_SELCMP;
        colset = COL_SELSCH;
    }

    else
    {
        minpx = maxpx;
        int maxpx = minpx + m_listFp->GetColumnWidth( 3 );

        if( pos.x > minpx && pos.x < maxpx )
        {
            colclr = COL_SELSCH;
            colset = COL_SELCMP;
        }
    }

    if( colclr < 0 )
        return;

    // Move selection to schematic or cmp file choice
    // according to the column position (COL_SELCMP or COL_SELSCH)
    m_listFp->SetItem( idx, colclr, wxT("") );
    m_listFp->SetItem( idx, colset, wxT("X") );

    event.Skip();
}


void DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::OnSize( wxSizeEvent& aEvent )
{
    recalculateColumns();
    aEvent.Skip();
}


void DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR::recalculateColumns()
{
    const int margin = 16;
    int totalLength = 0;
    int sel_length = GetTextSize( wxT("XX"), m_listFp ).x;
    int maxRefLength = GetTextSize( wxT("XXX"), m_listFp ).x;

    sel_length += margin;
    m_listFp->SetColumnWidth( COL_SELSCH, sel_length );
    m_listFp->SetColumnWidth( COL_SELCMP, sel_length );

    // Find max character width of column Reference
    for( int i = 0; i < m_listFp->GetItemCount(); i++ )
    {
        int length = GetTextSize( m_listFp->GetItemText( i, COL_REF ), m_listFp ).x;

        if( length > maxRefLength )
            maxRefLength = length;
    }


    // Use the lengths of column texts to create a scale of the max list width
    // to set the column widths
    maxRefLength += margin;
    totalLength = maxRefLength + sel_length + sel_length;

    int cwidth = (GetClientSize().x - totalLength) / 2;

    m_listFp->SetColumnWidth( COL_REF, maxRefLength );
    m_listFp->SetColumnWidth( COL_FPSCH, cwidth - 2 );
    m_listFp->SetColumnWidth( COL_FPCMP, cwidth );
}

