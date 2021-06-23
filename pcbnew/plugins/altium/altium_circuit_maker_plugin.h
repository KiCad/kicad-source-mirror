/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

/**
 * @file pcad_plugin.h
 * @brief Pcbnew PLUGIN for Altium *.PcbDoc format.
 */

#ifndef ALTIUM_CIRCUIT_MAKER_PLUGIN_H_
#define ALTIUM_CIRCUIT_MAKER_PLUGIN_H_


#include <io_mgr.h>

class ALTIUM_CIRCUIT_MAKER_PLUGIN : public PLUGIN
{
public:
    const wxString PluginName() const override;

    BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe, const PROPERTIES* aProperties,
                 PROJECT* aProject = nullptr,
                 PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    const wxString GetFileExtension() const override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        // TODO?
        return 0;
    }

    ALTIUM_CIRCUIT_MAKER_PLUGIN();
    ~ALTIUM_CIRCUIT_MAKER_PLUGIN();

private:
    const PROPERTIES* m_props;
    BOARD*            m_board;
};

#endif // ALTIUM_CIRCUIT_MAKER_PLUGIN_H_
