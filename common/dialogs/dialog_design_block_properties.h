/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mike Williams <mikebwilliams@gmail.com>
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

#ifndef DIALOG_DESIGN_BLOCK_PROPERTIES_H
#define DIALOG_DESIGN_BLOCK_PROPERTIES_H

#include <dialogs/dialog_design_block_properties_base.h>
#include <json_common.h>

class SCH_EDIT_FRAME;
class DESIGN_BLOCK;

class DIALOG_DESIGN_BLOCK_PROPERTIES : public DIALOG_DESIGN_BLOCK_PROPERTIES_BASE
{
public:
    DIALOG_DESIGN_BLOCK_PROPERTIES( wxWindow* aParent, DESIGN_BLOCK* aDesignBlock, bool aDisableName = false );
    ~DIALOG_DESIGN_BLOCK_PROPERTIES() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool TransferDataToGrid();
    bool TransferDataFromGrid();

    void OnAddField( wxCommandEvent& aEvent ) override;
    void OnDeleteField( wxCommandEvent& aEvent ) override;
    void OnMoveFieldUp( wxCommandEvent& aEvent ) override;
    void OnMoveFieldDown( wxCommandEvent& aEvent ) override;

private:
    DESIGN_BLOCK*                             m_designBlock;
    nlohmann::ordered_map<wxString, wxString> m_fields;
};

#endif
