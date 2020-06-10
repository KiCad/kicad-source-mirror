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
    }
}


void DRC_RULES_PARSER::Parse( std::vector<DRC_SELECTOR*>& aSelectors,
                              std::vector<DRC_RULE*>& aRules )
{
    std::vector< std::pair<DRC_SELECTOR*, wxString> > selectorRules;
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

        case T_selector:
        {
            wxString ruleName;

            aSelectors.push_back( parseDRC_SELECTOR( &ruleName ) );
            selectorRules.emplace_back( aSelectors.back(), ruleName );
        }
            break;

        case T_rule:
            aRules.push_back( parseDRC_RULE() );
            break;

        default:
            Expecting( "selector or rule" );
        }
    }

    // Hook up the selectors to their rules
    std::map<wxString, DRC_RULE*> ruleMap;

    for( DRC_RULE* rule : aRules )
        ruleMap[ rule->m_Name ] = rule;

    for( const std::pair<DRC_SELECTOR*, wxString>& entry : selectorRules )
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
}


DRC_SELECTOR* DRC_RULES_PARSER::parseDRC_SELECTOR( wxString* aRuleName )
{
    NETCLASSES&   netclasses = m_board->GetDesignSettings().m_NetClasses;
    DRC_SELECTOR* selector = new DRC_SELECTOR();
    T             token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_match_netclass:
        {
            NeedSYMBOL();
            NETCLASSPTR netclass = netclasses.Find( FromUTF8() );

            if( netclass )
            {
                selector->m_MatchNetclasses.push_back( std::move( netclass ) );
            }
            else
            {
                // Interesting situation here: if we don't inform the user they may have a typo
                // and can't figure out why their rules don't work.
                // If we do tell them then it gets really noisy if they're using a single rule
                // file for a class of board.
            }

            NeedRIGHT();
        }
            break;

        case T_match_type:
            switch( NextTok() )
            {
            case T_track:      selector->m_MatchTypes.push_back( PCB_TRACE_T );             break;
            case T_via:        selector->m_MatchTypes.push_back( PCB_LOCATE_STDVIA_T );     break;
            case T_micro_via:  selector->m_MatchTypes.push_back( PCB_LOCATE_UVIA_T );       break;
            case T_buried_via: selector->m_MatchTypes.push_back( PCB_LOCATE_BBVIA_T );      break;
            case T_pad:        selector->m_MatchTypes.push_back( PCB_PAD_T );               break;
            case T_zone:       selector->m_MatchTypes.push_back( PCB_ZONE_AREA_T );         break;
            case T_text:       selector->m_MatchTypes.push_back( PCB_LOCATE_TEXT_T );       break;
            case T_graphic:    selector->m_MatchTypes.push_back( PCB_LOCATE_GRAPHIC_T );    break;
            case T_hole:       selector->m_MatchTypes.push_back( PCB_LOCATE_HOLE_T );       break;
            case T_npth:       selector->m_MatchTypes.push_back( PCB_LOCATE_NPTH_T );       break;
            case T_pth:        selector->m_MatchTypes.push_back( PCB_LOCATE_PTH_T );        break;
            case T_board_edge: selector->m_MatchTypes.push_back( PCB_LOCATE_BOARD_EDGE_T ); break;
            default:           Expecting( "track, via, micro_via, buried_via, pad, zone, text, "
                                          "graphic, hole, npth, pth, or board_edge" );
            }
            NeedRIGHT();
            break;

        case T_match_layer:
            NeedSYMBOL();
            selector->m_MatchLayers.push_back( m_layerMap[ curText ] );
            NeedRIGHT();
            break;

        case T_match_area:
            // TODO
            break;

        case T_rule:
            NeedSYMBOL();
            *aRuleName = FromUTF8();
            NeedRIGHT();
            break;

        default:
            Expecting( "match_netclass, match_type, match_layer, match_area, or rule" );
        }
    }

    return selector;
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
            rule->m_Condition = FromUTF8();
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
            value = parseValue( token );

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
            value = parseValue( token );

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
            value = parseValue( token );

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


int DRC_RULES_PARSER::parseValue( DRCRULE_T::T aToken )
{
    char* tmp;

    errno = 0;

    double fval = strtod( CurText(), &tmp );

    if( errno )
    {
        THROW_PARSE_ERROR( _( "Invalid floating point number" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }

    if( CurText() == tmp )
    {
        THROW_PARSE_ERROR( _( "Missing floating point number" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }

    return KiROUND( fval * IU_PER_MM );
}
