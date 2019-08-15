/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_tree_synchronizing_adapter.h>
#include <sch_screen.h>

class LIB_ALIAS;
class LIB_PART;
class PART_LIB;
class SCH_PLUGIN;
class LIB_EDIT_FRAME;
class SYMBOL_LIB_TABLE;
class SYMBOL_LIB_TABLE_ROW;


class LIB_LOGGER : public wxLogGui
{
public:
    LIB_LOGGER() :
            m_previousLogger( nullptr ),
            m_activated( false )
    { }

    ~LIB_LOGGER() override
    {
        Deactivate();
    }

    void Activate()
    {
        if( !m_activated )
        {
            m_previousLogger = wxLog::GetActiveTarget();
            wxLog::SetActiveTarget( this );
            m_activated = true;
        }
    }

    void Deactivate()
    {
        if( m_activated )
        {
            Flush();
            m_activated = false;
            wxLog::SetActiveTarget( m_previousLogger );
        }
    }

    void Flush() override
    {
        if( m_bHasMessages )
        {
            wxLogMessage( _( "Not all libraries could be loaded.  Use the Manage Symbol Libraries dialog \n"
                             "to adjust paths and add or remove libraries." ) );
            wxLogGui::Flush();
        }
    }

private:
    wxLog* m_previousLogger;
    bool   m_activated;
};


/**
 * Class to handle modifications to the symbol libraries.
 */
class LIB_MANAGER
{
public:
    LIB_MANAGER( LIB_EDIT_FRAME& aFrame );

    /**
     * Updates the LIB_MANAGER data to synchronize with Symbol Library Table.
     */
    void Sync( bool aForce = false, std::function<void(int, int, const wxString&)> aProgressCallback
            = [](int, int, const wxString&){} );

    int GetHash() const;

    bool HasModifications() const;

    /**
     * Returns a library hash value to determine if it has changed.
     *
     * For buffered libraries, it returns a number corresponding to the number of modifications.
     * For original libraries, hash is computed basing on the library URI. Returns -1 when the
     * requested library does not exist.
     */
    int GetLibraryHash( const wxString& aLibrary ) const;

    /**
     * Returns the array of library names.
     */
    wxArrayString GetLibraryNames() const;

    /**
     * Finds a single library within the (aggregate) library table.
     */
    SYMBOL_LIB_TABLE_ROW* GetLibrary( const wxString& aLibrary ) const;

    std::list<LIB_ALIAS*> GetAliases( const wxString& aLibrary ) const;

    /**
     * Creates an empty library and adds it to the library table. The library file is created.
     */
    bool CreateLibrary( const wxString& aFilePath, SYMBOL_LIB_TABLE* aTable )
    {
        return addLibrary( aFilePath, true, aTable );
    }

    /**
     * Adds an existing library. The library is added to the library table as well.
     */
    bool AddLibrary( const wxString& aFilePath, SYMBOL_LIB_TABLE* aTable )
    {
        return addLibrary( aFilePath, false, aTable );
    }

    /**
     * Updates the part buffer with a new version of the part.
     * The library buffer creates a copy of the part.
     * It is required to save the library to use the updated part in the schematic editor.
     */
    bool UpdatePart( LIB_PART* aPart, const wxString& aLibrary );

    /**
     * Updates the part buffer with a new version of the part when the name has changed.
     * The old library buffer will be deleted and a new one created with the new name.
     */
    bool UpdatePartAfterRename( LIB_PART* aPart, const wxString& oldAlias,
                                const wxString& aLibrary );

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
     * Returns true if library exists.  If \a aCheckEnabled is set, then the library must
     * also be enabled in the library table.
     */
    bool LibraryExists( const wxString& aLibrary, bool aCheckEnabled = false ) const;

    /**
     * Returns true if library has unsaved modifications.
     */
    bool IsLibraryModified( const wxString& aLibrary ) const;

    /**
     * Returns true if part has unsaved modifications.
     */
    bool IsPartModified( const wxString& aAlias, const wxString& aLibrary ) const;

    /**
     * Clears the modified flag for all parts in a library.
     */
    bool ClearLibraryModified( const wxString& aLibrary ) const;

    /**
     * Clears the modified flag for a part.
     */
    bool ClearPartModified( const wxString& aAlias, const wxString& aLibrary ) const;

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
     * Saves library to a file, including unsaved changes.
     * @param aLibrary is the library name.
     * @param aFileName is the target file name.
     * @return True on success, false otherwise.
     */
    bool SaveLibrary( const wxString& aLibrary, const wxString& aFileName );

    /**
     * Reverts unsaved changes for a particular part.
     * @return The LIB_ID of the reverted part (which may be different in the case
     * of a rename)
     */
    LIB_ID RevertPart( const wxString& aAlias, const wxString& aLibrary );

    /**
     * Reverts unsaved changes for a particular library.
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
     * Returns a library name that is not currently in use.
     * Used for generating names for new libraries.
     */
    wxString GetUniqueLibraryName() const;

    /**
     * Returns the adapter object that provides the stored data.
     */
    LIB_TREE_MODEL_ADAPTER::PTR& GetAdapter() { return m_adapter; }

