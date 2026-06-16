/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

/**
 * @file test_project_local_settings.cpp
 *
 * Guards issue #24193 where opening a V8 project in V10 left footprint
 * reference and value text invisible because the PRL schema 3->5 migration
 * mistranslated the legacy numeric layer ids.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <project.h>
#include <project/project_local_settings.h>
#include <settings/layer_settings_utils.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;


class PROJECT_LOCAL_SETTINGS_FIXTURE
{
public:
    PROJECT_LOCAL_SETTINGS_FIXTURE()
    {
        m_tempDir = fs::temp_directory_path() / "kicad_project_local_settings_test";
        fs::remove_all( m_tempDir );
        fs::create_directories( m_tempDir );
    }

    ~PROJECT_LOCAL_SETTINGS_FIXTURE()
    {
        fs::remove_all( m_tempDir );
    }

    void writeAndLoadProject( const std::string& aProjectName, const std::string& aPrlContents,
                              SETTINGS_MANAGER& aSettingsManager )
    {
        fs::path projectDir = m_tempDir / aProjectName;
        fs::create_directories( projectDir );

        fs::path prlPath = projectDir
                           / ( aProjectName + "." + FILEEXT::ProjectLocalSettingsFileExtension );

        {
            std::ofstream prlFile( prlPath );
            prlFile << aPrlContents;
        }

        std::string proContents = R"({ "meta": { "filename": ")" + aProjectName + R"(.kicad_pro", "version": 3 } })";

        fs::path proPath = projectDir / ( aProjectName + ".kicad_pro" );

        {
            std::ofstream proFile( proPath );
            proFile << proContents;
        }

        BOOST_REQUIRE( aSettingsManager.LoadProject( wxString( proPath.string() ) ) );
    }

    fs::path m_tempDir;
};


BOOST_FIXTURE_TEST_SUITE( ProjectLocalSettings, PROJECT_LOCAL_SETTINGS_FIXTURE )


/**
 * https://gitlab.com/kicad/code/kicad/-/issues/24193
 */
BOOST_AUTO_TEST_CASE( V8PrlFootprintTextStaysVisible )
{
    // Verbatim v8 export from the issue #24193 reproduction case.
    std::string prlContent = R"({
        "board": {
            "active_layer": 0,
            "active_layer_preset": "All Layers",
            "auto_track_width": true,
            "hidden_netclasses": [],
            "hidden_nets": [],
            "high_contrast_mode": 0,
            "net_color_mode": 1,
            "opacity": {
                "images": 0.6,
                "pads": 1.0,
                "tracks": 1.0,
                "vias": 1.0,
                "zones": 0.6
            },
            "selection_filter": {
                "dimensions": true,
                "footprints": true,
                "graphics": true,
                "keepouts": true,
                "lockedItems": false,
                "otherItems": true,
                "pads": true,
                "text": true,
                "tracks": true,
                "vias": true,
                "zones": true
            },
            "visible_items": [
                0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19,
                20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 32, 33, 34, 35,
                36, 39, 40
            ],
            "visible_layers": "fffffff_ffffffff",
            "zone_display_mode": 0
        },
        "meta": {
            "filename": "v8_project.kicad_prl",
            "version": 3
        },
        "project": {
            "files": []
        }
    })";

    SETTINGS_MANAGER settingsManager;
    writeAndLoadProject( "v8_project", prlContent, settingsManager );

    PROJECT_LOCAL_SETTINGS& localSettings = settingsManager.Prj().GetLocalSettings();

    BOOST_CHECK_MESSAGE(
            localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_FP_REFERENCES ) ),
            "LAYER_FP_REFERENCES should be visible after migrating a V8 PRL" );

    BOOST_CHECK_MESSAGE(
            localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_FP_VALUES ) ),
            "LAYER_FP_VALUES should be visible after migrating a V8 PRL" );

    BOOST_CHECK_MESSAGE(
            localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_FP_TEXT ) ),
            "LAYER_FP_TEXT should be visible after migrating a V8 PRL" );
}


/**
 * Offset 5 maps to LAYER_FP_TEXT through the frozen table, not the live enum
 * where commit 43e327b6 shifted it to 6.  The negative assertions prove the
 * migration consumed the numeric list rather than falling back to defaults.
 */
BOOST_AUTO_TEST_CASE( MinimalVisibleItemsAppliesMigration )
{
    // Offsets 5 (FP_TEXT), 17 (FP_VALUES), 18 (FP_REFERENCES) in the v8 enum.
    std::string prlContent = R"({
        "board": {
            "visible_items": [5, 17, 18],
            "visible_layers": "fffffff_ffffffff"
        },
        "meta": {
            "filename": "minimal_project.kicad_prl",
            "version": 3
        },
        "project": { "files": [] }
    })";

    SETTINGS_MANAGER settingsManager;
    writeAndLoadProject( "minimal_project", prlContent, settingsManager );

    PROJECT_LOCAL_SETTINGS& localSettings = settingsManager.Prj().GetLocalSettings();

    BOOST_CHECK( localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_FP_TEXT ) ) );
    BOOST_CHECK( localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_FP_VALUES ) ) );
    BOOST_CHECK( localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_FP_REFERENCES ) ) );

    BOOST_CHECK( !localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_TRACKS ) ) );
    BOOST_CHECK( !localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_ZONES ) ) );
    BOOST_CHECK( !localSettings.m_VisibleItems.test( GAL_LAYER_INDEX( LAYER_PADS ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
