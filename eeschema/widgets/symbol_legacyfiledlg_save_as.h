/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYMBOL_LEGACYFILEDLG_SAVE_AS_
#define SYMBOL_LEGACYFILEDLG_SAVE_AS_

#include <symbol_editor/symbol_saveas_type.h>

#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>

/**
 * Helper control to inquire user what to do on library save as operation.
 */
class SYMBOL_LEGACYFILEDLG_SAVE_AS : public wxPanel
{
public:
    SYMBOL_LEGACYFILEDLG_SAVE_AS( wxWindow* aParent ) : wxPanel( aParent )
    {
        m_simpleSaveAs = new wxRadioButton( this, wxID_ANY, _( "Do not update library tables" ),
                                            wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
        m_simpleSaveAs->SetToolTip( _( "Do not perform any additional operations after saving "
                                       "library." ) );
        m_replaceTableEntry =
                new wxRadioButton( this, wxID_ANY, _( "Update existing library table entry" ) );
        m_replaceTableEntry->SetToolTip( _( "Update symbol library table entry to point to new "
                                            "library.\n\n"
                                            "The original library will no longer be available "
                                            "for use." ) );
        m_addGlobalTableEntry =
                new wxRadioButton( this, wxID_ANY, _( "Add new global library table entry" ) );
        m_addGlobalTableEntry->SetToolTip( _( "Add new entry to the global symbol library table."
                                              "\n\nThe symbol library table nickname is suffixed "
                                              "with\nan integer to prevent duplicate table "
                                              "entries." ) );
        m_addProjectTableEntry =
                new wxRadioButton( this, wxID_ANY, _( "Add new project library table entry" ) );
        m_addProjectTableEntry->SetToolTip( _( "Add new entry to the project symbol library table."
                                               "\n\nThe symbol library table nickname is suffixed "
                                               "with\nan integer to prevent duplicate table "
                                               "entries." ) );

        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add( m_simpleSaveAs, 0, wxLEFT | wxRIGHT | wxTOP, 5 );
        sizer->Add( m_replaceTableEntry, 0, wxLEFT | wxRIGHT | wxTOP, 5 );
        sizer->Add( m_addGlobalTableEntry, 0, wxLEFT | wxRIGHT | wxTOP, 5 );
        sizer->Add( m_addProjectTableEntry, 0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 5 );

        SetSizerAndFit( sizer );

        SetOption( m_option );
    }

    ~SYMBOL_LEGACYFILEDLG_SAVE_AS() { m_option = GetOption(); }

    void SetOption( SYMBOL_SAVEAS_TYPE aOption )
    {
        m_simpleSaveAs->SetValue( aOption == SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS );
        m_replaceTableEntry->SetValue( aOption == SYMBOL_SAVEAS_TYPE::REPLACE_TABLE_ENTRY );
        m_addGlobalTableEntry->SetValue( aOption == SYMBOL_SAVEAS_TYPE::ADD_GLOBAL_TABLE_ENTRY );
        m_addProjectTableEntry->SetValue( aOption == SYMBOL_SAVEAS_TYPE::ADD_PROJECT_TABLE_ENTRY );
    }

    SYMBOL_SAVEAS_TYPE GetOption() const
    {
        if( m_replaceTableEntry->GetValue() )
            return SYMBOL_SAVEAS_TYPE::REPLACE_TABLE_ENTRY;
        else if( m_addGlobalTableEntry->GetValue() )
            return SYMBOL_SAVEAS_TYPE::ADD_GLOBAL_TABLE_ENTRY;
        else if( m_addProjectTableEntry->GetValue() )
            return SYMBOL_SAVEAS_TYPE::ADD_PROJECT_TABLE_ENTRY;
        else
            return SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS;
    }

    /**
     * Create a new panel to add to a wxFileDialog object.
     *
     * The caller owns the created object and is responsible for deleting it.
     *
     * @param aParent is the parent window that will own the created object.
     * @return the newly created panel to add to the wxFileDialog.
     */
    static wxWindow* Create( wxWindow* aParent )
    {
        wxCHECK( aParent, nullptr );

        return new SYMBOL_LEGACYFILEDLG_SAVE_AS( aParent );
    }

private:
    static SYMBOL_SAVEAS_TYPE m_option;

    wxRadioButton* m_simpleSaveAs;
    wxRadioButton* m_replaceTableEntry;
    wxRadioButton* m_addGlobalTableEntry;
    wxRadioButton* m_addProjectTableEntry;
};

#endif