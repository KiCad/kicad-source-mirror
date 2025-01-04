/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file include/plugins/kicad_plugin.h
 * defines the most basic functions which all kicad plugins must implement.
 * In the implementation the definitions must make use of the KICAD_PLUGIN_EXPORT
 * to ensure symbol visibility.
 */

#ifndef KICAD_PLUGIN_H
#define KICAD_PLUGIN_H

#ifndef _WIN32
    #ifndef KICAD_PLUGIN_EXPORT
    #define KICAD_PLUGIN_EXPORT extern "C" __attribute__((__visibility__("default")))
    #endif
#else
    #ifndef KICAD_PLUGIN_EXPORT
    #define KICAD_PLUGIN_EXPORT extern "C" __declspec( dllexport )
    #endif
#endif

/**
 * Return the name of the implemented plugin class, for example 3DPLUGIN.
 *
 * This should be implemented in a source module which is compiled as part of every implementation
 * of a specific plugin class.
 *
 * @return is the NULL-terminated UTF-8 string representing the plugin class.
 */
KICAD_PLUGIN_EXPORT char const* GetKicadPluginClass( void );

/**
 * Retrieve the version of the Plugin Class.
 *
 * This value is used to ensure API compatibility of a plugin as per typical practice. This must
 * be implemented in a source module which is compiled as part of every implementation of a
 * specific plugin class.
 *
 * @param Major will hold the Plugin Class Major version.
 * @param Minor will hold the Plugin Class Minor version.
 * @param Revision will hold the Plugin Class Revision.
 * @param Patch will hold the Plugin Class Patch level.
 */
KICAD_PLUGIN_EXPORT void GetClassVersion( unsigned char* Major, unsigned char* Minor,
                                          unsigned char* Patch, unsigned char* Revision );

/**
 * Return true if the class version reported by the Plugin Loader is compatible with the specific
 * implementation of a plugin.
 *
 * This function must be defined by each specific plugin and it is the plugin developer's
 * responsibility to ensure that the Plugin is in fact compatible with the Plugin Loader. The
 * Plugin Loader shall reject any Plugin with a different Major number regardless of the return
 * value of this function.
 */
KICAD_PLUGIN_EXPORT bool CheckClassVersion( unsigned char Major, unsigned char Minor,
                                            unsigned char Patch, unsigned char Revision );

/**
 * Return the name of the plugin instance, for example IDFv3.
 *
 * This string may be used to check for name conflicts or to display informational messages about
 * loaded plugins. This method must be implemented in specific instantiations of a plugin class.
 *
 * @return is the NULL-terminated UTF-8 string representing the plugin name.
 */
KICAD_PLUGIN_EXPORT const char* GetKicadPluginName( void );


/**
 * Retrieve the version of the instantiated plugin for informational purposes.
 *
 * Do not confuse this with GetClassVersion which is used to determine API compatibility.
 *
 * @param Major will hold the Plugin Major version.
 * @param Minor will hold the Plugin Minor version.
 * @param Patch will hold the Plugin Patch level.
 * @param Revision will hold the Plugin Revision.
 */
KICAD_PLUGIN_EXPORT void GetPluginVersion( unsigned char* Major, unsigned char* Minor,
                                           unsigned char* Patch, unsigned char* Revision );

#endif  // KICAD_PLUGIN_H
