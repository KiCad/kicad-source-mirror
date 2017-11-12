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

#include <lib_manager.h>
#include <class_libentry.h>
#include <class_library.h>
#include <libeditframe.h>

#include <kiway.h>
#include <profile.h>
#include <symbol_lib_table.h>
#include <sch_legacy_plugin.h>
#include <list>


LIB_MANAGER::LIB_MANAGER( LIB_EDIT_FRAME& aFrame )
    : m_frame( aFrame ), m_symbolTable( aFrame.Prj().SchSymbolLibTable() )
{
    m_adapter = LIB_MANAGER_ADAPTER::Create( this );
    m_adapter->ShowUnits( true );
    Sync();
}


void LIB_MANAGER::Sync( bool aForce )
{
    int libTableHash = m_symbolTable->GetModifyHash();

    if( aForce || m_syncHash != libTableHash )
    {
        getAdapter()->Sync();
        m_syncHash = libTableHash;
    }
}


int LIB_MANAGER::GetHash() const
{
    int hash = m_symbolTable->GetModifyHash();

    for( const auto& libBuf : m_libs )
        hash += libBuf.second.GetHash();

    return hash;
}


int LIB_MANAGER::GetLibraryHash( const wxString& aLibrary ) const
{
    const auto libBufIt = m_libs.find( aLibrary );

    if( libBufIt != m_libs.end() )
        return libBufIt->second.GetHash();

    auto row = m_symbolTable->FindRow( aLibrary );

    // return -1 if library does not exist or 0 if not modified
    return row ? std::hash<std::string>{}( row->GetFullURI( true ).ToStdString() ) : -1;
}


wxArrayString LIB_MANAGER::GetLibraryNames() const
{
    wxArrayString res;

    for( const auto& libName : m_symbolTable->GetLogicalLibs() )
        res.Add( libName );

    return res;
}


bool LIB_MANAGER::FlushAll()
{
    bool result = true;

    for( auto& libBuf : m_libs )
        result &= FlushLibrary( libBuf.first );

    return result;
}


bool LIB_MANAGER::FlushLibrary( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no changes to flush
        return true;

    LIB_BUFFER& libBuf = it->second;
    wxArrayString aliases;
    m_symbolTable->EnumerateSymbolLib( aLibrary, aliases );

    // TODO probably this could be implemented more efficiently
    for( const auto& alias : aliases )
        m_symbolTable->DeleteAlias( aLibrary, alias );

    // Assume all libraries are successfully saved
    bool res = true;

    for( const auto& partBuf : libBuf.GetBuffers() )
    {
        if( !libBuf.SaveBuffer( partBuf, m_symbolTable ) )
        {
            // Something went wrong but try to save other libraries
            res = false;
        }
    }

    if( res )
        libBuf.ClearDeletedBuffer();

    return res;
}


bool LIB_MANAGER::IsLibraryModified( const wxString& aLibrary ) const
{
    wxCHECK( LibraryExists( aLibrary ), false );
    auto it = m_libs.find( aLibrary );
    return it != m_libs.end() ? it->second.IsModified() : false;
}


bool LIB_MANAGER::IsPartModified( const wxString& aAlias, const wxString& aLibrary ) const
{
    wxCHECK( LibraryExists( aLibrary ), false );
    auto libIt = m_libs.find( aLibrary );

    if( libIt == m_libs.end() )
        return false;

    const LIB_BUFFER& buf = libIt->second;
    auto partBuf = buf.GetBuffer( aAlias );
    return partBuf ? partBuf->IsModified() : false;
}


bool LIB_MANAGER::IsLibraryReadOnly( const wxString& aLibrary ) const
{
    wxCHECK( LibraryExists( aLibrary ), true );
    wxFileName fn( m_symbolTable->GetFullURI( aLibrary ) );
    return !fn.IsFileWritable();
}


