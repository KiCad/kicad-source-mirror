/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Miguel Angel Ajo <miguelangel@nbee.es>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <dialog_footprint_wizard_list_base.h>
#include <footprint_wizard.h>

class FOOTPRINT_WIZARD_FRAME;


class DIALOG_FOOTPRINT_WIZARD_LIST: public DIALOG_FOOTPRINT_WIZARD_LIST_BASE
{
public:
    DIALOG_FOOTPRINT_WIZARD_LIST( FOOTPRINT_WIZARD_FRAME* aParent );
    ~DIALOG_FOOTPRINT_WIZARD_LIST() = default;

    const wxString& GetWizard();
    FOOTPRINT_WIZARD_FRAME* ParentFrame();

private:
    void initLists();
    void OnCellFpGeneratorClick( wxGridEvent& event ) override;
    void OnCellFpGeneratorDoubleClick( wxGridEvent& event ) override;

private:
    wxString m_selectedWizard;    ///< The selected footprint wizard identifier
};
