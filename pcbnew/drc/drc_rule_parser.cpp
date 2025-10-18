/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <zones.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule_condition.h>
#include <drc_rules_lexer.h>
#include <pcbexpr_evaluator.h>
#include <reporter.h>
#include <component_classes/component_class_assignment_rule.h>

using namespace DRCRULE_T;


DRC_RULES_PARSER::DRC_RULES_PARSER( const wxString& aSource, const wxString& aSourceDescr ) :
        DRC_RULES_LEXER( aSource.ToStdString(), aSourceDescr ),
        m_requiredVersion( 0 ),
        m_tooRecent( false ),
        m_reporter( nullptr )
{
}


void DRC_RULES_PARSER::reportError( const wxString& aMessage, int aOffset )
{
    wxString rest;
    wxString first = aMessage.BeforeFirst( '|', &rest );

    if( m_reporter )
    {
        wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                         CurLineNumber(),
                                         CurOffset() + aOffset,
                                         first,
                                         rest );

        m_reporter->Report( msg, RPT_SEVERITY_ERROR );
    }
    else
    {
        wxString msg = wxString::Format( _( "ERROR: %s%s" ), first, rest );

        THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() + aOffset );
    }
}


void DRC_RULES_PARSER::reportDeprecation( const wxString& oldToken, const wxString& newToken )
{
    if( m_reporter )
    {
        wxString msg = wxString::Format( _( "The '%s' keyword has been deprecated.  "
                                            "Please use '%s' instead." ),
                                         oldToken,
                                         newToken);

        m_reporter->Report( msg, RPT_SEVERITY_WARNING );
    }
}


bool DRC_RULES_PARSER::checkUnresolvedTextVariable()
{
    size_t pos = curText.find( "${" );

    if( pos == std::string::npos )
        return false;

    reportError( _( "Unresolved text variable" ), (int) pos );
    return true;
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


void DRC_RULES_PARSER::expected( const wxString& expectedTokens )
{
    wxString msg;

    if( curText.starts_with( "${" ) )
        msg.Printf( _( "Unresolved text variable." ) );
    else
        msg.Printf( _( "Unrecognized item '%s'.| Expected %s." ), FromUTF8(), expectedTokens );

    reportError( msg );
    parseUnknown();
}


wxString DRC_RULES_PARSER::parseExpression()
{
    wxString expr;
    int      depth = 1;

    for( T token = NextTok();  token != T_EOF;  token = NextTok() )
    {
        if( token == T_LEFT )
            depth++;

        if( token == T_RIGHT )
        {
            if( --depth == 0 )
                break;
        }

        if( !expr.IsEmpty() )
            expr += CurSeparator();

        checkUnresolvedTextVariable();
        expr += FromUTF8();
    }

    return expr;
}


void DRC_RULES_PARSER::Parse( std::vector<std::shared_ptr<DRC_RULE>>& aRules, REPORTER* aReporter )
{
    bool     haveVersion = false;
    wxString msg;

    m_reporter = aReporter;

    for( T token = NextTok(); token != T_EOF; token = NextTok() )
    {
        if( checkUnresolvedTextVariable() )
            continue;

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
            }
            else if( (int) token == DSN_NUMBER )
            {
                m_requiredVersion = (int)strtol( CurText(), nullptr, 10 );
                m_tooRecent = ( m_requiredVersion > DRC_RULE_FILE_VERSION );

                if( (int) NextTok() != DSN_RIGHT )
                    reportError( _( "Missing ')'." ) );
            }
            else
            {
                expected( _( "version number" ) );  // translate "version number"; it is not a token
            }

            break;

        case T_rule:
            aRules.emplace_back( parseDRC_RULE() );
            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            break;

        default:
            expected( wxT( "rule or version" ) );
        }
    }

    if( m_reporter && !m_reporter->HasMessage() )
        m_reporter->Report( _( "No errors found." ), RPT_SEVERITY_INFO );

    m_reporter = nullptr;
}


