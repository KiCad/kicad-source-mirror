/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/dc.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/wx_grid.h>
#include <scintilla_tricks.h>
#include <widgets/indicator_icon.h>
#include <kiplatform/ui.h>


//-------- GRID_CELL_TEXT_EDITOR ------------------------------------------------------
//

GRID_CELL_TEXT_EDITOR::GRID_CELL_TEXT_EDITOR() : wxGridCellTextEditor()
{}


void GRID_CELL_TEXT_EDITOR::SetValidator( const wxValidator& validator )
{
    // keep our own copy because wxGridCellTextEditor's is annoyingly private
    m_validator.reset( static_cast<wxValidator*>( validator.Clone() ) );

    wxGridCellTextEditor::SetValidator( *m_validator );
}


void GRID_CELL_TEXT_EDITOR::StartingKey( wxKeyEvent& event )
{
    if( m_validator )
    {
        m_validator->SetWindow( Text() );
        m_validator->ProcessEvent( event );
    }

    if( event.GetSkipped() )
    {
        wxGridCellTextEditor::StartingKey( event );
        event.Skip( false );
    }
}


void GRID_CELL_TEXT_EDITOR::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    WX_GRID::CellEditorTransformSizeRect( rect );

#if defined( __WXMSW__ )
    rect.Offset( 0, 1 );
#elif defined( __WXMAC__ )
    rect.Offset( 0, 2 );
    rect.SetHeight( rect.GetHeight() - 4 );
#endif

    wxGridCellEditor::SetSize( rect );      // NOLINT(*-parent-virtual-call)
}


//-------- GRID_CELL_TEXT_RENDERER ------------------------------------------------------
//

GRID_CELL_TEXT_RENDERER::GRID_CELL_TEXT_RENDERER() :
        wxGridCellStringRenderer()
{}


void GRID_CELL_TEXT_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect,
                                    int aRow, int aCol, bool isSelected )
{
    WX_GRID_TABLE_BASE* table = dynamic_cast<WX_GRID_TABLE_BASE*>( aGrid.GetTable() );

    if( !table || !table->IsExpanderColumn( aCol ) )
        return wxGridCellStringRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    wxString value = aGrid.GetCellValue( aRow, aCol );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    // draw the icon
    int leftCut = aDC.FromDIP( 4 );

    INDICATOR_ICON::ICON_ID state = ROW_ICON_PROVIDER::STATE::OFF;

    if( table->GetGroupType( aRow ) == GROUP_COLLAPSED )
        state = ROW_ICON_PROVIDER::STATE::CLOSED;
    else if( table->GetGroupType( aRow ) == GROUP_EXPANDED )
        state = ROW_ICON_PROVIDER::STATE::OPEN;

    wxBitmap bitmap = static_cast<WX_GRID&>( aGrid ).GetRowIconProvider()->GetIndicatorIcon( state );
    bitmap.SetScaleFactor( KIPLATFORM::UI::GetPixelScaleFactor( &aGrid ) );

    aDC.DrawBitmap( bitmap,
                    rect.GetLeft() + leftCut,
                    rect.GetTop() + ( rect.GetHeight() - bitmap.GetLogicalHeight() ) / 2,
                    true );

    leftCut += bitmap.GetLogicalWidth();

    leftCut += aDC.FromDIP( 4 );

    if( table->GetGroupType( aRow ) == CHILD_ITEM )
        leftCut += aDC.FromDIP( 12 );

    rect.x += leftCut;
    rect.width -= leftCut;

    // draw the text
    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, value, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}


