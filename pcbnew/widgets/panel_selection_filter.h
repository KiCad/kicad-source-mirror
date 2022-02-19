/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_PANEL_SELECTION_FILTER_H
#define KICAD_PANEL_SELECTION_FILTER_H

#include <widgets/panel_selection_filter_base.h>


class PCB_SELECTION_TOOL;
struct SELECTION_FILTER_OPTIONS;


class PANEL_SELECTION_FILTER : public PANEL_SELECTION_FILTER_BASE
{
public:
    PANEL_SELECTION_FILTER( wxWindow* aParent );

    ~PANEL_SELECTION_FILTER() = default;

    void SetCheckboxesFromFilter( SELECTION_FILTER_OPTIONS& aOptions );

    void OnLanguageChanged();

protected:
    void OnFilterChanged( wxCommandEvent& aEvent ) override;

private:
    bool setFilterFromCheckboxes( SELECTION_FILTER_OPTIONS& aOptions );

    void onRightClick( wxMouseEvent& aEvent );

    void onPopupSelection( wxCommandEvent& aEvent );

private:
    PCB_BASE_EDIT_FRAME* m_frame;
    PCB_SELECTION_TOOL*  m_tool;
    wxCheckBox*          m_onlyCheckbox;
};


#endif // KICAD_PANEL_SELECTION_FILTER_H
