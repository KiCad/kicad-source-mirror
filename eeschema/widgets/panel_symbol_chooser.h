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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef PANEL_SYMBOL_CHOOSER_H
#define PANEL_SYMBOL_CHOOSER_H

#include <template_fieldnames.h>
#include <variant_symbol_utils.h>
#include <widgets/lib_tree.h>
#include <symbol_tree_model_adapter.h>
#include <footprint_info.h>
#include <widgets/html_window.h>
#include <mdw/client/mdw_part.h>
#include <widgets/workspace_parts_search.h>

#include <map>
#include <optional>

class wxPanel;
class wxTimer;
class wxChoice;
class wxSplitterWindow;
class wxStaticText;

class SYMBOL_LIBRARY_FILTER;
class LIB_SYMBOL;
class SYMBOL_PREVIEW_WIDGET;
class FOOTPRINT_PREVIEW_WIDGET;
class FOOTPRINT_SELECT_WIDGET;
class SCH_BASE_FRAME;
struct PICKED_SYMBOL;


/// Feeds one search row to LIB_TREE_NODE_ITEM, which copies what it needs
class WORKSPACE_ROW_ITEM : public LIB_TREE_ITEM
{
public:
    explicit WORKSPACE_ROW_ITEM( const WORKSPACE_PARTS_SEARCH::ROW& aRow ) :
            m_row( aRow )
    {
    }

    LIB_ID   GetLIB_ID() const override { return m_row.symbol; }
    wxString GetName() const override { return m_row.text; }
    wxString GetLibNickname() const override { return m_row.symbol.GetLibNickname(); }
    wxString GetDesc() override { return m_row.part.DisplayDescription(); }
    wxString GetFootprint() const override;
    void     GetChooserFields( std::map<wxString, wxString>& aColumnMap ) override;

    std::vector<SEARCH_TERM>& GetSearchTerms() override { return m_terms; }

private:
    const WORKSPACE_PARTS_SEARCH::ROW& m_row;
    std::vector<SEARCH_TERM>           m_terms;
};


class PANEL_SYMBOL_CHOOSER : public wxPanel
{
public:
    /**
     * Create dialog to choose symbol.
     *
     * @param aFrame  the parent frame (usually a SCH_EDIT_FRAME or SYMBOL_CHOOSER_FRAME)
     * @param aParent the parent window (usually a DIALOG_SHIM or SYMBOL_CHOOSER_FRAME)
     * @param aFilter
     * @param aHistoryList
     * @param aAlreadyPlaced
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit fields (currently just
     *                          footprint selection) will not be available.
     * @param aShowFootprints   if false, all footprint preview and selection features are disabled. This
     *                          forces aAllowFieldEdits false too.
     * @param aCancelled [out] value indicating the user has cancelled the loading symbols progress dialog
     *                   before we even get to showing the symbol chooser dialog.
     * @param aAcceptHandler a handler to be called on double-click of a footprint
     * @param aEscapeHandler a handler to be called on \<ESC\>
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
     * @param aBodyStyle if not NULL the body style is filled here.
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr, int* aBodyStyle = nullptr ) const;

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

    void SetCompatibilityCallback( SYMBOL_COMPAT_FUNC aFunc );

    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> Adapter() const { return m_adapter; }

    void Regenerate();

    /// Accept the selection, first checking that a workspace part is still current
    void AcceptSelection();

    /// The workspace part behind the accepted symbol, or nothing for a library symbol
    const std::optional<MDW_PART_INFO>& GetWorkspacePart() const { return m_pickedPart; }

protected:
    static constexpr int DBLCLICK_DELAY = 100; // milliseconds

    wxPanel* constructRightPanel( wxWindow* aParent );

    void OnDetailsCharHook( wxKeyEvent& aEvt );
    void onCloseTimer( wxTimerEvent& aEvent );
    void onOpenLibsTimer( wxTimerEvent& aEvent );

    void onFootprintSelected( wxCommandEvent& aEvent );
    void onSymbolSelected( wxCommandEvent& aEvent );

    void onSelectBodyStyle( wxCommandEvent& aEvent );
    void updateBodyStyleChoice( LIB_SYMBOL* aSymbol );

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

    void refillWorkspaceGroup( size_t aGroup );

    /// The search row behind a tree node, or null for a library node
    const WORKSPACE_PARTS_SEARCH::ROW* workspaceRow( const LIB_TREE_NODE* aNode, size_t* aGroup = nullptr ) const;

    WORKSPACE_PARTS_SEARCH::RESOLUTION resolveWorkspacePart( const MDW_PART_INFO& aPart, LIB_ID& aSymbol ) const;

    wxString workspacePartDetails( size_t aGroup, const WORKSPACE_PARTS_SEARCH::ROW& aRow,
                                   const wxString& aNote ) const;

    /// Pick aPart, placing aSymbol, and close
    void acceptWorkspacePart( const MDW_PART_INFO& aPart, const LIB_ID& aSymbol );

public:
    static std::mutex g_Mutex;

protected:
    static wxString           g_symbolSearchString;
    static wxString           g_powerSearchString;

    LIB_ID                    m_curSymbolId;
    MDW_ITEM_REF              m_curPart;          ///< The workspace part selected, empty for a library row

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

    SCH_BASE_FRAME*           m_frame;
    std::function<void()>     m_acceptHandler;
    std::function<void()>     m_escapeHandler;

    bool                      m_showPower;
    bool                      m_allow_field_edits;
    bool                      m_show_footprints;
    wxString                  m_fp_override;
    wxStaticText*             m_bodyStyleLabel;
    wxChoice*                 m_bodyStyleChoice;
    int                       m_selectedBodyStyle;

    std::vector<std::pair<FIELD_T, wxString>>  m_field_edits;

    SYMBOL_COMPAT_FUNC        m_compatCallback;

    std::unique_ptr<WORKSPACE_PARTS_SEARCH> m_workspaceParts;
    std::vector<LIB_TREE_NODE_LIBRARY*>     m_workspaceGroups;

    /// Group and row index of each workspace node now in the tree
    std::map<const LIB_TREE_NODE*, std::pair<size_t, size_t>> m_workspaceRows;

    MDW_OP                       m_confirmOp;
    wxTimer*                     m_confirmTimer;
    std::optional<MDW_PART_INFO> m_pickedPart;
    LIB_ID                       m_pickedSymbol;

    /// The workspace part each Recently Used symbol was placed as, by formatted LIB_ID
    std::map<wxString, MDW_PART_IDENTITY> m_recentParts;
};

#endif /* PANEL_SYMBOL_CHOOSER_H */
