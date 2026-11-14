/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/test/unit_test.hpp>

#include <eda_cursor_broadcaster.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>


class TEST_GAL : public KIGFX::GAL
{
public:
    TEST_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aOptions ) :
            GAL( aOptions )
    {
    }

    void ForceDisplayCursor( bool aEnabled ) { m_forceDisplayCursor = aEnabled; }
};


class TEST_CURSOR_OWNER : public EDA_CURSOR_OWNER
{
public:
    TEST_CURSOR_OWNER() { EDA_CURSOR_BROADCASTER::Instance().Register( this ); }

    ~TEST_CURSOR_OWNER() override { EDA_CURSOR_BROADCASTER::Instance().Unregister( this ); }

    bool IsCursorInCanvas() const override { return m_inCanvas; }

    void HideCursorInCanvas() override { m_inCanvas = false; }

    void Enter() { m_inCanvas = true; }

private:
    bool m_inCanvas = false;
};


BOOST_AUTO_TEST_SUITE( GalCursor )


BOOST_AUTO_TEST_CASE( HiddenOutsideCanvas )
{
    KIGFX::GAL_DISPLAY_OPTIONS options;
    TEST_GAL                   gal( options );

    gal.SetCursorEnabled( true );
    BOOST_CHECK( !gal.IsCursorEnabled() );

    gal.SetCursorInCanvas( true );
    BOOST_CHECK( gal.IsCursorEnabled() );

    gal.SetCursorInCanvas( false );
    BOOST_CHECK( !gal.IsCursorEnabled() );

    gal.SetCursorEnabled( false );
    gal.ForceDisplayCursor( true );
    BOOST_CHECK( !gal.IsCursorEnabled() );

    gal.SetCursorInCanvas( true );
    BOOST_CHECK( gal.IsCursorEnabled() );
}


BOOST_AUTO_TEST_CASE( EnterHidesEveryOtherLiveOwner )
{
    TEST_CURSOR_OWNER first;
    TEST_CURSOR_OWNER entering;

    first.Enter();

    {
        TEST_CURSOR_OWNER destroyed;
        destroyed.Enter();
    }

    entering.Enter();
    EDA_CURSOR_BROADCASTER::Instance().NotifyCursorEntered( &entering );

    BOOST_CHECK( !first.IsCursorInCanvas() );
    BOOST_CHECK( entering.IsCursorInCanvas() );
}


BOOST_AUTO_TEST_SUITE_END()
