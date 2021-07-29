/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <symbol_library_manager.h>
#include <symbol_library.h>
#include <dialogs/html_messagebox.h>
#include <symbol_edit_frame.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <kiway.h>
#include <profile.h>
#include <sch_io_mgr.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <symbol_lib_table.h>
#include <symbol_async_loader.h>
#include <widgets/progress_reporter.h>
#include <list>
#include <locale_io.h>
#include <wx/log.h>
#include <string_utils.h>
#include "lib_logger.h"


SYMBOL_LIBRARY_MANAGER::SYMBOL_LIBRARY_MANAGER( SYMBOL_EDIT_FRAME& aFrame ) :
        m_frame( aFrame ),
        m_syncHash( 0 )
{
    m_adapter = SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( &m_frame, this );
    m_adapter->ShowUnits( false );
    m_logger = new LIB_LOGGER();
}


SYMBOL_LIBRARY_MANAGER::~SYMBOL_LIBRARY_MANAGER()
{
    delete m_logger;
}


void SYMBOL_LIBRARY_MANAGER::Sync( const wxString& aForceRefresh,
                                   std::function<void( int, int,
                                                       const wxString& )> aProgressCallback )
{
    m_logger->Activate();
    {
        getAdapter()->Sync( aForceRefresh, aProgressCallback );
        m_syncHash = symTable()->GetModifyHash();
    }
    m_logger->Deactivate();
}


void SYMBOL_LIBRARY_MANAGER::Preload( PROGRESS_REPORTER& aReporter )
{
    const int progressIntervalMillis = 60;

    SYMBOL_ASYNC_LOADER loader( symTable()->GetLogicalLibs(), symTable(), false, nullptr,
                                &aReporter );

    LOCALE_IO toggle;

    loader.Start();

    while( !loader.Done() )
    {
        aReporter.KeepRefreshing();

        wxMilliSleep( progressIntervalMillis );
    }

    if( aReporter.IsCancelled() )
    {
        loader.Abort();
    }
    else
    {
        loader.Join();
    }

    if( !loader.GetErrors().IsEmpty() )
    {
        HTML_MESSAGE_BOX dlg( &m_frame, _( "Load Error" ) );

        dlg.MessageSet( _( "Errors loading symbols:" ) );

        wxString msg = loader.GetErrors();
        msg.Replace( "\n", "<BR>" );

        dlg.AddHTML_Text( msg );
        dlg.ShowModal();
    }
}


bool SYMBOL_LIBRARY_MANAGER::HasModifications() const
{
    for( const auto& lib : m_libs )
    {
        if( lib.second.IsModified() )
            return true;
    }

    return false;
}


int SYMBOL_LIBRARY_MANAGER::GetHash() const
{
    int hash = symTable()->GetModifyHash();

    for( const auto& lib : m_libs )
        hash += lib.second.GetHash();

    return hash;
}


int SYMBOL_LIBRARY_MANAGER::GetLibraryHash( const wxString& aLibrary ) const
{
    const auto libBufIt = m_libs.find( aLibrary );

    if( libBufIt != m_libs.end() )
        return libBufIt->second.GetHash();

    auto row = GetLibrary( aLibrary );

    // return -1 if library does not exist or 0 if not modified
    return row ? std::hash<std::string>{}( aLibrary.ToStdString() +
                                           row->GetFullURI( true ).ToStdString() ) : -1;
}


wxArrayString SYMBOL_LIBRARY_MANAGER::GetLibraryNames() const
{
    wxArrayString res;

    for( const auto& libName : symTable()->GetLogicalLibs() )
        res.Add( libName );

    return res;
}


SYMBOL_LIB_TABLE_ROW* SYMBOL_LIBRARY_MANAGER::GetLibrary( const wxString& aLibrary ) const
{
    SYMBOL_LIB_TABLE_ROW* row = nullptr;

    try
    {
        row = symTable()->FindRow( aLibrary, true );
    }
    catch( const IO_ERROR& e )
    {
        wxLogMessage( _( "Library '%s' not found in the Symbol Library Table." ) + e.What(),
                      aLibrary );
    }

    return row;
}


