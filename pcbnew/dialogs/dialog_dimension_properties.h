/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_DIALOG_DIMENSION_PROPERTIES_H
#define KICAD_DIALOG_DIMENSION_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <wx/valnum.h>

#include "dialog_dimension_properties_base.h"


class BOARD_ITEM;
class PCB_DIMENSION_BASE;
class PCB_BASE_EDIT_FRAME;


class DIALOG_DIMENSION_PROPERTIES : public DIALOG_DIMENSION_PROPERTIES_BASE
{
public:
    DIALOG_DIMENSION_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem );

    ~DIALOG_DIMENSION_PROPERTIES();

protected:
    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

private:
    void onFontSelected( wxCommandEvent &aEvent ) override;
    void onBoldToggle( wxCommandEvent &aEvent ) override;
    void onAlignButton( wxCommandEvent &aEvent ) override;
    void onThickness( wxCommandEvent &aEvent ) override;

    void updateDimensionFromDialog( PCB_DIMENSION_BASE* aTarget );

    void updatePreviewText();

private:
    PCB_BASE_EDIT_FRAME*    m_frame;

    PCB_DIMENSION_BASE*     m_dimension;
    PCB_DIMENSION_BASE*     m_previewDimension;

    PCB_LAYER_BOX_SELECTOR* m_cbLayerActual;       // The active layer box control
    wxTextCtrl*             m_txtValueActual;      // The active value control

    UNIT_BINDER             m_textWidth;
    UNIT_BINDER             m_textHeight;
    UNIT_BINDER             m_textThickness;
    UNIT_BINDER             m_textPosX;
    UNIT_BINDER             m_textPosY;
    UNIT_BINDER             m_orientation;         // rotation in degrees
    UNIT_BINDER             m_lineThickness;
    UNIT_BINDER             m_arrowLength;
    UNIT_BINDER             m_extensionOffset;
    UNIT_BINDER             m_extensionOvershoot;
};


#endif // KICAD_DIALOG_DIMENSION_PROPERTIES_H
