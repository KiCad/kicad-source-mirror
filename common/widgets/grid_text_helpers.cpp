/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string_utils.h>
#include <wx/stc/stc.h>
#include <widgets/grid_text_helpers.h>
#include <scintilla_tricks.h>


//-------- GRID_CELL_ESCAPED_TEXT_RENDERER ------------------------------------------------------
//

GRID_CELL_ESCAPED_TEXT_RENDERER::GRID_CELL_ESCAPED_TEXT_RENDERER() :
        wxGridCellStringRenderer()
{
}

void GRID_CELL_ESCAPED_TEXT_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                            const wxRect& aRect, int aRow, int aCol,
                                            bool isSelected )
{
    wxString unescaped = UnescapeString( aGrid.GetCellValue( aRow, aCol ) );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, unescaped, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}


wxSize GRID_CELL_ESCAPED_TEXT_RENDERER::GetBestSize( wxGrid & aGrid, wxGridCellAttr & aAttr,
                                                     wxDC & aDC, int aRow, int aCol )
{
    wxString unescaped = UnescapeString( aGrid.GetCellValue( aRow, aCol ) );
    return wxGridCellStringRenderer::DoGetBestSize( aAttr, aDC, unescaped );
}


//-------- GRID_CELL_STC_EDITOR -----------------------------------------------------------------
//

GRID_CELL_STC_EDITOR::GRID_CELL_STC_EDITOR( bool aIgnoreCase,
                                            std::function<void( wxStyledTextEvent&,
                                                                SCINTILLA_TRICKS* )> aOnChar ) :
    m_scintillaTricks( nullptr ),
    m_ignoreCase( aIgnoreCase ),
    m_onChar( aOnChar )
{ }


void GRID_CELL_STC_EDITOR::Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler )
{
    m_control = new wxStyledTextCtrl( aParent );

    stc_ctrl()->SetTabIndents( false );
    stc_ctrl()->SetBackSpaceUnIndents( false );
    stc_ctrl()->SetViewEOL( false );
    stc_ctrl()->SetViewWhiteSpace( false );
    stc_ctrl()->SetIndentationGuides( false );
    stc_ctrl()->SetMarginWidth( 0, 0 ); // Symbol margin
    stc_ctrl()->SetMarginWidth( 1, 0 ); // Line-number margin
    stc_ctrl()->SetEOLMode( wxSTC_EOL_LF );
    stc_ctrl()->AutoCompSetMaxWidth( 25 );
    stc_ctrl()->AutoCompSetIgnoreCase( m_ignoreCase );
    stc_ctrl()->UsePopUp( 0 );

    // A hack which causes Scintilla to auto-size the text editor canvas
    // See: https://github.com/jacobslusser/ScintillaNET/issues/216
    stc_ctrl()->SetScrollWidth( 1 );
    stc_ctrl()->SetScrollWidthTracking( true );

    m_scintillaTricks = new SCINTILLA_TRICKS(
            stc_ctrl(), wxEmptyString, true,
            // onAccept handler
            [this]()
            {
                stc_ctrl()->AutoCompComplete();
            },
            // onCharAdded handler
            [this]( wxStyledTextEvent& aEvent )
            {
                m_onChar( aEvent, m_scintillaTricks );
            } );

    stc_ctrl()->Bind( wxEVT_KILL_FOCUS, &GRID_CELL_STC_EDITOR::onFocusLoss, this );

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


wxStyledTextCtrl* GRID_CELL_STC_EDITOR::stc_ctrl() const
{
    return static_cast<wxStyledTextCtrl*>( m_control );
}


wxString GRID_CELL_STC_EDITOR::GetValue() const
{
    return stc_ctrl()->GetText();
}


void GRID_CELL_STC_EDITOR::Show( bool aShow, wxGridCellAttr* aAttr )
{
    if( !aShow )
        stc_ctrl()->AutoCompCancel();

    wxGridCellEditor::Show( aShow, aAttr );
}


void GRID_CELL_STC_EDITOR::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler()
                                                                );

    // Don't immediately end if we get a kill focus event within BeginEdit
    evtHandler->SetInSetFocus( true );

    m_value = aGrid->GetTable()->GetValue( aRow, aCol );

    stc_ctrl()->SetFocus();
    stc_ctrl()->SetText( m_value );
    stc_ctrl()->SelectAll();
}


bool GRID_CELL_STC_EDITOR::EndEdit( int, int, const wxGrid*, const wxString&, wxString *aNewVal )
{
    const wxString value = stc_ctrl()->GetText();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void GRID_CELL_STC_EDITOR::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
}


void GRID_CELL_STC_EDITOR::onFocusLoss( wxFocusEvent& aEvent )
{
    if( stc_ctrl() )
        stc_ctrl()->AutoCompCancel();

    aEvent.Skip();
}
