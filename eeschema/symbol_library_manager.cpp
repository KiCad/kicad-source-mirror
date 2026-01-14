/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "symbol_library_manager.h"

#include <dialogs/html_message_box.h>
#include <symbol_edit_frame.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <project_sch.h>
#include <kiway.h>
#include <core/profile.h>
#include <wx_filename.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <progress_reporter.h>
#include <list>
#include <locale_io.h>
#include <confirm.h>
#include <string_utils.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>

#include "lib_logger.h"


SYMBOL_LIBRARY_MANAGER::SYMBOL_LIBRARY_MANAGER( SCH_BASE_FRAME& aFrame ) :
        m_frame( aFrame )
{
    m_logger = new LIB_LOGGER();
}


SYMBOL_LIBRARY_MANAGER::~SYMBOL_LIBRARY_MANAGER()
{
    delete m_logger;
}


bool SYMBOL_LIBRARY_MANAGER::HasModifications() const
{
    for( const auto& [name, buffer] : m_libs )
    {
        if( buffer.IsModified() )
            return true;
    }

    return false;
}


int SYMBOL_LIBRARY_MANAGER::GetHash() const
{
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
    return adapter->GetModifyHash();
}


int SYMBOL_LIBRARY_MANAGER::GetLibraryHash( const wxString& aLibrary ) const
{
    if( const auto libBufIt = m_libs.find( aLibrary ); libBufIt != m_libs.end() )
        return libBufIt->second.GetHash();

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    if( auto uri = manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, aLibrary, true ); uri )
    {
        return std::hash<std::string>{}( aLibrary.ToStdString() + uri->ToStdString() );
    }

    return -1;
}


wxArrayString SYMBOL_LIBRARY_MANAGER::GetLibraryNames() const
{
    wxArrayString res;

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    for( const LIBRARY_TABLE_ROW* row : manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL ) )
    {
        // Database libraries are hidden from the symbol editor at the moment
        if( row->Type() == SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_DATABASE ) )
            continue;

        res.Add( row->Nickname() );
    }

    return res;
}


bool SYMBOL_LIBRARY_MANAGER::SaveLibrary( const wxString& aLibrary, const wxString& aFileName,
                                          SCH_IO_MGR::SCH_FILE_T aFileType )
{
    wxCHECK( aFileType != SCH_IO_MGR::SCH_FILE_T::SCH_LEGACY, false );

    wxFileName fn( aFileName );

    if( fn.FileExists() && !fn.IsFileWritable() )
        return false;

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( aFileType ) );
    bool                res = true;    // assume all libraries are successfully saved
    std::map<std::string, UTF8>     properties;

    properties.emplace( SCH_IO_KICAD_LEGACY::PropBuffering, "" );

    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
    {
        // Handle buffered library
        LIB_BUFFER& libBuf = it->second;

        const auto& symbolBuffers = libBuf.GetBuffers();

        for( const std::shared_ptr<SYMBOL_BUFFER>& symbolBuf : symbolBuffers )
        {
            wxCHECK2( symbolBuf, continue );

            if( !libBuf.SaveBuffer( *symbolBuf, aFileName, &*pi, true ) )
            {
                // Something went wrong, but try to save other libraries
                res = false;
            }
        }

        // clear the deleted symbols buffer only if data is saved to the original file
        wxFileName original, destination( aFileName );
        LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

        if( auto uri = manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, aLibrary, true ); uri )
        {
            original = *uri;
            original.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
        }

        destination.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );

        if( res && original == destination )
        {
            // Delete symbols that were removed from the buffer before clearing the deleted list
            for( const std::shared_ptr<SYMBOL_BUFFER>& deletedBuf : libBuf.GetDeletedBuffers() )
            {
                wxCHECK2( deletedBuf, continue );

                const wxString& originalName = deletedBuf->GetOriginal().GetName();

                try
                {
                    if( pi->LoadSymbol( aFileName, originalName ) )
                        pi->DeleteSymbol( aFileName, originalName, &properties );
                }
                catch( const IO_ERROR& ioe )
                {
                    wxLogError( _( "Error deleting symbol %s from library '%s'." ) + wxS( "\n%s" ),
                                UnescapeString( originalName ), aFileName, ioe.What() );
                    res = false;
                }
            }

            libBuf.ClearDeletedBuffer();
        }
    }
    else
    {
        // Handle original library
        for( LIB_SYMBOL* symbol : getOriginalSymbols( aLibrary ) )
        {
            LIB_SYMBOL* newSymbol;

            try
            {
                if( symbol->IsDerived() )
                {
                    std::shared_ptr< LIB_SYMBOL > oldParent = symbol->GetParent().lock();

                    wxCHECK_MSG( oldParent, false,
                                 wxString::Format( wxT( "Derived symbol '%s' found with undefined parent." ),
                                                   symbol->GetName() ) );

                    LIB_SYMBOL* libParent = pi->LoadSymbol( aLibrary, oldParent->GetName(), &properties );

                    if( !libParent )
                    {
                        libParent = new LIB_SYMBOL( *oldParent );
                        pi->SaveSymbol( aLibrary, libParent, &properties );
                    }
                    else
                    {
                        // Copy embedded files from the in-memory parent to the loaded parent
                        // This ensures that any embedded files added to the parent are preserved
                        //
                        // We do this manually rather than using the assignment operator to avoid
                        // potential ABI issues where the size of EMBEDDED_FILES differs between
                        // compilation units, potentially causing the assignment to overwrite
                        // members of LIB_SYMBOL (like m_me) that follow the EMBEDDED_FILES base.
                        libParent->ClearEmbeddedFiles();

                        for( const auto& [name, file] : oldParent->EmbeddedFileMap() )
                            libParent->AddFile( new EMBEDDED_FILES::EMBEDDED_FILE( *file ) );

                        libParent->SetAreFontsEmbedded( oldParent->GetAreFontsEmbedded() );
                        libParent->SetFileAddedCallback( oldParent->GetFileAddedCallback() );
                    }

                    newSymbol = new LIB_SYMBOL( *symbol );
                    newSymbol->SetParent( libParent );
                    pi->SaveSymbol( aLibrary, newSymbol, &properties );
                }
                else if( !pi->LoadSymbol( aLibrary, symbol->GetName(), &properties ) )
                {
                    pi->SaveSymbol( aLibrary, new LIB_SYMBOL( *symbol ), &properties );
                }
            }
            catch( ... )
            {
                res = false;
                break;
            }
        }
    }

    try
    {
        pi->SaveLibrary( aFileName );
    }
    catch( ... )
    {
        // return false because something happens.
        // The library is not successfully saved
        res = false;
    }

    return res;
}


