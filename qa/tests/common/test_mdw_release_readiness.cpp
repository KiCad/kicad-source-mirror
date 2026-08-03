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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <mdw/client/mdw_release_readiness.h>


BOOST_AUTO_TEST_SUITE( MdwReleaseReadinessTests )


// One step at a time, in the order the user must take them
BOOST_AUTO_TEST_CASE( ReadinessOrdersSaveLockBranchCommit )
{
    for( bool dirty : { false, true } )
    {
        for( bool modified : { false, true } )
        {
            for( bool baseKnown : { false, true } )
            {
                for( MDW_BASELINE_HOLD hold :
                     { MDW_BASELINE_HOLD::NONE, MDW_BASELINE_HOLD::UNCOMMITTED, MDW_BASELINE_HOLD::DETACHED,
                       MDW_BASELINE_HOLD::NO_REMOTE, MDW_BASELINE_HOLD::LOCKED } )
                {
                    MDW_PROJECT_STATE state;
                    state.locally_modified = modified;
                    state.base_revision_id = baseKnown ? wxString( wxS( "abc123" ) ) : wxString();

                    MDW_RELEASE_STEP   step = MDW_RELEASE_STEP::READY;
                    MDW_RELEASE_ACTION action = MDW_RELEASE_ACTION::RELEASE;

                    if( dirty )
                    {
                        step = MDW_RELEASE_STEP::SAVE;
                        action = MDW_RELEASE_ACTION::SAVE_ALL;
                    }
                    else if( hold == MDW_BASELINE_HOLD::LOCKED )
                    {
                        step = MDW_RELEASE_STEP::LOCKED;
                        action = MDW_RELEASE_ACTION::NONE;
                    }
                    else if( hold == MDW_BASELINE_HOLD::DETACHED )
                    {
                        step = MDW_RELEASE_STEP::NOT_ON_BRANCH;
                        action = MDW_RELEASE_ACTION::NONE;
                    }
                    else if( hold == MDW_BASELINE_HOLD::UNCOMMITTED || modified || !baseKnown )
                    {
                        step = MDW_RELEASE_STEP::COMMIT;
                        action = MDW_RELEASE_ACTION::COMMIT;
                    }
                    else if( hold == MDW_BASELINE_HOLD::NO_REMOTE )
                    {
                        step = MDW_RELEASE_STEP::NO_REMOTE;
                        action = MDW_RELEASE_ACTION::NONE;
                    }

                    MDW_RELEASE_READINESS readiness =
                            MdwReleaseReadiness( dirty, state, hold, wxS( "carol" ), MDW_CHECKIN_MODEL::WORKING_COPY );

                    BOOST_TEST_CONTEXT( "dirty " << dirty << " modified " << modified << " base " << baseKnown
                                                 << " hold " << static_cast<int>( hold ) )
                    {
                        BOOST_CHECK( readiness.step == step );
                        BOOST_CHECK( readiness.action == action );
                        BOOST_CHECK( !readiness.summary.IsEmpty() );
                        BOOST_CHECK( readiness.summary.Contains( wxS( "carol" ) )
                                     == ( step == MDW_RELEASE_STEP::LOCKED ) );
                    }
                }
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
