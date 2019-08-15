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

#include <lib_manager.h>
#include <class_libentry.h>
#include <class_library.h>
#include <lib_edit_frame.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <kiway.h>
#include <profile.h>
#include <symbol_lib_table.h>
#include <sch_legacy_plugin.h>
#include <list>


LIB_MANAGER::LIB_MANAGER( LIB_EDIT_FRAME& aFrame ) :
        m_frame( aFrame ),
        m_syncHash( 0 )
{
    m_adapter = SYMBOL_TREE_SYNCHRONIZING_ADAPTER::Create( &m_frame, this );
    m_adapter->ShowUnits( false );
}


void LIB_MANAGER::Sync( bool aForce,
                        std::function<void( int, int, const wxString& )> aProgressCallback )
{
    m_logger.Activate();
    {
        int libTableHash = symTable()->GetModifyHash();

        if( aForce || m_syncHash != libTableHash )
        {
            getAdapter()->Sync( aForce, aProgressCallback );
            m_syncHash = libTableHash;
        }
    }
    m_logger.Deactivate();
}


bool LIB_MANAGER::HasModifications() const
{
    for( const auto& lib : m_libs )
    {
        if( lib.second.IsModified() )
            return true;
    }

    return false;
}


int LIB_MANAGER::GetHash() const
{
    int hash = symTable()->GetModifyHash();

    for( const auto& lib : m_libs )
        hash += lib.second.GetHash();

    return hash;
}


int LIB_MANAGER::GetLibraryHash( const wxString& aLibrary ) const
{
    const auto libBufIt = m_libs.find( aLibrary );

    if( libBufIt != m_libs.end() )
        return libBufIt->second.GetHash();

    auto row = GetLibrary( aLibrary );

    // return -1 if library does not exist or 0 if not modified
    return row ? std::hash<std::string>{}( aLibrary.ToStdString() + row->GetFullURI( true ).ToStdString() ) : -1;
}


wxArrayString LIB_MANAGER::GetLibraryNames() const
{
    wxArrayString res;

    for( const auto& libName : symTable()->GetLogicalLibs() )
        res.Add( libName );

    return res;
}


SYMBOL_LIB_TABLE_ROW* LIB_MANAGER::GetLibrary( const wxString& aLibrary ) const
{
    SYMBOL_LIB_TABLE_ROW* row = nullptr;

    try
    {
        row = symTable()->FindRow( aLibrary );
    }
    catch( const IO_ERROR& e )
    {
        wxLogMessage( _( "Cannot find library \"%s\" in the Symbol Library Table (%s)" ),
                      aLibrary, e.What() );
    }

    return row;
}


bool LIB_MANAGER::SaveLibrary( const wxString& aLibrary, const wxString& aFileName )
{
    wxCHECK( LibraryExists( aLibrary ), false );
    wxFileName fn( aFileName );
    wxCHECK( !fn.FileExists() || fn.IsFileWritable(), false );
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );
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

        for( auto part : getOriginalParts( aLibrary ) )
            pi->SaveSymbol( aLibrary, new LIB_PART( *part ), &properties );
    }

    pi->SaveLibrary( aFileName );
    return res;
}


bool LIB_MANAGER::IsLibraryModified( const wxString& aLibrary ) const
{
    auto it = m_libs.find( aLibrary );
    return it != m_libs.end() ? it->second.IsModified() : false;
}


bool LIB_MANAGER::IsPartModified( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    const LIB_BUFFER& buf = libIt->second;
    auto partBuf = buf.GetBuffer( aAlias );
    return partBuf ? partBuf->IsModified() : false;
}


bool LIB_MANAGER::ClearLibraryModified( const wxString& aLibrary ) const
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


bool LIB_MANAGER::ClearPartModified( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libI = m_libs.find( aLibrary );

    if( libI == m_libs.end() )
        return false;

    auto partBuf = libI->second.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    partBuf->GetScreen()->ClrModify();
    return true;
}


bool LIB_MANAGER::IsLibraryReadOnly( const wxString& aLibrary ) const
{
    wxCHECK( LibraryExists( aLibrary ), true );
    wxFileName fn( symTable()->GetFullURI( aLibrary ) );
    return ( fn.FileExists() && !fn.IsFileWritable() ) || !fn.IsDirWritable();
}


