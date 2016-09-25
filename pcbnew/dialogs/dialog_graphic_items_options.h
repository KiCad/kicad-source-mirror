/**
 * @file dialog_graphic_items_options.h
 */

 /*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras> jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 1992-2016 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _DIALOG_GRAPHIC_ITEMS_OPTIONS_H_
#define _DIALOG_GRAPHIC_ITEMS_OPTIONS_H_

#include <dialog_graphic_items_options_base.h>

/*!
 * DIALOG_GRAPHIC_ITEMS_OPTIONS class declaration
 */

class DIALOG_GRAPHIC_ITEMS_OPTIONS: public DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE
{
private:
    BOARD_DESIGN_SETTINGS  m_brdSettings;
    PCB_BASE_FRAME * m_parent;

public:
    DIALOG_GRAPHIC_ITEMS_OPTIONS( PCB_BASE_FRAME* parent );
    ~DIALOG_GRAPHIC_ITEMS_OPTIONS();

private:
    void initValues( );
    void OnOkClick( wxCommandEvent& event ) override;
    void OnCancelClick( wxCommandEvent& event ) override;

    void OnInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }
};

#endif
    // _DIALOG_GRAPHIC_ITEMS_OPTIONS_H_
