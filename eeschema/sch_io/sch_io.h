/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_H_
#define SCH_IO_H_

#include <io/io_base.h>
#include <sch_io/sch_io_mgr.h>
#include <import_export.h>
#include <map>
#include <enum_vector.h>
#include <reporter.h>
#include <i18n_utility.h>
#include <wx/arrstr.h>

class SYMBOL_LIBRARY_MANAGER_ADAPTER;

/**
 * Base class that schematic file and library loading and saving plugins should derive from.
 * Implementations can provide either LoadSchematicFile() or SaveSchematicFile() functions,
 * or both. SCH_IOs throw exceptions, so it is best that you wrap your calls to these
 * functions in a try catch block.  Plugins throw exceptions because it is illegal
 * for them to have any user interface calls in them whatsoever, i.e. no windowing
 * or screen printing at all.
 *
 * <pre>
 *   try
 *   {
 *        SCH_IO_MGR::LoadSchematicFile(...);
 *   or
 *        SCH_IO_MGR::SaveSchematicFile(...);
 *   }
 *   catch( const IO_ERROR& ioe )
 *   {
 *        // grab text from ioe, show in error window.
 *   }
 * </pre>
 */
class SCH_IO : public IO_BASE
{
public:

    //-----<PUBLIC SCH_IO API>-------------------------------------------------

    /**
     * Returns schematic file description for the #SCH_IO.
     */
    virtual const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const;

    /**
     * Checks if this SCH_IO can read the specified schematic file.
     * If not overriden, extension check is used.
     */
    virtual bool CanReadSchematicFile( const wxString& aFileName ) const;

    /**
     * Return the modification hash from the library cache.
     *
     * @note This is temporary until the new s-expr file format is implement.  The new file
     *       format will embed symbols instead of referencing them from the library.  This
     *       function can be removed when the new file format is implemented.
     *
     * @return the modification hash of the library cache.
     */
    virtual int GetModifyHash() const = 0;