bool SYMBOL_LIBRARY_MANAGER::IsLibraryModified( const wxString& aLibrary ) const
{
    auto it = m_libs.find( aLibrary );
    return it != m_libs.end() ? it->second.IsModified() : false;
}


bool SYMBOL_LIBRARY_MANAGER::IsSymbolModified( const wxString& aSymbolName, const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    const LIB_BUFFER&                    buf = libIt->second;
    const std::shared_ptr<SYMBOL_BUFFER> symbolBuf = buf.GetBuffer( aSymbolName );

    return symbolBuf ? symbolBuf->IsModified() : false;
}


void SYMBOL_LIBRARY_MANAGER::SetSymbolModified( const wxString& aSymbolName, const wxString& aLibrary )
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return;

    const LIB_BUFFER&              buf = libIt->second;
    std::shared_ptr<SYMBOL_BUFFER> symbolBuf = buf.GetBuffer( aSymbolName );

    wxCHECK( symbolBuf, /* void */ );

    symbolBuf->GetScreen()->SetContentModified();
}


bool SYMBOL_LIBRARY_MANAGER::ClearLibraryModified( const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    for( auto& symbolBuf : libIt->second.GetBuffers() )
    {
        SCH_SCREEN* screen = symbolBuf->GetScreen();

        if( screen )
            screen->SetContentModified( false );
    }

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::ClearSymbolModified( const wxString& aSymbolName, const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    auto symbolBuf = libIt->second.GetBuffer( aSymbolName );
    wxCHECK( symbolBuf, false );

    symbolBuf->GetScreen()->SetContentModified( false );
    return true;
}


bool SYMBOL_LIBRARY_MANAGER::IsLibraryReadOnly( const wxString& aLibrary ) const
{
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
    return !adapter->IsSymbolLibWritable( aLibrary );
}


bool SYMBOL_LIBRARY_MANAGER::IsLibraryLoaded( const wxString& aLibrary ) const
{
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
    return adapter->IsLibraryLoaded( aLibrary );
}


std::list<LIB_SYMBOL*> SYMBOL_LIBRARY_MANAGER::EnumerateSymbols( const wxString& aLibrary ) const
{
    std::list<LIB_SYMBOL*> ret;
    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        for( const std::shared_ptr<SYMBOL_BUFFER>& symbolBuf : libIt->second.GetBuffers() )
            ret.push_back( &symbolBuf->GetSymbol() );
    }
    else
    {
        SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
        std::vector<LIB_SYMBOL*> symbols = adapter->GetSymbols( aLibrary );

        std::copy( symbols.begin(), symbols.end(), std::back_inserter( ret ) );
    }

    return ret;
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::GetBufferedSymbol( const wxString& aSymbolName, const wxString& aLibrary )
{
    // try the library buffers first
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    LIB_SYMBOL* bufferedSymbol = libBuf.GetSymbol( aSymbolName );

    if( !bufferedSymbol ) // no buffer symbol found
    {
        SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );

        // create a copy of the symbol
        try
        {
            LIB_SYMBOL* symbol = adapter->LoadSymbol( aLibrary, aSymbolName );

            if( symbol == nullptr )
                THROW_IO_ERROR( _( "Symbol not found." ) );

            LIB_SYMBOL* bufferedParent = nullptr;

            // Create parent symbols on demand so parent symbol can be set.
            if( symbol->IsDerived() )
            {
                std::shared_ptr<LIB_SYMBOL> parent = symbol->GetParent().lock();
                wxCHECK_MSG( parent, nullptr, wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                                                symbol->GetName() ) );

                // Check if the parent symbol buffer has already be created.
                bufferedParent = libBuf.GetSymbol( parent->GetName() );

                if( !bufferedParent )
                {
                    std::unique_ptr<LIB_SYMBOL> newParent = std::make_unique<LIB_SYMBOL>( *parent );
                    bufferedParent = newParent.get();
                    libBuf.CreateBuffer( std::move( newParent ), std::make_unique<SCH_SCREEN>() );
                }
            }

            std::unique_ptr<LIB_SYMBOL> newSymbol = std::make_unique<LIB_SYMBOL>( *symbol );
            bufferedSymbol = newSymbol.get();

            if( bufferedParent )
                newSymbol->SetParent( bufferedParent );

            libBuf.CreateBuffer( std::move( newSymbol ), std::make_unique<SCH_SCREEN>() );
        }
        catch( const IO_ERROR& e )
        {
            wxString msg;

            msg.Printf( _( "Error loading symbol %s from library '%s'." ),
                        aSymbolName,
                        aLibrary );
            DisplayErrorMessage( &m_frame, msg, e.What() );
            bufferedSymbol = nullptr;
        }
    }

    return bufferedSymbol;
}


