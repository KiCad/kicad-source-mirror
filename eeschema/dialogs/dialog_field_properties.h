/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
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

#ifndef DIALOG_FIELD_PROPERTIES_H
#define DIALOG_FIELD_PROPERTIES_H

#include <dialog_field_properties_base.h>
#include <widgets/unit_binder.h>

class SCH_BASE_FRAME;
class SCH_FIELD;
class EDA_TEXT;
class SCINTILLA_TRICKS;
class SCH_COMMIT;


/**
 * This class is setup in expectation of its children possibly using Kiway player so
 * #DIALOG_SHIM::ShowQuasiModal is required when calling any subclasses.
 */
class DIALOG_FIELD_PROPERTIES : public DIALOG_FIELD_PROPERTIES_BASE
{
public:
    DIALOG_FIELD_PROPERTIES( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                             const SCH_FIELD* aField );

    ~DIALOG_FIELD_PROPERTIES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    SCH_BASE_FRAME* GetParent() { return dynamic_cast<SCH_BASE_FRAME*>( wxDialog::GetParent() ); }

    const wxString& GetText() const { return m_text; }

    void UpdateField( SCH_FIELD* aField );

    void UpdateField( SCH_COMMIT* aCommit, SCH_FIELD* aField, SCH_SHEET_PATH* aSheetPath );

protected:
    void init();

    void updateText( SCH_FIELD* aField );

    void onScintillaCharAdded( wxStyledTextEvent &aEvent );

    /**
     * Handle the select button next to the text value field. The current assumption
     * is that this event will only be enabled for footprint type fields. In the future
     * this function may need to be moved to the subclasses to access m_field and check for
     * the field type if more select actions are desired.
     *
     * @param aEvent is the wX event thrown when the button is clicked, this isn't used
     */
    void OnTextValueSelectButtonClick( wxCommandEvent& aEvent ) override;

    /**
     * Used to select the variant part of some text fields (for instance, the question mark
     * or number in a reference).
     */
    void OnSetFocusText( wxFocusEvent& event ) override;

    void onOrientButton( wxCommandEvent &aEvent );
    void onHAlignButton( wxCommandEvent &aEvent );
    void onVAlignButton( wxCommandEvent &aEvent );

protected:
    UNIT_BINDER       m_posX;
    UNIT_BINDER       m_posY;
    UNIT_BINDER       m_textSize;

    FIELD_T           m_fieldId;
    wxString          m_text;
    KIFONT::FONT*     m_font;
    bool              m_isItalic;
    bool              m_isBold;
    KIGFX::COLOR4D    m_color;
    VECTOR2I          m_position;
    int               m_size;
    bool              m_isVertical;
    GR_TEXT_V_ALIGN_T m_verticalJustification;
    GR_TEXT_H_ALIGN_T m_horizontalJustification;
    bool              m_isVisible;
    bool              m_isNameVisible;
    bool              m_allowAutoplace;

    bool              m_firstFocus;

    SCINTILLA_TRICKS* m_scintillaTricks;
    std::string       m_netlist;

    const SCH_FIELD*  m_field;
    bool              m_isSheetFilename;
};

#endif    // DIALOG_FIELD_PROPERTIES_H
