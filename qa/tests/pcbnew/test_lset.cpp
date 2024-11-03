/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <layer_ids.h>
#include <lset.h>


BOOST_AUTO_TEST_SUITE( LSet )

struct LSETS_TO_TEST
{
    LSET lset;

    std::string expectedFmtHex;
    std::string expectedFmtBin;
};


const static std::vector<LSETS_TO_TEST> type_to_ext_cases = {
    { LSET( { F_Cu, F_Fab } ), "00000000_00000000_00000008_00000001",
            "0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|"
            "0000_0000|0000_0000|0000_0000|0000_1000|0000_0000|0000_0000|0000_0000|0000_0001" },
    { LSET( { In14_Cu, B_Adhes, Rescue } ), "00000000_00000000_00000020_40000800",
      "0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|"
      "0000_0000|0000_0000|0000_0000|0010_0000|0100_0000|0000_0000|0000_1000|0000_0000" }
};


BOOST_AUTO_TEST_CASE( FmtHex )
{
    for( const auto& c : type_to_ext_cases )
    {
        BOOST_CHECK_EQUAL( c.expectedFmtHex, c.lset.FmtHex() );
    }
}


BOOST_AUTO_TEST_CASE( FmtBin )
{
    for( const auto& c : type_to_ext_cases )
    {
        BOOST_CHECK_EQUAL( c.expectedFmtBin, c.lset.FmtBin() );
    }
}
// Utility macro to test layer name
#define TEST_LAYER_NAME(layer_id, expected_name) \
    BOOST_CHECK_EQUAL(LSET::Name(layer_id), wxString(expected_name))

// Test standard predefined layers
BOOST_AUTO_TEST_CASE(LSETNamePredefinedLayers)
{
    TEST_LAYER_NAME(F_Cu, "F.Cu");
    TEST_LAYER_NAME(B_Cu, "B.Cu");
    TEST_LAYER_NAME(F_SilkS, "F.SilkS");
    TEST_LAYER_NAME(B_SilkS, "B.SilkS");
    TEST_LAYER_NAME(F_Mask, "F.Mask");
    TEST_LAYER_NAME(B_Mask, "B.Mask");
    TEST_LAYER_NAME(F_Adhes, "F.Adhes");
    TEST_LAYER_NAME(B_Adhes, "B.Adhes");
    TEST_LAYER_NAME(F_Paste, "F.Paste");
    TEST_LAYER_NAME(B_Paste, "B.Paste");
    TEST_LAYER_NAME(F_CrtYd, "F.CrtYd");
    TEST_LAYER_NAME(B_CrtYd, "B.CrtYd");
    TEST_LAYER_NAME(F_Fab, "F.Fab");
    TEST_LAYER_NAME(B_Fab, "B.Fab");
    TEST_LAYER_NAME(Dwgs_User, "Dwgs.User");
    TEST_LAYER_NAME(Cmts_User, "Cmts.User");
    TEST_LAYER_NAME(Eco1_User, "Eco1.User");
    TEST_LAYER_NAME(Eco2_User, "Eco2.User");
    TEST_LAYER_NAME(Edge_Cuts, "Edge.Cuts");
    TEST_LAYER_NAME(Margin, "Margin");
    TEST_LAYER_NAME(Rescue, "Rescue");
}
// Test NameToLayer function for internal copper layers
BOOST_AUTO_TEST_CASE(LSETNameToLayerInternalCuLayers)
{
    for (int i = 1; i <= 300; i++)
    {
        wxString layerName = wxString::Format("In%d.Cu", i);
        PCB_LAYER_ID expectedLayer = static_cast<PCB_LAYER_ID>(In1_Cu + (i - 1) * 2);
        BOOST_CHECK_EQUAL(LSET::NameToLayer(layerName), expectedLayer);
    }
}

// Test NameToLayer function for user-defined layers
BOOST_AUTO_TEST_CASE(LSETNameToLayerUserDefinedLayers)
{
    for (int i = 1; i <= 300; i++)
    {
        wxString layerName = wxString::Format("User.%d", i);
        PCB_LAYER_ID expectedLayer = static_cast<PCB_LAYER_ID>(User_1 + (i - 1) * 2);
        BOOST_CHECK_EQUAL(LSET::NameToLayer(layerName), expectedLayer);
    }
}

// Test NameToLayer function for predefined layers
BOOST_AUTO_TEST_CASE(LSETNameToLayerPredefinedLayers)
{
    std::vector<std::pair<wxString, PCB_LAYER_ID>> layerTests = {
        {"F.Cu", F_Cu},
        {"B.Cu", B_Cu},
        {"F.SilkS", F_SilkS},
        {"B.SilkS", B_SilkS},
        {"F.Mask", F_Mask},
        {"B.Mask", B_Mask},
        {"F.Adhes", F_Adhes},
        {"B.Adhes", B_Adhes},
        {"F.Paste", F_Paste},
        {"B.Paste", B_Paste},
        {"F.CrtYd", F_CrtYd},
        {"B.CrtYd", B_CrtYd},
        {"F.Fab", F_Fab},
        {"B.Fab", B_Fab},
        {"Dwgs.User", Dwgs_User},
        {"Cmts.User", Cmts_User},
        {"Eco1.User", Eco1_User},
        {"Eco2.User", Eco2_User},
        {"Edge.Cuts", Edge_Cuts},
        {"Margin", Margin},
        {"Rescue", Rescue}
    };

    for (const auto& test : layerTests)
    {
        wxString layerName = test.first;
        PCB_LAYER_ID expectedLayer = test.second;
        BOOST_CHECK_EQUAL(LSET::NameToLayer(layerName), expectedLayer);
    }
}

// Test dynamically generated user-defined layers
BOOST_AUTO_TEST_CASE(LSETNameUserDefinedLayers)
{
    for (int i = User_1; i <= User_9; i += 2) {
        wxString expected_name = wxString::Format( "User.%d", (i - Rescue) / 2 );
        wxString actual_name = LSET::Name( static_cast<PCB_LAYER_ID>( i ) );
        BOOST_CHECK_EQUAL( expected_name, actual_name );
    }
}

// Test dynamically generated internal copper layers
BOOST_AUTO_TEST_CASE(LSETNameInternalCuLayers)
{
    for (int i = In1_Cu; i <= In30_Cu; i += 2) {
        wxString expected_name = wxString::Format("In%d.Cu", (i - B_Cu) / 2);
        TEST_LAYER_NAME(static_cast<PCB_LAYER_ID>(i), expected_name);
    }
}

BOOST_AUTO_TEST_SUITE_END()
