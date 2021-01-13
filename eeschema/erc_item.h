/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <rc_item.h>


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
        return allItemTypes;
    }

    bool IsSheetSpecific() const { return m_sheetSpecific; }
    void SetIsSheetSpecific( bool aSpecific = true ) { m_sheetSpecific = aSpecific; }

private:
    ERC_ITEM( int aErrorCode = 0, const wxString& aTitle = "", const wxString& aSettingsKey = "" )
    {
        m_errorCode     = aErrorCode;
        m_errorTitle    = aTitle;
        m_settingsKey   = aSettingsKey;
        m_sheetSpecific = false;
    }

    /// A list of all ERC_ITEM types which are valid error codes
    static std::vector<std::reference_wrapper<RC_ITEM>> allItemTypes;

    static ERC_ITEM heading_connections;
    static ERC_ITEM heading_conflicts;
    static ERC_ITEM heading_misc;

    static ERC_ITEM duplicateSheetName;
    static ERC_ITEM pinNotConnected;
    static ERC_ITEM pinNotDriven;
    static ERC_ITEM powerpinNotDriven;
    static ERC_ITEM pinTableWarning;
    static ERC_ITEM pinTableError;
    static ERC_ITEM hierLabelMismatch;
    static ERC_ITEM noConnectConnected;
    static ERC_ITEM noConnectDangling;
    static ERC_ITEM labelDangling;
    static ERC_ITEM globalLabelDangling;
    static ERC_ITEM similarLabels;
    static ERC_ITEM differentUnitFootprint;
    static ERC_ITEM differentUnitNet;
    static ERC_ITEM busDefinitionConflict;
    static ERC_ITEM multipleNetNames;
    static ERC_ITEM netNotBusMember;
    static ERC_ITEM busLabelSyntax;
    static ERC_ITEM busToBusConflict;
    static ERC_ITEM busToNetConflict;
    static ERC_ITEM unresolvedVariable;
    static ERC_ITEM wireDangling;
    static ERC_ITEM libSymbolIssues;

    /// True if this item is specific to a sheet instance (as opposed to applying to all instances)
    bool m_sheetSpecific;
};


#endif      // ERC_ITEM_H
