/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see change_log.txt for contributors.
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


#include <board.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule_condition.h>
#include <drc_rules_lexer.h>
#include <pcb_expr_evaluator.h>
#include <reporter.h>

using namespace DRCRULE_T;


DRC_RULES_PARSER::DRC_RULES_PARSER( const wxString& aSource, const wxString& aSourceDescr ) :
        DRC_RULES_LEXER( aSource.ToStdString(), aSourceDescr ),
        m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
{
}


DRC_RULES_PARSER::DRC_RULES_PARSER( FILE* aFile, const wxString& aFilename ) :
        DRC_RULES_LEXER( aFile, aFilename ),
        m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
{
}


void DRC_RULES_PARSER::reportError( const wxString& aMessage )
{
    wxString rest;
    wxString first = aMessage.BeforeFirst( '|', &rest );

    if( m_reporter )
    {
        wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ), CurLineNumber(),
                                         CurOffset(), first, rest );

        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
    }
    else
    {
        wxString msg = wxString::Format( _( "ERROR: %s%s" ), first, rest );

        THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }
}


void DRC_RULES_PARSER::parseUnknown()
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


void DRC_RULES_PARSER::Parse( std::vector<DRC_RULE*>& aRules, REPORTER* aReporter )
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
                m_requiredVersion = (int)strtol( CurText(), nullptr, 10 );
                m_tooRecent = ( m_requiredVersion > DRC_RULE_FILE_VERSION );
                token = NextTok();
            }
            else
            {
                msg.Printf( _( "Unrecognized item '%s'.| Expected version number." ),
                            FromUTF8() );
                reportError( msg );
            }

            if( (int) token != DSN_RIGHT )
            {
                msg.Printf( _( "Unrecognized item '%s'." ),
                            FromUTF8() );
                reportError( msg );
                parseUnknown();
            }

            break;

        case T_rule:
            aRules.push_back( parseDRC_RULE() );
            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            break;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ), FromUTF8(),
                        "'rule', 'version'" );
            reportError( msg );
            parseUnknown();
        }
    }

    if( m_reporter && !m_reporter->HasMessage() )
        m_reporter->Report( _( "No errors found." ), RPT_SEVERITY_INFO );

    m_reporter = nullptr;
}


DRC_RULE* DRC_RULES_PARSER::parseDRC_RULE()
{
    DRC_RULE* rule = new DRC_RULE();
    T         token = NextTok();
    wxString  msg;

    if( !IsSymbol( token ) )
        reportError( _( "Missing rule name." ) );

    rule->m_Name = FromUTF8();

    for( token = NextTok(); token != T_RIGHT && token != T_EOF; token = NextTok() )
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
                rule->m_Condition = new DRC_RULE_CONDITION( FromUTF8() );
                rule->m_Condition->Compile( m_reporter, CurLineNumber(), CurOffset() );
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
            rule->m_LayerSource = FromUTF8();
            rule->m_LayerCondition = parseLayer();
            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            return rule;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ), FromUTF8(),
                           "'constraint', 'condition', 'disallow'" );
            reportError( msg );
            parseUnknown();
        }
    }

    if( (int) CurTok() != DSN_RIGHT )
        reportError( _( "Missing ')'." ) );

    return rule;
}