wxArrayString LIB_MANAGER::GetAliasNames( const wxString& aLibrary ) const
{
    wxArrayString names;
    wxCHECK( LibraryExists( aLibrary ), names );

    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )
    {
        try
        {
            m_symbolTable->EnumerateSymbolLib( aLibrary, names );
        }
        catch( IO_ERROR& e )
        {
            return names;
        }
    }
    else
    {
        names = it->second.GetAliasNames();
    }

    return names;
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
            for( int i = 0; i < partBuf->GetPart()->GetAliasCount(); ++i )
                ret.push_back( partBuf->GetPart()->GetAlias( i ) );
        }
    }
    else
    {
        wxArrayString symbols;

        try
        {
            m_symbolTable->EnumerateSymbolLib( aLibrary, symbols );
        }
        catch( IO_ERROR& e )
        {
            return ret;
        }

        for( const auto& symbol : symbols )
            ret.push_back( m_symbolTable->LoadSymbol( aLibrary, symbol ) );
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
        // create a partCopy
        LIB_ALIAS* alias = m_symbolTable->LoadSymbol( aLibrary, aAlias );
        wxCHECK( alias, nullptr );
        bufferedPart = new LIB_PART( *alias->GetPart(), nullptr );
        libBuf.CreateBuffer( bufferedPart, new SCH_SCREEN( &m_frame.Kiway() ) );
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


bool LIB_MANAGER::UpdatePart( LIB_PART* aPart, const wxString& aLibrary, wxString aOldName )
{
    wxCHECK( LibraryExists( aLibrary ), false );
    wxCHECK( aPart, false );
    const wxString partName = aOldName.IsEmpty() ? aPart->GetName() : aOldName;
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( partName );
    LIB_PART* partCopy = new LIB_PART( *aPart, nullptr );

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

    getAdapter()->UpdateLibrary( aLibrary );

    return true;
}


bool LIB_MANAGER::FlushPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return true;

    auto partBuf = it->second.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    return it->second.SaveBuffer( partBuf, m_symbolTable );
}


bool LIB_MANAGER::RevertPart( const wxString& aAlias, const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // no items to flush
        return true;

    auto partBuf = it->second.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    partBuf->GetScreen()->ClrModify();
    partBuf->SetPart( new LIB_PART( *partBuf->GetOriginal() ) );

    return true;
}


bool LIB_MANAGER::RevertLibrary( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it == m_libs.end() )    // nothing to reverse
        return false;

    m_libs.erase( it );
    getAdapter()->UpdateLibrary( aLibrary );

    return true;
}


bool LIB_MANAGER::RemovePart( const wxString& aAlias, const wxString& aLibrary )
{
    LIB_BUFFER& libBuf = getLibraryBuffer( aLibrary );
    auto partBuf = libBuf.GetBuffer( aAlias );
    wxCHECK( partBuf, false );

    bool res = libBuf.DeleteBuffer( partBuf );
    getAdapter()->UpdateLibrary( aLibrary );

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
    return m_symbolTable->LoadSymbol( aLibrary, aAlias );
}


bool LIB_MANAGER::PartExists( const wxString& aAlias, const wxString& aLibrary ) const
{
    auto libBufIt = m_libs.find( aLibrary );

    if( libBufIt != m_libs.end() )
        return !!libBufIt->second.GetBuffer( aAlias );

    return !!m_symbolTable->LoadSymbol( aLibrary, aAlias );
}


bool LIB_MANAGER::LibraryExists( const wxString& aLibrary ) const
{
    if( aLibrary.IsEmpty() )
        return false;

    if( m_libs.count( aLibrary ) > 0 )
        return true;

    return m_symbolTable->HasLibrary( aLibrary );
}


