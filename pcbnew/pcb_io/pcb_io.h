/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#pragma once

#include <io/io_base.h>
#include <pcb_io/pcb_io_mgr.h>

#include <cstdint>
#include <config.h>
#include <vector>
#include <wx/arrstr.h>
#include <i18n_utility.h>

class BOARD;
class FOOTPRINT;
class PROJECT;
class PROGRESS_REPORTER;

/**
 * A base class that #BOARD loading and saving plugins should derive from.
 *
 * Implementations can provide either Load() or Save() functions, or both. PCB_IOs throw
 * exceptions, so it is best that you wrap your calls to these functions in a try catch block.
 * Plugins throw exceptions because it is illegal for them to have any user interface calls in
 * them whatsoever, i.e. no windowing or screen printing at all.
 *
 *The compiler writes the "zero argument" constructor for a PCB_IO automatically if you do
 * not provide one.  If you decide you need to provide a zero argument constructor of your
 * own design, that is allowed.  It must be public, and it is what the #PCB_IO_MGR uses.  Parameters
 * may be passed into a PCB_IO via the #PROPERTIES variable for any of the public API functions
 * which take one.
 *
 *
 * <pre>
 *   try
 *   {
 *        PCB_IO_MGR::Load(...);
 *   or
 *        PCB_IO_MGR::Save(...);
 *   }
 *   catch( const IO_ERROR& ioe )
 *   {
 *        // grab text from ioe, show in error window.
 *   }
 * </pre>
 */
class PCB_IO : public IO_BASE
{
public:
    /**
     * Returns board file description for the PCB_IO.
     */
    virtual const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const
    {
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }

    /**
     * Work-around for lack of dynamic_cast across compile units on Mac
     */
    bool IsPCB_IO() const override { return true; }

    /**
     * Checks if this PCB_IO can read the specified board file.
     * If not overriden, extension check is used.
     */
    virtual bool CanReadBoard( const wxString& aFileName ) const;

    /**
     * Checks if this PCB_IO can read a footprint from specified file or directory.
     * If not overriden, extension check is used.
     */
    virtual bool CanReadFootprint( const wxString& aFileName ) const;

    /**
     * Registers a KIDIALOG callback for collecting info from the user.
     */
    virtual void SetQueryUserCallback( std::function<bool( wxString aTitle, int aIcon,
                                                           wxString aMessage,
                                                           wxString aAction )> aCallback )
    { }

