/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_PROJECT_FILE_H
#define KICAD_PROJECT_FILE_H

#include <common.h> // needed for wxstring hash template
#include <kiid.h>
#include <project/board_project_settings.h>
#include <settings/json_settings.h>
#include <settings/nested_settings.h>

class BOARD_DESIGN_SETTINGS;
class ERC_SETTINGS;
class NET_SETTINGS;
class SCHEMATIC_SETTINGS;
class TEMPLATES;

/**
 * For files like sheets and boards, a pair of that object KIID and display name
 * Display name is typically blank for the project root sheet
 */
typedef std::pair<KIID, wxString> FILE_INFO_PAIR;

/**
 * For storing PcbNew MRU paths of various types
 */
enum LAST_PATH_TYPE : unsigned int
{
    LAST_PATH_NETLIST = 0,
    LAST_PATH_STEP,
    LAST_PATH_IDF,
    LAST_PATH_VRML,
    LAST_PATH_SPECCTRADSN,
    LAST_PATH_GENCAD,

    LAST_PATH_SIZE
};

/**
 * The backing store for a PROJECT, in JSON format.
 *
 * There is either zero or one PROJECT_FILE for every PROJECT
 * (you can have a dummy PROJECT that has no file)
 */
class PROJECT_FILE : public JSON_SETTINGS
{
public:
    /**
     * Construct the project file for a project
     * @param aFullPath is the full disk path to the project
     */
    PROJECT_FILE( const wxString& aFullPath );

    virtual ~PROJECT_FILE() = default;

    virtual bool MigrateFromLegacy( wxConfigBase* aCfg ) override;

    bool SaveToFile( const wxString& aDirectory = "", bool aForce = false ) override;

    void SetProject( PROJECT* aProject )
    {
        m_project = aProject;
    }

    std::vector<FILE_INFO_PAIR>& GetSheets()
    {
        return m_sheets;
    }

    std::vector<FILE_INFO_PAIR>& GetBoards()
    {
        return m_boards;
    }

    NET_SETTINGS& NetSettings()
    {
        return *m_NetSettings;
    }

protected:
    wxString getFileExt() const override;

    wxString getLegacyFileExt() const override;

private:

    /// An list of schematic sheets in this project
    std::vector<FILE_INFO_PAIR> m_sheets;

    /// A list of board files in this project
    std::vector<FILE_INFO_PAIR> m_boards;

    /// A link to the owning PROJECT
    PROJECT* m_project;

    /**
     * Below are project-level settings that have not been moved to a dedicated file
     */
public:

    /**
     * Shared params, used by more than one application
     */

    /// The list of pinned symbol libraries
    std::vector<wxString> m_PinnedSymbolLibs;

    /// The list of pinned footprint libraries
    std::vector<wxString> m_PinnedFootprintLibs;

    std::map<wxString, wxString> m_TextVars;

    /**
     * Eeschema params
     */

    // Schematic ERC settings: lifecycle managed by SCHEMATIC
    ERC_SETTINGS* m_ErcSettings;

    // Schematic editing and misc settings: lifecycle managed by SCHEMATIC
    SCHEMATIC_SETTINGS* m_SchematicSettings;

    // Legacy parameters LibDir and LibName, for importing old projects
    wxString m_LegacyLibDir;

    wxArrayString m_LegacyLibNames;

    /**
     * CvPcb params
     */

    /// List of equivalence (equ) files used in the project
    std::vector<wxString> m_EquivalenceFiles;

    /**
     * PcbNew params
     */

    /// Page layout description file
    wxString m_BoardPageLayoutDescrFile;

    /// MRU path storage
    wxString m_PcbLastPath[LAST_PATH_SIZE];

    /**
     * Board design settings for this project's board.  This will be initialized by PcbNew after
     * loading a board so that BOARD_DESIGN_SETTINGS doesn't need to live in common for now.
     * Owned by the BOARD; may be null if a board isn't loaded: be careful
     */
    BOARD_DESIGN_SETTINGS* m_BoardSettings;

    /**
     * Net settings for this project (owned here)
     *
     * @note If we go multi-board in the future, we have to decide whether to use a global
     *       NET_SETTINGS or have one per board.  Right now I think global makes more sense
     *       (one set of schematics, one netlist partitioned into multiple boards)
     */
    std::shared_ptr<NET_SETTINGS> m_NetSettings;

    /// List of stored layer presets
    std::vector<LAYER_PRESET> m_LayerPresets;
};

// Specializations to allow directly reading/writing FILE_INFO_PAIRs from JSON

void to_json( nlohmann::json& aJson, const FILE_INFO_PAIR& aPair );

void from_json( const nlohmann::json& aJson, FILE_INFO_PAIR& aPair );

#endif
