
#ifndef DIALOG_EDIT_ONE_FIELD_H_
#define DIALOG_EDIT_ONE_FIELD_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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


/**
 * Class DIALOG_EDIT_ONE_FIELD
 * is a basic class to edit a field: a schematic or a lib component field
 * <p>
 * This class is setup in expectation of its children
 * possibly using Kiway player so ShowQuasiModal is required when calling
 * any subclasses.
 */
class DIALOG_EDIT_ONE_FIELD : public DIALOG_LIB_EDIT_TEXT_BASE
{
protected:
    SCH_BASE_FRAME* m_parent;
    int m_textshape;
    int m_textsize;
    int m_textorient;
    EDA_TEXT_HJUSTIFY_T m_textHjustify;
    EDA_TEXT_VJUSTIFY_T m_textVjustify;
    bool m_text_invisible;

public:
    DIALOG_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle ):
        DIALOG_LIB_EDIT_TEXT_BASE( aParent )
    {
        m_parent = aParent;
        SetTitle( aTitle );

        // Avoid not initialized members:
        m_textshape = 0;
        m_textsize = 0;
        m_textorient = 0;
        m_textHjustify = GR_TEXT_HJUSTIFY_CENTER;
        m_textVjustify = GR_TEXT_VJUSTIFY_CENTER;
        m_text_invisible = false;
    }

    // ~DIALOG_EDIT_ONE_FIELD() {};

    /**
     * Function TransfertDataToField
     * Converts fields from dialog window to variables to be used by child classes
     */
    virtual void TransfertDataToField();

    void SetTextField( const wxString& aText )
    {
         m_TextValue->SetValue( aText );
    }


protected:
    /**
     * Function initDlg_base
     * Common dialog option initialization for the subclasses to call
     */
    void initDlg_base();

    /**
     * Function OnTextValueSelectButtonClick
     * Handles the select button next to the text value field. The current assumption
     * is that this event will only be enabled for footprint type fields. In the future
     * this function may need to be moved to the subclasses to access m_field and check for
     * the field type if more select actions are desired.
     *
     * @param aEvent is the the wX event thrown when the button is clicked, this isn't used
     */
    void OnTextValueSelectButtonClick( wxCommandEvent& aEvent );

    void OnOkClick( wxCommandEvent& aEvent )
    {
        EndQuasiModal( wxID_OK );
    }

    void OnCancelClick( wxCommandEvent& event )
    {
        EndQuasiModal( wxID_CANCEL );
    }

    void OnCloseDialog( wxCloseEvent& aEvent )
    {
        EndQuasiModal( wxID_CANCEL );
    }
};


/**
 * Class DIALOG_LIB_EDIT_ONE_FIELD
 * is a the class to handle editing a single component field
 * in the library editor.
 * <p>
 * Note: Use ShowQuasiModal when calling this class!
 */
class DIALOG_LIB_EDIT_ONE_FIELD : public DIALOG_EDIT_ONE_FIELD
{
private:
    LIB_FIELD* m_field;

public:
    DIALOG_LIB_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent, const wxString& aTitle,
                               LIB_FIELD* aField ):
        DIALOG_EDIT_ONE_FIELD( aParent, aTitle )
    {
        m_field = aField;
        initDlg();
        GetSizer()->SetSizeHints( this );
        Centre();
    }

    ~DIALOG_LIB_EDIT_ONE_FIELD() {};

    void TransfertDataToField();

    /**
     * Function GetTextField
     * Returns the dialog's text field value with spaces filtered to underscores
     */
    wxString GetTextField();

private:
    /**
     * Function initDlg
     * Initializes dialog data using the LIB_FIELD container of data, this function is
     * otherwise identical to DIALOG_SCH_EDIT_ONE_FIELD::initDlg()
     */
    void initDlg( );
};


/**
 * Class DIALOG_SCH_EDIT_ONE_FIELD
 * is a the class to handle editing a single component field
 * in the schematic editor.
 * <p>
 * Note: Use ShowQuasiModal when calling this class!
 */
class DIALOG_SCH_EDIT_ONE_FIELD : public DIALOG_EDIT_ONE_FIELD
{
private:
    SCH_FIELD* m_field;

public:
    DIALOG_SCH_EDIT_ONE_FIELD( SCH_BASE_FRAME* aParent,
                               const wxString& aTitle, SCH_FIELD* aField ):
        DIALOG_EDIT_ONE_FIELD( aParent, aTitle )
    {
        m_field = aField;
        initDlg();
        GetSizer()->SetSizeHints( this );
        Centre();
    }

    // ~DIALOG_SCH_EDIT_ONE_FIELD() {};

    void TransfertDataToField();

    /**
     * Function GetTextField
     * Retrieves text field value from dialog with whitespaced on both sides trimmed
     */
    wxString GetTextField();

private:
    /**
     * Function initDlg
     * Initializes dialog data using the SCH_FIELD container of data, this function is
     * otherwise identical to DIALOG_LIB_EDIT_ONE_FIELD::initDlg()
     */
    void initDlg( );
};

#endif    // DIALOG_EDIT_ONE_FIELD_H_
