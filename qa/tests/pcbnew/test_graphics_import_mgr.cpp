/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <import_gfx/graphics_import_mgr.h>
#include <import_gfx/graphics_import_plugin.h>

#include <regex>

/**
 * Declares a struct as the Boost test fixture.
 */
BOOST_AUTO_TEST_SUITE( GraphicsImportMgr )

static bool pluginHandlesExt( const GRAPHICS_IMPORT_PLUGIN& aPlugin, const std::string& aExt )
{
    const auto exts = aPlugin.GetFileExtensions();

    for( const auto& ext : exts )
    {
        std::regex ext_reg( ext );

        if( std::regex_match( aExt, ext_reg ) )
            return true;
    }

    return false;
}

struct TYPE_TO_EXTS
{
    // The type of the plugin
    GRAPHICS_IMPORT_MGR::GFX_FILE_T m_type;

    /// The list of extensions we expect this plugin to handle
    std::vector<std::string> m_exts;

    /// The name of the plugin
    std::string m_name;
};

const static std::vector<TYPE_TO_EXTS> type_to_ext_cases = {
    {
            GRAPHICS_IMPORT_MGR::GFX_FILE_T::DXF,
            { "dxf" },
            "AutoCAD DXF",
    },
    {
            GRAPHICS_IMPORT_MGR::GFX_FILE_T::SVG,
            { "svg" },
            "Scalable Vector Graphics",
    },
};

/**
 * Check we can look a plugin up by type and get the right one
 */
BOOST_AUTO_TEST_CASE( SelectByType )
{
    GRAPHICS_IMPORT_MGR mgr( {} );

    for( const auto& c : type_to_ext_cases )
    {
        auto plugin = mgr.GetPlugin( c.m_type );

        BOOST_CHECK( !!plugin );

        if( plugin )
        {
            for( const auto& ext : c.m_exts )
            {
                BOOST_CHECK_MESSAGE( pluginHandlesExt( *plugin, ext ),
                        "Plugin '" << plugin->GetName() << "' handles extension: " << ext );
            }
        }
    }
}

/**
 * Check we can look a plugin up by ext and get the right one
 */
BOOST_AUTO_TEST_CASE( SelectByExt )
{
    GRAPHICS_IMPORT_MGR mgr( {} );

    for( const auto& c : type_to_ext_cases )
    {
        for( const auto& ext : c.m_exts )
        {
            auto plugin = mgr.GetPluginByExt( wxString( ext ) );

            BOOST_CHECK( !!plugin );

            if( plugin )
            {
                // This is an ugly way to check the right plugin,
                // as we have to keep a list of expected strings (the plugins
                // don't report any kind of other unique identifier).
                // But it's quick and dirty and it's good enough!
                BOOST_CHECK_EQUAL( c.m_name, plugin->GetName() );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
