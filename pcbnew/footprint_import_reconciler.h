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

#ifndef FOOTPRINT_IMPORT_RECONCILER_H
#define FOOTPRINT_IMPORT_RECONCILER_H

#include <map>
#include <memory>
#include <vector>
#include <wx/string.h>

class BOARD;
class FOOTPRINT;
class FOOTPRINT_LIBRARY_ADAPTER;
class REPORTER;

/**
 * Outcome of a post-import footprint-library reconciliation pass.
 */
struct FOOTPRINT_IMPORT_RECONCILE_RESULT
{
    int      m_linkedToSource = 0;   ///< board FPIDs re-pointed at a provenance source library
    int      m_linkedToCache  = 0;   ///< board FPIDs re-pointed at the generated cache
    int      m_unresolved     = 0;   ///< board FPIDs left unresolved
    int      m_savedToCache   = 0;   ///< distinct definitions written into the cache library
    wxString m_cacheNickname;        ///< nickname of the generated cache, empty if none written
    wxString m_cacheLibraryPath;     ///< absolute path to the generated .pretty, empty if none

    bool Ok() const { return m_unresolved == 0; }
};

/**
 * Frame-independent, non-interactive service that reconciles the footprint-library references of a
 * freshly imported (non-KiCad) board so that every board footprint FPID resolves to a registered
 * project library.
 *
 * A footprint whose definition provably lives in a provenance source library is re-linked there
 * ("prefer source"); anything left over is written into a single manager-chosen generated cache
 * (.pretty) registered in the project footprint-library table ("generate residual").  The service
 * never scans arbitrary loaded/global libraries and never deletes a user library.
 *
 * The footprint-library adapter and project path are injected so the service can be exercised with
 * a temporary project and a locally-owned LIBRARY_MANAGER, without a frame.
 */
class FOOTPRINT_IMPORT_RECONCILER
{
public:
    FOOTPRINT_IMPORT_RECONCILER( FOOTPRINT_LIBRARY_ADAPTER& aAdapter, const wxString& aProjectPath,
                                 REPORTER* aReporter = nullptr );

    /**
     * Reconcile @p aBoard against the importer definitions and the provenance source libraries.
     *
     * @param aBoard is the imported board whose footprint FPIDs are re-pointed in place.
     * @param aDefinitions are the caller-owned canonical footprint definitions produced by the
     *                     importer (e.g. PCB_IO::GetImportedCachedLibraryFootprints()).  Ownership
     *                     is consumed by this call.
     * @param aCacheNickname is the manager-chosen collision-free generated-cache nickname.
     * @param aSourceLibNicknames are the provenance source footprint-library nicknames registered
     *                            for this import.
     */
    FOOTPRINT_IMPORT_RECONCILE_RESULT
    Reconcile( BOARD* aBoard, std::vector<std::unique_ptr<FOOTPRINT>> aDefinitions,
               const wxString& aCacheNickname, const std::vector<wxString>& aSourceLibNicknames );

    /// Options string identifying a footprint-library table row as a generated import cache.
    static const wxString& ManagedCacheOption();

private:
    /// Write the residual definitions into an atomically-published .pretty and register its row.
    void writeAndRegisterCache( const wxString&                       aCacheNickname,
                                const std::map<wxString, FOOTPRINT*>& aCacheDefs,
                                FOOTPRINT_IMPORT_RECONCILE_RESULT&    aResult );

    /// Insert or refresh the project footprint-library-table row for the generated cache.
    /// Returns false when no project table exists or the row could not be created.
    bool registerCacheRow( const wxString& aCacheNickname );

    FOOTPRINT_LIBRARY_ADAPTER& m_adapter;
    wxString                   m_projectPath;
    REPORTER*                  m_reporter;
};

#endif // FOOTPRINT_IMPORT_RECONCILER_H
