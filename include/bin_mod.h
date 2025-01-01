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

#ifndef BIN_MOD_H_
#define BIN_MOD_H_

#include <wx/string.h>

#include <search_stack.h>

class APP_SETTINGS_BASE;

/**
 * Pertains to a single program module, either an EXE or a DSO/DLL ("bin_mod").
 *
 * It manages miscellaneous configuration file information pertinent to one bin_mod.
 * Because it serves in both DSO/DLLs and in EXEs, its name is neutral.  Accessors are
 * in containing (wrapper) classes.
 */
struct BIN_MOD
{
    BIN_MOD( const char* aName );
    ~BIN_MOD();

    void Init();
    void End();

    /**
     * Takes ownership of a new application settings object.
     *
     * @param aPtr is the settings object for this module.
     */
    void InitSettings( APP_SETTINGS_BASE* aPtr ) { m_config = aPtr; }

    const char*         m_name;             ///< name of this binary module, static C string.

    APP_SETTINGS_BASE*  m_config; ///< maybe from $HOME/.${m_name}
    wxString            m_help_file;

    SEARCH_STACK        m_search;
};

#endif // BIN_MOD_H_
