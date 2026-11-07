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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <text_eval/text_eval_wrapper.h>

BOOST_AUTO_TEST_CASE( EmbeddedSiPrefixesAndUppercaseExponents )
{
    const std::pair<wxString, wxString> cases[] = {
        { "@{4k7}",  "4700" },
        { "@{1m2}",  "0.0012" },
        { "@{1E3}",  "1000" },
        { "@{1e3}",  "1000" },
        { "@{1E-3}", "0.001" },
        { "@{1E+3}", "1000" },
        { "@{1E}",   "1e+18" },
        { "@{4.7k}", "4700" }
    };
    for( const auto& [expression, expected] : cases )
    {
        EXPRESSION_EVALUATOR evaluator;
        BOOST_CHECK_EQUAL( evaluator.Evaluate( expression ), expected );
        BOOST_CHECK( !evaluator.HasErrors() );
    }
}
