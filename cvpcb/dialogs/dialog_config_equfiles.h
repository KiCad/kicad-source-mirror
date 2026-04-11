/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010-2015 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef DIALOG_CONFIG_EQUFILES_H
#define DIALOG_CONFIG_EQUFILES_H

#include <dialog_config_equfiles_base.h>

class DIALOG_CONFIG_EQUFILES : public DIALOG_CONFIG_EQUFILES_BASE
{
public:
    DIALOG_CONFIG_EQUFILES( wxWindow* parent );
    ~DIALOG_CONFIG_EQUFILES() {};

private:
    // Virtual event handlers
    void OnCloseWindow( wxCloseEvent& event ) override;
    void OnOkClick( wxCommandEvent& event ) override;
    void OnAddFiles( wxCommandEvent& event ) override;
    void OnEditEquFile( wxCommandEvent& event ) override;
    void OnRemoveFiles( wxCommandEvent& event ) override;
    void OnButtonMoveUp( wxCommandEvent& event ) override;
    void OnButtonMoveDown( wxCommandEvent& event ) override;
};

#endif      // DIALOG_CONFIG_EQUFILES_H
