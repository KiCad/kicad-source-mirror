/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
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

#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <eeschema_settings.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <schematic.h>
#include <sch_line.h>
#include <sch_painter.h>
#include <sch_screen.h>
#include <sch_view.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/ee_grid_helper.h>


namespace
{
class HEADLESS_GAL : public KIGFX::GAL
{
public:
    explicit HEADLESS_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aOptions ) :
            GAL( aOptions )
    {
    }
};


class TEST_EE_GRID_HELPER : public EE_GRID_HELPER
{
public:
    explicit TEST_EE_GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
            EE_GRID_HELPER( aToolMgr )
    {
    }

    SNAP_MANAGER&                SnapManager() { return getSnapManager(); }
    const std::optional<ANCHOR>& SnapItem() const { return m_snapItem; }
};


struct EE_SNAP_FIXTURE
{
    SETTINGS_MANAGER                     settingsManager;
    KIGFX::GAL_DISPLAY_OPTIONS           options;
    HEADLESS_GAL                         gal;
    KIGFX::SCH_VIEW                      view;
    KIGFX::SCH_PAINTER                   painter;
    EESCHEMA_SETTINGS                    settings;
    TOOL_MANAGER                         manager;
    std::unique_ptr<SCHEMATIC>           schematic;
    std::unique_ptr<TEST_EE_GRID_HELPER> helper;
    SCH_LINE*                            wire = nullptr;

    EE_SNAP_FIXTURE() :
            gal( options ),
            view( nullptr ),
            painter( &gal )
    {
        KI_TEST::LoadSchematic( settingsManager, wxS( "../common/git/mdw_ctrl_board/pic_sockets" ), schematic );

        const VECTOR2I start( schIUScale.mmToIU( 259.08 ), schIUScale.mmToIU( 157.48 ) );
        const VECTOR2I end( schIUScale.mmToIU( 259.08 ), schIUScale.mmToIU( 119.38 ) );

        for( SCH_ITEM* item : schematic->RootScreen()->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsWire() && line->GetStartPoint() == start && line->GetEndPoint() == end )
                wire = line;
        }

        BOOST_REQUIRE( wire );

        const double grid = schIUScale.MilsToIU( 50 );

        gal.SetGridSize( { grid, grid } );
        gal.SetGridOrigin( { 0, 0 } );
        gal.SetWorldUnitLength( SCH_WORLD_UNIT );
        gal.SetScreenSize( { 2000, 1200 } );
        view.SetGAL( &gal );
        view.SetPainter( &painter );
        view.SetCenter( VECTOR2D( start ) );
        view.SetScale( 10.0 );

        // Only the wire is shown so its own anchor is the one retained
        view.Add( wire );

        manager.SetEnvironment( schematic.get(), &view, nullptr, &settings, nullptr );
        helper = std::make_unique<TEST_EE_GRID_HELPER>( &manager );
    }

    /// End the wire's lifetime and build a different one in its storage, as the heap did on windows-01
    SNAP_TARGET_ID ReplaceWire()
    {
        std::destroy_at( wire );
        std::construct_at( wire, VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 10 ) ), LAYER_WIRE );
        return SnapTargetId( wire->m_Uuid );
    }
};


VECTOR2I mm( double aX, double aY )
{
    return VECTOR2I( schIUScale.mmToIU( aX ), schIUScale.mmToIU( aY ) );
}


bool carriesTarget( const SNAP_RESULT& aResult, const SNAP_TARGET_ID& aTarget )
{
    for( const SNAP_STABLE_ID& id : aResult.accepted )
    {
        if( id.target == aTarget )
            return true;
    }

    return false;
}
} // namespace


BOOST_FIXTURE_TEST_SUITE( EeSnapPipeline, EE_SNAP_FIXTURE )


BOOST_AUTO_TEST_CASE( ConstructionSnapKeepsTargetOfReplacedWire )
{
    helper->AddConstructionItems( { wire }, true, true );
    BOOST_REQUIRE( !helper->SnapManager().GetConstructionItems().empty() );

    const SNAP_TARGET_ID original = SnapTargetId( wire->m_Uuid );
    const SNAP_TARGET_ID replacement = ReplaceWire();

    SNAP_RESULT result = helper->ResolveSnap( mm( 259.08, 165 ), GRID_WIRES );

    BOOST_CHECK( carriesTarget( result, original ) );
    BOOST_CHECK( !carriesTarget( result, replacement ) );
}


BOOST_AUTO_TEST_CASE( RetainedSnapKeepsTargetOfReplacedWire )
{
    const VECTOR2I       end = mm( 259.08, 157.48 );
    const SNAP_TARGET_ID original = SnapTargetId( wire->m_Uuid );

    helper->ResolveSnap( end, GRID_WIRES );
    BOOST_REQUIRE( helper->SnapItem() && helper->SnapItem()->InvolvesItem( *wire ) );

    const SNAP_TARGET_ID replacement = ReplaceWire();

    SNAP_RESULT result = helper->ResolveSnap( end + VECTOR2I( 1, 0 ), GRID_WIRES );

    BOOST_CHECK( carriesTarget( result, original ) );
    BOOST_CHECK( !carriesTarget( result, replacement ) );
}


BOOST_AUTO_TEST_SUITE_END()
