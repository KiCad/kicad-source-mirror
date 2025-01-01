/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
class SPICE_SETTINGS : public NESTED_SETTINGS
{
public:
    SPICE_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~SPICE_SETTINGS() {}

    virtual bool operator==( const SPICE_SETTINGS& aRhs ) const = 0;

    bool operator!=( const SPICE_SETTINGS& aRhs ) const { return !( *this == aRhs ); }

    wxString GetWorkbookFilename() const { return m_workbookFilename; }
    void     SetWorkbookFilename( const wxString& aFilename ) { m_workbookFilename = aFilename; }

    bool GetFixIncludePaths() const { return m_fixIncludePaths; }
    void SetFixIncludePaths( bool aFixIncludePaths ) { m_fixIncludePaths = aFixIncludePaths; }

private:
    wxString m_workbookFilename;
    bool     m_fixIncludePaths;
};

/**
 * Ngspice simulator compatibility modes.
 *
 * @note The ngspice model modes are mutually exclusive.
 */
enum class NGSPICE_COMPATIBILITY_MODE
{
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
class NGSPICE_SETTINGS : public SPICE_SETTINGS
{
public:
    NGSPICE_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );
    virtual ~NGSPICE_SETTINGS() {}

    bool operator==( const SPICE_SETTINGS& aRhs ) const override;

    NGSPICE_COMPATIBILITY_MODE GetCompatibilityMode() const { return m_compatibilityMode; }
    void SetCompatibilityMode( NGSPICE_COMPATIBILITY_MODE aMode ) { m_compatibilityMode = aMode; }

private:
    NGSPICE_COMPATIBILITY_MODE m_compatibilityMode;
};


#endif // __SPICE_SETTINGS_H__
