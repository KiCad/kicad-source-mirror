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

#include <scintilla_tricks.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/grid_text_helpers.h>
#include <grid_tricks.h>
#include <dialogs/html_message_box.h>
#include <../sim/simulator_frame.h>
#include <dialog_user_defined_signals.h>
#include <string_utils.h>


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

    m_addButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    SetupStandardButtons();
    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
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
}


void DIALOG_USER_DEFINED_SIGNALS::onDeleteSignal( wxCommandEvent& event )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                m_grid->DeleteRows( row, 1 );
            } );
}


void DIALOG_USER_DEFINED_SIGNALS::onScintillaCharAdded( wxStyledTextEvent &aEvent, SCINTILLA_TRICKS* aTricks )
{
    wxStyledTextCtrl* textCtrl = aTricks->Scintilla();
    wxArrayString     tokens;

    for( const wxString& signal : m_frame->SimPlotVectors() )
        tokens.push_back( signal );

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

    if( start >= 0 )    // i.e. at least one char entered
    {
        wxString partial = textCtrl->GetRange( start, text_pos );
        aTricks->DoAutocomplete( partial, tokens );
    }

    textCtrl->SetFocus();
}


bool DIALOG_USER_DEFINED_SIGNALS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !m_grid->CommitPendingChanges() )
        return false;

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


