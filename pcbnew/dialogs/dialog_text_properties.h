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

#ifndef DIALOG_TEXT_PROPERTIES_H
#define DIALOG_TEXT_PROPERTIES_H

#include <wx/hyperlink.h>
#include <widgets/unit_binder.h>
#include <wx/valnum.h>

#include <dialog_text_properties_base.h>


class PCB_BASE_EDIT_FRAME;
class BOARD_ITEM;
class EDA_TEXT;
class PCB_TEXT;
class SCINTILLA_TRICKS;
class HTML_MESSAGE_BOX;
class wxHyperlinkCtrl;


class DIALOG_TEXT_PROPERTIES : public DIALOG_TEXT_PROPERTIES_BASE
{
public:
    DIALOG_TEXT_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_TEXT* aText );
    ~DIALOG_TEXT_PROPERTIES();

    /**
     * Used to select the variant part of some text fields (for instance, the question mark
     * or number in a reference).
     * @param event
     */
    virtual void OnSetFocusText( wxFocusEvent& event ) override;

private:
    void onFontSelected( wxCommandEvent& aEvent ) override;
    void onBoldToggle( wxCommandEvent& aEvent ) override;
    void onAlignButton( wxCommandEvent& aEvent ) override;
    void onValignButton( wxCommandEvent& aEvent ) override;
    void onThickness( wxCommandEvent& aEvent ) override;
    void onTextSize( wxCommandEvent& aEvent ) override;
    void onAutoTextThickness( wxCommandEvent& aEvent ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void onMultiLineTCLostFocus( wxFocusEvent& event ) override;

    void onSyntaxHelp( wxHyperlinkEvent& aEvent );

private:
    PCB_BASE_EDIT_FRAME* m_frame;
    PCB_TEXT*            m_item;

    UNIT_BINDER m_textWidth;
    UNIT_BINDER m_textHeight;
    UNIT_BINDER m_thickness;
    UNIT_BINDER m_posX;
    UNIT_BINDER m_posY;
    UNIT_BINDER m_orientation; // rotation in degrees

    SCINTILLA_TRICKS* m_scintillaTricks;
    wxHyperlinkCtrl*  m_syntaxHelp;
    HTML_MESSAGE_BOX* m_helpWindow;
};


#endif //DIALOG_TEXT_PROPERTIES_H
