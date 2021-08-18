/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SPICE_SETTINGS_H__
#define __SPICE_SETTINGS_H__

#include <settings/nested_settings.h>


/**
 * Storage for simulator specific settings.
 */
class SPICE_SIMULATOR_SETTINGS : public NESTED_SETTINGS
{
public:
    SPICE_SIMULATOR_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~SPICE_SIMULATOR_SETTINGS() {}

    virtual bool operator==( const SPICE_SIMULATOR_SETTINGS& aRhs ) const = 0;

    bool operator!=( const SPICE_SIMULATOR_SETTINGS& aRhs ) const { return !( *this == aRhs ); }

    wxString GetWorkbookFilename() const { return m_workbookFilename; }
    void     SetWorkbookFilename( const wxString& aFilename ) { m_workbookFilename = aFilename; }

    bool GetFixPassiveVals() const { return m_fixPassiveVals; }
    void SetFixPassiveVals( bool aFixPassiveVals )
    {
        m_fixPassiveVals = aFixPassiveVals;
    }

    bool GetFixIncludePaths() const { return m_fixIncludePaths; }
    void SetFixIncludePaths( bool aFixIncludePaths )
    {
        m_fixIncludePaths = aFixIncludePaths;
    }

private:
    wxString m_workbookFilename;
    bool m_fixPassiveVals;
    bool m_fixIncludePaths;
};

/**
 * Ngspice simulator  model compatibility modes.
 *
 * @note The ngspice model modes are mutually exclusive.
 */
enum class NGSPICE_MODEL_MODE {
    USER_CONFIG,
    NGSPICE,
    PSPICE,
    LTSPICE,
    LT_PSPICE,
    HSPICE
};


/**
 * Container for Ngspice simulator settings.
 */
class NGSPICE_SIMULATOR_SETTINGS : public SPICE_SIMULATOR_SETTINGS
{
public:
    NGSPICE_SIMULATOR_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );
    virtual ~NGSPICE_SIMULATOR_SETTINGS() {}

    bool operator==( const SPICE_SIMULATOR_SETTINGS& aRhs ) const override;

    NGSPICE_MODEL_MODE GetModelMode() const { return m_modelMode; }
    void SetModelMode( NGSPICE_MODEL_MODE aMode ) { m_modelMode = aMode; }

private:
    NGSPICE_MODEL_MODE m_modelMode;
};


#endif // __SPICE_SETTINGS_H__
