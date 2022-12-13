/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sch_lib_plugin_cache.h"

#include <lib_symbol.h>
#include <wx_filename.h>


SCH_LIB_PLUGIN_CACHE::SCH_LIB_PLUGIN_CACHE( const wxString& aFullPathAndFileName ) :
    m_modHash( 1 ),
    m_fileName( aFullPathAndFileName ),
    m_libFileName( aFullPathAndFileName ),
    m_isWritable( true ),
    m_isModified( false )
{
    m_libType = SCH_LIB_TYPE::LT_EESCHEMA;
}


SCH_LIB_PLUGIN_CACHE::~SCH_LIB_PLUGIN_CACHE()
{
    // When the cache is destroyed, all of the alias objects on the heap should be deleted.
    for( auto& symbol : m_symbols )
        delete symbol.second;

    m_symbols.clear();
}


void SCH_LIB_PLUGIN_CACHE::Save( const std::optional<bool>& aOpt )
{
    wxCHECK( false, /* void */ );
}


wxFileName SCH_LIB_PLUGIN_CACHE::GetRealFile() const
{
    wxFileName fn( m_libFileName );

    // If m_libFileName is a symlink follow it to the real source file
    WX_FILENAME::ResolvePossibleSymlinks( fn );
    return fn;
}


wxDateTime SCH_LIB_PLUGIN_CACHE::GetLibModificationTime()
{
    wxFileName fn = GetRealFile();

    // update the writable flag while we have a wxFileName, in a network this
    // is possibly quite dynamic anyway.
    m_isWritable = fn.IsFileWritable();

    return fn.GetModificationTime();
}


bool SCH_LIB_PLUGIN_CACHE::IsFile( const wxString& aFullPathAndFileName ) const
{
    return m_fileName == aFullPathAndFileName;
}


bool SCH_LIB_PLUGIN_CACHE::IsFileChanged() const
{
    wxFileName fn = GetRealFile();

    if( m_fileModTime.IsValid() && fn.IsOk() && fn.FileExists() )
        return fn.GetModificationTime() != m_fileModTime;

    return false;
}


LIB_SYMBOL* SCH_LIB_PLUGIN_CACHE::removeSymbol( LIB_SYMBOL* aSymbol )
{
    wxCHECK_MSG( aSymbol != nullptr, nullptr, "NULL pointer cannot be removed from library." );

    LIB_SYMBOL* firstChild = nullptr;
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aSymbol->GetName() );

    if( it == m_symbols.end() )
        return nullptr;

    // If the entry pointer doesn't match the name it is mapped to in the library, we
    // have done something terribly wrong.
    wxCHECK_MSG( *it->second == aSymbol, nullptr,
                 "Pointer mismatch while attempting to remove alias entry <" + aSymbol->GetName() +
                 "> from library cache <" + m_libFileName.GetName() + ">." );

    // If the symbol is a root symbol used by other symbols find the first alias that uses
    // the root symbol and make it the new root.
    if( aSymbol->IsRoot() )
    {
        for( const std::pair<const wxString, LIB_SYMBOL*>& entry : m_symbols )
        {
            if( entry.second->IsAlias()
              && entry.second->GetParent().lock() == aSymbol->SharedPtr() )
            {
                firstChild = entry.second;
                break;
            }
        }

        if( firstChild )
        {
            for( LIB_ITEM& drawItem : aSymbol->GetDrawItems() )
            {
                if( drawItem.Type() == LIB_FIELD_T )
                {
                    LIB_FIELD& field = static_cast<LIB_FIELD&>( drawItem );

                    if( firstChild->FindField( field.GetCanonicalName() ) )
                        continue;
                }

                LIB_ITEM* newItem = (LIB_ITEM*) drawItem.Clone();
                drawItem.SetParent( firstChild );
                firstChild->AddDrawItem( newItem );
            }

            // Reparent the remaining aliases.
            for( const std::pair<const wxString, LIB_SYMBOL*>& entry : m_symbols )
            {
                if( entry.second->IsAlias()
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


void SCH_LIB_PLUGIN_CACHE::AddSymbol( const LIB_SYMBOL* aSymbol )
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


LIB_SYMBOL* SCH_LIB_PLUGIN_CACHE::GetSymbol( const wxString& aName )
{
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aName );

    if( it != m_symbols.end() )
    {
        return it->second;
    }

    return nullptr;
}