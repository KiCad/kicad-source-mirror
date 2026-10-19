/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/git/dialog_text_diff.h>

#include <diff_merge/diff_scene.h>
#include <kiplatform/ui.h>
#include <widgets/ui_common.h>

#include <wx/settings.h>
#include <wx/stc/stc.h>

static constexpr int STYLE_ADDED = 1;
static constexpr int STYLE_REMOVED = 2;


void FillTextDiffControl( wxStyledTextCtrl* aCtrl, const std::vector<KIGIT::TEXT_DIFF_LINE>& aLines )
{
    wxStyledTextCtrl* stc = aCtrl;

    stc->SetReadOnly( false );
    stc->ClearAll();
    stc->StyleSetFont( wxSTC_STYLE_DEFAULT, KIUI::GetMonospacedUIFont() );

    // Scintilla ignores the desktop theme, and StyleClearAll copies the default style onto
    // every other one, so these have to come before it.
    stc->StyleSetForeground( wxSTC_STYLE_DEFAULT, wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
    stc->StyleSetBackground( wxSTC_STYLE_DEFAULT, wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOX ) );
    stc->StyleClearAll();
    stc->SetMarginWidth( 1, 0 );

    const KICAD_DIFF::DIFF_COLOR_THEME theme = KICAD_DIFF::DIFF_COLOR_THEME::ForText( KIPLATFORM::UI::IsDarkTheme() );

    stc->StyleSetForeground( STYLE_ADDED, theme.added.ToColour() );
    stc->StyleSetForeground( STYLE_REMOVED, theme.removed.ToColour() );

    for( const KIGIT::TEXT_DIFF_LINE& line : aLines )
    {
        int start = stc->GetLength();

        switch( line.kind )
        {
        case KIGIT::TEXT_DIFF_KIND::ADDED: stc->AppendText( wxS( "+ " ) + line.text + wxS( "\n" ) ); break;
        case KIGIT::TEXT_DIFF_KIND::REMOVED: stc->AppendText( wxS( "- " ) + line.text + wxS( "\n" ) ); break;
        case KIGIT::TEXT_DIFF_KIND::CONTEXT: stc->AppendText( wxS( "  " ) + line.text + wxS( "\n" ) ); break;
        }

        if( line.kind != KIGIT::TEXT_DIFF_KIND::CONTEXT )
        {
            stc->StartStyling( start );
            stc->SetStyling( stc->GetLength() - start,
                             line.kind == KIGIT::TEXT_DIFF_KIND::ADDED ? STYLE_ADDED : STYLE_REMOVED );
        }
    }

    stc->SetReadOnly( true );
}


DIALOG_TEXT_DIFF::DIALOG_TEXT_DIFF( wxWindow* aParent, const wxString& aTitle,
                                    const std::vector<KIGIT::TEXT_DIFF_LINE>& aLines ) :
        DIALOG_TEXT_DIFF_BASE( aParent, wxID_ANY, aTitle )
{
    FillTextDiffControl( m_diff, aLines );

    SetupStandardButtons();
    finishDialogSettings();
}
