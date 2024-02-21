/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef PANEL_DESIGN_BLOCK_CHOOSER_H
#define PANEL_DESIGN_BLOCK_CHOOSER_H

#include <widgets/lib_tree.h>
#include <design_block_tree_model_adapter.h>
#include <widgets/html_window.h>

class wxPanel;
class wxTimer;
class wxSplitterWindow;

class SCH_EDIT_FRAME;
class DESIGN_BLOCK_PREVIEW_WIDGET;


class PANEL_DESIGN_BLOCK_CHOOSER : public wxPanel
{
public:
/**
 * Create dialog to choose design_block.
 *
 * @param aFrame  the parent frame (usually a SCH_EDIT_FRAME or DESIGN_BLOCK_CHOOSER_FRAME)
 * @param aParent the parent window (usually a DIALOG_SHIM or DESIGN_BLOCK_CHOOSER_FRAME)
 * @param aAcceptHandler a handler to be called on double-click of a footprint
 * @param aEscapeHandler a handler to be called on <ESC>
 */
    PANEL_DESIGN_BLOCK_CHOOSER( SCH_EDIT_FRAME* aFrame, wxWindow* aParent,
                                std::vector<LIB_ID>&  aHistoryList,
                                std::function<void()> aSelectHandler );

    ~PANEL_DESIGN_BLOCK_CHOOSER();

    void OnChar( wxKeyEvent& aEvent );

    void FinishSetup();

    void SetPreselect( const LIB_ID& aPreselect );

    void RefreshLibs( bool aProgress = false );

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * For multi-unit design_blocks, if the user selects the design_block itself rather than picking
     * an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced with whatever
     * default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the design_block that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;
    void   SelectLibId( const LIB_ID& aLibId );

    LIB_TREE* GetLibTree() { return m_tree; }

protected:
    static constexpr int DBLCLICK_DELAY = 100; // milliseconds

    void OnDetailsCharHook( wxKeyEvent& aEvt );
    void onCloseTimer( wxTimerEvent& aEvent );
    void onOpenLibsTimer( wxTimerEvent& aEvent );

    void onDesignBlockSelected( wxCommandEvent& aEvent );

    /**
     * Handle the selection of an item. This is called when either the search box or the tree
     * receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it is a design_block, the
     * design_block is picked.
     */
    void onDesignBlockChosen( wxCommandEvent& aEvent );

    void addDesignBlockToHistory( const LIB_ID& aLibId );
    void rebuildHistoryNode();

    void displayErrors( wxTopLevelWindow* aWindow );

    static wxString g_designBlockSearchString;

    wxTimer*          m_dbl_click_timer;
    wxTimer*          m_open_libs_timer;
    wxSplitterWindow* m_vsplitter;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    LIB_TREE*    m_tree;
    DESIGN_BLOCK_PREVIEW_WIDGET* m_preview;

    SCH_EDIT_FRAME*       m_frame;
    std::function<void()> m_selectHandler;

    std::vector<LIB_ID> m_historyList;
};

#endif /* PANEL_DESIGN_BLOCK_CHOOSER_H */
