/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _DIALOG_PAD_PROPERTIES_H_
#define _DIALOG_PAD_PROPERTIES_H_

#include <pcb_base_frame.h>
#include <wx/valnum.h>
#include <board.h>
#include <footprint.h>
#include <padstack.h>
#include <pcb_shape.h>
#include <origin_viewitem.h>
#include <dialog_pad_properties_base.h>
#include <widgets/text_ctrl_eval.h>
#include <pcb_draw_panel_gal.h>
#include <widgets/unit_binder.h>
#include <widgets/margin_offset_binder.h>

/**
 * DIALOG_PAD_PROPERTIES, derived from DIALOG_PAD_PROPERTIES_BASE,
 * created by wxFormBuilder
 */
// The wxWidgets window name. Used to retrieve the dialog by window name
#define PAD_PROPERTIES_DLG_NAME wxT( "pad_properties_dlg_name" )

class DIALOG_PAD_PROPERTIES : public DIALOG_PAD_PROPERTIES_BASE
{
public:
    DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, PAD* aPad );
    ~DIALOG_PAD_PROPERTIES();

private:
    void prepareCanvas();       // Initialize the canvases (legacy or gal) to display the pad
    void initValues();
    void initPadstackLayerValues();
    bool padValuesOK();         ///< test if all values are acceptable for the pad
    void redraw();
    void updateRoundRectCornerValues();
    void afterPadstackModeChanged();

    /**
     * Updates the CheckBox states in pad layers list, based on the layer_mask (if non-empty)
     * or the default layers for the current pad type.
     */
    void updatePadLayersList( LSET layer_mask, bool remove_unconnected, bool keep_top_bottom );

    /// Copy values from dialog field to aPad's members
    bool transferDataToPad( PAD* aPad );

    bool Show( bool aShow ) override;

    // event handlers:
    void OnInitDialog( wxInitDialogEvent& event ) override;
    void OnResize( wxSizeEvent& event );
	void OnCancel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void onTeardropsUpdateUi( wxUpdateUIEvent& event ) override;
    void OnPadstackModeChanged( wxCommandEvent& event ) override;
    void OnEditLayerChanged( wxCommandEvent& event ) override;

    void OnUpdateUINonCopperWarning( wxUpdateUIEvent& event ) override;

    void onBackDrillChoice( wxCommandEvent& event ) override;
    void onTopPostMachining( wxCommandEvent& event ) override;
    void onBottomPostMachining( wxCommandEvent& event ) override;

    void OnPadShapeSelection( wxCommandEvent& event ) override;
    void OnDrillShapeSelected( wxCommandEvent& event ) override;
	void onChangePadMode( wxCommandEvent& event ) override;
	void OnOffsetCheckbox( wxCommandEvent& event ) override;
	void OnPadToDieCheckbox( wxCommandEvent& event ) override;
    void OnPadToDieDelayCheckbox( wxCommandEvent& event ) override;

    void PadOrientEvent( wxCommandEvent& event ) override;
    void PadTypeSelected( wxCommandEvent& event ) override;

    void UpdateLayersDropdown();
    void OnSetCopperLayers( wxCommandEvent& event ) override;
    void OnSetLayers( wxCommandEvent& event ) override;

    // Called when corner setup value is changed for rounded rect pads
    void onCornerSizePercentChange( wxCommandEvent& event ) override;
    void onCornerRadiusChange( wxCommandEvent& event ) override;

    /// Called when a dimension has changed.
    /// Update the graphical pad shown in the panel.
    void OnValuesChanged( wxCommandEvent& event ) override;

    /// Updates the different parameters for the component being edited.
    /// Automatically fired from the OK button click.

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    /// Return the pad property currently selected
    PAD_PROP getSelectedProperty();

    // Show/hide the hole size Y widgets
    // Setting the X/Diameter label according to the selected hole type
    void updateHoleControls();

    // Show/hide the pad size Y widgets
    // Setting the X/Diameter label according to the selected hole type
    void updatePadSizeControls();

    void onModify( wxCommandEvent& aEvent ) override;
    void onModify( wxSpinDoubleEvent& aEvent ) override;

    // Return the largest chamfer ratio allowed by the current pad shape
    double getMaxChamferRatio() const;

    // Return the largest corner radius allowed by the current pad shape
    double getMaxCornerRadius() const;

    void updateAllowedPadChamferCorners();

private:
    PCB_BASE_FRAME* m_parent;
    PAD*            m_currentPad;       // pad currently being edited
    PAD*            m_previewPad;       // a working copy used to show changes
    PAD*            m_masterPad;        // pad used to create new pads in board or FP editor
    BOARD*          m_board;            // the main board: this is the board handled by the PCB
                                        //    editor or the dummy board used by the FP editor
    bool            m_initialized;
    bool            m_canEditNetName;   // true only if the caller is the board editor
    bool            m_isFpEditor;       // true if the caller is the footprint editor
    PCB_LAYER_ID    m_editLayer;        // Which copper layer of the padstack is being edited
    std::map<int, PCB_LAYER_ID> m_editLayerCtrlMap;

    std::vector<std::shared_ptr<PCB_SHAPE>> m_primitives;     // the custom shape primitives in
                                                              // local coords, orient 0
                                                              // must define a single copper area
    COLOR4D                  m_selectedColor; // color used to draw selected primitives when
                                              //     editing a custom pad shape

    std::vector<PCB_SHAPE*>  m_highlight;     // shapes highlighted in GAL mode
    PCB_DRAW_PANEL_GAL*      m_padPreviewGAL;
    KIGFX::ORIGIN_VIEWITEM*  m_axisOrigin;    // origin of the preview canvas

    static bool              m_sketchPreview; // session storage
    static int               m_page;          // remember the last open page during session


    UNIT_BINDER m_posX, m_posY;
    UNIT_BINDER m_sizeX, m_sizeY;
    UNIT_BINDER m_offsetX, m_offsetY;
    UNIT_BINDER m_padToDie;
    UNIT_BINDER m_padToDieDelay;
    UNIT_BINDER m_trapDelta;
    UNIT_BINDER m_cornerRadius;
    UNIT_BINDER m_cornerRatio;
    UNIT_BINDER m_chamferRatio;
    UNIT_BINDER m_mixedCornerRatio;
    UNIT_BINDER m_mixedChamferRatio;
    UNIT_BINDER m_holeX, m_holeY;
    UNIT_BINDER m_clearance;
    UNIT_BINDER m_maskMargin;
    MARGIN_OFFSET_BINDER m_pasteMargin;
    UNIT_BINDER m_thermalGap;
    UNIT_BINDER m_spokeWidth;
    UNIT_BINDER m_spokeAngle;
    UNIT_BINDER m_pad_orientation;
    UNIT_BINDER m_teardropMaxLenSetting;
    UNIT_BINDER m_teardropMaxHeightSetting;

    UNIT_BINDER m_topPostMachineSize1Binder;
    UNIT_BINDER m_topPostMachineSize2Binder;
    UNIT_BINDER m_bottomPostMachineSize1Binder;
    UNIT_BINDER m_bottomPostMachineSize2Binder;

    UNIT_BINDER m_backDrillTopSizeBinder;
    UNIT_BINDER m_backDrillBottomSizeBinder;
};


#endif      // #ifndef _DIALOG_PAD_PROPERTIES_H_
