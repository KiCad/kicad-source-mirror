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
#include <class_library.h>
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

        dlg.MessageSet( _( "Errors were encountered loading symbols:" ) );

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
        wxLogMessage( _( "Cannot find library \"%s\" in the Symbol Library Table (%s)" ),
                      aLibrary, e.What() );
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

        const auto& partBuffers = libBuf.GetBuffers();

        for( const auto& partBuf : partBuffers )
        {
            if( !libBuf.SaveBuffer( partBuf, aFileName, &*pi, true ) )
            {
                // Something went wrong, but try to save other libraries
                res = false;
            }
        }

        // clear the deleted parts buffer only if data is saved to the original file
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
        for( LIB_SYMBOL* symbol : getOriginalParts( aLibrary ) )
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


bool SYMBOL_LIBRARY_MANAGER::IsPartModified( const wxString& aAlias,
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


bool SYMBOL_LIBRARY_MANAGER::ClearPartModified( const wxString& aAlias,
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
            ret.push_back( symbolBuf->GetPart() );
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


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::GetBufferedPart( const wxString& aAlias,
                                                   const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), nullptr );

    // try the library buffers first
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    LIB_SYMBOL* bufferedPart = libBuf.GetPart( aAlias );

    if( !bufferedPart ) // no buffer symbol found
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
                bufferedParent = libBuf.GetPart( parent->GetName() );

                if( !bufferedParent )
                {
                    bufferedParent = new LIB_SYMBOL( *parent.get() );
                    libBuf.CreateBuffer( bufferedParent, new SCH_SCREEN );
                }
            }

            bufferedPart = new LIB_SYMBOL( *symbol );

            if( bufferedParent )
                bufferedPart->SetParent( bufferedParent );

            libBuf.CreateBuffer( bufferedPart, new SCH_SCREEN );
        }
        catch( const IO_ERROR& e )
        {
            wxLogMessage( _( "Error loading symbol \"%s\" from library \"%s\". (%s)" ),
                          aAlias, aLibrary, e.What() );
            bufferedPart = nullptr;
        }
    }

    return bufferedPart;
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


bool SYMBOL_LIBRARY_MANAGER::UpdatePart( LIB_SYMBOL* aSymbol, const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), false );
    wxCHECK( aSymbol, false );
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto symbolBuf = libBuf.GetBuffer( aSymbol->GetName() );

    if( symbolBuf )     // Existing symbol.
    {
        LIB_SYMBOL* bufferedPart = const_cast< LIB_SYMBOL* >( symbolBuf->GetPart() );

        wxCHECK( bufferedPart, false );

        *bufferedPart = *aSymbol;
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


bool SYMBOL_LIBRARY_MANAGER::UpdatePartAfterRename( LIB_SYMBOL* aSymbol, const wxString& aOldName,
                                                    const wxString& aLibrary )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto symbolBuf = libBuf.GetBuffer( aOldName );

    wxCHECK( symbolBuf, false );

    libBuf.UpdateBuffer( symbolBuf, aSymbol );
    m_frame.SyncLibraries( false );

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::FlushPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return true;

    auto symbolBuf = it->second.GetBuffer( aAlias );
    wxCHECK( symbolBuf, false );

    return it->second.SaveBuffer( symbolBuf, symTable() );
}


LIB_ID SYMBOL_LIBRARY_MANAGER::RevertPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return LIB_ID( aLibrary, aAlias );

    auto symbolBuf = it->second.GetBuffer( aAlias );
    wxCHECK( symbolBuf, LIB_ID( aLibrary, aAlias ) );
    LIB_SYMBOL original( *symbolBuf->GetOriginal() );

    if( original.GetName() != aAlias )
    {
        UpdatePartAfterRename( &original, aAlias, aLibrary );
    }
    else
    {
        symbolBuf->SetPart( new LIB_SYMBOL( original ) );
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

            RevertPart( lib.first, buffer->GetOriginal()->GetName() );
        }
    }

    return retv;
}


bool SYMBOL_LIBRARY_MANAGER::RemovePart( const wxString& aAlias, const wxString& aLibrary )
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
        LIB_SYMBOL* symbol = libIt->second.GetPart( aAlias );

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
        wxLogMessage( _( "Cannot load symbol \"%s\" from library \"%s\" (%s)" ),
                      aAlias, aLibrary, e.What() );
    }

    return alias;
}


bool SYMBOL_LIBRARY_MANAGER::PartExists( const wxString& aAlias, const wxString& aLibrary ) const
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