wxSize GRID_CELL_TEXT_RENDERER::GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col )
{
    WX_GRID_TABLE_BASE* table = dynamic_cast<WX_GRID_TABLE_BASE*>( grid.GetTable() );

    if( !table || !table->IsExpanderColumn( col ) )
        return wxGridCellStringRenderer::GetBestSize( grid, attr, dc, row, col );

    INDICATOR_ICON::ICON_ID state = ROW_ICON_PROVIDER::STATE::OFF;
    wxBitmap                bitmap = static_cast<WX_GRID&>( grid ).GetRowIconProvider()->GetIndicatorIcon( state );

    bitmap.SetScaleFactor( KIPLATFORM::UI::GetPixelScaleFactor( &grid ) );

    wxString text = grid.GetCellValue( row, col );
    wxSize   size = wxGridCellStringRenderer::DoGetBestSize( attr, dc, text );

    size.x += bitmap.GetLogicalWidth() + dc.FromDIP( 8 );

    if( table->GetGroupType( row ) == CHILD_ITEM )
        size.x += dc.FromDIP( 12 );

    size.y = std::max( size.y, dc.FromDIP( 2 ) );

    return size;
}


//-------- GRID_CELL_ESCAPED_TEXT_RENDERER ------------------------------------------------------
//

GRID_CELL_ESCAPED_TEXT_RENDERER::GRID_CELL_ESCAPED_TEXT_RENDERER() :
        wxGridCellStringRenderer()
{}

void GRID_CELL_ESCAPED_TEXT_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC, const wxRect& aRect,
                                            int aRow, int aCol, bool isSelected )
{
    wxString unescaped = UnescapeString( aGrid.GetCellValue( aRow, aCol ) );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, unescaped, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}


wxSize GRID_CELL_ESCAPED_TEXT_RENDERER::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                                     int aRow, int aCol )
{
    wxString unescaped = UnescapeString( aGrid.GetCellValue( aRow, aCol ) );
    return wxGridCellStringRenderer::DoGetBestSize( aAttr, aDC, unescaped );
}


//-------- GRID_CELL_STC_EDITOR -----------------------------------------------------------------
//

GRID_CELL_STC_EDITOR::GRID_CELL_STC_EDITOR( bool aIgnoreCase, bool aSingleLine,
                                            std::function<void( wxStyledTextEvent&, SCINTILLA_TRICKS* )> onCharFn ) :
        m_scintillaTricks( nullptr ),
        m_ignoreCase( aIgnoreCase ),
        m_singleLine( aSingleLine ),
        m_onCharFn( std::move( onCharFn ) )
{}


void GRID_CELL_STC_EDITOR::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    WX_GRID::CellEditorTransformSizeRect( rect );

#if defined( __WXMSW__ )
#if !wxCHECK_VERSION( 3, 3, 0 )
    rect.Offset( -1, 0 );
    // hack no longer needed with wx 3.3
    rect.SetHeight( rect.GetHeight() + 6 );
#else
    rect.Offset( 0, 1 );
#endif
#elif defined( __WXGTK__ )
    rect.Offset( -1, 3 );
#else
    rect.Offset( 1, 3 );
    rect.SetWidth( rect.GetWidth() - 1 );
    rect.SetHeight( rect.GetHeight() - 4 );
#endif
    wxGridCellEditor::SetSize( rect );
}