SCH_SCREEN* SYMBOL_LIBRARY_MANAGER::GetScreen( const wxString& aSymbolName, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );
    wxCHECK( it != m_libs.end(), nullptr );

    LIB_BUFFER&                    buf = it->second;
    std::shared_ptr<SYMBOL_BUFFER> symbolBuf = buf.GetBuffer( aSymbolName );

    return symbolBuf ? symbolBuf->GetScreen() : nullptr;
}


bool SYMBOL_LIBRARY_MANAGER::UpdateSymbol( LIB_SYMBOL* aSymbol, const wxString& aLibrary )
{
    LIB_BUFFER&                    libBuf = getLibraryBuffer( aLibrary );
    std::shared_ptr<SYMBOL_BUFFER> symbolBuf = libBuf.GetBuffer( aSymbol->GetName() );

    if( symbolBuf )     // Existing symbol.
    {
        LIB_SYMBOL& bufferedSymbol = symbolBuf->GetSymbol();

        // If we are coming from a different library, the library ID needs to be preserved
        const LIB_ID libId = bufferedSymbol.GetLibId();
        bufferedSymbol = *aSymbol;
        bufferedSymbol.SetLibId( libId );

        symbolBuf->GetScreen()->SetContentModified();
    }
    else                // New symbol
    {
        std::unique_ptr<LIB_SYMBOL> symbolCopy = std::make_unique<LIB_SYMBOL>( *aSymbol, nullptr );

        symbolCopy->SetLibId( LIB_ID( aLibrary, aSymbol->GetLibId().GetLibItemName() ) );

        auto        newScreen = std::make_unique<SCH_SCREEN>();
        SCH_SCREEN& screen = *newScreen;
        libBuf.CreateBuffer( std::move( symbolCopy ), std::move( newScreen ) );
        screen.SetContentModified();
    }

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::UpdateSymbolAfterRename( LIB_SYMBOL* aSymbol, const wxString& aOldName,
                                                      const wxString& aLibrary )
{
    LIB_BUFFER&                    libBuf = getLibraryBuffer( aLibrary );
    std::shared_ptr<SYMBOL_BUFFER> symbolBuf = libBuf.GetBuffer( aOldName );

    wxCHECK( symbolBuf && aSymbol, false );

    libBuf.UpdateBuffer( *symbolBuf, *aSymbol );
    OnDataChanged();

    return true;
}


LIB_ID SYMBOL_LIBRARY_MANAGER::RevertSymbol( const wxString& aSymbolName, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return LIB_ID( aLibrary, aSymbolName );

    std::shared_ptr<SYMBOL_BUFFER> symbolBuf = it->second.GetBuffer( aSymbolName );
    wxCHECK( symbolBuf, LIB_ID( aLibrary, aSymbolName ) );
    LIB_SYMBOL original( symbolBuf->GetOriginal() );

    if( original.GetName() != aSymbolName )
    {
        UpdateSymbolAfterRename( &original, aSymbolName, aLibrary );
    }
    else
    {
        // copy the initial data to the current symbol to restore
        symbolBuf->GetSymbol() = original;
        OnDataChanged();
    }

    return LIB_ID( aLibrary, original.GetName() );
}


bool SYMBOL_LIBRARY_MANAGER::RevertLibrary( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // nothing to reverse
        return false;

    m_libs.erase( it );
    OnDataChanged();

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::RevertAll()
{
    bool retv = true;

    // Nothing to revert.
    if( GetHash() == 0 )
        return true;

    for( const auto& lib : m_libs )
    {
        if( !lib.second.IsModified() )
            continue;

        for( const std::shared_ptr<SYMBOL_BUFFER>& buffer : lib.second.GetBuffers() )
        {
            if( !buffer->IsModified() )
                continue;

            RevertSymbol( lib.first, buffer->GetOriginal().GetName() );
        }
    }

    return retv;
}


bool SYMBOL_LIBRARY_MANAGER::RemoveSymbol( const wxString& aSymbolName, const wxString& aLibrary )
{
    LIB_BUFFER&                    libBuf = getLibraryBuffer( aLibrary );
    std::shared_ptr<SYMBOL_BUFFER> symbolBuf = libBuf.GetBuffer( aSymbolName );
    wxCHECK( symbolBuf, false );

    bool retv = true;

    retv &= libBuf.DeleteBuffer( *symbolBuf );
    OnDataChanged();

    return retv;
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::GetSymbol( const wxString& aSymbolName, const wxString& aLibrary ) const
{
    // Try the library buffers first
    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        LIB_SYMBOL* symbol = libIt->second.GetSymbol( aSymbolName );

        if( symbol )
            return symbol;
    }

    // Get the original symbol
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
    LIB_SYMBOL* symbol = nullptr;

    try
    {
        symbol = adapter->LoadSymbol( aLibrary, aSymbolName );
    }
    catch( const IO_ERROR& e )
    {
        wxString msg;

        msg.Printf( _( "Cannot load symbol '%s' from library '%s'." ),
                    aSymbolName,
                    aLibrary );
        DisplayErrorMessage( &m_frame, msg, e.What() );
    }

    return symbol;
}


bool SYMBOL_LIBRARY_MANAGER::SymbolExists( const wxString& aSymbolName,
                                           const wxString& aLibrary ) const
{
    auto        libBufIt = m_libs.find( aLibrary );
    LIB_SYMBOL* symbol = nullptr;

    if( libBufIt != m_libs.end() )
        return libBufIt->second.GetBuffer( aSymbolName ) != nullptr;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );

    try
    {
        symbol = adapter->LoadSymbol( aLibrary, aSymbolName );
    }
    catch( IO_ERROR& )
    {
        // checking if certain symbol exists, so its absence is perfectly fine
    }

    return symbol != nullptr;
}


bool SYMBOL_LIBRARY_MANAGER::SymbolNameInUse( const wxString& aName, const wxString& aLibrary )
{
    wxArrayString existing;

    // NB: GetSymbolNames() is mostly used for GUI stuff, so it returns unescaped names
    GetSymbolNames( aLibrary, existing );

    wxString unescapedName = UnescapeString( aName );

    for( wxString& candidate : existing )
    {
        if( candidate.CmpNoCase( unescapedName ) == 0 )
            return true;
    }

    return false;
}


bool SYMBOL_LIBRARY_MANAGER::LibraryExists( const wxString& aLibrary, bool aCheckEnabled ) const
{
    if( aLibrary.IsEmpty() )
        return false;

    if( m_libs.count( aLibrary ) > 0 )
        return true;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );

    return adapter->HasLibrary( aLibrary, aCheckEnabled );
}


