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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <bitmaps.h>
#include <scintilla_tricks.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/grid_text_helpers.h>
#include <grid_tricks.h>
#include <dialogs/html_message_box.h>
#include <../sim/simulator_frame.h>
#include <sim/sim_signal_utils.h>
#include <dialog_user_defined_signals.h>
#include <widgets/wx_infobar.h>
#include <string_utils.h>
#include <gal/color4d.h>

#include <wx/settings.h>

#include <map>
#include <set>


DIALOG_USER_DEFINED_SIGNALS::DIALOG_USER_DEFINED_SIGNALS( SIMULATOR_FRAME* aParent,
                                                          std::map<int, wxString>* aSignals ) :
        DIALOG_USER_DEFINED_SIGNALS_BASE( aParent ),
        m_frame( aParent ),
        m_signals( aSignals ),
        m_helpWindow( nullptr )
{
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_grid->SetColAttr( 1, attr );

    for( const auto& [ id, signal ] : *m_signals )
        addGridRow( signal, id );

    // The theme's line colour can match the cell background; derive a visible one.
    KIGFX::COLOR4D bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    KIGFX::COLOR4D fg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    m_grid->SetGridLineColour( fg.Mix( bg, 0.35 ).ToColour() );
    m_grid->EnableGridLines( true );

    // The default highlight hides the grid lines of its row.
    m_grid->SetCellHighlightPenWidth( 1 );

    m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    m_addButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    SetupStandardButtons();
    Layout();

    // Bound on the grid: the dialog's size event arrives before layout.
    m_grid->Bind( wxEVT_SIZE,
                  [this]( wxSizeEvent& aEvent )
                  {
                      adjustGridColumns();
                      aEvent.Skip();
                  } );

    CallAfter(
            [this]()
            {
                adjustGridColumns();
            } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_USER_DEFINED_SIGNALS::adjustGridColumns()
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( m_grid );

    int indexWidth = m_grid->GetTextExtent( wxString::Format( wxT( "%d" ), m_grid->GetNumberRows() ) ).x + 12;
    indexWidth = std::max( indexWidth, 24 );

    if( m_grid->GetRowLabelSize() != indexWidth )
        m_grid->SetRowLabelSize( indexWidth );

    // Column 1 holds the signal id and is zero-width; the expression column takes the rest.
    // The client size excludes only a scrollbar that takes space, unlike a macOS overlay one
    int width = m_grid->GetClientSize().x - indexWidth;

    if( width > 0 && width != m_grid->GetColSize( 0 ) )
        m_grid->SetColSize( 0, width );
}


DIALOG_USER_DEFINED_SIGNALS::~DIALOG_USER_DEFINED_SIGNALS()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


void DIALOG_USER_DEFINED_SIGNALS::addGridRow( const wxString& aText, int aId )
{
    int row = m_grid->GetNumberRows();

    m_grid->AppendRows();
    m_grid->SetCellValue( row, 0, aText );
    m_grid->SetCellValue( row, 1, wxString::Format( wxS( "%d" ), aId ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_STC_EDITOR( true, true,
            [this]( wxStyledTextEvent& aEvent, SCINTILLA_TRICKS* aScintillaTricks )
            {
                onScintillaCharAdded( aEvent, aScintillaTricks );
            } ) );
    m_grid->SetAttr( row, 0, attr );
}


void DIALOG_USER_DEFINED_SIGNALS::onAddSignal( wxCommandEvent& event )
{
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                long newId = 0;

                for( int ii = 0; ii < m_grid->GetNumberRows(); ++ii )
                {
                    long usedId;
                    m_grid->GetCellValue( ii, 1 ).ToLong( &usedId );

                    if( usedId >= newId )
                        newId = usedId + 1;
                }

                addGridRow( wxEmptyString, (int) newId );
                return { m_grid->GetNumberRows() - 1, 0 };
            } );

    adjustGridColumns();
}


void DIALOG_USER_DEFINED_SIGNALS::onDeleteSignal( wxCommandEvent& event )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                m_grid->DeleteRows( row, 1 );
            } );

    adjustGridColumns();
}


