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

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

#include <wx/dir.h>
#include <wx/filename.h>

#include <board.h>
#include <footprint.h>
#include <import_proj_properties.h>
#include <lib_id.h>
#include <pad.h>
#include <project.h>
#include <project_pcb.h>
#include <reporter.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>
#include <io/io_mgr.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <footprint_library_adapter.h>
#include <libraries/library_table.h>


FOOTPRINT_IMPORT_RECONCILER::FOOTPRINT_IMPORT_RECONCILER( FOOTPRINT_LIBRARY_ADAPTER& aAdapter,
                                                         const wxString& aProjectPath,
                                                         REPORTER&       aReporter ) :
        m_adapter( aAdapter ),
        m_projectPath( aProjectPath ),
        m_reporter( aReporter )
{
}


namespace
{
std::multiset<wxString> padNumbers( const FOOTPRINT& aFp )
{
    std::multiset<wxString> numbers;

    for( const PAD* pad : aFp.Pads() )
        numbers.insert( pad->GetNumber() );

    return numbers;
}


// two tools never draw a footprint identically, so equivalence is the pad set
bool sameInterface( const FOOTPRINT& aLhs, const FOOTPRINT& aRhs )
{
    return padNumbers( aLhs ) == padNumbers( aRhs );
}


// unset and explicit default thermal settings resolve the same
bool sameCopperOverrides( const PADSTACK& aLhs, const PADSTACK& aRhs, PCB_LAYER_ID aLayer )
{
    const PADSTACK::COPPER_LAYER_PROPS& lhs = aLhs.CopperLayer( aLayer );
    const PADSTACK::COPPER_LAYER_PROPS& rhs = aRhs.CopperLayer( aLayer );

    return lhs.zone_connection.value_or( ZONE_CONNECTION::INHERITED )
                   == rhs.zone_connection.value_or( ZONE_CONNECTION::INHERITED )
           && lhs.thermal_spoke_width == rhs.thermal_spoke_width
           && aLhs.ThermalSpokeAngle( aLayer ) == aRhs.ThermalSpokeAngle( aLayer )
           && lhs.thermal_gap == rhs.thermal_gap
           && lhs.clearance == rhs.clearance
           && ( aLhs.Shape( aLayer ) != PAD_SHAPE::CUSTOM || aLhs.AnchorShape( aLayer ) == aRhs.AnchorShape( aLayer ) );
}


// the parity check skips pad overrides and fab settings, yet updating from the library replaces them
bool samePadOverrides( FOOTPRINT& aInstance, const FOOTPRINT& aDef )
{
    std::unique_ptr<FOOTPRINT> def( static_cast<FOOTPRINT*>( aDef.Clone() ) );

    def->SetParent( aInstance.GetBoard() );

    if( def->IsFlipped() != aInstance.IsFlipped() )
        def->Flip( { 0, 0 }, FLIP_DIRECTION::TOP_BOTTOM );

    std::set<PAD*, FOOTPRINT::cmp_pads> lhs( aInstance.Pads().begin(), aInstance.Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> rhs( def->Pads().begin(), def->Pads().end() );

    bool same = std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                            []( const PAD* aLhs, const PAD* aRhs )
                            {
                                const PADSTACK& lhsStack = aLhs->Padstack();
                                const PADSTACK& rhsStack = aRhs->Padstack();

                                if( !( lhsStack.FrontOuterLayers() == rhsStack.FrontOuterLayers() )
                                    || !( lhsStack.BackOuterLayers() == rhsStack.BackOuterLayers() )
                                    || aLhs->GetCustomShapeInZoneOpt() != aRhs->GetCustomShapeInZoneOpt()
                                    || !( aLhs->GetTeardropParams() == aRhs->GetTeardropParams() )
                                    || aLhs->GetPadToDieDelay() != aRhs->GetPadToDieDelay()
                                    || aLhs->GetSimElectricalType() != aRhs->GetSimElectricalType()
                                    || aLhs->GetTopBackdrillSize() != aRhs->GetTopBackdrillSize()
                                    || aLhs->GetTopBackdrillLayer() != aRhs->GetTopBackdrillLayer()
                                    || aLhs->GetBottomBackdrillSize() != aRhs->GetBottomBackdrillSize()
                                    || aLhs->GetBottomBackdrillLayer() != aRhs->GetBottomBackdrillLayer()
                                    || !( lhsStack.FrontPostMachining() == rhsStack.FrontPostMachining() )
                                    || !( lhsStack.BackPostMachining() == rhsStack.BackPostMachining() ) )
                                {
                                    return false;
                                }

                                bool layersMatch = true;

                                // the stacks may group layers differently, so walk the layers of both
                                auto check =
                                        [&]( PCB_LAYER_ID aLayer )
                                        {
                                            layersMatch &= sameCopperOverrides( lhsStack, rhsStack, aLayer );
                                        };

                                lhsStack.ForEachUniqueLayer( check );
                                rhsStack.ForEachUniqueLayer( check );

                                return layersMatch;
                            } );

    // a parentless temporary must not touch the board timestamp when it is destroyed
    def->SetParent( nullptr );

    return same;
}


// cheap pose-independent key, so the full library comparison only runs between footprints that can be equal
using POSE_SIGNATURE = std::tuple<size_t, size_t, size_t, long long, long long>;


POSE_SIGNATURE poseSignature( const FOOTPRINT& aFp )
{
    std::unique_ptr<FOOTPRINT> pose( static_cast<FOOTPRINT*>( aFp.Clone() ) );

    if( pose->IsFlipped() )
        pose->Flip( pose->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );

    pose->SetOrientation( ANGLE_0 );

    BOX2I bbox = pose->GetBoundingBox( false );

    return { pose->Pads().size(), pose->GraphicalItems().size(), pose->Zones().size(), (long long) bbox.GetWidth(),
             (long long) bbox.GetHeight() };
}


// reuse existing row/dir only if prior import-managed cache
bool isManagedCache( const LIBRARY_TABLE_ROW* aRow )
{
    return aRow && aRow->GetOptionsMap().count( IMPORT_PROJ_PROPS::MANAGED_CACHE_KEY ) > 0;
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

    // a nickname with no footprint name links to nothing, and it would name a library that does not exist
    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetFPID().GetLibItemName().empty() && !fp->GetFPID().GetLibNickname().empty() )
            fp->SetFPID( LIB_ID() );
    }

    // source nickname + item name, so same-name parts from different libraries stay split
    using SOURCE_KEY = std::pair<wxString, wxString>;

    std::map<SOURCE_KEY, FOOTPRINT*>            defByKey;
    std::map<wxString, std::vector<FOOTPRINT*>> defsByName;

    for( const std::unique_ptr<FOOTPRINT>& def : aDefinitions )
    {
        wxString name = def->GetFPID().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        defByKey.emplace( SOURCE_KEY( def->GetFPID().GetUniStringLibNickname(), name ), def.get() );
        defsByName[name].push_back( def.get() );
    }

    // definitions can carry a different nickname from the placed footprints, so a unique name still
    // matches, but an ambiguous one must not
    auto findDef = [&]( const wxString& aNick, const wxString& aName ) -> FOOTPRINT*
    {
        if( auto it = defByKey.find( SOURCE_KEY( aNick, aName ) ); it != defByKey.end() )
            return it->second;

        auto byName = defsByName.find( aName );

        if( byName == defsByName.end() || byName->second.size() != 1 )
            return nullptr;

        return byName->second.front();
    };

    // preload source libs before membership queries
    for( const wxString& nick : aSourceLibNicknames )
    {
        if( m_adapter.GetRow( nick ) )
            m_adapter.LoadOne( nick );
    }

    std::set<wxString> provenance( aSourceLibNicknames.begin(), aSourceLibNicknames.end() );

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
            if( !m_adapter.GetRow( nick ) )
                continue;

            // A nickname the importer emitted is not provenance.  An unrelated library that
            // happens to carry the name must not swallow the imported definition, so it takes the
            // link only when it holds the same footprint.
            if( provenance.count( nick ) )
            {
                // some plugins throw for a footprint their library lacks
                bool exists = false;

                try
                {
                    exists = m_adapter.FootprintExists( nick, aName );
                }
                catch( const IO_ERROR& )
                {
                }

                if( !exists )
                    continue;
            }
            else
            {
                FOOTPRINT* def = findDef( ownNick, aName );

                if( !def )
                    continue;

                // one load answers both existence and equivalence, FootprintExists is itself a load
                std::unique_ptr<FOOTPRINT> candidate( m_adapter.LoadFootprint( nick, aName, true ) );

                if( !candidate || !sameInterface( *candidate, *def ) )
                    continue;
            }

            matches.push_back( nick );
        }

        return matches.size() == 1 ? matches.front() : wxString( wxEmptyString );
    };

    // per-instance target, empty target = cache-bound
    std::map<SOURCE_KEY, wxString>                targetByKey;
    std::set<SOURCE_KEY>                          cacheKeys;
    std::map<SOURCE_KEY, std::vector<FOOTPRINT*>> instancesByKey;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        wxString name = fp->GetFPID().GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        SOURCE_KEY key( fp->GetFPID().GetUniStringLibNickname(), name );

        instancesByKey[key].push_back( fp );

        if( targetByKey.count( key ) )
            continue;

        wxString sourceNick = resolveSource( fp, name );
        targetByKey[key] = sourceNick;

        if( sourceNick.IsEmpty() )
            cacheKeys.insert( key );
    }

    // a .pretty holds one file per footprint, so the file name must be unique, and case-folded for
    // a cache moved to Windows or macOS
    std::set<wxString> takenFiles;

    auto uniqueName = [&takenFiles]( const wxString& aName )
    {
        for( int suffix = 0; ; ++suffix )
        {
            wxString candidate = suffix ? wxString::Format( wxS( "%s_%d" ), aName, suffix ) : aName;

            // the library indexes items by file name, so the item name must already be a legal one
            ReplaceIllegalFileNameChars( candidate, '_' );

            wxString fileName = candidate;
            fileName.MakeLower();

            if( takenFiles.insert( fileName ).second )
                return candidate;
        }
    };

    // canonical def per cache key, fall back to unique placed instance if importer gave none
    std::map<wxString, FOOTPRINT*>          cacheDefs;
    std::map<SOURCE_KEY, wxString>          cacheNameByKey;
    std::map<const FOOTPRINT*, wxString>    cacheNameByDef;
    std::vector<std::unique_ptr<FOOTPRINT>> placedDefs;
    std::vector<wxString>                   renameReports;

    std::map<const FOOTPRINT*, wxString> cacheNameByInstance;

    for( const SOURCE_KEY& key : cacheKeys )
    {
        const wxString& name = key.second;
        FOOTPRINT*      def = findDef( key.first, name );

        // each placed instance must equal the copy it is linked to, or updating from the library edits the board
        struct ITEM
        {
            FOOTPRINT*     m_Def;
            POSE_SIGNATURE m_Signature;
            wxString       m_CacheName;
        };

        std::vector<ITEM> items;

        if( def )
            items.push_back( { def, poseSignature( *def ), wxString() } );

        for( FOOTPRINT* instance : instancesByKey[key] )
        {
            POSE_SIGNATURE signature = poseSignature( *instance );

            auto match = std::find_if( items.begin(), items.end(),
                                       [&]( const ITEM& aItem )
                                       {
                                           return aItem.m_Signature == signature
                                                  && !instance->FootprintNeedsUpdate(
                                                          aItem.m_Def, BOARD_ITEM::COMPARE_FLAGS::DRC )
                                                  && samePadOverrides( *instance, *aItem.m_Def );
                                       } );

            if( match == items.end() )
            {
                placedDefs.emplace_back( static_cast<FOOTPRINT*>( instance->Clone() ) );
                items.push_back( { placedDefs.back().get(), signature, wxString() } );
                match = items.end() - 1;
            }

            // the cache name is bound lazily so only items that serve an instance are published
            if( match->m_CacheName.IsEmpty() )
            {
                // one definition serving several source libraries stays a single cache item
                auto known = cacheNameByDef.find( match->m_Def );

                if( known != cacheNameByDef.end() )
                {
                    match->m_CacheName = known->second;
                }
                else
                {
                    match->m_CacheName = uniqueName( name );

                    cacheNameByDef[match->m_Def] = match->m_CacheName;
                    cacheDefs[match->m_CacheName] = match->m_Def;
                }
            }

            cacheNameByInstance[instance] = match->m_CacheName;
        }

        if( items.size() > 1 && !instancesByKey[key].empty() )
        {
            m_reporter.Report( wxString::Format( _( "Imported footprint '%s' has %zu differing placed "
                                                    "versions; each is kept as its own library item." ),
                                                 name, items.size() ),
                               RPT_SEVERITY_INFO );
        }

        // a definition nothing is placed from still goes in under its own name
        if( def && cacheNameByDef.find( def ) == cacheNameByDef.end() )
        {
            wxString cacheName = uniqueName( name );

            cacheNameByDef[def] = cacheName;
            cacheDefs[cacheName] = def;
        }

        if( !instancesByKey[key].empty() )
            cacheNameByKey[key] = cacheNameByInstance[instancesByKey[key].front()];
        else if( def )
            cacheNameByKey[key] = cacheNameByDef[def];

        if( cacheNameByKey.count( key ) && cacheNameByKey[key] != name )
        {
            renameReports.push_back(
                    wxString::Format( _( "Imported footprint '%s' from '%s' was renamed to '%s' "
                                         "in the import cache." ), name, key.first, cacheNameByKey[key] ) );
        }
    }

    // write residuals to an atomic .pretty and register the row
    if( !cacheDefs.empty() )
        writeAndRegisterCache( aCacheNickname, cacheDefs, result );

    // no rename happened if the cache did not publish
    if( !result.m_cacheNickname.IsEmpty() )
    {
        for( const wxString& report : renameReports )
            m_reporter.Report( report, RPT_SEVERITY_WARNING );
    }

    // re-point nicks to the resolved lib, cache-bound footprints also take their cache item name
    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        LIB_ID   fpid = fp->GetFPID();
        wxString name = fpid.GetUniStringLibItemName();

        if( name.IsEmpty() )
            continue;

        SOURCE_KEY key( fpid.GetUniStringLibNickname(), name );
        auto       it = targetByKey.find( key );

        if( it == targetByKey.end() )
        {
            result.m_unresolved++;
            continue;
        }

        // empty resolution = cache-bound, resolves only once the cache is published
        if( it->second.IsEmpty() )
        {
            auto cacheName = cacheNameByInstance.find( fp );

            if( cacheName == cacheNameByInstance.end() )
            {
                auto byKey = cacheNameByKey.find( key );

                if( byKey != cacheNameByKey.end() )
                    cacheName = cacheNameByInstance.emplace( fp, byKey->second ).first;
            }

            if( result.m_cacheNickname.IsEmpty() || cacheName == cacheNameByInstance.end() )
            {
                result.m_unresolved++;
                continue;
            }

            fpid.SetLibNickname( aCacheNickname );
            fpid.SetLibItemName( cacheName->second );
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

    // a nickname the user already owns is never repurposed, whatever its row points at
    LIBRARY_TABLE_ROW* existingRow = m_adapter.GetRow( aCacheNickname ).value_or( nullptr );

    if( existingRow && !isManagedCache( existingRow ) )
    {
        m_reporter.Report( wxString::Format( _( "A footprint library named '%s' is already "
                                                "registered; leaving imported footprints "
                                                "unresolved." ), aCacheNickname ),
                           RPT_SEVERITY_ERROR );
        return;
    }

    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );

    if( !pi )
    {
        m_reporter.Report( _( "Cannot reconcile imported footprints: no KiCad footprint "
                              "writer." ), RPT_SEVERITY_ERROR );
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

        // without this every save re-parses the whole library written so far
        std::map<std::string, UTF8> properties { { "skip_cache_validation", "" } };

        // the definitions are ours to consume and FootprintSave copies what it keeps
        for( const auto& [name, def] : aCacheDefs )
        {
            LIB_ID id = def->GetFPID();

            id.SetLibNickname( aCacheNickname );
            id.SetLibItemName( name );
            def->SetFPID( id );
            def->SetReference( wxS( "REF**" ) );
            pi->FootprintSave( tempPath, def, &properties );
        }

        wrote = true;
    }
    catch( const IO_ERROR& ioe )
    {
        m_reporter.Report( wxString::Format( _( "Error writing imported footprint cache "
                                                "'%s': %s" ), aCacheNickname, ioe.What() ),
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
        if( isManagedCache( existingRow ) )
        {
            safeDelete( finalPath );
        }
        else
        {
            m_reporter.Report( wxString::Format( _( "A library already exists at '%s'; leaving "
                                                    "imported footprints unresolved." ),
                                                 finalPath ),
                               RPT_SEVERITY_ERROR );
            safeDelete( tempPath );
            return;
        }
    }

    if( !wxRenameFile( tempPath, finalPath, false ) )
    {
        m_reporter.Report( wxString::Format( _( "Could not publish imported footprint cache to "
                                                "'%s'." ), finalPath ),
                           RPT_SEVERITY_ERROR );
        safeDelete( tempPath );
        return;
    }

    // only claim the cache when its table row is registered, else FPIDs re-point to a dead nickname
    if( !registerCacheRow( aCacheNickname ) )
        return;

    aResult.m_cacheNickname = aCacheNickname;
    aResult.m_savedToCache = static_cast<int>( aCacheDefs.size() );
}


