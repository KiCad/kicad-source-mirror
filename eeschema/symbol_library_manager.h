/*
 * This program source code file is symbol of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A SYMBOLICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SYMBOL_LIBRARY_MANAGER_H
#define SYMBOL_LIBRARY_MANAGER_H

#include <map>
#include <list>
#include <deque>
#include <set>
#include <memory>
#include <wx/arrstr.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <libraries/library_table.h>

class LIB_SYMBOL;
class LEGACY_SYMBOL_LIB;
class PROGRESS_REPORTER;
class SCH_IO;
class SCH_BASE_FRAME;
class LIB_LOGGER;


enum class SYMBOL_NAME_FILTER
{
    ALL,
    ROOT_ONLY,
    DERIVED_ONLY
};


class SYMBOL_BUFFER
{
public:
    SYMBOL_BUFFER( std::unique_ptr<LIB_SYMBOL> aSymbol = nullptr,
                   std::unique_ptr<SCH_SCREEN> aScreen = nullptr );
    ~SYMBOL_BUFFER();

    LIB_SYMBOL& GetSymbol() const { return *m_symbol; }
    void        SetSymbol( std::unique_ptr<LIB_SYMBOL> aSymbol );

    LIB_SYMBOL& GetOriginal() const { return *m_original; }
    void        SetOriginal( std::unique_ptr<LIB_SYMBOL> aSymbol );

    bool        IsModified() const;
    SCH_SCREEN* GetScreen() const { return m_screen.get(); }

private:
    std::unique_ptr<SCH_SCREEN> m_screen;
    std::unique_ptr<LIB_SYMBOL> m_symbol;   // Working copy
    std::unique_ptr<LIB_SYMBOL> m_original; // Initial state of the symbol
};


/// Store a working copy of a library.
class LIB_BUFFER
{
public:
    LIB_BUFFER( const wxString& aLibrary ) :
            m_libName( aLibrary ),
            m_hash( 1 )
    {}

    bool IsModified() const
    {
        if( !m_deleted.empty() )
            return true;

        for( const std::shared_ptr<SYMBOL_BUFFER>& symbolBuf : m_symbols )
        {
            if( symbolBuf->IsModified() )
                return true;
        }

        return false;
    }

    int GetHash() const { return m_hash; }

    /// Return the working copy of a #LIB_SYMBOL root object with specified alias.
    LIB_SYMBOL* GetSymbol( const wxString& aAlias ) const;

    /// Create a new buffer to store a symbol. #LIB_BUFFER takes ownership of \a aCopy.
    bool CreateBuffer( std::unique_ptr<LIB_SYMBOL> aCopy, std::unique_ptr<SCH_SCREEN> aScreen );

    /// Update the buffered symbol with the contents of \a aCopy.
    bool UpdateBuffer( SYMBOL_BUFFER& aSymbolBuf, const LIB_SYMBOL& aCopy );

    /**
     * Delete the given symbol buffer from the library buffer.
     */
    bool DeleteBuffer( const SYMBOL_BUFFER& aSymbolBuf );

    /// Return the deleted symbol buffers that need to be removed from the library file.
    const std::deque<std::shared_ptr<SYMBOL_BUFFER>>& GetDeletedBuffers() const { return m_deleted; }

    void ClearDeletedBuffer() { m_deleted.clear(); }

    /// Save stored modifications using a plugin. aBuffer decides whether the changes
    /// should be cached or stored directly to the disk (for #SCH_IO_KICAD_LEGACY).
    bool SaveBuffer( SYMBOL_BUFFER& aSymbolBuf, const wxString& aFileName, SCH_IO* aPlugin,
                     bool aBuffer );

    /// Return a symbol buffer with #LIB_SYMBOL holding a symbolic alias.
    std::shared_ptr<SYMBOL_BUFFER> GetBuffer( const wxString& aAlias ) const;

    /// Return all buffered symbols.
    const std::deque<std::shared_ptr<SYMBOL_BUFFER>>& GetBuffers() const { return m_symbols; }

    /**
     * Check to see any symbols in the buffer are derived from a parent named \a aParentName.
     *
     * @param aParentName is the name of the parent to test.
     * @return true if any symbols are found derived from a symbol named \a aParent, otherwise
     *         false.
     */
    bool HasDerivedSymbols( const wxString& aParentName ) const;

    /**
     * Fetch a list of root symbols names from the library buffer.
     *
     * @param aRootSymbolNames is a reference to a list to populate with root symbol names.
     * @param aFilter is the symbol derivation type.
     */
    void GetSymbolNames( wxArrayString&     aSymbolNames,
                         SYMBOL_NAME_FILTER aFilter = SYMBOL_NAME_FILTER::ALL );

    /**
     * Fetch all of the symbols derived from a \a aSymbolName into \a aList.
     *
     * @param aSymbolName is the name of the symbol to search for derived symbols in this
     *                    buffer.
     * @param aList is the list of symbols names derived from \a aSymbolName.
     * @return a size_t count of the number of symbols derived from \a aSymbolName.
     */
    size_t GetDerivedSymbolNames( const wxString& aSymbolName, wxArrayString& aList );

