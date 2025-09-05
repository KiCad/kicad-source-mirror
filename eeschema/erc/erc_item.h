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

#ifndef ERC_ITEM_H
#define ERC_ITEM_H

#include <optional>
#include <rc_item.h>
#include "sch_sheet_path.h"

/**
 * A specialisation of the RC_TREE_MODEL class to enable ERC errors / warnings to be resolved in
 * a specific sheet path context. This allows the displayed ERC descriptions in the ERC dialog to
 * reflect component references on a per-sheet basis in hierarchical schematics.
 * @see RC_TREE_MODEL
 */
class ERC_TREE_MODEL : public RC_TREE_MODEL
{
public:
    ERC_TREE_MODEL( EDA_DRAW_FRAME* aParentFrame, wxDataViewCtrl* aView ) :
            RC_TREE_MODEL( aParentFrame, aView )
    {
    }

    ~ERC_TREE_MODEL() {}

    /**
     * Override of RC_TREE_MODEL::GetValue which returns item descriptions in a specific
     * SCH_SHEET_PATH context, if a context is available on the given SCH_MARKER or ERC_ITEM
     * targets.
     */
    void GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                   unsigned int aCol ) const override;
};

class ERC_ITEM : public RC_ITEM
{
public:
    /**
     * Constructs an ERC_ITEM for the given error code
     * @see ERCE_T
     */
    static std::shared_ptr<ERC_ITEM> Create( int aErrorCode );

    static std::shared_ptr<ERC_ITEM> Create( const wxString& aErrorKey )
    {
        for( const RC_ITEM& item : allItemTypes )
        {
            if( aErrorKey == item.GetSettingsKey() )
                return std::make_shared<ERC_ITEM>( static_cast<const ERC_ITEM&>( item ) );
        }

        return nullptr;
    }

    static std::vector<std::reference_wrapper<RC_ITEM>> GetItemsWithSeverities()
    {
        static std::vector<std::reference_wrapper<RC_ITEM>> itemsWithSeverities;

        if( itemsWithSeverities.empty() )
        {
            for( RC_ITEM& item : allItemTypes )
            {
                if( &item == &heading_internal )
                    break;

                itemsWithSeverities.push_back( item );
            }
        }

        return itemsWithSeverities;
    }

    /**
     * Determines whether the ERC item is bound to a specific sheet, or is common across multiple
     * sheets (e.g. whether the error is internal to a hierarchical sheet, or is due to an enclosing
     * context interacting with the hierarchical sheet)
     * @return true if ERC applies to a specific sheet, otherwise false
     */
    bool IsSheetSpecific() const { return m_sheetSpecificPath.has_value(); }

    /**
     * Sets the SCH_SHEET_PATH this ERC item is bound to
     * @param aSpecificSheet The SCH_SHEET_PATH containing the ERC violation
     */
    void SetSheetSpecificPath( const SCH_SHEET_PATH& aSpecificSheet )
    {
        m_sheetSpecificPath = aSpecificSheet;
    }

    /**
     * Gets the SCH_SHEET_PATH this ERC item is bound to. Throws std::bad_optional_access if there
     * is no specific sheet path binding
     * @return the SCH_SHEET_PATH containing the ERC violation
     */
    const SCH_SHEET_PATH& GetSpecificSheetPath() const
    {
        wxASSERT( m_sheetSpecificPath.has_value() );
        return m_sheetSpecificPath.value();
    }

    /**
     * Sets the SCH_SHEET_PATH of the main item causing this ERC violation to (e.g. a schematic
     * pin). This allows violations to be specific to particular uses of shared hierarchical
     * schematics.
     * @param mainItemSheet the SCH_SHEET_PATH of the item causing the ERC violation
     */
    void SetItemsSheetPaths( const SCH_SHEET_PATH& mainItemSheet )
    {
        m_mainItemSheet = mainItemSheet;
    }

    /**
     * Set the SCH_SHEET PATHs of the main and auxiliary items causing this ERC violation to (e.g.
     * two schematic pins which have a mutual connection violation). This allows violations to be
     * specific to particular uses of shared hierarchical schematics.
     * @param mainItemSheet the SCH_SHEET_PATH of the first item causing the ERC violation
     * @param auxItemSheet  the SCH_SHEET_PATH of the second item causing the ERC violation
     */
    void SetItemsSheetPaths( const SCH_SHEET_PATH& mainItemSheet,
                             const SCH_SHEET_PATH& auxItemSheet )
    {
        m_mainItemSheet = mainItemSheet;
        m_auxItemSheet = auxItemSheet;
    }

