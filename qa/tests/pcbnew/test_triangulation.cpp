/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct TRIANGULATE_TEST_FIXTURE
{
    TRIANGULATE_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( RegressionTriangulationTests, TRIANGULATE_TEST_FIXTURE )
{
    std::vector<wxString> tests = {
                                    "issue2568",
                                    "issue5313",
                                    "issue5320",
                                    "issue5567",
                                    "issue5830",
                                    "issue6039",
                                    "issue6260",
                                    "issue7086",
                                    "issue14294",
                                    "issue17559"
                                };


    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        for( ZONE* zone : m_board->Zones() )
        {
            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq())
            {
                auto poly = zone->GetFilledPolysList( layer );
                double area = poly->Area();
                double tri_area = 0.0;

                for( int ii = 0; ii < poly->TriangulatedPolyCount(); ii++ )
                {
                    const auto tri_poly = poly->TriangulatedPolygon( ii );

                    for( auto& tri : tri_poly->Triangles() )
                        tri_area += tri.Area();
                }

                double diff = std::abs( area - tri_area );

                // The difference should be less than 1e-4 square mm
                BOOST_CHECK_MESSAGE( diff < 1e8, "Triangulation area mismatch in " + relPath + " layer " + LayerName( layer ) + " difference: " + std::to_string( diff ) );
            }
        }
    }
}

