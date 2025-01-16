/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/grid_combobox.h>


//-------- Renderer ---------------------------------------------------------------------
// None required; just render as normal text.



//-------- Editor -----------------------------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor


GRID_CELL_COMBOBOX::GRID_CELL_COMBOBOX( const wxArrayString& names ) :
        m_names( names )
{
}


wxGridCellEditor* GRID_CELL_COMBOBOX::Clone() const
{
    return new GRID_CELL_COMBOBOX( m_names );
}


void GRID_CELL_COMBOBOX::Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler )
{
    m_control = new wxComboBox( aParent, wxID_ANY, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize, m_names );

    wxGridCellEditor::Create(aParent, aId, aEventHandler);
}


wxString GRID_CELL_COMBOBOX::GetValue() const
{
    return Combo()->GetValue();
}


void GRID_CELL_COMBOBOX::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    rect.Inflate( -1 );

#if defined( __WXMAC__ )
    rect.Inflate( 3 );      // no FOCUS_RING, even on Mac
#endif

    Combo()->SetSize( rect, wxSIZE_ALLOW_MINUS_ONE );
}


void GRID_CELL_COMBOBOX::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Don't immediately end if we get a kill focus event within BeginEdit
    evtHandler->SetInSetFocus( true );

    // These event handlers are needed to properly dismiss the editor when the popup is closed
    m_control->Bind(wxEVT_COMBOBOX_DROPDOWN, &GRID_CELL_COMBOBOX::onComboDropDown, this);
    m_control->Bind(wxEVT_COMBOBOX_CLOSEUP, &GRID_CELL_COMBOBOX::onComboCloseUp, this);

    m_value = aGrid->GetTable()->GetValue( aRow, aCol );

    Combo()->SetFocus();

    // Work around a wxWidgets bug where the drop-down is the wrong width on the first drop.
    Combo()->Set( Combo()->GetStrings() );

    Combo()->ChangeValue( m_value );
    Combo()->SelectAll();

#ifdef __WXOSX_COCOA__
    // This is a work around for the combobox being simply dismissed when a
    // choice is made in it under OS X. The bug is almost certainly due to a
    // problem in focus events generation logic but it's not obvious to fix and
    // for now this at least allows one to use wxGrid.
    Combo()->Popup();
#endif

    // When dropping down the menu, a kill focus event
    // happens after this point, so we can't reset the flag yet.
#if !defined(__WXGTK__)
    evtHandler->SetInSetFocus( false );
#endif
}


bool GRID_CELL_COMBOBOX::EndEdit( int , int , const wxGrid* , const wxString& , wxString *aNewVal )
{
    const wxString value = Combo()->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void GRID_CELL_COMBOBOX::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
}


void GRID_CELL_COMBOBOX::Reset()
{
}


void GRID_CELL_COMBOBOX::onComboDropDown( wxCommandEvent& aEvent )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Once the combobox is dropped, reset the flag to allow the focus-loss handler
    // to function and close the editor.
    evtHandler->SetInSetFocus( false );
}


void GRID_CELL_COMBOBOX::onComboCloseUp( wxCommandEvent& aEvent )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

    // Forward the combobox close up event to the cell event handler as a focus kill event
    // so that the grid editor is dismissed when the combox closes, otherwise it leaves the
    // dropdown arrow visible in the cell.
    wxFocusEvent event( wxEVT_KILL_FOCUS, m_control->GetId() );
    event.SetEventObject( m_control );
    evtHandler->ProcessEvent( event );
}
