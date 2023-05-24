/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_TEXTBOX_PROPERTIES_H
#define DIALOG_TEXTBOX_PROPERTIES_H

#include <widgets/unit_binder.h>
#include <wx/valnum.h>

#include <dialog_textbox_properties_base.h>


class PCB_BASE_EDIT_FRAME;
class BOARD_ITEM;
class EDA_TEXT;
class FP_TEXTBOX;
class PCB_TEXTBOX;
class SCINTILLA_TRICKS;


class DIALOG_TEXTBOX_PROPERTIES : public DIALOG_TEXTBOX_PROPERTIES_BASE
{
public:
    DIALOG_TEXTBOX_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem );
    ~DIALOG_TEXTBOX_PROPERTIES();

private:
    void onFontSelected( wxCommandEvent &aEvent ) override;
    void onBoldToggle( wxCommandEvent &aEvent ) override;
    void onAlignButton( wxCommandEvent &aEvent ) override;
    void onThickness( wxCommandEvent &aEvent ) override;
    void onBorderChecked( wxCommandEvent& event ) override;
    void onScintillaCharAdded( wxStyledTextEvent &aEvent );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void onMultiLineTCLostFocus( wxFocusEvent& event ) override;

private:
    PCB_BASE_EDIT_FRAME* m_frame;
    BOARD_ITEM*          m_item;           // FP_TEXTBOX or PCB_TEXTBOX
    EDA_TEXT*            m_edaText;        // always non-null
    FP_TEXTBOX*          m_fpTextBox;      // only non-null for FP_TEXTBOXes
    PCB_TEXTBOX*         m_pcbTextBox;     // only non-null for PCB_TEXTBOXes

    UNIT_BINDER          m_textWidth;
    UNIT_BINDER          m_textHeight;
    UNIT_BINDER          m_thickness;
    UNIT_BINDER          m_orientation;     // rotation in degrees
    UNIT_BINDER          m_borderWidth;

    SCINTILLA_TRICKS*    m_scintillaTricks;
};


#endif //DIALOG_TEXTBOX_PROPERTIES_H
