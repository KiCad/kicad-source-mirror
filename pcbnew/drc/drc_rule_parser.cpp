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
#include <drc/drc_rule_parser.h>
#include <drc_rules_lexer.h>
#include <class_board.h>
#include <class_board_item.h>
#include <pcb_expr_evaluator.h>

using namespace DRCRULE_T;


DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, const wxString& aSource,
                                    const wxString& aSourceDescr ) :
        DRC_RULES_LEXER( aSource.ToStdString(), aSourceDescr ),
        m_board( aBoard ),
        m_requiredVersion( 0 ),
        m_tooRecent( false )
{
    initLayerMap();
}


DRC_RULES_PARSER::DRC_RULES_PARSER( BOARD* aBoard, FILE* aFile, const wxString& aFilename ) :
        DRC_RULES_LEXER( aFile, aFilename ),
        m_board( aBoard ),
        m_requiredVersion( 0 ),
        m_tooRecent( false )
{
    initLayerMap();
}


void DRC_RULES_PARSER::initLayerMap()
{
    for( LAYER_NUM layer = 0;  layer < PCB_LAYER_ID_COUNT;  ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );
        m_layerMap[ untranslated ] = PCB_LAYER_ID( layer );

        std::string userName = m_board->GetLayerName( PCB_LAYER_ID( layer ) ).ToStdString();
        m_layerMap[ userName ] = PCB_LAYER_ID( layer );
    }
}


void DRC_RULES_PARSER::Parse( std::vector<DRC_RULE*>& aRules )
{
    bool haveVersion = false;

    for( T token = NextTok();  token != T_EOF;  token = NextTok() )
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
            m_requiredVersion = (int)strtol( CurText(), NULL, 10 );
            m_tooRecent = ( m_requiredVersion > DRC_RULE_FILE_VERSION );
            haveVersion = true;
            NeedRIGHT();
            break;

        case T_rule:
            aRules.push_back( parseDRC_RULE() );
            break;

        default:
            Expecting( "selector or rule" );
        }
    }
}


DRC_RULE* DRC_RULES_PARSER::parseDRC_RULE()
{
    DRC_RULE* rule = new DRC_RULE();
    T         token = NextTok();

    if( !IsSymbol( token ) )
        Expecting( "rule name" );

    rule->m_Name = FromUTF8();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_disallow:
            switch( NextTok() )
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
            default: Expecting( "track, via, micro_via, blind_via, pad, zone, text, "
                                "graphic, or hole" );
            }

            rule->m_ConstraintFlags = DISALLOW_CONSTRAINT;
            NeedRIGHT();
            break;

        case T_constraint:
            parseConstraint( rule );
            break;

        case T_condition:
            NeedSYMBOL();
            rule->m_Condition.m_Expression = FromUTF8();

            if( !rule->m_Condition.Compile() )
            {
                LIBEVAL::ERROR_STATUS error = rule->m_Condition.GetCompilationError();
                THROW_PARSE_ERROR( error.message, CurSource(), CurLine(), CurLineNumber(),
                                   CurOffset() + error.srcPos );
            }

            NeedRIGHT();
            break;

        default:
            Expecting( "disallow, constraint or condition" );
        }
    }

    return rule;
}


void DRC_RULES_PARSER::parseConstraint( DRC_RULE* aRule )
{
    T   token;
    int constraintType;
    int value;

    switch( NextTok() )
    {
    case T_clearance:     constraintType = CLEARANCE_CONSTRAINT; break;
    case T_track_width:   constraintType = TRACK_CONSTRAINT;     break;
    case T_annulus_width: constraintType = ANNULUS_CONSTRAINT;   break;
    case T_hole:          constraintType = HOLE_CONSTRAINT;      break;
    default: Expecting( "clearance, track_width, annulus_width, or hole" ); return;
    }

    aRule->m_ConstraintFlags |= constraintType;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_min:
            NextTok();
            parseValueWithUnits( FromUTF8(), value );

            switch( constraintType )
            {
            case CLEARANCE_CONSTRAINT: aRule->m_Clearance.Min = value;       break;
            case TRACK_CONSTRAINT:     aRule->m_TrackConstraint.Min = value; break;
            case ANNULUS_CONSTRAINT:   aRule->m_MinAnnulusWidth = value;     break;
            case HOLE_CONSTRAINT:      aRule->m_MinHole = value;             break;
            }

            NeedRIGHT();
            break;

        case T_max:
            NextTok();
            parseValueWithUnits( FromUTF8(), value );

            switch( constraintType )
            {
            case CLEARANCE_CONSTRAINT: aRule->m_Clearance.Max = value;       break;
            case TRACK_CONSTRAINT:     aRule->m_TrackConstraint.Max = value; break;
            default: Expecting( "min" );
            }

            NeedRIGHT();
            break;

        case T_opt:
            NextTok();
            parseValueWithUnits( FromUTF8(), value );

            switch( constraintType )
            {
            case CLEARANCE_CONSTRAINT: aRule->m_Clearance.Opt = value;       break;
            case TRACK_CONSTRAINT:     aRule->m_TrackConstraint.Opt = value; break;
            default: Expecting( "min" );
            }

            NeedRIGHT();
            break;

        default:
            Expecting( "allow or constraint" );
        }
    }
}


void DRC_RULES_PARSER::parseValueWithUnits( const wxString& aExpr, int& aResult )
{
    PCB_EXPR_EVALUATOR evaluator;

    bool ok = evaluator.Evaluate( aExpr );

    if( !ok )
    {
        LIBEVAL::ERROR_STATUS error = evaluator.GetErrorStatus();
        THROW_PARSE_ERROR( error.message, CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() + error.srcPos );
    }

    aResult = evaluator.Result();
};
