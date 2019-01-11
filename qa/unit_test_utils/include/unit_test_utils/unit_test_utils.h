/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef UNIT_TEST_UTILS__H
#define UNIT_TEST_UTILS__H

#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include <functional>

/**
 * If HAVE_EXPECTED_FAILURES is defined, this means that
 * boost::unit_test::expected_failures is available.
 *
 * Wrap expected-to-fail tests with this to prevent them being compiled
 * on platforms with older (<1.59) Boost versions.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#if BOOST_VERSION >= 105900
#define HAVE_EXPECTED_FAILURES
#endif

/**
 * BOOST_TEST, while extremely handy, is not available in Boost < 1.59.
 * Undef it here to prevent use. Using it can cause older packaging like
 * Ubuntu LTS (which is on Boost 1.58) to fail.
 *
 * Use BOOST_CHECK_{EQUAL,NE,etc} instead.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#undef BOOST_TEST


#if BOOST_VERSION < 105900

/*
 * BOOST_TEST_INFO is not available before 1.59. It's not critical for
 * test pass/fail, it's just info, so just pass along to a logging
 * function.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#define BOOST_TEST_INFO( A ) BOOST_TEST_MESSAGE( A )

/*
 *
 * BOOST_TEST_CONTEXT provides scoped info, but again, only after 1.59.
 * Replacing with a call to BOOST_TEST_MESSAGE will work, and the
 * scoping will still work for newer boosts.
 *
 * This can be removed when our minimum boost version is 1.59 or higher.
 */
#define BOOST_TEST_CONTEXT( A ) BOOST_TEST_MESSAGE( A );

#endif

/*
 * Define a helper to make it easier to use the right namespace for
 * defining the print helpers like this:
 *
 * template<>
 * struct BOOST_PRINT::print_log_value< MY_TYPE >
 */
#if BOOST_VERSION < 105900
namespace BOOST_PRINT = boost::test_tools;
#else
namespace BOOST_PRINT = boost::test_tools::tt_detail;
#endif
#endif // UNIT_TEST_UTILS__H