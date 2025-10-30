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

#ifndef TOOLS_MULTICHANNEL_TOOL_H
#define TOOLS_MULTICHANNEL_TOOL_H

#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <tools/pcb_tool_base.h>
#include <tools/pcb_picker_tool.h>
#include <connectivity/topo_match.h>

#include <pad.h>
#include <footprint.h>
#include <reporter.h>
#include <zone_settings.h>

class wxWindow;

struct REPEAT_LAYOUT_OPTIONS
{
    bool                    m_copyRouting = true;
    bool                    m_connectedRoutingOnly = false;
    bool                    m_copyPlacement = true;
    bool                    m_copyOtherItems = true;
    bool                    m_groupItems = false;
    bool                    m_includeLockedItems = true;
    FOOTPRINT*              m_anchorFp = nullptr;
};

struct RULE_AREA;

struct RULE_AREA_COMPAT_DATA
{
    bool                      m_isOk = false;
    bool                      m_doCopy = false;
    wxString                  m_errorMsg;
    TMATCH::COMPONENT_MATCHES m_matchingComponents;
    std::vector<wxString>     m_mismatchReasons;
    /// Filled in by copyRuleAreaContents with items that were affected by the copy operation.
    std::unordered_set<BOARD_ITEM*> m_affectedItems;
    /// Filled in by copyRuleAreaContents with affected items that can be grouped together.
    std::unordered_set<BOARD_ITEM*> m_groupableItems;
};

struct RULE_AREA
{
    PLACEMENT_SOURCE_T        m_sourceType = PLACEMENT_SOURCE_T::SHEETNAME;
    ZONE*                     m_oldZone = nullptr;
    ZONE*                     m_zone = nullptr;
    std::set<FOOTPRINT*>      m_components;
    std::unordered_set<EDA_ITEM*> m_designBlockItems;
    bool                      m_existsAlready = false;
    bool                      m_generateEnabled = false;
    wxString                  m_sheetPath;
    wxString                  m_sheetName;
    wxString                  m_ruleName;
    wxString                  m_componentClass;
    wxString                  m_groupName;
    VECTOR2I                  m_center;
};


struct RULE_AREAS_DATA
{
    bool                  m_replaceExisting = false;
    RULE_AREA*            m_refRA = nullptr;
    REPEAT_LAYOUT_OPTIONS m_options;

    std::vector<RULE_AREA>                                m_areas;
    std::unordered_map<RULE_AREA*, RULE_AREA_COMPAT_DATA> m_compatMap;
};


class MULTICHANNEL_TOOL : public PCB_TOOL_BASE, public PCB_PICKER_TOOL::RECEIVER
{
public:
    MULTICHANNEL_TOOL();
    ~MULTICHANNEL_TOOL();

    int RepeatLayout( const TOOL_EVENT& aEvent, ZONE* aRefZone );
    int RepeatLayout( const TOOL_EVENT& aEvent, RULE_AREA& aRefArea, RULE_AREA& aTargetArea );
    int AutogenerateRuleAreas( const TOOL_EVENT& aEvent );

    void UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint ) override {};
    void UpdatePickedItem( const EDA_ITEM* aItem ) override;

    void ShowMismatchDetails( wxWindow* aParent, const wxString& aSummary,
                              const std::vector<wxString>& aReasons ) const;

    RULE_AREAS_DATA* GetData() { return &m_areas; }

    void GeneratePotentialRuleAreas();
    void FindExistingRuleAreas();
    int  CheckRACompatibility( ZONE* aRefZone );

private:
    void setTransitions() override;
    int  repeatLayout( const TOOL_EVENT& aEvent );

    wxString stripComponentIndex( const wxString& aRef ) const;

    bool findComponentsInRuleArea( RULE_AREA* aRuleArea, std::set<FOOTPRINT*>& aComponents );
    bool findOtherItemsInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_ITEM*>& aItems );
    int  findRoutingInRuleArea( RULE_AREA* aRuleArea, std::set<BOARD_CONNECTED_ITEM*>& aOutput,
                                std::shared_ptr<CONNECTIVITY_DATA> aConnectivity, const SHAPE_POLY_SET& aRAPoly,
                                const REPEAT_LAYOUT_OPTIONS& aOpts ) const;
    bool copyRuleAreaContents( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                               BOARD_COMMIT* aCommit, REPEAT_LAYOUT_OPTIONS aOpts,
                               RULE_AREA_COMPAT_DATA& aCompatData );

    const SHAPE_LINE_CHAIN buildRAOutline( std::set<FOOTPRINT*>& aFootprints, int aMargin );

    std::set<FOOTPRINT*> queryComponentsInSheet( wxString aSheetName ) const;
    std::set<FOOTPRINT*> queryComponentsInComponentClass( const wxString& aComponentClassName ) const;
    std::set<FOOTPRINT*> queryComponentsInGroup( const wxString& aGroupName ) const;

    RULE_AREA* findRAByName( const wxString& aName );
    bool       resolveConnectionTopology( RULE_AREA* aRefArea, RULE_AREA* aTargetArea,
                                          RULE_AREA_COMPAT_DATA& aMatches );
    void       fixupNet( BOARD_CONNECTED_ITEM* aRef, BOARD_CONNECTED_ITEM* aTarget,
                         TMATCH::COMPONENT_MATCHES& aComponentMatches );
    bool       pruneExistingGroups( COMMIT& aCommit, const std::unordered_set<BOARD_ITEM*>& aItemsToCheck );

    std::unique_ptr<REPORTER> m_reporter;
    RULE_AREAS_DATA           m_areas;
};


#endif // TOOLS_MULTICHANNEL_TOOL
