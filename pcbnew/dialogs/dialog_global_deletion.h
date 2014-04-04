/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef _DIALOG_GLOBAL_DELETION_H_
#define _DIALOG_GLOBAL_DELETION_H_

#include <dialog_global_deletion_base.h>

class DIALOG_GLOBAL_DELETION: public DIALOG_GLOBAL_DELETION_BASE
{
public:
    DIALOG_GLOBAL_DELETION( PCB_EDIT_FRAME* parent );
    void SetCurrentLayer( LAYER_NUM aLayer );

private:
    PCB_EDIT_FRAME* m_Parent;
    LAYER_NUM m_currentLayer;

    void OnOkClick( wxCommandEvent& event )
    {
        AcceptPcbDelete();
        EndModal(wxID_OK);
    }

    void OnCancelClick( wxCommandEvent& event )
    {
        EndModal(wxID_CANCEL);
    }

    void AcceptPcbDelete();
    void OnCheckDeleteTracks( wxCommandEvent& event );
    void OnCheckDeleteModules( wxCommandEvent& event );
};

#endif  // _DIALOG_GLOBAL_DELETION_H_
