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
            SCH_SHAPE( SHAPE_T::POLY, LAYER_RULE_AREAS, 0 /* line width */, FILL_T::NO_FILL, SCH_RULE_AREA_T ),
            m_excludedFromSim( false ),
            m_excludedFromBOM( false ),
            m_excludedFromBoard( false ),
            m_DNP( false )
    {
        SetLayer( LAYER_RULE_AREAS );
    }

    virtual ~SCH_RULE_AREA() {}

    wxString GetClass() const override;

    wxString GetFriendlyName() const override;

    EDA_ITEM* Clone() const override;

    /**
     * Set or clear the exclude from simulation flag.
     */
    void SetExcludedFromSim( bool aExcludeFromSim, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromSim = aExcludeFromSim;
    }

    bool GetExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromSim;
    }

    /**
     * Set or clear the exclude from schematic bill of materials flag.
     */
    void SetExcludedFromBOM( bool aExcludeFromBOM, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromBOM = aExcludeFromBOM;
    }

    bool GetExcludedFromBOM( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromBOM;
    }

    /**
     * Set or clear exclude from board netlist flag.
     */
    void SetExcludedFromBoard( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                               const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromBoard = aExclude;
    }

    bool GetExcludedFromBoard( const SCH_SHEET_PATH* aInstance = nullptr,
                               const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromBoard;
    }

    /**
     * Set or clear the 'Do Not Populate' flag.
     */
    bool GetDNP( const SCH_SHEET_PATH* aInstance = nullptr,
                 const wxString& aVariantName = wxEmptyString ) const override { return m_DNP; }
    void SetDNP( bool aDNP, const SCH_SHEET_PATH* aInstance = nullptr,
                 const wxString& aVariantName = wxEmptyString ) override { m_DNP = aDNP; }

    std::vector<int> ViewGetLayers() const override;

    bool IsFilledForHitTesting() const override
    {
        return false;
    }

    virtual std::vector<SHAPE*> MakeEffectiveShapes( bool aEdgeOnly = false ) const override;

    virtual void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    /// Refresh the list of items which this rule area affects.
    void RefreshContainedItemsAndDirectives( SCH_SCREEN* screen );

    /// Update all rule area connectvity / caches in the given sheet paths.
    ///
    /// @return A map of all updated rule areas and their owning screen.
    static std::vector<std::pair<SCH_RULE_AREA*, SCH_SCREEN*>>
    UpdateRuleAreasInScreens( std::unordered_set<SCH_SCREEN*>& screens, KIGFX::SCH_VIEW* view );

    /// Return a set of all items contained within the rule area.
    const std::unordered_set<SCH_ITEM*>& GetContainedItems() const;

    const std::unordered_set<KIID>& GetPastContainedItems() const;

    /// Return the set of all directive labels attached to the rule area border.
    const std::unordered_set<SCH_DIRECTIVE_LABEL*>& GetDirectives() const;

    /// Resolve the netclass of this rule area from connected directive labels.
    ///
    /// @return The resolved netclass (if any), and the SCH_ITEM providing the declaration.
    const std::vector<std::pair<wxString, SCH_ITEM*>> GetResolvedNetclasses( const SCH_SHEET_PATH* aSheetPath ) const;

    /// Get the message panel info for the rule area.
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    /// Add a directive label which applies to items within ths rule area.
    void addDirective( SCH_DIRECTIVE_LABEL* label );

    /// Add an item to the list of items which this rule area affects.
    void addContainedItem( SCH_ITEM* item );

    /// Reset all item and directive caches, saving the current state first.
    void resetCaches();

protected:
    bool          m_excludedFromSim;
    bool          m_excludedFromBOM;
    bool          m_excludedFromBoard;
    bool          m_DNP;                   ///< True if symbol is set to 'Do Not Populate'.

    /// All #SCH_ITEM objects currently contained or intersecting the rule area.  No ownership.
    std::unordered_set<SCH_ITEM*>            m_items;
    std::unordered_set<KIID>                 m_itemIDs;

    /// All #SCH_DIRECTIVE_LABEL objects attached to the rule area border.  No ownership.
    std::unordered_set<SCH_DIRECTIVE_LABEL*> m_directives;
    std::unordered_set<KIID>                 m_directiveIDs;

    /// All #SCH_ITEM objectss contained or intersecting the rule area in the previous update.
    std::unordered_set<KIID>                 m_prev_items;

    /// All SCH_DIRECTIVE_LABEL objects attached to the rule area border in the previous update.
    std::unordered_set<KIID>                 m_prev_directives;
};

#endif
