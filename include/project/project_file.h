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
class COMPONENT_CLASS_SETTINGS;
class TUNING_PROFILES;
class LAYER_PAIR_SETTINGS;
class SCHEMATIC_SETTINGS;
class TEMPLATES;

/**
 * For files like sheets and boards, a pair of that object KIID and display name
 * Display name is typically blank for the project root sheet
 */
typedef std::pair<KIID, wxString> FILE_INFO_PAIR;

/**
 * Information about a top-level schematic sheet
 */
struct TOP_LEVEL_SHEET_INFO
{
    KIID     uuid;          ///< Unique identifier for the sheet
    wxString name;          ///< Display name for the sheet
    wxString filename;      ///< Relative path to the sheet file

    TOP_LEVEL_SHEET_INFO() = default;

    TOP_LEVEL_SHEET_INFO( const KIID& aUuid, const wxString& aName, const wxString& aFilename )
        : uuid( aUuid ), name( aName ), filename( aFilename )
    {}

    bool operator==( const TOP_LEVEL_SHEET_INFO& aOther ) const
    {
        return uuid == aOther.uuid && name == aOther.name && filename == aOther.filename;
    }

    bool operator!=( const TOP_LEVEL_SHEET_INFO& aOther ) const
    {
        return !( *this == aOther );
    }
};

/**
 * For storing PcbNew MRU paths of various types
 */
enum LAST_PATH_TYPE : unsigned int
{
    LAST_PATH_FIRST = 0,
    LAST_PATH_NETLIST = LAST_PATH_FIRST,
    LAST_PATH_IDF,
    LAST_PATH_VRML,
    LAST_PATH_SPECCTRADSN,
    LAST_PATH_PLOT,

    LAST_PATH_SIZE
};

/**
 * The backing store for a PROJECT, in JSON format.
 *
 * There is either zero or one PROJECT_FILE for every PROJECT
 * (you can have a dummy PROJECT that has no file)
 */
class KICOMMON_API PROJECT_FILE : public JSON_SETTINGS
{
public:
    /**
     * Construct the project file for a project
     * @param aFullPath is the full disk path to the project
     */
    PROJECT_FILE( const wxString& aFullPath );

    virtual ~PROJECT_FILE() = default;

    virtual bool MigrateFromLegacy( wxConfigBase* aCfg ) override;

    bool LoadFromFile( const wxString& aDirectory = "" ) override;

    bool SaveToFile( const wxString& aDirectory = "", bool aForce = false ) override;

    bool SaveAs( const wxString& aDirectory, const wxString& aFile );

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

    std::vector<TOP_LEVEL_SHEET_INFO>& GetTopLevelSheets()
    {
        return m_topLevelSheets;
    }

    const std::vector<TOP_LEVEL_SHEET_INFO>& GetTopLevelSheets() const
    {
        return m_topLevelSheets;
    }

    std::shared_ptr<NET_SETTINGS>& NetSettings()
    {
        return m_NetSettings;
    }

    std::shared_ptr<COMPONENT_CLASS_SETTINGS>& ComponentClassSettings()
    {
        return m_ComponentClassSettings;
    }

    std::shared_ptr<TUNING_PROFILES>& TuningProfileParameters() { return m_tuningProfileParameters; }

    /**
     * @return true if it should be safe to auto-save this file without user action
     */
    bool ShouldAutoSave() const { return !m_wasMigrated && !m_isFutureFormat; }

protected:
    wxString getFileExt() const override;

    wxString getLegacyFileExt() const override;

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

    /// The list of pinned design block libraries
    std::vector<wxString> m_PinnedDesignBlockLibs;

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

    /// Bus alias definitions for the schematic project
    std::map<wxString, std::vector<wxString>> m_BusAliases;

    /**
     * CvPcb params
     */

    /// List of equivalence (equ) files used in the project
    std::vector<wxString> m_EquivalenceFiles;

    /**
     * PcbNew params
     */

    /// Drawing sheet file
    wxString m_BoardDrawingSheetFile;

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



    /**
     * Component class settings for the project (owned here)
     */
    std::shared_ptr<COMPONENT_CLASS_SETTINGS> m_ComponentClassSettings;

    /**
     * Tuning profile parameters for this project
     */
    std::shared_ptr<TUNING_PROFILES> m_tuningProfileParameters;

    std::vector<LAYER_PRESET>     m_LayerPresets;   /// List of stored layer presets
    std::vector<VIEWPORT>         m_Viewports;      /// List of stored viewports (pos + zoom)
    std::vector<VIEWPORT3D>       m_Viewports3D;    /// List of stored 3D viewports (view matrixes)
    std::vector<LAYER_PAIR_INFO>  m_LayerPairInfos; /// Layer pair list for the board

    struct IP2581_BOM             m_IP2581Bom;      /// IPC-2581 BOM settings

private:
    /**
     * Schema version 2: Bump for KiCad 9 layer numbering changes.
     *
     * Migrate layer presets to use new enum values for copper layers.
     */
    bool migrateSchema1To2();

    /**
     * Schema version 3: move layer presets to use named render layers.
     */
    bool migrateSchema2To3();

    /// An list of schematic sheets in this project
    std::vector<FILE_INFO_PAIR> m_sheets;

    /// A list of top-level schematic sheets in this project
    std::vector<TOP_LEVEL_SHEET_INFO> m_topLevelSheets;

    /// A list of board files in this project
    std::vector<FILE_INFO_PAIR> m_boards;

    /// A link to the owning PROJECT
    PROJECT* m_project;

    bool m_wasMigrated;
};

// Specializations to allow directly reading/writing FILE_INFO_PAIRs from JSON

void to_json( nlohmann::json& aJson, const FILE_INFO_PAIR& aPair );

void from_json( const nlohmann::json& aJson, FILE_INFO_PAIR& aPair );

// Specializations to allow directly reading/writing TOP_LEVEL_SHEET_INFO from JSON

void to_json( nlohmann::json& aJson, const TOP_LEVEL_SHEET_INFO& aInfo );

void from_json( const nlohmann::json& aJson, TOP_LEVEL_SHEET_INFO& aInfo );

#endif
