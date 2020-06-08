/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ERC_SETTINGS_H
#define _ERC_SETTINGS_H

#include <erc.h>
#include <erc_item.h>
#include <settings/nested_settings.h>
#include <widgets/ui_common.h>


class SCH_MARKER;

/**
 * Container for ERC settings
 *
 * Currently only stores flags about checks to run, but could later be expanded
 * to contain the matrix of electrical pin types.
 */
class ERC_SETTINGS : public NESTED_SETTINGS
{
public:
    ERC_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~ERC_SETTINGS();

    void LoadDefaults()
    {
        m_Severities[ERCE_SIMILAR_LABELS]      = RPT_SEVERITY_WARNING;
        m_Severities[ERCE_GLOBLABEL]           = RPT_SEVERITY_WARNING;
        m_Severities[ERCE_DRIVER_CONFLICT]     = RPT_SEVERITY_WARNING;
        m_Severities[ERCE_BUS_ENTRY_CONFLICT]  = RPT_SEVERITY_WARNING;
        m_Severities[ERCE_BUS_TO_BUS_CONFLICT] = RPT_SEVERITY_ERROR;
        m_Severities[ERCE_BUS_TO_NET_CONFLICT] = RPT_SEVERITY_ERROR;
    }

    bool operator==( const ERC_SETTINGS& other ) const
    {
        return ( other.m_Severities == m_Severities );
    }

    bool operator!=( const ERC_SETTINGS& other ) const
    {
        return !( other == *this );
    }

    bool IsTestEnabled( int aErrorCode ) const
    {
        return m_Severities.at( aErrorCode ) != RPT_SEVERITY_IGNORE;
    }

    int GetSeverity( int aErrorCode ) const;

    void SetSeverity( int aErrorCode, int aSeverity );

    std::map<int, int> m_Severities;
};

/**
 * SHEETLIST_ERC_ITEMS_PROVIDER
 * is an implementation of the RC_ITEM_LISTinterface which uses the global SHEETLIST
 * to fulfill the contract.
 */
class SHEETLIST_ERC_ITEMS_PROVIDER : public RC_ITEMS_PROVIDER
{
private:
    SCHEMATIC*               m_schematic;
    int                      m_severities;
    std::vector<SCH_MARKER*> m_filteredMarkers;

public:
    SHEETLIST_ERC_ITEMS_PROVIDER( SCHEMATIC* aSchematic ) :
            m_schematic( aSchematic ),
            m_severities( 0 )
    { }

    void SetSeverities( int aSeverities ) override;

    int GetCount( int aSeverity = -1 ) override;

    ERC_ITEM* GetItem( int aIndex ) override;

    void DeleteItem( int aIndex, bool aDeep ) override;

    void DeleteAllItems() override;
};


#endif
