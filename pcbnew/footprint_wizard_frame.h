/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo Pelayo, miguelangel@nbee.es
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file footprint_wizard_frame.h
 */

#ifndef FOOTPRINT_WIZARD_FRAME_H_
#define FOOTPRINT_WIZARD_FRAME_H_

#include <memory>
#include <wx/gdicmn.h>
#include <footprint_wizard.h>
#include <pcb_base_edit_frame.h>
#include <nlohmann/json_fwd.hpp>

class wxSashLayoutWindow;
class wxSashEvent;
class FOOTPRINT_EDIT_FRAME;
class FOOTPRINT_WIZARD_PROPERTIES_PANEL;

class FOOTPRINT_WIZARD_FRAME : public PCB_BASE_EDIT_FRAME
{
public:
    FOOTPRINT_WIZARD_FRAME( KIWAY* aKiway, wxWindow* parent, FRAME_T aFrameType );

    ~FOOTPRINT_WIZARD_FRAME();

    ///< @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    FOOTPRINT* GetBuiltFootprint();

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    void SelectCurrentWizard( wxCommandEvent& aDummy ); // Open the wizard selector dialog

    void DefaultParameters();           // Reset the initial (default) values of the wizard prms

    /**
     * Will let the caller exit from the wait loop, and get the built footprint.
     */
    void ExportSelectedFootprint( wxCommandEvent& aEvent );

    void RebuildWizardParameters();
    void OnWizardParametersChanged();

    FOOTPRINT_WIZARD_MANAGER* Manager() const { return m_wizardManager.get(); }

private:
    void                OnSize( wxSizeEvent& event ) override;

    /**
     * Redraw the message panel.
     *
     * Display the current footprint info, or clear the message panel if nothing is loaded.
     */
    void UpdateMsgPanel() override;

    /**
     * Rebuild the GAL view (reint tool manager, colors and drawings) must be run after any
     * footprint change.
     */
    void updateView();

    /**
     * Resize the child windows when dragging a sash window border.
     */
    void OnSashDrag( wxSashEvent& event );

    /**
     * Create the list of parameters for the current page.
     */
    void ReCreateParameterList();

    /**
     * Prepare the grid where parameters are displayed.
     */
    /**
     * Show the list of footprint wizards available into the system.
     */
    void SelectFootprintWizard();

    /**
     * Regenerate the current footprint.
     */
    void RegenerateFootprint();

    /**
     * Reloads the wizard by name.
     */
    FOOTPRINT_WIZARD* GetMyWizard();

    /**
     * Show all the details about the current wizard.
     */
    void DisplayWizardInfos();

    void doCloseWindow() override;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );


    /// @copydoc PCB_BASE_FRAME::Update3DView
    void Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle = nullptr ) override;

    DECLARE_EVENT_TABLE()

protected:
    FOOTPRINT_WIZARD* m_currentWizard;
    wxString        m_wizardStatus;         ///< current wizard status

private:
    FOOTPRINT_WIZARD_PROPERTIES_PANEL* m_parametersPanel; ///< Panel for the parameter grid
    wxTextCtrl*     m_buildMessageBox;

    wxString        m_auiPerspective;       ///< Encoded string describing the AUI layout
    std::unique_ptr<nlohmann::json> m_viewerAuiState;

    bool            m_wizardListShown;      ///< A show-once flag for the wizard list

    std::unique_ptr<FOOTPRINT_WIZARD_MANAGER> m_wizardManager;
};



#endif    // FOOTPRINT_WIZARD_FRM_H_
