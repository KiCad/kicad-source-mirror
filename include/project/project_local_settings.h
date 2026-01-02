/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <layer_ids.h>
#include <project/board_project_settings.h>
#include <project/sch_project_settings.h>
#include <settings/json_settings.h>
#include <wildcards_and_files_ext.h>
#include <settings/app_settings.h>

class PROJECT;

struct KICOMMON_API PROJECT_FILE_STATE
{
    wxString fileName;
    bool open;
    struct WINDOW_STATE window;
};


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
class KICOMMON_API PROJECT_LOCAL_SETTINGS : public JSON_SETTINGS
{
public:
    PROJECT_LOCAL_SETTINGS( PROJECT* aProject, const wxString& aFilename );

    virtual ~PROJECT_LOCAL_SETTINGS() {}

    bool MigrateFromLegacy( wxConfigBase* aLegacyConfig ) override;

    bool SaveAs( const wxString& aDirectory, const wxString& aFile );

    bool SaveToFile( const wxString& aDirectory = "", bool aForce = false ) override;

    void SetProject( PROJECT* aProject ) { m_project = aProject; }

    void SaveFileState( const wxString& aFileName, const WINDOW_SETTINGS* aWindowCfg, bool aOpen );

    const PROJECT_FILE_STATE* GetFileState( const wxString& aFileName );

    void ClearFileState();

    /**
     * @return true if it should be safe to auto-save this file without user action
     */
    bool ShouldAutoSave() const { return !m_wasMigrated && !m_isFutureFormat; }

protected:
    wxString getFileExt() const override
    {
        return FILEEXT::ProjectLocalSettingsFileExtension;
    }

    wxString getLegacyFileExt() const override
    {
        return wxT( "NO_SUCH_FILE_EXTENSION" );
    }

public:

    /**
     * Project scope
     */

    /// File based state
    std::vector<PROJECT_FILE_STATE> m_files;

    std::vector<wxString>           m_OpenJobSets;

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

    /// The current net color mode
    NET_COLOR_MODE m_NetColorMode;

    /// The state of the net inspector panel
    PANEL_NET_INSPECTOR_SETTINGS m_NetInspectorPanel;

    /// The current setting for whether to automatically adjust track widths to match
    bool m_AutoTrackWidth;

    /// How zones are drawn
    ZONE_DISPLAY_MODE m_ZoneDisplayMode;

    /// Whether Zone fill should always be solid for performance with large boards.
    bool m_PrototypeZoneFill;

    double m_TrackOpacity;     ///< Opacity override for all tracks
    double m_ViaOpacity;       ///< Opacity override for all types of via
    double m_PadOpacity;       ///< Opacity override for SMD pads and PTH
    double m_ZoneOpacity;      ///< Opacity override for filled zones
    double m_ShapeOpacity;     ///< Opacity override for graphic shapes
    double m_ImageOpacity;     ///< Opacity override for user images

    /**
     * A list of netnames that have been manually hidden in the board editor.
     * Currently, hiding nets means hiding the ratsnest for those nets.
     */
    std::vector<wxString> m_HiddenNets;
    std::set<wxString> m_HiddenNetclasses;

    /// State of the selection filter widgets
    PCB_SELECTION_FILTER_OPTIONS m_PcbSelectionFilter;
    SCH_SELECTION_FILTER_OPTIONS m_SchSelectionFilter;

    /// Collapsed nodes in the schematic hierarchy navigator
    std::vector<wxString> m_SchHierarchyCollapsed;

    // Upstream git repo info
    wxString m_GitRepoUsername;
    wxString m_GitRepoType;
    wxString m_GitSSHKey;

    /// If true, KiCad will not use Git integration for this project even if a .git directory exists
    bool m_GitIntegrationDisabled;

private:
    /// A link to the owning project
    PROJECT* m_project;

    bool m_wasMigrated;
};

#endif