private:
    /**
     * Remove all symbols derived from \a aParent from the library buffer.
     *
     * @param aParent is the #SYMBOL_BUFFER to check against.
     * @return the count of #SYMBOL_BUFFER objects removed from the library.
     */
    int removeChildSymbols( const SYMBOL_BUFFER& aSymbolBuf );

private:
    std::deque<std::shared_ptr<SYMBOL_BUFFER>> m_symbols;
    std::deque<std::shared_ptr<SYMBOL_BUFFER>> m_deleted;   ///< Buffer for deleted symbols until
                                                            ///<   library is saved.
    const wxString                             m_libName;   ///< Buffered library name
    int                                        m_hash;
};


/**
 * Class to handle modifications to the symbol libraries.
 * TODO(JE) Library tables - this class probably shouldn't exist anymore
 */
class SYMBOL_LIBRARY_MANAGER
{
public:
    SYMBOL_LIBRARY_MANAGER( SCH_BASE_FRAME& aFrame );
    virtual ~SYMBOL_LIBRARY_MANAGER();

    int GetHash() const;

    bool HasModifications() const;

    /**
     * Return a library hash value to determine if it has changed.
     *
     * For buffered libraries, it returns a number corresponding to the number of modifications.
     * For original libraries, hash is computed basing on the library URI. Returns -1 when the
     * requested library does not exist.
     */
    int GetLibraryHash( const wxString& aLibrary ) const;

    /**
     * Return the array of library names.
     */
    wxArrayString GetLibraryNames() const;

    std::list<LIB_SYMBOL*> EnumerateSymbols( const wxString& aLibrary ) const;

    /**
     * Create an empty library and adds it to the library table.
     *
     * The library file is created.
     */
    bool CreateLibrary( const wxString& aFilePath, LIBRARY_TABLE_SCOPE aScope )
    {
        return addLibrary( aFilePath, true, aScope );
    }

    /**
     * Add an existing library.
     *
     * The library is added to the library table as well.
     */
    bool AddLibrary( const wxString& aFilePath, LIBRARY_TABLE_SCOPE aScope )
    {
        return addLibrary( aFilePath, false, aScope );
    }

    /**
     * Update the symbol buffer with a new version of the symbol.
     *
     * The library buffer creates a copy of the symbol.
     *
     * It is required to save the library to use the updated symbol in the schematic editor.
     */
    bool UpdateSymbol( LIB_SYMBOL* aSymbol, const wxString& aLibrary );

    /**
     * Update the symbol buffer with a new version of the symbol when the name has changed.
     *
     * The old library buffer will be deleted and a new one created with the new name.
     */
    bool UpdateSymbolAfterRename( LIB_SYMBOL* aSymbol, const wxString& aOldSymbolName,
                                  const wxString& aLibrary );

    /**
     * Update the library buffer with a new version of the library.
     */
    bool UpdateLibraryBuffer( const wxString& aLibrary );

    /**
     * Remove the symbol from the symbol buffer.
     *
     * It is required to save the library to have the symbol removed in the schematic editor.
     */
    bool RemoveSymbol( const wxString& aSymbolName, const wxString& aLibrary );

    /**
     * Return either an alias of a working LIB_SYMBOL copy, or alias of the original symbol if there
     * is no working copy.
     */
    LIB_SYMBOL* GetSymbol( const wxString& aSymbolName, const wxString& aLibrary ) const;

    /**
     * Return the symbol copy from the buffer.
     *
     * In case it does not exist yet, the copy is created.  #SYMBOL_LIBRARY_MANAGER retains
     * the ownership.
     */
    LIB_SYMBOL* GetBufferedSymbol( const wxString& aSymbolName, const wxString& aLibrary );

