/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "footprint_import_reconciler.h"

#include <map>
#include <set>

#include <wx/dir.h>
#include <wx/filename.h>

#include <board.h>
#include <footprint.h>
#include <lib_id.h>
#include <pad.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <io/io_mgr.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <footprint_library_adapter.h>
#include <libraries/library_table.h>


const wxString& FOOTPRINT_IMPORT_RECONCILER::ManagedCacheOption()
{
    static const wxString option = wxS( "kicad_import_cache=1" );
    return option;
}


FOOTPRINT_IMPORT_RECONCILER::FOOTPRINT_IMPORT_RECONCILER( FOOTPRINT_LIBRARY_ADAPTER& aAdapter,
                                                         const wxString& aProjectPath,
                                                         REPORTER*       aReporter ) :
        m_adapter( aAdapter ),
        m_projectPath( aProjectPath ),
        m_reporter( aReporter )
{
}


namespace
{
void report( REPORTER* aReporter, const wxString& aMsg, SEVERITY aSeverity )
{
    if( aReporter )
        aReporter->Report( aMsg, aSeverity );
}


// structural signature, flags same-name placed instances that differ
wxString placedSignature( const FOOTPRINT* aFp )
{
    BOX2I bbox = aFp->GetBoundingBox( false );

    return wxString::Format( wxS( "%zu:%zu:%d:%d" ), aFp->Pads().size(),
                             aFp->GraphicalItems().size(), bbox.GetWidth(), bbox.GetHeight() );
}


// reuse existing row/dir only if prior import-managed cache
bool existingIsManagedCache( FOOTPRINT_LIBRARY_ADAPTER& aAdapter, const wxString& aNickname )
{
    std::optional<LIBRARY_TABLE_ROW*> row = aAdapter.GetRow( aNickname );

    if( !row || !*row )
        return false;

    return ( *row )->GetOptionsMap().count( "kicad_import_cache" ) > 0;
}
}


FOOTPRINT_IMPORT_RECONCILE_RESULT
FOOTPRINT_IMPORT_RECONCILER::Reconcile( BOARD* aBoard,
                                        std::vector<std::unique_ptr<FOOTPRINT>> aDefinitions,
                                        const wxString& aCacheNickname,
                                        const std::vector<wxString>& aSourceLibNicknames )
{
    FOOTPRINT_IMPORT_RECONCILE_RESULT result;

    if( !aBoard )
        return result;

    // index importer defs by FPID item name
    std::map<wxString, FOOTPRINT*> defByName;

    for( const std::unique_ptr<FOOTPRINT>& def : aDefinitions )
    {
        wxString name = def->GetFPID().GetUniStringLibItemName();

        if( !name.IsEmpty() )
            defByName.emplace( name, def.get() );
    }

    // preload source libs before membership queries
    for( const wxString& nick : aSourceLibNicknames )
    {
        if( m_adapter.GetRow( nick ) )
            m_adapter.LoadOne( nick );
    }

    // resolve one source lib, empty if none or ambiguous
    auto resolveSource = [&]( const FOOTPRINT* aFp, const wxString& aName ) -> wxString
    {
        std::vector<wxString> candidates;
        wxString              ownNick = aFp->GetFPID().GetUniStringLibNickname();

        if( !ownNick.IsEmpty() )
            candidates.push_back( ownNick );

        for( const wxString& nick : aSourceLibNicknames )
        {
            if( nick != ownNick )
                candidates.push_back( nick );
        }

        std::vector<wxString> matches;

        for( const wxString& nick : candidates )
        {
            if( m_adapter.GetRow( nick ) && m_adapter.FootprintExists( nick, aName ) )
                matches.push_back( nick );
        }

        return matches.size() == 1 ? matches.front() : wxString( wxEmptyString );
    };

    // per-instance target keyed by nick+name, so same-name parts from different libs stay split
    // empty target = cache-bound
    std::map<wxString, wxString>                targetByKey;
    std::set<wxString>                          cacheNames;
    std::map<wxString, std::vector<FOOTPRINT*>> instancesByName;

    auto keyOf = []( const wxString& aNick, const wxString& aName )
    {
        return aNick + wxS( "\x1f" ) + aName;
    };

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        instancesByName[name].push_back( fp );

        wxString key = keyOf( fp->GetFPID().GetUniStringLibNickname(), name );

        if( targetByKey.count( key ) )
            continue;

        wxString sourceNick = resolveSource( fp, name );
        targetByKey[key] = sourceNick;

        if( sourceNick.IsEmpty() )
            cacheNames.insert( name );
    }

    // canonical def per cache name, fall back to unique placed instance if importer gave none
    std::map<wxString, FOOTPRINT*>                 cacheDefs;
    std::map<wxString, std::unique_ptr<FOOTPRINT>> placedDefs;

    for( const wxString& name : cacheNames )
    {
        if( auto it = defByName.find( name ); it != defByName.end() )
        {
            cacheDefs[name] = it->second;
            continue;
        }

        const std::vector<FOOTPRINT*>& instances = instancesByName[name];

        if( instances.empty() )
            continue;

        wxString firstSig = placedSignature( instances.front() );

        for( auto it = instances.begin() + 1; it != instances.end(); ++it )
        {
            if( placedSignature( *it ) != firstSig )
            {
                report( m_reporter,
                        wxString::Format( _( "Imported footprint '%s' has conflicting placed "
                                             "definitions; keeping the first." ), name ),
                        RPT_SEVERITY_WARNING );
            }
        }

        placedDefs[name] =
                std::unique_ptr<FOOTPRINT>( static_cast<FOOTPRINT*>( instances.front()->Clone() ) );
        cacheDefs[name] = placedDefs[name].get();
    }

    // write residuals to an atomic .pretty and register the row
    if( !cacheDefs.empty() )
        writeAndRegisterCache( aCacheNickname, cacheDefs, result );

    // re-point board FPID nicks to resolved lib, keep item name
    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        LIB_ID   fpid = fp->GetFPID();
        wxString name = fpid.GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        auto it = targetByKey.find( keyOf( fpid.GetUniStringLibNickname(), name ) );

        if( it == targetByKey.end() )
        {
            result.m_unresolved++;
            continue;
        }

        // empty resolution = cache-bound, resolves only once the cache is published
        if( it->second.IsEmpty() )
        {
            if( result.m_cacheNickname.IsEmpty() )
            {
                result.m_unresolved++;
                continue;
            }

            fpid.SetLibNickname( aCacheNickname );
            fp->SetFPID( fpid );
            result.m_linkedToCache++;
        }
        else
        {
            fpid.SetLibNickname( it->second );
            fp->SetFPID( fpid );
            result.m_linkedToSource++;
        }
    }

    return result;
}


