/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <diff_merge/pcb_diff_canvas_context.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <settings/settings_manager.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( PcbDiffCanvasContext )


BOOST_AUTO_TEST_CASE( ForcedPainterColorIgnoresPcbLayerColors )
{
    const KIGFX::COLOR4D       changeColor( 0.38, 0.38, 0.38, 0.42 );
    KIGFX::PCB_RENDER_SETTINGS settings;
    ConfigurePcbDiffContextRenderSettings( settings, changeColor );

    // Unchanged board renders a single neutral grey, so real per-layer colors are
    // ignored: every copper layer resolves to the same colour.
    KIGFX::COLOR4D fcu = settings.GetColor( static_cast<const BOARD_ITEM*>( nullptr ), F_Cu );
    KIGFX::COLOR4D bcu = settings.GetColor( static_cast<const BOARD_ITEM*>( nullptr ), B_Cu );

    BOOST_CHECK_CLOSE( fcu.r, bcu.r, 0.001 );
    BOOST_CHECK_CLOSE( fcu.g, bcu.g, 0.001 );
    BOOST_CHECK_CLOSE( fcu.b, bcu.b, 0.001 );

    // The forced change colour is applied to highlighted (changed) items.
    KIGFX::COLOR4D brightened = settings.GetLayerColor( LAYER_BRIGHTENED );

    BOOST_CHECK_CLOSE( brightened.r, changeColor.r, 0.001 );
    BOOST_CHECK_CLOSE( brightened.g, changeColor.g, 0.001 );
    BOOST_CHECK_CLOSE( brightened.b, changeColor.b, 0.001 );
    BOOST_CHECK_CLOSE( brightened.a, changeColor.a, 0.001 );
}


BOOST_AUTO_TEST_CASE( CollectorIncludesFootprintChildren )
{
    SETTINGS_MANAGER       settings;
    std::unique_ptr<BOARD> board;
    KI_TEST::LoadBoard( settings, "complex_hierarchy", board );
    BOOST_REQUIRE( board );

    std::vector<KIGFX::VIEW_ITEM*> items = CollectBoardDiffContextItems( *board );

    bool foundPad = false;

    for( KIGFX::VIEW_ITEM* item : items )
    {
        if( dynamic_cast<PAD*>( item ) )
        {
            foundPad = true;
            break;
        }
    }

    BOOST_CHECK( foundPad );
}


BOOST_AUTO_TEST_SUITE_END()
