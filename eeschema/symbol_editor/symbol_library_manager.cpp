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
#include <symbol_edit_frame.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <kiway.h>
#include <profile.h>
#include <sch_io_mgr.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <symbol_lib_table.h>
#include <list>


SYMBOL_LIBRARY_MANAGER::SYMBOL_LIBRARY_MANAGER( SYMBOL_EDIT_FRAME& aFrame ) :
        m_frame( aFrame ),
        m_syncHash( 0 )
{
    m_adapter = SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( &m_frame, this );
    m_adapter->ShowUnits( false );
}


void SYMBOL_LIBRARY_MANAGER::Sync( const wxString& aForceRefresh,
                                   std::function<void( int, int, const wxString& )> aProgressCallback )
{
    m_logger.Activate();
    {
        getAdapter()->Sync( aForceRefresh, aProgressCallback );
        m_syncHash = symTable()->GetModifyHash();
    }
    m_logger.Deactivate();
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

    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
    {
        // Handle buffered library
        LIB_BUFFER& libBuf = it->second;

        const auto& partBuffers = libBuf.GetBuffers();

        for( const auto& partBuf : partBuffers )
        {
            if( !libBuf.SaveBuffer( partBuf, &*pi, true ) )
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
        PROPERTIES properties;
        properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

        for( LIB_PART* part : getOriginalParts( aLibrary ) )
        {
            LIB_PART* newSymbol;

            if( part->IsAlias() )
            {
                std::shared_ptr< LIB_PART > oldParent = part->GetParent().lock();

                wxCHECK_MSG( oldParent, false,
                             wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                               part->GetName() ) );

                LIB_PART* libParent = pi->LoadSymbol( aLibrary, oldParent->GetName(), &properties );

                if( !libParent )
                {
                    libParent = new LIB_PART( *oldParent.get() );
                    pi->SaveSymbol( aLibrary, libParent, &properties );
                }

                newSymbol = new LIB_PART( *part );
                newSymbol->SetParent( libParent );
                pi->SaveSymbol( aLibrary, newSymbol, &properties );
            }
            else if( !pi->LoadSymbol( aLibrary, part->GetName(), &properties ) )
            {
                pi->SaveSymbol( aLibrary, new LIB_PART( *part ), &properties );
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


bool SYMBOL_LIBRARY_MANAGER::IsPartModified( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    const LIB_BUFFER& buf = libIt->second;
    auto partBuf = buf.GetBuffer( aAlias );
    return partBuf ? partBuf->IsModified() : false;
}


bool SYMBOL_LIBRARY_MANAGER::ClearLibraryModified( const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    for( auto& partBuf : libIt->second.GetBuffers() )
    {
        SCH_SCREEN* screen = partBuf->GetScreen();

        if( screen )
            screen->ClrModify();
    }

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::ClearPartModified( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libI = m_libs.find( aLibrary );

    if( libI == m_libs.end() )
        return false;

    auto partBuf = libI->second.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    partBuf->GetScreen()->ClrModify();
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


std::list<LIB_PART*> SYMBOL_LIBRARY_MANAGER::GetAliases( const wxString& aLibrary ) const
{
    std::list<LIB_PART*> ret;
    wxCHECK( LibraryExists( aLibrary ), ret );

    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        for( auto& partBuf : libIt->second.GetBuffers() )
        {
            ret.push_back( partBuf->GetPart() );
        }
    }
    else
    {
        std::vector<LIB_PART*> aliases;

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


LIB_PART* SYMBOL_LIBRARY_MANAGER::GetBufferedPart( const wxString& aAlias,
                                                   const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), nullptr );

    // try the library buffers first
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    LIB_PART* bufferedPart = libBuf.GetPart( aAlias );

    if( !bufferedPart ) // no buffer part found
    {
        // create a copy of the part
        try
        {
            LIB_PART* part = symTable()->LoadSymbol( aLibrary, aAlias );

            if( part == nullptr )
                THROW_IO_ERROR( _( "Symbol not found." ) );

            LIB_PART* bufferedParent = nullptr;

            // Create parent symbols on demand so parent symbol can be set.
            if( part->IsAlias() )
            {
                std::shared_ptr< LIB_PART > parent = part->GetParent().lock();
                wxCHECK_MSG( parent, nullptr,
                             wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                               part->GetName() ) );

                // Check if the parent symbol buffer has already be created.
                bufferedParent = libBuf.GetPart( parent->GetName() );

                if( !bufferedParent )
                {
                    bufferedParent = new LIB_PART( *parent.get() );
                    libBuf.CreateBuffer( bufferedParent, new SCH_SCREEN );
                }
            }

            bufferedPart = new LIB_PART( *part );

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
    auto partBuf = buf.GetBuffer( aAlias );
    return partBuf ? partBuf->GetScreen() : nullptr;
}


bool SYMBOL_LIBRARY_MANAGER::UpdatePart( LIB_PART* aPart, const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), false );
    wxCHECK( aPart, false );
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( aPart->GetName() );

    if( partBuf )     // Existing symbol.
    {
        LIB_PART* bufferedPart = const_cast< LIB_PART* >( partBuf->GetPart() );

        wxCHECK( bufferedPart, false );

        *bufferedPart = *aPart;
        partBuf->GetScreen()->SetModify();
    }
    else              // New symbol
    {
        LIB_PART* partCopy = new LIB_PART( *aPart, nullptr );

        partCopy->SetLibId( LIB_ID( aLibrary, aPart->GetLibId().GetLibItemName() ) );

        SCH_SCREEN* screen = new SCH_SCREEN;
        libBuf.CreateBuffer( partCopy, screen );
        screen->SetModify();
    }

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::UpdatePartAfterRename( LIB_PART* aPart, const wxString& aOldName,
                                                    const wxString& aLibrary )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( aOldName );

    wxCHECK( partBuf, false );

    libBuf.UpdateBuffer( partBuf, aPart );
    m_frame.SyncLibraries( false );

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::FlushPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return true;

    auto partBuf = it->second.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    return it->second.SaveBuffer( partBuf, symTable() );
}


LIB_ID SYMBOL_LIBRARY_MANAGER::RevertPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return LIB_ID( aLibrary, aAlias );

    auto partBuf = it->second.GetBuffer( aAlias );
    wxCHECK( partBuf, LIB_ID( aLibrary, aAlias ) );
    LIB_PART original( *partBuf->GetOriginal() );

    if( original.GetName() != aAlias )
    {
        UpdatePartAfterRename( &original, aAlias, aLibrary );
    }
    else
    {
        partBuf->SetPart( new LIB_PART( original ) );
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
    auto partBuf = libBuf.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    bool retv = true;

    retv &= libBuf.DeleteBuffer( partBuf );
    m_frame.SyncLibraries( false );

    return retv;
}


LIB_PART* SYMBOL_LIBRARY_MANAGER::GetAlias( const wxString& aAlias,
                                            const wxString& aLibrary ) const
{
    // Try the library buffers first
    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        LIB_PART* part = libIt->second.GetPart( aAlias );

        if( part )
            return part;
    }

    // Get the original part
    LIB_PART* alias = nullptr;

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
    LIB_PART* alias = nullptr;

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


std::set<LIB_PART*> SYMBOL_LIBRARY_MANAGER::getOriginalParts( const wxString& aLibrary )
{
    std::set<LIB_PART*> parts;
    wxCHECK( LibraryExists( aLibrary ), parts );

    try
    {
        wxArrayString aliases;
        symTable()->EnumerateSymbolLib( aLibrary, aliases );

        for( const auto& aliasName : aliases )
        {
            LIB_PART* alias = symTable()->LoadSymbol( aLibrary, aliasName );
            parts.insert( alias );
        }
    }
    catch( const IO_ERROR& e )
    {
        wxLogMessage( _( "Cannot enumerate library \"%s\" (%s)" ), aLibrary, e.What() );
    }

    return parts;
}


SYMBOL_LIBRARY_MANAGER::LIB_BUFFER& SYMBOL_LIBRARY_MANAGER::getLibraryBuffer( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
        return it->second;

    // The requested buffer does not exist yet, so create one
    auto ret = m_libs.emplace( aLibrary, LIB_BUFFER( aLibrary ) );
    LIB_BUFFER& buf = ret.first->second;

    for( auto part : getOriginalParts( aLibrary ) )
    {
        LIB_PART* newSymbol;

        if( part->IsAlias() )
        {
            std::shared_ptr< LIB_PART > oldParent = part->GetParent().lock();

            wxCHECK_MSG( oldParent, buf,
                         wxString::Format( "Derived symbol '%s' found with undefined parent.",
                                           part->GetName() ) );

            LIB_PART* libParent = buf.GetPart( oldParent->GetName() );

            if( !libParent )
            {
                libParent = new LIB_PART( *oldParent.get() );
                buf.CreateBuffer( libParent, new SCH_SCREEN );
            }

            newSymbol = new LIB_PART( *part );
            newSymbol->SetParent( libParent );
            buf.CreateBuffer( newSymbol, new SCH_SCREEN );
        }
        else if( !buf.GetPart( part->GetName() ) )
        {
            buf.CreateBuffer( new LIB_PART( *part ), new SCH_SCREEN );
        }
    }

    return buf;
}


SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PART_BUFFER( LIB_PART* aPart,
                                                  std::unique_ptr<SCH_SCREEN> aScreen ) :
        m_screen( std::move( aScreen ) ),
        m_part( aPart )
{
    m_original = new LIB_PART( *aPart );
}


SYMBOL_LIBRARY_MANAGER::PART_BUFFER::~PART_BUFFER()
{
    delete m_part;
    delete m_original;
}


void SYMBOL_LIBRARY_MANAGER::PART_BUFFER::SetPart( LIB_PART* aPart )
{
    wxCHECK( m_part != aPart, /* void */ );
    wxASSERT( aPart );
    delete m_part;
    m_part = aPart;

    // If the part moves libraries then the original moves with it
    if( m_original->GetLibId().GetLibNickname() != m_part->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_part->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


void SYMBOL_LIBRARY_MANAGER::PART_BUFFER::SetOriginal( LIB_PART* aPart )
{
    wxCHECK( m_original != aPart, /* void */ );
    wxASSERT( aPart );
    delete m_original;
    m_original = aPart;

    // The original is not allowed to have a different library than its part
    if( m_original->GetLibId().GetLibNickname() != m_part->GetLibId().GetLibNickname() )
    {
        m_original->SetLibId( LIB_ID( m_part->GetLibId().GetLibNickname(),
                                      m_original->GetLibId().GetLibItemName() ) );
    }
}


bool SYMBOL_LIBRARY_MANAGER::PART_BUFFER::IsModified() const
{
    return m_screen && m_screen->IsModify();
}


LIB_PART* SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::GetPart( const wxString& aAlias ) const
{
    auto buf = GetBuffer( aAlias );

    if( !buf )
        return nullptr;

    LIB_PART* part = buf->GetPart();

    wxCHECK( part, nullptr );

    return  part;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::CreateBuffer( LIB_PART* aCopy, SCH_SCREEN* aScreen )
{
    wxASSERT( aCopy );
    wxASSERT( aCopy->GetLib() == nullptr );
    std::unique_ptr<SCH_SCREEN> screen( aScreen );
    auto partBuf = std::make_shared<PART_BUFFER>( aCopy, std::move( screen ) );
    m_parts.push_back( partBuf );

    // Set the parent library name,
    // otherwise it is empty as no library has been given as the owner during object construction
    LIB_ID libId = aCopy->GetLibId();
    libId.SetLibNickname( m_libName );
    aCopy->SetLibId( libId );
    ++m_hash;

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::UpdateBuffer( SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aPartBuf,
                                                       LIB_PART* aCopy )
{
    wxCHECK( aCopy && aPartBuf, false );

    LIB_PART* bufferedPart = aPartBuf->GetPart();

    wxCHECK( bufferedPart, false );

    *bufferedPart = *aCopy;
    ++m_hash;

    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::DeleteBuffer( SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aPartBuf )
{
    auto partBufIt = std::find( m_parts.begin(), m_parts.end(), aPartBuf );
    wxCHECK( partBufIt != m_parts.end(), false );

    bool retv = true;

    // Remove all derived symbols to prevent broken inheritance.
    if( aPartBuf->GetPart()->IsRoot() && HasDerivedSymbols( aPartBuf->GetPart()->GetName() )
            && removeChildSymbols( aPartBuf ) == 0 )
    {
        retv = false;
    }

    m_deleted.emplace_back( *partBufIt );
    m_parts.erase( partBufIt );
    ++m_hash;

    return retv;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::SaveBuffer( SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aPartBuf,
                                                     SYMBOL_LIB_TABLE* aLibTable )
{
    wxCHECK( aPartBuf, false );
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );
    SYMBOL_LIB_TABLE::SAVE_T result;

    if( part->IsAlias() )
    {
        LIB_PART* originalPart;
        LIB_PART* newCachedPart = new LIB_PART( *part );
        std::shared_ptr< LIB_PART > bufferedParent = part->GetParent().lock();

        wxCHECK( bufferedParent, false );

        LIB_PART* cachedParent = aLibTable->LoadSymbol( m_libName, bufferedParent->GetName() );

        if( !cachedParent )
        {
            cachedParent = new LIB_PART( *bufferedParent.get() );
            newCachedPart->SetParent( cachedParent );
            result = aLibTable->SaveSymbol( m_libName, cachedParent );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            result = aLibTable->SaveSymbol( m_libName, newCachedPart );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

            LIB_PART* originalParent = new LIB_PART( *bufferedParent.get() );
            aPartBuf->SetOriginal( originalParent );
            originalPart = new LIB_PART( *part );
            originalPart->SetParent( originalParent );
            aPartBuf->SetOriginal( originalPart );
        }
        else
        {
            newCachedPart->SetParent( cachedParent );
            result = aLibTable->SaveSymbol( m_libName, newCachedPart );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

            SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR originalBufferedParent =
                    GetBuffer( bufferedParent->GetName() );
            wxCHECK( originalBufferedParent, false );
            originalPart = new LIB_PART( *part );
            originalPart->SetParent( originalBufferedParent->GetPart() );
            aPartBuf->SetOriginal( originalPart );
        }
    }
    else
    {
        wxArrayString derivedSymbols;

        if( GetDerivedSymbolNames( part->GetName(), derivedSymbols ) == 0 )
        {
            result = aLibTable->SaveSymbol( m_libName, new LIB_PART( *part ) );
            wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            aPartBuf->SetOriginal( new LIB_PART( *part ) );
        }
        else
        {
            LIB_PART* parentSymbol = new LIB_PART( *part );

            aLibTable->SaveSymbol( m_libName, parentSymbol );

            for( auto entry : derivedSymbols )
            {
                SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR symbol = GetBuffer( entry );
                LIB_PART* derivedSymbol = new LIB_PART( *symbol->GetPart() );
                derivedSymbol->SetParent( parentSymbol );
                result = aLibTable->SaveSymbol( m_libName, derivedSymbol );
                wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );
            }
        }
    }

    ++m_hash;
    return true;
}


bool SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::SaveBuffer( SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aPartBuf,
                                                     SCH_PLUGIN* aPlugin, bool aBuffer )
{
    wxCHECK( aPartBuf, false );
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );

    wxString errorMsg = _( "An error \"%s\" occurred saving symbol \"%s\" to library \"%s\"" );

    // set properties to prevent save file on every symbol save
    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    if( part->IsAlias() )
    {
        LIB_PART* originalPart;
        LIB_PART* newCachedPart = new LIB_PART( *part );
        std::shared_ptr< LIB_PART > bufferedParent = part->GetParent().lock();

        wxCHECK( bufferedParent, false );

        LIB_PART* cachedParent = nullptr;

        try
        {
            cachedParent = aPlugin->LoadSymbol( m_libName, bufferedParent->GetName() );
        }
        catch( const IO_ERROR& )
        {
            return false;
        }

        if( !cachedParent )
        {
            cachedParent = new LIB_PART( *bufferedParent.get() );
            newCachedPart->SetParent( cachedParent );

            try
            {
                aPlugin->SaveSymbol( m_libName, cachedParent, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), cachedParent->GetName() );
                return false;
            }

            try
            {
                aPlugin->SaveSymbol( m_libName, newCachedPart, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), newCachedPart->GetName() );
                return false;
            }

            LIB_PART* originalParent = new LIB_PART( *bufferedParent.get() );
            aPartBuf->SetOriginal( originalParent );
            originalPart = new LIB_PART( *part );
            originalPart->SetParent( originalParent );
            aPartBuf->SetOriginal( originalPart );
        }
        else
        {
            newCachedPart->SetParent( cachedParent );

            try
            {
                aPlugin->SaveSymbol( m_libName, newCachedPart, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), newCachedPart->GetName() );
                return false;
            }

            SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR originalBufferedParent =
                    GetBuffer( bufferedParent->GetName() );
            wxCHECK( originalBufferedParent, false );
            originalPart = new LIB_PART( *part );
            originalPart->SetParent( originalBufferedParent->GetPart() );
            aPartBuf->SetOriginal( originalPart );
        }
    }
    else
    {
        wxArrayString derivedSymbols;

        if( GetDerivedSymbolNames( part->GetName(), derivedSymbols ) == 0 )
        {
            try
            {
                aPlugin->SaveSymbol( m_libName, new LIB_PART( *part ),
                                     aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), part->GetName() );
                return false;
            }

            aPartBuf->SetOriginal( new LIB_PART( *part ) );
        }
        else
        {
            LIB_PART* parentSymbol = new LIB_PART( *part );

            // Save the modified root symbol.
            try
            {
                aPlugin->SaveSymbol( m_libName, parentSymbol, aBuffer ? &properties : nullptr );
            }
            catch( const IO_ERROR& ioe )
            {
                wxLogError( errorMsg, ioe.What(), part->GetName() );
                return false;
            }

            aPartBuf->SetOriginal( new LIB_PART( *part ) );

            // Save the derived symbols.
            for( auto entry : derivedSymbols )
            {
                SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR symbol = GetBuffer( entry );
                LIB_PART* derivedSymbol = new LIB_PART( *symbol->GetPart() );
                derivedSymbol->SetParent( parentSymbol );

                try
                {
                    aPlugin->SaveSymbol( m_libName, new LIB_PART( *derivedSymbol ),
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


int SYMBOL_LIBRARY_MANAGER::LIB_BUFFER::removeChildSymbols( SYMBOL_LIBRARY_MANAGER::PART_BUFFER::PTR aPartBuf )
{
    wxCHECK( aPartBuf && aPartBuf->GetPart()->IsRoot(), 0 );

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

            if( parent->GetName() == aPartBuf->GetPart()->GetName() )
            {
                wxCHECK2( parent == aPartBuf->GetPart()->SharedPtr(), ++it; continue );

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
