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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCH_RULE_AREA_H
#define SCH_RULE_AREA_H

#include <unordered_set>
#include <map>
#include <utility>

#include <plotters/plotter.h>
#include <sch_plotter.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_label.h>
#include <sch_sheet_path.h>
#include <sch_view.h>

class SCHEMATIC;

class SCH_RULE_AREA;


/**
 * Variant information for a rule area.
 *
 * Rule area variants are DNP and exclusion differentials against the default rule area values.
 *
 * @note #SCH_RULE_AREA::m_excludedFromBoard is immutable across variants, as it is for sheets.
 */
class SCH_RULE_AREA_VARIANT : public VARIANT
{
public:
    SCH_RULE_AREA_VARIANT( const wxString& aName = wxEmptyString ) :
            VARIANT( aName )
    {
    }

    void InitializeAttributes( const SCH_RULE_AREA& aRuleArea );

    /// Return true if the variant carries any differential against the base rule area values.
    bool HasDifferentials( const SCH_RULE_AREA& aRuleArea ) const;
};


/**
 * Rule area variants for one sheet instance of the screen holding the rule area.
 */
struct SCH_RULE_AREA_INSTANCE
{
    /// Path of the sheet holding the rule area, as for #SCH_SYMBOL_INSTANCE.
    KIID_PATH m_Path;

    wxString  m_ProjectName;

    std::map<wxString, SCH_RULE_AREA_VARIANT> m_Variants;
};


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

    virtual ~SCH_RULE_AREA();

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

    wxString GetClass() const override;

    wxString GetFriendlyName() const override;

    EDA_ITEM* Clone() const override;

    /**
     * Set or clear the exclude from simulation flag.
     */
    void SetExcludedFromSim( bool aExcludeFromSim, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override;

    bool GetExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString&       aVariantName = wxEmptyString ) const override;

    bool GetExcludedFromSimProp() const
    {
        return GetExcludedFromSim( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromSimProp( bool aExcludeFromSim )
    {
        SetExcludedFromSim( aExcludeFromSim, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    /**
     * Set or clear the exclude from schematic bill of materials flag.
     */
    void SetExcludedFromBOM( bool aExcludeFromBOM, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override;

    bool GetExcludedFromBOM( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString&       aVariantName = wxEmptyString ) const override;

    bool GetExcludedFromBOMProp() const
    {
        return GetExcludedFromBOM( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
    }

    void SetExcludedFromBOMProp( bool aExcludeFromBOM )
    {
        SetExcludedFromBOM( aExcludeFromBOM, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() );
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

    bool GetExcludedFromBoardProp() const { return GetExcludedFromBoard(); }
    void SetExcludedFromBoardProp( bool aExclude ) { SetExcludedFromBoard( aExclude ); }

    /**
     * Set or clear the 'Do Not Populate' flag.
     */
    bool GetDNP( const SCH_SHEET_PATH* aInstance = nullptr,
                 const wxString&       aVariantName = wxEmptyString ) const override;
    void SetDNP( bool aDNP, const SCH_SHEET_PATH* aInstance = nullptr,
                 const wxString& aVariantName = wxEmptyString ) override;

    bool GetDNPProp() const { return GetDNP( &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() ); }

    void SetDNPProp( bool aDNP ) { SetDNP( aDNP, &Schematic()->CurrentSheet(), Schematic()->GetCurrentVariant() ); }

    std::vector<int> ViewGetLayers() const override;

    bool IsFilledForHitTesting() const override
    {
        return false;
    }

    virtual std::vector<SHAPE*> MakeEffectiveShapes( bool aEdgeOnly = false ) const override;

    virtual void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                       int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    bool HasConnectivityChanges( const SCH_ITEM* aItem, const SCH_SHEET_PATH* aInstance = nullptr ) const override;

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

    /// Return true when the rule area exists only as a container for attached directive labels.
    /// This requires at least one attached directive and no design effect of its own (no DNP and
    /// no exclude-from flags set).
    bool IsDirectiveLabelOnlyArea() const
    {
        return !m_directives.empty() && !m_DNP && !m_excludedFromSim && !m_excludedFromBOM
               && !m_excludedFromBoard && !HasVariantDifferentials();
    }

    /// Get the message panel info for the rule area.
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /// Remove an item from this rule area's caches (called when the item is deleted).
    void RemoveItem( SCH_ITEM* aItem );

    /// Remove a directive label from this rule area's caches (called when the label is deleted).
    void RemoveDirective( SCH_DIRECTIVE_LABEL* aLabel );

    const std::vector<SCH_RULE_AREA_INSTANCE>& GetInstances() const { return m_instances; }

    /// Add an instance, replacing any existing instance with the same path.
    void AddInstance( const SCH_RULE_AREA_INSTANCE& aInstance );

    void ClearInstances() { m_instances.clear(); }

    /// Delete a variant definition from every instance.
    void DeleteVariant( const wxString& aVariantName );

    /// Rename a variant definition on every instance.
    void RenameVariant( const wxString& aOldName, const wxString& aNewName );

    /// Copy a variant definition to a new name on every instance holding it.
    void CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant );

    /// Test if any instance defines a variant.
    bool HasVariant( const wxString& aVariantName ) const;

    /// Test if any instance holds a variant that resolves differently from the base values.
    bool HasVariantDifferentials() const;

    /// Break the const-contract for the test suite (only)
    std::unordered_set<SCH_ITEM*>& _GetContainedItems() { return m_items; }
    std::unordered_set<SCH_DIRECTIVE_LABEL*>& _GetContainedDirectives() { return m_directives; }

protected:
    /// Add a directive label which applies to items within ths rule area.
    void addDirective( SCH_DIRECTIVE_LABEL* label );

    /// Add an item to the list of items which this rule area affects.
    void addContainedItem( SCH_ITEM* item );

    /// Reset all item and directive caches, saving the current state first.
    void resetCaches();

    void swapData( SCH_ITEM* aItem ) override;

    SCH_RULE_AREA_INSTANCE*       getInstance( const KIID_PATH& aPath );
    const SCH_RULE_AREA_INSTANCE* getInstance( const KIID_PATH& aPath ) const;

    bool getVariantAttribute( const SCH_SHEET_PATH* aInstance, const wxString& aVariantName,
                              bool SCH_RULE_AREA::*aBase, bool SCH_RULE_AREA_VARIANT::*aOverride ) const;

    void setVariantAttribute( bool aValue, const SCH_SHEET_PATH* aInstance, const wxString& aVariantName,
                              bool SCH_RULE_AREA::*aBase, bool SCH_RULE_AREA_VARIANT::*aOverride );

protected:
    bool          m_excludedFromSim;
    bool          m_excludedFromBOM;
    bool          m_excludedFromBoard;
    bool          m_DNP;                   ///< True if symbol is set to 'Do Not Populate'.

    std::vector<SCH_RULE_AREA_INSTANCE> m_instances;

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
