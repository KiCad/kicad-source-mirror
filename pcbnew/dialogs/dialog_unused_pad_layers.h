/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef __dialog_remove_unused_pads__
#define __dialog_remove_unused_pads__

#include "dialog_unused_pad_layers_base.h"

class COMMIT;
class PCB_BASE_FRAME;
class PCB_SELECTION;

class DIALOG_UNUSED_PAD_LAYERS : public DIALOG_UNUSED_PAD_LAYERS_BASE
{
public:
    DIALOG_UNUSED_PAD_LAYERS( PCB_BASE_FRAME* aParent, const PCB_SELECTION& aItems,
                              COMMIT& aCommit  );

private:
    void updateImage();

    /**
     * Update layers of pads and vias
     * aRemoveLayers = true to remove not connected layers
     * false to set all layers to active
     */
    void updatePadsAndVias( bool aRemoveLayers );

    void onApply( wxCommandEvent& event ) override;
    void onOK( wxCommandEvent& event ) override;

    void syncImages( wxCommandEvent& aEvent ) override;

    PCB_BASE_FRAME*      m_frame;
    const PCB_SELECTION& m_items;     // List of items to be modified.
    COMMIT&              m_commit;    // An undo record to add any changes to.
};

#endif // __dialog_remove_unused_pads__
