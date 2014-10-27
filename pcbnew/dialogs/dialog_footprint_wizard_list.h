/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_FOOTPRINT_WIZARD_LIST_H_
#define _DIALOG_FOOTPRINT_WIZARD_LIST_H_

#include <dialog_footprint_wizard_list_base.h>
#include <class_footprint_wizard.h>

class DIALOG_FOOTPRINT_WIZARD_LIST: public DIALOG_FOOTPRINT_WIZARD_LIST_BASE
{
private:
    FOOTPRINT_WIZARD *m_FootprintWizard;

public:
    DIALOG_FOOTPRINT_WIZARD_LIST( wxWindow * parent );

    FOOTPRINT_WIZARD* GetWizard();

private:
    void OnCellWizardClick( wxGridEvent& event );
    void OnOpenButtonClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
};

#endif  // _DIALOG_FOOTPRINT_WIZARD_LIST_H_
