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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <set>
#include <vector>
#include <wx/string.h>
#include <wx/listctrl.h>

#include <dialogs/dialog_migrate_3d_models_base.h>
#include <3d_canvas/board_adapter.h>
#include <3d_rendering/track_ball.h>
#include <math/vector3.h>

class PCB_EDIT_FRAME;
class BOARD;
class FOOTPRINT;
class EDA_3D_CANVAS;


/**
 * Dialog that offers to migrate obsolete WRL 3D model references on a loaded
 * board to current STEP models.  Shown when a board references `.wrl` / `.wrz`
 * files that the FILENAME_RESOLVER cannot resolve (typically KiCad 9 boards
 * opened in KiCad 10+).
 *
 * Three-column layout: list of missing WRL references, list of plausible
 * STEP replacements ranked by name similarity, and a 3D preview of the
 * selected candidate.  Selections are applied as a single BOARD_COMMIT
 * when the user clicks "Replace Models".
 */
class DIALOG_MIGRATE_3D_MODELS : public DIALOG_MIGRATE_3D_MODELS_BASE
{
public:
    DIALOG_MIGRATE_3D_MODELS( PCB_EDIT_FRAME* aFrame );
    ~DIALOG_MIGRATE_3D_MODELS() override;

    /// Scan the board for unresolvable `.wrl`/`.wrz` references.  Returns
    /// true if any were found (in which case the dialog is worth showing).
    bool HasMissingModels() const { return !m_missing.empty(); }

    /// Cheap precheck that avoids constructing the dialog (and its embedded
    /// 3D canvas) when the board has nothing to migrate.  Returns true when
    /// at least one footprint model references a `.wrl`/`.wrz` file that
    /// the project's FILENAME_RESOLVER cannot resolve.
    static bool BoardHasUnresolvedWrlReferences( PCB_EDIT_FRAME* aFrame );

    /// Count of unique unresolvable `.wrl`/`.wrz` references on the board
    /// (deduplicated by filename).  Intended for the post-load infobar label.
    static int CountUnresolvedWrlReferences( PCB_EDIT_FRAME* aFrame );

    /// Silently rewrite unresolvable `.wrl`/`.wrz` references to STEP files
    /// whose filename stem matches in the standard 3D search paths.  Uses
    /// BOARD_COMMIT so the change is undoable and marks the board dirty.
    /// Intended for the GUI load path; has no UI and opens no dialog.
    /// Returns the number of FP_3DMODEL entries rewritten.
    static int AutoMigrateByFilename( PCB_EDIT_FRAME* aFrame );

protected:
    void OnMissingSelected( wxListEvent& event ) override;
    void OnCandidateSelected( wxListEvent& event ) override;
    void OnAddSearchDirectoryClick( wxCommandEvent& event ) override;
    void OnOpenExternalFileClick( wxCommandEvent& event ) override;
    void OnReplaceClick( wxCommandEvent& event ) override;
    void OnKeepClick( wxCommandEvent& event ) override;

private:
    /// An entry in the STEP catalog.  m_absPath is the deduplication key.
    struct CATALOG_ENTRY
    {
        wxString m_absPath;  ///< Absolute on-disk path
        wxString m_stem;     ///< Filename without directory or extension
        wxString m_parent;   ///< Parent directory basename (e.g. "Resistor_SMD.3dshapes")
    };

    /// A ranked candidate shown in column 2.  A score of 0 means "keep existing".
    struct MATCH_CANDIDATE
    {
        wxString m_absPath;  ///< Absolute path, or empty for the "keep existing" row
        wxString m_display;  ///< Label shown to the user
        int      m_score;    ///< Higher = better match
    };

    void collectMissingModels();
    void buildCatalog();
    void scanDirectory( const wxString& aDir );
    void populateMissingList();
    void rankAllCandidates();
    std::vector<MATCH_CANDIDATE> rankCandidatesFor( const wxString& aWrlFilename ) const;

    /// Set up the throwaway board/footprint that the preview canvas renders.
    void initPreviewBoard();

    /// Replace the preview's footprint and model list to reflect the current
    /// missing-list selection + candidate choice.  Pass -1 for aCandAbsPath.IsEmpty()
    /// (or an empty path) to render the representative footprint with no 3D
    /// model attached.
    void showPreview( int aMissingIndex, const wxString& aCandAbsPath );
    void populateCandidatesList( int aMissingIndex );

    /// Apply bold styling to the missing-list row iff no replacement is
    /// currently selected (m_selectedPerMissing[idx] == -1), so rows still
    /// needing user attention stand out visually.
    void updateMissingItemStyle( int aMissingIndex );

    /// Size the dialog on first construction: cap at 0.9 * parent width when
    /// opening for the first time, always cap at 0.9 * screen width.
    void applyInitialSizeCaps();

    PCB_EDIT_FRAME*              m_frame;

    /// Preview pipeline.  We host the footprint on a throwaway board so the
    /// canvas renders it on FR4 with pads/silk in place, the same way the
    /// footprint properties 3D preview panel does.
    BOARD*                       m_dummyBoard;
    FOOTPRINT*                   m_dummyFootprint;   ///< Owned by m_dummyBoard
    BOARD_ADAPTER                m_boardAdapter;
    TRACK_BALL                   m_trackBallCamera;
    EDA_3D_CANVAS*               m_previewPane;

    /// Transform data captured from the first FP_3DMODEL entry that referenced
    /// each missing WRL filename, so candidate replacements sit at the same
    /// position the original WRL would have occupied.
    struct MISSING_XFORM
    {
        VECTOR3D m_scale    { 1, 1, 1 };
        VECTOR3D m_rotation { 0, 0, 0 };
        VECTOR3D m_offset   { 0, 0, 0 };
        double   m_opacity  = 1.0;
    };

    /// Unique WRL filenames (as stored in FP_3DMODEL::m_Filename, i.e. with
    /// `${VAR}` still present) that the resolver could not locate.
    std::vector<wxString>                        m_missing;
    /// Source FP_3DMODEL transform per missing filename, used when building
    /// the candidate model for preview.
    std::vector<MISSING_XFORM>                   m_missingXform;
    /// Representative footprint per missing filename.  Non-owning pointer into
    /// the real board; used as the template for m_dummyFootprint when the
    /// corresponding row is selected in the missing-list.
    std::vector<const FOOTPRINT*>                m_missingRepFp;
    /// Ranked candidates per missing filename, keyed by index into m_missing.
    std::vector<std::vector<MATCH_CANDIDATE>>          m_candidatesPerMissing;
    /// Which candidate index the user has selected for each missing entry.
    /// -1 means "keep existing" (the default).
    std::vector<int>                             m_selectedPerMissing;

    /// Catalog of STEP-compatible model paths, deduped by absolute path.
    std::vector<CATALOG_ENTRY>                   m_catalog;
    /// Set of already-scanned directories (absolute path, case-normalised).
    std::set<wxString>                           m_scannedDirs;
};
