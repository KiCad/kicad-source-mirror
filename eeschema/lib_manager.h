/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LIB_MANAGER_H
#define LIB_MANAGER_H

#include <map>
#include <list>
#include <deque>
#include <set>
#include <memory>
#include <wx/arrstr.h>
#include <lib_manager_adapter.h>

class LIB_ALIAS;
class LIB_PART;
class LIB_BUFFER;
class PART_LIB;
class SCH_SCREEN;
class LIB_EDIT_FRAME;
class SYMBOL_LIB_TABLE;

/**
 * Class to handle modifications to the symbol libraries.
 */
class LIB_MANAGER
{
public:
    LIB_MANAGER( LIB_EDIT_FRAME& aFrame );

    /**
     * Updates the LIB_MANAGER data to account for the changes introduced to the project libraries.
     * @see PROJECT::SchLibs()
     */
    void Sync();

    int GetHash() const;

    int GetLibraryHash( const wxString& aLibrary ) const;

    /**
     * Returns the array of library names.
     */
    wxArrayString GetLibraryNames() const;

    /**
     * Returns a set containing all part names for a specific library.
     */
    wxArrayString GetAliasNames( const wxString& aLibrary ) const;

    std::list<LIB_ALIAS*> GetAliases( const wxString& aLibrary ) const;

    /**
     * Creates an empty library and adds it to the library table. The library file is created.
     */
    bool CreateLibrary( const wxString& aFilePath )
    {
        return addLibrary( aFilePath, true );
    }

    /**
     * Adds an existing library. The library is added to the library table as well.
     */
    bool AddLibrary( const wxString& aFilePath )
    {
        return addLibrary( aFilePath, false );
    }

    /**
     * Updates the part buffer with a new version of the part.
     * It is required to save the library to use the updated part in the schematic editor.
     */
    bool UpdatePart( LIB_PART* aPart, const wxString& aLibrary, wxString aOldName = wxEmptyString );

    /**
     * Removes the part from the part buffer.
     * It is required to save the library to have the part removed in the schematic editor.
     */
    bool RemovePart( const wxString& aName, const wxString& aLibrary );

    /**
     * Returns either an alias of a working LIB_PART copy, or alias of the original part if there
     * is no working copy.
     */
    LIB_ALIAS* GetAlias( const wxString& aAlias, const wxString& aLibrary ) const;

    /**
     * Returns the part copy from the buffer. In case it does not exist yet, the copy is created.
     * LIB_MANAGER retains the ownership.
     */
    LIB_PART* GetBufferedPart( const wxString& aAlias, const wxString& aLibrary );

    /**
     * Returns the screen used to edit a specific part. LIB_MANAGER retains the ownership.
     */
    SCH_SCREEN* GetScreen( const wxString& aAlias, const wxString& aLibrary );

    /**
     * Returns true if part with a specific alias exists in library (either original one or buffered).
     */
    bool PartExists( const wxString& aAlias, const wxString& aLibrary ) const;

    /**
     * Returns true if library exists.
     */
    bool LibraryExists( const wxString& aLibrary ) const;

    /**
     * Returns true if library has unsaved modifications.
     */
    bool IsLibraryModified( const wxString& aLibrary ) const;

    /**
     * Returns true if part has unsaved modifications.
     */
    bool IsPartModified( const wxString& aAlias, const wxString& aLibrary ) const;

    /**
     * Returns true if the library is stored in a read-only file.
     * @return True on success, false otherwise.
     */
    bool IsLibraryReadOnly( const wxString& aLibrary ) const;

    /**
     * Saves part changes to the library copy used by the schematic editor. Not it is not
     * necessarily saved to the file.
     * @return True on success, false otherwise.
     */
    bool FlushPart( const wxString& aAlias, const wxString& aLibrary );

    /**
     * Saves changes to the library copy used by the schematic editor. Note it is not
     * necessarily saved to the file.
     * @param aLibrary is the library name.
     * @return True on success, false otherwise.
     */
    bool FlushLibrary( const wxString& aLibrary );

    /**
     * Saves all changes to libraries.
     * @return True if all changes have been flushed successfully, false otherwise.
     */
    bool FlushAll();

    /**
     * Reverts unsaved changes for a particular part.
     * @return True on success, false otherwise.
     */
    bool RevertPart( const wxString& aAlias, const wxString& aLibrary );

    /**
     * Reverts unsaved changes for a particular library.
     * @return True on success, false otherwise.
     */
    bool RevertLibrary( const wxString& aLibrary );

    /**
     * Replaces all characters considered illegal in library/part names with underscores.
     */
    static wxString ValidateName( const wxString& aName );

    /**
     * Returns a library name that is not currently in use.
     * Used for generating names for new libraries.
     */
    wxString GetUniqueLibraryName() const;

    /**
     * Returns a component name that is not stored in a library.
     * Used for generating names for new components.
     */
    wxString GetUniqueComponentName( const wxString& aLibrary ) const;

