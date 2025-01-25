/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file symbol_library.h
 * @brief Definition for symbol library class.
 */

#ifndef SYMBOL_LIBRARY_H
#define SYMBOL_LIBRARY_H

#include <mutex>
#include <boost/ptr_container/ptr_vector.hpp>
#include <wx/filename.h>

#include <project.h>
#include <sch_io/sch_io_mgr.h>
#include <symbol_library_common.h>


class LIB_SYMBOL;
class LIB_ID;
class LINE_READER;
class OUTPUTFORMATTER;
class SCH_IO;
class LEGACY_SYMBOL_LIB;


/**
 * A collection of #SYMBOL_LIB objects.
 *
 * It extends from PROJECT::_ELEM so it can be hung in the PROJECT.  It does not use any
 * UI calls, but rather simply throws an IO_ERROR when there is a problem.
 */
class LEGACY_SYMBOL_LIBS : public boost::ptr_vector<LEGACY_SYMBOL_LIB>, public PROJECT::_ELEM
{
public:
    PROJECT::ELEM ProjectElementType() override { return PROJECT::ELEM::SCH_SYMBOL_LIBS; }

    LEGACY_SYMBOL_LIBS() {}

    /**
     * Allocate and adds a symbol library to the library list.
     *
     * @param aFileName is the file name object of symbol library.
     * @throw IO_ERROR if there's any problem loading.
     */
    LEGACY_SYMBOL_LIB* AddLibrary( const wxString& aFileName );

    /**
     * Insert a symbol library into the library list.
     *
     * @param aFileName is the file name object of symbol library.
     * @param aIterator is an iterator to insert library in front of.
     * @return the new SYMBOL_LIB, which remains owned by this SYMBOL_LIBS container.
     * @throw IO_ERROR if there's any problem loading.
     */
    LEGACY_SYMBOL_LIB* AddLibrary( const wxString& aFileName, LEGACY_SYMBOL_LIBS::iterator& aIterator );

    /**
     * Refreshes the library from the (possibly updated) contents on disk
     *
     * @param aFileName is the file name of the symbol library
     * @return true if successfully updated
     */
    bool ReloadLibrary( const wxString& aFileName );

    /**
     * Load all of the project's libraries into this container, which should
     * be cleared before calling it.
     *
     * @note This method is only to be used when loading legacy projects.  All further symbol
     *       library access should be done via the symbol library table.
     */
    void LoadAllLibraries( PROJECT* aProject, bool aShowProgress=true );

    static void GetLibNamesAndPaths( PROJECT* aProject, wxString* aPaths,
                                     wxArrayString* aNames = nullptr );

    static void SetLibNamesAndPaths( PROJECT* aProject, const wxString& aPaths,
                                     const wxArrayString& aNames );

    /**
     * Return the name of the cache library after potentially fixing it from
     * an older naming scheme.  That is, the old file is renamed if needed.
     *
     * @param aFullProjectFilename is the *.pro filename with absolute path.
     */
    static const wxString CacheName( const wxString& aFullProjectFilename );

    /**
     * Find a symbol library by \a aName.
     *
     * @param aName is the library file name without path or extension to find.
     * @return the symbol library if found, otherwise NULL.
     */
    LEGACY_SYMBOL_LIB* FindLibrary( const wxString& aName );

    LEGACY_SYMBOL_LIB* FindLibraryByFullFileName( const wxString& aFullFileName );

    LEGACY_SYMBOL_LIB* GetCacheLibrary();

    /**
     * Return the list of symbol library file names without path and extension.
     *
     * @param aSorted sort the list of name if true.  Otherwise use the library load order.
     * @return the list of library names.
     */
    wxArrayString GetLibraryNames( bool aSorted = true );

    /**
     * Search all libraries in the list for a symbol.
     *
     * A symbol object will always be returned.  If the entry found
     * is an alias.  The root symbol will be found and returned.
     *
     * @param aLibId is the #LIB_ID of the symbol to search for.
     * @param aLibraryName is the name of the library to search for symbol.
     * @return the symbol object if found, otherwise NULL.
     */
    LIB_SYMBOL* FindLibSymbol( const LIB_ID& aLibId, const wxString& aLibraryName = wxEmptyString );

    /**
     * Search all libraries in the list for a #LIB_SYMBOL using a case insensitive comparison.
     *
     * Helper function used in dialog to find all candidates.
     * During a long time, eeschema was using a case insensitive search.
     * Therefore, for old schematics (<= 2013), or libs, for some symbols,
     * the chip name (name of alias in lib) can be broken.
     * This function can be used to display a list of candidates, in symbol properties dialog.
     *
     * @param aEntryName is the name of entries to search for (case insensitive).
     * @param aLibraryName is the name of the library to search.
     * @param aCandidates is a std::vector to store candidates.
     */
    void FindLibraryNearEntries( std::vector<LIB_SYMBOL*>& aCandidates, const wxString& aEntryName,
                                 const wxString& aLibraryName = wxEmptyString );

    int GetLibraryCount() { return size(); }
};


/**
 * Object used to load, save, search, and otherwise manipulate symbol library files.
 *
 * @warning This code is obsolete with the exception of the cache library.  All other
 *          symbol library I/O is managed by the #SCH_IO_MGR object.
 */
