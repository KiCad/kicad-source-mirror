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

#ifndef KICAD_MERGETOOL_FRAME_H
#define KICAD_MERGETOOL_FRAME_H

#include <kiway_player.h>


/**
 * Top-level host frame for the 3-way merge resolution dialog.
 *
 * Constructed by the kicad project-manager binary when launched with the
 * `--mergetool BASE LOCAL REMOTE MERGED` arguments so it can serve as a
 * `git mergetool` driver. The frame is never Show()n — it exists only to
 * be the wxApp top window so KIWAY-dispatched modal dialogs have a valid
 * parent and so the wxApp event loop has a handler to drive.
 *
 * The launcher in kicad.cpp queues RunMerge() via wxCallAfter so the merge
 * starts after the event loop is up. RunMerge inspects the output extension
 * and calls the owning kiface's interactive merge export (KIFACE_MERGE_DOCUMENT
 * via KICAD_DIFF::DispatchMerge) with the interactive flag set, storing the
 * exit code in `m_exitCode`. The launcher then schedules the loop exit with
 * that code so the kicad process exit status reflects the merge result for
 * `git mergetool`.
 */
/// Bundle of the four input paths so the call sites can't accidentally
/// permute git's BASE/LOCAL/REMOTE/MERGED ordering — all four would otherwise
/// be same-typed wxStrings.
struct MERGETOOL_PATHS
{
    wxString ancestor;
    wxString ours;
    wxString theirs;
    wxString output;
};


class MERGETOOL_FRAME : public KIWAY_PLAYER
{
public:
    MERGETOOL_FRAME( KIWAY* aKiway, wxWindow* aParent, const MERGETOOL_PATHS& aPaths );

    ~MERGETOOL_FRAME() override;

    /// Run the merge synchronously. Returns the JOB exit code. The frame is
    /// closed by the launcher on return regardless of success or failure.
    int RunMerge();

    /// The mergetool has no canvas; this frame exists only to host the modal
    /// resolution dialog. Required by the TOOLS_HOLDER interface.
    wxWindow* GetToolCanvas() const override { return nullptr; }

    /// Override the inherited config getter to return null. The default
    /// implementation falls back to Kiface().KifaceSettings(), but the
    /// project-manager binary that hosts this frame has a Kiface() stub that
    /// aborts when called. Returning null short-circuits the
    /// SaveSettings(cfg) call in EDA_BASE_FRAME::windowClosing so the frame
    /// can close cleanly without touching settings — appropriate for a
    /// transient launcher frame that has no persistent window state.
    APP_SETTINGS_BASE* config() const override { return nullptr; }

protected:
    void doCloseWindow() override;

private:
    MERGETOOL_PATHS m_paths;
};

#endif // KICAD_MERGETOOL_FRAME_H