    virtual void SaveLibrary( const wxString& aFileName,
                              const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Load information from some input file format that this #SCH_IO implementation
     * knows about, into either a new #SCH_SHEET or an existing one. This may be used to load an
     * entire new #SCH_SHEET, or to augment an existing one if \a aAppendToMe is not NULL.
     *
     * @param aFileName is the name of the file to use as input and may be foreign in
     *                  nature or native in nature.
     *
     * @param aKiway is the #KIWAY object used to access the symbol libraries loaded
     *               by the project.
     *
     * @param aAppendToMe is an existing #SCH_SHEET to append to, but if NULL then this means
     *                    "do not append, rather load anew".
     *
     * @param aProperties is an associative array that can be used to tell the loader how to
     *                    load the file, because it can take any number of additional named
     *                    arguments that the plugin is known to support. These are tuning
     *                    parameters for the import or load.  The caller continues to own
     *                    this object (plugin may not delete it), and plugins should expect
     *                    it to be optionally NULL.
     *
     * @return the successfully loaded schematic, or the same one as \a aAppendToMe
     *         if \a aAppendToMe was not NULL, and the caller owns it.
     *
     * @throw IO_ERROR if there is a problem loading, and its contents should say what went
     *                 wrong, using line number and character offsets of the input file if
     *                 possible.
     */
    virtual SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                          SCH_SHEET*             aAppendToMe = nullptr,
                                          const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Write \a aSchematic to a storage file in a format that this #SCH_IO implementation
     * knows about, or it can be used to write a portion of \a aSchematic to a special kind
     * of export file.
     *
     * @param aFileName is the name of a file to save to on disk.
     *
     * @param aSheet     is the class #SCH_SHEET in memory document tree from which to extract
     *                   information when writing to \a aFileName.  The caller continues to
     *                   own the SCHEMATIC, and the plugin should refrain from modifying the
     *                   SCHEMATIC if possible.
     *
     * @param aSchematic is the #SCHEMATIC object used to access any schematic-wide or project
     *                   information needed to save the document.
     *
     * @param aProperties is an associative array that can be used to tell the saver how to
     *                    save the file, because it can take any number of additional named
     *                    tuning arguments that the plugin is known to support.  The caller
     *                    continues to own this object (plugin may not delete it), and plugins
     *                    should expect it to be optionally NULL.  Set the
     *                    #PropSaveCurrentSheetOnly property to only save the current sheet.
     *                    Otherwise, all hierarchical sheets are saved.
     *
     * @throw IO_ERROR if there is a problem saving or exporting.
     */
    virtual void SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet,
                                    SCHEMATIC*             aSchematic,
                                    const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Populate a list of #LIB_SYMBOL alias names contained within the library \a aLibraryPath.
     *
     * @param aSymbolNameList is an array to populate with the #LIB_SYMBOL names associated with
     *                        the library.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing one or more #LIB_SYMBOL objects.
     *
     * @param aProperties is an associative array that can be used to tell the plugin anything
     *                    needed about how to perform with respect to \a aLibraryPath.  The
     *                    caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if the library cannot be found, the part library cannot be loaded.
     */
    virtual void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                                     const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Populate a list of #LIB_SYMBOL aliases contained within the library \a aLibraryPath.
     *
     * @note It is the responsibility of the caller to delete the returned object from the heap.
     *       Failure to do this will result in memory leaks.
     *
     * @param aSymbolList is an array to populate with the #LIB_SYMBOL pointers associated with
     *                    the library.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing one or more #LIB_SYMBOL objects.
     *
     * @param aProperties is an associative array that can be used to tell the plugin anything
     *                    needed about how to perform with respect to \a aLibraryPath.  The
     *                    caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if the library cannot be found, the part library cannot be loaded.
     */
    virtual void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                     const wxString& aLibraryPath,
                                     const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Load a #LIB_SYMBOL object having \a aPartName from the \a aLibraryPath containing
     * a library format that this #SCH_IO knows about.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @param aPartName is the name of the #LIB_SYMBOL to load.
     *
     * @param aProperties is an associative array that can be used to tell the loader
     *                    implementation to do something special, because it can take
     *                    any number of additional named tuning arguments that the plugin
     *                    is known to support.  The caller continues to own this object
     *                    (plugin may not delete it), and plugins should expect it to be
     *                    optionally NULL.
     *
     * @return the part created on the heap if found caller shares it or NULL if not found.
     *
     * @throw IO_ERROR if the library cannot be found or read.  No exception
     *                 is thrown in the case where aAliasName cannot be found.
     */
    virtual LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                                    const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Write \a aSymbol to an existing library located at \a aLibraryPath.  If a #LIB_SYMBOL
     * by the same name already exists or there are any conflicting alias names, the new
     * #LIB_SYMBOL will silently overwrite any existing aliases and/or part because libraries
     * cannot have duplicate alias names.  It is the responsibility of the caller to check
     * the library for conflicts before saving.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @param aSymbol is what to store in the library.  The library is refreshed and the
     *                caller must update any #LIB_SYMBOL pointers that may have changed.
     *
     * @param aProperties is an associative array that can be used to tell the
     *                    saver how to save the symbol, because it can take any number of
     *                    additional named tuning arguments that the plugin is known to support.
     *                    The caller continues to own this object (plugin may not delete it), and
     *                    plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem saving.
     */
    virtual void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                             const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Delete the entire #LIB_SYMBOL associated with \a aAliasName from the library
     * \a aLibraryPath.
     *
     * @param aLibraryPath is a locator for the "library", usually a directory, file,
     *                     or URL containing several symbols.
     *
     * @param aSymbolName is the name of a #LIB_SYMBOL associated with it's root #LIB_SYMBOL
     *                    object to delete from the specified library.
     *
     * @param aProperties is an associative array that can be used to tell the library
     *                    delete function anything special, because it can take any number
     *                    of additional named tuning arguments that the plugin is known to
     *                    support.  The caller continues to own this object (plugin may not
     *                    delete it), and plugins should expect it to be optionally NULL.
     *
     * @throw IO_ERROR if there is a problem finding the alias or the library or deleting it.
     */
    virtual void DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                               const std::map<std::string, UTF8>* aProperties = nullptr );

