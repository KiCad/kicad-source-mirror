/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
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
#ifndef PANEL_SYMBOL_CHOOSER_H
#define PANEL_SYMBOL_CHOOSER_H

#include <template_fieldnames.h>
#include <widgets/lib_tree.h>
#include <symbol_tree_model_adapter.h>
#include <footprint_info.h>
#include <widgets/html_window.h>

class wxPanel;
class wxTimer;
class wxSplitterWindow;

class SYMBOL_LIBRARY_FILTER;
class SYMBOL_PREVIEW_WIDGET;
class FOOTPRINT_PREVIEW_WIDGET;
class FOOTPRINT_SELECT_WIDGET;
class SCH_BASE_FRAME;
struct PICKED_SYMBOL;


class PANEL_SYMBOL_CHOOSER : public wxPanel
{
public:
    /**
     * Create dialog to choose symbol.
     *
     * @param aFrame  the parent frame (usually a SCH_EDIT_FRAME or SYMBOL_CHOOSER_FRAME)
     * @param aParent the parent window (usually a DIALOG_SHIM or SYMBOL_CHOOSER_FRAME)
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit fields (currently just
     *                          footprint selection) will not be available.
     * @param aShowFootprints   if false, all footprint preview and selection features are disabled. This
     *                          forces aAllowFieldEdits false too.
     * @param aCancelled [out] value indicating the user has cancelled the loading symbols progress dialog
     *                   before we even get to showing the symbol chooser dialog.
     * @param aAcceptHandler a handler to be called on double-click of a footprint
     * @param aEscapeHandler a handler to be called on <ESC>
     */
    PANEL_SYMBOL_CHOOSER( SCH_BASE_FRAME* aFrame, wxWindow* aParent,
                          const SYMBOL_LIBRARY_FILTER* aFilter,
                          std::vector<PICKED_SYMBOL>& aHistoryList,
                          std::vector<PICKED_SYMBOL>& aAlreadyPlaced,
                          bool aAllowFieldEdits, bool aShowFootprints, bool& aCancelled,
                          std::function<void()> aAcceptHandler,
                          std::function<void()> aEscapeHandler );

    ~PANEL_SYMBOL_CHOOSER();

    void OnChar( wxKeyEvent& aEvent );

    void FinishSetup();

    void SetPreselect( const LIB_ID& aPreselect );

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * For multi-unit symbols, if the user selects the symbol itself rather than picking
     * an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced with whatever
     * default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    int GetItemCount() const { return m_adapter->GetItemCount(); }

    wxWindow* GetFocusTarget() const { return m_tree->GetFocusTarget(); }

    /**
     * Get a list of fields edited by the user.
     *
     * @return vector of pairs; each.first = field type, each.second = new value.
     */
    std::vector<std::pair<FIELD_T, wxString>> GetFields() const
    {
        return m_field_edits;
    }

    void ShutdownCanvases();

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> Adapter() const { return m_adapter; }

    void Regenerate();

protected:
    static constexpr int DBLCLICK_DELAY = 100; // milliseconds

    wxPanel* constructRightPanel( wxWindow* aParent );

    void OnDetailsCharHook( wxKeyEvent& aEvt );
    void onCloseTimer( wxTimerEvent& aEvent );
    void onOpenLibsTimer( wxTimerEvent& aEvent );

    void onFootprintSelected( wxCommandEvent& aEvent );
    void onSymbolSelected( wxCommandEvent& aEvent );

    /**
     * Handle parent frame menu events to block tree preview
     */
    void onMenuOpen( wxMenuEvent& aEvent );
    void onMenuClose( wxMenuEvent& aEvent );

    /**
     * Handle the selection of an item. This is called when either the search box or the tree
     * receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it is a symbol, the
     * symbol is picked.
     */
    void onSymbolChosen( wxCommandEvent& aEvent );

    /**
     * Look up the footprint for a given symbol specified in the #LIB_ID and display it.
     */
    void showFootprintFor( const LIB_ID& aLibId );

    /**
     * Display the given footprint by name.
     */
    void showFootprint( const wxString& aFootprint );

    /**
     * Populate the footprint selector for a given alias.
     *
     * @param aLibId the #LIB_ID of the selection or invalid to clear.
     */
    void populateFootprintSelector( const LIB_ID& aLibId );

public:
    static std::mutex g_Mutex;

protected:
    static wxString           g_symbolSearchString;
    static wxString           g_powerSearchString;

    wxTimer*                  m_dbl_click_timer;
    wxTimer*                  m_open_libs_timer;
    SYMBOL_PREVIEW_WIDGET*    m_symbol_preview;
    wxSplitterWindow*         m_hsplitter;
    wxSplitterWindow*         m_vsplitter;

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    FOOTPRINT_SELECT_WIDGET*  m_fp_sel_ctrl;
    FOOTPRINT_PREVIEW_WIDGET* m_fp_preview;
    LIB_TREE*                 m_tree;
    HTML_WINDOW*              m_details;

    static SCH_BASE_FRAME*    m_frame;      // Must be static becuase used in a static function
    std::function<void()>     m_acceptHandler;
    std::function<void()>     m_escapeHandler;

    bool                      m_showPower;
    bool                      m_allow_field_edits;
    bool                      m_show_footprints;
    wxString                  m_fp_override;

    std::vector<std::pair<FIELD_T, wxString>>  m_field_edits;
};

#endif /* PANEL_SYMBOL_CHOOSER_H */