std::set<LIB_SYMBOL*> SYMBOL_LIBRARY_MANAGER::getOriginalParts( const wxString& aLibrary )
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
        wxLogMessage( _( "Cannot enumerate library \"%s\" (%s)" ), aLibrary, e.What() );
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

    for( auto symbol : getOriginalParts( aLibrary ) )
    {
        LIB_SYMBOL* newSymbol;

        if( symbol->IsAlias() )
        {
            std::shared_ptr< LIB_SYMBOL > oldParent = symbol->GetParent().lock();

            wxCHECK_MSG( oldParent, buf,
                         wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                           symbol->GetName() ) );

            LIB_SYMBOL* libParent = buf.GetPart( oldParent->GetName() );

            if( !libParent )
            {
                libParent = new LIB_SYMBOL( *oldParent.get() );
                buf.CreateBuffer( libParent, new SCH_SCREEN );
            }

            newSymbol = new LIB_SYMBOL( *symbol );
            newSymbol->SetParent( libParent );
            buf.CreateBuffer( newSymbol, new SCH_SCREEN );
        }
        else if( !buf.GetPart( symbol->GetName() ) )
        {
            buf.CreateBuffer( new LIB_SYMBOL( *symbol ), new SCH_SCREEN );
        }
    }

    return buf;
}


SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PART_BUFFER( LIB_SYMBOL* aSymbol,
                                                  std::unique_ptr<SCH_SCREEN> aScreen ) :
        m_screen( std::move( aScreen ) ),
        m_part( aSymbol )
{
    m_original = new LIB_SYMBOL( *aSymbol );
}


SYMBOL_LIBRARY_MANAGER::PART_BUFFER::~PART_BUFFER()
{
    delete m_part;
    delete m_original;
}