void DRC_RULES_PARSER::ParseComponentClassAssignmentRules(
        std::vector<std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>>& aRules, REPORTER* aReporter )
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
            haveVersion = true; // don't keep on reporting it
        }

        switch( token )
        {
        case T_version:
            haveVersion = true;
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing version number." ) );
            }
            else if( (int) token == DSN_NUMBER )
            {
                m_requiredVersion = (int) strtol( CurText(), nullptr, 10 );
                m_tooRecent = ( m_requiredVersion > DRC_RULE_FILE_VERSION );

                if( (int) NextTok() != DSN_RIGHT )
                    reportError( _( "Missing ')'." ) );
            }
            else
            {
                expected( _( "version number" ) );  // translate "version number"; it is not a token
            }

            break;

        case T_assign_component_class:
            aRules.emplace_back( parseComponentClassAssignment() );
            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            break;

        default:
            expected( wxT( "assign_component_class or version" ) );
        }
    }

    if( m_reporter && !m_reporter->HasMessage() )
        m_reporter->Report( _( "No errors found." ), RPT_SEVERITY_INFO );

    m_reporter = nullptr;
}


std::shared_ptr<DRC_RULE> DRC_RULES_PARSER::parseDRC_RULE()
{
    std::shared_ptr<DRC_RULE> rule = std::make_shared<DRC_RULE>();

    T        token = NextTok();
    wxString msg;

    if( !IsSymbol( token ) )
        reportError( _( "Missing rule name." ) );

    checkUnresolvedTextVariable();
    rule->m_Name = FromUTF8();

    for( token = NextTok(); token != T_RIGHT && token != T_EOF; token = NextTok() )
    {
        if( checkUnresolvedTextVariable() )
            continue;

        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        switch( token )
        {
        case T_constraint:
            parseConstraint( rule.get() );
            break;

        case T_condition:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing condition expression." ) );
            }
            else if( IsSymbol( token ) )
            {
                checkUnresolvedTextVariable();
                rule->m_Condition = new DRC_RULE_CONDITION( FromUTF8() );

                if( !rule->m_Condition->Compile( m_reporter, CurLineNumber(), CurOffset() ) )
                    reportError( wxString::Format( _( "Could not parse expression '%s'." ), FromUTF8() ) );

                if( (int) NextTok() != DSN_RIGHT )
                    reportError( _( "Missing ')'." ) );
            }
            else
            {
                expected( _( "quoted expression" ) );   // translate "quoted expression"; it is not a token
            }

            break;

        case T_layer:
            if( rule->m_LayerCondition != LSET::AllLayersMask() )
                reportError( _( "'layer' keyword already present." ) );

            rule->m_LayerCondition = parseLayer( &rule->m_LayerSource );
            break;

        case T_severity:
            rule->m_Severity = parseSeverity();
            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            return rule;

        default:
            expected( wxT( "constraint, condition, or disallow" ) );
        }
    }

    if( (int) CurTok() != DSN_RIGHT )
        reportError( _( "Missing ')'." ) );

    return rule;
}


std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE> DRC_RULES_PARSER::parseComponentClassAssignment()
{
    std::shared_ptr<DRC_RULE_CONDITION> condition;

    T        token = NextTok();
    wxString msg;

    if( !IsSymbol( token ) )
        reportError( _( "Missing component class name." ) );

    checkUnresolvedTextVariable();
    wxString componentClass = FromUTF8();

    for( token = NextTok(); token != T_RIGHT && token != T_EOF; token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        switch( token )
        {
        case T_condition:
            token = NextTok();

            if( (int) token == DSN_RIGHT )
            {
                reportError( _( "Missing condition expression." ) );
            }
            else if( IsSymbol( token ) )
            {
                checkUnresolvedTextVariable();
                condition = std::make_shared<DRC_RULE_CONDITION>( FromUTF8() );

                if( !condition->Compile( m_reporter, CurLineNumber(), CurOffset() ) )
                    reportError( wxString::Format( _( "Could not parse expression '%s'." ), FromUTF8() ) );

                if( (int) NextTok() != DSN_RIGHT )
                    reportError( _( "Missing ')'." ) );
            }
            else
            {
                expected( _( "quoted expression" ) );   // translate "quoted expression"; it is not a token
            }

            break;

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            return nullptr;

        default:
            expected( wxT( "condition" ) );
        }
    }

    if( (int) CurTok() != DSN_RIGHT )
        reportError( _( "Missing ')'." ) );

    return std::make_shared<COMPONENT_CLASS_ASSIGNMENT_RULE>( componentClass, std::move( condition ) );
}


