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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <eda_base_frame.h>


BOOST_AUTO_TEST_SUITE( FrameCloseBlock )


BOOST_AUTO_TEST_CASE( TruthTable )
{
    using enum FRAME_CLOSE_BLOCK;

    struct ROW
    {
        bool              frameIsModal;
        bool              canVeto;
        bool              nestedLoop;
        bool              underLoopOwner;
        bool              modalDialog;
        bool              quasiModal;
        FRAME_CLOSE_BLOCK expected;
    };

    const ROW rows[] = {
        { false, true,  false, false, false, false, NONE },
        { false, false, false, false, false, true,  QUASI_MODAL },
        { false, false, false, false, true,  false, MODAL_DIALOG },
        { true,  true,  true,  false, true,  false, MODAL_DIALOG },
        { true,  true,  true,  false, false, false, NONE },
        { false, true,  true,  false, false, false, NESTED_LOOP },
        { false, false, true,  false, false, false, NONE },
        { false, true,  true,  true,  true,  false, NONE },
        { false, true,  false, true,  true,  false, MODAL_DIALOG },

        // A forced close under the loop owner goes through even with a registry modal open, so the window still
        // saves its settings
        { false, false, true,  true,  true,  false, NONE },

        // A quasi-modal blocks even under the loop owner, and a registry dialog is raised ahead of the loop owner
        { false, false, true,  true,  false, true,  QUASI_MODAL },
        { false, true,  true,  false, true,  false, MODAL_DIALOG },
    };

    for( size_t i = 0; i < std::size( rows ); ++i )
    {
        const ROW& r = rows[i];

        BOOST_TEST_CONTEXT( "row " << i )
        {
            BOOST_CHECK( GetFrameCloseBlock( r.frameIsModal, r.canVeto, r.nestedLoop, r.underLoopOwner,
                                             r.modalDialog, r.quasiModal )
                         == r.expected );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
