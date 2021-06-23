/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file fabmaster_plugin.h
 * @brief Pcbnew PLUGIN for Fabmaster (Allegro) ASCII format.
 */

#ifndef FABMASTER_PLUGIN_H_
#define FABMASTER_PLUGIN_H_


#include "import_fabmaster.h"
#include <io_mgr.h>

class FABMASTER_PLUGIN : public PLUGIN
{
public:

    // -----<PUBLIC PLUGIN API>--------------------------------------------------

    const wxString  PluginName() const override;

    BOARD*          Load( const wxString&    aFileName,
                          BOARD*             aAppendToMe,
                          const PROPERTIES*  aProperties = NULL,
                          PROJECT*           aProject = nullptr,
                          PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    const wxString  GetFileExtension() const override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        // No support for libraries....
        return 0;
    }

    // -----</PUBLIC PLUGIN API>-------------------------------------------------

    FABMASTER_PLUGIN();
    ~FABMASTER_PLUGIN();

private:
    const PROPERTIES*   m_props;
    BOARD*              m_board;

    FABMASTER           m_fabmaster;
};

#endif    // PCAD_PLUGIN_H_
