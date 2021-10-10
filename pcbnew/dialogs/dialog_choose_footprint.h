/*
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
#ifndef DIALOG_CHOOSE_FOOTPRINT_H
#define DIALOG_CHOOSE_FOOTPRINT_H

#include "dialog_shim.h"
#include <fp_tree_model_adapter.h>
#include <footprint_info.h>
#include <widgets/footprint_preview_widget.h>

class wxStaticBitmap;
class wxTextCtrl;
class wxStdDialogButtonSizer;
class wxDataViewCtrl;
class wxHtmlLinkEvent;
class wxPanel;
class wxChoice;
class wxButton;
class wxTimer;
class wxSplitterWindow;

class PCB_BASE_FRAME;
class LIB_TREE;
class FOOTPRINT;


/**
 * Dialog class to select a footprint from the libraries. This is the master
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
 *     auto adapter( FP_TREE_MODEL_ADAPTER::Create( Prj().PcbFootprintLibs() ) );
 *
 *     // Perform any configuration of adapter properties here
 *     adapter->SetPreselectNode( "LIB_NICKNAME", "FP_NAME", 2 );
 *
 *     // Initialize model from #FP_LIB_TABLE
 *     libNicknames = libs->GetLogicalLibs();
 *
 *     for( auto nickname : libNicknames )
 *     {
 *         adapter->AddLibrary( nickname );
 *     }
 *
 *     // Create and display dialog
 *     DIALOG_CHOOSE_FOOTPRINT dlg( this, title, adapter, 1 );
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
class DIALOG_CHOOSE_FOOTPRINT : public DIALOG_SHIM
{
public:
    /**
     * Create dialog to choose component.
     *
     * @param aParent   a PCB_BASE_FRAME parent window.
     * @param aAdapter  FP_TREE_MODEL_ADAPTER::PTR. See CMP_TREE_MODEL_ADAPTER
     *                  for documentation.
     */
    DIALOG_CHOOSE_FOOTPRINT( PCB_BASE_FRAME* aParent, const wxString& aTitle,
                             wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter );

    ~DIALOG_CHOOSE_FOOTPRINT();

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId() const;

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

    void OnCloseTimer( wxTimerEvent& aEvent );
    void OnUseBrowser( wxCommandEvent& aEvent );

    void OnComponentPreselected( wxCommandEvent& aEvent );

    /**
     * Handle the selection of an item. This is called when either the search
     * box or the tree receive an Enter, or the tree receives a double click.
     * If the item selected is a category, it is expanded or collapsed; if it
     * is a component, the component is picked.
     */
    void OnComponentSelected( wxCommandEvent& aEvent );

    wxTimer*                  m_dbl_click_timer;
    wxButton*                 m_browser_button;
    wxSplitterWindow*         m_hsplitter;
    wxSplitterWindow*         m_vsplitter;

    FOOTPRINT_PREVIEW_WIDGET* m_preview_ctrl;
    LIB_TREE*                 m_tree;

    PCB_BASE_FRAME*           m_parent;
    bool                      m_external_browser_requested;
};

#endif /* DIALOG_CHOOSE_FOOTPRINT_H */
