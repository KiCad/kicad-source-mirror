/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_RULE_AREA_H
#define SCH_RULE_AREA_H

#include <unordered_set>
#include <utility>

#include <plotters/plotter.h>
#include <sch_plotter.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_label.h>
#include <sch_sheet_path.h>
#include <sch_view.h>

class SCHEMATIC;

class SCH_RULE_AREA : public SCH_SHAPE
{
public:
    SCH_RULE_AREA() :
            SCH_SHAPE( SHAPE_T::POLY, LAYER_RULE_AREAS, 0 /* line width */, FILL_T::NO_FILL,
                       SCH_RULE_AREA_T )
    {
        SetLayer( LAYER_RULE_AREAS );
    }

    virtual ~SCH_RULE_AREA() {}

    wxString GetClass() const override;

    wxString GetFriendlyName() const override;

    EDA_ITEM* Clone() const override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    virtual std::vector<SHAPE*> MakeEffectiveShapes( bool aEdgeOnly = false ) const override;

    virtual void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    /// @brief Resets all item and directive caches, saving the current state first
    void ResetCaches( KIGFX::SCH_VIEW* view );

    /// @brief Refreshes the list of items which this rule area affects
    void RefreshContainedItemsAndDirectives(
            SCH_SCREEN* screen, KIGFX::SCH_VIEW* view,
            std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>>& forceUpdateRuleAreas );

    /// @brief Fetches all items which were, or are, within the rule area
    std::unordered_set<SCH_ITEM*> GetPastAndPresentContainedItems() const;

    /// @brief Updates all rule area connectvity / caches in the given sheet paths
    /// @returns A map of all updated rule areas and their owning screen
    static std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>>
    UpdateRuleAreasInScreens( std::unordered_set<SCH_SCREEN*>& screens, KIGFX::SCH_VIEW* view );

    /// @brief Returns a set of all items contained within the rule area
    const std::unordered_set<SCH_ITEM*>& GetContainedItems() const;

    /// @brief Returns the set of all directive labels attached to the rule area border
    const std::unordered_set<SCH_DIRECTIVE_LABEL*> GetDirectives() const;

    /// @brief Resolves the netclass of this rule area from connected directive labels
    /// @returns The resolved netclass (if any), and the SCH_ITEM providing the declaration
    const std::vector<std::pair<wxString, SCH_ITEM*>> GetResolvedNetclasses() const;

    /// @brief Clears and resets items and directives attached to this rule area
    void ResetDirectivesAndItems( KIGFX::SCH_VIEW* view );

    /// @brief Gets the message panel info for the rule area
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    /// @brief Adds a directive label which applies to items within ths rule area
    void addDirective( SCH_DIRECTIVE_LABEL* label, KIGFX::SCH_VIEW* view );

    /// @brief Clears the list of directives
    void clearDirectives( KIGFX::SCH_VIEW* view );

    /// @briefs Adds an item to the list of items which this rule area affects
    void addContainedItem( SCH_ITEM* item );

    /// @brief Clears the list of items which this rule area affects
    void clearContainedItems();

    /// All SCH_ITEMs currently contained or intersecting the rule area
    std::unordered_set<SCH_ITEM*>            m_items;

    /// All SCH_DIRECTIVE_LABELs attached to the rule area border
    std::unordered_set<SCH_DIRECTIVE_LABEL*> m_directives;

    /// All SCH_ITEMs contained or intersecting the rule area in the previous update
    std::unordered_set<SCH_ITEM*> m_prev_items;

    /// All SCH_DIRECTIVE_LABELs attached to the rule area border in the previous update
    std::unordered_set<SCH_DIRECTIVE_LABEL*> m_prev_directives;
};

#endif
