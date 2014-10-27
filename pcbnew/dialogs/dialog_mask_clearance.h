/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#ifndef _DIALOG_MASK_CLEARANCE_H_
#define _DIALOG_MASK_CLEARANCE_H_

#include <dialog_mask_clearance_base.h>

/**
 *  DIALOG_PADS_MASK_CLEARANCE, derived from DIALOG_PADS_MASK_CLEARANCE_BASE
 *  @see dialog_mask_clearance.h and dialog_mask_clearance.cpp,
 *  automatically created by wxFormBuilder
 */
class DIALOG_PADS_MASK_CLEARANCE : public DIALOG_PADS_MASK_CLEARANCE_BASE
{
private:
    PCB_EDIT_FRAME*  m_parent;
    BOARD_DESIGN_SETTINGS  m_brdSettings;

public:
    DIALOG_PADS_MASK_CLEARANCE( PCB_EDIT_FRAME* parent );
    ~DIALOG_PADS_MASK_CLEARANCE() {};
private:
    void         myInit();
    virtual void OnButtonOkClick( wxCommandEvent& event );
    virtual void OnButtonCancelClick( wxCommandEvent& event );
};

#endif    // _DIALOG_MASK_CLEARANCE_H_