std::list<LIB_ALIAS*> LIB_MANAGER::GetAliases( const wxString& aLibrary ) const
{
    std::list<LIB_ALIAS*> ret;
    wxCHECK( LibraryExists( aLibrary ), ret );

    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        for( auto& partBuf : libIt->second.GetBuffers() )
        {
            for( unsigned int i = 0; i < partBuf->GetPart()->GetAliasCount(); ++i )
                ret.push_back( partBuf->GetPart()->GetAlias( i ) );
        }
    }
    else
    {
        std::vector<LIB_ALIAS*> aliases;

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


LIB_PART* LIB_MANAGER::GetBufferedPart( const wxString& aAlias, const wxString& aLibrary )
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
            LIB_ALIAS* alias = symTable()->LoadSymbol( aLibrary, aAlias );

            if( alias == nullptr )
                THROW_IO_ERROR( _( "Symbol not found." ) );

            bufferedPart = new LIB_PART( *alias->GetPart(), nullptr );
            libBuf.CreateBuffer( bufferedPart, new SCH_SCREEN( &m_frame.Kiway() ) );
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


SCH_SCREEN* LIB_MANAGER::GetScreen( const wxString& aAlias, const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), nullptr );
    wxCHECK( !aAlias.IsEmpty(), nullptr );
    auto it = m_libs.find( aLibrary );
    wxCHECK( it != m_libs.end(), nullptr );

    LIB_BUFFER& buf = it->second;
    auto partBuf = buf.GetBuffer( aAlias );
    return partBuf ? partBuf->GetScreen() : nullptr;
}


bool LIB_MANAGER::UpdatePart( LIB_PART* aPart, const wxString& aLibrary )
{
    wxCHECK( LibraryExists( aLibrary ), false );
    wxCHECK( aPart, false );
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( aPart->GetName() );
    LIB_PART* partCopy = new LIB_PART( *aPart, nullptr );

    partCopy->SetLibId( LIB_ID( aLibrary, aPart->GetLibId().GetLibItemName() ) );

    if( partBuf )
    {
        libBuf.UpdateBuffer( partBuf, partCopy );
    }
    else    // New entry
    {
        SCH_SCREEN* screen = new SCH_SCREEN( &m_frame.Kiway() );
        libBuf.CreateBuffer( partCopy, screen );
        screen->SetModify();
    }

    return true;
}


bool LIB_MANAGER::UpdatePartAfterRename( LIB_PART* aPart, const wxString& oldAlias,
                                         const wxString& aLibrary )
{
    // This is essentially a delete/update.

    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( oldAlias );
    wxCHECK( partBuf, false );

    // Save the original record so it is transferred to the new buffer
    std::unique_ptr<LIB_PART> original( new LIB_PART( *partBuf->GetOriginal() ) );

    // Save the screen object, so it is transferred to the new buffer
    std::unique_ptr<SCH_SCREEN> screen = partBuf->RemoveScreen();

    if( !libBuf.DeleteBuffer( partBuf ) )
        return false;

    if( !UpdatePart( aPart, aLibrary ) )
        return false;

    partBuf = libBuf.GetBuffer( aPart->GetName() );
    partBuf->SetScreen( std::move( screen ) );
    wxCHECK( partBuf, false );
    partBuf->SetOriginal( original.release() ); // part buffer takes ownership of pointer

    SetCurrentPart( aPart->GetName() );
    m_frame.SyncLibraries( false );

    return true;
}


bool LIB_MANAGER::FlushPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return true;

    auto partBuf = it->second.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    return it->second.SaveBuffer( partBuf, symTable() );
}


LIB_ID LIB_MANAGER::RevertPart( const wxString& aAlias, const wxString& aLibrary )
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


bool LIB_MANAGER::RevertLibrary( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // nothing to reverse
        return false;

    m_libs.erase( it );
    m_frame.SyncLibraries( false );

    return true;
}


bool LIB_MANAGER::RevertAll()
{
    bool retv = true;

    // Nothing to revert.
    if( GetHash() == 0 )
        return true;

    for( auto lib : m_libs )
    {
        if( !lib.second.IsModified() )
            continue;

        for( auto buffer : lib.second.GetBuffers() )
        {
            if( !buffer->IsModified() )
                continue;

            RevertPart( lib.first, buffer->GetOriginal()->GetName() );
        }
    }

    return retv;
}


