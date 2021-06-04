/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_GROUP_PROPERTIES_H
#define DIALOG_GROUP_PROPERTIES_H

#include <dialogs/dialog_group_properties_base.h>

class PCB_BASE_EDIT_FRAME;
class TOOL_MANAGER;
class PCB_GROUP;


class DIALOG_GROUP_PROPERTIES : public DIALOG_GROUP_PROPERTIES_BASE
{
public:
    DIALOG_GROUP_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_GROUP* aTarget );
    ~DIALOG_GROUP_PROPERTIES();

    void OnMemberSelected( wxCommandEvent& event ) override;
    void OnAddMember( wxCommandEvent& event ) override;
    void OnRemoveMember( wxCommandEvent& event ) override;

    void DoAddMember( EDA_ITEM* aItem );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    PCB_BASE_EDIT_FRAME* m_brdEditor;
    TOOL_MANAGER*        m_toolMgr;
    PCB_GROUP*           m_group;
};

#endif  // DIALOG_GROUP_PROPERTIES_H
