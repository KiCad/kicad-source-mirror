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

#ifndef _DIALOG_BARCODE_PROPERTIES_H_
#define _DIALOG_BARCODE_PROPERTIES_H_

#include <base_units.h>
#include <board.h>
#include <dialog_barcode_properties_base.h>
#include <origin_viewitem.h>
#include <pcb_base_frame.h>
#include <pcb_draw_panel_gal.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/unit_binder.h>
#include <wx/valnum.h>

class PCB_BARCODE;

/**
 * DIALOG_BARCODE_PROPERTIES, derived from DIALOG_BARCODE_PROPERTIES_BASE,
 * created by wxFormBuilder
 */
class DIALOG_BARCODE_PROPERTIES : public DIALOG_BARCODE_PROPERTIES_BASE
{
public:
    DIALOG_BARCODE_PROPERTIES( PCB_BASE_FRAME* aParent, PCB_BARCODE* aBarcode );
    ~DIALOG_BARCODE_PROPERTIES();

private:
    PCB_BASE_FRAME* m_parent;
    PCB_BARCODE*    m_currentBarcode; // barcode currently being edited
    PCB_BARCODE*    m_dummyBarcode;   // a working copy used to show changes
    BOARD*          m_board;          // the main board: this is the board handled by the PCB

    KIGFX::ORIGIN_VIEWITEM* m_axisOrigin; // origin of the preview canvas

private:
    void prepareCanvas(); // Initialize the canvases (legacy or gal) to display the barcode
    void initValues();

    /// Copy values from dialog field to aBarcode's members
    bool transferDataToBarcode( PCB_BARCODE* aBarcode );

    // event handlers:
    void OnInitDialog( wxInitDialogEvent& event ) override;
    void OnResize( wxSizeEvent& event );
    void OnCancel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    /// Update the graphical barcode shown in the panel.
    void OnValuesChanged( wxCommandEvent& event ) override;


    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;
};

#endif // #ifndef _DIALOG_BARCODE_PROPERTIES_H_
