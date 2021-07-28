/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <map>
#include <mutex>
#include <boost/ptr_container/ptr_vector.hpp>
#include <wx/filename.h>

#include <sch_io_mgr.h>
#include <project.h>

class LIB_SYMBOL;
class LIB_ID;
class LINE_READER;
class OUTPUTFORMATTER;
class SCH_LEGACY_PLUGIN;
class SCH_PLUGIN;


#define DOC_EXT           "dcm"

/*
 * Symbol Library version and file header  macros.
 */
#define LIB_VERSION_MAJOR 2
#define LIB_VERSION_MINOR 4

/* Must be the first line of symbol library (.lib) files. */
#define LIBFILE_IDENT     "EESchema-LIBRARY Version"

#define LIB_VERSION( major, minor ) ( major * 100 + minor )

#define IS_LIB_CURRENT_VERSION( major, minor )              \
    (                                                       \
        LIB_VERSION( major1, minor1 ) ==                    \
        LIB_VERSION( LIB_VERSION_MAJOR, LIB_VERSION_MINOR)  \
    )

/*
 * Library versions 2.4 and lower use the old separate library (.lib) and
 * document (.dcm) files.  Symbol libraries after 2.4 merged the library
 * and document files into a single library file.  This macro checks if the
 * library version supports the old format
 */
#define USE_OLD_DOC_FILE_FORMAT( major, minor )                 \
    ( LIB_VERSION( major, minor ) <= LIB_VERSION( 2, 4 ) )

enum class SCH_LIB_TYPE
{
    LT_EESCHEMA,
    LT_SYMBOL
};

// Helper class to filter a list of libraries, and/or a list of SYMBOL_LIB
// in dialogs
class SCHLIB_FILTER
{
public:
    SCHLIB_FILTER()
    {
        m_filterPowerSymbols = false;
        m_forceLoad = false;
    }

    /**
     * add a lib name to the allowed libraries
     */
    void AddLib( const wxString& aLibName )
    {
        m_allowedLibs.Add( aLibName );
        m_forceLoad = false;
    }


    /**
     * add a lib name to the allowed libraries
     */
    void LoadFrom( const wxString& aLibName )
    {
        m_allowedLibs.Clear();
        m_allowedLibs.Add( aLibName );
        m_forceLoad = true;
    }

    /**
     * Clear the allowed libraries list (allows all libs)
     */
    void ClearLibList()
    {
        m_allowedLibs.Clear();
        m_forceLoad = false;
    }

    /**
     * Set the filtering of power symbols
     */
    void FilterPowerSymbols( bool aFilterEnable )
    {
        m_filterPowerSymbols = aFilterEnable;
    }

    // Accessors

    /**
     * @return true if the filtering of power symbols is on
     */
    bool GetFilterPowerSymbols() const { return m_filterPowerSymbols; }


    /**
     * @return am wxArrayString of the names of allowed libs
     */
    const wxArrayString& GetAllowedLibList() const { return m_allowedLibs; }

    /**
     * @return the name of the lib to use to load a symbol, or an a empty string
     * Useful to load (in lib editor or lib viewer) a symbol from a given library
     */
    const wxString& GetLibSource() const
    {
        static wxString dummy;

        if( m_forceLoad && m_allowedLibs.GetCount() > 0 )
            return m_allowedLibs[0];
        else
            return dummy;
    }

private:
    wxArrayString m_allowedLibs;        ///< a list of lib names to list some libraries
                                        ///< if empty: no filter
    bool          m_filterPowerSymbols; ///< true to filter (show only) power symbols
    bool          m_forceLoad;          // When true, load a symbol lib from the lib
                                        // which is given in m_allowedLibs[0]
};


/* Helpers for creating a list of symbol libraries. */
class SYMBOL_LIB;
class wxRegEx;

/**
 * LIB_SYMBOL map sorting.
 */