bool FOOTPRINT_IMPORT_RECONCILER::registerCacheRow( const wxString& aCacheNickname )
{
    std::optional<LIBRARY_TABLE*> tableOpt = m_adapter.ProjectTable();

    if( !tableOpt || !*tableOpt )
    {
        m_reporter.Report( _( "Cannot register imported footprint cache: no project library "
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
    row->SetOptions( IMPORT_PROJ_PROPS::ManagedCacheOption() );
    row->SetScope( LIBRARY_TABLE_SCOPE::PROJECT );

    // an unsaved row is gone on restart, so the cache cannot be claimed
    if( !table->Save() )
    {
        m_reporter.Report( _( "Error saving project footprint library table; imported footprints "
                              "left unresolved." ), RPT_SEVERITY_ERROR );
        return false;
    }

    // load the cache so membership and the updater resolve it
    m_adapter.LoadOne( aCacheNickname );
    return true;
}


FOOTPRINT_IMPORT_RECONCILE_RESULT
ReconcileImportedFootprints( std::vector<std::unique_ptr<FOOTPRINT>> aDefinitions, BOARD& aBoard,
                             PROJECT& aProject, const wxString& aBoardPath,
                             const std::map<std::string, UTF8>* aProperties, REPORTER& aReporter )
{
    FOOTPRINT_IMPORT_RECONCILE_RESULT result;

    // an importer that publishes nothing still reconciles, the placed footprints are the fallback
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &aProject );

    if( !adapter )
        return result;

    // manager pre-commits the cache nickname + source libs; standalone import derives from filename
    wxString              cacheNick;
    std::vector<wxString> sourceLibs;
    IMPORT_PROJ_PROPS::ReadFootprintProps( aProperties, cacheNick, sourceLibs );

    if( cacheNick.IsEmpty() )
        cacheNick = IMPORT_PROJ_PROPS::MakeCacheNickname( wxFileName( aBoardPath ).GetName() );

    FOOTPRINT_IMPORT_RECONCILER reconciler( *adapter, aProject.GetProjectPath(), aReporter );

    // reconciliation failure must not abort the import
    try
    {
        result = reconciler.Reconcile( &aBoard, std::move( aDefinitions ), cacheNick, sourceLibs );
    }
    catch( const IO_ERROR& ioe )
    {
        aReporter.Report( wxString::Format( _( "Could not reconcile imported footprint libraries: "
                                               "%s" ), ioe.What() ), RPT_SEVERITY_ERROR );
    }

    return result;
}


FOOTPRINT_IMPORT_RECONCILE_RESULT
ReconcileImportedFootprints( PCB_IO& aPlugin, BOARD& aBoard, PROJECT& aProject,
                             const wxString& aBoardPath,
                             const std::map<std::string, UTF8>* aProperties, REPORTER& aReporter )
{
    std::vector<std::unique_ptr<FOOTPRINT>> definitions;

    try
    {
        for( FOOTPRINT* footprint : aPlugin.GetImportedCachedLibraryFootprints() )
            definitions.emplace_back( footprint );
    }
    catch( const IO_ERROR& )
    {
        // importer retains no definitions, the placed footprints are the fallback
    }

    return ReconcileImportedFootprints( std::move( definitions ), aBoard, aProject, aBoardPath,
                                        aProperties, aReporter );
}


std::map<wxString, wxString> ImportedFootprintIds( const BOARD& aBoard )
{
    std::map<wxString, wxString> ids;
    std::set<wxString>           repeated;

    for( const FOOTPRINT* footprint : aBoard.Footprints() )
    {
        const wxString& ref = footprint->GetReference();

        if( ref.IsEmpty() || footprint->GetFPID().GetLibItemName().empty() )
            continue;

        if( !ids.emplace( ref, footprint->GetFPID().Format().wx_str() ).second )
            repeated.insert( ref );
    }

    for( const wxString& ref : repeated )
        ids.erase( ref );

    return ids;
}
