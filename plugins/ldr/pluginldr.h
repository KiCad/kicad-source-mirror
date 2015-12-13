/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * defines the most basic functions which all kicad plugin loaders require.
 */


#ifndef PLUGINLDR_H
#define PLUGINLDR_H

#include <dlfcn.h>
#include <string>
#include <wx/string.h>

// helper functions to link functions
#ifdef _WIN32
#define LINK_ITEM( funcPtr, funcType, funcName ) \
    funcPtr = (funcType) GetProcAddress( m_dlHandle, funcName );
#else
#define LINK_ITEM( funcPtr, funcType, funcName ) \
    *(void**) (&funcPtr) = dlsym( m_dlHandle, funcName );
#endif

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
private:
    bool ok;    // set TRUE if all functions are linked
    GET_PLUGIN_CLASS    m_getPluginClass;
    GET_CLASS_VERSION   m_getClassVersion;
    CHECK_CLASS_VERSION m_checkClassVersion;
    GET_PLUGIN_NAME     m_getPluginName;
    GET_VERSION         m_getVersion;

    wxString m_fileName;        // name of last opened Plugin

protected:
    std::string m_error;    // error message

    /**
     * Function open
     * opens a plugin of the specified class and links the extensions
     * required by kicad_plugin. Returns true on success otherwise
     * false.
     */
    bool open( const wxString& aFullFileName, const char* aPluginClass );

    /**
     * Function close
     * nullifies internal pointers in preparation for closing the plugin
     */
    void close( void );

    /**
     * Function reopen
     * reopens a plugin and returns true on success
     */
    bool reopen( void );

    // handle to the opened plugin
    #ifdef _WIN32
    HMODULE m_dlHandle;
    #else
    void* m_dlHandle;
    #endif

public:
    KICAD_PLUGIN_LDR();
    virtual ~KICAD_PLUGIN_LDR();

    /**
     * Function GetLoaderVersion
     * returns the version information of the Plugin Loader
     * for plugin compatibility checking.
     */
    virtual void GetLoaderVersion( unsigned char* Major, unsigned char* Minor,
        unsigned char* Revision, unsigned char* Patch ) const = 0;

    /**
     * Function Open
     * opens a plugin of the given class, performs version compatibility checks,
     * and links all required functions.
     *
     * @return true on success, otherwise false and a message may be accessible
     * via GetLastError()
     */
    virtual bool Open( const wxString& aFullFileName ) = 0;

    /**
     * Function Close
     * cleans up and closes/unloads the plugin
     */
    virtual void Close( void ) = 0;

    /**
     * Function GetLastError
     * returns the value of the internal error string
     */
    std::string GetLastError( void ) const;

    // the following functions are the equivalent of those required by kicad_plugin.h

    // returns the Plugin Class or NULL if no plugin loaded
    char const* GetKicadPluginClass( void );

    // returns false if no plugin loaded
    bool GetClassVersion( unsigned char* Major, unsigned char* Minor,
        unsigned char* Revision, unsigned char* Patch );

    // returns false if the class version check fails or no plugin is loaded
    bool CheckClassVersion( unsigned char Major, unsigned char Minor,
        unsigned char Revision, unsigned char Patch );

    // returns the Plugin Name or NULL if no plugin loaded
    const char* GetKicadPluginName( void );

    // returns false if no plugin is loaded
    bool GetVersion( unsigned char* Major, unsigned char* Minor,
        unsigned char* Revision, unsigned char* Patch );
};

#endif  // PLUGINLDR_H
