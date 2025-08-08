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

// Include the wxFormBuider header base:
#include <vector>
#include <dialogs/dialog_position_relative_base.h>

#include <tool/tool_manager.h>
#include <widgets/unit_binder.h>
#include <tools/pcb_picker_tool.h>

class DIALOG_POSITION_RELATIVE : public DIALOG_POSITION_RELATIVE_BASE,
                                 public PCB_PICKER_TOOL::RECEIVER
{
public:
    // Constructor and destructor
    DIALOG_POSITION_RELATIVE( PCB_BASE_FRAME* aParent );
    ~DIALOG_POSITION_RELATIVE() = default;

    // Implement the RECEIVER interface for the callback from the TOOL
    void UpdatePickedItem( const EDA_ITEM* aItem ) override;
    void UpdatePickedPoint( const std::optional<VECTOR2I>& aPoint ) override;

private:
    /**
     * Reset a text field to be 0 if it was exited while blank.
     */
    void OnTextFocusLost( wxFocusEvent& event ) override;

    void OnPolarChanged( wxCommandEvent& event ) override;
    void OnClear( wxCommandEvent& event ) override;

    void OnSelectItemClick( wxCommandEvent& event ) override;
    void OnSelectPointClick( wxCommandEvent& event ) override;
    void OnUseGridOriginClick( wxCommandEvent& event ) override;
    void OnUseUserOriginClick( wxCommandEvent& event ) override;
    void OnOkClick( wxCommandEvent& event ) override;

    /**
     * Convert a given Cartesian point into a polar representation.
     *
     * Linear units are not considered, the answer is in the same units as given.
     */
    void ToPolarDeg( double x, double y, double& r, EDA_ANGLE& q );

    /**
     * Get the (Cartesian) translation described by the text entries.
     *
     * @param val is the output translation vector.
     * @param set to true to interpret as polar coordinates.
     * @return false if error (though the text conversion functions don't report errors).
     */
    bool getTranslationInIU( VECTOR2I& val, bool polar );

    // Update controls and their labels after changing the coordinates type (polar/cartesian)
    void updateDialogControls( bool aPolar );

    // Update controls and labels after changing anchor type
    void updateAnchorInfo( const BOARD_ITEM* aItem );

    // Get the current anchor position.
    VECTOR2I getAnchorPos();

private:
    /**
     * Persistent dialog options.
     */
    enum ANCHOR_TYPE
    {
        ANCHOR_GRID_ORIGIN,
        ANCHOR_USER_ORIGIN,
        ANCHOR_ITEM,
        ANCHOR_POINT
    };

    static ANCHOR_TYPE s_anchorType;

private:
    TOOL_MANAGER* m_toolMgr;
    VECTOR2I      m_anchorItemPosition;

    UNIT_BINDER   m_xOffset;
    UNIT_BINDER   m_yOffset;

    double        m_stateX;
    double        m_stateY;
    double        m_stateRadius;
    EDA_ANGLE     m_stateTheta;
};
