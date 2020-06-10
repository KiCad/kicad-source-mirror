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

#ifndef DRC_RULE_PARSER_H
#define DRC_RULE_PARSER_H

#include <core/typeinfo.h>
#include <netclass.h>
#include <layers_id_colors_and_visibility.h>
#include <drc/drc_rule.h>
#include <drc_rules_lexer.h>


class BOARD_ITEM;


#define DRC_RULE_FILE_VERSION      20200515


class DRC_RULES_PARSER : public DRC_RULES_LEXER
{
public:
    DRC_RULES_PARSER( BOARD* aBoard, const wxString& aSource, const wxString& aSourceDescr );
    DRC_RULES_PARSER( BOARD* aBoard, FILE* aFile, const wxString& aFilename );

    void Parse( std::vector<DRC_SELECTOR*>& aSelectors, std::vector<DRC_RULE*>& aRules );

private:
    void initLayerMap();

    DRC_SELECTOR* parseDRC_SELECTOR( wxString* aRuleName );

    DRC_RULE* parseDRC_RULE();

    void parseConstraint( DRC_RULE* aRule );
    int parseValue( DRCRULE_T::T aToken );

private:
    BOARD* m_board;
    int    m_requiredVersion;
    bool   m_tooRecent;

    std::unordered_map<std::string, PCB_LAYER_ID> m_layerMap;
};

#endif      // DRC_RULE_PARSER_H
