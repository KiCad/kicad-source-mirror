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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_SWAP_LAYERS_H
#define DIALOG_SWAP_LAYERS_H

#include "dialog_swap_layers_base.h"

class PCB_EDIT_FRAME;
class LAYER_GRID_TABLE;


class DIALOG_SWAP_LAYERS : public DIALOG_SWAP_LAYERS_BASE
{
public:
    DIALOG_SWAP_LAYERS( PCB_BASE_EDIT_FRAME* aParent,
                        std::map<PCB_LAYER_ID, PCB_LAYER_ID>& aArray );
    ~DIALOG_SWAP_LAYERS() override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnSize( wxSizeEvent& event ) override;

    void adjustGridColumns();

private:
    PCB_BASE_EDIT_FRAME*                  m_parent;
    std::map<PCB_LAYER_ID, PCB_LAYER_ID>& m_layerMap;

    LAYER_GRID_TABLE*                     m_gridTable;
};

#endif  // DIALOG_SWAP_LAYERS_H
