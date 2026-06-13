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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_SCRATCH_DOC_H
#define KICAD_SCRATCH_DOC_H

#include <functional>
#include <memory>
#include <utility>

#include <project.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>
#include <wx/string.h>


/**
 * Move-only RAII wrapper for "load a foreign PROJECT non-active and unload
 * it on scope exit." Project-only counterpart to SCRATCH_DOC (which adds a
 * dependent document); use when only the project's settings are needed.
 *
 * The aRequireProjectFile flag splits two real use cases. The diff/merge
 * JOB handlers load from temp blobs that often have no sibling .kicad_pro,
 * so they pass false and accept a SETTINGS_MANAGER defaults-only slot. The
 * Import-Settings dialogs pass true because a missing .kicad_pro means
 * there's no meaningful settings source to import from.
 *
 * If the project was already loaded in the SETTINGS_MANAGER before the
 * wrapper was constructed, the destructor leaves it alone so the slot
 * belongs to whoever opened it first.
 */
class SCRATCH_PROJECT
{
public:
    SCRATCH_PROJECT() = default;

    SCRATCH_PROJECT( SETTINGS_MANAGER& aMgr, const wxString& aDocOrProjectPath,
                     bool aRequireProjectFile = false ) :
            m_mgr( &aMgr )
    {
        wxFileName pro( aDocOrProjectPath );
        pro.SetExt( FILEEXT::ProjectFileExtension );
        pro.MakeAbsolute();

        const wxString projectPath = pro.GetFullPath();
        const bool     fileMissing = !pro.FileExists();

        // Even if a defaults-only slot was loaded earlier by someone else,
        // refuse when the caller demands a real .kicad_pro that doesn't exist.
        // Otherwise the import dialog accepts garbage from a stale slot.
        if( aRequireProjectFile && fileMissing )
            return;

        if( PROJECT* existing = aMgr.GetProject( projectPath ) )
        {
            // Someone else already loaded this slot; don't UnloadProject in
            // our dtor or we'd pull the rug out from under them.
            m_project = existing;
            m_owned   = false;
            return;
        }

        const bool loadOk = aMgr.LoadProject( projectPath, /*aSetActive=*/false );
        m_project = aMgr.GetProject( projectPath );

        if( !m_project )
            return;

        m_owned = true;

        // Reject a malformed .kicad_pro when the file is present but failed
        // to parse — a defaults-only slot is not a substitute.
        if( !loadOk && !fileMissing )
        {
            release();
            m_project = nullptr;
        }
    }

    ~SCRATCH_PROJECT() { release(); }

    SCRATCH_PROJECT( const SCRATCH_PROJECT& )            = delete;
    SCRATCH_PROJECT& operator=( const SCRATCH_PROJECT& ) = delete;

    SCRATCH_PROJECT( SCRATCH_PROJECT&& aOther ) noexcept { *this = std::move( aOther ); }

    SCRATCH_PROJECT& operator=( SCRATCH_PROJECT&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            release();

            m_mgr     = std::exchange( aOther.m_mgr, nullptr );
            m_project = std::exchange( aOther.m_project, nullptr );
            m_owned   = std::exchange( aOther.m_owned, false );
        }

        return *this;
    }

    PROJECT* GetProject() const { return m_project; }

    /// True iff the project loaded (or was already loaded). False on parse
    /// failure or, when require-file was set, on missing .kicad_pro.
    bool IsValid() const { return m_project != nullptr; }

private:
    void release()
    {
        if( m_owned && m_mgr && m_project && m_project != &m_mgr->Prj() )
            m_mgr->UnloadProject( m_project, false );

        m_owned = false;
    }

    SETTINGS_MANAGER* m_mgr     = nullptr;
    PROJECT*          m_project = nullptr;
    bool              m_owned   = false;
};


/**
 * Move-only RAII wrapper for "load a KiCad document into a non-active scratch
 * PROJECT and clean up after." Pattern used by the diff/merge JOB handlers
 * so they can load ancestor/ours/theirs documents without disturbing the
 * active editor's project settings.
 *
 * The destructor sequences cleanup in the only safe order:
 *
 *   1. clearProject(doc) — severs the document's raw m_project pointer
 *      (BOARD::ClearProject / SCHEMATIC::SetProject(nullptr)).
 *   2. doc.reset() — destroys the document while its project is still
 *      attached to the SETTINGS_MANAGER.
 *   3. mgr->UnloadProject(project, false) — releases the scratch project,
 *      but only if it isn't the active Prj() (the user might have asked us
 *      to load a path that happens to be the live project).
 *
 * Construct via the factory `LoadScratchDoc()` so the call site doesn't
 * have to spell the LoadProject/GetProject dance.
 */