    /**
     * Returns the adapter object that provides the stored data.
     */
    CMP_TREE_MODEL_ADAPTER_BASE::PTR& GetAdapter() { return m_adapter; }

private:
    ///> Parent frame
    LIB_EDIT_FRAME& m_frame;

    ///> Extracts library name basing on the file name
    static wxString getLibraryName( const wxString& aFilePath );

    ///> Helper function to add either existing or create new library
    bool addLibrary( const wxString& aFilePath, bool aCreate );

    SYMBOL_LIB_TABLE* m_symbolTable;

    ///> Class to store a working copy of a LIB_PART object and editor context.
    class PART_BUFFER
    {
    public:
        PART_BUFFER( LIB_PART* aPart = nullptr, SCH_SCREEN* aScreen = nullptr );
        ~PART_BUFFER();

        LIB_PART* GetPart() const { return m_part; }
        void SetPart( LIB_PART* aPart );

        LIB_PART* GetOriginal() const { return m_original; }
        void SetOriginal( LIB_PART* aPart );

        bool IsModified() const;
        SCH_SCREEN* GetScreen() const { return m_screen; }

        typedef std::shared_ptr<PART_BUFFER> PTR;
        typedef std::weak_ptr<PART_BUFFER> WEAK_PTR;

    private:
        SCH_SCREEN* m_screen;

        ///> Working copy
        LIB_PART* m_part;

        ///> Initial state of the part
        LIB_PART* m_original;
    };


    ///> Class to store a working copy of a library
    class LIB_BUFFER
    {
    public:
        LIB_BUFFER( const wxString& aLibrary )
            : m_libName( aLibrary ), m_hash( 1 )
        {
        }

        bool IsModified() const
        {
            if( !m_deleted.empty() )
                return true;

            for( const auto& partBuf : m_parts )
            {
                if( partBuf->IsModified() )
                    return true;
            }

            return false;
        }

        int GetHash() const
        {
            return m_hash;
        }

        ///> Returns all alias names for stored parts
        wxArrayString GetAliasNames() const;

        ///> Returns the working copy of a LIB_PART object with specified alias
        LIB_PART* GetPart( const wxString& aAlias ) const
        {
            auto buf = GetBuffer( aAlias );
            return buf ? buf->GetPart() : nullptr;
        }

        ///> Creates a new buffer to store a part. LIB_BUFFER takes ownership of aCopy.
        bool CreateBuffer( LIB_PART* aCopy, SCH_SCREEN* aScreen );

        ///> Updates the stored part. LIB_BUFFER takes ownership of aCopy.
        bool UpdateBuffer( PART_BUFFER::PTR aPartBuf, LIB_PART* aCopy );

        bool DeleteBuffer( PART_BUFFER::PTR aPartBuf );

        void ClearDeletedBuffer()
        {
            m_deleted.clear();
        }

        ///> Saves stored modifications to a Symbol Library Table
        bool SaveBuffer( PART_BUFFER::PTR aPartBuf, SYMBOL_LIB_TABLE* aLibTable );

        ///> Returns a part buffer with LIB_PART holding a particular alias
        PART_BUFFER::PTR GetBuffer( const wxString& aAlias ) const
        {
            auto it = m_aliases.find( aAlias );
            return it != m_aliases.end() ? it->second.lock() : PART_BUFFER::PTR( nullptr );
        }

        ///> Returns all buffered parts
        const std::deque<PART_BUFFER::PTR>& GetBuffers() const
        {
            return m_parts;
        }

        ///> Returns all aliases of buffered parts
        const std::map<wxString, PART_BUFFER::WEAK_PTR>& GetAliases() const
        {
            return m_aliases;
        }

    private:
        ///> Creates alias entries for a particular part buffer
        bool addAliases( PART_BUFFER::PTR aPartBuf );

        ///> Removes alias entries for a particular part buffer
        bool removeAliases( PART_BUFFER::PTR aPartBuf );

        std::map<wxString, PART_BUFFER::WEAK_PTR> m_aliases;
        std::deque<PART_BUFFER::PTR> m_parts;

        ///> Buffer to keep deleted parts until the library is saved
        std::deque<PART_BUFFER::PTR> m_deleted;

        /// Buffered library name
        const wxString m_libName;

        int m_hash;

        friend class PART_BUFFER;
    };

    ///> Returns an existing library buffer or creates one to using
    ///> Symbol Library Table to get the original data.
    LIB_BUFFER& getLibraryBuffer( const wxString& aLibrary );

    ///> The library buffers
    std::map<wxString, LIB_BUFFER> m_libs;

    LIB_MANAGER_ADAPTER::PTR m_adapter;
    LIB_MANAGER_ADAPTER* getAdapter() { return static_cast<LIB_MANAGER_ADAPTER*>( m_adapter.get() ); }
};

#endif /* LIB_MANAGER_H */