class LEGACY_SYMBOL_LIB
{
public:
    LEGACY_SYMBOL_LIB( SCH_LIB_TYPE aType, const wxString& aFileName,
                       SCH_IO_MGR::SCH_FILE_T aPluginType = SCH_IO_MGR::SCH_LEGACY );
    ~LEGACY_SYMBOL_LIB();

    /**
     * @return a magic number that changes if the library has changed
     */
    int GetModHash() const { return m_mod_hash; }

    SCH_IO_MGR::SCH_FILE_T GetPluginType() const { return m_pluginType; }

    void SetPluginType( SCH_IO_MGR::SCH_FILE_T aPluginType );

    void Create( const wxString& aFileName = wxEmptyString );

    void SetFileName( const wxString& aFileName ) { fileName = aFileName; }

    bool IsModified() const
    {
        return isModified;
    }

    bool IsCache() const;

    void SetCache();

    bool IsBuffering() const;

    void EnableBuffering( bool aEnable = true );

    void Save( bool aSaveDocFile = true );

    /**
     * @return true if current user does not have write access to the library file.
     */
    bool IsReadOnly() const { return !fileName.IsFileWritable(); }

    /**
     * Load a string array with the names of all the entries in this library.
     *
     * @param aNames is the array to place entry names into.
     */
    void GetSymbolNames( wxArrayString& aNames ) const;

    /**
     * Load a vector with all the entries in this library.
     *
     * @param aSymbols is a vector to receive the aliases.
     */
    void GetSymbols( std::vector<LIB_SYMBOL*>& aSymbols ) const;

    /**
     * Find #LIB_SYMBOL by \a aName.
     *
     * @param aName is the name of the symbol, case sensitive.
     * @return LIB_SYMBOL pointer symbol if found, else NULL.
     */
    LIB_SYMBOL* FindSymbol( const wxString& aName ) const;

    LIB_SYMBOL* FindSymbol( const LIB_ID& aLibId ) const;

    /**
     * Add \a aSymbol entry to library.
     *
     * @note A #LIB_SYMBOL can have an alias list so these alias will be added in library.
     *       and the any existing duplicate aliases will be removed from the library.
     *
     * @param aSymbol is the symbol to add, caller retains ownership, a clone is added.
     */
    void AddSymbol( LIB_SYMBOL* aSymbol );

    /**
     * Safely remove \a aEntry from the library and return the next entry.
     *
     * The next entry returned depends on the entry being removed.  If the entry being
     * remove also removes the symbol, then the next entry from the list is returned.
     * If the entry being used only removes an alias from a symbol, then the next alias
     * of the symbol is returned.
     *
     * @param aEntry is the entry to remove from library.
     * @return The next entry in the library or NULL if the library is empty.
     */
    LIB_SYMBOL* RemoveSymbol( LIB_SYMBOL* aEntry );

    /**
     * Replace an existing symbol entry in the library.
     *
     * @note A symbol can have an alias list so these aliases will be added in library and
     *       previously existing alias removed.
     *
     * @param aOldSymbol is the symbol to replace.
     * @param aNewSymbol is the new symbol.
     */
    LIB_SYMBOL* ReplaceSymbol( LIB_SYMBOL* aOldSymbol, LIB_SYMBOL* aNewSymbol );

    /**
     * Return the file name without path or extension.
     *
     * @return the name of library file.
     */
    const wxString GetName() const            { return fileName.GetName(); }

    /**
     * Return the full file library name with path and extension.
     *
     * @return the full library file name with path and extension.
     */
    wxString GetFullFileName() const          { return fileName.GetFullPath(); }

    /**
     * Return the logical name of the library.
     *
     * @return The logical name of this library.
     */
    const wxString GetLogicalName() const
    {
        /*  for now is the filename without path or extension.

            Technically the library should not know its logical name!
            This will eventually come out of a pair of lookup tables using a
            reverse lookup using the full name or library pointer as a key.
            Search will be by project lookup table and then user lookup table if
            not found.
        */
        return fileName.GetName();
    }

    /**
     * Allocate and load a symbol library file.
     *
     * @param aFileName is the file name of the symbol library to load.
     * @return SYMBOL_LIB* is the allocated and loaded SYMBOL_LIB, which is owned by the caller.
     * @throw IO_ERROR if there's any problem loading the library.
     */
    static LEGACY_SYMBOL_LIB* LoadSymbolLibrary( const wxString& aFileName );

private:
    SCH_LIB_TYPE    type;           ///< Library type indicator.
    wxFileName      fileName;       ///< Library file name.
    wxDateTime      timeStamp;      ///< Library save time and date.
    int             versionMajor;   ///< Library major version number.
    int             versionMinor;   ///< Library minor version number.
    wxString        header;         ///< first line of loaded library.
    bool            isModified;     ///< Library modification status.
    int             m_mod_hash;     ///< incremented each time library is changed.

    SCH_IO_MGR::SCH_FILE_T        m_pluginType;
    std::unique_ptr< SCH_IO > m_plugin;
    std::unique_ptr<std::map<std::string, UTF8>> m_properties;   ///< Library properties
};


/**
 * Case insensitive library name comparison.
 */
bool operator==( const LEGACY_SYMBOL_LIB& aLibrary, const wxString& aName );
bool operator!=( const LEGACY_SYMBOL_LIB& aLibrary, const wxString& aName );

#endif  //  SYMBOL_LIBRARY_H