bool SYMBOL_LIBRARY_MANAGER::SaveLibrary( const wxString& aLibrary, const wxString& aFileName,
                                          SCH_IO_MGR::SCH_FILE_T aFileType )
{
    wxCHECK( aFileType != SCH_IO_MGR::SCH_FILE_T::SCH_LEGACY && LibraryExists( aLibrary ), false );
    wxFileName fn( aFileName );
    wxCHECK( !fn.FileExists() || fn.IsFileWritable(), false );
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( aFileType ) );
    bool res = true;    // assume all libraries are successfully saved

    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
    {
        // Handle buffered library
        LIB_BUFFER& libBuf = it->second;

        const auto& symbolBuffers = libBuf.GetBuffers();

        for( const auto& symbolBuf : symbolBuffers )
        {
            if( !libBuf.SaveBuffer( symbolBuf, aFileName, &*pi, true ) )
            {
                // Something went wrong, but try to save other libraries
                res = false;
            }
        }

        // clear the deleted symbols buffer only if data is saved to the original file
        wxFileName original, destination( aFileName );
        auto row = GetLibrary( aLibrary );

        if( row )
        {
            original = row->GetFullURI( true );
            original.Normalize();
        }

        destination.Normalize();

        if( res && original == destination )
            libBuf.ClearDeletedBuffer();
    }
    else
    {
        // Handle original library
        for( LIB_SYMBOL* symbol : getOriginalSymbols( aLibrary ) )
        {
            LIB_SYMBOL* newSymbol;

            if( symbol->IsAlias() )
            {
                std::shared_ptr< LIB_SYMBOL > oldParent = symbol->GetParent().lock();

                wxCHECK_MSG( oldParent, false,
                             wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                               symbol->GetName() ) );

                LIB_SYMBOL* libParent = pi->LoadSymbol( aLibrary, oldParent->GetName(),
                                                        &properties );

                if( !libParent )
                {
                    libParent = new LIB_SYMBOL( *oldParent.get() );
                    pi->SaveSymbol( aLibrary, libParent, &properties );
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


bool SYMBOL_LIBRARY_MANAGER::IsSymbolModified( const wxString& aAlias,
                                               const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    const LIB_BUFFER& buf = libIt->second;
    auto symbolBuf = buf.GetBuffer( aAlias );
    return symbolBuf ? symbolBuf->IsModified() : false;
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


bool SYMBOL_LIBRARY_MANAGER::ClearSymbolModified( const wxString& aAlias,
                                                  const wxString& aLibrary ) const
{
    auto libI = m_libs.find( aLibrary );

    if( libI == m_libs.end() )
        return false;

    auto symbolBuf = libI->second.GetBuffer( aAlias );
    wxCHECK( symbolBuf, false );

    symbolBuf->GetScreen()->SetContentModified( false );
    return true;
}


bool SYMBOL_LIBRARY_MANAGER::IsLibraryReadOnly( const wxString& aLibrary ) const
{
    wxCHECK( LibraryExists( aLibrary ), true );

    return !symTable()->IsSymbolLibWritable( aLibrary );
}


bool SYMBOL_LIBRARY_MANAGER::IsLibraryLoaded( const wxString& aLibrary ) const
{
    wxCHECK( LibraryExists( aLibrary ), false );

    return symTable()->IsSymbolLibLoaded( aLibrary );
}


std::list<LIB_SYMBOL*> SYMBOL_LIBRARY_MANAGER::GetAliases( const wxString& aLibrary ) const
{
    std::list<LIB_SYMBOL*> ret;
    wxCHECK( LibraryExists( aLibrary ), ret );

    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        for( auto& symbolBuf : libIt->second.GetBuffers() )
        {
            ret.push_back( symbolBuf->GetSymbol() );
        }
    }
    else
    {
        std::vector<LIB_SYMBOL*> aliases;

        try
        {
            symTable()->LoadSymbolLib( aliases, aLibrary );
        }
        catch( const IO_ERROR& e )
        {
            wxLogWarning( e.Problem() );
        }

        std::copy( aliases.begin(), aliases.end(), std::back_inserter( ret ) );
    }

    return ret;
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::GetBufferedSymbol( const wxString& aAlias,
                                                       const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), nullptr );

    // try the library buffers first
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    LIB_SYMBOL* bufferedSymbol = libBuf.GetSymbol( aAlias );

    if( !bufferedSymbol ) // no buffer symbol found
    {
        // create a copy of the symbol
        try
        {
            LIB_SYMBOL* symbol = symTable()->LoadSymbol( aLibrary, aAlias );

            if( symbol == nullptr )
                THROW_IO_ERROR( _( "Symbol not found." ) );

            LIB_SYMBOL* bufferedParent = nullptr;

            // Create parent symbols on demand so parent symbol can be set.
            if( symbol->IsAlias() )
            {
                std::shared_ptr< LIB_SYMBOL > parent = symbol->GetParent().lock();
                wxCHECK_MSG( parent, nullptr,
                             wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                               symbol->GetName() ) );

                // Check if the parent symbol buffer has already be created.
                bufferedParent = libBuf.GetSymbol( parent->GetName() );

                if( !bufferedParent )
                {
                    bufferedParent = new LIB_SYMBOL( *parent.get() );
                    libBuf.CreateBuffer( bufferedParent, new SCH_SCREEN );
                }
            }

            bufferedSymbol = new LIB_SYMBOL( *symbol );

            if( bufferedParent )
                bufferedSymbol->SetParent( bufferedParent );

            libBuf.CreateBuffer( bufferedSymbol, new SCH_SCREEN );
        }
        catch( const IO_ERROR& e )
        {
            wxLogMessage( _( "Error loading symbol %s from library '%s'. (%s)" ),
                          aAlias, aLibrary, e.What() );
            bufferedSymbol = nullptr;
        }
    }

    return bufferedSymbol;
}


SCH_SCREEN* SYMBOL_LIBRARY_MANAGER::GetScreen( const wxString& aAlias, const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), nullptr );
    wxCHECK( !aAlias.IsEmpty(), nullptr );
    auto it = m_libs.find( aLibrary );
    wxCHECK( it != m_libs.end(), nullptr );

    LIB_BUFFER& buf = it->second;
    auto symbolBuf = buf.GetBuffer( aAlias );
    return symbolBuf ? symbolBuf->GetScreen() : nullptr;
}


