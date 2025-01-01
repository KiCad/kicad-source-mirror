/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file pluginldr.h
 * defines the most basic functions which all KiCad plugin loaders require.
 */


#ifndef PLUGINLDR_H
#define PLUGINLDR_H

#include <string>
#include <wx/dynlib.h>
#include <wx/string.h>

// Mask for plugin loader tracing.
extern const wxChar* const tracePluginLoader;

#define MASK_PLUGINLDR wxT( "PLUGIN_LOADER" )


// helper function to link functions in the plugin
#define LINK_ITEM( funcPtr, funcType, funcName ) \
    funcPtr = (funcType) m_PluginLoader.GetSymbol( wxT( funcName ) )

// typedefs of the functions exported by the 3D Plugin Class
typedef char const* (*GET_PLUGIN_CLASS) ( void );

typedef void (*GET_CLASS_VERSION) ( unsigned char*, unsigned char*,
    unsigned char*, unsigned char* );

typedef bool (*CHECK_CLASS_VERSION) ( unsigned char, unsigned char,
    unsigned char, unsigned char );

typedef const char* (*GET_PLUGIN_NAME) ( void );

typedef void (*GET_VERSION) ( unsigned char*, unsigned char*,
    unsigned char*, unsigned char* );


class KICAD_PLUGIN_LDR
{
public:
    KICAD_PLUGIN_LDR();
    virtual ~KICAD_PLUGIN_LDR();

    /**
     * Return the version information of the Plugin Loader for plugin compatibility checking.
     */
    virtual void GetLoaderVersion( unsigned char* Major, unsigned char* Minor,
                                   unsigned char* Patch, unsigned char* Revision ) const = 0;

    /**
     * Open a plugin of the given class, performs version compatibility checks,
     * and links all required functions.
     *
     * @return true on success or false if failure. An error message may be accessible
     *         via GetLastError()
     */
    virtual bool Open( const wxString& aFullFileName ) = 0;

    /**
     * Clean up and closes/unloads the plugin.
     */
    virtual void Close( void ) = 0;

    /**
     * Return the value of the internal error string.
     */
    std::string GetLastError( void ) const;

    // Return the Plugin Class or NULL if no plugin loaded.
    char const* GetKicadPluginClass( void );

    // Return false if no plugin loaded.
    bool GetClassVersion( unsigned char* Major, unsigned char* Minor,
                          unsigned char* Patch, unsigned char* Revision );

    // Return false if the class version check fails or no plugin is loaded.
    bool CheckClassVersion( unsigned char Major, unsigned char Minor,
                            unsigned char Patch, unsigned char Revision );

    // Return the Plugin Name or NULL if no plugin loaded.
    const char* GetKicadPluginName( void );

    // Return false if no plugin is loaded.
    bool GetVersion( unsigned char* Major, unsigned char* Minor,
                     unsigned char* Patch, unsigned char* Revision );

    void GetPluginInfo( std::string& aPluginInfo );

protected:
    /**
     * Open a plugin of the specified class and links the extensions required by kicad_plugin.
     *
     * @return true on success otherwise false.
     */
    bool open( const wxString& aFullFileName, const char* aPluginClass );

    /**
     * Nullify internal pointers in preparation for closing the plugin.
     */
    void close( void );

    /**
     * Reopen a plugin.
     *
     * @return true on success or false on failure.
     */
    bool reopen( void );

    std::string m_error;    // error message

    // the plugin loader
    wxDynamicLibrary m_PluginLoader;

private:
    bool                ok;                       // set TRUE if all functions are linked
    GET_PLUGIN_CLASS    m_getPluginClass;
    GET_CLASS_VERSION   m_getClassVersion;
    CHECK_CLASS_VERSION m_checkClassVersion;
    GET_PLUGIN_NAME     m_getPluginName;
    GET_VERSION         m_getVersion;
    wxString            m_fileName;               // name of last opened Plugin
    std::string         m_pluginInfo;             // Name:Version tag for plugin
};

#endif  // PLUGINLDR_H
