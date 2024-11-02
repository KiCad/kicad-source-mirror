/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TOOLS_MULTICHANNEL_TOOL_H
#define TOOLS_MULTICHANNEL_TOOL_H

#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <tools/pcb_tool_base.h>
#include <connectivity/topo_match.h>

#include <pad.h>
#include <footprint.h>
#include <reporter.h>

enum class REPEAT_LAYOUT_EDGE_MODE
{
    INSIDE = 0,
    TOUCHING,
    CLIP
};

struct REPEAT_LAYOUT_OPTIONS
{
    bool                    m_copyRouting = true;
    bool                    m_copyPlacement = true;
    bool                    m_groupItems = false;
    bool                    m_moveOffRAComponents = true;
    bool                    m_includeLockedItems = true;
    bool                    m_keepOldRouting = false;
    bool                    m_copyOnlyMatchingRAShapes = false;
    REPEAT_LAYOUT_EDGE_MODE m_edgeMode = REPEAT_LAYOUT_EDGE_MODE::INSIDE;
};


struct RULE_AREA;

struct RULE_AREA_COMPAT_DATA
{
    RULE_AREA*                m_refArea = nullptr;
    bool                      m_isOk = false;
    bool                      m_doCopy = false;
    wxString                  m_errorMsg;
    TMATCH::COMPONENT_MATCHES m_matchingComponents;
};

struct RULE_AREA
{
    ZONE*                            m_oldArea = nullptr;
    ZONE*                            m_area = nullptr;
    std::set<FOOTPRINT*>             m_raFootprints;
    std::set<FOOTPRINT*>             m_sheetComponents;
    bool                             m_existsAlready = false;
    bool                             m_generateEnabled = false;
    wxString                         m_sheetPath;
    wxString                         m_sheetName;
    wxString                         m_ruleName;
    VECTOR2I                         m_center;
};


struct RA_SHEET
{
    bool     m_generateEnabled = false;
    wxString m_sheetPath;
    wxString m_sheetName;
};


struct RULE_AREAS_DATA
{
    bool                  m_replaceExisting = false;
    RULE_AREA*            m_refRA = nullptr;
    REPEAT_LAYOUT_OPTIONS m_options;

    std::vector<RULE_AREA>                                m_areas;
    std::unordered_map<RULE_AREA*, RULE_AREA_COMPAT_DATA> m_compatMap;
};


class MULTICHANNEL_TOOL : public PCB_TOOL_BASE
{
public:
    MULTICHANNEL_TOOL();
    ~MULTICHANNEL_TOOL();

    RULE_AREAS_DATA* GetData() { return &m_areas; }
    int AutogenerateRuleAreas( const TOOL_EVENT& aEvent );
    int RepeatLayout( const TOOL_EVENT& aEvent, ZONE* aRefZone );
    void QuerySheets();
    void FindExistingRuleAreas();
    int CheckRACompatibility( ZONE *aRefZone );

private:
    void     setTransitions() override;
    int      repeatLayout( const TOOL_EVENT& aEvent );

    wxString stripComponentIndex( wxString aRef ) const;
    bool     identifyComponentsInRuleArea( ZONE* aRuleArea, std::set<FOOTPRINT*>& aComponents );
    const SHAPE_LINE_CHAIN buildRAOutline( std::set<FOOTPRINT*>& aFootprints, int aMargin );
    std::set<FOOTPRINT*>   queryComponentsInSheet( wxString aSheetName );
    RULE_AREA* findRAByName( const wxString& aName );
    bool       resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                          RULE_AREA_COMPAT_DATA& aMatches );
    bool       copyRuleAreaContents( TMATCH::COMPONENT_MATCHES& aMatches, BOARD_COMMIT* aCommit, RULE_AREA* aRefArea,
                                     RULE_AREA* aTargetArea, REPEAT_LAYOUT_OPTIONS aOpts, std::unordered_set<BOARD_ITEM*>& aAffectedItems,
                                     std::unordered_set<BOARD_ITEM*>& aGroupableItems );
    int        findRoutedConnections( std::set<BOARD_ITEM*>&             aOutput,
                                      std::shared_ptr<CONNECTIVITY_DATA> aConnectivity,
                                      const SHAPE_POLY_SET& aRAPoly, RULE_AREA* aRA, FOOTPRINT* aFp,
                                      const REPEAT_LAYOUT_OPTIONS& aOpts ) const;

    bool pruneExistingGroups( COMMIT& aCommit, const std::unordered_set<BOARD_ITEM*>& aItemsToCheck );

    std::unique_ptr<REPORTER> m_reporter;
    RULE_AREAS_DATA           m_areas;
};


#endif // TOOLS_MULTICHANNEL_TOOL
