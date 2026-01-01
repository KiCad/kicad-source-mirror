/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


class KICOMMON_API KICAD_SETTINGS : public APP_SETTINGS_BASE
{
public:
    KICAD_SETTINGS();

    virtual ~KICAD_SETTINGS() {}

    virtual bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    int m_LeftWinWidth;
    bool m_ShowHistoryPanel;


    std::vector<wxString> m_OpenProjects;

    wxString m_lastDesignBlockLibDir;

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

    bool     m_KiCadUpdateCheck;
    wxString m_lastUpdateCheckTime;
    wxString m_lastReceivedUpdate;

    // Last position of the template window
    wxPoint m_TemplateWindowPos;
    // Last size of the template window
    wxSize m_TemplateWindowSize;
    // Last used project template path (for pre-selection in dialog)
    wxString m_LastUsedTemplate;

protected:
    virtual std::string getLegacyFrameName() const override { return "KicadFrame"; }
};

#endif
