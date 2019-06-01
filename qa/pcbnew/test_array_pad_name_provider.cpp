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

        module->Add( pad.release() );
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
    bool                           m_using_module;
    std::vector<wxString>          m_existing_pads;
    std::unique_ptr<ARRAY_OPTIONS> m_arr_opts;
    std::vector<wxString>          m_exp_arr_names;
};


/**
 * Get Array Pad Name Provider cases when a module is looked
 * at to determine what names are available.
 */
std::vector<APNP_CASE> GetModuleAPNPCases()
{
    std::vector<APNP_CASE> cases;

    auto opts = std::make_unique<ARRAY_GRID_OPTIONS>();

    // simple linear numbering
    opts->m_2dArrayNumbering = false;
    opts->m_pri_axis.SetOffset( 1 );
    opts->m_pri_axis.SetAxisType( ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC );

    cases.push_back( {
            "Simple linear, skip some",
            true,
            { "1", "3" },
            std::move( opts ),
            { "2", "4", "5", "6", "7" },
    } );

    // one without a module
    opts = std::make_unique<ARRAY_GRID_OPTIONS>();

    // simple linear numbering (again)
    opts->m_2dArrayNumbering = false;
    opts->m_pri_axis.SetOffset( 1 );
    opts->m_pri_axis.SetAxisType( ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC );

    cases.push_back( {
            "Simple linear, no module",
            false,
            {}, // not used
            std::move( opts ),
            { "1", "2", "3", "4", "5" },
    } );

    // Grid numberings with skips don't make a lot of sense, there is
    // no particular contract made for them

    return cases;
}


/**
 * Check that an #ARRAY_PAD_NAME_PROVIDER provides the right names
 * @param aProvider the provider
 * @param aExpNames ordered list of expected names
 */
void CheckPadNameProvider( ARRAY_PAD_NAME_PROVIDER& aProvider, std::vector<wxString> aExpNames )
{
    std::vector<wxString> got_names;

    for( unsigned i = 0; i < aExpNames.size(); ++i )
    {
        got_names.push_back( aProvider.GetNextPadName() );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS(
            aExpNames.begin(), aExpNames.end(), got_names.begin(), got_names.end() );
}


BOOST_AUTO_TEST_CASE( ModuleCases )
{
    for( const auto& c : GetModuleAPNPCases() )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            std::unique_ptr<MODULE> module;

            if( c.m_using_module )
            {
                module = ModuleWithPads( c.m_existing_pads );
            }

            ARRAY_PAD_NAME_PROVIDER apnp( module.get(), *c.m_arr_opts );

            CheckPadNameProvider( apnp, c.m_exp_arr_names );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