    /**
     * Load information from some input file format that this PCB_IO implementation
     * knows about into either a new #BOARD or an existing one.
     *
     * This may be used to load an entire new #BOARD, or to augment an existing one if
     * @a aAppendToMe is not NULL.
     *
     * @param aFileName is the name of the file to use as input and may be foreign in
     *                  nature or native in nature.
     * @param aAppendToMe is an existing BOARD to append to, but if NULL then this means
     *                    "do not append, rather load anew".
     * @param aProperties is an associative array that can be used to tell the loader how to
     *                    load the file, because it can take any number of additional named
     *                    arguments that the plugin is known to support. These are tuning
     *                    parameters for the import or load.  The caller continues to own
     *                    this object (plugin may not delete it), and plugins should expect
     *                    it to be optionally NULL.
     * @param aProject is the optional #PROJECT object primarily used by third party
     *                 importers.
     * @return the successfully loaded board, or the same one as \a aAppendToMe if aAppendToMe
     *         was not NULL, and caller owns it.
     *
     * @throw IO_ERROR if there is a problem loading, and its contents should say what went
     *                 wrong, using line number and character offsets of the input file if
     *                 possible.
     */
    virtual BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                              const std::map<std::string, UTF8>* aProperties = nullptr,
                              PROJECT* aProject = nullptr );

    /**
     * Return a container with the cached library footprints generated in the last call to
     * #Load. This function is intended to be used ONLY by the non-KiCad board importers for the
     * purpose of obtaining the footprint library of the design and creating a project-specific
     * library.
     *
     * @return Footprints (caller owns the objects)
     */
    virtual std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints();

    /**
     * Write @a aBoard to a storage file in a format that this PCB_IO implementation knows
     * about or it can be used to write a portion of \a aBoard to a special kind of export
     * file.
     *
     * @param aFileName is the name of a file to save to on disk.
     * @param aBoard is the class #BOARD in memory document tree from which to extract
     *               information when writing to \a aFileName.  The caller continues to
     *               own the BOARD, and the plugin should refrain from modifying the BOARD
     *               if possible.
     * @param aProperties is an associative array that can be used to tell the saver how to
     *                    save the file, because it can take any number of additional named
     *                    tuning arguments that the plugin is known to support.  The caller
     *                    continues to own this object (plugin may not delete it) and plugins
     *                    should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem saving or exporting.
     */
    virtual void SaveBoard( const wxString& aFileName, BOARD* aBoard,
                            const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Return a list of footprint names contained within the library at @a aLibraryPath.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several footprints.
     * @param aProperties is an associative array that can be used to tell the plugin
     *                    anything needed about how to perform with respect to @a aLibraryPath.
     *                    The caller continues to own this object (plugin may not delete it),
     *                    and plugins should expect it to be optionally NULL.
     * @param aFootprintNames is the array of available footprint names inside a library.
     * @param aBestEfforts if true, don't throw on errors, just return an empty list.
     *
     * @throw IO_ERROR if the library cannot be found, or footprint cannot be loaded.
     */
    virtual void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                                     bool aBestEfforts,
                                     const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Generate a timestamp representing all the files in the library (including the library
     * directory).
     *
     * Timestamps should not be considered ordered, they either match or they don't.
     */
    virtual long long GetLibraryTimestamp( const wxString& aLibraryPath ) const = 0;

    /**
     * Load a single footprint from @a aFootprintPath and put its name in @a aFootprintNameOut.
     * If this is a footprint library, the first footprint should be loaded.
     * The default implementation uses FootprintEnumerate and FootprintLoad to load first footprint.
     *
     * @param aLibraryPath is a path of the footprint file.
     * @param aFootprintNameOut is the name output of the loaded footprint.
     * @param aProperties is an associative array that can be used to tell the loader
     *                    implementation to do something special, because it can take
     *                    any number of  additional named tuning arguments that the plugin
     *                    is known to support.  The caller continues to own this object
     *                    (plugin may not delete it), and plugins should expect it to be
     *                    optionally NULL.
     * @return the #FOOTPRINT object if found, caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the footprint cannot be found or read.
     */
    virtual FOOTPRINT* ImportFootprint( const wxString& aFootprintPath, wxString& aFootprintNameOut,
                                        const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Load a footprint having @a aFootprintName from the @a aLibraryPath containing a library
     * format that this PCB_IO knows about.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file, or URL
     *                     containing several footprints.
     * @param aFootprintName is the name of the footprint to load.
     * @param aProperties is an associative array that can be used to tell the loader
     *                    implementation to do something special, because it can take
     *                    any number of  additional named tuning arguments that the plugin
     *                    is known to support.  The caller continues to own this object
     *                    (plugin may not delete it), and plugins should expect it to be
     *                    optionally NULL.
     * @param aKeepUUID = true to keep initial items UUID, false to set new UUID
     *                   normally true if loaded in the footprint editor, false
     *                   if loaded in the board editor. Make sense only in kicad_plugin
     * @return the #FOOTPRINT object if found, caller owns it, else NULL if not found.
     *
     * @throw   IO_ERROR if the library cannot be found or read.  No exception is thrown in
     *                   the case where \a aFootprintName cannot be found.
     */
    virtual FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                      bool  aKeepUUID = false,
                                      const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * A version of FootprintLoad() for use after FootprintEnumerate() for more efficient
     * cache management.
     */
    virtual const FOOTPRINT* GetEnumeratedFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                                                     const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Check for the existence of a footprint.
     */
    virtual bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName,
                                  const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Write @a aFootprint to an existing library located at @a aLibraryPath.
     * If a footprint by the same name already exists, it is replaced.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file, or URL
     *                      containing several footprints.
     * @param aFootprint is what to store in the library. The caller continues to own the
     *                   footprint after this call.
     * @param aProperties is an associative array that can be used to tell the saver how to
     *                    save the footprint, because it can take any number of additional
     *                    named tuning arguments that the plugin is known to support.  The
     *                    caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    virtual void FootprintSave( const wxString& aLibraryPath, const FOOTPRINT* aFootprint,
                                const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Delete @a aFootprintName from the library at @a aLibraryPath.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file, or URL
     *                     containing several footprints.
     * @param aFootprintName is the name of a footprint to delete from the specified library.
     * @param aProperties is an associative array that can be used to tell the library delete
     *                    function anything special, because it can take any number of additional
     *                    named tuning arguments that the plugin is known to support.  The caller
     *                    continues to own this object (plugin may not delete it), and plugins
     *                    should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem finding the footprint or the library, or deleting it.
     */
    virtual void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                                  const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Append supported PLUGIN options to @a aListToAppenTo along with internationalized
     * descriptions.
     *
     * Options are typically appended so that a derived #PLUGIN can call its base class
     * function by the same name first, thus inheriting options declared there.  Some base
     * class options could pertain to all Footprint*() functions in all derived PLUGINs.
     *
     * @note Since aListToAppendTo is a #PROPERTIES object, all options will be unique and
     *       last guy wins.
     *
     * @param aListToAppendTo holds a tuple of
     * <dl>
     *  <dt>option</dt>
     *  <dd>This eventually is what shows up into the fp-lib-table "options"
     *      field, possibly combined with others.</dd>
     *  <dt>internationalized description</dt>
     *  <dd>The internationalized description is displayed in DIALOG_PLUGIN_OPTIONS.
     *      It may be multi-line and be quite explanatory of the option.</dd>
     * </dl>
     * <br>
     *  In the future perhaps @a aListToAppendTo evolves to something capable of also
     *  holding a wxValidator for the cells in said dialog:
     *  http://forums.wxwidgets.org/viewtopic.php?t=23277&p=104180.
     *  This would require a 3 column list, and introducing wx GUI knowledge to
     * PLUGIN, which has been avoided to date.
     */
    virtual void GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const override;

    virtual ~PCB_IO()
    {};

protected:
    PCB_IO( const wxString& aName ) :
            IO_BASE( aName ),
            m_board( nullptr ),
            m_props( nullptr )
    {}

    /// The board BOARD being worked on, no ownership here
    BOARD* m_board;

    /// Properties passed via Save() or Load(), no ownership, may be NULL.
    const std::map<std::string, UTF8>* m_props;
};
