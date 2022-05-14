/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _KICAD_SETTINGS_H
#define _KICAD_SETTINGS_H

#include <settings/app_settings.h>
#define PCM_DEFAULT_REPOSITORY_URL "https://repository.kicad.org/repository.json"


class KICAD_SETTINGS : public APP_SETTINGS_BASE
{
public:
    KICAD_SETTINGS();

    virtual ~KICAD_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    int m_LeftWinWidth;

    /**
     * @brief General setting for various update checks
     *
     * A one time popup asks user to allow/disallow update checks on startup.
     * This is currently used by PCM.
     *
     * See enum below for meaning of values.
     */
    int m_updateCheck;

    enum UPDATE_CHECK
    {
        UNINITIALIZED = 0,
        NOT_ALLOWED = 1,
        ALLOWED = 2
    };

    std::vector<wxString> m_OpenProjects;

    std::vector<std::pair<wxString, wxString>> m_PcmRepositories;
    wxString                                   m_PcmLastDownloadDir;

    // This controls background update check for PCM.
    // It is set according to m_updateCheck on first start.
    bool m_PcmUpdateCheck;
    // Auto add libs to global table
    bool m_PcmLibAutoAdd;
    // Auto remove libs
    bool m_PcmLibAutoRemove;
    // Generated library nickname prefix
    wxString m_PcmLibPrefix;

protected:
    virtual std::string getLegacyFrameName() const override { return "KicadFrame"; }
};

#endif
