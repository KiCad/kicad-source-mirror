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

#ifndef DIALOG_GROUP_PROPERTIES_H
#define DIALOG_GROUP_PROPERTIES_H

#include <memory>
#include <dialogs/dialog_group_properties_base.h>

class EDA_DRAW_FRAME;
class TOOL_MANAGER;
class EDA_GROUP;
class EDA_ITEM;
class COMMIT;
class BASE_SCREEN;


class DIALOG_GROUP_PROPERTIES : public DIALOG_GROUP_PROPERTIES_BASE
{
public:
    DIALOG_GROUP_PROPERTIES( EDA_DRAW_FRAME* aParent, EDA_GROUP* aTarget, const std::shared_ptr<COMMIT>& aCommit );
    ~DIALOG_GROUP_PROPERTIES() override;

    void OnMemberSelected( wxCommandEvent& event ) override;
    void OnAddMember( wxCommandEvent& event ) override;
    void OnRemoveMember( wxCommandEvent& event ) override;

    void DoAddMember( EDA_ITEM* aItem );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    EDA_DRAW_FRAME* m_frame;
    TOOL_MANAGER*   m_toolMgr;
    EDA_GROUP*      m_group;
    std::shared_ptr<COMMIT> m_commit;
};

#endif  // DIALOG_GROUP_PROPERTIES_H
