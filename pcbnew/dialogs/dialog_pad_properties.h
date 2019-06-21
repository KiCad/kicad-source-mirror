/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcbnew.h>
#include <pcb_base_frame.h>
#include <base_units.h>
#include <wx/valnum.h>
#include <class_board.h>
#include <class_module.h>
#include <class_drawsegment.h>
#include <origin_viewitem.h>
#include <dialog_pad_properties_base.h>
#include <widgets/text_ctrl_eval.h>
#include <pcb_draw_panel_gal.h>
#include <widgets/unit_binder.h>

/**
 * class DIALOG_PAD_PROPERTIES, derived from DIALOG_PAD_PROPERTIES_BASE,
 * created by wxFormBuilder
 */
class DIALOG_PAD_PROPERTIES : public DIALOG_PAD_PROPERTIES_BASE
{
public:
    DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, D_PAD* aPad );
    ~DIALOG_PAD_PROPERTIES();

private:
    PCB_BASE_FRAME* m_parent;
    D_PAD*  m_currentPad;           // pad currently being edited
    D_PAD*  m_dummyPad;             // a working copy used to show changes
    D_PAD*  m_padMaster;            // pad used to create new pads in board or footprint editor
    BOARD*  m_board;                // the main board: this is the board handled by the PCB
                                    //    editor or the dummy board used by the footprint editor
    bool    m_isFlipped;            // indicates the parent footprint is flipped (mirrored) in
                                    //    which case some Y coordinates values must be negated
    bool    m_canUpdate;
    bool    m_canEditNetName;       // true only if the caller is the board editor

    std::vector<PAD_CS_PRIMITIVE> m_primitives;    // the list of custom shape primitives (basic
                                                   //     shapes), in local coords, orient 0
                                                   //     must define a single copper area
    COLOR4D                       m_selectedColor; // color used to draw selected primitives when
                                                   //     editing a custom pad shape

    std::vector<DRAWSEGMENT*>     m_highlight;     // shapes highlighted in GAL mode
    KIGFX::ORIGIN_VIEWITEM*       m_axisOrigin;    // origin of the preview canvas
    static bool                   m_sketchPreview; // session storage

    UNIT_BINDER m_posX, m_posY;
    UNIT_BINDER m_sizeX, m_sizeY;
    UNIT_BINDER m_offsetX, m_offsetY;
    UNIT_BINDER m_padToDie;
    UNIT_BINDER m_trapDelta;
    UNIT_BINDER m_cornerRadius;
    UNIT_BINDER m_holeX, m_holeY;
    wxFloatingPointValidator<double>    m_OrientValidator;
    double      m_OrientValue;
    UNIT_BINDER m_clearance;
    UNIT_BINDER m_maskClearance, m_pasteClearance;
    UNIT_BINDER m_spokeWidth, m_thermalGap;

private:
    void prepareCanvas();       // Initialize the canvases (legacy or gal) to display the pad
    void initValues();
    void displayPrimitivesList();
    bool padValuesOK();         ///< test if all values are acceptable for the pad
    void redraw();
    void editPrimitive();
    void updateRoundRectCornerValues();
    void enablePrimitivePage( bool aEnable );   ///< enable (or disable) the primitive page editor

    /**
     * Function setPadLayersList
     * updates the CheckBox states in pad layers list,
     * @param layer_mask = pad layer mask (ORed layers bit mask)
     */
    void setPadLayersList( LSET layer_mask );

    /// Copy values from dialog field to aPad's members
    bool transferDataToPad( D_PAD* aPad );

    // event handlers:
    void OnInitDialog( wxInitDialogEvent& event ) override;
    void OnResize( wxSizeEvent& event );
	void OnCancel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void OnUpdateUINonCopperWarning( wxUpdateUIEvent& event ) override
    {
        bool isOnCopperLayer = ( m_dummyPad->GetLayerSet() & LSET::AllCuMask() ).any();
        m_nonCopperWarningBook->SetSelection( isOnCopperLayer ? 0 : 1 );
    }

    void OnPadShapeSelection( wxCommandEvent& event ) override;
    void OnDrillShapeSelected( wxCommandEvent& event ) override;
	void onChangePadMode( wxCommandEvent& event ) override;

    void PadOrientEvent( wxCommandEvent& event ) override;
    void PadTypeSelected( wxCommandEvent& event ) override;

    void OnSetLayers( wxCommandEvent& event ) override;
    void OnPaintShowPanel( wxPaintEvent& event ) override;

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

    /// Event handlers of basic shapes list panel
    void onDeletePrimitive( wxCommandEvent& event ) override;
    void onEditPrimitive( wxCommandEvent& event ) override;
    void onAddPrimitive( wxCommandEvent& event ) override;
    void onGeometryTransform( wxCommandEvent& event ) override;
    void onDuplicatePrimitive( wxCommandEvent& event ) override;

    /// Called on a double click on the basic shapes list
    void onPrimitiveDClick( wxMouseEvent& event ) override;
    /// Called on selection/deselection of a basic shape
	void OnPrimitiveSelection( wxListEvent& event ) override;
};

