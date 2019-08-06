/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef DIALOG_CHOOSE_COMPONENT_H
#define DIALOG_CHOOSE_COMPONENT_H

#include "dialog_shim.h"
#include <symbol_tree_model_adapter.h>
#include <footprint_info.h>

class wxStaticBitmap;
class wxTextCtrl;
class wxStdDialogButtonSizer;
class wxDataViewCtrl;
class wxHtmlWindow;
class wxHtmlLinkEvent;
class wxPanel;
class wxChoice;
class wxButton;
class wxTimer;

class LIB_TREE;
class SYMBOL_PREVIEW_WIDGET;
class FOOTPRINT_PREVIEW_WIDGET;
class FOOTPRINT_SELECT_WIDGET;
class LIB_ALIAS;
class LIB_PART;
class SCH_BASE_FRAME;
class SCH_DRAW_PANEL;


/**
 * Dialog class to select a component from the libraries. This is the master
 * View class in a Model-View-Adapter (mediated MVC) architecture. The other
 * pieces are in:
 *
 * - Adapter: CMP_TREE_MODEL_ADAPTER in common/cmp_tree_model_adapter.h
 * - Model: CMP_TREE_NODE and descendants in common/cmp_tree_model.h
 *
 * Because everything is tied together in the adapter class, see that file
 * for thorough documentation. A simple example usage follows:
 *
 *     // Create the adapter class
 *     auto adapter( SYMBOL_TREE_MODEL_ADAPTER::Create( Prj().SchSymbolLibTable() ) );
 *
 *     // Perform any configuration of adapter properties here
 *     adapter->SetPreselectNode( "LIB_NICKNAME", "SYMBO_NAME", 2 );
 *
 *     // Initialize model from #SYMBOL_LIB_TABLE
 *     libNicknames = libs->GetLogicalLibs();
 *
 *     for( auto nickname : libNicknames )
 *     {
 *         adapter->AddLibrary( nickname );
 *     }
 *
 *     // Create and display dialog
 *     DIALOG_CHOOSE_COMPONENT dlg( this, title, adapter, 1 );
 *     bool selected = ( dlg.ShowModal() != wxID_CANCEL );
 *
 *     // Receive part
 *     if( selected )
 *     {
 *         int unit;
 *         #LIB_ID id = dlg.GetSelectedAlias( &unit );
 *         do_something( id, unit );
 *     }
 *
 */
class DIALOG_CHOOSE_COMPONENT : public DIALOG_SHIM
{
public:
    /**
     * Create dialog to choose component.
     *
     * @param aParent   a SCH_BASE_FRAME parent window.
     * @param aTitle    Dialog title.
     * @param aAdapter  SYMBOL_TREE_MODEL_ADAPTER::PTR. See CMP_TREE_MODEL_ADAPTER
     *                  for documentation.
     * @param aDeMorganConvert  preferred deMorgan conversion
     *                          (TODO: should happen in dialog)
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit fields
     *                          (currently just footprint selection) will not be available.
     * @param aShowFootprints   if false, all footprint preview and selection features are
     *                          disabled. This forces aAllowFieldEdits false too.
     * @param aAllowBrowser     show a Select with Browser button
     */
    DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                             SYMBOL_TREE_MODEL_ADAPTER::PTR& aAdapter, int aDeMorganConvert,
                             bool aAllowFieldEdits, bool aShowFootprints, bool aAllowBrowser );

    ~DIALOG_CHOOSE_COMPONENT();

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * For multi-unit components, if the user selects the component itself
     * rather than picking an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced
     * with whatever default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    /**
     * Get a list of fields edited by the user.
     * @return vector of pairs; each.first = field ID, each.second = new value
     */
    std::vector<std::pair<int, wxString>> GetFields() const
    {
        return m_field_edits;
    }

    /** Function IsExternalBrowserSelected
     *
     * @return true, iff the user pressed the thumbnail view of the component to
     *               launch the component browser.
     */
    bool IsExternalBrowserSelected() const
    {
        return m_external_browser_requested;
    }

    static std::mutex g_Mutex;

protected:
    static constexpr int DblClickDelay = 100; // milliseconds

    wxPanel* ConstructRightPanel( wxWindow* aParent );

    void OnInitDialog( wxInitDialogEvent& aEvent );
    void OnCharHook( wxKeyEvent& aEvt );
    void OnCloseTimer( wxTimerEvent& aEvent );
    void OnUseBrowser( wxCommandEvent& aEvent );

    void OnFootprintSelected( wxCommandEvent& aEvent );
    void OnComponentPreselected( wxCommandEvent& aEvent );

    /**
     * Handle the selection of an item. This is called when either the search
     * box or the tree receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it
     * is a component, the component is picked.
     */
    void OnComponentSelected( wxCommandEvent& aEvent );

    /**
     * Look up the footprint for a given symbol specified in the #LIB_ID and display it.
     */
    void ShowFootprintFor( LIB_ID const& aLibId );

    /**
     * Display the given footprint by name.
     */
    void ShowFootprint( wxString const& aFootprint );

    /**
     * Populate the footprint selector for a given alias.
     *
     * @param aLibId the #LIB_ID of the selection or invalid to clear
     */
    void PopulateFootprintSelector( LIB_ID const& aLibId );

    /**
     * Display a given symbol into the schematic symbol preview.
     * when no symbol selected, display a tooltip
     */
    void RenderPreview( LIB_PART* aComponent, int aUnit );

    wxTimer*                  m_dbl_click_timer;
    SYMBOL_PREVIEW_WIDGET*    m_symbol_preview;
    wxButton*                 m_browser_button;
    wxSplitterWindow*         m_hsplitter;
    wxSplitterWindow*         m_vsplitter;

    FOOTPRINT_SELECT_WIDGET*  m_fp_sel_ctrl;
    FOOTPRINT_PREVIEW_WIDGET* m_fp_preview;
    LIB_TREE*                 m_tree;
    wxHtmlWindow*             m_details;

    static int                m_h_sash_pos;     // remember sash positions during a session
    static int                m_v_sash_pos;

    SCH_BASE_FRAME*           m_parent;
    int                       m_deMorganConvert;
    bool                      m_allow_field_edits;
    bool                      m_show_footprints;
    bool                      m_external_browser_requested;
    wxString                  m_fp_override;

    std::vector<std::pair<int, wxString>>  m_field_edits;

    // Remember the dialog size during a session
    static wxSize             m_last_dlg_size;
};

#endif /* DIALOG_CHOOSE_COMPONENT_H */