wxString SYMBOL_LIBRARY_MANAGER::GetUniqueLibraryName() const
{
    wxString name = "New_Library";

    if( !LibraryExists( name ) )
        return name;

    name += "_";

    for( unsigned int i = 0; i < std::numeric_limits<unsigned int>::max(); ++i )
    {
        if( !LibraryExists( name + wxString::Format( "%u", i ) ) )
            return name + wxString::Format( "%u", i );
    }

    wxFAIL;
    return wxEmptyString;
}


void SYMBOL_LIBRARY_MANAGER::GetSymbolNames( const wxString& aLibraryName, wxArrayString& aSymbolNames,
                                             SYMBOL_NAME_FILTER aFilter )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibraryName );

    libBuf.GetSymbolNames( aSymbolNames, aFilter );
}


size_t SYMBOL_LIBRARY_MANAGER::GetDerivedSymbolNames( const wxString& aSymbolName, const wxString& aLibraryName,
                                                      wxArrayString& aList )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibraryName );

    return libBuf.GetDerivedSymbolNames( aSymbolName, aList );
}


size_t SYMBOL_LIBRARY_MANAGER::GetLibraryCount() const
{
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
    return adapter->GetLibraryNames().size();
}


wxString SYMBOL_LIBRARY_MANAGER::getLibraryName( const wxString& aFilePath )
{
    wxFileName fn( aFilePath );
    return fn.GetName();
}


