/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef _DIALOG_PLUGIN_OPTIONS_H_
#define _DIALOG_PLUGIN_OPTIONS_H_

#include <dialog_plugin_options_base.h>
#include <string_utf8_map.h>

/**
 * DIALOG_PLUGIN_OPTIONS
 * is an options editor in the form of a two column name/value
 * spreadsheet like (table) UI.
 */
class DIALOG_PLUGIN_OPTIONS : public DIALOG_PLUGIN_OPTIONS_BASE
{
public:
    DIALOG_PLUGIN_OPTIONS( wxWindow* aParent, const wxString& aNickname,
                           const STRING_UTF8_MAP& aPluginOptions, const wxString& aFormattedOptions,
                           wxString* aResult );

    ~DIALOG_PLUGIN_OPTIONS() override;

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

private:
    const wxString& m_callers_options;
    wxString*       m_result;
    STRING_UTF8_MAP m_choices;
    wxString        m_initial_help;
    bool            m_grid_widths_dirty;

    int appendRow();

    void appendOption();

    //-----<event handlers>------------------------------------------------------
    void onListBoxItemSelected( wxCommandEvent& event ) override;

    void onListBoxItemDoubleClicked( wxCommandEvent& event ) override;

    void onAppendOption( wxCommandEvent& ) override;

    void onAppendRow( wxCommandEvent& ) override;

    void onDeleteRow( wxCommandEvent& ) override;

    void onGridCellChange( wxGridEvent& aEvent ) override;

    void onUpdateUI( wxUpdateUIEvent& ) override;

    void onSize( wxSizeEvent& aEvent ) override;
};


#endif // _DIALOG_PLUGIN_OPTIONS_H_

