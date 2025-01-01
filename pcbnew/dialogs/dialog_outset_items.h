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

#pragma once

#include <pcb_base_frame.h>
#include <dialogs/dialog_outset_items_base.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/unit_binder.h>
#include <tools/item_modification_routine.h>

/**
 * DIALOG_OUTSET_ITEMS, derived from DIALOG_OUTSET_ITEMS_BASE,
 * created by wxFormBuilder
 */
class DIALOG_OUTSET_ITEMS : public DIALOG_OUTSET_ITEMS_BASE
{
public:
    DIALOG_OUTSET_ITEMS( PCB_BASE_FRAME& aParent, OUTSET_ROUTINE::PARAMETERS& aParams );
    ~DIALOG_OUTSET_ITEMS();

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnLayerDefaultClick( wxCommandEvent& event ) override;
    void OnCopyLayersChecked( wxCommandEvent& event ) override;
    void OnRoundToGridChecked( wxCommandEvent& event ) override;

private:

    PCB_BASE_FRAME&             m_parent;
    OUTSET_ROUTINE::PARAMETERS& m_params;

    UNIT_BINDER m_outset;
    UNIT_BINDER m_lineWidth;
    UNIT_BINDER m_roundingGrid;
};

