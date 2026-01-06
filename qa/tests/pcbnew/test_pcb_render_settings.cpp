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

/**
 * @file test_pcb_render_settings.cpp
 * Test suite for PCB_RENDER_SETTINGS
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcb_painter.h>
#include <pcb_point.h>
#include <layer_ids.h>
#include <settings/color_settings.h>

class TEST_PCB_RENDER_SETTINGS_FIXTURE
{
public:
    KIGFX::PCB_RENDER_SETTINGS m_settings;
    COLOR_SETTINGS             m_colorSettings;

    TEST_PCB_RENDER_SETTINGS_FIXTURE()
    {
        m_colorSettings.SetColor( LAYER_POINTS, KIGFX::COLOR4D( 1.0, 0.0, 1.0, 1.0 ) );

        for( int i = 0; i < PCB_LAYER_ID_COUNT; i++ )
            m_colorSettings.SetColor( i, KIGFX::COLOR4D( 0.5, 0.5, 0.5, 1.0 ) );

        m_settings.LoadColors( &m_colorSettings );
    }
};


BOOST_FIXTURE_TEST_SUITE( PcbRenderSettings, TEST_PCB_RENDER_SETTINGS_FIXTURE )


/**
 * Test that PCB_RENDER_SETTINGS::GetColor returns a visible color for POINT layers.
 *
 * This is a regression test for issue #22634 where pressing ESC caused footprint
 * points to vanish because UpdateAllLayersColor() was requesting the color for
 * the virtual POINT layer (LAYER_POINT_START + boardLayer), which had no color
 * defined and returned transparent.
 */
BOOST_AUTO_TEST_CASE( PointLayerColorIsVisible )
{
    PCB_POINT point( nullptr );
    point.SetLayer( F_SilkS );

    int pointLayer = POINT_LAYER_FOR( F_SilkS );

    KIGFX::COLOR4D color = m_settings.GetColor( &point, pointLayer );

    BOOST_CHECK_MESSAGE( color.a > 0.0,
                         "Color for POINT layer should not be transparent (alpha was "
                         + std::to_string( color.a ) + ")" );

    BOOST_CHECK_MESSAGE( color == m_colorSettings.GetColor( LAYER_POINTS ),
                         "POINT layer color should match LAYER_POINTS color" );
}


/**
 * Test that multiple board layers all map to LAYER_POINTS color.
 */
BOOST_AUTO_TEST_CASE( AllPointLayersMapToLayerPoints )
{
    KIGFX::COLOR4D expectedColor = m_colorSettings.GetColor( LAYER_POINTS );

    std::vector<PCB_LAYER_ID> testLayers = { F_Cu, B_Cu, F_SilkS, B_SilkS, Edge_Cuts, User_1 };

    for( PCB_LAYER_ID boardLayer : testLayers )
    {
        PCB_POINT point( nullptr );
        point.SetLayer( boardLayer );

        int pointLayer = POINT_LAYER_FOR( boardLayer );
        KIGFX::COLOR4D color = m_settings.GetColor( &point, pointLayer );

        BOOST_CHECK_MESSAGE( color == expectedColor,
                             "POINT layer for " + std::to_string( boardLayer )
                             + " should have LAYER_POINTS color" );
    }
}


BOOST_AUTO_TEST_SUITE_END()
