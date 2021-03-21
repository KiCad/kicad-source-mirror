/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_EESCHEMA_DEFUALT_FIELDS_H
#define PANEL_EESCHEMA_DEFUALT_FIELDS_H

#include <template_fieldnames.h>
#include "panel_eeschema_template_fieldnames_base.h"

class SCH_EDIT_FRAME;


class PANEL_EESCHEMA_TEMPLATE_FIELDNAMES : public PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE
{
public:
    PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( SCH_EDIT_FRAME* aFrame, wxWindow* aWindow, bool aGlobal );
    ~PANEL_EESCHEMA_TEMPLATE_FIELDNAMES() override;

    void ImportSettingsFrom( TEMPLATES* templateMgr );

protected:
    /**
     * Adds a new template fieldname (with default values) to the template fieldnames data.
     *
     * Process the wxWidgets @a event produced when the user presses the Add buton for the
     * template fieldnames control.
     *
     * @param event The wxWidgets produced event information
     */
    void OnAddButtonClick( wxCommandEvent& event ) override;

    /**
     * Deletes the selected template fieldname from the template fieldnames data.
     *
     * Process the wxWidgets @a event produced when the user presses the Delete button for the
     * template fieldnames control.
     *
     * @param event The wxWidgets produced event information
     */
    void OnDeleteButtonClick( wxCommandEvent& event ) override;

private:
    void AdjustGridColumns( int aWidth );

    void OnSizeGrid( wxSizeEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool TransferDataToGrid();
    bool TransferDataFromGrid();

protected:
    SCH_EDIT_FRAME*     m_frame;
    TEMPLATE_FIELDNAMES m_fields;
    bool                m_global;   // Editing global (vs. project) fieldname templates

    int                 m_checkboxColWidth;
};


#endif //PANEL_EESCHEMA_DEFUALT_FIELDS_H
