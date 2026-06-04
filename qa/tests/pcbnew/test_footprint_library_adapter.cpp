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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <footprint_library_adapter.h>
#include <libraries/library_manager.h>


namespace
{

/**
 * Exposes a way to seed a LOAD_ERROR sentinel entry into the project-scope map,
 * exactly as the async loader leaves it when a library fails to load (status set
 * to LOAD_ERROR, plugin and row left null).
 */
class TEST_FOOTPRINT_LIBRARY_ADAPTER : public FOOTPRINT_LIBRARY_ADAPTER
{
public:
    using FOOTPRINT_LIBRARY_ADAPTER::FOOTPRINT_LIBRARY_ADAPTER;

    void SeedLoadError( const wxString& aNickname )
    {
        m_libraries[aNickname].status.load_status = LOAD_STATUS::LOAD_ERROR;
    }
};

} // namespace


BOOST_AUTO_TEST_SUITE( FootprintLibraryAdapter )


/**
 * Regression test for a null-plugin dereference crash.
 *
 * A footprint library that fails to load leaves a LOAD_ERROR sentinel with a null
 * plugin in the adapter's map. IsFootprintLibWritable() is called for every table
 * row by the footprint editor and chooser, so it must report such a library as not
 * writable instead of dereferencing the null plugin.
 */
BOOST_AUTO_TEST_CASE( IsFootprintLibWritableHandlesFailedLoad )
{
    LIBRARY_MANAGER                manager;
    TEST_FOOTPRINT_LIBRARY_ADAPTER adapter( manager );

    adapter.SeedLoadError( wxS( "BadLib" ) );

    BOOST_CHECK_EQUAL( adapter.IsFootprintLibWritable( wxS( "BadLib" ) ), false );

    // A library that was never even attempted must also be safe.
    BOOST_CHECK_EQUAL( adapter.IsFootprintLibWritable( wxS( "NeverSeen" ) ), false );
}


BOOST_AUTO_TEST_SUITE_END()