/**
 * a dialog to edit basics shapes parameters.
 * Polygonal shape is not handles by this dialog
 */
class DIALOG_PAD_PRIMITIVES_PROPERTIES: public DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE
{
public:
    DIALOG_PAD_PRIMITIVES_PROPERTIES( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                      PAD_CS_PRIMITIVE * aShape );

    /**
     * Function TransferDataFromWindow
     * Transfer data out of the GUI.
     */
    bool TransferDataFromWindow() override;

private:
    /**
     * Function TransferDataToWindow
     * Transfer data into the GUI.
     */
    bool TransferDataToWindow() override;

    // The basic shape currently edited
    PAD_CS_PRIMITIVE * m_shape;

    UNIT_BINDER        m_startX;
    UNIT_BINDER        m_startY;
    UNIT_BINDER        m_ctrl1X;
    UNIT_BINDER        m_ctrl1Y;
    UNIT_BINDER        m_ctrl2X;
    UNIT_BINDER        m_ctrl2Y;
    UNIT_BINDER        m_endX;
    UNIT_BINDER        m_endY;
    UNIT_BINDER        m_radius;
    UNIT_BINDER        m_thickness;
};


/**
 * a dialog to edit basic polygonal shape parameters
 */
class DIALOG_PAD_PRIMITIVE_POLY_PROPS: public DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE
{
    // The basic shape currently edited
    PAD_CS_PRIMITIVE * m_shape;

    // The working copy of the basic shape currently edited
    PAD_CS_PRIMITIVE m_currshape;

    UNIT_BINDER      m_thickness;

public:
    DIALOG_PAD_PRIMITIVE_POLY_PROPS( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                     PAD_CS_PRIMITIVE * aShape );
    ~DIALOG_PAD_PRIMITIVE_POLY_PROPS();

    /**
     * Function TransferDataFromWindow
     * Transfer data out of the GUI.
     */
    bool TransferDataFromWindow() override;

private:
    /**
     * Function TransferDataToWindow
     * Transfer data into the GUI.
     */
    bool TransferDataToWindow() override;

    /**
     * test for a valid polygon (a not self intersectiong polygon)
     */
    bool Validate() override;

    // Events handlers:
    void OnValidateButton( wxCommandEvent& event );
    void OnButtonAdd( wxCommandEvent& event ) override;
    void OnButtonDelete( wxCommandEvent& event ) override;
    void onPaintPolyPanel( wxPaintEvent& event ) override;
    void onPolyPanelResize( wxSizeEvent& event ) override;
    void onGridSelect( wxGridRangeSelectEvent& event ) override;
    void onCellChanging( wxGridEvent& event );
    void onCellSelect( wxGridEvent& event ) override
    {
        event.Skip();
    }

    bool doValidate( bool aRemoveRedundantCorners );

};


/** A dialog to apply geometry transforms to a shape or set of shapes
 * (move, rotate around origin, scaling factor, duplication).
 * shapes are scaled, then moved then rotated.
 * aList is a list of shapes to transform or duplicate
 * if aShowDuplicate == false, the parameter "Duplicate count" is disabled
 */

class DIALOG_PAD_PRIMITIVES_TRANSFORM : public DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE
{
public:
    DIALOG_PAD_PRIMITIVES_TRANSFORM( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                     std::vector<PAD_CS_PRIMITIVE*>& aList, bool aShowDuplicate );

    /**
     * Apply geometric transform (rotation, move, scale) defined in dialog
     * aDuplicate = 1 .. n to duplicate the list of shapes
     * aDuplicate = 0 to transform the list of shapes
     * The duplicated items are transformed, but the initial shpes are not modified.
     * The duplicated items are added to aList
     */
    void Transform( std::vector<PAD_CS_PRIMITIVE>* aList = NULL, int aDuplicateCount = 0 );

    /**
     * @return the number of duplicate, chosen by user
     */
    int GetDuplicateCount() { return m_spinCtrlDuplicateCount->GetValue(); }

private:
    std::vector<PAD_CS_PRIMITIVE*>& m_list;

    UNIT_BINDER  m_vectorX;
    UNIT_BINDER  m_vectorY;
    UNIT_BINDER  m_rotation;
};

#endif      // #ifndef _DIALOG_PAD_PROPERTIES_H_
