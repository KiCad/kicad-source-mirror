/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "symbol_import_manager.h"
#include <lib_symbol.h>


SYMBOL_IMPORT_MANAGER::SYMBOL_IMPORT_MANAGER()
{
}


SYMBOL_IMPORT_MANAGER::~SYMBOL_IMPORT_MANAGER()
{
}


void SYMBOL_IMPORT_MANAGER::Clear()
{
    m_symbols.clear();
    m_parentMap.clear();
    m_derivativesMap.clear();
}


void SYMBOL_IMPORT_MANAGER::AddSymbol( const wxString& aName, const wxString& aParentName,
                                       bool aIsPower, LIB_SYMBOL* aSymbol )
{
    SYMBOL_IMPORT_INFO info;
    info.m_name = aName;
    info.m_parentName = aParentName;
    info.m_isPower = aIsPower;
    info.m_symbol.reset( aSymbol );
    info.m_existsInDest = false;
    info.m_checked = false;
    info.m_autoSelected = false;

    m_symbols[aName] = std::move( info );
}


void SYMBOL_IMPORT_MANAGER::CheckExistingSymbols( SYMBOL_EXISTS_FUNC aExistsFunc )
{
    for( auto& [name, info] : m_symbols )
    {
        info.m_existsInDest = aExistsFunc( name );
    }
}


void SYMBOL_IMPORT_MANAGER::BuildDependencyMaps()
{
    m_parentMap.clear();
    m_derivativesMap.clear();

    for( const auto& [name, info] : m_symbols )
    {
        if( !info.m_parentName.IsEmpty() )
        {
            m_parentMap[name] = info.m_parentName;
            m_derivativesMap[info.m_parentName].push_back( name );
        }
    }
}


std::vector<wxString> SYMBOL_IMPORT_MANAGER::GetSymbolNames() const
{
    std::vector<wxString> names;
    names.reserve( m_symbols.size() );

    for( const auto& [name, info] : m_symbols )
        names.push_back( name );

    return names;
}


SYMBOL_IMPORT_INFO* SYMBOL_IMPORT_MANAGER::GetSymbolInfo( const wxString& aName )
{
    auto it = m_symbols.find( aName );
    return ( it != m_symbols.end() ) ? &it->second : nullptr;
}


const SYMBOL_IMPORT_INFO* SYMBOL_IMPORT_MANAGER::GetSymbolInfo( const wxString& aName ) const
{
    auto it = m_symbols.find( aName );
    return ( it != m_symbols.end() ) ? &it->second : nullptr;
}


std::set<wxString> SYMBOL_IMPORT_MANAGER::GetAncestors( const wxString& aSymbolName ) const
{
    std::set<wxString> ancestors;
    wxString current = aSymbolName;

    while( true )
    {
        auto it = m_parentMap.find( current );

        if( it == m_parentMap.end() || it->second.IsEmpty() )
            break;

        wxString parent = it->second;

        if( m_symbols.find( parent ) == m_symbols.end() )
            break;

        ancestors.insert( parent );
        current = parent;
    }

    return ancestors;
}


std::set<wxString> SYMBOL_IMPORT_MANAGER::GetDescendants( const wxString& aSymbolName ) const
{
    std::set<wxString> descendants;
    std::vector<wxString> toProcess = { aSymbolName };

    while( !toProcess.empty() )
    {
        wxString current = toProcess.back();
        toProcess.pop_back();

        auto it = m_derivativesMap.find( current );

        if( it != m_derivativesMap.end() )
        {
            for( const wxString& child : it->second )
            {
                if( descendants.find( child ) == descendants.end() )
                {
                    descendants.insert( child );
                    toProcess.push_back( child );
                }
            }
        }
    }

    return descendants;
}


wxString SYMBOL_IMPORT_MANAGER::GetParent( const wxString& aSymbolName ) const
{
    auto it = m_parentMap.find( aSymbolName );
    return ( it != m_parentMap.end() ) ? it->second : wxString();
}


std::vector<wxString> SYMBOL_IMPORT_MANAGER::GetDirectDerivatives( const wxString& aSymbolName ) const
{
    auto it = m_derivativesMap.find( aSymbolName );

    if( it != m_derivativesMap.end() )
        return it->second;

    return {};
}


bool SYMBOL_IMPORT_MANAGER::IsDerived( const wxString& aSymbolName ) const
{
    auto it = m_parentMap.find( aSymbolName );
    return it != m_parentMap.end() && !it->second.IsEmpty();
}