void DRC_RULES_PARSER::parseConstraint( DRC_RULE* aRule )
{
    DRC_CONSTRAINT c;
    int            value;
    EDA_UNITS      units = EDA_UNITS::UNSCALED;
    EDA_DATA_TYPE  unitsType = EDA_DATA_TYPE::UNITLESS;
    wxString       msg;
    bool           allowsTimeDomain = false;

    auto validateAndSetValueWithUnits =
            [this, &allowsTimeDomain, &unitsType, &c]( int aValue, const EDA_UNITS aUnits, auto aSetter )
            {
                const EDA_DATA_TYPE unitsTypeTmp = UNITS_PROVIDER::GetTypeFromUnits( aUnits );

                if( !allowsTimeDomain && unitsTypeTmp == EDA_DATA_TYPE::TIME )
                    reportError( _( "Time based units not allowed for constraint type." ) );

                if( ( c.m_Value.HasMin() || c.m_Value.HasMax() || c.m_Value.HasOpt() )
                    && unitsType != unitsTypeTmp )
                {
                    reportError( _( "Mixed units for constraint values." ) );
                }

                unitsType = unitsTypeTmp;
                aSetter( aValue );

                if( allowsTimeDomain )
                {
                    if( unitsType == EDA_DATA_TYPE::TIME )
                    {
                        c.SetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN );
                        c.ClearOption( DRC_CONSTRAINT::OPTIONS::SPACE_DOMAIN );
                    }
                    else
                    {
                        c.SetOption( DRC_CONSTRAINT::OPTIONS::SPACE_DOMAIN );
                        c.ClearOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN );
                    }
                }
            };

    T token = NextTok();

    if( checkUnresolvedTextVariable() )
        return;

    if( token == T_mechanical_clearance )
    {
        reportDeprecation( wxT( "mechanical_clearance" ), wxT( "physical_clearance" ) );
        token = T_physical_clearance;
    }
    else if( token == T_mechanical_hole_clearance )
    {
        reportDeprecation( wxT( "mechanical_hole_clearance" ), wxT( "physical_hole_clearance" ) );
        token = T_physical_hole_clearance;
    }
    else if( token == T_hole )
    {
        reportDeprecation( wxT( "hole" ), wxT( "hole_size" ) );
        token = T_hole_size;
    }
    else if( (int) token == DSN_RIGHT || token == T_EOF )
    {
        msg.Printf( _( "Missing constraint type.|  Expected %s." ),
                    wxT( "assertion, clearance, hole_clearance, edge_clearance, physical_clearance, "
                         "physical_hole_clearance, courtyard_clearance, silk_clearance, hole_size, "
                         "hole_to_hole, track_width, track_angle, track_segment_length, annular_width, "
                         "disallow, zone_connection, thermal_relief_gap, thermal_spoke_width, "
                         "min_resolved_spokes, solder_mask_expansion, solder_paste_abs_margin, "
                         "solder_paste_rel_margin, length, skew, via_count, via_dangling, via_diameter, "
                         "diff_pair_gap or diff_pair_uncoupled" ) );

        reportError( msg );
        return;
    }

    switch( token )
    {
    case T_assertion:                 c.m_Type = ASSERTION_CONSTRAINT;                 break;
    case T_clearance:                 c.m_Type = CLEARANCE_CONSTRAINT;                 break;
    case T_creepage:                  c.m_Type = CREEPAGE_CONSTRAINT;                  break;
    case T_hole_clearance:            c.m_Type = HOLE_CLEARANCE_CONSTRAINT;            break;
    case T_edge_clearance:            c.m_Type = EDGE_CLEARANCE_CONSTRAINT;            break;
    case T_hole_size:                 c.m_Type = HOLE_SIZE_CONSTRAINT;                 break;
    case T_hole_to_hole:              c.m_Type = HOLE_TO_HOLE_CONSTRAINT;              break;
    case T_courtyard_clearance:       c.m_Type = COURTYARD_CLEARANCE_CONSTRAINT;       break;
    case T_silk_clearance:            c.m_Type = SILK_CLEARANCE_CONSTRAINT;            break;
    case T_text_height:               c.m_Type = TEXT_HEIGHT_CONSTRAINT;               break;
    case T_text_thickness:            c.m_Type = TEXT_THICKNESS_CONSTRAINT;            break;
    case T_track_width:               c.m_Type = TRACK_WIDTH_CONSTRAINT;               break;
    case T_track_angle:               c.m_Type = TRACK_ANGLE_CONSTRAINT;               break;
    case T_track_segment_length:      c.m_Type = TRACK_SEGMENT_LENGTH_CONSTRAINT;      break;
    case T_connection_width:          c.m_Type = CONNECTION_WIDTH_CONSTRAINT;          break;
    case T_annular_width:             c.m_Type = ANNULAR_WIDTH_CONSTRAINT;             break;
    case T_via_diameter:              c.m_Type = VIA_DIAMETER_CONSTRAINT;              break;
    case T_via_dangling:              c.m_Type = VIA_DANGLING_CONSTRAINT;              break;
    case T_zone_connection:           c.m_Type = ZONE_CONNECTION_CONSTRAINT;           break;
    case T_thermal_relief_gap:        c.m_Type = THERMAL_RELIEF_GAP_CONSTRAINT;        break;
    case T_thermal_spoke_width:       c.m_Type = THERMAL_SPOKE_WIDTH_CONSTRAINT;       break;
    case T_min_resolved_spokes:       c.m_Type = MIN_RESOLVED_SPOKES_CONSTRAINT;       break;
    case T_solder_mask_expansion:     c.m_Type = SOLDER_MASK_EXPANSION_CONSTRAINT;     break;
    case T_solder_paste_abs_margin:   c.m_Type = SOLDER_PASTE_ABS_MARGIN_CONSTRAINT;   break;
    case T_solder_paste_rel_margin:   c.m_Type = SOLDER_PASTE_REL_MARGIN_CONSTRAINT;   break;
    case T_disallow:                  c.m_Type = DISALLOW_CONSTRAINT;                  break;
    case T_length:                    c.m_Type = LENGTH_CONSTRAINT;                    break;
    case T_skew:                      c.m_Type = SKEW_CONSTRAINT;                      break;
    case T_via_count:                 c.m_Type = VIA_COUNT_CONSTRAINT;                 break;
    case T_diff_pair_gap:             c.m_Type = DIFF_PAIR_GAP_CONSTRAINT;             break;
    case T_diff_pair_uncoupled:       c.m_Type = MAX_UNCOUPLED_CONSTRAINT;             break;
    case T_physical_clearance:        c.m_Type = PHYSICAL_CLEARANCE_CONSTRAINT;        break;
    case T_physical_hole_clearance:   c.m_Type = PHYSICAL_HOLE_CLEARANCE_CONSTRAINT;   break;
    case T_bridged_mask:              c.m_Type = BRIDGED_MASK_CONSTRAINT;              break;
    default:
        expected( wxT( "assertion, clearance, hole_clearance, edge_clearance, physical_clearance, "
                       "physical_hole_clearance, courtyard_clearance, silk_clearance, hole_size, "
                       "hole_to_hole, track_width, track_angle, track_segment_length, annular_width, "
                       "disallow, zone_connection, thermal_relief_gap, thermal_spoke_width, "
                       "min_resolved_spokes, solder_mask_expansion, solder_paste_abs_margin, "
                       "solder_paste_rel_margin, length, skew, via_count, via_dangling, via_diameter, "
                       "diff_pair_gap, diff_pair_uncoupled or bridged_mask" ) );
        return;
    }

    if( aRule->FindConstraint( c.m_Type ) )
    {
        msg.Printf( _( "Rule already has a '%s' constraint." ), FromUTF8() );
        reportError( msg );
    }

    bool unitless = c.m_Type == VIA_COUNT_CONSTRAINT
                    || c.m_Type == MIN_RESOLVED_SPOKES_CONSTRAINT
                    || c.m_Type == TRACK_ANGLE_CONSTRAINT
                    || c.m_Type == VIA_DANGLING_CONSTRAINT
                    || c.m_Type == BRIDGED_MASK_CONSTRAINT;

    allowsTimeDomain = c.m_Type == LENGTH_CONSTRAINT || c.m_Type == SKEW_CONSTRAINT;

    if( c.m_Type == DISALLOW_CONSTRAINT )
    {
        for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
        {
            if( (int) token == DSN_STRING )
                token = GetCurStrAsToken();

            switch( token )
            {
            case T_track:       c.m_DisallowFlags |= DRC_DISALLOW_TRACKS;        break;
            case T_via:         c.m_DisallowFlags |= DRC_DISALLOW_THROUGH_VIAS
                                              | DRC_DISALLOW_BLIND_VIAS
                                              | DRC_DISALLOW_BURIED_VIAS
                                              | DRC_DISALLOW_MICRO_VIAS;       break;
            case T_through_via: c.m_DisallowFlags |= DRC_DISALLOW_THROUGH_VIAS; break;
            case T_blind_via:   c.m_DisallowFlags |= DRC_DISALLOW_BLIND_VIAS;   break;
            case T_buried_via:  c.m_DisallowFlags |= DRC_DISALLOW_BURIED_VIAS;  break;
            case T_micro_via:   c.m_DisallowFlags |= DRC_DISALLOW_MICRO_VIAS;  break;
            case T_pad:        c.m_DisallowFlags |= DRC_DISALLOW_PADS;       break;
            case T_zone:       c.m_DisallowFlags |= DRC_DISALLOW_ZONES;      break;
            case T_text:       c.m_DisallowFlags |= DRC_DISALLOW_TEXTS;      break;
            case T_graphic:    c.m_DisallowFlags |= DRC_DISALLOW_GRAPHICS;   break;
            case T_hole:       c.m_DisallowFlags |= DRC_DISALLOW_HOLES;      break;
            case T_footprint:  c.m_DisallowFlags |= DRC_DISALLOW_FOOTPRINTS; break;

            case T_EOF:
                reportError( _( "Missing ')'." ) );
                return;

            default:
                expected( wxT( "track, via, through_via, blind_via, micro_via, buried_via, pad, zone, text, "
                               "graphic, hole, or footprint." ) );
                return;
            }
        }

        if( (int) CurTok() != DSN_RIGHT )
            reportError( _( "Missing ')'." ) );

        aRule->AddConstraint( c );
        return;
    }
    else if( c.m_Type == ZONE_CONNECTION_CONSTRAINT )
    {
        token = NextTok();

        if( (int) token == DSN_STRING )
            token = GetCurStrAsToken();

        switch( token )
        {
        case T_solid:           c.m_ZoneConnection = ZONE_CONNECTION::FULL;    break;
        case T_thermal_reliefs: c.m_ZoneConnection = ZONE_CONNECTION::THERMAL; break;
        case T_none:            c.m_ZoneConnection = ZONE_CONNECTION::NONE;    break;

        case T_EOF:
            reportError( _( "Missing ')'." ) );
            return;

        default:
            expected( wxT( "solid, thermal_reliefs or none." ) );
            return;
        }

        if( (int) NextTok() != DSN_RIGHT )
            reportError( _( "Missing ')'." ) );

        aRule->AddConstraint( c );
        return;
    }
    else if( c.m_Type == MIN_RESOLVED_SPOKES_CONSTRAINT )
    {
        // We don't use a min/max/opt structure here because it would give a strong implication
        // that you could specify the optimal number of spokes.  We don't want to open that door
        // because the spoke generator is highly optimized around being able to "cheat" off of a
        // cartesian coordinate system.

        token = NextTok();

        if( (int) token == DSN_NUMBER )
        {
            value = (int) strtol( CurText(), nullptr, 10 );
            c.m_Value.SetMin( value );

            if( (int) NextTok() != DSN_RIGHT )
                reportError( _( "Missing ')'." ) );
        }
        else
        {
            expected( _( "number" ) );  // translate "number"; it is not a token
        }

        aRule->AddConstraint( c );
        return;
    }
    else if( c.m_Type == ASSERTION_CONSTRAINT )
    {
        token = NextTok();

        if( (int) token == DSN_RIGHT )
            reportError( _( "Missing assertion expression." ) );

        if( IsSymbol( token ) )
        {
            c.m_Test = new DRC_RULE_CONDITION( FromUTF8() );
            c.m_Test->Compile( m_reporter, CurLineNumber(), CurOffset() );

            if( (int) NextTok() != DSN_RIGHT )
                reportError( _( "Missing ')'." ) );
        }
        else
        {
            expected( _( "quoted expression" ) );   // translate "quoted expression"; it is not a token
        }

        aRule->AddConstraint( c );
        return;
    }

    for( token = NextTok(); token != T_RIGHT && token != T_EOF; token = NextTok() )
    {
        if( token != T_LEFT )
            reportError( _( "Missing '('." ) );

        token = NextTok();

        switch( token )
        {
        case T_within_diff_pairs:
            if( c.m_Type == SKEW_CONSTRAINT )
                c.SetOption( DRC_CONSTRAINT::OPTIONS::SKEW_WITHIN_DIFF_PAIRS );
            else
                reportError( _( "within_diff_pairs option invalid for constraint type." ) );

            if( (int) NextTok() != DSN_RIGHT )
                reportError( _( "Missing ')'." ) );

            break;

        case T_min:
        {
            size_t   offset = CurOffset() + GetTokenString( token ).length();
            wxString expr = parseExpression();

            if( expr.IsEmpty() )
            {
                reportError( _( "Missing min value." ) );
                break;
            }

            parseValueWithUnits( (int) offset, expr, value, units, unitless );
            validateAndSetValueWithUnits( value, units,
                                          [&c]( const int aValue )
                                          {
                                              c.m_Value.SetMin( aValue );
                                          } );

            break;
        }

        case T_max:
        {
            size_t   offset = CurOffset() + GetTokenString( token ).length();
            wxString expr = parseExpression();

            if( expr.IsEmpty() )
            {
                reportError( _( "Missing max value." ) );
                break;
            }

            parseValueWithUnits( (int) offset, expr, value, units, unitless );
            validateAndSetValueWithUnits( value, units,
                                          [&c]( const int aValue )
                                          {
                                              c.m_Value.SetMax( aValue );
                                          } );

            break;
        }

        case T_opt:
        {
            size_t   offset = CurOffset() + GetTokenString( token ).length();
            wxString expr = parseExpression();

            if( expr.IsEmpty() )
            {
                reportError( _( "Missing opt value." ) );
                break;
            }

            parseValueWithUnits( (int) offset, expr, value, units, unitless );
            validateAndSetValueWithUnits( value, units,
                                          [&c]( const int aValue )
                                          {
                                              c.m_Value.SetOpt( aValue );
                                          } );

            break;
        }

        case T_EOF:
            reportError( _( "Incomplete statement." ) );
            return;

        default:
            expected( wxT( "min, max, opt, or within_diff_pairs" ) );
        }
    }

    aRule->AddConstraint( c );
}


