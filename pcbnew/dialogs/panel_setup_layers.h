/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
    PANEL_SETUP_LAYERS_CTLs() : name( nullptr ), checkbox( nullptr ), choice( nullptr ) {}
    PANEL_SETUP_LAYERS_CTLs( wxTextCtrl* aName, wxCheckBox* aCheckBox, wxControl* aChoiceOrDesc )
    {
        name     = aName;
        checkbox = aCheckBox;
        choice   = aChoiceOrDesc;
    }

    wxTextCtrl*      name;
    wxCheckBox*     checkbox;
    wxControl*      choice;
};


class PANEL_SETUP_LAYERS : public PANEL_SETUP_LAYERS_BASE
{
public:
    PANEL_SETUP_LAYERS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );

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

    bool IsInitialized() const { return m_initialized; }

private:
    void setLayerCheckBox( PCB_LAYER_ID layer, bool isChecked );
    void setCopperLayerCheckBoxes( int copperCount );
    void setUserDefinedLayerCheckBoxes();

    void showBoardLayerNames();
    void showSelectedLayerCheckBoxes( const LSET& enableLayerMask );
    void showLayerTypes();
    bool transferDataFromWindow();

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    virtual void addUserDefinedLayer( wxCommandEvent& aEvent ) override;

    bool testLayerNames();

    /**
     * Return a list of layers removed from the board that contain items.
     * Footprints and items owned by footprints are not taken in account
     */
    LSEQ getRemovedLayersWithItems();

    /**
     * Return a list of layers in use in footprints, and therefore not removable.
     */
    LSEQ getNonRemovableLayers();

    wxTextCtrl* getName( PCB_LAYER_ID aLayer );
    wxCheckBox* getCheckBox( PCB_LAYER_ID aLayer );
    wxChoice* getChoice( PCB_LAYER_ID aLayer );

    void initialize_front_tech_layers();
    void initialize_layers_controls();
    void initialize_back_tech_layers();

    void append_user_layer( PCB_LAYER_ID aLayer );

private:
    PCB_EDIT_FRAME*            m_frame;
    PANEL_SETUP_BOARD_STACKUP* m_physicalStackup;
    BOARD*                     m_pcb;
    LSET                       m_enabledLayers;
    bool                       m_initialized;

    std::map<PCB_LAYER_ID, PANEL_SETUP_LAYERS_CTLs> m_layersControls;


    wxCheckBox*   m_CrtYdFrontCheckBox;
    wxTextCtrl*   m_CrtYdFrontName;
    wxStaticText* m_CrtYdFrontStaticText;

    wxCheckBox*   m_FabFrontCheckBox;
    wxTextCtrl*   m_FabFrontName;
    wxStaticText* m_FabFrontStaticText;

    wxCheckBox*   m_AdhesFrontCheckBox;
    wxTextCtrl*   m_AdhesFrontName;
    wxStaticText* m_AdhesFrontStaticText;

    wxCheckBox*   m_SoldPFrontCheckBox;
    wxTextCtrl*   m_SoldPFrontName;
    wxStaticText* m_SoldPFrontStaticText;

    wxCheckBox*   m_SilkSFrontCheckBox;
    wxTextCtrl*   m_SilkSFrontName;
    wxStaticText* m_SilkSFrontStaticText;

    wxCheckBox*   m_MaskFrontCheckBox;
    wxTextCtrl*   m_MaskFrontName;
    wxStaticText* m_MaskFrontStaticText;

    wxCheckBox*   m_MaskBackCheckBox;
    wxTextCtrl*   m_MaskBackName;
    wxStaticText* m_MaskBackStaticText;

    wxCheckBox*   m_SilkSBackCheckBox;
    wxTextCtrl*   m_SilkSBackName;
    wxStaticText* m_SilkSBackStaticText;

    wxCheckBox*   m_SoldPBackCheckBox;
    wxTextCtrl*   m_SoldPBackName;
    wxStaticText* m_SoldPBackStaticText;

    wxCheckBox*   m_AdhesBackCheckBox;
    wxTextCtrl*   m_AdhesBackName;
    wxStaticText* m_AdhesBackStaticText;

    wxCheckBox*   m_FabBackCheckBox;
    wxTextCtrl*   m_FabBackName;
    wxStaticText* m_FabBackStaticText;

    wxCheckBox*   m_CrtYdBackCheckBox;
    wxTextCtrl*   m_CrtYdBackName;
    wxStaticText* m_CrtYdBackStaticText;

    wxCheckBox*   m_PCBEdgesCheckBox;
    wxTextCtrl*   m_PCBEdgesName;
    wxStaticText* m_PCBEdgesStaticText;

    wxCheckBox*   m_MarginCheckBox;
    wxTextCtrl*   m_MarginName;
    wxStaticText* m_MarginStaticText;

    wxCheckBox*   m_Eco1CheckBox;
    wxTextCtrl*   m_Eco1Name;
    wxStaticText* m_Eco1StaticText;

    wxCheckBox*   m_Eco2CheckBox;
    wxTextCtrl*   m_Eco2Name;
    wxStaticText* m_Eco2StaticText;

    wxCheckBox*   m_CommentsCheckBox;
    wxTextCtrl*   m_CommentsName;
    wxStaticText* m_CommentsStaticText;

    wxCheckBox*   m_DrawingsCheckBox;
    wxTextCtrl*   m_DrawingsName;
    wxStaticText* m_DrawingsStaticText;
};



#endif //PANEL_SETUP_LAYERS_H