template<typename DOC>
struct SCRATCH_DOC
{
    using CLEAR_FN = std::function<void( DOC* )>;

    SETTINGS_MANAGER*    mgr     = nullptr;
    std::unique_ptr<DOC> doc;
    PROJECT*             project = nullptr;
    CLEAR_FN             clearProject;

    /// True when *this* load created the project (LoadScratchDoc found the
    /// path not yet in the SettingsManager). False when the project was
    /// already loaded by a previous call (e.g. ancestor and ours share a
    /// path, or the caller is diffing against the active editor's project).
    /// Only owners unload — otherwise two SCRATCH_DOCs holding the same
    /// PROJECT* would double-unload it and the second would dangle.
    bool                 ownsProject = false;

    SCRATCH_DOC() = default;

    SCRATCH_DOC( const SCRATCH_DOC& )            = delete;
    SCRATCH_DOC& operator=( const SCRATCH_DOC& ) = delete;

    SCRATCH_DOC( SCRATCH_DOC&& aOther ) noexcept { *this = std::move( aOther ); }

    SCRATCH_DOC& operator=( SCRATCH_DOC&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            release();

            mgr          = std::exchange( aOther.mgr, nullptr );
            doc          = std::move( aOther.doc );
            project      = std::exchange( aOther.project, nullptr );
            ownsProject  = std::exchange( aOther.ownsProject, false );
            clearProject = std::move( aOther.clearProject );
        }

        return *this;
    }

    ~SCRATCH_DOC() { release(); }

private:
    void release()
    {
        if( doc && clearProject )
            clearProject( doc.get() );

        doc.reset();

        if( mgr && project && ownsProject && project != &mgr->Prj() )
            mgr->UnloadProject( project, false );

        mgr         = nullptr;
        project     = nullptr;
        ownsProject = false;
    }
};


/**
 * Construct a SCRATCH_DOC by loading a project non-active and then handing it
 * to the caller's document loader. The loader gets the live PROJECT* and must
 * return a unique_ptr<DOC> (nullptr on failure). The clear callback severs
 * the document's project back-pointer in the destructor.
 */
template<typename DOC, typename Loader, typename ClearFn>
SCRATCH_DOC<DOC> LoadScratchDoc( SETTINGS_MANAGER& aMgr, const wxString& aDocPath,
                                 Loader aLoader, ClearFn aClearFn )
{
    SCRATCH_DOC<DOC> out;
    out.mgr          = &aMgr;
    out.clearProject = aClearFn;

    if( aDocPath.IsEmpty() )
        return out;

    wxFileName pro( aDocPath );
    pro.SetExt( FILEEXT::ProjectFileExtension );
    pro.MakeAbsolute();

    // LoadProject returns true both when newly loaded AND when already
    // present in the manager (see SETTINGS_MANAGER::LoadProject line 970).
    // We need to know which case we hit so SCRATCH_DOC::release() doesn't
    // double-unload a project shared with another SCRATCH_DOC — query the
    // manager BEFORE LoadProject so we can compare.
    const bool projectAlreadyLoaded = ( aMgr.GetProject( pro.GetFullPath() ) != nullptr );

    // LoadProject returns false but still inserts a defaults-only PROJECT
    // when no .kicad_pro exists alongside the document. A present-but-
    // malformed .kicad_pro should be a hard failure though, otherwise the
    // diff/merge runs against garbage settings.
    const bool projectLoadOk = aMgr.LoadProject( pro.GetFullPath(), /*aSetActive=*/false );
    out.project = aMgr.GetProject( pro.GetFullPath() );

    if( !out.project )
        return out;

    out.ownsProject = !projectAlreadyLoaded;

    if( !projectLoadOk && pro.FileExists() )
    {
        if( out.ownsProject && out.project != &aMgr.Prj() )
            aMgr.UnloadProject( out.project, false );

        out.project     = nullptr;
        out.ownsProject = false;
        return out;
    }

    out.doc = aLoader( out.project );

    return out;
}

#endif // KICAD_SCRATCH_DOC_H
