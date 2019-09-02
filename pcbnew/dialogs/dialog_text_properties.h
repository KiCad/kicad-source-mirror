/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_TEXT_PROPERTIES_H
#define DIALOG_TEXT_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <wx/valnum.h>

#include <dialog_text_properties_base.h>


class PCB_BASE_EDIT_FRAME;
class BOARD_ITEM;
class EDA_TEXT;
class TEXTE_MODULE;
class TEXTE_PCB;


class DIALOG_TEXT_PROPERTIES : public DIALOG_TEXT_PROPERTIES_BASE
{
public:
    DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem );
    ~DIALOG_TEXT_PROPERTIES();

private:
    PCB_BASE_EDIT_FRAME* m_Parent;
    BOARD_ITEM*          m_item;        // TEXTE_MODULE, TEXTE_PCB, or DIMENSION
    EDA_TEXT*            m_edaText;     // always non-null
    TEXTE_MODULE*        m_modText;     // only non-null for TEXTE_MODULEs
    TEXTE_PCB*           m_pcbText;     // only non-null for TEXTE_PCBs and DIMENSIONs

    UNIT_BINDER          m_textWidth;
    UNIT_BINDER          m_textHeight;
    UNIT_BINDER          m_thickness;
    UNIT_BINDER          m_posX;
    UNIT_BINDER          m_posY;
    UNIT_BINDER          m_linesThickness;  // lines thikness for dimension graphic

    wxFloatingPointValidator<double>    m_OrientValidator;
    double               m_OrientValue;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnCharHook( wxKeyEvent& aEvent );
    void OnDimensionTextChange( wxCommandEvent& event ) override;
    void OnDimensionUnitsChange( wxCommandEvent& event ) override;
};


#endif //DIALOG_TEXT_PROPERTIES_H
