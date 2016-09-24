/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  cvpcb/dialogs/dialog_display_options.h
 */

#ifndef _DIALOG_DISPLAY_OPTIONS_H_
#define _DIALOG_DISPLAY_OPTIONS_H_

#include <dialog_display_options_base.h>

/* Class DIALOG_FOOTPRINTS_DISPLAY_OPTIONS
 *  derived from DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE,
 *  created by wxformBuilder
*/

class DIALOG_FOOTPRINTS_DISPLAY_OPTIONS :
    public DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE
{
private:
PCB_BASE_FRAME * m_Parent;

public:
    DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( PCB_BASE_FRAME* parent );
    ~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS();


private:
    void initDialog( );
    void UpdateObjectSettings( void );
    void OnApplyClick( wxCommandEvent& event ) override;
    void OnCancelClick( wxCommandEvent& event ) override;
    void OnOkClick( wxCommandEvent& event ) override;
};

#endif      // _DIALOG_DISPLAY_OPTIONS_H_