bool SYMBOL_LIBRARY_MANAGER::UpdateSymbol( LIB_SYMBOL* aSymbol, const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), false );
    wxCHECK( aSymbol, false );
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto symbolBuf = libBuf.GetBuffer( aSymbol->GetName() );

    if( symbolBuf )     // Existing symbol.
    {
        LIB_SYMBOL* bufferedSymbol = const_cast< LIB_SYMBOL* >( symbolBuf->GetSymbol() );

        wxCHECK( bufferedSymbol, false );

        *bufferedSymbol = *aSymbol;
        symbolBuf->GetScreen()->SetContentModified();
    }
    else              // New symbol
    {
        LIB_SYMBOL* symbolCopy = new LIB_SYMBOL( *aSymbol, nullptr );

        symbolCopy->SetLibId( LIB_ID( aLibrary, aSymbol->GetLibId().GetLibItemName() ) );

        SCH_SCREEN* screen = new SCH_SCREEN;
        libBuf.CreateBuffer( symbolCopy, screen );
        screen->SetContentModified();
    }

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::UpdateSymbolAfterRename( LIB_SYMBOL* aSymbol, const wxString& aOldName,
                                                      const wxString& aLibrary )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto symbolBuf = libBuf.GetBuffer( aOldName );

    wxCHECK( symbolBuf, false );

    libBuf.UpdateBuffer( symbolBuf, aSymbol );
    m_frame.SyncLibraries( false );

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::FlushSymbol( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return true;

    auto symbolBuf = it->second.GetBuffer( aAlias );
    wxCHECK( symbolBuf, false );

    return it->second.SaveBuffer( symbolBuf, symTable() );
}


