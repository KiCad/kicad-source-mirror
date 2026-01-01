/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCH_IO_LIB_CACHE_H_
#define _SCH_IO_LIB_CACHE_H_

#include <mutex>
#include <optional>

#include <wx/filename.h>

#include <symbol_library_common.h>


class LIB_SYMBOL;
class OUTPUTFORMATTER;


/**
 * A base cache assistant implementation for the symbol library portion of the #SCH_IO API.
 */
class SCH_IO_LIB_CACHE
{
public:
    SCH_IO_LIB_CACHE( const wxString& aLibraryPath );
    virtual ~SCH_IO_LIB_CACHE();

    void IncrementModifyHash()
    {
        std::lock_guard<std::mutex> mut( m_modHashMutex );
        m_modHash++;
    }

    int GetModifyHash()
    {
        std::lock_guard<std::mutex> mut( m_modHashMutex );
        return m_modHash;
    }

    // Most all functions in this class throw IO_ERROR exceptions.  There are no
    // error codes nor user interface calls from here, nor in any SCH_IO objects.
    // Catch these exceptions higher up please.

    /// Save the entire library to file m_libFileName;
    virtual void Save( const std::optional<bool>& aOpt = std::nullopt );

    virtual void Load() = 0;

    virtual void AddSymbol( const LIB_SYMBOL* aSymbol );

    virtual void DeleteSymbol( const wxString& aName ) = 0;

    virtual LIB_SYMBOL* GetSymbol( const wxString& aName );

    // If m_libFileName is a symlink follow it to the real source file
    wxFileName GetRealFile() const;

    long long GetLibModificationTime();

    bool IsFile( const wxString& aFullPathAndFileName ) const;

    bool IsFileChanged() const;

    void SetModified( bool aModified = true ) { m_isModified = aModified; }

    /**
     * @return true if the library had a parse error during loading.
     *
     * When a parse error occurs, only symbols before the error are loaded.
     * Saving a library in this state would permanently lose symbols after the error.
     */
    bool HasParseError() const { return m_hasParseError; }

    /**
     * Set the parse error state.
     *
     * @param aHasError true if the library had a parse error during loading.
     */
    void SetParseError( bool aHasError = true ) { m_hasParseError = aHasError; }

    wxString GetLogicalName() const { return m_libFileName.GetName(); }

    void SetFileName( const wxString& aFileName ) { m_libFileName = aFileName; }

    wxString GetFileName() const { return m_libFileName.GetFullPath(); }

    const LIB_SYMBOL_MAP& GetSymbolMap() const { return m_symbols; }

protected:
    LIB_SYMBOL* removeSymbol( LIB_SYMBOL* aAlias );

    int               m_modHash;      // Keep track of the modification status of the library.
    std::mutex        m_modHashMutex;

    wxString          m_fileName;     // Absolute path and file name.
    wxFileName        m_libFileName;  // Absolute path and file name is required here.
    long long         m_fileModTime;
    LIB_SYMBOL_MAP    m_symbols;      // Map of names of #LIB_SYMBOL pointers.
    bool              m_isWritable;
    bool              m_isModified;
    bool              m_hasParseError; // True if library had parse error during load.
    SCH_LIB_TYPE      m_libType;      // Is this cache a symbol or symbol library.
};

#endif   // _SCH_IO_LIB_CACHE_H_
