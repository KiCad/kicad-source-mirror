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

class PCB_EDIT_FRAME;
class EDA_3D_MODEL_VIEWER;


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
    void showPreview( const wxString& aAbsPath );
    void populateCandidatesList( int aMissingIndex );

    /// Apply bold styling to the missing-list row iff no replacement is
    /// currently selected (m_selectedPerMissing[idx] == -1), so rows still
    /// needing user attention stand out visually.
    void updateMissingItemStyle( int aMissingIndex );

    /// Size the dialog on first construction: cap at 0.9 * parent width when
    /// opening for the first time, always cap at 0.9 * screen width.
    void applyInitialSizeCaps();

    PCB_EDIT_FRAME*              m_frame;
    EDA_3D_MODEL_VIEWER*         m_modelViewer;

    /// Unique WRL filenames (as stored in FP_3DMODEL::m_Filename, i.e. with
    /// `${VAR}` still present) that the resolver could not locate.
    std::vector<wxString>                        m_missing;
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
