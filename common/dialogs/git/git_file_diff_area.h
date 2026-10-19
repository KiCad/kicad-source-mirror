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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GIT_FILE_DIFF_AREA_H
#define GIT_FILE_DIFF_AREA_H

#include <functional>
#include <memory>

#include <wx/panel.h>

#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_transfer.h>
#include <diff_merge/diff_doc_kind.h>
#include <git/text_diff.h>

class EDA_BASE_FRAME;
class WIDGET_DIFF_CANVAS;
class wxSplitterWindow;
class wxStaticText;
class wxStyledTextCtrl;

namespace KICAD_DIFF
{
class DIFF_CANVAS_CONTEXT;
}


/**
 * Shows what changed in one file: the old and new drawing side by side for a schematic or
 * board, a line diff for a text file, or a note when there is nothing to compare.
 */
class GIT_FILE_DIFF_AREA : public wxPanel
{
public:
    GIT_FILE_DIFF_AREA( wxWindow* aParent, EDA_BASE_FRAME* aFrame );
    ~GIT_FILE_DIFF_AREA() override;

    /// Writes the two sides on a worker with the session's action lock held. Captures must not
    /// refer to widgets, frames or the project, which belong to the UI thread.
    using MATERIALIZE = std::function<bool( KIGIT_COMMON&, GIT_DIFF_WORKSPACE&, GIT_FILE_SIDES& )>;

    /// Compare the two sides aMaterialize writes out, or show its reason instead.
    void ShowFile( const std::shared_ptr<KIGIT_COMMON>& aSession, const MATERIALIZE& aMaterialize,
                   const wxString& aBeforeLabel, const wxString& aAfterLabel );

    /// Drop the documents and the drawing canvases.
    void Clear();

private:
    void showNote( const wxString& aNote );
    void showDiff( const GIT_FILE_SIDES& aSides, const std::vector<KIGIT::TEXT_DIFF_LINE>& aTextLines,
                   const wxString& aBeforeLabel, const wxString& aAfterLabel );
    void showTextDiff( const std::vector<KIGIT::TEXT_DIFF_LINE>& aLines );
    bool showDrawingDiff( KICAD_DIFF::DOC_KIND aKind, const wxString& aBefore, const wxString& aAfter,
                          const wxString& aBeforeLabel, const wxString& aAfterLabel );

    /// The two drawing canvases exist only while a drawing is compared. Each holds a GAL
    /// context, and a hidden one still paints over the text diff on macOS.
    void ensureCanvases();
    void destroyCanvases();

    /// Give aFollower the view of aLead, so panning or zooming one moves both.
    void followCanvas( WIDGET_DIFF_CANVAS* aLead, WIDGET_DIFF_CANVAS* aFollower );

    EDA_BASE_FRAME*     m_frame;
    wxStaticText*       m_note;
    wxSplitterWindow*   m_canvasSplitter;
    wxStaticText*       m_beforeLabel;
    wxStaticText*       m_afterLabel;
    WIDGET_DIFF_CANVAS* m_beforeCanvas;
    WIDGET_DIFF_CANVAS* m_afterCanvas;
    wxStyledTextCtrl*   m_textDiff;
    wxSizer*            m_legend;

    /// A newer file selection discards any materialization still running for the previous one.
    KIGIT_QUERY m_query;

    /// The documents the two canvases draw, held by the kiface that loaded them. Reset before
    /// the canvases go.
    std::unique_ptr<KICAD_DIFF::DIFF_CANVAS_CONTEXT> m_diffContext;
};

#endif // GIT_FILE_DIFF_AREA_H
