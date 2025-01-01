/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file dialog_dielectric_list_manager.h
 */

#ifndef DIALOG_DIELECTRIC_MANAGER_H
#define DIALOG_DIELECTRIC_MANAGER_H

#include "dialog_dielectric_list_manager_base.h"
#include "stackup_predefined_prms.h"
#include "dielectric_material.h"

/**
 * a Dialog to select/change/add a dielectric material from a material list
 */
class DIALOG_DIELECTRIC_MATERIAL: public DIALOG_DIELECTRIC_MATERIAL_BASE
{
public:
    DIALOG_DIELECTRIC_MATERIAL( wxWindow* aParent, DIELECTRIC_SUBSTRATE_LIST& aMaterialList );
    ~DIALOG_DIELECTRIC_MATERIAL();

    /// @return the selected substrate. If no substrate selected
    /// a empty substrate is returned
    DIELECTRIC_SUBSTRATE GetSelectedSubstrate();

private:
    void onListKeyDown( wxListEvent& event ) override;
    void onListItemSelected( wxListEvent& event ) override;
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void initMaterialList();    // Fills the dialog with available materials

    /// The list of available materials
    DIELECTRIC_SUBSTRATE_LIST& m_materialList;
};

#endif  // #ifndef DIALOG_DIELECTRIC_MANAGER_H