void DRC_RULES_PARSER::parseConstraint( DRC_RULE* aRule )
{
    DRC_CONSTRAINT constraint;

    int       value;
    wxString  msg;

    T token = NextTok();

    if( (int) token == DSN_RIGHT || token == T_EOF )
    {
        msg.Printf( _( "Missing constraint type.|  Expected %s." ),
                    "'clearance', 'hole_clearance', 'edge_clearance', 'hole', 'hole_to_hole', "
                    "'courtyard_clearance', 'silk_clearance', 'track_width', 'annular_width', "
                    "'disallow', 'length', 'skew', 'via_count', 'diff_pair_gap' or "
                    "'diff_pair_uncoupled'" );
        reportError( msg );
        return;
    }

    switch( token )
    {
    case T_clearance:           constraint.m_Type = CLEARANCE_CONSTRAINT;                break;
    case T_hole_clearance:      constraint.m_Type = HOLE_CLEARANCE_CONSTRAINT;           break;
    case T_edge_clearance:      constraint.m_Type = EDGE_CLEARANCE_CONSTRAINT;           break;
    case T_hole:  // legacy token
    case T_hole_size:           constraint.m_Type = HOLE_SIZE_CONSTRAINT;                break;
    case T_hole_to_hole:        constraint.m_Type = HOLE_TO_HOLE_CONSTRAINT;             break;
    case T_courtyard_clearance: constraint.m_Type = COURTYARD_CLEARANCE_CONSTRAINT;      break;
    case T_silk_clearance:      constraint.m_Type = SILK_CLEARANCE_CONSTRAINT;           break;
    case T_track_width:         constraint.m_Type = TRACK_WIDTH_CONSTRAINT;              break;
    case T_annular_width:       constraint.m_Type = ANNULAR_WIDTH_CONSTRAINT;            break;
    case T_disallow:            constraint.m_Type = DISALLOW_CONSTRAINT;                 break;
    case T_length:              constraint.m_Type = LENGTH_CONSTRAINT;                   break;
    case T_skew:                constraint.m_Type = SKEW_CONSTRAINT;                     break;
    case T_via_count:           constraint.m_Type = VIA_COUNT_CONSTRAINT;                break;
    case T_diff_pair_gap:       constraint.m_Type = DIFF_PAIR_GAP_CONSTRAINT;            break;
    case T_diff_pair_uncoupled: constraint.m_Type = DIFF_PAIR_MAX_UNCOUPLED_CONSTRAINT;  break;
    default:
        msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ), FromUTF8(),
                    "'clearance', 'hole_clearance', 'edge_clearance', 'hole_size', hole_to_hole',"
                    "'courtyard_clearance', 'silk_clearance', 'track_width', 'annular_width', "
                    "'disallow', 'length', 'skew', 'diff_pair_gap' or 'diff_pair_uncoupled'." );
        reportError( msg );
    }

    if( constraint.m_Type == DISALLOW_CONSTRAINT )
    {
        for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
        {
            if( (int) token == DSN_STRING )
                token = GetCurStrAsToken();

            switch( token )
            {
            case T_track:      constraint.m_DisallowFlags |= DRC_DISALLOW_TRACKS;     break;
            case T_via:        constraint.m_DisallowFlags |= DRC_DISALLOW_VIAS;       break;
            case T_micro_via:  constraint.m_DisallowFlags |= DRC_DISALLOW_MICRO_VIAS; break;
            case T_buried_via: constraint.m_DisallowFlags |= DRC_DISALLOW_BB_VIAS;    break;
            case T_pad:        constraint.m_DisallowFlags |= DRC_DISALLOW_PADS;       break;
            case T_zone:       constraint.m_DisallowFlags |= DRC_DISALLOW_ZONES;      break;
            case T_text:       constraint.m_DisallowFlags |= DRC_DISALLOW_TEXTS;      break;
            case T_graphic:    constraint.m_DisallowFlags |= DRC_DISALLOW_GRAPHICS;   break;
            case T_hole:       constraint.m_DisallowFlags |= DRC_DISALLOW_HOLES;      break;
            case T_footprint:  constraint.m_DisallowFlags |= DRC_DISALLOW_FOOTPRINTS; break;

            case T_EOF:
                reportError( _( "Missing ')'." ) );
                return;

            default:
                msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ), FromUTF8(),
                            "'track', 'via', 'micro_via', 'buried_via', 'pad', 'zone', 'text', "
                            "'graphic', 'hole' or 'footprint'." );
                reportError( msg );
                break;
            }
        }

        if( (int) CurTok() != DSN_RIGHT )
            reportError( _( "Missing ')'." ) );

        aRule->AddConstraint( constraint );
        return;
    }

    for( token = NextTok(); token != T_RIGHT && token != T_EOF; token = NextTok() )
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
            constraint.m_Value.SetMin( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ),
                                               FromUTF8() ) );
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
            constraint.m_Value.SetMax( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ),
                                               FromUTF8() ) );
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
            constraint.m_Value.SetOpt( value );

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ),
                                               FromUTF8() ) );
                parseUnknown();
            }

            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            return;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ),
                        FromUTF8(),
                        "'min', 'max', 'opt'" );
            reportError( msg );
            parseUnknown();
        }
    }

    if( (int) CurTok() != DSN_RIGHT )
        reportError( _( "Missing ')'." ) );

    aRule->AddConstraint( constraint );
}


void DRC_RULES_PARSER::parseValueWithUnits( const wxString& aExpr, int& aResult )
{
    auto errorHandler = [&]( const wxString& aMessage, int aOffset )
    {
        wxString rest;
        wxString first = aMessage.BeforeFirst( '|', &rest );

        if( m_reporter )
        {
            wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                             CurLineNumber(), CurOffset() + aOffset, first, rest );

            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }
        else
        {
            wxString msg = wxString::Format( _( "ERROR: %s%s" ), first, rest );

            THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(),
                               CurOffset() + aOffset );
        }
    };

    PCB_EXPR_EVALUATOR evaluator;
    evaluator.SetErrorCallback( errorHandler );

    evaluator.Evaluate( aExpr );
    aResult = evaluator.Result();
}


LSET DRC_RULES_PARSER::parseLayer()
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
        wxPGChoices& layerMap = ENUM_MAP<PCB_LAYER_ID>::Instance().Choices();

        for( unsigned ii = 0; ii < layerMap.GetCount(); ++ii )
        {
            wxPGChoiceEntry& entry = layerMap[ii];

            if( entry.GetText().Matches( layerName ) )
                retVal.set( ToLAYER_ID( entry.GetValue() ) );
        }

        if( !retVal.any() )
        {
            reportError( wxString::Format( _( "Unrecognized layer '%s'." ),
                                           layerName ) );
            retVal.set( Rescue );
        }
    }

    if( (int) NextTok() != DSN_RIGHT )
    {
        reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
        parseUnknown();
    }

    return retVal;
}
