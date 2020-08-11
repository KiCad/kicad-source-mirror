/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <class_board.h>
#include <class_board_item.h>

#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_rule_parser.h>
#include <drc_proto/drc_rules_lexer.h>
#include <drc_proto/drc_engine.h> // drc_dbg
#include <pcb_expr_evaluator.h>
#include <reporter.h>

using namespace DRCRULEPROTO_T;

test::DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, FILE* aFile, const wxString& aFilename )
        : DRC_RULES_PROTO_LEXER( aFile, aFilename ),
          m_board( aBoard ),
          m_requiredVersion( 0 ),
          m_tooRecent( false )
{
}


void test::DRC_RULES_PARSER::reportError( const wxString& aMessage )
{
    wxString rest;
    wxString first = aMessage.BeforeFirst( '|', &rest );
    wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                     CurLineNumber(),
                                     CurOffset(),
                                     first,
                                     rest );

    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
}


void test::DRC_RULES_PARSER::parseUnknown()
{
    int depth = 1;

    for( T token = NextTok();  token != T_EOF;  token = NextTok() )
    {
        if( token == T_LEFT )
            depth++;

        if( token == T_RIGHT )
        {
            if( --depth == 0 )
                break;
        }
    }
}


void test::DRC_RULES_PARSER::Parse( std::vector<test::DRC_RULE_CONDITION*>& aConditions,
                                    std::vector<test::DRC_RULE*>& aRules,
                                    REPORTER* aReporter )
{
    bool     haveVersion = false;
    wxString msg;

    m_reporter = aReporter;

    for( T token = NextTok(); token != T_EOF; token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        if( !haveVersion && token != T_version )
        {
            reportError( _( "Missing version statement." ) );
            haveVersion = true;     // don't keep on reporting it
        }

        switch( token )
        {
        case T_version:
            haveVersion = true;
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing version number." ) );
                break;
            }

            if( (int) token == DSN_NUMBER )
            {
                m_requiredVersion = (int)strtol( CurText(), NULL, 10 );
                m_tooRecent = ( m_requiredVersion > DRC_RULE_FILE_VERSION );
                token = NextTok();
            }
            else
            {
                msg.Printf( _( "Unrecognized item '%s'.| Expected version number" ), FromUTF8() );
                reportError( msg );
            }

            if( (int) token != DSN_RIGHT )
            {
                msg.Printf( _( "Unrecognized item '%s'." ), FromUTF8() );
                reportError( msg );
                parseUnknown();
            }

            break;

        case T_condition:
            aConditions.push_back( parseCONDITION() );
            break;

        case T_rule:
        {
            auto rule = parseRULE();
            drc_dbg(0, "Parsed rule: '%s' type '%s'\n", (const char*) rule->GetName().c_str(), (const char*) rule->GetTestProviderName().c_str() );
            aRules.push_back( rule );
            break;
        }

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected 'rule', 'condition' or 'version'." ),
                        FromUTF8() );
            reportError( msg );
            parseUnknown();
        }
    }
}


test::DRC_RULE_CONDITION* test::DRC_RULES_PARSER::parseCONDITION()
{
    test::DRC_RULE_CONDITION* cond = new test::DRC_RULE_CONDITION();
    T                         token;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        //printf( "Do token xxx %d '%s'\n", token, (const char*) FromUTF8().c_str() );

        // TODO: Needs updating to report errors through REPORTER

        switch( token )
        {
        case T_expression:
            NeedSYMBOL();
            cond->m_Expression = FromUTF8();
            break;

        case T_rule:
            NeedSYMBOL();
            cond->m_TargetRuleName = FromUTF8();
            break;

        default:
            Expecting( "rule or expression" );
            break;
        }
        NeedRIGHT();
    }

    //NeedRIGHT()

    return cond;
}


test::DRC_RULE* test::DRC_RULES_PARSER::parseRULE()
{
    DRC_RULE* rule  = new DRC_RULE();
    T         token = NextTok();
    int       value;
    wxString  msg;

    if( !IsSymbol( token ) )
        reportError( _( "Missing rule name." ) );

    rule->m_Priority = 0;
    rule->m_Enabled = true;
    
    rule->m_Name = FromUTF8();

    printf("parseRule '%s'\n", (const char *) rule->m_Name.c_str() );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        //printf( "Do token xxx %d '%s'\n", token, (const char*) FromUTF8().c_str() );

        switch( token )
        {
        case T_type:
            // TODO: I assume this won't be in the final impl and so doesn't need converting
            //       to new error reporting framework?
            NeedSYMBOL();
            rule->m_TestProviderName = FromUTF8();
            NeedRIGHT();
            break;

        case T_min:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing min value." ) );
                break;
            }

            parseValueWithUnits( FromUTF8(), value );
            rule->m_Constraint.m_Value.SetMin( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;

        case T_opt:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing opt value." ) );
                break;
            }

            parseValueWithUnits( FromUTF8(), value );
            rule->m_Constraint.m_Value.SetOpt( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;
        
        case T_max:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing max value." ) );
                break;
            }

            parseValueWithUnits( FromUTF8(), value );
            rule->m_Constraint.m_Value.SetMax( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;
        
        case T_allow:
            rule->m_Constraint.m_Allow = parseInt("allowed");
            NeedRIGHT();
            break;

        case T_enable:
            rule->m_Enabled = parseInt("enabled");
            NeedRIGHT();
            break;

        case T_severity:
            token = NextTok();
            switch( token )
            {
                case T_error:
                case T_warning:
                case T_ignore: break; // fixme
                default:
                    Expecting( "error, warning or ignore" );
                    break;
            }
            NeedRIGHT();


            break;

        case T_priority:
            rule->m_Priority = parseInt("priotity");
            NeedRIGHT();
            break;



        default:
            // TODO: reconcile
            //Expecting( "type, min, opt, max, allow, enable, priority or severity" );
            msg.Printf( _( "Unrecognized item '%s'.| Expected 'min', 'max' or 'opt'." ),
                        FromUTF8() );
            reportError( msg );
            parseUnknown();
            break;
        }

        
    }

    return rule;
}


void test::DRC_RULES_PARSER::parseValueWithUnits( const wxString& aExpr, int& aResult )
{
    PCB_EXPR_EVALUATOR evaluator;

    evaluator.Evaluate( aExpr );

    if( evaluator.IsErrorPending() )
    {
        auto err = evaluator.GetError();
        wxString str;
        str.Printf( "Error: %s (line %d, offset %d)", err.message, CurLineNumber(), err.srcPos + CurOffset() );

        m_reporter->Report( str, RPT_SEVERITY_ERROR );
        return;
    }

    aResult = evaluator.Result();
};


LSET test::DRC_RULES_PARSER::parseLayer()
{
    LSET retVal;
    int  token = NextTok();

    if( (int) token == DSN_RIGHT )
    {
        reportError( _( "Missing layer name or type." ) );
        return LSET::AllCuMask();
    }
    else if( token == T_outer )
    {
        retVal = LSET::ExternalCuMask();
    }
    else if( token == T_inner )
    {
        retVal = LSET::InternalCuMask();
    }
    else
    {
        wxString     layerName = FromUTF8();
        PCB_LAYER_ID layer = ENUM_MAP<PCB_LAYER_ID>::Instance().ToEnum( layerName );

        if( layer == UNDEFINED_LAYER )
            reportError( wxString::Format( _( "Unrecognized layer '%s' " ), layerName ) );

        retVal.set( layer );
    }

    if( (int) NextTok() != DSN_RIGHT )
    {
        reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
        parseUnknown();
    }

    return retVal;
}
