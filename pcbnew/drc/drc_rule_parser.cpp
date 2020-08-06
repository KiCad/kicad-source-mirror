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

#include <drc/drc_rule_parser.h>
#include <drc_rules_lexer.h>
#include <pcb_expr_evaluator.h>
#include <reporter.h>

using namespace DRCRULE_T;


DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, const wxString& aSource,
                                    const wxString& aSourceDescr ) :
        DRC_RULES_LEXER( aSource.ToStdString(), aSourceDescr ),
        m_board( aBoard ),
        m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
{
}


DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, FILE* aFile, const wxString& aFilename ) :
        DRC_RULES_LEXER( aFile, aFilename ),
        m_board( aBoard ),
        m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
{
}


void DRC_RULES_PARSER::reportError( const wxString& aMessage )
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

    for( T token = NextTok();  token != T_EOF;  token = NextTok() )
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
            msg.Printf( _( "Unrecognized item '%s'.| Expected 'rule' or 'version'." ), FromUTF8() );
            reportError( msg );
            parseUnknown();
        }
    }

    if( !m_reporter->HasMessage() )
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

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        switch( token )
        {
        case T_disallow:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing disallowed type.|  Expected 'track', 'via', 'micro_via', "
                                "'blind_via', 'pad', 'zone', 'text', 'graphic' or 'hole'." ) );
                break;
            }
            else if( (int) token == DSN_STRING )
            {
                token = GetCurStrAsToken();
            }

            switch( token )
            {
            case T_track:      rule->m_DisallowFlags |= DISALLOW_TRACKS;     break;
            case T_via:        rule->m_DisallowFlags |= DISALLOW_VIAS;       break;
            case T_micro_via:  rule->m_DisallowFlags |= DISALLOW_MICRO_VIAS; break;
            case T_buried_via: rule->m_DisallowFlags |= DISALLOW_BB_VIAS;    break;
            case T_pad:        rule->m_DisallowFlags |= DISALLOW_PADS;       break;
            case T_zone:       rule->m_DisallowFlags |= DISALLOW_ZONES;      break;
            case T_text:       rule->m_DisallowFlags |= DISALLOW_TEXTS;      break;
            case T_graphic:    rule->m_DisallowFlags |= DISALLOW_GRAPHICS;   break;
            case T_hole:       rule->m_DisallowFlags |= DISALLOW_HOLES;      break;
            case T_footprint:  rule->m_DisallowFlags |= DISALLOW_FOOTPRINTS; break;
            default:
                msg.Printf( _( "Unrecognized item '%s'.|  Expected 'track', 'via', 'micro_via', "
                               "'blind_via', 'pad', 'zone', 'text', 'graphic' or 'hole'." ),
                            FromUTF8() );
                reportError( msg );
            }

            rule->m_ConstraintFlags = DISALLOW_CONSTRAINT;

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;

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
                rule->m_Condition.m_Expression = FromUTF8();
                rule->m_Condition.Compile( m_reporter, CurLineNumber(), CurOffset() );
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
            rule->m_LayerCondition = parseLayer();
            break;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected 'constraint', 'condition' or "
                           "'disallow'." ),
                        FromUTF8() );
            reportError( msg );
            parseUnknown();
        }
    }

    return rule;
}


void DRC_RULES_PARSER::parseConstraint( DRC_RULE* aRule )
{
    T        token;
    int      constraintType = 0;
    int      value;
    wxString msg;

    token = NextTok();

    if( (int) token == DSN_RIGHT )
    {
        reportError( _( "Missing constraint type.|  Expected 'clearance', 'track_width', "
                        "'annulus_width' or 'hole'." ) );
        return;
    }

    switch( token )
    {
    case T_clearance:     constraintType = CLEARANCE_CONSTRAINT; break;
    case T_track_width:   constraintType = TRACK_CONSTRAINT;     break;
    case T_annulus_width: constraintType = ANNULUS_CONSTRAINT;   break;
    case T_hole:          constraintType = HOLE_CONSTRAINT;      break;
    default:
        msg.Printf( _( "Unrecognized item '%s'.| Expected 'clearance', 'track_width', "
                       "'annulus_width' or 'hole'." ),
                    FromUTF8() );
        reportError( msg );
    }

    aRule->m_ConstraintFlags |= constraintType;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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

            switch( constraintType )
            {
            case CLEARANCE_CONSTRAINT: aRule->m_Clearance.Min = value;       break;
            case TRACK_CONSTRAINT:     aRule->m_TrackConstraint.Min = value; break;
            case ANNULUS_CONSTRAINT:   aRule->m_MinAnnulusWidth = value;     break;
            case HOLE_CONSTRAINT:      aRule->m_MinHole = value;             break;
            }

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

            switch( constraintType )
            {
            case CLEARANCE_CONSTRAINT: aRule->m_Clearance.Max = value;       break;
            case TRACK_CONSTRAINT:     aRule->m_TrackConstraint.Max = value; break;
            }

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

            switch( constraintType )
            {
            case CLEARANCE_CONSTRAINT: aRule->m_Clearance.Opt = value;       break;
            case TRACK_CONSTRAINT:     aRule->m_TrackConstraint.Opt = value; break;
            }

            if( (int) NextTok() != DSN_RIGHT )
            {
                reportError( wxString::Format( _( "Unrecognized item '%s'." ), FromUTF8() ) );
                parseUnknown();
            }

            break;

        default:
            msg.Printf( _( "Unrecognized item '%s'.| Expected 'min', 'max' or 'opt'." ),
                        FromUTF8() );
            reportError( msg );
            parseUnknown();
        }
    }
}


void DRC_RULES_PARSER::parseValueWithUnits( const wxString& aExpr, int& aResult )
{
    PCB_EXPR_EVALUATOR evaluator( m_reporter, CurLineNumber(), CurOffset() );

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
