/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/paged_dialog.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <tool/tool_manager.h>
#include <drc/drc.h>
#include <panel_setup_rules.h>


PANEL_SETUP_RULES::PANEL_SETUP_RULES( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_RULES_BASE( aParent->GetTreebook() ),
        m_frame( aFrame ),
        m_lastCaretPos( -1 )
{
    m_textEditor->SetIndentationGuides( wxSTC_IV_LOOKBOTH );

    wxColour highlight = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
   	wxColour highlightText = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    if( KIGFX::COLOR4D( highlightText ).GetBrightness() > 0.5 )
        highlight = highlight.ChangeLightness( 75 );
    else
        highlight = highlight.ChangeLightness( 125 );

    m_textEditor->StyleSetForeground( wxSTC_STYLE_BRACELIGHT, highlightText );
    m_textEditor->StyleSetBackground( wxSTC_STYLE_BRACELIGHT, highlight );
    m_textEditor->StyleSetForeground( wxSTC_STYLE_BRACEBAD, *wxRED );

    m_textEditor->Bind( wxEVT_STC_CHARADDED, &PANEL_SETUP_RULES::onScintillaCharAdded, this );
    m_textEditor->Bind( wxEVT_STC_UPDATEUI, &PANEL_SETUP_RULES::onScintillaUpdateUI, this );
}


void PANEL_SETUP_RULES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    constexpr int flags = wxSTC_FIND_REGEXP| wxSTC_FIND_POSIX;

    m_textEditor->SearchAnchor();

    int i = std::max( 0, m_textEditor->SearchPrev( flags, "\( *rule " ) );
    int currentPos = m_textEditor->GetCurrentPos();

    enum
    {
        NONE,
        STRING,
        SEXPR_OPEN,
        SEXPR_TOKEN,
    };

    std::stack<wxString> sexprs;
    wxString             partial;
    int                  context = NONE;

    for( ; i < currentPos; ++i )
    {
        char c = (char) m_textEditor->GetCharAt( i );

        if( c == '\\' )
        {
            i++;  // skip escaped char
            continue;
        }

        if( context == STRING )
        {
            if( c == '"' )
                context = NONE;
            else
                partial += c;

            continue;
        }

        if( c == '"' )
        {
            partial = wxEmptyString;
            context = STRING;
        }
        else if( c == '(' )
        {
            if( context == SEXPR_OPEN && !partial.IsEmpty() )
                sexprs.push( partial );

            partial = wxEmptyString;
            context = SEXPR_OPEN;
        }
        else if( c == ')' )
        {
            sexprs.pop();
            context = NONE;
        }
        else if( c == ' ' )
        {
            if( context == SEXPR_OPEN && !partial.IsEmpty() )
            {
                sexprs.push( partial );

                wxString top = sexprs.size() ? sexprs.top() : wxEmptyString;

                if( top == "constraint" || top == "disallow" )
                {
                    partial = wxEmptyString;
                    context = SEXPR_TOKEN;
                    continue;
                }
            }

            context = NONE;
        }
        else
        {
            partial += c;
        }
    }

    // NB: tokens MUST be in alphabetical order because the Scintilla engine is going
    // to do a binary search on them
    wxString tokens;

    if( context == SEXPR_OPEN )
    {
        if( sexprs.empty() )
            tokens = "rule version";
        else if( sexprs.top() == "rule" )
            tokens = "condition constraint disallow";
        else if( sexprs.top() == "constraint" )
            tokens = "max min opt";

        if( !tokens.IsEmpty() )
            m_textEditor->AutoCompShow( partial.size(), tokens );
    }
    else if( context == SEXPR_TOKEN )
    {
        if( sexprs.top() == "constraint" )
            tokens = "annulus_width clearance hole track_width";
        else if( sexprs.top() == "disallow" )
            tokens = "buried_via graphic hole micro_via pad text track via zone";

        int wordStartPos = m_textEditor->WordStartPosition( currentPos, true );
        wxASSERT( currentPos - wordStartPos == partial.size() );

        if( !tokens.IsEmpty() )
            m_textEditor->AutoCompShow( partial.size(), tokens );
    }
}


void PANEL_SETUP_RULES::onScintillaUpdateUI( wxStyledTextEvent& aEvent )
{
    auto isBrace = []( int c ) -> bool
                   {
                       return c == '(' || c == ')';
                   };

    // Has the caret changed position?
    int caretPos = m_textEditor->GetCurrentPos();

    if( m_lastCaretPos != caretPos )
    {
        m_lastCaretPos = caretPos;
        int bracePos1 = -1;
        int bracePos2 = -1;

        // Is there a brace to the left or right?
        if( caretPos > 0 && isBrace( m_textEditor->GetCharAt( caretPos-1 ) ) )
            bracePos1 = ( caretPos - 1 );
        else if( isBrace( m_textEditor->GetCharAt( caretPos ) ) )
            bracePos1 = caretPos;

        if( bracePos1 >= 0 )
        {
            // Find the matching brace
            bracePos2 = m_textEditor->BraceMatch( bracePos1 );

            if( bracePos2 == -1 )
            {
                m_textEditor->BraceBadLight( bracePos1 );
                m_textEditor->SetHighlightGuide( 0 );
            }
            else
            {
                m_textEditor->BraceHighlight( bracePos1, bracePos2 );
                m_textEditor->SetHighlightGuide( m_textEditor->GetColumn( bracePos1 ) );
            }
        }
        else
        {
            // Turn off brace matching
            m_textEditor->BraceHighlight( -1, -1 );
            m_textEditor->SetHighlightGuide( 0 );
        }
    }
}


bool PANEL_SETUP_RULES::TransferDataToWindow()
{
    wxString   rulesFilepath = m_frame->Prj().AbsolutePath( "drc-rules" );
    wxFileName rulesFile( rulesFilepath );

    if( rulesFile.FileExists() )
        m_textEditor->LoadFile( rulesFile.GetFullPath() );

    return true;
}


bool PANEL_SETUP_RULES::TransferDataFromWindow()
{
    if( m_textEditor->SaveFile( m_frame->Prj().AbsolutePath( "drc-rules" ) ) )
    {
        m_frame->GetToolManager()->GetTool<DRC>()->Reset( TOOL_BASE::MODEL_RELOAD );
        return true;
    }

    return false;
}


