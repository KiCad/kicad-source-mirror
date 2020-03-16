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
#include <widgets/ui_common.h>


class PARAM_CFG;


/**
 * Container for ERC settings
 *
 * Currently only stores flags about checks to run, but could later be expanded
 * to contain the matrix of electrical pin types.
 */
class ERC_SETTINGS
{
public:
    ERC_SETTINGS()
    {
        for( int i = ERCE_FIRST; i <= ERCE_LAST; ++i )
            m_Severities[ i ] = RPT_SEVERITY_ERROR;

        m_Severities[ ERCE_UNSPECIFIED ] = RPT_SEVERITY_UNDEFINED;
    }

    void LoadDefaults()
    {
        m_Severities[ ERCE_SIMILAR_LABELS ] = RPT_SEVERITY_WARNING;
        m_Severities[ ERCE_GLOBLABEL ] = RPT_SEVERITY_WARNING;
        m_Severities[ ERCE_DRIVER_CONFLICT ] = RPT_SEVERITY_WARNING;
        m_Severities[ ERCE_BUS_ENTRY_CONFLICT ] = RPT_SEVERITY_WARNING;
        m_Severities[ ERCE_BUS_TO_BUS_CONFLICT ] = RPT_SEVERITY_ERROR;
        m_Severities[ ERCE_BUS_TO_NET_CONFLICT ] = RPT_SEVERITY_ERROR;
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

    std::vector<PARAM_CFG*> GetProjectFileParameters();

    std::map<int, int> m_Severities;
};

#endif
