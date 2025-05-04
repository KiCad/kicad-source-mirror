/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 John Beard, john.j.beard@gmail.com
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

// Include the wxFormBuider header base:
#include <dialog_create_array_base.h>

#include <memory>

#include <array_options.h>
#include <board_item.h>
#include <pcb_base_frame.h>
#include <tools/pcb_picker_tool.h>
#include <widgets/unit_binder.h>
#include <widgets/widget_save_restore.h>


class DIALOG_CREATE_ARRAY : public DIALOG_CREATE_ARRAY_BASE, public PCB_PICKER_TOOL::RECEIVER
{
public:
    /**
     * Construct a new dialog.
     *
     * @param aParent the parent window
     * @param aOptions the options that will be updated when dialog is closed with OK
     * @param aEnableNumbering enable pad numbering
     * @param aOrigPos original item position (used for computing the circular array radius)
     */
    DIALOG_CREATE_ARRAY( PCB_BASE_FRAME* aParent, std::unique_ptr<ARRAY_OPTIONS>& aOptions,
                         bool enableNumbering, const VECTOR2I& aOrigPos );
    ~DIALOG_CREATE_ARRAY();

    // Implement the RECEIVER interface for the callback from the TOOL
    void UpdatePickedItem( const EDA_ITEM* aItem ) override;
    void UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint ) override;

private:
    // Event callbacks
    void OnParameterChanged( wxCommandEvent& aEvent ) override;

    // Center select buttons
    void OnSelectCenterButton( wxCommandEvent& aEvent ) override;

    // Axis numbering alphabet
    void OnAxisNumberingChange( wxCommandEvent& aEvent ) override;

    // Internal callback handlers
    void setControlEnablement();
    void calculateCircularArrayProperties();

    bool TransferDataFromWindow() override;

    PCB_BASE_FRAME* m_frame;

    /**
     * The settings to re-seat on dialog OK.
     */
    std::unique_ptr<ARRAY_OPTIONS>& m_settings;

    /*
     * The position of the original item(s), used for finding radius, etc
     */
    const VECTOR2I m_originalItemPosition;

    // Decide whether to display pad numbering options or not
    bool m_isFootprintEditor;

    UNIT_BINDER m_hSpacing, m_vSpacing;
    UNIT_BINDER m_hOffset, m_vOffset;
    UNIT_BINDER m_hCentre, m_vCentre;
    UNIT_BINDER m_circAngle, m_circOffset;

    WIDGET_SAVE_RESTORE m_cfg_persister;
};
