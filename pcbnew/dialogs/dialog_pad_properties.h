/**
 * @file dialog_pad_properties.h
 * @brief dialog pad properties editor.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxBasePcbFrame.h>
#include <base_units.h>
#include <wx/valnum.h>

#include <class_board.h>
#include <class_module.h>

#include <origin_viewitem.h>

#include <dialog_pad_properties_base.h>
#include <pcb_draw_panel_gal.h>



/**
 * class DIALOG_PAD_PROPERTIES, derived from DIALOG_PAD_PROPERTIES_BASE,
 * created by wxFormBuilder
 */
class DIALOG_PAD_PROPERTIES : public DIALOG_PAD_PROPERTIES_BASE
{
public:
    DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, D_PAD* aPad );
    void OnInitDialog( wxInitDialogEvent& event ) override;
    ~DIALOG_PAD_PROPERTIES()
    {
        delete m_dummyPad;
        delete m_axisOrigin;
    }

private:
    PCB_BASE_FRAME* m_parent;
    KIGFX::ORIGIN_VIEWITEM* m_axisOrigin;
    D_PAD*  m_currentPad;           // pad currently being edited
    D_PAD*  m_dummyPad;             // a working copy used to show changes
    D_PAD*  m_padMaster;            // The pad used to create new pads in board or
                                    // footprint editor
    BOARD*  m_board;                // the main board: this is the board handled by
                                    // the PCB editor, if running or the dummy
                                    // board used by the footprint editor
                                    // (could happen when the Footprint editor will be run
                                    // alone, outside the board editor
    bool    m_isFlipped;            // true if the parent footprint (therefore pads) is flipped (mirrored)
                                    // in this case, some Y coordinates values must be negated
    bool    m_canUpdate;
    bool    m_canEditNetName;       // true only if the caller is the board editor

    wxFloatingPointValidator<double>    m_OrientValidator;
    double  m_OrientValue;

private:
    void initValues();
    bool padValuesOK();       ///< test if all values are acceptable for the pad
    void redraw();
    void updateRoundRectCornerValues();

    /**
     * Function setPadLayersList
     * updates the CheckBox states in pad layers list,
     * @param layer_mask = pad layer mask (ORed layers bit mask)
     */
    void setPadLayersList( LSET layer_mask );

    /// Copy values from dialog field to aPad's members
    bool transferDataToPad( D_PAD* aPad );

    // event handlers:
    void OnResize( wxSizeEvent& event );

    void OnPadShapeSelection( wxCommandEvent& event ) override;
    void OnDrillShapeSelected( wxCommandEvent& event ) override;

    void PadOrientEvent( wxCommandEvent& event ) override;
    void PadTypeSelected( wxCommandEvent& event ) override;

    void OnSetLayers( wxCommandEvent& event ) override;
    void OnPaintShowPanel( wxPaintEvent& event ) override;

    // Called when corner setup value is changed for rounded rect pads
    void onCornerSizePercentChange( wxCommandEvent& event ) override;

    /// Called when a dimension has changed.
    /// Update the graphical pad shown in the panel.
    void OnValuesChanged( wxCommandEvent& event ) override;

    /// Updates the different parameters for the component being edited.
    /// Automatically fired from the OK button click.
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;
};

#endif      // #ifndef _DIALOG_PAD_PROPERTIES_H_
