/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef DIALOG_MICROVIA_STACK_H
#define DIALOG_MICROVIA_STACK_H

#include <dialogs/dialog_microvia_stack_base.h>
#include <widgets/unit_binder.h>

class PCB_BASE_EDIT_FRAME;
class PCB_VIA_STACK;


class DIALOG_MICROVIA_STACK : public DIALOG_MICROVIA_STACK_BASE
{
public:
    DIALOG_MICROVIA_STACK( PCB_BASE_EDIT_FRAME* aParent, PCB_VIA_STACK* aStack );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onTypeChanged( wxCommandEvent& aEvent );
    void onUseNetclass( wxCommandEvent& aEvent );
    void updateEnableState();

    PCB_VIA_STACK* m_stack;

    UNIT_BINDER m_viaSize;
    UNIT_BINDER m_viaDrill;
    UNIT_BINDER m_pitch;
};

#endif // DIALOG_MICROVIA_STACK_H
