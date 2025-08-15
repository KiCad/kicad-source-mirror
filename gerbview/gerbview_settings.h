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

#pragma once

#include <settings/app_settings.h>
#include <excellon_defaults.h>
#include <gbr_display_options.h>

class GERBVIEW_SETTINGS : public APP_SETTINGS_BASE
{
public:
    struct APPEARANCE
    {
        bool show_border_and_titleblock;
        bool show_dcodes;
        bool show_negative_objects;
        wxString page_type;
    };

    GERBVIEW_SETTINGS();
    virtual ~GERBVIEW_SETTINGS() = default;

    std::map<std::string, nlohmann::json> GetFileHistories() override;

    bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    /**
     * return the Excellon default values to read a drill file
     * @param aNCDefaults is the EXCELLON_DEFAULTS to store these prms
     */
    void GetExcellonDefaults( EXCELLON_DEFAULTS& aNCDefaults )
    {
        aNCDefaults = m_ExcellonDefaults;
    }

public:
    APPEARANCE            m_Appearance;
    GBR_DISPLAY_OPTIONS   m_Display;
    int                   m_BoardLayersCount;

    std::vector<wxString> m_DrillFileHistory;
    std::vector<wxString> m_ZipFileHistory;
    std::vector<wxString> m_JobFileHistory;

    /**
     * A list of GERBER_DRAWLAYERS_COUNT length containing a mapping of gerber layers
     * to PCB layers, used when exporting gerbers to a PCB
     */
    std::vector<int> m_GerberToPcbLayerMapping;

    EXCELLON_DEFAULTS m_ExcellonDefaults;

protected:
    virtual std::string getLegacyFrameName() const override { return "GerberFrame"; }

};