    /**
     * Append supported #SCH_IO options to \a aListToAppenTo along with internationalized
     * descriptions.  Options are typically appended so that a derived SCH_IO can call
     * its base class function by the same name first, thus inheriting options declared there.
     * (Some base class options could pertain to all Symbol*() functions in all derived
     * SCH_IOs.)  Note that since aListToAppendTo is a PROPERTIES object, all options
     * will be unique and last guy wins.
     *
     * @param aListToAppendTo holds a tuple of
     * <dl>
     *   <dt>option</dt>
     *   <dd>This eventually is what shows up into the fp-lib-table "options"
     *       field, possibly combined with others.</dd>
     *   <dt>internationalized description</dt>
     *   <dd>The internationalized description is displayed in DIALOG_PLUGIN_OPTIONS.
     *      It may be multi-line and be quite explanatory of the option.</dd>
     *  </dl>
     * <br>
     *  In the future perhaps \a aListToAppendTo evolves to something capable of also
     *  holding a wxValidator for the cells in said dialog:
     *  http://forums.wxwidgets.org/viewtopic.php?t=23277&p=104180.
     *   This would require a 3 column list, and introducing wx GUI knowledge to
     *   #SCH_IO, which has been avoided to date.
     */
    virtual void GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const override;

    /**
     * @return true if this plugin supports libraries that contain sub-libraries.
     */
    virtual bool SupportsSubLibraries() const { return false; }

    /**
     * Retrieves a list of sub-libraries in this library.
     *
     * Some types of symbol library support sub-libraries, which are a single-level organizational
     * hierarchy that is implementation-defined per plugin.  Most of KiCad ignores sub-libraries and
     * treats the hierarchy between library and symbol as flat, but the sub-libraries are used for
     * sorting and grouping symbols in the symbol chooser.
     *
     * Has no effect if SupportsSubLibraries() returns false.
     *
     * @param aNames will be filled with a list of sub-libraries within this symbol library
     */
    virtual void GetSubLibraryNames( std::vector<wxString>& aNames ) {}

    /**
     * Gets a description of a sublibrary.
     *
     * Has no effect if SupportsSubLibraries() returns false.
     *
     * @param aName contains the name of the sublibrary for which the description is retrieved
     *
     * @return the description of the sublibrary
     */
    virtual wxString GetSubLibraryDescription( const wxString& aName ) { return wxEmptyString; }

    /**
     * Retrieves a list of (custom) field names that are present on symbols in this library.
     * The plugin is responsible for guaranteeing that this list contains the set of unique
     * custom field names present on any symbols contained in the library.
     *
     * The required KiCad fields are not included in this list.
     *
     * @param aNames will be filled with any custom fields present in this library.
     */
    virtual void GetAvailableSymbolFields( std::vector<wxString>& aNames ) {}

    /**
     * Retrieves a list of (custom) field names that should be shown by default for this library
     * in the symbol chooser.  This list should be a subset of the result returned by
     * GetAvailableSymbolFields().
     *
     * The preference for which fields to hide and show for a given library is stored on a
     * per-library basis in a user's preferences (or in the project local settings for a project-
     * local library).  The set of fields returned by GetDefaultSymbolFields() will be used if this
     * preference is missing.
     *
     * @param aNames will be filled with the custom field names that should be shown by default
     */
    virtual void GetDefaultSymbolFields( std::vector<wxString>& aNames )
    {
        return GetAvailableSymbolFields( aNames );
    }

    /**
     * Return an error string to the caller.
     *
     * This is useful for schematic loaders that can load partial schematics where throwing
     * an exception would be problematic such as the KiCad legacy plugin.
     *
     * @return an unformatted string containing errors if any.
     */
    virtual const wxString& GetError() const;

    /**
     * Some library plugins need to interface with other loaded libraries.
     * To do this, they receive a reference to the project-specific manager adapter.
     */
    virtual void SetLibraryManagerAdapter( SYMBOL_LIBRARY_MANAGER_ADAPTER* aAdapter ) {}

    //-----</PUBLIC SCH_IO API>------------------------------------------------


    /*  The compiler writes the "zero argument" constructor for a SCH_IO
        automatically if you do not provide one. If you decide you need to
        provide a zero argument constructor of your own design, that is allowed.
        It must be public, and it is what the SCH_IO_MGR uses.  Parameters may be
        passed into a SCH_IO via the PROPERTIES variable for any of the public
        API functions which take one.
    */
    virtual ~SCH_IO() { }

protected:

    SCH_IO( const wxString& aName ) : IO_BASE( aName )
    {}
};


#endif // SCH_IO_H_
