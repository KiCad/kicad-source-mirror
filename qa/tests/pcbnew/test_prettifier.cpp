/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include <fmt/std.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <plugins/kicad/pcb_plugin.h>
#include <board.h>
#include <footprint.h>
#include <settings/settings_manager.h>


struct PRETTIFIER_TEST_FIXTURE
{
    PRETTIFIER_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
};


BOOST_FIXTURE_TEST_CASE( FootprintPrettifier, PRETTIFIER_TEST_FIXTURE )
{
    std::vector<wxString> footprints = {
        "Reverb_BTDR-1V",
        "Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal"
    };

    std::unique_ptr<FOOTPRINT> original, converted;
    PCB_PLUGIN plugin;

    std::string tempLibPath = fmt::format( "{}/prettifier.pretty",
                                           std::filesystem::temp_directory_path() );
    std::filesystem::remove_all( tempLibPath );
    std::filesystem::create_directory( tempLibPath );

    for( const wxString& footprint : footprints )
    {
        BOOST_TEST_CONTEXT( footprint.ToStdString() )
        {
            std::string inPath = fmt::format( "{}prettifier/{}.kicad_mod",
                                              KI_TEST::GetPcbnewTestDataDir(),
                                              footprint.ToStdString() );

            BOOST_CHECK_NO_THROW( original = KI_TEST::ReadFootprintFromFileOrStream( inPath ) );
            BOOST_REQUIRE( original.get() );

            BOOST_CHECK_NO_THROW( plugin.FootprintSave( tempLibPath, original.get() ) );

            std::string newPath = fmt::format( "{}/{}.kicad_mod", tempLibPath,
                                               original->GetFPIDAsString().ToStdString() );

            BOOST_CHECK_NO_THROW( converted = KI_TEST::ReadFootprintFromFileOrStream( newPath ) );
            BOOST_REQUIRE( converted.get() );

            // Hack around the fact that PAD::operator== compares footprint UUIDs, even though
            // these UUIDs cannot be preserved through a round-trip
            const_cast<KIID&>( converted->m_Uuid ) = original->m_Uuid;

            // File should parse the same way
            BOOST_REQUIRE( *original == *converted );

            // And the formatting should match
            std::string goldenPath = fmt::format( "{}prettifier/{}_formatted.kicad_mod",
                                                  KI_TEST::GetPcbnewTestDataDir(),
                                                  footprint.ToStdString() );
            {
            std::ifstream test( newPath );
            std::ifstream golden( goldenPath );

            BOOST_REQUIRE( !test.fail() && !golden.fail() );
            BOOST_REQUIRE_MESSAGE( test.tellg() == golden.tellg(), "File sizes didn't match!" );

            test.seekg( 0, std::ifstream::beg );
            golden.seekg( 0, std::ifstream::beg );

            BOOST_REQUIRE_MESSAGE( std::equal( std::istreambuf_iterator<char>( test.rdbuf() ),
                                               std::istreambuf_iterator<char>(),
                                               std::istreambuf_iterator<char>( golden.rdbuf() ) ),
                                   "Formatted footprints do not match!" );
            }

            std::filesystem::remove( newPath );
        }
    }
}

