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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/aui/auibar.h>
#include <wx/aui/dockart.h>
#include <widgets/panel_notebook_base.h>


class WX_AUI_TOOLBAR_ART : public wxAuiDefaultToolBarArt
{
public:
    WX_AUI_TOOLBAR_ART() :
            wxAuiDefaultToolBarArt()
    {}

    virtual ~WX_AUI_TOOLBAR_ART() = default;

#if wxCHECK_VERSION( 3, 3, 0 )
    wxSize GetToolSize( wxReadOnlyDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem ) override;
#else
    wxSize GetToolSize( wxDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem ) override;
#endif

    /**
     * Unfortunately we need to re-implement this to actually be able to control the size
     */
    void DrawButton( wxDC& aDc, wxWindow* aWindow, const wxAuiToolBarItem& aItem,
                     const wxRect& aRect ) override;
};


class WX_AUI_DOCK_ART : public wxAuiDefaultDockArt
{
public:
    WX_AUI_DOCK_ART();
};


/**
 * A pass-through to the existing wxAuiTabArt provider, except that it kills close buttons on
 * PANEL_NOTEBOOK_BASEs which are not closable.
 */
class WX_AUI_TAB_ART : public wxAuiTabArt
{
public:
    WX_AUI_TAB_ART( wxAuiTabArt* aTabArt ) :
            wxAuiTabArt(),
            m_tabArt( aTabArt->Clone() )
    {}

    ~WX_AUI_TAB_ART()
    {
        delete m_tabArt;
    }

    wxAuiTabArt* Clone() override
    {
        return new WX_AUI_TAB_ART( m_tabArt );
    }

    void SetFlags( unsigned int flags ) override { m_tabArt->SetFlags( flags ); }

    void SetSizingInfo( const wxSize& tabCtrlSize, size_t tabCount, wxWindow* wnd = nullptr ) override
    {
        m_tabArt->SetSizingInfo( tabCtrlSize, tabCount, wnd );
    }

    void SetNormalFont( const wxFont& font ) override       { m_tabArt->SetNormalFont( font ); }
    void SetSelectedFont( const wxFont& font ) override     { m_tabArt->SetSelectedFont( font ); }
    void SetMeasuringFont( const wxFont& font ) override    { m_tabArt->SetMeasuringFont( font ); }
    void SetColour( const wxColour& colour ) override       { m_tabArt->SetColour( colour ); }
    void SetActiveColour( const wxColour& colour ) override { m_tabArt->SetActiveColour( colour ); }

    void DrawBorder( wxDC& dc, wxWindow* wnd, const wxRect& rect ) override
    {
        m_tabArt->DrawBorder( dc, wnd, rect );
    }

    void DrawBackground( wxDC& dc, wxWindow* wnd, const wxRect& rect ) override
    {
        m_tabArt->DrawBackground( dc, wnd, rect );
    }

    void DrawTab( wxDC& dc, wxWindow* wnd, const wxAuiNotebookPage& pane, const wxRect& inRect,
                  int closeButtonState, wxRect* outTabRect, wxRect* outButtonRect, int* xExtent) override
    {
        PANEL_NOTEBOOK_BASE* panel = dynamic_cast<PANEL_NOTEBOOK_BASE*>( pane.window );

        if( panel && !panel->GetClosable() )
            closeButtonState = wxAUI_BUTTON_STATE_HIDDEN;

        m_tabArt->DrawTab( dc, wnd, pane, inRect, closeButtonState, outTabRect, outButtonRect, xExtent );
    }

    void DrawButton( wxDC& dc, wxWindow* wnd, const wxRect& inRect, int bitmapId, int buttonState,
                     int orientation, wxRect* outRect) override
    {
        m_tabArt->DrawButton( dc, wnd, inRect, bitmapId, buttonState, orientation, outRect );
    }

    wxSize GetTabSize( wxDC& dc, wxWindow* wnd, const wxString& caption, const wxBitmapBundle& bitmap,
                       bool active, int closeButtonState, int* xExtent ) override
    {
        return m_tabArt->GetTabSize( dc, wnd, caption, bitmap, active, closeButtonState, xExtent );
    }

    int ShowDropDown( wxWindow* wnd, const wxAuiNotebookPageArray& items, int activeIdx ) override
    {
        return m_tabArt->ShowDropDown( wnd, items, activeIdx );
    }

    int GetIndentSize() override                           { return m_tabArt->GetIndentSize(); }
    int GetBorderWidth( wxWindow* wnd ) override           { return m_tabArt->GetBorderWidth( wnd ); }
    int GetAdditionalBorderSpace( wxWindow* wnd ) override { return m_tabArt->GetAdditionalBorderSpace( wnd ); }

    int GetBestTabCtrlSize( wxWindow* wnd, const wxAuiNotebookPageArray& pages,
                            const wxSize& requiredBmpSize ) override
    {
        return m_tabArt->GetBestTabCtrlSize( wnd, pages, requiredBmpSize );
    }

    void UpdateColoursFromSystem() override { m_tabArt->UpdateColoursFromSystem(); }

private:
    wxAuiTabArt* m_tabArt;
};

