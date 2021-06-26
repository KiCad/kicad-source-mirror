/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see change_log.txt for contributors.
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


#ifndef PANEL_SETUP_LAYERS_H
#define PANEL_SETUP_LAYERS_H

#include <widgets/unit_binder.h>
#include <widgets/paged_dialog.h>
#include <panel_setup_layers_base.h>

class PCB_EDIT_FRAME;
class BOARD;
class BOARD_DESIGN_SETTINGS;
class PANEL_SETUP_BOARD_STACKUP;


/**
 * The 3 UI control pointers for a single board layer.
 */
struct PANEL_SETUP_LAYERS_CTLs
{
    PANEL_SETUP_LAYERS_CTLs( wxControl* aName, wxCheckBox* aCheckBox, wxControl* aChoiceOrDesc )
    {
        name     = aName;
        checkbox = aCheckBox;
        choice   = aChoiceOrDesc;
    }

    wxControl*      name;
    wxCheckBox*     checkbox;
    wxControl*      choice;
};


class PANEL_SETUP_LAYERS : public PANEL_SETUP_LAYERS_BASE
{
public:
    PANEL_SETUP_LAYERS( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame );

    void ImportSettingsFrom( BOARD* aBoard );

    /**
     * Check and warn if inner copper layers will be deleted
     *
     * This function warns users if they are going to delete inner copper layers because
     * they're importing settings from a board with less copper layers than the board
     * already loaded.
     *
     * @param aWorkingBoard is the currently loaded PCB
     * @param aImportedBoard is the PCB imported to get settings from.
     * @return Approval to delete inner copper if needed.
     */
    bool CheckCopperLayerCount( BOARD* aWorkingBoard, BOARD* aImportedBoard );

    ///< @return the selected layer mask within the UI checkboxes
    LSET GetUILayerMask();

    ///< @return the layer name within the UI wxTextCtrl
    wxString GetLayerName( LAYER_NUM layer );

    /**
     * Called when switching to this tab to make sure that any changes to the copper layer count
     * made on the physical stackup page are reflected here
     * @param aNumCopperLayers is the number of copper layers in the board
     */
    void SyncCopperLayers( int aNumCopperLayers );

    void SetPhysicalStackupPanel( PANEL_SETUP_BOARD_STACKUP* aPanel )
    {
        m_physicalStackup = aPanel;
    }

private:

    void setLayerCheckBox( LAYER_NUM layer, bool isChecked );
    void setCopperLayerCheckBoxes( int copperCount );
    void setMandatoryLayerCheckBoxes();
    void setUserDefinedLayerCheckBoxes();

    void showBoardLayerNames();
    void showSelectedLayerCheckBoxes( LSET enableLayerMask );
    void showLayerTypes();

    int getLayerTypeIndex( LAYER_NUM layer );

    void OnCheckBox( wxCommandEvent& event ) override;
    void DenyChangeCheckBox( wxCommandEvent& event ) override;
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    virtual void addUserDefinedLayer( wxCommandEvent& aEvent ) override;

    bool testLayerNames();

    /**
     * Return a list of layers removed from the board that contain items.
     */
    LSEQ getRemovedLayersWithItems();

    /**
     * Return a list of layers in use in footprints, and therefore not removable.
     */
    LSEQ getNonRemovableLayers();

    PANEL_SETUP_LAYERS_CTLs getCTLs( LAYER_NUM aLayerNumber );
    wxControl*  getName( LAYER_NUM aLayer );
    wxCheckBox* getCheckBox( LAYER_NUM aLayer );
    wxChoice*   getChoice( LAYER_NUM aLayer );

    PAGED_DIALOG*              m_parentDialog;
    PCB_EDIT_FRAME*            m_frame;
    PANEL_SETUP_BOARD_STACKUP* m_physicalStackup;
    BOARD*                     m_pcb;
    LSET                       m_enabledLayers;
};



#endif //PANEL_SETUP_LAYERS_H


