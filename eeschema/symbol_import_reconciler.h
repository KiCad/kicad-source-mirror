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

#ifndef SYMBOL_IMPORT_RECONCILER_H
#define SYMBOL_IMPORT_RECONCILER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <wx/string.h>

#include <reporter.h>

class LIB_SYMBOL;
class PROJECT;
class SCHEMATIC;
class SCH_IO;
class SYMBOL_LIBRARY_ADAPTER;
class UTF8;

/**
 * Outcome of a post-import symbol-library reconciliation pass.
 */
struct SYMBOL_IMPORT_RECONCILE_RESULT
{
    int      m_linkedToSource = 0;   ///< schematic LIB_IDs re-pointed at a provenance source library
    int      m_linkedToCache  = 0;   ///< schematic LIB_IDs re-pointed at the generated cache
    int      m_unresolved     = 0;   ///< schematic LIB_IDs left unresolved
    int      m_savedToCache   = 0;   ///< distinct definitions written into the cache library
    wxString m_cacheNickname;        ///< nickname of the generated cache, empty if none written
};

/**
 * Frame-independent, non-interactive service that reconciles the symbol-library references of a
 * freshly imported (non-KiCad) schematic so that every symbol LIB_ID resolves to a registered
 * project library.
 *
 * This is the schematic counterpart of #FOOTPRINT_IMPORT_RECONCILER and follows the same policy.
 * A symbol whose definition provably lives in a provenance source library is re-linked there
 * ("prefer source"); anything left over is written into a single caller-chosen generated cache
 * (.kicad_sym) registered in the project symbol-library table ("generate residual").  The service
 * never scans arbitrary loaded/global libraries and never deletes a user library.
 *
 * The symbol-library adapter and project path are injected so the service can be exercised with a
 * temporary project and a locally-owned LIBRARY_MANAGER, without a frame.
 */
class SYMBOL_IMPORT_RECONCILER
{
public:
    SYMBOL_IMPORT_RECONCILER( SYMBOL_LIBRARY_ADAPTER& aAdapter, const wxString& aProjectPath,
                              REPORTER& aReporter = NULL_REPORTER::GetInstance() );

    /**
     * Reconcile @p aSchematic against the importer definitions and the provenance source libraries.
     *
     * @param aSchematic is the imported schematic whose symbol LIB_IDs are re-pointed in place.
     * @param aDefinitions are the caller-owned canonical symbol definitions produced by the
     *                     importer (e.g. SCH_IO::GetImportedCachedLibrarySymbols()).  Ownership
     *                     is consumed by this call.
     * @param aCacheNickname is the caller-chosen collision-free generated-cache nickname.
     * @param aSourceLibNicknames are the provenance source symbol-library nicknames registered
     *                            for this import.
     */
    SYMBOL_IMPORT_RECONCILE_RESULT
    Reconcile( SCHEMATIC* aSchematic, std::vector<std::unique_ptr<LIB_SYMBOL>> aDefinitions,
               const wxString& aCacheNickname, const std::vector<wxString>& aSourceLibNicknames );

private:
    /// Write the residual definitions into an atomically-published .kicad_sym and register its row.
    /// @p aCacheDefs is keyed by the item name each definition takes in the cache.
    void writeAndRegisterCache( const wxString&                        aCacheNickname,
                                const std::map<wxString, LIB_SYMBOL*>& aCacheDefs,
                                SYMBOL_IMPORT_RECONCILE_RESULT&        aResult );

    /// Insert or refresh the project symbol-library-table row for the generated cache.
    /// Returns false when no project table exists or the row could not be created.
    bool registerCacheRow( const wxString& aCacheNickname );

    SYMBOL_LIBRARY_ADAPTER& m_adapter;
    wxString                m_projectPath;
    REPORTER&               m_reporter;
};

/**
 * Reconcile @p aSchematic against the definitions @p aPlugin retained while loading it.
 *
 * Shared by the interactive import and the `kicad-cli sch import` job so both materialize the
 * project symbol library and re-point LIB_IDs.  Importers publishing no definitions of their own
 * are skipped, and a reconciliation failure is reported rather than thrown so an import is never
 * aborted by it.
 *
 * @param aProperties carries the manager-chosen cache nickname and provenance source libraries;
 *                    a standalone import passes nullptr and the nickname is derived from
 *                    @p aSchematicPath.
 */
SYMBOL_IMPORT_RECONCILE_RESULT
ReconcileImportedSymbols( SCH_IO& aPlugin, SCHEMATIC& aSchematic, PROJECT& aProject,
                          const wxString& aSchematicPath,
                          const std::map<std::string, UTF8>* aProperties, REPORTER& aReporter );

#endif // SYMBOL_IMPORT_RECONCILER_H