void DRC_RULES_PARSER::parseValueWithUnits( int aOffset, const wxString& aExpr, int& aResult,
                                            EDA_UNITS& aUnits, bool aUnitless )
{
    aResult = 0.0;
    aUnits = EDA_UNITS::UNSCALED;

    auto errorHandler =
            [&]( const wxString& message, int offset )
            {
                wxString rest;
                wxString first = message.BeforeFirst( '|', &rest );

                if( m_reporter )
                {
                    wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                                     CurLineNumber(),
                                                     aOffset + offset,
                                                     first,
                                                     rest );

                    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
                }
                else
                {
                    wxString msg = wxString::Format( _( "ERROR: %s%s" ), first, rest );

                    THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(),
                                       CurOffset() + aOffset );
                }
            };

    PCBEXPR_EVALUATOR evaluator( aUnitless ? (LIBEVAL::UNIT_RESOLVER*) new PCBEXPR_UNITLESS_RESOLVER()
                                           : (LIBEVAL::UNIT_RESOLVER*) new PCBEXPR_UNIT_RESOLVER() );
    evaluator.SetErrorCallback( errorHandler );

    if( evaluator.Evaluate( aExpr ) )
    {
        aResult = evaluator.Result();
        aUnits = evaluator.Units();
    }
}


