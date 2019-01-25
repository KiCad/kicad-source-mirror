/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file test_array_pad_name_provider.cpp
 * Test suite for the #ARRAY_PAD_NAME_PROVIDER class
 */

#include <unit_test_utils/unit_test_utils.h>

#include <array_pad_name_provider.h> // UUT

#include <common.h> // make_unique

#include <class_module.h>
#include <class_pad.h>

/**
 * Make a module with a given list of named pads
 */
static std::unique_ptr<MODULE> ModuleWithPads( const std::vector<wxString> aNames )
{
    auto module = std::make_unique<MODULE>( nullptr );

    for( const auto& name : aNames )
    {
        auto pad = std::make_unique<D_PAD>( module.get() );

        pad->SetName( name );

        module->PadsList().PushBack( pad.release() );
    }

    return module;
}

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( ArrayPadNameProv )


struct APNP_CASE
{
    std::string                    m_case_name;
    std::vector<wxString>          m_existing_pads;
    std::unique_ptr<ARRAY_OPTIONS> m_arr_opts;
    std::vector<wxString>          m_exp_arr_names;
};


std::vector<APNP_CASE> GetAPNPCases()
{
    std::vector<APNP_CASE> cases;

    auto opts = std::make_unique<ARRAY_GRID_OPTIONS>();

    // simple linear numbering
    opts->m_2dArrayNumbering = false;
    opts->m_numberingOffsetX = 0;
    opts->m_priAxisNumType = ARRAY_OPTIONS::NUMBERING_TYPE_T::NUMBERING_NUMERIC;

    cases.push_back( {
            "Simple linear, skip some",
            { "1", "3" },
            std::move( opts ),
            { "2", "4", "5", "6", "7" },
    } );

    opts = std::make_unique<ARRAY_GRID_OPTIONS>();

    // Grid numberings with skips don't make a lot of sense, there is
    // no particular contract made for them

    return cases;
}

BOOST_AUTO_TEST_CASE( Cases )
{
    for( const auto& c : GetAPNPCases() )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            auto module = ModuleWithPads( c.m_existing_pads );

            ARRAY_PAD_NAME_PROVIDER apnp( module.get(), *c.m_arr_opts );

            std::vector<wxString> got_names;

            for( unsigned i = 0; i < c.m_exp_arr_names.size(); ++i )
            {
                got_names.push_back( apnp.GetNextPadName() );
            }

            BOOST_CHECK_EQUAL_COLLECTIONS( c.m_exp_arr_names.begin(), c.m_exp_arr_names.end(),
                    got_names.begin(), got_names.end() );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()