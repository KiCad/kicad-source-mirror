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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>
#include <deque>

#include <board.h>
#include <pcb_shape.h>
#include <eda_item.h>
#include <locale_io.h>
#include <geometry/shape_poly_set.h>
#include <import_gfx/dxf_import_plugin.h>
#include <import_gfx/graphics_importer_pcbnew.h>
#include <tools/convert_tool.h>

#include <pcbnew_utils/board_file_utils.h>


/**
 * Exposes CONVERT_TOOL's private chaining routine so the regression test can drive the real
 * production code rather than a reimplementation.
 */
class CONVERT_TOOL_TEST_FIXTURE
{
public:
    SHAPE_POLY_SET MakePolys( const std::deque<EDA_ITEM*>& aItems )
    {
        return m_tool.makePolysFromChainedSegs( aItems, CENTERLINE );
    }

private:
    CONVERT_TOOL m_tool;
};


BOOST_FIXTURE_TEST_SUITE( ConvertTool, CONVERT_TOOL_TEST_FIXTURE )


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/22127
 *
 * A Fusion360-exported DXF outline made of lines and splines chains into a single closed loop only
 * if sub-tolerance gaps at the spline junctions are bridged.  The spline endpoints in this file
 * differ from their neighbours' by up to ~52 IU (well under the 100 IU chaining tolerance), so
 * "Create Polygon from Selection" must still produce one closed polygon covering every shape.
 */
BOOST_AUTO_TEST_CASE( CreatePolygonFromSplineOutline )
{
    BOARD board;

    DXF_IMPORT_PLUGIN        plugin;
    GRAPHICS_IMPORTER_PCBNEW importer( &board );

    plugin.SetImporter( &importer );
    plugin.SetUnit( DXF_IMPORT_UNITS::MM );
    importer.SetLayer( Edge_Cuts );

    wxString dxfPath = wxString::FromUTF8(
            KI_TEST::GetPcbnewTestDataDir() + "../common/import_gfx/issue22127_fusion360_splines.dxf" );

    {
        LOCALE_IO dummy;
        BOOST_REQUIRE( plugin.Load( dxfPath ) );
        BOOST_REQUIRE( plugin.Import() );
    }

    std::deque<EDA_ITEM*> items;

    for( std::unique_ptr<EDA_ITEM>& item : importer.GetItems() )
        items.push_back( item.get() );

    // The outline is built from lines and splines (splines become a chain of bezier PCB_SHAPEs).
    BOOST_REQUIRE_MESSAGE( items.size() >= 6, "Expected several imported shapes, got " << items.size() );

    for( EDA_ITEM* item : items )
        item->ClearFlags( SKIP_STRUCT );

    SHAPE_POLY_SET result = MakePolys( items );

    BOOST_CHECK_MESSAGE( result.OutlineCount() == 1,
                         "Spline+line outline should chain into exactly one closed polygon, got "
                                 << result.OutlineCount() );

    if( result.OutlineCount() == 1 )
    {
        const SHAPE_LINE_CHAIN& outline = result.COutline( 0 );

        BOOST_CHECK_MESSAGE( outline.IsClosed(), "Resulting polygon must be closed" );
        BOOST_CHECK_MESSAGE( outline.Area() > 0.0,
                             "Resulting polygon must enclose a non-zero area" );
    }

    // A closed subset loop would also satisfy the checks above, so confirm that every imported
    // shape was actually chained.  makePolysFromChainedSegs marks consumed items with SKIP_STRUCT.
    size_t chainedItems = std::count_if( items.begin(), items.end(),
                                         []( EDA_ITEM* aItem )
                                         {
                                             return aItem->GetFlags() & SKIP_STRUCT;
                                         } );

    BOOST_CHECK_EQUAL( chainedItems, items.size() );
}


BOOST_AUTO_TEST_SUITE_END()