void DIALOG_USER_DEFINED_SIGNALS::onCheckSyntax( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    std::vector<wxString> signals = m_frame->Signals();

    if( signals.empty() && m_frame->Netnames().empty() )
    {
        m_infoBar->ShowMessageFor( _( "There are no signals to check against yet.  Run the "
                                      "simulation first." ),
                                   8000, wxICON_WARNING );
        return;
    }

    // ngspice ignores case in vector and node names
    std::set<wxString> known;

    for( const wxString& signal : signals )
        known.insert( SimSignalBaseName( signal ).Lower() );

    // Voltages are valid whether or not all voltages are saved.
    for( const wxString& netname : m_frame->Netnames() )
        known.insert( wxString::Format( wxS( "V(%s)" ), netname ).Lower() );

    // Component voltages are expanded in any case.
    std::set<wxString> deviceVoltages;

    for( const wxString& name : m_frame->DeviceVoltages() )
        deviceVoltages.insert( name.Lower() );

    wxArrayString problems;

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        wxString expr = m_grid->GetCellValue( row, 0 );
        expr.Trim( true ).Trim( false );

        if( expr.IsEmpty() )
            continue;

        if( !SIM_SIGNAL::ParensBalanced( expr ) )
        {
            problems.Add( wxString::Format( _( "%s: unbalanced parentheses." ), EscapeHTML( expr ) ) );
            continue;
        }

        for( const wxString& ref : SIM_SIGNAL::ExtractSignalRefs( expr ) )
        {
            if( known.count( ref.Lower() ) || deviceVoltages.count( ref.Lower() ) )
                continue;

            problems.Add( wxString::Format( _( "%s: %s is not a known signal." ), EscapeHTML( expr ),
                                            EscapeHTML( ref ) ) );
        }
    }

    // Nets differing only in case are the same net in ngspice.
    std::map<wxString, std::set<wxString>> netsByLower;

    for( const wxString& netname : m_frame->Netnames() )
        netsByLower[netname.Lower()].insert( netname );

    for( const auto& [lower, spellings] : netsByLower )
    {
        if( spellings.size() < 2 )
            continue;

        wxString list;

        for( const wxString& spelling : spellings )
        {
            if( !list.IsEmpty() )
                list += wxS( ", " );

            list += spelling;
        }

        problems.Add( wxString::Format( _( "Nets %s differ only in capitalization; ngspice cannot "
                                           "tell them apart." ),
                                        EscapeHTML( list ) ) );
    }

    if( problems.IsEmpty() )
    {
        m_infoBar->ShowMessageFor( _( "No problems found." ), 6000, wxICON_INFORMATION );
        return;
    }

    m_infoBar->Dismiss();

    HTML_MESSAGE_BOX dlg( this, _( "Signal Check" ) );

    dlg.SetDialogSizeInDU( 400, 250 );
    dlg.MessageSet( wxString::Format( _( "%d problem(s) found:" ), (int) problems.GetCount() ) );
    dlg.ListSet( problems );
    dlg.ShowModal();
}