void GRID_CELL_STC_EDITOR::Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler )
{
    m_control = new wxStyledTextCtrl( aParent, wxID_ANY, wxDefaultPosition, wxSize( 0, 0 ),
                                      wxBORDER_NONE );

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
            stc_ctrl(), wxEmptyString, m_singleLine,

            // onAcceptFn
            [this]( wxKeyEvent& aEvent )
            {
                HandleReturn( aEvent );
            },

            // onCharFn
            [this]( wxStyledTextEvent& aEvent )
            {
                m_onCharFn( aEvent, m_scintillaTricks );
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


void GRID_CELL_STC_EDITOR::StartingKey( wxKeyEvent& event )
{
    int ch;

    bool isPrintable;

#if wxUSE_UNICODE
    ch = event.GetUnicodeKey();

    if( ch != WXK_NONE )
        isPrintable = true;
    else
#endif // wxUSE_UNICODE
    {
        ch = event.GetKeyCode();
        isPrintable = ch >= WXK_SPACE && ch < WXK_START;
    }

    switch( ch )
    {
    case WXK_DELETE:
        // Delete the initial character when starting to edit with DELETE.
        stc_ctrl()->DeleteRange( 0, 1 );
        break;

    case WXK_BACK:
        // Delete the last character when starting to edit with BACKSPACE.
        stc_ctrl()->DeleteBack();
        break;

    default:
        if( isPrintable )
            stc_ctrl()->WriteText( static_cast<wxChar>( ch ) );
        break;
    }
}


void GRID_CELL_STC_EDITOR::Show( bool aShow, wxGridCellAttr* aAttr )
{
    if( !aShow )
        stc_ctrl()->AutoCompCancel();

    wxGridCellEditor::Show( aShow, aAttr );
}


void GRID_CELL_STC_EDITOR::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto evtHandler = static_cast<wxGridCellEditorEvtHandler*>( m_control->GetEventHandler() );

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


//-------- Editor Base Class for GRID_TEXT_BUTTON_HELPERS ------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor
//
// Note: this class is here instead of in grid_text_button_helpers.h/cpp to
// keep from dragging a ton of stuff into kicommon.


wxString GRID_CELL_TEXT_BUTTON::GetValue() const
{
    return Combo()->GetValue();
}


void GRID_CELL_TEXT_BUTTON::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    WX_GRID::CellEditorTransformSizeRect( rect );

    wxGridCellEditor::SetSize( rect );
}


void GRID_CELL_TEXT_BUTTON::StartingKey( wxKeyEvent& event )
{
    // Note: this is a copy of wxGridCellTextEditor's StartingKey()

    // Since this is now happening in the EVT_CHAR event EmulateKeyPress is no
    // longer an appropriate way to get the character into the text control.
    // Do it ourselves instead.  We know that if we get this far that we have
    // a valid character, so not a whole lot of testing needs to be done.

    // wxComboCtrl inherits from wxTextEntry, so can statically cast
    wxTextEntry* textEntry = static_cast<wxTextEntry*>( Combo() );
    int ch;

    bool isPrintable;

#if wxUSE_UNICODE
    ch = event.GetUnicodeKey();

    if( ch != WXK_NONE )
        isPrintable = true;
    else
#endif // wxUSE_UNICODE
    {
        ch = event.GetKeyCode();
        isPrintable = ch >= WXK_SPACE && ch < WXK_START;
    }

    switch( ch )
    {
    case WXK_DELETE:
        // Delete the initial character when starting to edit with DELETE.
        textEntry->Remove( 0, 1 );
        break;

    case WXK_BACK:
        // Delete the last character when starting to edit with BACKSPACE.
    {
        const long pos = textEntry->GetLastPosition();
        textEntry->Remove( pos - 1, pos );
    }
        break;

    default:
        if( isPrintable )
            textEntry->WriteText( static_cast<wxChar>( ch ) );

        break;
    }
}


void GRID_CELL_TEXT_BUTTON::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto evtHandler = static_cast< wxGridCellEditorEvtHandler* >( m_control->GetEventHandler() );

    // Don't immediately end if we get a kill focus event within BeginEdit
    evtHandler->SetInSetFocus( true );

    m_value = aGrid->GetTable()->GetValue( aRow, aCol );

    Combo()->SetValue( m_value );
    Combo()->SetFocus();
}


bool GRID_CELL_TEXT_BUTTON::EndEdit( int, int, const wxGrid*, const wxString&, wxString *aNewVal )
{
    const wxString value = Combo()->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void GRID_CELL_TEXT_BUTTON::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
}


void GRID_CELL_TEXT_BUTTON::Reset()
{
    Combo()->SetValue( m_value );
}


#if wxUSE_VALIDATORS
void GRID_CELL_TEXT_BUTTON::SetValidator( const wxValidator& validator )
{
    m_validator.reset( static_cast< wxValidator* >( validator.Clone() ) );
}
#endif