LSET DRC_RULES_PARSER::parseLayer( wxString* aSource )
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
        *aSource = GetTokenString( token );
        retVal = LSET::ExternalCuMask();
    }
    else if( token == T_inner )
    {
        *aSource = GetTokenString( token );
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
            {
                *aSource = layerName;
                retVal.set( ToLAYER_ID( entry.GetValue() ) );
            }
        }

        if( !retVal.any() )
        {
            if( !checkUnresolvedTextVariable() )
                reportError( wxString::Format( _( "Unrecognized layer '%s'." ), layerName ) );

            retVal.set( Rescue );
        }
    }

    if( (int) NextTok() != DSN_RIGHT )
        reportError( _( "Missing ')'." ) );

    return retVal;
}


SEVERITY DRC_RULES_PARSER::parseSeverity()
{
    SEVERITY retVal = RPT_SEVERITY_UNDEFINED;
    wxString msg;

    T token = NextTok();

    if( (int) token == DSN_RIGHT || token == T_EOF )
    {
        reportError( _( "Missing severity name." ) );
        return RPT_SEVERITY_UNDEFINED;
    }

    switch( token )
    {
    case T_ignore:    retVal = RPT_SEVERITY_IGNORE;    break;
    case T_warning:   retVal = RPT_SEVERITY_WARNING;   break;
    case T_error:     retVal = RPT_SEVERITY_ERROR;     break;
    case T_exclusion: retVal = RPT_SEVERITY_EXCLUSION; break;

    default:
        expected( wxT( "ignore, warning, error, or exclusion" ) );
    }

    if( (int) NextTok() != DSN_RIGHT )
        reportError( _( "Missing ')'." ) );

    return retVal;
}