bool LIB_MANAGER::RemovePart( const wxString& aAlias, const wxString& aLibrary )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    bool res = libBuf.DeleteBuffer( partBuf );
    m_frame.SyncLibraries( false );

    return res;
}


LIB_ALIAS* LIB_MANAGER::GetAlias( const wxString& aAlias, const wxString& aLibrary ) const
{
    // Try the library buffers first
    auto libIt = m_libs.find( aLibrary );

    if( libIt != m_libs.end() )
    {
        LIB_PART* part = libIt->second.GetPart( aAlias );

        if( part )
            return part->GetAlias( aAlias );
    }

    // Get the original part
    LIB_ALIAS* alias = nullptr;

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


bool LIB_MANAGER::PartExists( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libBufIt = m_libs.find( aLibrary );
    LIB_ALIAS* alias = nullptr;

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


bool LIB_MANAGER::LibraryExists( const wxString& aLibrary, bool aCheckEnabled ) const
{
    if( aLibrary.IsEmpty() )
        return false;

    if( m_libs.count( aLibrary ) > 0 )
        return true;

    return symTable()->HasLibrary( aLibrary, aCheckEnabled );
}


wxString LIB_MANAGER::GetUniqueLibraryName() const
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


wxString LIB_MANAGER::getLibraryName( const wxString& aFilePath )
{
    wxFileName fn( aFilePath );
    return fn.GetName();
}


bool LIB_MANAGER::addLibrary( const wxString& aFilePath, bool aCreate, SYMBOL_LIB_TABLE* aTable )
{
    wxCHECK( aTable, false );
    wxString libName = getLibraryName( aFilePath );
    wxCHECK( !LibraryExists( libName ), false );  // either create or add an existing one

    // try to use path normalized to an environmental variable or project path
    wxString relPath = NormalizePath( aFilePath, &Pgm().GetLocalEnvVariables(), &m_frame.Prj() );

    if( relPath.IsEmpty() )
        relPath = aFilePath;

    wxString typeName = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY );
    SYMBOL_LIB_TABLE_ROW* libRow = new SYMBOL_LIB_TABLE_ROW( libName, relPath, typeName );
    aTable->InsertRow( libRow );

    if( aCreate )
    {
        try
        {
            aTable->CreateSymbolLib( libName );
        }
        catch( const IO_ERROR& e )
        {
            aTable->RemoveRow( libRow );
            return false;
        }
    }

    m_frame.SyncLibraries( false );

    return true;
}


SYMBOL_LIB_TABLE* LIB_MANAGER::symTable() const
{
    return m_frame.Prj().SchSymbolLibTable();
}


std::set<LIB_PART*> LIB_MANAGER::getOriginalParts( const wxString& aLibrary )
{
    std::set<LIB_PART*> parts;
    wxCHECK( LibraryExists( aLibrary ), parts );

    try
    {
        wxArrayString aliases;
        symTable()->EnumerateSymbolLib( aLibrary, aliases );

        for( const auto& aliasName : aliases )
        {
            LIB_ALIAS* alias = symTable()->LoadSymbol( aLibrary, aliasName );
            parts.insert( alias->GetPart() );
        }
    }
    catch( const IO_ERROR& e )
    {
        wxLogMessage( _( "Cannot enumerate library \"%s\" (%s)" ), aLibrary, e.What() );
    }

    return parts;
}


LIB_MANAGER::LIB_BUFFER& LIB_MANAGER::getLibraryBuffer( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
        return it->second;

    // The requested buffer does not exist yet, so create one
    auto ret = m_libs.emplace( aLibrary, LIB_BUFFER( aLibrary ) );
    LIB_BUFFER& buf = ret.first->second;

    for( auto part : getOriginalParts( aLibrary ) )
        buf.CreateBuffer( new LIB_PART( *part, nullptr ), new SCH_SCREEN( &m_frame.Kiway() ) );

    return buf;
}


LIB_MANAGER::PART_BUFFER::PART_BUFFER( LIB_PART* aPart, std::unique_ptr<SCH_SCREEN> aScreen ) :
        m_screen( std::move( aScreen ) ),
        m_part( aPart )
{
    m_original = new LIB_PART( *aPart );
}


LIB_MANAGER::PART_BUFFER::~PART_BUFFER()
{
    delete m_part;
    delete m_original;
}


void LIB_MANAGER::PART_BUFFER::SetPart( LIB_PART* aPart )
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


void LIB_MANAGER::PART_BUFFER::SetOriginal( LIB_PART* aPart )
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


bool LIB_MANAGER::PART_BUFFER::IsModified() const
{
    return m_screen && m_screen->IsModify();
}


bool LIB_MANAGER::LIB_BUFFER::CreateBuffer( LIB_PART* aCopy, SCH_SCREEN* aScreen )
{
    wxASSERT( m_aliases.count( aCopy->GetName() ) == 0 );   // only for new parts
    wxASSERT( aCopy->GetLib() == nullptr );
    std::unique_ptr<SCH_SCREEN> screen( aScreen );
    auto partBuf = std::make_shared<PART_BUFFER>( aCopy, std::move( screen ) );
    m_parts.push_back( partBuf );
    addAliases( partBuf );

    // Set the parent library name,
    // otherwise it is empty as no library has been given as the owner during object construction
    LIB_ID& libId = (LIB_ID&) aCopy->GetLibId();
    libId.SetLibNickname( m_libName );
    ++m_hash;

    return true;
}


bool LIB_MANAGER::LIB_BUFFER::UpdateBuffer( LIB_MANAGER::PART_BUFFER::PTR aPartBuf,
                                            LIB_PART* aCopy )
{
    bool ret = true;

    ret &= removeAliases( aPartBuf );
    aPartBuf->SetPart( aCopy );
    ret &= addAliases( aPartBuf );
    ++m_hash;

    return ret;
}


bool LIB_MANAGER::LIB_BUFFER::DeleteBuffer( LIB_MANAGER::PART_BUFFER::PTR aPartBuf )
{
    auto partBufIt = std::find( m_parts.begin(), m_parts.end(), aPartBuf );
    wxCHECK( partBufIt != m_parts.end(), false );
    m_deleted.emplace_back( *partBufIt );
    m_parts.erase( partBufIt );
    ++m_hash;

    return removeAliases( aPartBuf );
}


bool LIB_MANAGER::LIB_BUFFER::SaveBuffer( LIB_MANAGER::PART_BUFFER::PTR aPartBuf,
                                          SYMBOL_LIB_TABLE* aLibTable )
{
    wxCHECK( aPartBuf, false );
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );
    SYMBOL_LIB_TABLE::SAVE_T result = aLibTable->SaveSymbol( m_libName, new LIB_PART( *part ) );
    wxCHECK( result == SYMBOL_LIB_TABLE::SAVE_OK, false );

    aPartBuf->SetOriginal( new LIB_PART( *part ) );
    ++m_hash;
    return true;
}


