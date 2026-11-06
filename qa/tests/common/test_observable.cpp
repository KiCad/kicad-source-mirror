/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <core/observable.h>


namespace
{
struct WATCHER
{
    void Ping() {}
};
} // namespace


BOOST_AUTO_TEST_SUITE( Observable )

BOOST_AUTO_TEST_CASE( MoveAssignedLinkLeavesOldObservable )
{
    UTIL::OBSERVABLE<WATCHER> first;
    UTIL::OBSERVABLE<WATCHER> second;
    WATCHER                   watcher;

    UTIL::LINK link = first.Subscribe( &watcher );
    link = second.Subscribe( &watcher );

    BOOST_CHECK_EQUAL( first.size(), 0 );
    BOOST_CHECK_EQUAL( second.size(), 1 );
}

BOOST_AUTO_TEST_SUITE_END()
