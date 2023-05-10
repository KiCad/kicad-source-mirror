/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <panel_setup_rules.h>
#include <widgets/wx_html_report_box.h>
#include <dialogs/html_message_box.h>
#include <scintilla_tricks.h>
#include <drc/drc_rule_parser.h>
#include <tools/drc_tool.h>
#include <pgm_base.h>

PANEL_SETUP_RULES::PANEL_SETUP_RULES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_RULES_BASE( aParentWindow ),
        m_frame( aFrame ),
        m_scintillaTricks( nullptr ),
        m_helpWindow( nullptr )
{
    m_scintillaTricks = new SCINTILLA_TRICKS( m_textEditor, wxT( "()" ), false,
            [this]()
            {
                wxPostEvent( PAGED_DIALOG::GetDialog( this ),
                             wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            } );

    m_textEditor->AutoCompSetSeparator( '|' );

    m_netClassRegex.Compile( "^NetClass\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_netNameRegex.Compile( "^NetName\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_typeRegex.Compile( "^Type\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_viaTypeRegex.Compile( "^Via_Type\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_padTypeRegex.Compile( "^Pad_Type\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_pinTypeRegex.Compile( "^Pin_Type\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_fabPropRegex.Compile( "^Fabrication_Property\\s*[!=]=\\s*$", wxRE_ADVANCED );
    m_shapeRegex.Compile( "^Shape\\s*[!=]=\\s*$", wxRE_ADVANCED );

    m_compileButton->SetBitmap( KiBitmap( BITMAPS::drc ) );

    m_textEditor->SetZoom( Pgm().GetCommonSettings()->m_Appearance.text_editor_zoom );

    m_textEditor->UsePopUp( 0 );
    m_textEditor->Bind( wxEVT_STC_CHARADDED, &PANEL_SETUP_RULES::onScintillaCharAdded, this );
    m_textEditor->Bind( wxEVT_STC_AUTOCOMP_CHAR_DELETED, &PANEL_SETUP_RULES::onScintillaCharAdded, this );
    m_textEditor->Bind( wxEVT_CHAR_HOOK, &PANEL_SETUP_RULES::onCharHook, this );
}


PANEL_SETUP_RULES::~PANEL_SETUP_RULES( )
{
    Pgm().GetCommonSettings()->m_Appearance.text_editor_zoom = m_textEditor->GetZoom();

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


void PANEL_SETUP_RULES::OnContextMenu(wxMouseEvent &event)
{
    wxMenu   menu;

    menu.Append( wxID_UNDO, _( "Undo" ) );
    menu.Append( wxID_REDO, _( "Redo" ) );

    menu.AppendSeparator();

    menu.Append( 1, _( "Cut" ) );       // Don't use wxID_CUT, wxID_COPY, etc.  On Mac (at least),
    menu.Append( 2, _( "Copy" ) );      // wxWidgets never delivers them to us.
    menu.Append( 3, _( "Paste" ) );
    menu.Append( 4, _( "Delete" ) );

    menu.AppendSeparator();

    menu.Append( 5, _( "Select All" ) );

    menu.AppendSeparator();

    menu.Append( wxID_ZOOM_IN, _( "Zoom In" ) );
    menu.Append( wxID_ZOOM_OUT, _( "Zoom Out" ) );


    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case wxID_UNDO:
        m_textEditor->Undo();
        break;
    case wxID_REDO:
        m_textEditor->Redo();
        break;

    case 1:
        m_textEditor->Cut();
        break;
    case 2:
        m_textEditor->Copy();
        break;
    case 3:
        m_textEditor->Paste();
        break;
    case 4:
    {
        long from, to;
        m_textEditor->GetSelection( &from, &to );

        if( to > from )
            m_textEditor->DeleteRange( from, to );

        break;
    }

    case 5:
        m_textEditor->SelectAll();
        break;

    case wxID_ZOOM_IN:
        m_textEditor->ZoomIn();
        break;
    case wxID_ZOOM_OUT:
        m_textEditor->ZoomOut();
        break;
    }
}


void PANEL_SETUP_RULES::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    PAGED_DIALOG::GetDialog( this )->SetModified();
    m_textEditor->SearchAnchor();

    wxString rules = m_textEditor->GetText();
    int currentPos = m_textEditor->GetCurrentPos();
    int startPos = 0;

    for( int line = m_textEditor->LineFromPosition( currentPos ); line > 0; line-- )
    {
        int      lineStart = m_textEditor->PositionFromLine( line );
        wxString beginning = m_textEditor->GetTextRange( lineStart, lineStart + 10 );

        if( beginning.StartsWith( wxT( "(rule " ) ) )
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
        SEXPR_STRING,
        STRUCT_REF
    };

    auto isDisallowToken =
            []( const wxString& token ) -> bool
            {
                return token == wxT( "buried_via" )
                    || token == wxT( "graphic" )
                    || token == wxT( "hole" )
                    || token == wxT( "micro_via" )
                    || token == wxT( "pad" )
                    || token == wxT( "text" )
                    || token == wxT( "track" )
                    || token == wxT( "via" )
                    || token == wxT( "zone" );
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
            while( !sexprs.empty() && ( sexprs.top() == wxT( "assertion" )
                                     || sexprs.top() == wxT( "disallow" )
                                     || isDisallowToken( sexprs.top() )
                                     || sexprs.top() == wxT( "min_resolved_spokes" )
                                     || sexprs.top() == wxT( "zone_connection" ) ) )
            {
                sexprs.pop();
            }

            if( !sexprs.empty() )
                sexprs.pop();

            context = NONE;
        }
        else if( c == ' ' )
        {
            if( context == SEXPR_OPEN && ( partial == wxT( "constraint" )
                                        || partial == wxT( "disallow" )
                                        || partial == wxT( "layer" )
                                        || partial == wxT( "severity" ) ) )
            {
                m_textEditor->AutoCompCancel();
                sexprs.push( partial );

                partial = wxEmptyString;
                context = SEXPR_TOKEN;
                continue;
            }
            else if( partial == wxT( "disallow" )
                  || isDisallowToken( partial )
                  || partial == wxT( "min_resolved_spokes" )
                  || partial == wxT( "zone_connection" ) )
            {
                m_textEditor->AutoCompCancel();
                sexprs.push( partial );

                partial = wxEmptyString;
                context = SEXPR_TOKEN;
                continue;
            }
            else if( partial == wxT( "rule" )
                  || partial == wxT( "assertion" )
                  || partial == wxT( "condition" ) )
            {
                m_textEditor->AutoCompCancel();
                sexprs.push( partial );

                partial = wxEmptyString;
                context = SEXPR_STRING;
                continue;
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
            tokens = wxT( "rule|"
                          "version" );
        }
        else if( sexprs.top() == wxT( "rule" ) )
        {
            tokens = wxT( "condition|"
                          "constraint|"
                          "layer|"
                          "severity" );
        }
        else if( sexprs.top() == wxT( "constraint" ) )
        {
            tokens = wxT( "max|min|opt" );
        }
    }
    else if( context == SEXPR_TOKEN )
    {
        if( sexprs.empty() )
        {
            /* badly formed grammar */
        }
        else if( sexprs.top() == wxT( "constraint" ) )
        {
            tokens = wxT( "annular_width|"
                          "assertion|"
                          "clearance|"
                          "connection_width|"
                          "courtyard_clearance|"
                          "diff_pair_gap|"
                          "diff_pair_uncoupled|"
                          "disallow|"
                          "edge_clearance|"
                          "length|"
                          "hole_clearance|"
                          "hole_size|"
                          "hole_to_hole|"
                          "min_resolved_spokes|"
                          "physical_clearance|"
                          "physical_hole_clearance|"
                          "silk_clearance|"
                          "skew|"
                          "text_height|"
                          "text_thickness|"
                          "thermal_relief_gap|"
                          "thermal_spoke_width|"
                          "track_width|"
                          "via_count|"
                          "via_diameter|"
                          "zone_connection" );
        }
        else if( sexprs.top() == wxT( "disallow" ) || isDisallowToken( sexprs.top() ) )
        {
            tokens = wxT( "buried_via|"
                          "graphic|"
                          "hole|"
                          "micro_via|"
                          "pad|"
                          "text|"
                          "track|"
                          "via|"
                          "zone" );
        }
        else if( sexprs.top() == wxT( "zone_connection" ) )
        {
            tokens = wxT( "none|solid|thermal_reliefs" );
        }
        else if( sexprs.top() == wxT( "min_resolved_spokes" ) )
        {
            tokens = wxT( "0|1|2|3|4" );
        }
        else if( sexprs.top() == wxT( "layer" ) )
        {
            tokens = wxT( "inner|outer|\"x\"" );
        }
        else if( sexprs.top() == wxT( "severity" ) )
        {
            tokens = wxT( "warning|error|ignore|exclusion" );
        }
    }
    else if( context == SEXPR_STRING && !sexprs.empty()
            && ( sexprs.top() == wxT( "condition" ) || sexprs.top() == wxT( "assertion" ) ) )
    {
        m_textEditor->AddText( wxT( "\"" ) );
    }
    else if( context == STRING && !sexprs.empty()
            && ( sexprs.top() == wxT( "condition" ) || sexprs.top() == wxT( "assertion" ) ) )
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
                    // TODO: It would be nice to replace IsHiddenFromRulesEditor with a nickname
                    // system, so that two different properies don't need to be created.  This is
                    // a bigger change than I want to make right now, though.
                    if( prop->IsHiddenFromRulesEditor() )
                        continue;

                    wxString ref( prop->Name() );
                    ref.Replace( wxT( " " ), wxT( "_" ) );
                    propNames.insert( ref );
                }
            }

            for( const wxString& propName : propNames )
                tokens += wxT( "|" ) + propName;

            PCB_EXPR_BUILTIN_FUNCTIONS& functions = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

            for( const wxString& funcSig : functions.GetSignatures() )
            {
                if( !funcSig.Contains( "DEPRECATED" ) )
                    tokens += wxT( "|" ) + funcSig;
            }
        }
        else if( expr_context == STRING )
        {
            if( m_netClassRegex.Matches( last ) )
            {
                BOARD_DESIGN_SETTINGS&         bds = m_frame->GetBoard()->GetDesignSettings();
                std::shared_ptr<NET_SETTINGS>& netSettings = bds.m_NetSettings;

                for( const auto& [ name, netclass ] : netSettings->m_NetClasses )
                    tokens += wxT( "|" ) + name;
            }
            else if( m_netNameRegex.Matches( last ) )
            {
                BOARD* board = m_frame->GetBoard();

                for( const wxString& netnameCandidate : board->GetNetClassAssignmentCandidates() )
                    tokens += wxT( "|" ) + netnameCandidate;
            }
            else if( m_typeRegex.Matches( last ) )
            {
                tokens = wxT( "Bitmap|"
                              "Dimension|"
                              "Footprint|"
                              "Graphic|"
                              "Group|"
                              "Leader|"
                              "Pad|"
                              "Target|"
                              "Text|"
                              "Text Box|"
                              "Track|"
                              "Via|"
                              "Zone" );
            }
            else if( m_viaTypeRegex.Matches( last ) )
            {
                tokens = wxT( "Through|"
                              "Blind/buried|"
                              "Micro" );
            }
            else if( m_padTypeRegex.Matches( last ) )
            {
                tokens = wxT( "Through-hole|"
                              "SMD|"
                              "Edge connector|"
                              "NPTH, mechanical" );
            }
            else if( m_pinTypeRegex.Matches( last ) )
            {
                tokens = wxT( "Input|"
                              "Output|"
                              "Bidirectional|"
                              "Tri-state|"
                              "Passive|"
                              "Free|"
                              "Unspecified|"
                              "Power input|"
                              "Power output|"
                              "Open collector|"
                              "Open emitter|"
                              "Unconnected" );
            }
            else if( m_fabPropRegex.Matches( last ) )
            {
                tokens = wxT( "None|"
                              "BGA pad|"
                              "Fiducial, global to board|"
                              "Fiducial, local to footprint|"
                              "Test point pad|"
                              "Heatsink pad|"
                              "Castellated pad" );
            }
            else if( m_shapeRegex.Matches( last ) )
            {
                tokens = wxT( "Segment|"
                              "Rectangle|"
                              "Arc|"
                              "Circle|"
                              "Polygon|"
                              "Bezier" );
            }
        }
    }

    if( !tokens.IsEmpty() )
        m_scintillaTricks->DoAutocomplete( partial, wxSplit( tokens, '|' ) );
}


void PANEL_SETUP_RULES::OnCompile( wxCommandEvent& event )
{
    m_errorsReport->Clear();

    try
    {
        std::vector<std::shared_ptr<DRC_RULE>> dummyRules;

        DRC_RULES_PARSER parser( m_textEditor->GetText(), _( "DRC rules" ) );

        parser.Parse( dummyRules, m_errorsReport );
    }
    catch( PARSE_ERROR& pe )
    {
        wxString msg = wxString::Format( wxT( "%s <a href='%d:%d'>%s</a>%s" ),
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

            m_textEditor->EmptyUndoBuffer();

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
    msg.Replace( wxT( "Ctrl+" ), wxT( "Cmd+" ) );
#endif

    m_helpWindow = new HTML_MESSAGE_BOX( nullptr, _( "Syntax Help" ) );
    m_helpWindow->SetDialogSizeInDU( 320, 320 );

    wxString html_txt;
    ConvertMarkdown2Html( wxGetTranslation( msg ), html_txt );
    m_helpWindow->AddHTML_Text( html_txt );

    m_helpWindow->ShowModeless();
}