LIB_ID SYMBOL_LIBRARY_MANAGER::RevertSymbol( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return LIB_ID( aLibrary, aAlias );

    auto symbolBuf = it->second.GetBuffer( aAlias );
    wxCHECK( symbolBuf, LIB_ID( aLibrary, aAlias ) );
    LIB_SYMBOL original( *symbolBuf->GetOriginal() );

    if( original.GetName() != aAlias )
    {
        UpdateSymbolAfterRename( &original, aAlias, aLibrary );
    }
    else
    {
        symbolBuf->SetSymbol( new LIB_SYMBOL( original ) );
        m_frame.SyncLibraries( false );
    }

    return LIB_ID( aLibrary, original.GetName() );
}


bool SYMBOL_LIBRARY_MANAGER::RevertLibrary( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // nothing to reverse
        return false;

    m_libs.erase( it );
    m_frame.SyncLibraries( false );

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

        for( const auto& buffer : lib.second.GetBuffers() )
        {
            if( !buffer->IsModified() )
                continue;

            RevertSymbol( lib.first, buffer->GetOriginal()->GetName() );
        }
    }

    return retv;
}


bool SYMBOL_LIBRARY_MANAGER::RemoveSymbol( const wxString& aAlias, const wxString& aLibrary )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto symbolBuf = libBuf.GetBuffer( aAlias );
    wxCHECK( symbolBuf, false );

    bool retv = true;

    retv &= libBuf.DeleteBuffer( symbolBuf );
    m_frame.SyncLibraries( false );

    return retv;
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::GetAlias( const wxString& aAlias,
                                              const wxString& aLibrary ) const
{
    // Try the library buffers first
    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        LIB_SYMBOL* symbol = libIt->second.GetSymbol( aAlias );

        if( symbol )
            return symbol;
    }

    // Get the original symbol
    LIB_SYMBOL* alias = nullptr;

    try
    {
        alias = symTable()->LoadSymbol( aLibrary, aAlias );
    }
    catch( const IO_ERROR& e )
    {
        wxLogMessage( _( "Cannot load symbol '%s' from library '%s'." ) + e.What(),
                      aAlias,
                      aLibrary );
    }

    return alias;
}


bool SYMBOL_LIBRARY_MANAGER::SymbolExists( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libBufIt = m_libs.find( aLibrary );
    LIB_SYMBOL* alias = nullptr;

    if( libBufIt != m_libs.end() )
        return !!libBufIt->second.GetBuffer( aAlias );

    try
    {
        alias = symTable()->LoadSymbol( aLibrary, aAlias );
    }
    catch( IO_ERROR& )
    {
        // checking if certain symbol exists, so its absence is perfectly fine
    }

    return alias != nullptr;
}


bool SYMBOL_LIBRARY_MANAGER::LibraryExists( const wxString& aLibrary, bool aCheckEnabled ) const
{
    if( aLibrary.IsEmpty() )
        return false;

    if( m_libs.count( aLibrary ) > 0 )
        return true;

    return symTable()->HasLibrary( aLibrary, aCheckEnabled );
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


void SYMBOL_LIBRARY_MANAGER::GetRootSymbolNames( const wxString& aLibraryName,
                                                 wxArrayString& aRootSymbolNames )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibraryName );

    libBuf.GetRootSymbolNames( aRootSymbolNames );
}


bool SYMBOL_LIBRARY_MANAGER:: HasDerivedSymbols( const wxString& aSymbolName,
                                                 const wxString& aLibraryName )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibraryName );

    return libBuf.HasDerivedSymbols( aSymbolName );
}


size_t SYMBOL_LIBRARY_MANAGER::GetLibraryCount() const
{
    return symTable()->GetLogicalLibs().size();
}


wxString SYMBOL_LIBRARY_MANAGER::getLibraryName( const wxString& aFilePath )
{
    wxFileName fn( aFilePath );
    return fn.GetName();
}