bool SYMBOL_LIBRARY_MANAGER::addLibrary( const wxString& aFilePath, bool aCreate, LIBRARY_TABLE_SCOPE aScope )
{
    wxString libName = getLibraryName( aFilePath );
    wxCHECK( !LibraryExists( libName ), false );  // either create or add an existing one

    // try to use path normalized to an environmental variable or project path
    wxString relPath = NormalizePath( aFilePath, &Pgm().GetLocalEnvVariables(), &m_frame.Prj() );

    SCH_IO_MGR::SCH_FILE_T schFileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( aFilePath, aCreate ? KICTL_CREATE
                                                                                                    : 0 );

    if( schFileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        schFileType = SCH_IO_MGR::SCH_LEGACY;

    SYMBOL_LIBRARY_ADAPTER*       adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );
    LIBRARY_MANAGER&              manager = Pgm().GetLibraryManager();
    std::optional<LIBRARY_TABLE*> optTable = manager.Table( LIBRARY_TABLE_TYPE::SYMBOL, aScope );
    wxCHECK( optTable, false );
    LIBRARY_TABLE* table = optTable.value();
    bool           success = true;

    try
    {
        LIBRARY_TABLE_ROW& row = table->InsertRow();

        row.SetNickname( libName );
        row.SetURI( relPath );
        row.SetType( SCH_IO_MGR::ShowType( schFileType ) );

        table->Save().map_error(
                [&]( const LIBRARY_ERROR& aError )
                {
                    wxMessageBox( _( "Error saving library table:\n\n" ) + aError.message,
                                  _( "File Save Error" ), wxOK | wxICON_ERROR );
                    success = false;
                } );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( nullptr, ioe.What() );
        return false;
    }

    if( success )
    {
        manager.ReloadTables( aScope, { LIBRARY_TABLE_TYPE::SYMBOL } );

        // Tables are reinitialized. So reinit table reference.
        optTable = manager.Table( LIBRARY_TABLE_TYPE::SYMBOL, aScope );
        wxCHECK( optTable, false );
        table = optTable.value();

        if( aCreate )
        {
            wxCHECK( schFileType != SCH_IO_MGR::SCH_FILE_T::SCH_LEGACY, false );

            if( !adapter->CreateLibrary( libName ) )
            {
                table->Rows().erase( table->Rows().end() - 1 );
                return false;
            }
        }

        adapter->LoadOne( libName );
        OnDataChanged();
    }

    return success;
}


std::set<LIB_SYMBOL*> SYMBOL_LIBRARY_MANAGER::getOriginalSymbols( const wxString& aLibrary )
{
    std::set<LIB_SYMBOL*> symbols;
    wxCHECK( LibraryExists( aLibrary ), symbols );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame.Prj() );

    for( LIB_SYMBOL* symbol : adapter->GetSymbols( aLibrary ) )
        symbols.insert( symbol );

    return symbols;
}


LIB_BUFFER& SYMBOL_LIBRARY_MANAGER::getLibraryBuffer( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
        return it->second;

    // The requested buffer does not exist yet, so create one
    auto ret = m_libs.emplace( aLibrary, LIB_BUFFER( aLibrary ) );
    LIB_BUFFER& buf = ret.first->second;

    for( LIB_SYMBOL* symbol : getOriginalSymbols( aLibrary ) )
    {
        if( symbol->IsDerived() )
        {
            std::shared_ptr<LIB_SYMBOL> oldParent = symbol->GetParent().lock();

            wxCHECK_MSG( oldParent, buf, wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                                           symbol->GetName() ) );

            LIB_SYMBOL* libParent = buf.GetSymbol( oldParent->GetName() );

            if( !libParent )
            {
                std::unique_ptr<LIB_SYMBOL> newParent = std::make_unique<LIB_SYMBOL>( *oldParent.get() );
                libParent = newParent.get();
                buf.CreateBuffer( std::move( newParent ), std::make_unique<SCH_SCREEN>() );
            }

            std::unique_ptr<LIB_SYMBOL> newSymbol = std::make_unique<LIB_SYMBOL>( *symbol );
            newSymbol->SetParent( libParent );
            buf.CreateBuffer( std::move( newSymbol ), std::make_unique<SCH_SCREEN>() );
        }
        else if( !buf.GetSymbol( symbol->GetName() ) )
        {
            buf.CreateBuffer( std::make_unique<LIB_SYMBOL>( *symbol ), std::make_unique<SCH_SCREEN>() );
        }
    }

    return buf;
}


