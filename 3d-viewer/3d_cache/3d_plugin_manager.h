/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file 3d_plugin_manager.h
 * manages 3D model plugins
 */

#ifndef PLUGIN_MANAGER_3D_H
#define PLUGIN_MANAGER_3D_H

#include <map>
#include <list>
#include <string>
#include <wx/string.h>

class wxWindow;
class KICAD_PLUGIN_LDR_3D;
class SCENEGRAPH;

class S3D_PLUGIN_MANAGER
{
public:
    S3D_PLUGIN_MANAGER();
    virtual ~S3D_PLUGIN_MANAGER();

    /**
     * Return the list of file filters; this will contain at least the default
     * "All Files (*.*)|*.*" and the file filters supported by any available plugins.
     *
     * @return a pointer to the internal filter list.
     */
    std::list< wxString > const* GetFileFilters( void ) const noexcept;

    SCENEGRAPH* Load3DModel( const wxString& aFileName, std::string& aPluginInfo );

    /**
     * Iterate through all discovered plugins and closes them to reclaim memory.
     *
     * The individual plugins will be automatically reloaded as calls are made to load
     * specific models.
     */
    void ClosePlugins( void );

    /**
     * Check the given tag and returns true if the plugin named in the tag is not loaded
     * or the plugin is loaded and the version matches.
     */
    bool CheckTag( const char* aTag );

private:
    /// load plugins
    void loadPlugins( void );

    /// list potential plugins
    void listPlugins( const wxString& aPath, std::list< wxString >& aPluginList );

    /// check the existence of a plugin name and add it to the list
    void checkPluginName( const wxString& aPath, std::list< wxString >& aPluginList );

    /// check the existence of a path and add it to the path search list
    void checkPluginPath( const wxString& aPath, std::list< wxString >& aSearchList );

    /// add an entry to the file filter list
    void addFilterString( const wxString& aFilterString );

    /// add entries to the extension map
    void addExtensionMap( KICAD_PLUGIN_LDR_3D* aPlugin );

    /// list of discovered plugins
    std::list< KICAD_PLUGIN_LDR_3D* > m_Plugins;

    /// mapping of extensions to available plugins
    std::multimap< const wxString, KICAD_PLUGIN_LDR_3D* > m_ExtMap;

    /// list of file filters
    std::list< wxString > m_FileFilters;
};

#endif  // PLUGIN_MANAGER_3D_H
