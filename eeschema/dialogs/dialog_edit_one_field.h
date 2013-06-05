
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

// Basic class to edit a field: a schematic or a lib component field
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
    }

    // ~DIALOG_EDIT_ONE_FIELD() {};

    virtual void TransfertDataToField();

    void SetTextField( const wxString& aText )
    {
         m_TextValue->SetValue( aText );
    }

protected:
    void initDlg_base( );
    void OnOkClick( wxCommandEvent& aEvent )
    {
        EndModal(wxID_OK);
    }

    void OnCancelClick( wxCommandEvent& aEvent )
    {
        EndModal(wxID_CANCEL);
    }
};


// Class to edit a lib component field
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
        GetSizer()->SetSizeHints(this);
        Centre();
    }

    ~DIALOG_LIB_EDIT_ONE_FIELD() {};

    void TransfertDataToField();
    wxString GetTextField();

private:
    void initDlg( );
};


// Class to edit a schematic component field
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
        GetSizer()->SetSizeHints(this);
        Centre();
    }

    // ~DIALOG_SCH_EDIT_ONE_FIELD() {};

    void TransfertDataToField();
    wxString GetTextField();

private:
    void initDlg( );
};

#endif    // DIALOG_EDIT_ONE_FIELD_H_