void FOOTPRINT_IMPORT_RECONCILER::writeAndRegisterCache(
        const wxString& aCacheNickname, const std::map<wxString, FOOTPRINT*>& aCacheDefs,
        FOOTPRINT_IMPORT_RECONCILE_RESULT& aResult )
{
    wxFileName finalFn( m_projectPath, aCacheNickname, FILEEXT::KiCadFootprintLibPathExtension );
    wxString   finalPath = finalFn.GetFullPath();
    wxString   tempPath = finalPath + wxS( ".tmp" );

    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );

    if( !pi )
    {
        report( m_reporter, _( "Cannot reconcile imported footprints: no KiCad footprint writer." ),
                RPT_SEVERITY_ERROR );
        return;
    }

    // best-effort cleanup, must not throw
    auto safeDelete = [&pi]( const wxString& aPath )
    {
        try
        {
            pi->DeleteLibrary( aPath );
        }
        catch( const IO_ERROR& )
        {
        }
    };

    bool wrote = false;

    try
    {
        if( wxDir::Exists( tempPath ) )
            pi->DeleteLibrary( tempPath );

        pi->CreateLibrary( tempPath );

        for( const auto& [name, def] : aCacheDefs )
        {
            std::unique_ptr<FOOTPRINT> copy( static_cast<FOOTPRINT*>( def->Clone() ) );
            LIB_ID                     id = copy->GetFPID();

            id.SetLibNickname( aCacheNickname );
            copy->SetFPID( id );
            copy->SetReference( wxS( "REF**" ) );
            pi->FootprintSave( tempPath, copy.get() );
        }

        wrote = true;
    }
    catch( const IO_ERROR& ioe )
    {
        report( m_reporter,
                wxString::Format( _( "Error writing imported footprint cache '%s': %s" ),
                                  aCacheNickname, ioe.What() ),
                RPT_SEVERITY_ERROR );
    }

    if( !wrote )
    {
        if( wxDir::Exists( tempPath ) )
            safeDelete( tempPath );

        return;
    }

    // publish temp->final, replace only a managed cache, never a user lib
    if( wxDir::Exists( finalPath ) )
    {
        if( existingIsManagedCache( m_adapter, aCacheNickname ) )
        {
            safeDelete( finalPath );
        }
        else
        {
            report( m_reporter,
                    wxString::Format( _( "A library already exists at '%s'; leaving imported "
                                         "footprints unresolved." ), finalPath ),
                    RPT_SEVERITY_ERROR );
            safeDelete( tempPath );
            return;
        }
    }

    if( !wxRenameFile( tempPath, finalPath, false ) )
    {
        report( m_reporter,
                wxString::Format( _( "Could not publish imported footprint cache to '%s'." ),
                                  finalPath ),
                RPT_SEVERITY_ERROR );
        safeDelete( tempPath );
        return;
    }

    // only claim the cache when its table row is registered, else FPIDs re-point to a dead nickname
    if( !registerCacheRow( aCacheNickname ) )
        return;

    aResult.m_cacheNickname = aCacheNickname;
    aResult.m_cacheLibraryPath = finalPath;
    aResult.m_savedToCache = static_cast<int>( aCacheDefs.size() );
}


bool FOOTPRINT_IMPORT_RECONCILER::registerCacheRow( const wxString& aCacheNickname )
{
    std::optional<LIBRARY_TABLE*> tableOpt = m_adapter.ProjectTable();

    if( !tableOpt || !*tableOpt )
    {
        report( m_reporter, _( "Cannot register imported footprint cache: no project library "
                               "table." ), RPT_SEVERITY_ERROR );
        return false;
    }

    LIBRARY_TABLE*     table = *tableOpt;
    wxString           cacheFile = aCacheNickname + wxS( "." )
                                   + wxString( FILEEXT::KiCadFootprintLibPathExtension );
    wxString           uri = wxS( "${KIPRJMOD}/" ) + cacheFile;
    LIBRARY_TABLE_ROW* row = table->HasRow( aCacheNickname )
                                     ? table->Row( aCacheNickname ).value_or( nullptr )
                                     : &table->InsertRow();

    if( !row )
        return false;

    row->SetNickname( aCacheNickname );
    row->SetURI( uri );
    row->SetType( wxS( "KiCad" ) );
    row->SetOptions( ManagedCacheOption() );
    row->SetScope( LIBRARY_TABLE_SCOPE::PROJECT );

    if( !table->Save() )
    {
        report( m_reporter, _( "Error saving project footprint library table." ),
                RPT_SEVERITY_WARNING );
    }

    // load the cache so membership and the updater resolve it
    m_adapter.LoadOne( aCacheNickname );
    return true;
}