    /**
     * Gets the SCH_SHEET_PATH of the main item causing this ERC violation
     * @return SCH_SHEET_PATH containing the main item
     */
    SCH_SHEET_PATH& GetMainItemSheetPath()
    {
        wxASSERT( MainItemHasSheetPath() );
        return m_mainItemSheet.value();
    }

    /**
     * Gets the SCH_SHEET_PATH of the auxiliary item causing this ERC violation
     * @return SCH_SHEET_PATH containing the auxiliary item
     */
    SCH_SHEET_PATH& GetAuxItemSheetPath()
    {
        wxASSERT( AuxItemHasSheetPath() );
        return m_auxItemSheet.value();
    }

    /**
     * Determines whether the main item causing this ERC violation has a specific
     * SCH_SHEET_PATH binding.
     * @return true if the item ERC violation is specific to a sheet, false otherwise
     */
    bool MainItemHasSheetPath() { return m_mainItemSheet.has_value(); }

    /**
     * Determines whether the auxiliary item causing this ERC violation has a specific
     * SCH_SHEET_PATH binding.
     * @return true if the item ERC violation is specific to a sheet, false otherwise
     */
    bool AuxItemHasSheetPath() { return m_auxItemSheet.has_value(); }

private:
    ERC_ITEM( int aErrorCode = 0, const wxString& aTitle = "", const wxString& aSettingsKey = "" )
    {
        m_errorCode     = aErrorCode;
        m_errorTitle    = aTitle;
        m_settingsKey = aSettingsKey;
    }

    std::optional<SCH_SHEET_PATH> m_mainItemSheet;
    std::optional<SCH_SHEET_PATH> m_auxItemSheet;

    /// A list of all ERC_ITEM types which are valid error codes
    static std::vector<std::reference_wrapper<RC_ITEM>> allItemTypes;

    static ERC_ITEM heading_connections;
    static ERC_ITEM heading_conflicts;
    static ERC_ITEM heading_misc;
    static ERC_ITEM heading_internal;

    static ERC_ITEM duplicateSheetName;
    static ERC_ITEM endpointOffGrid;
    static ERC_ITEM pinNotConnected;
    static ERC_ITEM pinNotDriven;
    static ERC_ITEM powerpinNotDriven;
    static ERC_ITEM duplicatePinError;
    static ERC_ITEM pinTableWarning;
    static ERC_ITEM pinTableError;
    static ERC_ITEM genericWarning;
    static ERC_ITEM genericError;
    static ERC_ITEM hierLabelMismatch;
    static ERC_ITEM noConnectConnected;
    static ERC_ITEM fourWayJunction;
    static ERC_ITEM labelMultipleWires;
    static ERC_ITEM noConnectDangling;
    static ERC_ITEM labelDangling;
    static ERC_ITEM isolatedPinLabel;
    static ERC_ITEM singleGlobalLabel;
    static ERC_ITEM sameLocalGlobalLabel;
    static ERC_ITEM similarLabels;
    static ERC_ITEM similarPower;
    static ERC_ITEM similarLabelAndPower;
    static ERC_ITEM differentUnitFootprint;
    static ERC_ITEM differentUnitNet;
    static ERC_ITEM busDefinitionConflict;
    static ERC_ITEM multipleNetNames;
    static ERC_ITEM netNotBusMember;
    static ERC_ITEM busToBusConflict;
    static ERC_ITEM busToNetConflict;
    static ERC_ITEM groundPinNotGround;
    static ERC_ITEM stackedPinName;
    static ERC_ITEM unresolvedVariable;
    static ERC_ITEM undefinedNetclass;
    static ERC_ITEM simulationModelIssues;
    static ERC_ITEM wireDangling;
    static ERC_ITEM libSymbolIssues;
    static ERC_ITEM libSymbolMismatch;
    static ERC_ITEM footprintLinkIssues;
    static ERC_ITEM footprintFilters;
    static ERC_ITEM unannotated;
    static ERC_ITEM extraUnits;
    static ERC_ITEM missingUnits;
    static ERC_ITEM missingInputPin;
    static ERC_ITEM missingBidiPin;
    static ERC_ITEM missingPowerInputPin;
    static ERC_ITEM differentUnitValue;
    static ERC_ITEM duplicateReference;
    static ERC_ITEM busEntryNeeded;
    static ERC_ITEM unconnectedWireEndpoint;

    /// True if this item is specific to a sheet instance (as opposed to applying to all instances)
    std::optional<SCH_SHEET_PATH> m_sheetSpecificPath;
};


#endif      // ERC_ITEM_H