bool LIB_MANAGER::LIB_BUFFER::SaveBuffer( LIB_MANAGER::PART_BUFFER::PTR aPartBuf,
                                          SCH_PLUGIN* aPlugin, bool aBuffer )
{
    wxCHECK( aPartBuf, false );
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );

    // set properties to prevent save file on every symbol save
    PROPERTIES properties;
    properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    // TODO there is no way to check if symbol has been successfully saved
    aPlugin->SaveSymbol( m_libName, new LIB_PART( *part ), aBuffer ? &properties : nullptr );
    aPartBuf->SetOriginal( new LIB_PART( *part ) );
    ++m_hash;
    return true;
}


bool LIB_MANAGER::LIB_BUFFER::addAliases( PART_BUFFER::PTR aPartBuf )
{
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );
    bool ret = true;        // Assume everything is ok

    for( unsigned int i = 0; i < part->GetAliasCount(); ++i )
    {
        bool newAlias;
        std::tie( std::ignore, newAlias ) = m_aliases.emplace( part->GetAlias( i )->GetName(),
                                                               aPartBuf );

        if( !newAlias )     // Overwrite check
        {
            wxFAIL;
            ret = false;
        }
    }

    return ret;
}


bool LIB_MANAGER::LIB_BUFFER::removeAliases( PART_BUFFER::PTR aPartBuf )
{
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );
    bool ret = true;        // Assume everything is ok

    for( unsigned int i = 0; i < part->GetAliasCount(); ++i )
    {
        auto aliasIt = m_aliases.find( part->GetAlias( i )->GetName() );

        if( aliasIt == m_aliases.end() )
        {
            wxFAIL;
            ret = false;
            continue;
        }

        // Be sure the alias belongs to the assigned owner
        wxASSERT( aliasIt->second.lock() == aPartBuf );

        m_aliases.erase( aliasIt );
    }

    return ret;
}