void DIALOG_USER_DEFINED_SIGNALS::onScintillaCharAdded( wxStyledTextEvent &aEvent, SCINTILLA_TRICKS* aTricks )
{
    wxStyledTextCtrl*  textCtrl = aTricks->Scintilla();
    wxArrayString      tokens;
    std::set<wxString> seen;

    auto addToken = [&]( const wxString& aToken )
    {
        // Case-insensitive, so vectors don't re-add signals already offered.
        if( !aToken.IsEmpty() && seen.insert( aToken.Lower() ).second )
            tokens.push_back( aToken );
    };

    // Skip user-defined signals: they are whole expressions, not names.
    std::set<wxString> userDefined;

    for( const auto& [id, signal] : m_frame->UserDefinedSignals() )
        userDefined.insert( signal );

    // ngspice lower-cases vectors; offer the schematic's spelling.
    for( const wxString& signal : m_frame->Signals() )
    {
        if( !userDefined.count( signal ) )
            addToken( SimSignalBaseName( signal ) );
    }

    // Vectors without a schematic spelling ("time", "frequency"), except the internal derived ones
    for( const wxString& vector : m_frame->SimPlotVectors() )
    {
        if( !SIM_SIGNAL::IsDerivedSignalVector( vector ) )
            addToken( vector );
    }

    tokens.push_back( wxS( "sqrt(x)" ) );
    tokens.push_back( wxS( "sin(x)" ) );
    tokens.push_back( wxS( "cos(x)" ) );
    tokens.push_back( wxS( "tan(x)" ) );
    tokens.push_back( wxS( "sinh(x)" ) );
    tokens.push_back( wxS( "cosh(x)" ) );
    tokens.push_back( wxS( "tanh(x)" ) );
    tokens.push_back( wxS( "asin(x)" ) );
    tokens.push_back( wxS( "acos(x)" ) );
    tokens.push_back( wxS( "atan(x)" ) );
    tokens.push_back( wxS( "asinh(x)" ) );
    tokens.push_back( wxS( "acosh(x)" ) );
    tokens.push_back( wxS( "atanh(x)" ) );
    tokens.push_back( wxS( "arctan(x)" ) );
    tokens.push_back( wxS( "exp(x)" ) );
    tokens.push_back( wxS( "ln(x)" ) );
    tokens.push_back( wxS( "log(x)" ) );
    tokens.push_back( wxS( "abs(x)" ) );
    tokens.push_back( wxS( "nint(x)" ) );
    tokens.push_back( wxS( "int(x)" ) );
    tokens.push_back( wxS( "floor(x)" ) );
    tokens.push_back( wxS( "ceil(x)" ) );
    tokens.push_back( wxS( "pow(x,y)" ) );
    tokens.push_back( wxS( "pwr(x,y)" ) );
    tokens.push_back( wxS( "min(x,y)" ) );
    tokens.push_back( wxS( "max(x,y)" ) );
    tokens.push_back( wxS( "sgn(x)" ) );
    tokens.push_back( wxS( "ternary_fcn(x,y,z)" ) );
    tokens.push_back( wxS( "gauss(nom,rvar,sigma)" ) );
    tokens.push_back( wxS( "agauss(nom,avar,sigma)" ) );
    tokens.push_back( wxS( "unif(nom,rvar)" ) );
    tokens.push_back( wxS( "aunif(nom,avar)" ) );
    tokens.push_back( wxS( "limit(nom,avar)" ) );

    int text_pos = textCtrl->GetCurrentPos();
    int start = textCtrl->WordStartPosition( text_pos, true );
    int parenCount = 0;

    for( start = text_pos - 1; start > 0; start-- )
    {
        wxUniChar c = textCtrl->GetCharAt( start );

        if( c == '(' )
        {
            if( parenCount )
            {
                start += 1;
                break;
            }
            else
            {
                parenCount++;
            }
        }
        else if( wxIsalpha( c ) && parenCount )
        {
            break;
        }
        else if( !wxIsalnum( c ) && c != '/' )
        {
            start += 1;
            break;
        }
    }

    wxString partial = start >= 0 ? textCtrl->GetRange( start, text_pos ) : wxString();

    // An empty partial matches every token, so ")" or an operator would pop the whole list up
    if( partial.IsEmpty() )
        aTricks->CancelAutocomplete();
    else
        aTricks->DoAutocomplete( partial, tokens );

    textCtrl->SetFocus();
}


bool DIALOG_USER_DEFINED_SIGNALS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !m_grid->CommitPendingChanges() )
        return false;

    // Signals are looked up by name, so a copy of a native name would capture the native signal
    std::set<wxString> native;

    for( const wxString& signal : m_frame->NativeSignals() )
        native.insert( SimSignalBaseName( signal ).Lower() );

    for( int ii = 0; ii < m_grid->GetNumberRows(); ++ii )
    {
        wxString signal = m_grid->GetCellValue( ii, 0 );

        if( native.count( signal.Strip( wxString::both ).Lower() ) )
        {
            m_grid->SetGridCursor( ii, 0 );
            m_infoBar->ShowMessageFor( wxString::Format( _( "'%s' is already a signal and needs no "
                                                            "user-defined copy." ),
                                                         signal ),
                                       8000, wxICON_ERROR );
            return false;
        }
    }

    m_signals->clear();

    for( int ii = 0; ii < m_grid->GetNumberRows(); ++ii )
    {
        wxString signal = m_grid->GetCellValue( ii, 0 );

        if( !signal.IsEmpty() )
        {
            long id;
            m_grid->GetCellValue( ii, 1 ).ToLong( &id );
            (*m_signals)[ (int) id ] = signal;
        }
    }

    return true;
}


void DIALOG_USER_DEFINED_SIGNALS::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    wxString msg =
#include "../sim/user_defined_signals_help_md.h"
     ;

    m_helpWindow = new HTML_MESSAGE_BOX( nullptr, _( "Syntax Help" ) );

    wxSize sz( 320, 320 );
    m_helpWindow->SetMinSize( m_helpWindow->ConvertDialogToPixels( sz ) );
    m_helpWindow->SetDialogSizeInDU( sz.x, sz.y );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( msg ), html_txt );
    m_helpWindow->AddHTML_Text( html_txt );
    m_helpWindow->ShowModeless();
}


