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
#include <html_messagebox.h>
#include <scintilla_tricks.h>
#include <drc/drc_rule_parser.h>

PANEL_SETUP_RULES::PANEL_SETUP_RULES( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_RULES_BASE( aParent->GetTreebook() ),
        m_Parent( aParent ),
        m_frame( aFrame ),
        m_scintillaTricks( nullptr )
{
    m_scintillaTricks = new SCINTILLA_TRICKS( m_textEditor, wxT( "{}" ) );

    int    size = wxNORMAL_FONT->GetPointSize();
    wxFont fixedFont( size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

    for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
        m_textEditor->StyleSetFont( i, fixedFont );

    m_textEditor->Bind( wxEVT_STC_CHARADDED, &PANEL_SETUP_RULES::onScintillaCharAdded, this );
}


PANEL_SETUP_RULES::~PANEL_SETUP_RULES( )
{
    delete m_scintillaTricks;
};


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
        wxChar c = m_textEditor->GetCharAt( i );

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
            {
                m_textEditor->AutoCompCancel();
                sexprs.push( partial );
            }

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
                m_textEditor->AutoCompCancel();
                sexprs.push( partial );

                if( sexprs.size()
                    && ( sexprs.top() == "constraint" || sexprs.top() == "disallow" ) )
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
            m_scintillaTricks->DoAutocomplete( partial, wxSplit( tokens, ' ' ) );
    }
    else if( context == SEXPR_TOKEN )
    {
        if( sexprs.top() == "constraint" )
            tokens = "annulus_width clearance hole track_width";
        else if( sexprs.top() == "disallow" )
            tokens = "buried_via graphic hole micro_via pad text track via zone";

        if( !tokens.IsEmpty() )
            m_scintillaTricks->DoAutocomplete( partial, wxSplit( tokens, ' ' ) );
    }
}


bool PANEL_SETUP_RULES::TransferDataToWindow()
{
    wxString   rulesFilepath = m_frame->Prj().AbsolutePath( "drc-rules" );
    wxFileName rulesFile( rulesFilepath );

    if( rulesFile.FileExists() )
        m_textEditor->LoadFile( rulesFile.GetFullPath() );

    m_originalText = m_textEditor->GetText();

    return true;
}


bool PANEL_SETUP_RULES::TransferDataFromWindow()
{
    if( m_originalText == m_textEditor->GetText() )
        return true;

    try
    {
        std::vector<DRC_SELECTOR*> dummySelectors;
        std::vector<DRC_RULE*>     dummyRules;

        DRC_RULES_PARSER parser( m_frame->GetBoard(), m_textEditor->GetText(), _( "DRC rules" ) );

        parser.Parse( dummySelectors, dummyRules );
    }
    catch( PARSE_ERROR& pe )
    {
        m_Parent->SetError( pe.What(), this, m_textEditor, pe.lineNumber, pe.byteIndex );
        return false;
    }

    if( m_textEditor->SaveFile( m_frame->Prj().AbsolutePath( "drc-rules" ) ) )
    {
        m_frame->GetToolManager()->GetTool<DRC>()->LoadRules();
        return true;
    }

    return false;
}


void PANEL_SETUP_RULES::OnSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    // Do not make this full sentence translatable: it contains keywords
    // Only a few titles can be traslated.
    wxString msg;
    msg << "<b>" << _( "Top-level Clauses" ) << "</b>";
    msg <<  "<pre>"
            "(version &lt;number>)\r"
            "(rule &lt;rule_name> &lt;rule_clause> ...)\r"
            "\r</pre><b>";
    msg << _( "Rule Clauses" );
    msg <<  "</b>"
            "<pre>"
            "(disallow &lt;item_type>)\r"
            "(constraint &lt;constraint_type> ...)\r"
            "(condition \"&lt;expression>\")\r"
            "\r</pre>"
            "<b>";
    msg << _( "Item Types" );
    msg <<  "</b>"
            "<pre>"
            "track         via               zone\r"
            "pad           micro_via         text\r"
            "hole          buried_via        graphic\r"
            "\r</pre>"
            "<b>";
    msg << _( "Constraint Types" );
    msg <<  "</b>"
            "<pre>"
            "clearance    annulus_width   track_width     hole\r"
            "\r</pre>"
            "<b>";
    msg << _( "Examples" );
    msg <<  "</b>"
            "<pre>"
            "(rule \"copper keepout\"\r"
            "   (disallow track) (disallow via) (disallow zone)\r"
            "   (condition \"A.name == no_copper\"))\r"
            "\r"
            "(rule \"BGA neckdown\"\r"
            "   (constraint track_width (min 0.2mm) (opt 0.25mm))\r"
            "   (constraint clearance (min 0.05) (opt 0.08mm))\r"
            "   (condition \"A.insideCourtyard('U3')\"))\r"
            "\r"
            "(rule HV\r"
            "   (constraint clearance (min 1.5mm))\r"
            "   (condition \"A.netclass == HV\"))\r"
            "\r"
            "(rule HV_HV\r"
            "   (constraint clearance (min 2.0mm))\r"
            "   (condition \"A.netclass == HV && B.netclass == HV\"))\r"
            "</pre>";

    HTML_MESSAGE_BOX dlg( m_parent, _( "Syntax Help" ) );
    dlg.SetDialogSizeInDU( 320, 320 );

    dlg.AddHTML_Text( msg );
    dlg.ShowModal();
}