bool SYMBOL_LIBRARY_MANAGER::addLibrary( const wxString& aFilePath, bool aCreate,
                                         SYMBOL_LIB_TABLE* aTable )
{
    wxCHECK( aTable, false );
    wxString libName = getLibraryName( aFilePath );
    wxCHECK( !LibraryExists( libName ), false );  // either create or add an existing one

    // try to use path normalized to an environmental variable or project path
    wxString relPath = NormalizePath( aFilePath, &Pgm().GetLocalEnvVariables(), &m_frame.Prj() );

    if( relPath.IsEmpty() )
        relPath = aFilePath;

    SCH_IO_MGR::SCH_FILE_T schFileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( aFilePath );
    wxString typeName = SCH_IO_MGR::ShowType( schFileType );
    SYMBOL_LIB_TABLE_ROW* libRow = new SYMBOL_LIB_TABLE_ROW( libName, relPath, typeName );
    aTable->InsertRow( libRow );

    if( aCreate )
    {
        wxCHECK( schFileType != SCH_IO_MGR::SCH_FILE_T::SCH_LEGACY, false );

        try
        {
            aTable->CreateSymbolLib( libName );
        }
        catch( const IO_ERROR& )
        {
            aTable->RemoveRow( libRow );
            return false;
        }
    }

    m_frame.SyncLibraries( false );

    return true;
}


SYMBOL_LIB_TABLE* SYMBOL_LIBRARY_MANAGER::symTable() const
{
    return m_frame.Prj().SchSymbolLibTable();
}


std::set<LIB_SYMBOL*> SYMBOL_LIBRARY_MANAGER::getOriginalSymbols( const wxString& aLibrary )
{
    std::set<LIB_SYMBOL*> symbols;
    wxCHECK( LibraryExists( aLibrary ), symbols );

    try
    {
        wxArrayString aliases;
        symTable()->EnumerateSymbolLib( aLibrary, aliases );

        for( const auto& aliasName : aliases )
        {
            LIB_SYMBOL* alias = symTable()->LoadSymbol( aLibrary, aliasName );
            symbols.insert( alias );
        }
    }
    catch( const IO_ERROR& e )
    {
        wxLogMessage( _( "Cannot enumerate library '%s'." ) + e.What(), aLibrary );
    }

    return symbols;
}


SYMBOL_LIBRARY_MANAGER::LIB_BUFFER& SYMBOL_LIBRARY_MANAGER::getLibraryBuffer(
        const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
        return it->second;

    // The requested buffer does not exist yet, so create one
    auto ret = m_libs.emplace( aLibrary, LIB_BUFFER( aLibrary ) );
    LIB_BUFFER& buf = ret.first->second;

    for( auto symbol : getOriginalSymbols( aLibrary ) )
    {
        LIB_SYMBOL* newSymbol;

        if( symbol->IsAlias() )
        {
            std::shared_ptr< LIB_SYMBOL > oldParent = symbol->GetParent().lock();

            wxCHECK_MSG( oldParent, buf,
                         wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                           symbol->GetName() ) );

            LIB_SYMBOL* libParent = buf.GetSymbol( oldParent->GetName() );

            if( !libParent )
            {
                libParent = new LIB_SYMBOL( *oldParent.get() );
                buf.CreateBuffer( libParent, new SCH_SCREEN );
            }

            newSymbol = new LIB_SYMBOL( *symbol );
            newSymbol->SetParent( libParent );
            buf.CreateBuffer( newSymbol, new SCH_SCREEN );
        }
        else if( !buf.GetSymbol( symbol->GetName() ) )
        {
            buf.CreateBuffer( new LIB_SYMBOL( *symbol ), new SCH_SCREEN );
        }
    }

    return buf;
}


SYMBOL_LIBRARY_MANAGER::SYMBOL_BUFFER::SYMBOL_BUFFER( LIB_SYMBOL* aSymbol,
                                                      std::unique_ptr<SCH_SCREEN> aScreen ) :
        m_screen( std::move( aScreen ) ),
        m_symbol( aSymbol )
{
    m_original = new LIB_SYMBOL( *aSymbol );
}


SYMBOL_LIBRARY_MANAGER::SYMBOL_BUFFER::~SYMBOL_BUFFER()
{
    delete m_symbol;
    delete m_original;
}


