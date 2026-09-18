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

#pragma once

#include <optional>

#include <wx/string.h>

#include <settings/environment.h>


namespace KI_TEST
{
/**
 * RAII helper that sets a process-level environment variable for the
 * lifetime of the object. On destruction, it restores the previous
 * entry, or removes the entry if it did not previously exist.
 *
 * Note that this affects the process-level environment, so it will
 * affect code that uses wxGetEnv() directly, but if variables have
 * already been read into the KiCad program-level environment variable
 * map, those will not be affected. If you want to affect the program-level
 * environment variable map, use #SCOPED_PGM_ENV_VAR
 */
class SCOPED_PROCESS_ENV_VAR
{
public:
    SCOPED_PROCESS_ENV_VAR( const wxString& aName, const std::optional<wxString>& aValue );
    ~SCOPED_PROCESS_ENV_VAR();

    // No copying, or the destructor will restore multiple times.
    SCOPED_PROCESS_ENV_VAR( const SCOPED_PROCESS_ENV_VAR& ) = delete;
    SCOPED_PROCESS_ENV_VAR& operator=( const SCOPED_PROCESS_ENV_VAR& ) = delete;

    /**
     * Set a new value for the environment variable (which will still only
     * be in effect for the lifetime of this object).
     */
    void SetValue( const wxString& aValue );

    /**
     * Get the current value of the environment variable, if it exists.
     */
    std::optional<wxString> GetValue() const;

    /**
     * Clear the environment variable for the lifetime of this object.
     * if it was previously set, it will be restored on destruction.
     */
    void ClearValue();

private:
    wxString                m_name;
    std::optional<wxString> m_oldValue;
};


/**
 * RAII helper that sets a KiCad program-level environment variable
 * for the lifetime of the object. On destruction, it restores the
 * previous entry, or removes the entry if it did not previously exist.
 *
 * Note that this only affects the KiCad program-level environment
 * variable map. It will not affect code that uses wxGetEnv() directly.
 */
class SCOPED_PGM_ENV_VAR
{
public:
    /**
     * @param aName the name of the environment variable to set
     * @param aValue the value to set the variable to. If nullopt, the variable is
     *               removed for the lifetime of the object.
     */
    SCOPED_PGM_ENV_VAR( const wxString& aName, const std::optional<wxString>& aValue );
    ~SCOPED_PGM_ENV_VAR();

    // No copying, or the destructor will restore multiple times.
    SCOPED_PGM_ENV_VAR( const SCOPED_PGM_ENV_VAR& ) = delete;
    SCOPED_PGM_ENV_VAR& operator=( const SCOPED_PGM_ENV_VAR& ) = delete;

    /**
     * Get the wrapped environment variable item. The caller can modify this
     * freely. Whatever it is set to will be restored on destruction.
     */
    ENV_VAR_ITEM&       GetItem();
    const ENV_VAR_ITEM& GetItem() const;

    /**
     * Clear the environment variable for the lifetime of the object.
     */
    void ClearItem() { m_pgmEnvVars.erase( m_name ); }

private:
    ENV_VAR_MAP&                m_pgmEnvVars;
    wxString                    m_name;
    std::optional<ENV_VAR_ITEM> m_oldValue;
};

} // namespace KI_TEST
