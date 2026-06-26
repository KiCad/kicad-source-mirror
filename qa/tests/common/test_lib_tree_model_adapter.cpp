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

#include <lib_tree_model_adapter.h>
#include <project.h>
#include <settings/app_settings.h>


/**
 * Minimal concrete adapter exposing the loaded column widths.  Construction runs
 * loadColumnConfig() against the injected settings, which is the path that previously
 * applied a corrupt persisted width.
 */
class TEST_LIB_TREE_MODEL_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    TEST_LIB_TREE_MODEL_ADAPTER( APP_SETTINGS_BASE::LIB_TREE& aSettings ) :
            LIB_TREE_MODEL_ADAPTER( nullptr, wxT( "pinned" ), aSettings )
    {
    }

    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::SYMBOL_LIB; }

    int LoadedWidth( const wxString& aColumn ) const
    {
        auto it = m_colWidths.find( aColumn );
        return it != m_colWidths.end() ? it->second : -1;
    }
};


BOOST_AUTO_TEST_SUITE( LibTreeModelAdapter )

/**
 * A persisted column width can become corrupt after a mixed-DPI monitor change (issue 24702
 * reported width 427218649), which pushes the tree content off-screen and leaves the
 * symbol/footprint chooser blank.  Loading such a value must fall back to the sane default
 * instead of applying it, while a legitimate persisted width is honoured.
 */
BOOST_AUTO_TEST_CASE( CorruptPersistedColumnWidthIgnoredOnLoad )
{
    APP_SETTINGS_BASE::LIB_TREE settings;
    settings.column_widths[ wxT( "Item" ) ] = 427218649;
    settings.column_widths[ wxT( "Description" ) ] = 450;

    TEST_LIB_TREE_MODEL_ADAPTER* adapter = new TEST_LIB_TREE_MODEL_ADAPTER( settings );

    // The corrupt width must not overwrite the default; the legitimate one must be applied.
    BOOST_CHECK_EQUAL( adapter->LoadedWidth( wxT( "Item" ) ), 300 );
    BOOST_CHECK_EQUAL( adapter->LoadedWidth( wxT( "Description" ) ), 450 );

    adapter->DecRef();
}

BOOST_AUTO_TEST_SUITE_END()
