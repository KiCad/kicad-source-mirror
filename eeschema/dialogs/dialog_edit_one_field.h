
#ifndef DIALOG_EDIT_ONE_FIELD_H_
#define DIALOG_EDIT_ONE_FIELD_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_lib_edit_text_base.h>
#include <widgets/unit_binder.h>
#include <lib_field.h>
#include <template_fieldnames.h>

class SCH_BASE_FRAME;
class SCH_FIELD;
class EDA_TEXT;
class SCINTILLA_TRICKS;


/**
 * A base class to edit schematic and symbol library fields.
 *
 * This class is setup in expectation of its children possibly using Kiway player so
 * #DIALOG_SHIM::ShowQuasiModal is required when calling any subclasses.
 */
class DIALOG_EDIT_ONE_FIELD : public DIALOG_LIB_EDIT_TEXT_BASE
{
public:
    DIALOG_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                           const EDA_TEXT* aTextItem );

    ~DIALOG_EDIT_ONE_FIELD() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    SCH_BASE_FRAME* GetParent() { return dynamic_cast< SCH_BASE_FRAME* >( wxDialog::GetParent() ); }

    const wxString& GetText() const { return m_text; }

protected:
    void init();

    void updateText( EDA_TEXT* aText );

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
    virtual void OnSetFocusText( wxFocusEvent& event ) override;

    UNIT_BINDER m_posX;
    UNIT_BINDER m_posY;
    UNIT_BINDER m_textSize;

    int         m_fieldId;
    bool        m_isPower;
    wxString    m_text;
    bool        m_isItalic;
    bool        m_isBold;
    wxPoint     m_position;
    int         m_size;
    bool        m_isVertical;
    int         m_verticalJustification;
    int         m_horizontalJustification;
    bool        m_isVisible;

    bool        m_firstFocus;

    SCINTILLA_TRICKS* m_scintillaTricks;
};


/**
 * Handle editing a single symbol field in the library editor.
 *
 * @note Use ShowQuasiModal when calling this class!
 */
class DIALOG_LIB_EDIT_ONE_FIELD : public DIALOG_EDIT_ONE_FIELD
{
public:
    DIALOG_LIB_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                               const LIB_FIELD* aField );

    ~DIALOG_LIB_EDIT_ONE_FIELD() {}

    void UpdateField( LIB_FIELD* aField )
    {
        aField->SetText( m_text );

        // VALUE === symbol name, so update the parent symbol if it changes.
        if( aField->GetId() == VALUE_FIELD && aField->GetParent() )
            aField->GetParent()->SetName( m_text );

        updateText( aField );
    }
};


/**
 * Handle editing a single symbol field in the schematic editor.
 *
 * @note Use ShowQuasiModal when calling this class!
 */
class DIALOG_SCH_EDIT_ONE_FIELD : public DIALOG_EDIT_ONE_FIELD
{
public:
    DIALOG_SCH_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                               const SCH_FIELD* aField );

    ~DIALOG_SCH_EDIT_ONE_FIELD() {}

    void onScintillaCharAdded( wxStyledTextEvent &aEvent );

    void UpdateField( SCH_FIELD* aField, SCH_SHEET_PATH* aSheetPath );

private:
    const SCH_FIELD* m_field;
};

#endif    // DIALOG_EDIT_ONE_FIELD_H_
