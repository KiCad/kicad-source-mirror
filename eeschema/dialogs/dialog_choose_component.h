/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <cmp_tree_model_adapter.h>
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

class COMPONENT_TREE;
class FOOTPRINT_PREVIEW_WIDGET;
class FOOTPRINT_SELECT_WIDGET;
class LIB_ALIAS;
class LIB_PART;
class SCH_BASE_FRAME;


/**
 * Dialog class to select a component from the libraries. This is the master
 * View class in a Model-View-Adapter (mediated MVC) architecture. The other
 * pieces are in:
 *
 * - Adapter: CMP_TREE_MODEL_ADAPTER in eeschema/cmp_tree_model_adapter.h
 * - Model: CMP_TREE_NODE and descendants in eeschema/cmp_tree_model.h
 *
 * Because everything is tied together in the adapter class, see that file
 * for thorough documentation. A simple example usage follows:
 *
 *     // Create the adapter class
 *     auto adapter( Prj().SchLibs() );
 *
 *     // Perform any configuration of adapter properties here
 *     adapter->SetPreselectNode( "TL072", 2 );
 *
 *     // Initialize model from PART_LIBs
 *     for( PART_LIB& lib: *libs )
 *         adapter->AddLibrary( lib );
 *
 *     // Create and display dialog
 *     DIALOG_CHOOSE_COMPONENT dlg( this, title, adapter, 1 );
 *     bool selected = ( dlg.ShowModal() != wxID_CANCEL );
 *
 *     // Receive part
 *     if( selected )
 *     {
 *         int unit;
 *         LIB_ALIAS* alias = dlg.GetSelectedAlias( &unit );
 *         do_something( alias, unit );
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
     * @param aAdapter  CMP_TREE_MODEL_ADAPTER::PTR. See CMP_TREE_MODEL_ADAPTER
     *                  for documentation.
     * @param aDeMorganConvert  preferred deMorgan conversion
     *                          (TODO: should happen in dialog)
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit
     *      fields (currently just footprint selection) will not be available.
     */
    DIALOG_CHOOSE_COMPONENT( SCH_BASE_FRAME* aParent, const wxString& aTitle,
            CMP_TREE_MODEL_ADAPTER::PTR& aAdapter, int aDeMorganConvert, bool aAllowFieldEdits );

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
     * @return the alias that has been selected, or NULL if there is none.
     */
    LIB_ALIAS* GetSelectedAlias( int* aUnit = nullptr ) const;

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

protected:
    static constexpr int DblClickDelay = 100; // milliseconds

    wxPanel* ConstructRightPanel( wxWindow* aParent );

    void OnInitDialog( wxInitDialogEvent& aEvent );
    void OnCloseTimer( wxTimerEvent& aEvent );
    void OnProgressTimer( wxTimerEvent& aEvent );

    void OnTreeSelect( wxDataViewEvent& aEvent );
    void OnTreeActivate( wxDataViewEvent& aEvent );

    void OnSchViewDClick( wxMouseEvent& aEvent );
    void OnSchViewPaint( wxPaintEvent& aEvent );

    void OnFootprintSelected( wxCommandEvent& aEvent );

    /**
     * Look up the footprint for a given alias and display it.
     */
    void ShowFootprintFor( LIB_ALIAS* aAlias );

    /**
     * Display the given footprint by name.
     */
    void ShowFootprint( wxString const& aName );

    /**
     * Populate the footprint selector for a given alias.
     *
     * @param aAlias alias, or null to clear
     */
    void PopulateFootprintSelector( LIB_ALIAS* aAlias );

    /**
     * Display a given component into the schematic symbol preview.
     */
    void RenderPreview( LIB_PART* aComponent, int aUnit );

    /**
     * Handle the selection of an item. This is called when either the search
     * box or the tree receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it
     * is a component, the component is picked.
     */
    void HandleItemSelection();

    wxTimer*        m_dbl_click_timer;
    wxPanel*        m_sch_view_ctrl;

    FOOTPRINT_SELECT_WIDGET*  m_fp_sel_ctrl;
    FOOTPRINT_PREVIEW_WIDGET* m_fp_view_ctrl;
    COMPONENT_TREE*           m_tree;

    SCH_BASE_FRAME*             m_parent;
    int                         m_deMorganConvert;
    bool                        m_allow_field_edits;
    bool                        m_external_browser_requested;
    wxString                    m_fp_override;

    static FOOTPRINT_ASYNC_LOADER          m_fp_loader;
    static std::unique_ptr<FOOTPRINT_LIST> m_fp_list;
    std::vector<std::pair<int, wxString>> m_field_edits;
};

#endif /* DIALOG_CHOOSE_COMPONENT_H */