struct LibSymbolMapSort
{
    bool operator() ( const wxString& aItem1, const wxString& aItem2 ) const
    {
        return aItem1 < aItem2;
    }
};

/// Symbol map used by symbol library object.

typedef std::map< wxString, LIB_SYMBOL*, LibSymbolMapSort > LIB_SYMBOL_MAP;
typedef boost::ptr_vector< SYMBOL_LIB >                     SYMBOL_LIBS_BASE;


/**
 * A collection of #SYMBOL_LIB objects.
 *
 * It extends from PROJECT::_ELEM so it can be hung in the PROJECT.  It does not use any
 * UI calls, but rather simply throws an IO_ERROR when there is a problem.
 */
class SYMBOL_LIBS : public SYMBOL_LIBS_BASE, public PROJECT::_ELEM
{
public:
    KICAD_T Type() override { return SYMBOL_LIBS_T; }

    static int        s_modify_generation;         ///< helper for GetModifyHash()
    static std::mutex s_generationMutex;

    SYMBOL_LIBS()
    {
        IncrementModifyGeneration();
    }

    static void IncrementModifyGeneration()
    {
        std::lock_guard<std::mutex> mut( SYMBOL_LIBS::s_generationMutex );
        ++SYMBOL_LIBS::s_modify_generation;
    }

    static int GetModifyGeneration()
    {
        std::lock_guard<std::mutex> mut( SYMBOL_LIBS::s_generationMutex );
        return SYMBOL_LIBS::s_modify_generation;
    }

    /// Return the modification hash for all libraries.  The value returned
    /// changes on every library modification.
    int GetModifyHash();

    /**
     * Allocate and adds a symbol library to the library list.
     *
     * @param aFileName is the file name object of symbol library.
     * @throw IO_ERROR if there's any problem loading.
     */
    SYMBOL_LIB* AddLibrary( const wxString& aFileName );

    /**
     * Insert a symbol library into the library list.
     *
     * @param aFileName is the file name object of symbol library.
     * @param aIterator is an iterator to insert library in front of.
     * @return the new SYMBOL_LIB, which remains owned by this SYMBOL_LIBS container.
     * @throw IO_ERROR if there's any problem loading.
     */
    SYMBOL_LIB* AddLibrary( const wxString& aFileName, SYMBOL_LIBS::iterator& aIterator );

    /**
     * Load all of the project's libraries into this container, which should
     * be cleared before calling it.
     *
     * @note This method is only to be used when loading legacy projects.  All further symbol
     *       library access should be done via the symbol library table.
     */
    void LoadAllLibraries( PROJECT* aProject, bool aShowProgress=true );

    /**
     * Save or load the names of the currently configured symbol libraries (without paths).
     */
    static void LibNamesAndPaths( PROJECT* aProject, bool doSave,
                                  wxString* aPaths, wxArrayString* aNames = nullptr );

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
    SYMBOL_LIB* FindLibrary( const wxString& aName );

    SYMBOL_LIB* FindLibraryByFullFileName( const wxString& aFullFileName );

    SYMBOL_LIB* GetCacheLibrary();

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
class SYMBOL_LIB
{
public:
    SYMBOL_LIB( SCH_LIB_TYPE aType, const wxString& aFileName,
              SCH_IO_MGR::SCH_FILE_T aPluginType = SCH_IO_MGR::SCH_LEGACY );
    ~SYMBOL_LIB();

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
    static SYMBOL_LIB* LoadLibrary( const wxString& aFileName );

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
    std::unique_ptr< SCH_PLUGIN > m_plugin;
    std::unique_ptr< PROPERTIES > m_properties;   ///< Library properties
};


/**
 * Case insensitive library name comparison.
 */
bool operator==( const SYMBOL_LIB& aLibrary, const wxString& aName );
bool operator!=( const SYMBOL_LIB& aLibrary, const wxString& aName );

#endif  //  SYMBOL_LIBRARY_H
