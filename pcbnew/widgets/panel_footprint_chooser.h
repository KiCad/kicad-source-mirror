/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
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
#ifndef PANEL_FOOTPRINT_CHOOSER_H
#define PANEL_FOOTPRINT_CHOOSER_H

#include <widgets/lib_tree.h>
#include <fp_tree_model_adapter.h>
#include <footprint_info.h>
#include <widgets/footprint_preview_widget.h>

class wxTimer;
class wxSplitterWindow;
class wxBoxSizer;

class PCB_BASE_FRAME;

// When a new footprint is selected, a custom event is sent, for instance to update
// 3D viewer. So declare a FP_SELECTION_EVENT event
wxDECLARE_EVENT(FP_SELECTION_EVENT, wxCommandEvent);

class PANEL_FOOTPRINT_CHOOSER : public wxPanel
{
public:
    /**
     * Create dialog to choose component.
     *
     * @param aFrame  the parent frame (usually a PCB_EDIT_FRAME or FOOTPRINT_CHOOSER_FRAME)
     * @param aParent the parent window (usually a DIALOG_SHIM or FOOTPRINT_CHOOSER_FRAME)
     * @param aAcceptHandler a handler to be called on double-click of a footprint
     * @param aEscapeHandler a handler to be called on <ESC>
     */
    PANEL_FOOTPRINT_CHOOSER( PCB_BASE_FRAME* aFrame, wxTopLevelWindow* aParent,
                             const wxArrayString& aFootprintHistoryList,
                             std::function<bool( LIB_TREE_NODE& )> aFilter,
                             std::function<void()> aAcceptHandler,
                             std::function<void()> aEscapeHandler );

    ~PANEL_FOOTPRINT_CHOOSER();

    void FinishSetup();

    void SetPreselect( const LIB_ID& aPreselect );

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId() const;

    int GetItemCount() const { return m_adapter->GetItemCount(); }

    wxWindow* GetFocusTarget() const { return m_tree->GetFocusTarget(); }

    wxSizer* GetFiltersSizer() const { return m_tree->GetFiltersSizer(); }

    void Regenerate() { m_tree->Regenerate( true ); }

    FOOTPRINT_PREVIEW_WIDGET* GetViewerPanel() const { return m_preview_ctrl; }

    wxSplitterWindow* GetVerticalSpliter() const { return m_vsplitter; }

    wxPanel* GetDetailsPanel() const { return m_detailsPanel; }

protected:
    static constexpr int DblClickDelay = 100; // milliseconds

    void OnDetailsCharHook( wxKeyEvent& aEvt );
    void onCloseTimer( wxTimerEvent& aEvent );
    void onOpenLibsTimer( wxTimerEvent& aEvent );

    /**
     * Handle parent frame menu events to block tree preview
     */
    void onMenuOpen( wxMenuEvent& aEvent );
    void onMenuClose( wxMenuEvent& aEvent );

    void onFootprintSelected( wxCommandEvent& aEvent );

    /**
     * Handle the selection of an item. This is called when either the search
     * box or the tree receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it
     * is a component, the component is picked.
     */
    void onFootprintChosen( wxCommandEvent& aEvent );

public:
    wxPanel*                  m_RightPanel;
    wxBoxSizer*               m_RightPanelSizer;

    const FOOTPRINT*          m_CurrFootprint;

protected:
    wxPanel*                  m_detailsPanel;
    wxTimer*                  m_dbl_click_timer;
    wxTimer*                  m_open_libs_timer;
    wxSplitterWindow*         m_hsplitter;
    wxSplitterWindow*         m_vsplitter;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    FOOTPRINT_PREVIEW_WIDGET*               m_preview_ctrl;
    LIB_TREE*                               m_tree;
    HTML_WINDOW*                            m_details;

    PCB_BASE_FRAME*                         m_frame;
    std::function<bool( LIB_TREE_NODE& )>   m_filter;
    std::function<void()>                   m_acceptHandler;
    std::function<void()>                   m_escapeHandler;
};

#endif /* PANEL_FOOTPRINT_CHOOSER_H */
