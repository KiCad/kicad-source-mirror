/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
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

#include <backannotate.h>
#include <dialog_update_from_pcb_base.h>

class SCH_EDIT_FRAME;


class DIALOG_UPDATE_FROM_PCB : public DIALOG_UPDATE_FROM_PCB_BASE
{
public:
    DIALOG_UPDATE_FROM_PCB( SCH_EDIT_FRAME* aParent );
    ~DIALOG_UPDATE_FROM_PCB() = default;

private:
    void updateData();

    bool TransferDataToWindow() override;
    void OnOptionChanged( wxCommandEvent& event ) override;
    void OnUpdateClick( wxCommandEvent& event ) override;

private:
    SCH_EDIT_FRAME*     m_frame;
};