void SYMBOL_LIBRARY_MANAGER::PART_BUFFER::SetPart( LIB_SYMBOL* aSymbol )
{
    wxCHECK( m_part != aSymbol, /* void */ );
    wxASSERT( aSymbol );
    delete m_part;
    m_part = aSymbol;

    // If the part moves libraries then the original moves with it
    if( m_original->GetLibId().GetLibNickname() != m_part->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_part->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


void SYMBOL_LIBRARY_MANAGER::PART_BUFFER::SetOriginal( LIB_SYMBOL* aSymbol )
{
    wxCHECK( m_original != aSymbol, /* void */ );
    wxASSERT( aSymbol );
    delete m_original;
    m_original = aSymbol;

    // The original is not allowed to have a different library than its part
    if( m_original->GetLibId().GetLibNickname() != m_part->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_part->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


bool SYMBOL_LIBRARY_MANAGER::PART_BUFFER::IsModified() const
{
    return m_screen && m_screen->IsContentModified();
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetPart( const wxString& aAlias ) const
{
    auto buf = GetBuffer( aAlias );

    if( !buf )
        return nullptr;

    LIB_SYMBOL* symbol = buf->GetPart();

    wxCHECK( symbol, nullptr );

    return  symbol;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::CreateBuffer( LIB_SYMBOL* aCopy, SCH_SCREEN* aScreen )
{
    wxASSERT( aCopy );
    wxASSERT( aCopy->GetLib() == nullptr );
    std::unique_ptr<SCH_SCREEN> screen( aScreen );
    auto symbolBuf = std::make_shared<PART_BUFFER>( aCopy, std::move( screen ) );
    m_parts.push_back( symbolBuf );

    // Set the parent library name,
    // otherwise it is empty as no library has been given as the owner during object construction
    LIB_ID libId = aCopy->GetLibId();
    libId.SetLibNickname( m_libName );
    aCopy->SetLibId( libId );
    ++m_hash;

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::UpdateBuffer(
        SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aSymbolBuf, LIB_SYMBOL* aCopy )
{
    wxCHECK( aCopy && aSymbolBuf, false );

    LIB_SYMBOL* bufferedPart = aSymbolBuf->GetPart();

    wxCHECK( bufferedPart, false );

    *bufferedPart = *aCopy;
    ++m_hash;

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::DeleteBuffer(
        SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aSymbolBuf )
{
    auto partBufIt = std::find( m_parts.begin(), m_parts.end(), aSymbolBuf );
    wxCHECK( partBufIt != m_parts.end(), false );

    bool retv = true;

    // Remove all derived symbols to prevent broken inheritance.
    if( aSymbolBuf->GetPart()->IsRoot() && HasDerivedSymbols( aSymbolBuf->GetPart()->GetName() )
            && removeChildSymbols( aSymbolBuf ) == 0 )
    {
        retv = false;
    }

    m_deleted.emplace_back( *partBufIt );
    m_parts.erase( partBufIt );
    ++m_hash;

    return retv;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::SaveBuffer(
        SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aSymbolBuf, SYMBOL_LIB_TABLE* aLibTable )
{
    wxCHECK( aSymbolBuf, false );
    LIB_SYMBOL* part = aSymbolBuf->GetPart();
    LIB_SYMBOL* originalPart = aSymbolBuf->GetOriginal();
    wxCHECK( part && originalPart, false );
    SYMBOL_LIB_TABLE::SAVE_T result;
    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    // Delete the original symbol if the symbol name has been changed.
    if( part->GetName() != originalPart->GetName() )
    {
        if( aLibTable->LoadSymbol( m_libName, originalPart->GetName() ) )
            aLibTable->DeleteSymbol( m_libName, originalPart->GetName() );
    }

    if( part->IsAlias() )
    {
        LIB_SYMBOL* newCachedPart = new LIB_SYMBOL( *part );
        std::shared_ptr< LIB_SYMBOL > bufferedParent = part->GetParent().lock();

        wxCHECK( bufferedParent, false );

        LIB_SYMBOL* cachedParent = aLibTable->LoadSymbol( m_libName, bufferedParent->GetName() );

        if( !cachedParent )
        {
            cachedParent = new LIB_SYMBOL( *bufferedParent.get() );
            newCachedPart->SetParent( cachedParent );
            result = aLibTable->SaveSymbol( m_libName, cachedParent );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            result = aLibTable->SaveSymbol( m_libName, newCachedPart );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

            LIB_SYMBOL* originalParent = new LIB_SYMBOL( *bufferedParent.get() );
            aSymbolBuf->SetOriginal( originalParent );
            originalPart = new LIB_SYMBOL( *part );
            originalPart->SetParent( originalParent );
            aSymbolBuf->SetOriginal( originalPart );
        }
        else
        {
            newCachedPart->SetParent( cachedParent );
            result = aLibTable->SaveSymbol( m_libName, newCachedPart );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

            SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR originalBufferedParent =
                    GetBuffer( bufferedParent->GetName() );
            wxCHECK( originalBufferedParent, false );
            originalPart = new LIB_SYMBOL( *part );
            originalPart->SetParent( originalBufferedParent->GetPart() );
            aSymbolBuf->SetOriginal( originalPart );
        }
    }
    else
    {
        wxArrayString derivedSymbols;

        if( GetDerivedSymbolNames( part->GetName(), derivedSymbols ) == 0 )
        {
            result = aLibTable->SaveSymbol( m_libName, new LIB_SYMBOL( *part ) );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            aSymbolBuf->SetOriginal( new LIB_SYMBOL( *part ) );
        }
        else
        {
            LIB_SYMBOL* parentSymbol = new LIB_SYMBOL( *part );

            aLibTable->SaveSymbol( m_libName, parentSymbol );

            for( auto entry : derivedSymbols )
            {
                SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR symbol = GetBuffer( entry );
                LIB_SYMBOL* derivedSymbol = new LIB_SYMBOL( *symbol->GetPart() );
                derivedSymbol->SetParent( parentSymbol );
                result = aLibTable->SaveSymbol( m_libName, derivedSymbol );
                wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            }
        }
    }

    ++m_hash;
    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::SaveBuffer(
        SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aSymbolBuf, const wxString& aFileName,
        SCH_PLUGIN* aPlugin, bool aBuffer )
{
    wxCHECK( aSymbolBuf, false );
    LIB_SYMBOL* part = aSymbolBuf->GetPart();
    LIB_SYMBOL* originalPart = aSymbolBuf->GetOriginal();
    wxCHECK( part && originalPart, false );
    wxCHECK( !aFileName.IsEmpty(), false );

    wxString errorMsg = _( "An error \"%s\" occurred saving symbol \"%s\" to library \"%s\"" );

    // set properties to prevent save file on every symbol save
    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    // Delete the original symbol if the symbol name has been changed.
    if( part->GetName() != originalPart->GetName() )
    {
        if( aPlugin->LoadSymbol( aFileName, originalPart->GetName() ) )
            aPlugin->DeleteSymbol( aFileName, originalPart->GetName(), &properties );
    }

    if( part->IsAlias() )
    {
        LIB_SYMBOL* newCachedPart = new LIB_SYMBOL( *part );
        std::shared_ptr< LIB_SYMBOL > bufferedParent = part->GetParent().lock();

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
            newCachedPart->SetParent( cachedParent );

            try
            {
                aPlugin->SaveSymbol( aFileName, cachedParent, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), cachedParent->GetName() );
                return false;
            }

            try
            {
                aPlugin->SaveSymbol( aFileName, newCachedPart, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), newCachedPart->GetName() );
                return false;
            }

            LIB_SYMBOL* originalParent = new LIB_SYMBOL( *bufferedParent.get() );
            aSymbolBuf->SetOriginal( originalParent );
            originalPart = new LIB_SYMBOL( *part );
            originalPart->SetParent( originalParent );
            aSymbolBuf->SetOriginal( originalPart );
        }
        else
        {
            newCachedPart->SetParent( cachedParent );

            try
            {
                aPlugin->SaveSymbol( aFileName, newCachedPart, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), newCachedPart->GetName() );
                return false;
            }

            SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR originalBufferedParent =
                    GetBuffer( bufferedParent->GetName() );
            wxCHECK( originalBufferedParent, false );
            originalPart = new LIB_SYMBOL( *part );
            originalPart->SetParent( originalBufferedParent->GetPart() );
            aSymbolBuf->SetOriginal( originalPart );
        }
    }
    else
    {
        wxArrayString derivedSymbols;

        if( GetDerivedSymbolNames( part->GetName(), derivedSymbols ) == 0 )
        {
            try
            {
                aPlugin->SaveSymbol( aFileName, new LIB_SYMBOL( *part ),
                                     aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), part->GetName() );
                return false;
            }

            aSymbolBuf->SetOriginal( new LIB_SYMBOL( *part ) );
        }
        else
        {
            LIB_SYMBOL* parentSymbol = new LIB_SYMBOL( *part );

            // Save the modified root symbol.
            try
            {
                aPlugin->SaveSymbol( aFileName, parentSymbol, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), part->GetName() );
                return false;
            }

            aSymbolBuf->SetOriginal( new LIB_SYMBOL( *part ) );

            // Save the derived symbols.
            for( auto entry : derivedSymbols )
            {
                SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR symbol = GetBuffer( entry );
                LIB_SYMBOL* derivedSymbol = new LIB_SYMBOL( *symbol->GetPart() );
                derivedSymbol->SetParent( parentSymbol );

                try
                {
                    aPlugin->SaveSymbol( aFileName, new LIB_SYMBOL( *derivedSymbol ),
                                         aBuffer ? &properties : nullptr );
                }
                catch( const IO_ERROR& ioe )
                {
                    wxLogError( errorMsg, ioe.What(), derivedSymbol->GetName() );
                    return false;
                }
            }
        }
    }

    ++m_hash;
    return true;
}


SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR
SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetBuffer( const wxString& aAlias ) const
{
    for( auto entry : m_parts )
    {
        if( entry->GetPart()->GetName() == aAlias )
            return entry;
    }

    return PART_BUFFER::PTR( nullptr );
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::HasDerivedSymbols( const wxString& aParentName ) const
{
    for( auto entry : m_parts )
    {
        if( entry->GetPart()->IsAlias() )
        {
            PART_SPTR parent = entry->GetPart()->GetParent().lock();

            // Check for inherited part without a valid parent.
            wxCHECK( parent, false );

            if( parent->GetName() == aParentName )
                return true;
        }
    }

    return false;
}


void SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetRootSymbolNames( wxArrayString& aRootSymbolNames )
{
    for( auto entry : m_parts )
    {
        if( entry->GetPart()->IsAlias() )
            continue;

        aRootSymbolNames.Add( entry->GetPart()->GetName() );
    }
}


size_t SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetDerivedSymbolNames( const wxString& aSymbolName,
                                                                  wxArrayString& aList )
{
    wxCHECK( !aSymbolName.IsEmpty(), 0 );

    for( auto entry : m_parts )
    {
        if( entry->GetPart()->IsAlias() )
        {
            PART_SPTR parent = entry->GetPart()->GetParent().lock();

            // Check for inherited part without a valid parent.
            wxCHECK( parent, false );

            if( parent->GetName() == aSymbolName )
                aList.Add( entry->GetPart()->GetName() );
        }
    }

    return aList.GetCount();
}


int SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::removeChildSymbols(
        SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aSymbolBuf )
{
    wxCHECK( aSymbolBuf && aSymbolBuf->GetPart()->IsRoot(), 0 );

    int cnt = 0;
    std::deque< SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR >::iterator it = m_parts.begin();

    while( it != m_parts.end() )
    {

        if( (*it)->GetPart()->IsRoot() )
        {
            ++it;
        }
        else
        {
            PART_SPTR parent = (*it)->GetPart()->GetParent().lock();

            wxCHECK2( parent, ++it; continue );

            if( parent->GetName() == aSymbolBuf->GetPart()->GetName() )
            {
                wxCHECK2( parent == aSymbolBuf->GetPart()->SharedPtr(), ++it; continue );

                m_deleted.emplace_back( *it );
                it = m_parts.erase( it );
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
