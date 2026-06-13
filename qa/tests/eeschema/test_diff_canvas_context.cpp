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
#include <schematic_utils/schematic_file_util.h>

#include <diff_merge/sch_diff_canvas_context.h>

#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( SchDiffCanvasContext )


BOOST_AUTO_TEST_CASE( ForcedPainterColorIgnoresSchematicLayerColors )
{
    const KIGFX::COLOR4D changeColor( 0.38, 0.38, 0.38, 0.42 );
    SCH_RENDER_SETTINGS settings;
    ConfigureSchDiffContextRenderSettings( settings, changeColor );

    // Unchanged schematic renders a single neutral grey, so real per-layer colors
    // are ignored: different layers resolve to the same colour.
    KIGFX::COLOR4D device = settings.GetColor( nullptr, LAYER_DEVICE );
    KIGFX::COLOR4D wire = settings.GetColor( nullptr, LAYER_WIRE );

    BOOST_CHECK_CLOSE( device.r, wire.r, 0.001 );
    BOOST_CHECK_CLOSE( device.g, wire.g, 0.001 );
    BOOST_CHECK_CLOSE( device.b, wire.b, 0.001 );

    // The forced change colour is applied to highlighted (changed) items.
    KIGFX::COLOR4D brightened = settings.GetLayerColor( LAYER_BRIGHTENED );

    BOOST_CHECK_CLOSE( brightened.r, changeColor.r, 0.001 );
    BOOST_CHECK_CLOSE( brightened.g, changeColor.g, 0.001 );
    BOOST_CHECK_CLOSE( brightened.b, changeColor.b, 0.001 );
    BOOST_CHECK_CLOSE( brightened.a, changeColor.a, 0.001 );
}


BOOST_AUTO_TEST_CASE( CollectorIncludesActualSchematicItems )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue18606/issue18606", schematic );
    BOOST_REQUIRE( schematic );

    // The fixture keeps its symbols on a sub-sheet, so collect from every screen.
    SCH_SCREENS screens( &schematic->Root() );
    bool        foundSymbol = false;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen && !foundSymbol; screen = screens.GetNext() )
    {
        for( KIGFX::VIEW_ITEM* item : CollectSchematicDiffContextItems( *schematic, screen ) )
        {
            if( dynamic_cast<SCH_SYMBOL*>( item ) )
            {
                foundSymbol = true;
                break;
            }
        }
    }

    BOOST_CHECK( foundSymbol );
}


BOOST_AUTO_TEST_SUITE_END()
