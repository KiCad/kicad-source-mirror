/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#ifndef _DIALOG_FREEROUTE_EXCHANGE_H_
#define _DIALOG_FREEROUTE_EXCHANGE_H_

#include <dialog_freeroute_exchange_base.h>

class DIALOG_FREEROUTE : public DIALOG_FREEROUTE_BASE
{
private:
    PCB_EDIT_FRAME* m_Parent;
    bool m_freeRouterFound;

private:
    // Virtual event handlers
    void OnOKButtonClick( wxCommandEvent& event );
    void OnExportButtonClick( wxCommandEvent& event );
    void OnLaunchButtonClick( wxCommandEvent& event );
    void OnImportButtonClick( wxCommandEvent& event );
    void OnHelpButtonClick( wxCommandEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );

    void MyInit ( );
    const wxString createDSN_File();

public:
    DIALOG_FREEROUTE( PCB_EDIT_FRAME* parent );
    ~DIALOG_FREEROUTE() {};

};

#endif

// _DIALOG_FREEROUTE_EXCHANGE_H_
