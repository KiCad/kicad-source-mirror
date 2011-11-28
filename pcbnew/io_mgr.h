#ifndef IO_MGR_H_
#define IO_MGR_H_

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors.
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

#include <wx/string.h>
#include <wx/hashmap.h>
#include <richio.h>


// http://docs.wxwidgets.org/trunk/classwx_hash_map.html
WX_DECLARE_STRING_HASH_MAP( wxString, PROPERTIES );

class BOARD;
class SCHEMATIC;
class PLUGIN;


/**
 * Class IO_MGR
 * is factory which returns an instance of a PLUGIN DSO/DLL.
 */
class IO_MGR
{
public:

    /**
     * Enum PCB_FILE_T
     * is a set of file types that the IO_MGR knows about, and for which there
     * has been a plugin written.
     */
    enum PCB_FILE_T
    {
        KICAD,
        // EAGLE,
        // ALTIUM,
        // etc.
    };

    /**
     * Function PluginFind
     * returns a PLUGIN which the caller can use to import, export, save, or load
     * design documents.  The returned PLUGIN, may be reference counted, so please
     * call PluginRelease() when you are done using the returned PLUGIN.
     *
     * @param aFileType is from PCB_FILE_T and tells which plugin to find.
     *
     * @return PLUGIN* - the plugin corresponding to aFileType or NULL if not found.
     *  Caller owns the returned object, and must call PluginRelease when done using it.
     */
    static PLUGIN* PluginFind( PCB_FILE_T aFileType );

    /**
     * Function PluginRelease
     * releases a PLUGIN back to the system, and may cause it to be unloaded from memory.
     *
     * @param aPlugin is the one to be released, and which is no longer usable.
     */
    static void PluginRelease( PLUGIN* aPlugin );

    /**
     * Function ShowType
     * returns a brief name for a plugin, given aFileType enum.
     */
    static const wxString& ShowType( PCB_FILE_T aFileType );

    /**
     * Function Load
     * finds the requested plugin and loads a BOARD, or throws an exception trying.
     *
     * @param aFileType is the type of file to load.
     *
     * @param aFileName is the name of the file to load.
     *
     * @param aAppendToMe is an existing BOARD to append to, use NULL if fresh
     *  board load wanted.
     *
     * @param aProperties is an associative array that allows the caller to
     *  pass additional tuning parameters to the plugin.
     *
     * @return BOARD* - caller owns it, never NULL because exception thrown if error.
     *
     * @throw IO_ERROR if the pluging cannot be found, file cannot be found,
     *  or file cannot be loaded.
     */
    static BOARD* Load( PCB_FILE_T aFileType, const wxString& aFileName,
            BOARD* aAppendToMe = NULL, PROPERTIES* aProperties = NULL );

    // etc.
};


/**
 * Class PLUGIN
 * is a base class that BOARD loading and saving plugins should derive from.
 * Implementations can provide either Load() or Save() functions, or both.
 * PLUGINs throw exceptions, so it is best that you wrap your calls to these
 * functions in a try catch block, and also do the switching to stardard C locale
 * and back, outside the region in which an exception can be thrown.  This means
 * the PLUGINs do not deal with the locale, the caller does.
 *
 * <pre>
 *
 *   // Switch the locale to standard C (needed to read floating point numbers
 *   // like 1.3)
 *
 *   SetLocaleTo_C_standard();
 *   try
 *   {
 *        pi->Load(...);
 *   }
 *   catch( IO_ERROR ioe )
 *   {
 *        // grab text from ioe, show in error window.
 *   }
 *   SetLocaleTo_Default();       // revert to the current locale
 *
 * </pre>
 */
class PLUGIN
{
public:

    virtual ~PLUGIN() {}

    /**
     * Class RELEASER
     * releases a PLUGIN in the context of a potential thrown exception, through
     * its destructor.
     */
    class RELEASER
    {
        PLUGIN* plugin;

    public:
        RELEASER( PLUGIN* aPlugin = NULL ) :
            plugin( aPlugin )
        {
        }

        ~RELEASER()
        {
            if( plugin )
                IO_MGR::PluginRelease( plugin );
        }

        operator PLUGIN* ()
        {
            return plugin;
        }

        PLUGIN* operator -> ()
        {
            return plugin;
        }
    };

    virtual const wxString& Name() = 0;

    //-----<BOARD STUFF>----------------------------------------------------

    /**
     * Function Load
     * loads a board file from some input file format that this implementation
     * knows about.
     *
     * @param aFileName is the name of the file to load and may be foreign in
     *  nature or native in nature.
     *
     * @param aAppendToMe is an existing BOARD to append to but is often NULL
     *  meaning do not append.
     *
     * @param aProperties is an associative array that can be used to tell the
     *  loader how to load the file, because it can take any number of
     *  additional named arguments that the plugin is known to support.
     *
     * @return BOARD* - the successfully loaded board, and caller owns it.
     *
     * @throw IO_ERROR if there is a problem loading, and its contents should
     *  say what went wrong.
     */
    virtual BOARD*  Load( const wxString& aFileName, BOARD* aAppendToMe,
                        PROPERTIES* aProperties = NULL );

    /**
     * Function Save
     * will write a full aBoard to a storage file in a format that only this
     * implementation knows about.  Or it can be used to write a portion of
     * aBoard to a special kind of export file.
     *
     * @param aFileName is the name of a file to save to on disk.
     * @param aBoard is the BOARD document (data tree) to save or export to disk.
     *
     * @param aProperties is an associative array that can be used to tell the
     *  saver how to save the file, because it can take any number of
     *  additional named arguments that the plugin is known to support.
     *
     * @throw IO_ERROR if there is a problem loading.
     */
    virtual void Save( const wxString* aFileName, BOARD* aBoard,
                    PROPERTIES* aProperties = NULL );

    //-----</BOARD STUFF>---------------------------------------------------

#if 0
    ///--------- Should split into two PLUGIN base types here, rather than being combined like this

    //-----<SCHEMATIC STUFF>------------------------------------------------

    /**
     * Function Load
     * loads a file from some special input file format that
     * only this implementation knows about.
     * @param aFileName is the name of the file to load and may be foreign in nature or native in nature.
     * @param aAppendToMe is an existing SCHEMATIC to append to but may be NULL.
     */
    virtual SCHEMATIC*  Load( const wxString& aFileName, SCHEMATIC* aAppendToMe,
                            PROPERTIES* aProperties = NULL )
    {
        // not pure virtual so that plugins only have to implement
        // Load() or Save() but not both.
    }

    /**
     * Function Save
     * will write aBoard to a storage file in a format that only this
     * implementation knows about.
     *
     * @param aFileName is the name of a file to save to on disk.
     *
     * @param aBoard is the SCHEMATIC document (ram data tree) to save or export to disk.
     */
    virtual void Save( const wxString* aFileName, SCHEMATIC* aSchematic,
                    PROPERTIES* aProperties = NULL )
    {
        // not pure virtual so that plugins only have to implement
        // Load() or Save() but not both.
    }

    //-----</SCHEMATIC STUFF>----------------------------------------------
#endif
};

#endif // IO_MGR_H_

