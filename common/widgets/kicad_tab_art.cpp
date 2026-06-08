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

#include <widgets/kicad_tab_art.h>

#include <algorithm>

#include <wx/dc.h>
#include <wx/pen.h>


TAB_VISUAL_STATE ResolveTabVisualState( bool aPreview, bool aModified, bool aPinned )
{
    // Pinning has no glyph; a pinned tab is already cleared of preview and renders normal.
    ( void ) aPinned;

    TAB_VISUAL_STATE state;

    // An edit promotes a preview to a permanent tab, so modified wins.
    if( aModified )
    {
        state.bold = true;
        state.showAsterisk = true;
    }
    else if( aPreview )
    {
        state.italic = true;
    }

    return state;
}


KICAD_TAB_ART::KICAD_TAB_ART( STATE_FN aProvider ) :
        m_stateFn( std::move( aProvider ) )
{
    // Sizing receives no per-tab state, so measure with the widest font (bold) and no tab can clip.
    wxFont measuring = m_measuringFont;
    measuring.SetWeight( wxFONTWEIGHT_BOLD );
    m_measuringFont = measuring;
}


wxAuiTabArt* KICAD_TAB_ART::Clone()
{
    return new KICAD_TAB_ART( m_stateFn );
}


TAB_VISUAL_STATE KICAD_TAB_ART::stateFor( wxWindow* aPageWindow ) const
{
    if( m_stateFn )
        return m_stateFn( aPageWindow );

    return TAB_VISUAL_STATE{};
}


void KICAD_TAB_ART::DrawTab( wxDC& aDc, wxWindow* aWnd, const wxAuiNotebookPage& aPage,
                             const wxRect& aInRect, int aCloseButtonState, wxRect* aOutTabRect,
                             wxRect* aOutButtonRect, int* aXExtent )
{
    const TAB_VISUAL_STATE st = stateFor( aPage.window );

    // Decorate a local copy so the notebook's stored caption stays unmutated.
    wxAuiNotebookPage local = aPage;

    if( st.showAsterisk )
        local.caption = wxT( "*" ) + local.caption;

    wxFont decorated = aPage.active ? m_selectedFont : m_normalFont;
    decorated.SetStyle( st.italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL );
    decorated.SetWeight( st.bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL );

    const wxFont savedNormal = m_normalFont;
    const wxFont savedSelected = m_selectedFont;

    m_normalFont = decorated;
    m_selectedFont = decorated;

    wxAuiDefaultTabArt::DrawTab( aDc, aWnd, local, aInRect, aCloseButtonState, aOutTabRect,
                                 aOutButtonRect, aXExtent );

    m_normalFont = savedNormal;
    m_selectedFont = savedSelected;
}


wxSize KICAD_TAB_ART::GetTabSize( wxDC& aDc, wxWindow* aWnd, const wxString& aCaption,
                                  const wxBitmapBundle& aBitmap, bool aActive,
                                  int aCloseButtonState, int* aXExtent )
{
    // Always reserve the leading "*" width since a modified tab prepends one when drawn.
    const wxString reserved = wxT( "*" ) + aCaption;

    return wxAuiDefaultTabArt::GetTabSize( aDc, aWnd, reserved, aBitmap, aActive, aCloseButtonState,
                                           aXExtent );
}
