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
#include <drc_rules_lexer.h>
#include <drc_proto/drc_engine.h> // drc_dbg
#include <pcb_expr_evaluator.h>
#include <reporter.h>

using namespace DRCRULE_T;

test::DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, const wxString& aSource,
                                    const wxString& aSourceDescr ) :
        DRC_RULES_LEXER( aSource.ToStdString(), aSourceDescr ),
          m_board( aBoard ),
          m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
{
}


test::DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, FILE* aFile, const wxString& aFilename ) :
        DRC_RULES_LEXER( aFile, aFilename ),
        m_board( aBoard ),
        m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
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


void test::DRC_RULES_PARSER::Parse( std::vector<DRC_RULE*>& aRules, REPORTER* aReporter )
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

        case T_rule:
            aRules.push_back( parseDRC_RULE() );
            break;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ),
                        FromUTF8(), "'rule', 'version'" );
            reportError( msg );
            parseUnknown();
        }
    }

    if( !m_reporter->HasMessage() )
        m_reporter->Report( _( "No errors found." ), RPT_SEVERITY_INFO );

    m_reporter = nullptr;
}


test::DRC_RULE* test::DRC_RULES_PARSER::parseDRC_RULE()
{
    DRC_RULE* rule = new DRC_RULE();
    T         token = NextTok();
    wxString  msg;

    if( !IsSymbol( token ) )
        reportError( _( "Missing rule name." ) );

    rule->m_Name = FromUTF8();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        switch( token )
        {
        case T_constraint:
            parseConstraint( rule );
            break;

        case T_condition:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing condition expression." ) );
                break;
            }

            if( IsSymbol( token ) )
            {
                auto condition = new DRC_RULE_CONDITION;

                condition->SetExpression( FromUTF8() );
                rule->SetCondition( condition );
            }
            else
            {
                msg.Printf( _( "Unrecognized item '%s'.| Expected quoted expression." ),
                            FromUTF8() );
                reportError( msg );
            }

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;

        case T_layer:
            rule->SetLayerCondition( parseLayer() );
            break;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ),
                           FromUTF8(),
                           "'constraint', 'condition', 'disallow'" );
            reportError( msg );
            parseUnknown();
        }
    }

    return rule;
}


void test::DRC_RULES_PARSER::parseConstraint( DRC_RULE* aRule )
{
    DRC_CONSTRAINT constraint;

    int       value;
    wxString  msg;

    T token = NextTok();

    if( (int) token == DSN_RIGHT )
    {
        msg.Printf( _( "Missing constraint type.|  Expected %s." ),
                        "'clearance', 'track_width', 'annulus_width', 'hole', 'disallow'" );
        reportError( msg );
        return;
    }
    
    switch( token )
    {
    case T_clearance:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_CLEARANCE; break;
    case T_hole_clearance:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_HOLE_CLEARANCE; break;
    case T_edge_clearance:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE; break;
    case T_hole:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_HOLE_SIZE; break;
    case T_courtyard_clearance:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_COURTYARD_CLEARANCE; break;
    case T_silk_to_pad:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_SILK_TO_PAD; break;
    case T_silk_to_silk:     constraint.m_Type = test::DRC_CONSTRAINT_TYPE_SILK_TO_SILK; break;
    case T_track_width:   constraint.m_Type = test::DRC_CONSTRAINT_TYPE_TRACK_WIDTH;      break;
    case T_annulus_width: constraint.m_Type = test::DRC_CONSTRAINT_TYPE_ANNULUS_WIDTH;   break;
    case T_disallow:      constraint.m_Type = test::DRC_CONSTRAINT_TYPE_DISALLOW;  break;
    default:
        msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ),
                    FromUTF8(),
                    "'clearance', 'track_width', 'annulus_width', 'hole', 'disallow'."
                   );
        reportError( msg );
    }

    if( constraint.m_Type == DRC_CONSTRAINT_TYPE_DISALLOW )
    {
        for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
        {
            if( (int) token == DSN_STRING )
                token = GetCurStrAsToken();

            switch( token )
            {
            case T_track:      constraint.m_DisallowFlags |= DISALLOW_TRACKS;     break;
            case T_via:        constraint.m_DisallowFlags |= DISALLOW_VIAS;       break;
            case T_micro_via:  constraint.m_DisallowFlags |= DISALLOW_MICRO_VIAS; break;
            case T_buried_via: constraint.m_DisallowFlags |= DISALLOW_BB_VIAS;    break;
            case T_pad:        constraint.m_DisallowFlags |= DISALLOW_PADS;       break;
            case T_zone:       constraint.m_DisallowFlags |= DISALLOW_ZONES;      break;
            case T_text:       constraint.m_DisallowFlags |= DISALLOW_TEXTS;      break;
            case T_graphic:    constraint.m_DisallowFlags |= DISALLOW_GRAPHICS;   break;
            case T_hole:       constraint.m_DisallowFlags |= DISALLOW_HOLES;      break;
            case T_footprint:  constraint.m_DisallowFlags |= DISALLOW_FOOTPRINTS; break;
            default:
                msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ),
                            FromUTF8(),
                            "'track', 'via', 'micro_via', "
                            "'blind_via', 'pad', 'zone', 'text', 'graphic', 'hole'."
                            );
                reportError( msg );
                parseUnknown();
            }
        }

        return;
    }

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        switch( token )
        {
        case T_min:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing min value." ) );
                break;
            }

            parseValueWithUnits( FromUTF8(), value );
            constraint.Value().SetMin( value );

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
            constraint.Value().SetMax( value );

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
            constraint.Value().SetOpt( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;
        
        /* fixme: bring these back?

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
        */
        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ),
                        FromUTF8(), "'min', 'max', 'opt'" );
            reportError( msg );
            parseUnknown();
        }
    }
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
