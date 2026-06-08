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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_TAB_ART_H
#define KICAD_TAB_ART_H

#include <functional>

#include <wx/aui/auibook.h>
#include <wx/aui/tabart.h>


/// Visual decorations derived from document state: preview is italic, modified is bold with a
/// leading asterisk, and modified wins over preview.
struct TAB_VISUAL_STATE
{
    bool italic       = false;
    bool bold         = false;
    bool showAsterisk = false;
};


/**
 * Resolve a tab's decorations from its document state flags.
 *
 * The pinned flag has no decoration of its own and is accepted only for API symmetry.
 */
TAB_VISUAL_STATE ResolveTabVisualState( bool aPreview, bool aModified, bool aPinned );


/// A wxAuiTabArt that renders editor tabs with preview/modified/pinned decorations. Per-page state
/// comes from a caller callback keyed on the page window, since wxAuiNotebookPage has no slot for it.
class KICAD_TAB_ART : public wxAuiDefaultTabArt
{
public:
    /// Returns the current visual state for a page window.
    using STATE_FN = std::function<TAB_VISUAL_STATE( wxWindow* )>;

    explicit KICAD_TAB_ART( STATE_FN aProvider );

    wxAuiTabArt* Clone() override;

    void DrawTab( wxDC& aDc, wxWindow* aWnd, const wxAuiNotebookPage& aPage, const wxRect& aInRect,
                  int aCloseButtonState, wxRect* aOutTabRect, wxRect* aOutButtonRect,
                  int* aXExtent ) override;

    wxSize GetTabSize( wxDC& aDc, wxWindow* aWnd, const wxString& aCaption,
                       const wxBitmapBundle& aBitmap, bool aActive, int aCloseButtonState,
                       int* aXExtent ) override;

private:
    /**
     * Visual state for a page, or a default if no provider is set.
     */
    TAB_VISUAL_STATE stateFor( wxWindow* aPageWindow ) const;

    STATE_FN m_stateFn;
};

#endif // KICAD_TAB_ART_H