    /**
     * Returns the currently modified library name.
     */
    const wxString& GetCurrentLib() const { return m_currentLib; }
    void SetCurrentLib( const wxString& aLibrary ) { m_currentLib = aLibrary; }

    /**
     * Returns the currently modified part name.
     */
    const wxString& GetCurrentPart() const { return m_currentPart; }
    void SetCurrentPart( const wxString& aPart ) { m_currentPart = aPart; }

    /**
     * Returns the current library and part name as LIB_ID.
     */
    LIB_ID GetCurrentLibId() const
    {
        return LIB_ID( m_currentLib, m_currentPart );
    }

private:
    ///> Extracts library name basing on the file name
    static wxString getLibraryName( const wxString& aFilePath );

    ///> Helper function to add either existing or create new library
    bool addLibrary( const wxString& aFilePath, bool aCreate, SYMBOL_LIB_TABLE* aTable );

    ///> Returns the current Symbol Library Table
    SYMBOL_LIB_TABLE* symTable() const;

    ///> Class to store a working copy of a LIB_PART object and editor context.
    class PART_BUFFER
    {
    public:
        PART_BUFFER( LIB_PART* aPart = nullptr, std::unique_ptr<SCH_SCREEN> aScreen = nullptr );
        ~PART_BUFFER();

        LIB_PART* GetPart() const { return m_part; }
        void SetPart( LIB_PART* aPart );

        LIB_PART* GetOriginal() const { return m_original; }
        void SetOriginal( LIB_PART* aPart );

        bool IsModified() const;
        SCH_SCREEN* GetScreen() const { return m_screen.get(); }

        ///> Transfer the screen ownership
        std::unique_ptr<SCH_SCREEN> RemoveScreen()
        {
            return std::move( m_screen );
        }

        bool SetScreen( std::unique_ptr<SCH_SCREEN> aScreen )
        {
            bool ret = !!m_screen;
            m_screen = std::move( aScreen );
            return ret;
        }

        typedef std::shared_ptr<PART_BUFFER> PTR;
        typedef std::weak_ptr<PART_BUFFER> WEAK_PTR;

    private:
        std::unique_ptr<SCH_SCREEN> m_screen;

        LIB_PART* m_part;        // Working copy
        LIB_PART* m_original;    // Initial state of the part
    };


    ///> Class to store a working copy of a library
    class LIB_BUFFER
    {
    public:
        LIB_BUFFER( const wxString& aLibrary ) :
                m_libName( aLibrary ),
                m_hash( 1 )
        { }

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

        int GetHash() const { return m_hash; }

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

        ///> Saves stored modifications to Symbol Lib Table. It may result in saving the symbol
        ///> to disk as well, depending on the row properties.
        bool SaveBuffer( PART_BUFFER::PTR aPartBuf, SYMBOL_LIB_TABLE* aLibTable );

        ///> Saves stored modifications using a plugin. aBuffer decides whether the changes
        ///> should be cached or stored directly to the disk (for SCH_LEGACY_PLUGIN).
        bool SaveBuffer( PART_BUFFER::PTR aPartBuf, SCH_PLUGIN* aPlugin, bool aBuffer );

        ///> Returns a part buffer with LIB_PART holding a particular alias
        PART_BUFFER::PTR GetBuffer( const wxString& aAlias ) const
        {
            auto it = m_aliases.find( aAlias );
            return it != m_aliases.end() ? it->second.lock() : PART_BUFFER::PTR( nullptr );
        }

        ///> Returns all buffered parts
        const std::deque<PART_BUFFER::PTR>& GetBuffers() const { return m_parts; }

    private:
        ///> Creates alias entries for a particular part buffer
        bool addAliases( PART_BUFFER::PTR aPartBuf );

        ///> Removes alias entries for a particular part buffer
        bool removeAliases( PART_BUFFER::PTR aPartBuf );

        std::map<wxString, PART_BUFFER::WEAK_PTR> m_aliases;
        std::deque<PART_BUFFER::PTR> m_parts;
        std::deque<PART_BUFFER::PTR> m_deleted;  // Buffer for deleted parts until library is saved
        const wxString               m_libName;  // Buffered library name
        int                          m_hash;
    };

    ///> Returns a set of LIB_PART objects belonging to the original library
    std::set<LIB_PART*> getOriginalParts( const wxString& aLibrary );

    ///> Returns an existing library buffer or creates one to using
    ///> Symbol Library Table to get the original data.
    LIB_BUFFER& getLibraryBuffer( const wxString& aLibrary );

    ///> The library buffers
    std::map<wxString, LIB_BUFFER> m_libs;

    LIB_EDIT_FRAME& m_frame;         // Parent frame
    LIB_LOGGER      m_logger;
    int             m_syncHash;      // Symbol Lib Table hash value from the last synchronization

    wxString        m_currentLib;    // Currently modified part
    wxString        m_currentPart;   // Currently modified library

    SYMBOL_TREE_SYNCHRONIZING_ADAPTER::PTR m_adapter;
    SYMBOL_TREE_SYNCHRONIZING_ADAPTER* getAdapter()
    {
        return static_cast<SYMBOL_TREE_SYNCHRONIZING_ADAPTER*>( m_adapter.get() );
    }
};

#endif /* LIB_MANAGER_H */