std::vector<wxString> SYMBOL_IMPORT_MANAGER::SetSymbolSelected( const wxString& aSymbolName,
                                                                bool aSelected )
{
    std::vector<wxString> changedSymbols;

    SYMBOL_IMPORT_INFO* info = GetSymbolInfo( aSymbolName );

    if( !info )
        return changedSymbols;

    if( aSelected )
    {
        info->m_checked = true;
        info->m_autoSelected = false;

        // Auto-select all ancestors
        std::set<wxString> ancestors = GetAncestors( aSymbolName );

        for( const wxString& ancestor : ancestors )
        {
            SYMBOL_IMPORT_INFO* ancestorInfo = GetSymbolInfo( ancestor );

            if( ancestorInfo && !ancestorInfo->m_checked && !ancestorInfo->m_autoSelected )
            {
                ancestorInfo->m_autoSelected = true;
                changedSymbols.push_back( ancestor );
            }
        }
    }
    else
    {
        info->m_checked = false;
        info->m_autoSelected = false;

        recalculateAutoSelections();

        for( const auto& [name, symInfo] : m_symbols )
        {
            if( name != aSymbolName )
                changedSymbols.push_back( name );
        }
    }

    return changedSymbols;
}


std::vector<wxString> SYMBOL_IMPORT_MANAGER::GetSelectedDescendants( const wxString& aSymbolName ) const
{
    std::vector<wxString> selectedDescendants;
    std::set<wxString> descendants = GetDescendants( aSymbolName );

    for( const wxString& desc : descendants )
    {
        const SYMBOL_IMPORT_INFO* info = GetSymbolInfo( desc );

        if( info && info->m_checked )
            selectedDescendants.push_back( desc );
    }

    return selectedDescendants;
}


void SYMBOL_IMPORT_MANAGER::DeselectWithDescendants( const wxString& aSymbolName )
{
    SYMBOL_IMPORT_INFO* info = GetSymbolInfo( aSymbolName );

    if( info )
    {
        info->m_checked = false;
        info->m_autoSelected = false;
    }

    std::set<wxString> descendants = GetDescendants( aSymbolName );

    for( const wxString& desc : descendants )
    {
        SYMBOL_IMPORT_INFO* descInfo = GetSymbolInfo( desc );

        if( descInfo )
        {
            descInfo->m_checked = false;
            descInfo->m_autoSelected = false;
        }
    }

    recalculateAutoSelections();
}


void SYMBOL_IMPORT_MANAGER::SelectAll( std::function<bool( const wxString& )> aFilter )
{
    for( auto& [name, info] : m_symbols )
    {
        if( !aFilter || aFilter( name ) )
        {
            info.m_checked = true;
            info.m_autoSelected = false;
        }
    }

    recalculateAutoSelections();
}


void SYMBOL_IMPORT_MANAGER::DeselectAll( std::function<bool( const wxString& )> aFilter )
{
    for( auto& [name, info] : m_symbols )
    {
        if( !aFilter || aFilter( name ) )
        {
            info.m_checked = false;
            info.m_autoSelected = false;
        }
    }

    recalculateAutoSelections();
}


std::vector<wxString> SYMBOL_IMPORT_MANAGER::GetSymbolsToImport() const
{
    std::vector<wxString> result;

    for( const auto& [name, info] : m_symbols )
    {
        if( info.m_checked || info.m_autoSelected )
            result.push_back( name );
    }

    return result;
}


int SYMBOL_IMPORT_MANAGER::GetManualSelectionCount() const
{
    int count = 0;

    for( const auto& [name, info] : m_symbols )
    {
        if( info.m_checked )
            count++;
    }

    return count;
}


int SYMBOL_IMPORT_MANAGER::GetAutoSelectionCount() const
{
    int count = 0;

    for( const auto& [name, info] : m_symbols )
    {
        if( info.m_autoSelected && !info.m_checked )
            count++;
    }

    return count;
}


std::vector<wxString> SYMBOL_IMPORT_MANAGER::GetConflicts() const
{
    std::vector<wxString> conflicts;

    for( const auto& [name, info] : m_symbols )
    {
        if( ( info.m_checked || info.m_autoSelected ) && info.m_existsInDest )
            conflicts.push_back( name );
    }

    return conflicts;
}


bool SYMBOL_IMPORT_MANAGER::MatchesFilter( const wxString& aSymbolName, const wxString& aFilter )
{
    if( aFilter.IsEmpty() )
        return true;

    return aSymbolName.Lower().Contains( aFilter.Lower() );
}


void SYMBOL_IMPORT_MANAGER::recalculateAutoSelections()
{
    // Clear all auto-selections
    for( auto& [name, info] : m_symbols )
        info.m_autoSelected = false;

    // Re-apply auto-selections for all checked symbols
    for( const auto& [name, info] : m_symbols )
    {
        if( info.m_checked )
        {
            std::set<wxString> ancestors = GetAncestors( name );

            for( const wxString& ancestor : ancestors )
            {
                SYMBOL_IMPORT_INFO* ancestorInfo = GetSymbolInfo( ancestor );

                if( ancestorInfo && !ancestorInfo->m_checked )
                    ancestorInfo->m_autoSelected = true;
            }
        }
    }
}