wxString LIB_MANAGER::ValidateName( const wxString& aName )
{
    wxString name( aName );
    name.Replace( " ", "_" );
    return name;
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


wxString LIB_MANAGER::GetUniqueComponentName( const wxString& aLibrary ) const
{
    wxString name = "New_Component";

    if( !PartExists( name, aLibrary ) )
        return name;

    name += "_";

    for( unsigned int i = 0; i < std::numeric_limits<unsigned int>::max(); ++i )
    {
        if( !PartExists( name + wxString::Format( "%u", i ), aLibrary ) )
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


bool LIB_MANAGER::addLibrary( const wxString& aFilePath, bool aCreate )
{
    wxString libName = getLibraryName( aFilePath );
    wxCHECK( !LibraryExists( libName ), false );  // either create or add an existing one

    // Select the target library table (global/project)
    SYMBOL_LIB_TABLE* libTable = m_frame.SelectSymLibTable();
    wxCHECK( libTable, false );

    SYMBOL_LIB_TABLE_ROW* libRow = new SYMBOL_LIB_TABLE_ROW( libName, aFilePath,
            SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) );
    libTable->InsertRow( libRow );

    if( aCreate )
        libTable->CreateSymbolLib( libName );

    getAdapter()->AddLibrary( libName );

    return true;
}


LIB_MANAGER::LIB_BUFFER& LIB_MANAGER::getLibraryBuffer( const wxString& aLibrary )
{
    auto it = m_libs.find( aLibrary );

    if( it != m_libs.end() )
        return it->second;

    wxArrayString aliases;
    auto ret = m_libs.emplace( aLibrary, LIB_BUFFER( aLibrary ) );
    LIB_BUFFER& buf = ret.first->second;
    m_symbolTable->EnumerateSymbolLib( aLibrary, aliases );
    // set collecting the processed LIB_PARTs
    std::set<LIB_PART*> processed;

    for( const auto& aliasName : aliases )
    {
        LIB_ALIAS* alias = m_symbolTable->LoadSymbol( aLibrary, aliasName );
        LIB_PART* part = alias->GetPart();

        if( !processed.count( part ) )
        {
            buf.CreateBuffer( new LIB_PART( *part, nullptr ), new SCH_SCREEN( &m_frame.Kiway() ) );
            processed.insert( part );
        }
    }

    return buf;
}


LIB_MANAGER::PART_BUFFER::PART_BUFFER( LIB_PART* aPart, SCH_SCREEN* aScreen )
    : m_screen( aScreen ), m_part( aPart )
{
    m_original = new LIB_PART( *aPart );
}


LIB_MANAGER::PART_BUFFER::~PART_BUFFER()
{
    delete m_screen;
    delete m_part;
    delete m_original;
}


void LIB_MANAGER::PART_BUFFER::SetPart( LIB_PART* aPart )
{
    wxCHECK( m_part != aPart, /* void */ );
    wxASSERT( aPart );
    delete m_part;
    m_part = aPart;
}


void LIB_MANAGER::PART_BUFFER::SetOriginal( LIB_PART* aPart )
{
    wxCHECK( m_original != aPart, /* void */ );
    wxASSERT( aPart );
    delete m_original;
    m_original = aPart;
}


bool LIB_MANAGER::PART_BUFFER::IsModified() const
{
    return m_screen && m_screen->IsModify();
}


wxArrayString LIB_MANAGER::LIB_BUFFER::GetAliasNames() const
{
    wxArrayString ret;

    for( const auto& alias : m_aliases )
        ret.push_back( alias.first );

    return ret;
}


bool LIB_MANAGER::LIB_BUFFER::CreateBuffer( LIB_PART* aCopy, SCH_SCREEN* aScreen )
{
    wxASSERT( m_aliases.count( aCopy->GetName() ) == 0 );   // only for new parts
    wxASSERT( aCopy->GetLib() == nullptr );
    auto partBuf = std::make_shared<PART_BUFFER>( aCopy, aScreen );
    m_parts.push_back( partBuf );
    addAliases( partBuf );

    // Set the parent library name,
    // otherwise it is empty as no library has been given as the owner during object construction
    // TODO create a class derived from 
    LIB_ID& libId = (LIB_ID&) aCopy->GetLibId();
    libId.SetLibNickname( m_libName );
    ++m_hash;

    return true;
}


bool LIB_MANAGER::LIB_BUFFER::UpdateBuffer( LIB_MANAGER::PART_BUFFER::PTR aPartBuf, LIB_PART* aCopy )
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

    // TODO Enable buffering to avoid to disable too frequent file saves
    //PROPERTIES properties;
    //properties.emplace( SCH_LEGACY_PLUGIN::PropBuffering, "" );

    if( aLibTable->SaveSymbol( m_libName, new LIB_PART( *part ) ) == SYMBOL_LIB_TABLE::SAVE_OK )
    {
        aPartBuf->GetScreen()->ClrModify();
        aPartBuf->SetOriginal( new LIB_PART( *part ) );
        ++m_hash;
        return true;
    }

    return false;
}


bool LIB_MANAGER::LIB_BUFFER::addAliases( PART_BUFFER::PTR aPartBuf )
{
    LIB_PART* part = aPartBuf->GetPart();
    wxCHECK( part, false );
    bool ret = true;        // Assume everything is ok

    for( int i = 0; i < part->GetAliasCount(); ++i )
    {
        bool newAlias;
        std::tie( std::ignore, newAlias ) = m_aliases.emplace( part->GetAlias( i )->GetName(), aPartBuf );

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

    for( int i = 0; i < part->GetAliasCount(); ++i )
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
