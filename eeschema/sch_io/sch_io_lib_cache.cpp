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

#include "sch_io_lib_cache.h"

#include <common.h>
#include <lib_symbol.h>
#include <wx_filename.h>


SCH_IO_LIB_CACHE::SCH_IO_LIB_CACHE( const wxString& aFullPathAndFileName ) :
    m_modHash( 1 ),
    m_fileName( aFullPathAndFileName ),
    m_libFileName( aFullPathAndFileName ),
    m_fileModTime( 0 ),
    m_isWritable( true ),
    m_isModified( false ),
    m_hasParseError( false )
{
    m_libType = SCH_LIB_TYPE::LT_EESCHEMA;
}


SCH_IO_LIB_CACHE::~SCH_IO_LIB_CACHE()
{
    // When the cache is destroyed, all of the alias objects on the heap should be deleted.
    for( auto& symbol : m_symbols )
        delete symbol.second;

    m_symbols.clear();
}


void SCH_IO_LIB_CACHE::Save( const std::optional<bool>& aOpt )
{
    wxCHECK( false, /* void */ );
}


wxFileName SCH_IO_LIB_CACHE::GetRealFile() const
{
    wxFileName fn( m_libFileName );

    // If m_libFileName is a symlink follow it to the real source file
    WX_FILENAME::ResolvePossibleSymlinks( fn );
    return fn;
}


long long SCH_IO_LIB_CACHE::GetLibModificationTime()
{
    wxFileName fn = GetRealFile();
    wxString wildcard = fn.GetFullName();

    // Update the writable flag while we have a wxFileName, in a network this is possibly quite dynamic anyway.
    if( !fn.IsDir() )
    {
        m_isWritable = fn.IsFileWritable();
        return fn.GetModificationTime().GetValue().GetValue();
    }
    else
    {
        m_isWritable = fn.IsDirWritable();
        wildcard = wxS( "*." ) + wxString( FILEEXT::KiCadSymbolLibFileExtension );
        return TimestampDir( fn.GetPath(), wildcard );
    }
}


bool SCH_IO_LIB_CACHE::IsFile( const wxString& aFullPathAndFileName ) const
{
    return m_fileName == aFullPathAndFileName;
}


bool SCH_IO_LIB_CACHE::IsFileChanged() const
{
    wxFileName fn = GetRealFile();

    if( !fn.IsOk() )
        return false;

    if( !fn.IsDir() && fn.IsFileReadable() )
        return fn.GetModificationTime().GetValue().GetValue() != m_fileModTime;

    if( fn.IsDir() && fn.IsDirReadable() )
        return TimestampDir( fn.GetPath(),
                             wxS( "*." ) + wxString( FILEEXT::KiCadSymbolLibFileExtension ) ) != m_fileModTime;

    return false;
}


LIB_SYMBOL* SCH_IO_LIB_CACHE::removeSymbol( LIB_SYMBOL* aSymbol )
{
    wxCHECK_MSG( aSymbol != nullptr, nullptr, "NULL pointer cannot be removed from library." );

    LIB_SYMBOL* firstChild = nullptr;
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aSymbol->GetName() );

    if( it == m_symbols.end() )
        return nullptr;

    // If the entry pointer doesn't match the name it is mapped to in the library, we
    // have done something terribly wrong.
    wxCHECK_MSG( &*it->second == aSymbol, nullptr,
                 "Pointer mismatch while attempting to remove alias entry <" + aSymbol->GetName() +
                 "> from library cache <" + m_libFileName.GetName() + ">." );

    // If the symbol is a root symbol used by other symbols find the first derived symbol that uses
    // the root symbol and make it the new root.
    if( aSymbol->IsRoot() )
    {
        for( const std::pair<const wxString, LIB_SYMBOL*>& entry : m_symbols )
        {
            if( entry.second->IsDerived()
              && entry.second->GetParent().lock() == aSymbol->SharedPtr() )
            {
                firstChild = entry.second;
                break;
            }
        }

        if( firstChild )
        {
            for( SCH_ITEM& drawItem : aSymbol->GetDrawItems() )
            {
                if( drawItem.Type() == SCH_FIELD_T )
                {
                    SCH_FIELD& field = static_cast<SCH_FIELD&>( drawItem );

                    if( firstChild->GetField( field.GetCanonicalName() ) )
                        continue;
                }

                SCH_ITEM* newItem = (SCH_ITEM*) drawItem.Clone();
                drawItem.SetParent( firstChild );
                firstChild->AddDrawItem( newItem );
            }

            // Reparent the remaining derived symbols.
            for( const std::pair<const wxString, LIB_SYMBOL*>& entry : m_symbols )
            {
                if( entry.second->IsDerived()
                      && entry.second->GetParent().lock() == aSymbol->SharedPtr() )
                {
                    entry.second->SetParent( firstChild );
                }
            }
        }
    }

    m_symbols.erase( it );
    delete aSymbol;
    m_isModified = true;
    IncrementModifyHash();
    return firstChild;
}


void SCH_IO_LIB_CACHE::AddSymbol( const LIB_SYMBOL* aSymbol )
{
    // aSymbol is cloned in SYMBOL_LIB::AddSymbol().  The cache takes ownership of aSymbol.
    wxString name = aSymbol->GetName();
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( name );

    if( it != m_symbols.end() )
    {
        removeSymbol( it->second );
    }

    m_symbols[ name ] = const_cast< LIB_SYMBOL* >( aSymbol );
    m_isModified = true;
    IncrementModifyHash();
}


LIB_SYMBOL* SCH_IO_LIB_CACHE::GetSymbol( const wxString& aName )
{
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aName );

    if( it != m_symbols.end() )
    {
        return it->second;
    }

    return nullptr;
}