bool SYMBOL_LIBRARY_MANAGER::UpdateLibraryBuffer( const wxString& aLibrary )
{
    try
    {
        m_libs.erase( aLibrary );
        getLibraryBuffer( aLibrary );
    }
    catch( const std::exception& e )
    {
        wxLogError( _( "Error updating library buffer: %s" ), e.what() );
        return false;
    }
    catch(...)
    {
        wxLogError( _( "Error updating library buffer." ) );
        return false;
    }

    getLibraryBuffer( aLibrary );

    return true;
}


SYMBOL_BUFFER::SYMBOL_BUFFER( std::unique_ptr<LIB_SYMBOL> aSymbol,
                              std::unique_ptr<SCH_SCREEN> aScreen ) :
        m_screen( std::move( aScreen ) ),
        m_symbol( std::move( aSymbol ) )
{
    wxASSERT( m_symbol );
    m_original = std::make_unique<LIB_SYMBOL>( *m_symbol );
    wxASSERT( m_original );
}


SYMBOL_BUFFER::~SYMBOL_BUFFER()
{
}


void SYMBOL_BUFFER::SetSymbol( std::unique_ptr<LIB_SYMBOL> aSymbol )
{
    wxCHECK( m_symbol != aSymbol, /* void */ );
    wxASSERT( aSymbol );
    m_symbol = std::move( aSymbol );

    // If the symbol moves libraries then the original moves with it
    if( m_original->GetLibId().GetLibNickname() != m_symbol->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_symbol->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


void SYMBOL_BUFFER::SetOriginal( std::unique_ptr<LIB_SYMBOL> aSymbol )
{
    wxCHECK( m_original != aSymbol, /* void */ );
    wxASSERT( aSymbol );
    m_original = std::move( aSymbol );

    // The original is not allowed to have a different library than its symbol
    if( m_original->GetLibId().GetLibNickname() != m_symbol->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_symbol->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


bool SYMBOL_BUFFER::IsModified() const
{
    return m_screen && m_screen->IsContentModified();
}


LIB_SYMBOL* LIB_BUFFER::GetSymbol( const wxString& aAlias ) const
{
    auto buf = GetBuffer( aAlias );

    if( !buf )
        return nullptr;

    LIB_SYMBOL& symbol = buf->GetSymbol();
    return &symbol;
}


bool LIB_BUFFER::CreateBuffer( std::unique_ptr<LIB_SYMBOL> aCopy,
                               std::unique_ptr<SCH_SCREEN> aScreen )
{
    wxASSERT( aCopy );
    wxASSERT( aCopy->GetLib() == nullptr );

    // Set the parent library name,
    // otherwise it is empty as no library has been given as the owner during object construction
    LIB_ID libId = aCopy->GetLibId();
    libId.SetLibNickname( m_libName );
    aCopy->SetLibId( libId );

    auto symbolBuf = std::make_shared<SYMBOL_BUFFER>( std::move( aCopy ), std::move( aScreen ) );
    m_symbols.push_back( std::move( symbolBuf ) );

    ++m_hash;

    return true;
}


bool LIB_BUFFER::UpdateBuffer( SYMBOL_BUFFER& aSymbolBuf, const LIB_SYMBOL& aCopy )
{
    LIB_SYMBOL& bufferedSymbol = aSymbolBuf.GetSymbol();

    bufferedSymbol = aCopy;
    ++m_hash;

    return true;
}


bool LIB_BUFFER::DeleteBuffer( const SYMBOL_BUFFER& aSymbolBuf )
{
    const auto sameBufferPredicate = [&]( const std::shared_ptr<SYMBOL_BUFFER>& aBuf )
    {
        return aBuf.get() == &aSymbolBuf;
    };

    auto symbolBufIt = std::find_if( m_symbols.begin(), m_symbols.end(), sameBufferPredicate );
    wxCHECK( symbolBufIt != m_symbols.end(), false );

    bool retv = true;

    // Remove all derived symbols to prevent broken inheritance.
    if( HasDerivedSymbols( aSymbolBuf.GetSymbol().GetName() )
        && ( removeChildSymbols( aSymbolBuf ) == 0 ) )
    {
        retv = false;
    }

    m_deleted.emplace_back( *symbolBufIt );
    m_symbols.erase( symbolBufIt );
    ++m_hash;

    return retv;
}


bool LIB_BUFFER::SaveBuffer( SYMBOL_BUFFER& aSymbolBuf, const wxString& aFileName, SCH_IO* aPlugin,
                             bool aBuffer )
{
    wxCHECK( !aFileName.IsEmpty(), false );

    wxString errorMsg = _( "Error saving symbol %s to library '%s'." ) + wxS( "\n%s" );

    // Set properties to prevent saving the file on every symbol save.
    std::map<std::string, UTF8> properties;
    properties.emplace( SCH_IO_KICAD_LEGACY::PropBuffering, "" );

    LIB_SYMBOL& libSymbol = aSymbolBuf.GetSymbol();

    {
        LIB_SYMBOL&    originalSymbol = aSymbolBuf.GetOriginal();
        const wxString originalName = originalSymbol.GetName();

        // Delete the original symbol if the symbol name has been changed.
        if( libSymbol.GetName() != originalSymbol.GetName() )
        {
            try
            {
                if( aPlugin->LoadSymbol( aFileName, originalName ) )
                    aPlugin->DeleteSymbol( aFileName, originalName, &properties );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( originalName ), aFileName, ioe.What() );
                return false;
            }
        }
    }

    LIB_SYMBOL* parentSymbol = nullptr;

    if( libSymbol.IsDerived() )
    {
        LIB_SYMBOL*                 newCachedSymbol = new LIB_SYMBOL( libSymbol );
        std::shared_ptr<LIB_SYMBOL> bufferedParent = libSymbol.GetParent().lock();
        parentSymbol = newCachedSymbol;

        wxCHECK( bufferedParent, false );

        LIB_SYMBOL* cachedParent = nullptr;

        try
        {
            cachedParent = aPlugin->LoadSymbol( aFileName, bufferedParent->GetName() );
        }
        catch( const IO_ERROR& )
        {
            return false;
        }

        if( !cachedParent )
        {
            cachedParent = new LIB_SYMBOL( *bufferedParent.get() );
            newCachedSymbol->SetParent( cachedParent );

            try
            {
                aPlugin->SaveSymbol( aFileName, cachedParent, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( cachedParent->GetName() ), aFileName,
                            ioe.What() );
                return false;
            }

            try
            {
                aPlugin->SaveSymbol( aFileName, newCachedSymbol, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( newCachedSymbol->GetName() ), aFileName,
                            ioe.What() );
                return false;
            }

            auto        originalParent = std::make_unique<LIB_SYMBOL>( *bufferedParent.get() );
            LIB_SYMBOL& parentRef = *originalParent;
            aSymbolBuf.SetOriginal( std::move( originalParent ) );

            auto newSymbol = std::make_unique<LIB_SYMBOL>( libSymbol );
            newSymbol->SetParent( &parentRef );
            aSymbolBuf.SetOriginal( std::move( newSymbol ) );
        }
        else
        {
            // Copy embedded files from the buffered parent to the cached parent
            // This ensures that any embedded files added to the parent are preserved
            //
            // We do this manually rather than using the assignment operator to avoid
            // potential ABI issues where the size of EMBEDDED_FILES differs between
            // compilation units, potentially causing the assignment to overwrite
            // members of LIB_SYMBOL (like m_me) that follow the EMBEDDED_FILES base.
            cachedParent->ClearEmbeddedFiles();

            for( const auto& [name, file] : bufferedParent->EmbeddedFileMap() )
                cachedParent->AddFile( new EMBEDDED_FILES::EMBEDDED_FILE( *file ) );

            cachedParent->SetAreFontsEmbedded( bufferedParent->GetAreFontsEmbedded() );
            cachedParent->SetFileAddedCallback( bufferedParent->GetFileAddedCallback() );

            newCachedSymbol->SetParent( cachedParent );

            try
            {
                aPlugin->SaveSymbol( aFileName, newCachedSymbol, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( newCachedSymbol->GetName() ), aFileName,
                            ioe.What() );
                return false;
            }

            auto originalBufferedParent = GetBuffer( bufferedParent->GetName() );
            wxCHECK( originalBufferedParent, false );

            auto newSymbol = std::make_unique<LIB_SYMBOL>( libSymbol );
            newSymbol->SetParent( &originalBufferedParent->GetSymbol() );
            aSymbolBuf.SetOriginal( std::move( newSymbol ) );
        }
    }
    else
    {
        parentSymbol = new LIB_SYMBOL( libSymbol );

        try
        {
            aPlugin->SaveSymbol( aFileName, parentSymbol, aBuffer ? &properties : nullptr );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( errorMsg, UnescapeString( libSymbol.GetName() ), aFileName, ioe.What() );
            return false;
        }

        aSymbolBuf.SetOriginal( std::make_unique<LIB_SYMBOL>( libSymbol ) );
    }

    wxArrayString derivedSymbols;

    // Reparent all symbols derived from the saved symbol.
    if( GetDerivedSymbolNames( libSymbol.GetName(), derivedSymbols ) != 0 )
    {
        // Save the derived symbols.
        for( const wxString& entry : derivedSymbols )
        {
            std::shared_ptr<SYMBOL_BUFFER> symbol = GetBuffer( entry );

            wxCHECK2( symbol, continue );

            LIB_SYMBOL* derivedSymbol = new LIB_SYMBOL( symbol->GetSymbol() );
            derivedSymbol->SetParent( parentSymbol );

            try
            {
                aPlugin->SaveSymbol( aFileName, new LIB_SYMBOL( *derivedSymbol ),
                                     aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( derivedSymbol->GetName() ), aFileName,
                            ioe.What() );
                return false;
            }
        }
    }

    ++m_hash;
    return true;
}


std::shared_ptr<SYMBOL_BUFFER> LIB_BUFFER::GetBuffer( const wxString& aSymbolName ) const
{
    for( std::shared_ptr<SYMBOL_BUFFER> entry : m_symbols )
    {
        if( entry->GetSymbol().GetName() == aSymbolName )
            return entry;
    }

    return std::shared_ptr<SYMBOL_BUFFER>( nullptr );
}


bool LIB_BUFFER::HasDerivedSymbols( const wxString& aParentName ) const
{
    for( const std::shared_ptr<SYMBOL_BUFFER>& entry : m_symbols )
    {
        if( std::shared_ptr<LIB_SYMBOL> parent = entry->GetSymbol().GetParent().lock() )
        {
            if( parent->GetName() == aParentName )
                return true;
        }
    }

    return false;
}


void LIB_BUFFER::GetSymbolNames( wxArrayString& aSymbolNames, SYMBOL_NAME_FILTER aFilter )
{
    for( std::shared_ptr<SYMBOL_BUFFER>& entry : m_symbols )
    {
        const LIB_SYMBOL& symbol = entry->GetSymbol();
        if( ( symbol.IsDerived() && ( aFilter == SYMBOL_NAME_FILTER::ROOT_ONLY ) )
            || ( symbol.IsRoot() && ( aFilter == SYMBOL_NAME_FILTER::DERIVED_ONLY ) ) )
        {
            continue;
        }
        aSymbolNames.Add( UnescapeString( symbol.GetName() ) );
    }
}


size_t LIB_BUFFER::GetDerivedSymbolNames( const wxString& aSymbolName, wxArrayString& aList )
{
    wxCHECK( !aSymbolName.IsEmpty(), 0 );

    // Parent: children map
    std::unordered_map<std::shared_ptr<LIB_SYMBOL>, std::vector<std::shared_ptr<LIB_SYMBOL>>> derivedMap;

    // Iterate the library once to resolve all derived symbol links.
    // This means we only need to iterate the library once, and we can then look up the links
    // as needed.
    for( std::shared_ptr<SYMBOL_BUFFER>& entry : m_symbols )
    {
        std::shared_ptr<LIB_SYMBOL> symbol = entry->GetSymbol().SharedPtr();

        if( std::shared_ptr<LIB_SYMBOL> parent = symbol->GetParent().lock() )
            derivedMap[parent].emplace_back( std::move( symbol ) );
    }

    const auto visit =
            [&]( LIB_SYMBOL& aSymbol )
            {
                aList.Add( aSymbol.GetName() );
            };

    // Assign to std::function to allow recursion
    const std::function<void( std::shared_ptr<LIB_SYMBOL>& )> getDerived =
            [&]( std::shared_ptr<LIB_SYMBOL>& aSymbol )
            {
                auto it = derivedMap.find( aSymbol );

                if( it != derivedMap.end() )
                {
                    for( std::shared_ptr<LIB_SYMBOL>& derivedSymbol : it->second )
                    {
                        visit( *derivedSymbol );

                        // Recurse to get symbols derived from this one
                        getDerived( derivedSymbol );
                    }
                }
            };

    // Start the recursion at the top
    std::shared_ptr<LIB_SYMBOL> symbol = GetSymbol( aSymbolName )->SharedPtr();
    getDerived( symbol );

    return aList.GetCount();
}


int LIB_BUFFER::removeChildSymbols( const SYMBOL_BUFFER& aSymbolBuf )
{
    int                                                  cnt = 0;
    wxArrayString                                        derivedSymbolNames;
    std::deque<std::shared_ptr<SYMBOL_BUFFER>>::iterator it;

    if( GetDerivedSymbolNames( aSymbolBuf.GetSymbol().GetName(), derivedSymbolNames ) )
    {
        for( const wxString& symbolName : derivedSymbolNames )
        {
            it = std::find_if( m_symbols.begin(), m_symbols.end(),
                               [symbolName]( std::shared_ptr<SYMBOL_BUFFER>& buf )
                               {
                                   return buf->GetSymbol().GetName() == symbolName;
                               } );

            wxCHECK2( it != m_symbols.end(), continue );

            m_deleted.emplace_back( *it );
            m_symbols.erase( it );
            cnt += 1;
        }
    }

    return cnt;
}
