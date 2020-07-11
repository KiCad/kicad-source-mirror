/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_PROJECT_LOCAL_SETTINGS_H
#define KICAD_PROJECT_LOCAL_SETTINGS_H

#include <layers_id_colors_and_visibility.h>
#include <project/board_project_settings.h>
#include <settings/json_settings.h>
#include <wildcards_and_files_ext.h>

class PROJECT;

/**
 * The project local settings are things that are attached to a particular project, but also might
 * be particular to a certain user editing that project, or change quickly, and therefore may not
 * want to be checked in to version control or otherwise distributed with the main project.
 *
 * Examples include layer visibility, recently-used design entry settings, and so on.
 *
 * The backing store is a JSON file named <project>.kicad_prl
 *
 * This file doesn't need to exist for a project to be loaded.  It will be created on-demand if
 * any of the things stored here are modified by the user.
 */
class PROJECT_LOCAL_SETTINGS : public JSON_SETTINGS
{
public:
    PROJECT_LOCAL_SETTINGS( const std::string& aFilename );

    virtual ~PROJECT_LOCAL_SETTINGS() {}

    bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    bool SaveToFile( const std::string& aDirectory = "", bool aForce = false ) override;

    void SetProject( PROJECT* aProject )
    {
        m_project = aProject;
    }

protected:

    wxString getFileExt() const override
    {
        return ProjectLocalSettingsFileExtension;
    }

    wxString getLegacyFileExt() const override
    {
        return wxT( "NO_SUCH_FILE_EXTENSION" );
    }

private:

    /// A link to the owning project
    PROJECT* m_project;

public:

    /**
     * Board settings
     */

    /// The board layers that are turned on for viewing (@see PCB_LAYER_ID)
    LSET m_VisibleLayers;

    /// The GAL layers (aka items) that are turned on for viewing (@see GAL_LAYER_ID)
    GAL_SET m_VisibleItems;

    /// The current (active) board layer for editing
    PCB_LAYER_ID m_ActiveLayer;

    /// The name of a LAYER_PRESET that is currently activated (or blank if none)
    wxString m_ActiveLayerPreset;

    /// The current contrast mode
    HIGH_CONTRAST_MODE m_ContrastModeDisplay;

    /**
     * A list of netnames that have been manually hidden in the board editor.
     * Currently, hiding nets means hiding the ratsnest for those nets.
     */
    std::vector<wxString> m_HiddenNets;

    /// State of the selection filter widget
    SELECTION_FILTER_OPTIONS m_SelectionFilter;
};

#endif