void SYMBOL_LIBRARY_MANAGER::SYMBOL_BUFFER::SetSymbol( LIB_SYMBOL* aSymbol )
{
    wxCHECK( m_symbol != aSymbol, /* void */ );
    wxASSERT( aSymbol );
    delete m_symbol;
    m_symbol = aSymbol;

    // If the symbol moves libraries then the original moves with it
    if( m_original->GetLibId().GetLibNickname() != m_symbol->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_symbol->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


void SYMBOL_LIBRARY_MANAGER::SYMBOL_BUFFER::SetOriginal( LIB_SYMBOL* aSymbol )
{
    wxCHECK( m_original != aSymbol, /* void */ );
    wxASSERT( aSymbol );
    delete m_original;
    m_original = aSymbol;

    // The original is not allowed to have a different library than its symbol
    if( m_original->GetLibId().GetLibNickname() != m_symbol->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_symbol->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


bool SYMBOL_LIBRARY_MANAGER::SYMBOL_BUFFER::IsModified() const
{
    return m_screen && m_screen->IsContentModified();
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetSymbol( const wxString& aAlias ) const
{
    auto buf = GetBuffer( aAlias );

    if( !buf )
        return nullptr;

    LIB_SYMBOL* symbol = buf->GetSymbol();

    wxCHECK( symbol, nullptr );

    return  symbol;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::CreateBuffer( LIB_SYMBOL* aCopy, SCH_SCREEN* aScreen )
{
    wxASSERT( aCopy );
    wxASSERT( aCopy->GetLib() == nullptr );
    std::unique_ptr<SCH_SCREEN> screen( aScreen );
    auto symbolBuf = std::make_shared<SYMBOL_BUFFER>( aCopy, std::move( screen ) );
    m_symbols.push_back( symbolBuf );

    // Set the parent library name,
    // otherwise it is empty as no library has been given as the owner during object construction
    LIB_ID libId = aCopy->GetLibId();
    libId.SetLibNickname( m_libName );
    aCopy->SetLibId( libId );
    ++m_hash;

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::UpdateBuffer( std::shared_ptr<SYMBOL_BUFFER> aSymbolBuf,
                                                       LIB_SYMBOL* aCopy )
{
    wxCHECK( aCopy && aSymbolBuf, false );

    LIB_SYMBOL* bufferedSymbol = aSymbolBuf->GetSymbol();

    wxCHECK( bufferedSymbol, false );

    *bufferedSymbol = *aCopy;
    ++m_hash;

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::DeleteBuffer( std::shared_ptr<SYMBOL_BUFFER> aSymbolBuf )
{
    auto symbolBufIt = std::find( m_symbols.begin(), m_symbols.end(), aSymbolBuf );
    wxCHECK( symbolBufIt != m_symbols.end(), false );

    bool retv = true;

    // Remove all derived symbols to prevent broken inheritance.
    if( aSymbolBuf->GetSymbol()->IsRoot() && HasDerivedSymbols( aSymbolBuf->GetSymbol()->GetName() )
      && removeChildSymbols( aSymbolBuf ) == 0 )
    {
        retv = false;
    }

    m_deleted.emplace_back( *symbolBufIt );
    m_symbols.erase( symbolBufIt );
    ++m_hash;

    return retv;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::SaveBuffer( std::shared_ptr<SYMBOL_BUFFER> aSymbolBuf,
                                                     SYMBOL_LIB_TABLE* aLibTable )
{
    wxCHECK( aSymbolBuf, false );
    LIB_SYMBOL* libSymbol = aSymbolBuf->GetSymbol();
    LIB_SYMBOL* originalSymbol = aSymbolBuf->GetOriginal();
    wxCHECK( libSymbol && originalSymbol, false );
    SYMBOL_LIB_TABLE::SAVE_T result;
    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    // Delete the original symbol if the symbol name has been changed.
    if( libSymbol->GetName() != originalSymbol->GetName() )
    {
        if( aLibTable->LoadSymbol( m_libName, originalSymbol->GetName() ) )
            aLibTable->DeleteSymbol( m_libName, originalSymbol->GetName() );
    }

    if( libSymbol->IsAlias() )
    {
        LIB_SYMBOL* newCachedSymbol = new LIB_SYMBOL( *libSymbol );
        std::shared_ptr< LIB_SYMBOL > bufferedParent = libSymbol->GetParent().lock();

        wxCHECK( bufferedParent, false );

        LIB_SYMBOL* cachedParent = aLibTable->LoadSymbol( m_libName, bufferedParent->GetName() );

        if( !cachedParent )
        {
            cachedParent = new LIB_SYMBOL( *bufferedParent.get() );
            newCachedSymbol->SetParent( cachedParent );
            result = aLibTable->SaveSymbol( m_libName, cachedParent );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            result = aLibTable->SaveSymbol( m_libName, newCachedSymbol );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

            LIB_SYMBOL* originalParent = new LIB_SYMBOL( *bufferedParent.get() );
            aSymbolBuf->SetOriginal( originalParent );
            originalSymbol = new LIB_SYMBOL( *libSymbol );
            originalSymbol->SetParent( originalParent );
            aSymbolBuf->SetOriginal( originalSymbol );
        }
        else
        {
            newCachedSymbol->SetParent( cachedParent );
            result = aLibTable->SaveSymbol( m_libName, newCachedSymbol );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

            auto originalBufferedParent = GetBuffer( bufferedParent->GetName() );
            wxCHECK( originalBufferedParent, false );
            originalSymbol = new LIB_SYMBOL( *libSymbol );
            originalSymbol->SetParent( originalBufferedParent->GetSymbol() );
            aSymbolBuf->SetOriginal( originalSymbol );
        }
    }
    else
    {
        wxArrayString derivedSymbols;

        if( GetDerivedSymbolNames( libSymbol->GetName(), derivedSymbols ) == 0 )
        {
            result = aLibTable->SaveSymbol( m_libName, new LIB_SYMBOL( *libSymbol ) );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            aSymbolBuf->SetOriginal( new LIB_SYMBOL( *libSymbol ) );
        }
        else
        {
            LIB_SYMBOL* parentSymbol = new LIB_SYMBOL( *libSymbol );

            aLibTable->SaveSymbol( m_libName, parentSymbol );

            for( auto entry : derivedSymbols )
            {
                std::shared_ptr<SYMBOL_BUFFER> symbol = GetBuffer( entry );
                LIB_SYMBOL* derivedSymbol = new LIB_SYMBOL( *symbol->GetSymbol() );
                derivedSymbol->SetParent( parentSymbol );
                result = aLibTable->SaveSymbol( m_libName, derivedSymbol );
                wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            }
        }
    }

    ++m_hash;
    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::SaveBuffer( std::shared_ptr<SYMBOL_BUFFER> aSymbolBuf,
                                                     const wxString& aFileName,
        SCH_PLUGIN* aPlugin, bool aBuffer )
{
    wxCHECK( aSymbolBuf, false );
    LIB_SYMBOL* libSymbol = aSymbolBuf->GetSymbol();
    LIB_SYMBOL* originalSymbol = aSymbolBuf->GetOriginal();
    wxCHECK( libSymbol && originalSymbol, false );
    wxCHECK( !aFileName.IsEmpty(), false );

    wxString errorMsg = _( "Error saving symbol %s to library '%s'." ) + wxS( "\n%s" );

    // set properties to prevent save file on every symbol save
    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    // Delete the original symbol if the symbol name has been changed.
    if( libSymbol->GetName() != originalSymbol->GetName() )
    {
        if( aPlugin->LoadSymbol( aFileName, originalSymbol->GetName() ) )
            aPlugin->DeleteSymbol( aFileName, originalSymbol->GetName(), &properties );
    }

    if( libSymbol->IsAlias() )
    {
        LIB_SYMBOL* newCachedSymbol = new LIB_SYMBOL( *libSymbol );
        std::shared_ptr< LIB_SYMBOL > bufferedParent = libSymbol->GetParent().lock();

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

            LIB_SYMBOL* originalParent = new LIB_SYMBOL( *bufferedParent.get() );
            aSymbolBuf->SetOriginal( originalParent );
            originalSymbol = new LIB_SYMBOL( *libSymbol );
            originalSymbol->SetParent( originalParent );
            aSymbolBuf->SetOriginal( originalSymbol );
        }
        else
        {
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
            originalSymbol = new LIB_SYMBOL( *libSymbol );
            originalSymbol->SetParent( originalBufferedParent->GetSymbol() );
            aSymbolBuf->SetOriginal( originalSymbol );
        }
    }
    else
    {
        wxArrayString derivedSymbols;

        if( GetDerivedSymbolNames( libSymbol->GetName(), derivedSymbols ) == 0 )
        {
            try
            {
                aPlugin->SaveSymbol( aFileName, new LIB_SYMBOL( *libSymbol ),
                                     aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( libSymbol->GetName() ), aFileName,
                            ioe.What() );
                return false;
            }

            aSymbolBuf->SetOriginal( new LIB_SYMBOL( *libSymbol ) );
        }
        else
        {
            LIB_SYMBOL* parentSymbol = new LIB_SYMBOL( *libSymbol );

            // Save the modified root symbol.
            try
            {
                aPlugin->SaveSymbol( aFileName, parentSymbol, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, UnescapeString( libSymbol->GetName() ), aFileName,
                            ioe.What() );
                return false;
            }

            aSymbolBuf->SetOriginal( new LIB_SYMBOL( *libSymbol ) );

            // Save the derived symbols.
            for( auto entry : derivedSymbols )
            {
                std::shared_ptr<SYMBOL_BUFFER> symbol = GetBuffer( entry );
                LIB_SYMBOL* derivedSymbol = new LIB_SYMBOL( *symbol->GetSymbol() );
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
    }

    ++m_hash;
    return true;
}


std::shared_ptr<SYMBOL_LIBRARY_MANAGER::SYMBOL_BUFFER>
SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetBuffer( const wxString& aAlias ) const
{
    for( auto entry : m_symbols )
    {
        if( entry->GetSymbol()->GetName() == aAlias )
            return entry;
    }

    return std::shared_ptr<SYMBOL_BUFFER>( nullptr );
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::HasDerivedSymbols( const wxString& aParentName ) const
{
    for( auto entry : m_symbols )
    {
        if( entry->GetSymbol()->IsAlias() )
        {
            LIB_SYMBOL_SPTR parent = entry->GetSymbol()->GetParent().lock();

            // Check for inherited symbol without a valid parent.
            wxCHECK( parent, false );

            if( parent->GetName() == aParentName )
                return true;
        }
    }

    return false;
}


void SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetRootSymbolNames( wxArrayString& aRootSymbolNames )
{
    for( auto entry : m_symbols )
    {
        if( entry->GetSymbol()->IsAlias() )
            continue;

        aRootSymbolNames.Add( UnescapeString( entry->GetSymbol()->GetName() ) );
    }
}


size_t SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetDerivedSymbolNames( const wxString& aSymbolName,
                                                                  wxArrayString& aList )
{
    wxCHECK( !aSymbolName.IsEmpty(), 0 );

    for( auto entry : m_symbols )
    {
        if( entry->GetSymbol()->IsAlias() )
        {
            LIB_SYMBOL_SPTR parent = entry->GetSymbol()->GetParent().lock();

            // Check for inherited symbol without a valid parent.
            wxCHECK( parent, false );

            if( parent->GetName() == aSymbolName )
                aList.Add( entry->GetSymbol()->GetName() );
        }
    }

    return aList.GetCount();
}


int SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::removeChildSymbols( std::shared_ptr<SYMBOL_BUFFER> aSymbolBuf )
{
    wxCHECK( aSymbolBuf && aSymbolBuf->GetSymbol()->IsRoot(), 0 );

    int cnt = 0;
    std::deque< std::shared_ptr<SYMBOL_BUFFER> >::iterator it = m_symbols.begin();

    while( it != m_symbols.end() )
    {

        if( (*it)->GetSymbol()->IsRoot() )
        {
            ++it;
        }
        else
        {
            LIB_SYMBOL_SPTR parent = (*it)->GetSymbol()->GetParent().lock();

            wxCHECK2( parent, ++it; continue );

            if( parent->GetName() == aSymbolBuf->GetSymbol()->GetName() )
            {
                wxCHECK2( parent == aSymbolBuf->GetSymbol()->SharedPtr(), ++it; continue );

                m_deleted.emplace_back( *it );
                it = m_symbols.erase( it );
                cnt++;
            }
            else
            {
                ++it;
            }
        }
    }

    return cnt;
}