    /**
     * Return the screen used to edit a specific symbol. #SYMBOL_LIBRARY_MANAGER retains the
     * ownership.
     */
    SCH_SCREEN* GetScreen( const wxString& aSymbolName, const wxString& aLibrary );

    /**
     * Return true if symbol with a specific alias exists in library (either original one or
     * buffered).
     */
    bool SymbolExists( const wxString& aSymbolName, const wxString& aLibrary ) const;

    /**
     * Return true if the symbol name is already in use in the specified library.
     */
    bool SymbolNameInUse( const wxString& aName, const wxString& aLibrary );

    /**
     * Return true if library exists.
     *
     * If \a aCheckEnabled is set, then the library must also be enabled in the library table.
     */
    bool LibraryExists( const wxString& aLibrary, bool aCheckEnabled = false ) const;

    /**
     * Return true if the library was successfully loaded.
     */
    bool IsLibraryLoaded( const wxString& aLibrary ) const;

    /**
     * Return true if library has unsaved modifications.
     */
    bool IsLibraryModified( const wxString& aLibrary ) const;

    /**
     * Return true if symbol has unsaved modifications.
     */
    bool IsSymbolModified( const wxString& aSymbolName, const wxString& aLibrary ) const;

    void SetSymbolModified( const wxString& aSymbolName, const wxString& aLibrary );

    /**
     * Clear the modified flag for all symbols in a library.
     */
    bool ClearLibraryModified( const wxString& aLibrary ) const;

    /**
     * Clear the modified flag for a symbol.
     */
    bool ClearSymbolModified( const wxString& aSymbolName, const wxString& aLibrary ) const;

    /**
     * Return true if the library is stored in a read-only file.
     *
     * @return True on success, false otherwise.
     */
    bool IsLibraryReadOnly( const wxString& aLibrary ) const;

    /**
     * Save library to a file, including unsaved changes.
     *
     * @param aLibrary is the library name.
     * @param aFileName is the target file name.
     * @return True on success, false otherwise.
     */
    bool SaveLibrary( const wxString& aLibrary, const wxString& aFileName,
                      SCH_IO_MGR::SCH_FILE_T aFileType = SCH_IO_MGR::SCH_FILE_T::SCH_LEGACY );

    /**
     * Revert unsaved changes for a symbol.
     *
     * @return The LIB_ID of the reverted symbol (which may be different in the case
     * of a rename)
     */
    LIB_ID RevertSymbol( const wxString& aSymbolName, const wxString& aLibrary );

    /**
     * Revert unsaved changes for a symbol library.
     *
     * @return True on success, false otherwise.
     */
    bool RevertLibrary( const wxString& aLibrary );

    /**
     * Revert all pending changes.
     *
     * @return True if all changes successfully reverted.
     */
    bool RevertAll();

    /**
     * Return a library name that is not currently in use.
     *
     * Used for generating names for new libraries.
     */
    wxString GetUniqueLibraryName() const;

    void GetSymbolNames( const wxString& aLibName, wxArrayString& aSymbolNames,
                         SYMBOL_NAME_FILTER aFilter = SYMBOL_NAME_FILTER::ALL );

    /**
     * Fetch all of the symbols derived from a \a aSymbolName into \a aList.
     */
    size_t GetDerivedSymbolNames( const wxString& aSymbolName, const wxString& aLibraryName, wxArrayString& aList );

    size_t GetLibraryCount() const;

protected:
    virtual void OnDataChanged() const {}

    /// Extract library name basing on the file name.
    static wxString getLibraryName( const wxString& aFilePath );

    /// Helper function to add either existing or create new library.
    bool addLibrary( const wxString& aFilePath, bool aCreate, LIBRARY_TABLE_SCOPE aScope );

    /**
     * Return a set of #LIB_SYMBOL objects belonging to the original library.
     */
    std::set<LIB_SYMBOL*> getOriginalSymbols( const wxString& aLibrary );

    /**
     * Return an existing library buffer or creates one to using symbol library table to get
     * the original data.
     */
    LIB_BUFFER& getLibraryBuffer( const wxString& aLibrary );

protected:
    std::map<wxString, LIB_BUFFER> m_libs;       ///< The library buffers
    SCH_BASE_FRAME&                m_frame;      ///< Parent frame
    LIB_LOGGER*                    m_logger;
};

#endif /* SYMBOL_LIBRARY_MANAGER_H */
