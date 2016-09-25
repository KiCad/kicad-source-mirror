
#ifndef DIALOG_EDIT_ONE_FIELD_H_
#define DIALOG_EDIT_ONE_FIELD_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2016 Wayne Stambaugh, stambaughw@gmail.com
 * Copyright (C) 2004-2016 KiCad Developers, see change_log.txt for contributors.
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

class SCH_BASE_FRAME;
class LIB_FIELD;
class SCH_FIELD;
class EDA_TEXT;


/**
 * Class DIALOG_EDIT_ONE_FIELD
 * is a base class to edit schematic and component library fields.
 * <p>
 * This class is setup in expectation of its children
 * possibly using Kiway player so ShowQuasiModal is required when calling
 * any subclasses.
 *</p>
 */
class DIALOG_EDIT_ONE_FIELD : public DIALOG_LIB_EDIT_TEXT_BASE
{
public:
    DIALOG_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                           const EDA_TEXT* aTextItem );

    ~DIALOG_EDIT_ONE_FIELD() {}

    virtual bool TransferDataToWindow() override;

    virtual bool TransferDataFromWindow() override;

    SCH_BASE_FRAME* GetParent() { return dynamic_cast< SCH_BASE_FRAME* >( wxDialog::GetParent() ); }

    const wxString& GetText() const { return m_text; }

protected:

    void init();

    void updateText( EDA_TEXT* aText );

    /**
     * Function OnTextValueSelectButtonClick
     * Handles the select button next to the text value field. The current assumption
     * is that this event will only be enabled for footprint type fields. In the future
     * this function may need to be moved to the subclasses to access m_field and check for
     * the field type if more select actions are desired.
     *
     * @param aEvent is the the wX event thrown when the button is clicked, this isn't used
     */
    void OnTextValueSelectButtonClick( wxCommandEvent& aEvent ) override;

    /// @todo Update DIALOG_SHIM to handle this transparently so no matter what mode the
    ///       dialogs is shown, everything is handled without this ugliness.
    void OnOkClick( wxCommandEvent& aEvent ) override
    {
        if( IsQuasiModal() )
            EndQuasiModal( wxID_OK );
        else
            EndDialog( wxID_OK );
    }

    void OnCancelClick( wxCommandEvent& event ) override
    {
        if( IsQuasiModal() )
            EndQuasiModal( wxID_CANCEL );
        else
            EndDialog( wxID_CANCEL );
    }

    void OnCloseDialog( wxCloseEvent& aEvent ) override
    {
        if( IsQuasiModal() )
            EndQuasiModal( wxID_CANCEL );
        else
            EndDialog( wxID_CANCEL );
    }

    int       m_fieldId;
    bool      m_isPower;
    wxString  m_text;
    int       m_style;
    int       m_size;
    bool      m_orientation;
    int       m_verticalJustification;
    int       m_horizontalJustification;
    bool      m_isVisible;
};


/**
 * Class DIALOG_LIB_EDIT_ONE_FIELD
 * is a the class to handle editing a single component field in the library editor.
 * <p>
 * @note Use ShowQuasiModal when calling this class!
 * </p>
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
        updateText( aField );
    }
};


/**
 * Class DIALOG_SCH_EDIT_ONE_FIELD
 * is a the class to handle editing a single component field in the schematic editor.
 * <p>
 * @note Use ShowQuasiModal when calling this class!
 * </p>
 */
class DIALOG_SCH_EDIT_ONE_FIELD : public DIALOG_EDIT_ONE_FIELD
{
public:
    DIALOG_SCH_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                               const SCH_FIELD* aField );

    ~DIALOG_SCH_EDIT_ONE_FIELD() {}

    void UpdateField( SCH_FIELD* aField, SCH_SHEET_PATH* aSheetPath );
};

#endif    // DIALOG_EDIT_ONE_FIELD_H_
