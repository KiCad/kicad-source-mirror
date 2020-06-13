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


#include <class_board.h>
#include <class_board_item.h>

#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_rule_parser.h>
#include <drc_proto/drc_rules_lexer.h>

#include <fctsys.h>

#include <pcb_expr_evaluator.h>

using namespace DRCRULE_T;


test::DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, FILE* aFile, const wxString& aFilename )
        : DRC_RULES_LEXER( aFile, aFilename ),
          m_board( aBoard ),
          m_requiredVersion( 0 ),
          m_tooRecent( false )
{
    for( LAYER_NUM layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );
        m_layerMap[untranslated] = PCB_LAYER_ID( layer );
    }
}


void test::DRC_RULES_PARSER::Parse(
        std::vector<test::DRC_RULE_CONDITION*>& aConditions, std::vector<test::DRC_RULE*>& aRules )
{
    bool haveVersion = false;

    for( T token = NextTok(); token != T_EOF; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( !haveVersion && token != T_version )
            Expecting( "version" );

        switch( token )
        {
        case T_version:
            NeedNUMBER( "version" );
            m_requiredVersion = (int) strtol( CurText(), NULL, 10 );
            m_tooRecent       = ( m_requiredVersion > DRC_RULE_FILE_VERSION );
            haveVersion       = true;
            NeedRIGHT();
            break;

        case T_condition:
            aConditions.push_back( parseCONDITION() );
            break;

        case T_rule:
            aRules.push_back( parseRULE() );
            break;

        default:
            Expecting( "condition or rule" );
        }
    }


#if 0
    // Hook up the selectors to their rules
    std::map<wxString, DRC_RULE*> ruleMap;

    for( DRC_RULE* rule : aRules )
        ruleMap[ rule->m_Name ] = rule;

    for( const std::pair<DRC_RULE_CONDITION*, wxString>& entry : conditionsRules )
    {
        if( ruleMap.count( entry.second ) )
        {
            entry.first->m_Rule = ruleMap[ entry.second ];
        }
        else
        {
            wxString errText = wxString::Format( _( "Rule \"%s\" not found." ), entry.second );
            THROW_PARSE_ERROR( errText, CurSource(), "", 0, 0 );
        }
    }
#endif
}


test::DRC_RULE_CONDITION* test::DRC_RULES_PARSER::parseCONDITION()
{
    test::DRC_RULE_CONDITION* cond = new test::DRC_RULE_CONDITION();
    T                         token;

    printf( "parsecondition\n" );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        printf( "Do token xxx %d '%s'\n", token, (const char*) FromUTF8().c_str() );

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
    int val;

    if( !IsSymbol( token ) )
        Expecting( "rule name" );

    rule->m_Name = FromUTF8();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        //printf( "Do token xxx %d '%s'\n", token, (const char*) FromUTF8().c_str() );

        switch( token )
        {
        case T_type:
            NeedSYMBOL();
            rule->m_TestProviderName = FromUTF8();
            break;

        case T_min:
            NeedSYMBOL();
            parseValueWithUnits ( FromUTF8(), val );
            rule->m_Constraint.m_Value.SetMin( val );
            break;

        case T_opt:
            NeedSYMBOL();
            parseValueWithUnits ( FromUTF8(), val );
            rule->m_Constraint.m_Value.SetOpt( val );
            break;
        
        case T_max:
            NeedSYMBOL();
            parseValueWithUnits ( FromUTF8(), val );
            rule->m_Constraint.m_Value.SetMax( val );
            break;
        
        case T_allow:
            rule->m_Constraint.m_Allow = parseInt("allowed");
            break;

        case T_enable:
            rule->m_Enabled = parseInt("enabled");
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


            break;

        case T_priority:
            rule->m_Priority = parseInt("priotity");
            break;



        default:
            Expecting( "type, min, opt, max, allow, enable, priority or severity" );
            break;
        }

        NeedRIGHT();
    }

    return rule;
}


void test::DRC_RULES_PARSER::parseValueWithUnits( const wxString& aExpr, int& aResult )
{
    PCB_EXPR_EVALUATOR evaluator;

    bool ok = evaluator.Evaluate( aExpr );

    if( !ok )
    {
        auto err = evaluator.GetErrorString();
        printf( "eval error: '%s'\n", (const char*) err.c_str() );

        THROW_PARSE_ERROR( err, "", "", 0, 0 );
    }

    aResult = evaluator.Result();
    printf("parseValueWithUnits: value %d\n", aResult );
};
