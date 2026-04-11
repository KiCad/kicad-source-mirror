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

#ifndef _DIALOG_GLOBAL_DELETION_H_
#define _DIALOG_GLOBAL_DELETION_H_

#include <dialog_global_deletion_base.h>

class DIALOG_GLOBAL_DELETION: public DIALOG_GLOBAL_DELETION_BASE
{
public:
    DIALOG_GLOBAL_DELETION( PCB_EDIT_FRAME* parent );
    void SetCurrentLayer( int aLayer );

    ///< @todo Move this back into the tool rather than having the dialog do the deleting.
    void DoGlobalDeletions();

private:
    void onCheckDeleteTracks( wxCommandEvent& event ) override;
    void onCheckDeleteFootprints( wxCommandEvent& event ) override;
    void onCheckDeleteDrawings( wxCommandEvent& event ) override;
    void onCheckDeleteBoardOutlines( wxCommandEvent& event ) override;

    PCB_EDIT_FRAME* m_Parent;
    int             m_currentLayer;
};

#endif  // _DIALOG_GLOBAL_DELETION_H_
