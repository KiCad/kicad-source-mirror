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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <libraries/legacy_cache_reconcile.h>

#include <set>

#include <lib_id.h>
#include <libraries/symbol_library_adapter.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <widgets/kistatusbar.h>


static bool loadLibrary( SYMBOL_LIBRARY_ADAPTER& aAdapter, const wxString& aNickname,
                         std::vector<KI_ERROR>& aErrors )
{
    // A library the preload already finished is fully loaded content-wise; forcing another
    // LoadOne() through LoadLibraryEntry() would just re-enumerate it on the UI thread
    if( aAdapter.IsLibraryLoaded( aNickname ) )
        return true;

    std::optional<LIB_STATUS> status = aAdapter.LoadLibraryEntry( aNickname );

    if( status && status->load_status == LOAD_STATUS::LOADED )
        return true;

    KI_ERROR message;

    if( status && status->error )
        message.SetTitle( wxString::Format( _( "Library '%s': %s" ), aNickname, status->error->message ) );
    else
        message.SetTitle( wxString::Format( _( "Library '%s' could not be loaded." ), aNickname ) );

    message.SetSeverity( RPT_SEVERITY_ERROR );
    aErrors.emplace_back( std::move( message ) );

    return false;
}


int ReconcileLegacyCacheSymbols( SYMBOL_LIBRARY_ADAPTER& aAdapter, const wxString& aCacheNickname,
                                 SCH_SCREENS& aScreens, std::vector<KI_ERROR>& aErrors )
{
    // A startup preload worker can still be enumerating a library on another thread; joining it
    // here before touching any LIB_DATA keeps the synchronous loads below from racing it
    aAdapter.AbortAsyncLoad();

    // Loading rather than observing what the preload reached is what keeps the result the same
    // from one open to the next
    if( !loadLibrary( aAdapter, aCacheNickname, aErrors ) )
        return 0;

    std::vector<wxString> cacheSymbols = aAdapter.GetSymbolNames( aCacheNickname );
    std::set<wxString>    cacheSymbolSet( cacheSymbols.begin(), cacheSymbols.end() );

    if( cacheSymbolSet.empty() )
        return 0;

    std::vector<wxString> loadedLibs;

    for( const LIBRARY_TABLE_ROW* row : aAdapter.Rows() )
    {
        const wxString& libName = row->Nickname();

        // A disabled row is never contacted by the loader; forcing it here would reach a
        // backend the user switched off
        if( libName == aCacheNickname || row->Disabled() )
            continue;

        if( loadLibrary( aAdapter, libName, aErrors ) )
            loadedLibs.push_back( libName );
    }

    int repointed = 0;

    for( SCH_SCREEN* screen = aScreens.GetFirst(); screen; screen = aScreens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            LIB_ID      newId = symbol->GetLibId();
            UTF8        fullLibName = newId.Format();

            if( !cacheSymbolSet.count( fullLibName.wx_str() ) )
                continue;

            bool alreadyExists = false;

            for( const wxString& libName : loadedLibs )
            {
                if( aAdapter.LoadSymbol( libName, fullLibName.wx_str() ) )
                {
                    alreadyExists = true;
                    break;
                }
            }

            if( !alreadyExists )
            {
                newId.SetLibNickname( aCacheNickname );
                newId.SetLibItemName( fullLibName );
                symbol->SetLibId( newId );
                ++repointed;
            }
        }
    }

    return repointed;
}
