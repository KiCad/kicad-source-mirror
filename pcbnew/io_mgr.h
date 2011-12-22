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
class PLUGIN;


/**
 * Class IO_MGR
 * is a factory which returns an instance of a PLUGIN.
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
        // add your type here.

        // EAGLE,
        // ALTIUM,
        // etc.
    };

    /**
     * Function PluginFind
     * returns a PLUGIN which the caller can use to import, export, save, or load
     * design documents.  The returned PLUGIN, may be reference counted, so please
     * call PluginRelease() when you are done using the returned PLUGIN.  It may or
     * may not be code running from a DLL/DSO.
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
     * @param aPlugin is the one to be released, and which is no longer usable
     *  after calling this.
     */
    static void PluginRelease( PLUGIN* aPlugin );

    /**
     * Function ShowType
     * returns a brief name for a plugin, given aFileType enum.
     */
    static const wxString ShowType( PCB_FILE_T aFileType );

    /**
     * Function Load
     * finds the requested PLUGIN and if found, calls the PLUGIN->Load(..) funtion
     * on it using the arguments passed to this function.  After the PLUGIN->Load()
     * function returns, the PLUGIN is Released() as part of this call.
     *
     * @param aFileType is the PCB_FILE_T of file to load.
     *
     * @param aFileName is the name of the file to load.
     *
     * @param aAppendToMe is an existing BOARD to append to, use NULL if fresh
     *  board load is wanted.
     *
     * @param aProperties is an associative array that allows the caller to
     *  pass additional tuning parameters to the PLUGIN.
     *
     * @return BOARD* - caller owns it, never NULL because exception thrown if error.
     *
     * @throw IO_ERROR if the PLUGIN cannot be found, file cannot be found,
     *  or file cannot be loaded.
     */
    static BOARD* Load( PCB_FILE_T aFileType, const wxString& aFileName,
            BOARD* aAppendToMe = NULL, PROPERTIES* aProperties = NULL );

    /**
     * Function Save
     * will write either a full aBoard to a storage file in a format that this
     * implementation knows about, or it can be used to write a portion of
     * aBoard to a special kind of export file.
     *
     * @param aFileType is the PCB_FILE_T of file to save.
     *
     * @param aFileName is the name of a file to save to on disk.
     * @param aBoard is the BOARD document (data tree) to save or export to disk.
     *
     * @param aBoard is the in memory document tree from which to extract information
     *  when writing to \a aFileName.  The caller continues to own the BOARD, and
     *  the plugin should refrain from modifying the BOARD if possible.
     *
     * @param aProperties is an associative array that can be used to tell the
     *  saver how to save the file, because it can take any number of
     *  additional named tuning arguments that the plugin is known to support.
     *  The caller continues to own this object (plugin may not delete it), and
     *  plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem saving or exporting.
     */
    static void Save( PCB_FILE_T aFileType, const wxString& aFileName,
            BOARD* aBoard, PROPERTIES* aProperties = NULL );
};


/**
 * Class PLUGIN
 * is a base class that BOARD loading and saving plugins should derive from.
 * Implementations can provide either Load() or Save() functions, or both.
 * PLUGINs throw exceptions, so it is best that you wrap your calls to these
 * functions in a try catch block.
 *
 * <pre>
 *   try
 *   {
 *        IO_MGR::Load(...);
 *   or
 *        IO_MGR::Save(...);
 *   }
 *   catch( IO_ERROR ioe )
 *   {
 *        // grab text from ioe, show in error window.
 *   }
 * </pre>
 */
class PLUGIN
{
public:

    //-----<PUBLIC PLUGIN API>-------------------------------------------------

    /**
     * Function PluginName
     * returns a brief hard coded name for this PLUGIN.
     */
    virtual const wxString& PluginName() = 0;

    /**
     * Function Load
     * loads a board file, or a portion of one, from some input file format
     * that this PLUGIN implementation knows about. This may be used to load an
     * entire new BOARD, or to augment an existing one if \a aAppendToMe is not NULL.
     *
     * @param aFileName is the name of the file to use as input and may be foreign in
     *  nature or native in nature.
     *
     * @param aAppendToMe is an existing BOARD to append to, but if NULL then
     *   this means "do not append, rather load anew".
     *
     * @param aProperties is an associative array that can be used to tell the
     *  loader how to load the file, because it can take any number of
     *  additional named arguments that the plugin is known to support. These are
     *  tuning parameters for the import or load.  The caller continues to own
     *  this object (plugin may not delete it), and plugins should expect it to
     *  be optionally NULL.
     *
     * @return BOARD* - the successfully loaded board, or the same one as aAppendToMe
     *  if aAppendToMe was not NULL, and caller owns it.
     *
     * @throw IO_ERROR if there is a problem loading, and its contents should
     *  say what went wrong, using line number and character offsets of the
     *  input file if possible.
     */
    virtual BOARD* Load( const wxString& aFileName, BOARD* aAppendToMe,
                        PROPERTIES* aProperties = NULL );

    /**
     * Function Save
     * will write a full aBoard to a storage file in a format that this
     * PLUGIN implementation knows about, or it can be used to write a portion of
     * aBoard to a special kind of export file.
     *
     * @param aFileName is the name of a file to save to on disk.
     *
     * @param aBoard is the class BOARD in memory document tree from which to
     *  extract information when writing to \a aFileName.  The caller continues to
     *  own the BOARD, and the plugin should refrain from modifying the BOARD if possible.
     *
     * @param aProperties is an associative array that can be used to tell the
     *  saver how to save the file, because it can take any number of
     *  additional named tuning arguments that the plugin is known to support.
     *  The caller continues to own this object (plugin may not delete it),
     *  and plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem saving or exporting.
     */
    virtual void Save( const wxString& aFileName, BOARD* aBoard,
                    PROPERTIES* aProperties = NULL );

    //-----</PUBLIC PLUGIN API>------------------------------------------------

    /*  The compiler writes the "zero argument" constructor for a PLUGIN
        automatically if you do not provide one. If you decide you need to
        provide a zero argument constructor of your own design, that is allowed.
        It must be public, and it is what the IO_MGR uses.  Parameters may be
        passed into a PLUGIN via the PROPERTIES variable for either Save() and Load().
    */

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
};



#if 0
    //-----<SCHEMATIC STUFF>------------------------------------------------
    // Should split into schematic specific PLUGIN base type

class SCHEMATIC;

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
     * will write aSchematic to a storage file in a format that only this
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


#endif // IO_MGR_H_
