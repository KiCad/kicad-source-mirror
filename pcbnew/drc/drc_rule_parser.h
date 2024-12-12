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

#ifndef DRC_RULE_PARSER_H
#define DRC_RULE_PARSER_H

#include <core/typeinfo.h>
#include <netclass.h>
#include <layer_ids.h>
#include <drc/drc_rule.h>
#include <drc_rules_lexer.h>


class BOARD_ITEM;


#define DRC_RULE_FILE_VERSION      20200610


class DRC_RULES_PARSER : public DRC_RULES_LEXER
{
public:
    DRC_RULES_PARSER( const wxString& aSource, const wxString& aSourceDescr );

    void Parse( std::vector<std::shared_ptr<DRC_RULE>>& aRules, REPORTER* aReporter );
    
    bool VerifyParseCondition( REPORTER* aReporter );

    void ParseComponentClassAssignmentRules(
            std::vector<std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>>& aRules,
            REPORTER*                                                      aReporter );

private:
    std::shared_ptr<DRC_RULE> parseDRC_RULE();

    std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE> parseComponentClassAssignment();

    void parseConstraint( DRC_RULE* aRule );
    void parseValueWithUnits( int aOffset, const wxString& aExpr, int& aResult, EDA_UNITS& aUnits,
                              bool aUnitless = false );
    LSET parseLayer( wxString* aSource );
    SEVERITY parseSeverity();
    void parseUnknown();
    wxString parseExpression();

    void reportError( const wxString& aMessage, int aOffset = 0 );
    void reportDeprecation( const wxString& oldToken, const wxString& newToken );
    bool checkUnresolvedTextVariable();
    void expected( const wxString& expectedTokens );

private:
    int       m_requiredVersion;
    bool      m_tooRecent;
    REPORTER* m_reporter;
};

#endif      // DRC_RULE_PARSER_H
