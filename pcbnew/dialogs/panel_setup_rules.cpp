/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <confirm.h>
#include <widgets/paged_dialog.h>
#include <pcb_edit_frame.h>
#include <pcb_expr_evaluator.h>
#include <board.h>
#include <board_design_settings.h>
#include <project.h>
#include <kicad_string.h>
#include <tool/tool_manager.h>
#include <panel_setup_rules.h>
#include <wx_html_report_box.h>
#include <wx/treebook.h>
#include <dialogs/html_messagebox.h>
#include <scintilla_tricks.h>
#include <drc/drc_rule_parser.h>
#include <tools/drc_tool.h>
#include <dialog_helpers.h>

PANEL_SETUP_RULES::PANEL_SETUP_RULES( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_RULES_BASE( aParent->GetTreebook() ),
        m_Parent( aParent ),
        m_frame( aFrame ),
        m_scintillaTricks( nullptr ),
        m_helpWindow( nullptr )
{
    m_scintillaTricks = new SCINTILLA_TRICKS( m_textEditor, wxT( "()" ), false,
            [this]()
            {
                wxPostEvent( m_Parent, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    m_netClassRegex.Compile( "NetClass\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_netNameRegex.Compile( "NetName\\s*[!=]=\\s*$", wxRE_ADVANCED );

    m_compileButton->SetBitmap( KiBitmap( BITMAPS::drc ) );

    m_textEditor->Bind( wxEVT_STC_CHARADDED, &PANEL_SETUP_RULES::onScintillaCharAdded, this );
    m_textEditor->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED, &PANEL_SETUP_RULES::onScintillaCharAdded, this );
    m_textEditor->Bind( wxEVT_CHAR_HOOK, &PANEL_SETUP_RULES::onCharHook, this );
}


PANEL_SETUP_RULES::~PANEL_SETUP_RULES( )
{
    delete m_scintillaTricks;

    if( m_helpWindow )
        m_helpWindow->Destroy();
};


void PANEL_SETUP_RULES::onCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE && !m_textEditor->AutoCompActive() )
    {
        if( m_originalText != m_textEditor->GetText() )
        {
            if( !IsOK( this, _( "Cancel Changes?" ) ) )
                return;
        }
    }

    aEvent.Skip();
}


void PANEL_SETUP_RULES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    m_Parent->SetModified();
    m_textEditor->SearchAnchor();

    wxString rules = m_textEditor->GetText();
    int currentPos = m_textEditor->GetCurrentPos();
    int startPos = 0;

    for( int line = m_textEditor->LineFromPosition( currentPos ); line > 0; line-- )
    {
        int      lineStart = m_textEditor->PositionFromLine( line );
        wxString beginning = m_textEditor->GetTextRange( lineStart, lineStart + 10 );

        if( beginning.StartsWith( "(rule " ) )
        {
            startPos = lineStart;
            break;
        }
    }

    enum
    {
        NONE,
        STRING,
        SEXPR_OPEN,
        SEXPR_TOKEN,
        STRUCT_REF
    };

    std::stack<wxString> sexprs;
    wxString             partial;
    wxString             last;
    int                  context = NONE;
    int                  expr_context = NONE;

    for( int i = startPos; i < currentPos; ++i )
    {
        wxChar c = m_textEditor->GetCharAt( i );

        if( c == '\\' )
        {
            i++;  // skip escaped char
        }
        else if( context == STRING )
        {
            if( c == '"' )
            {
                context = NONE;
            }
            else
            {
                if( expr_context == STRING )
                {
                    if( c == '\'' )
                        expr_context = NONE;
                    else
                        partial += c;
                }
                else if( c == '\'' )
                {
                    last = partial;
                    partial = wxEmptyString;
                    expr_context = STRING;
                }
                else if( c == '.' )
                {
                    partial = wxEmptyString;
                    expr_context = STRUCT_REF;
                }
                else
                {
                    partial += c;
                }
            }
        }
        else if( c == '"' )
        {
            last = partial;
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
            if( !sexprs.empty() )
                sexprs.pop();

            context = NONE;
        }
        else if( c == ' ' )
        {
            if( context == SEXPR_OPEN && !partial.IsEmpty() )
            {
                m_textEditor->AutoCompCancel();
                sexprs.push( partial );

                if( sexprs.size() && ( sexprs.top() == "constraint"
                                    || sexprs.top() == "disallow"
                                    || sexprs.top() == "layer" ) )
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
        {
            tokens = "rule "
                     "version";
        }
        else if( sexprs.top() == "rule" )
        {
            tokens = "condition "
                     "constraint "
                     "layer";
        }
        else if( sexprs.top() == "constraint" )
        {
            tokens = "max "
                     "min "
                     "opt";
        }
    }
    else if( context == SEXPR_TOKEN )
    {
        if( sexprs.empty() )
        {
            /* badly formed grammar */
        }
        else if( sexprs.top() == "constraint" )
        {
            tokens = "annular_width "
                     "clearance "
                     "courtyard_clearance "
                     "diff_pair_gap "
                     "diff_pair_uncoupled "
                     "disallow "
                     "edge_clearance "
                     "length "
                     "hole "
                     "hole_clearance "
                     "hole_to_hole "
                     "silk_clearance "
                     "skew "
                     "track_width "
                     "via_count ";
        }
        else if( sexprs.top() == "disallow"
              || sexprs.top() == "buried_via"
              || sexprs.top() == "graphic"
              || sexprs.top() == "hole"
              || sexprs.top() == "micro_via"
              || sexprs.top() == "pad"
              || sexprs.top() == "text"
              || sexprs.top() == "track"
              || sexprs.top() == "via"
              || sexprs.top() == "zone" )
        {
            tokens = "buried_via "
                     "graphic "
                     "hole "
                     "micro_via "
                     "pad "
                     "text "
                     "track "
                     "via "
                     "zone";
        }
        else if( sexprs.top() == "layer" )
        {
            tokens = "inner "
                     "outer "
                     "\"x\"";
        }
    }
    else if( context == STRING && !sexprs.empty() && sexprs.top() == "condition" )
    {
        if( expr_context == STRUCT_REF )
        {
            PROPERTY_MANAGER&  propMgr = PROPERTY_MANAGER::Instance();
            std::set<wxString> propNames;

            for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
            {
                const PROPERTY_LIST& props = propMgr.GetProperties( cls.type );

                for( PROPERTY_BASE* prop : props )
                {
                    wxString ref( prop->Name() );
                    ref.Replace( " ", "_" );
                    propNames.insert( ref );
                }
            }

            for( const wxString& propName : propNames )
                tokens += " " + propName;

            PCB_EXPR_BUILTIN_FUNCTIONS& functions = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

            for( const wxString& funcSig : functions.GetSignatures() )
                tokens += " " + funcSig;
        }
        else if( expr_context == STRING )
        {
            if( m_netClassRegex.Matches( last ) )
            {
                BOARD*                 board = m_frame->GetBoard();
                BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

                for( const std::pair<const wxString, NETCLASSPTR>& entry : bds.GetNetClasses() )
                    tokens += " " + entry.first;
            }
            else if( m_netNameRegex.Matches( last ) )
            {
                BOARD* board = m_frame->GetBoard();

                for( const wxString& netnameCandidate : board->GetNetClassAssignmentCandidates() )
                    tokens += " " + netnameCandidate;
            }
        }
    }

    if( !tokens.IsEmpty() )
        m_scintillaTricks-> DoAutocomplete( partial, wxSplit( tokens, ' ' ) );
}


void PANEL_SETUP_RULES::OnCompile( wxCommandEvent& event )
{
    m_errorsReport->Clear();

    try
    {
        std::vector<DRC_RULE*> dummyRules;

        DRC_RULES_PARSER parser( m_textEditor->GetText(), _( "DRC rules" ) );

        parser.Parse( dummyRules, m_errorsReport );
    }
    catch( PARSE_ERROR& pe )
    {
        wxString msg = wxString::Format( "%s <a href='%d:%d'>%s</a>%s",
                                         _( "ERROR:" ),
                                         pe.lineNumber,
                                         pe.byteIndex,
                                         pe.ParseProblem(),
                                         wxEmptyString );

        m_errorsReport->Report( msg, RPT_SEVERITY_ERROR );
    }

    m_errorsReport->Flush();
}


void PANEL_SETUP_RULES::OnErrorLinkClicked( wxHtmlLinkEvent& event )
{
    wxString      link = event.GetLinkInfo().GetHref();
    wxArrayString parts;
    long          line = 0, offset = 0;

    wxStringSplit( link, parts, ':' );

    if( parts.size() > 1 )
    {
        parts[0].ToLong( &line );
        parts[1].ToLong( &offset );
    }

    int pos = m_textEditor->PositionFromLine( line - 1 ) + ( offset - 1 );

    m_textEditor->GotoPos( pos );

    m_textEditor->SetFocus();
}


bool PANEL_SETUP_RULES::TransferDataToWindow()
{
    wxFileName rulesFile( m_frame->GetDesignRulesPath() );

    if( rulesFile.FileExists() )
    {
        wxTextFile file( rulesFile.GetFullPath() );

        if( file.Open() )
        {
            for ( wxString str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine() )
            {
                ConvertSmartQuotesAndDashes( &str );
                m_textEditor->AddText( str << '\n' );
            }

            wxCommandEvent dummy;
            OnCompile( dummy );
        }
    }

    m_originalText = m_textEditor->GetText();

    if( m_frame->Prj().IsNullProject() )
    {
        m_textEditor->ClearAll();
        m_textEditor->AddText( _( "Design rules cannot be added without a project" ) );
        m_textEditor->Disable();
    }

    return true;
}


bool PANEL_SETUP_RULES::TransferDataFromWindow()
{
    if( m_originalText == m_textEditor->GetText() )
        return true;

    if( m_frame->Prj().IsNullProject() )
        return true;

    wxString rulesFilepath = m_frame->GetDesignRulesPath();

    try
    {
        if( m_textEditor->SaveFile( rulesFilepath ) )
        {
            m_frame->GetBoard()->GetDesignSettings().m_DRCEngine->InitEngine( rulesFilepath );
            return true;
        }
    }
    catch( PARSE_ERROR& )
    {
        // Don't lock them in to the Setup dialog if they have bad rules.  They've already
        // saved them so we can allow an exit.
        return true;
    }

    return false;
}


void PANEL_SETUP_RULES::OnSyntaxHelp( wxHyperlinkEvent& aEvent )
{
    if( m_helpWindow )
    {
        m_helpWindow->ShowModeless();
        return;
    }

    wxString msg =
#include "dialogs/panel_setup_rules_help_md.h"
    ;

#ifdef __WXMAC__
    msg.Replace( "Ctrl+", "Cmd+" );
#endif

    m_helpWindow = new HTML_MESSAGE_BOX( nullptr, _( "Syntax Help" ) );
    m_helpWindow->SetDialogSizeInDU( 320, 320 );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( msg ), html_txt );
    m_helpWindow->m_htmlWindow->AppendToPage( html_txt );

    m_helpWindow->ShowModeless();
}
