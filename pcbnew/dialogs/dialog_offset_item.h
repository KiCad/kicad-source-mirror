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

#include <dialogs/dialog_offset_item_base.h>

#include <widgets/unit_binder.h>
#include <pcb_base_frame.h>

/**
 * Dialog that invites the user to enter some kind of offset.
 */
class DIALOG_OFFSET_ITEM : public DIALOG_OFFSET_ITEM_BASE
{
public:
    /**
     * @param aFrame The parent frame.
     * @param aOffset The offset to be edited.
     */
    DIALOG_OFFSET_ITEM( PCB_BASE_FRAME& aFrame, VECTOR2I& aOffset );

    virtual void OnTextFocusLost( wxFocusEvent& event ) override;
    virtual void OnClear( wxCommandEvent& event ) override;
    virtual void OnPolarChanged( wxCommandEvent& event ) override;

    virtual bool TransferDataToWindow() override;
    virtual bool TransferDataFromWindow() override;

private:
    void updateDialogControls( bool aPolar );

    const VECTOR2I m_originalOffset;
    VECTOR2I&      m_updatedOffset;

    UNIT_BINDER m_xOffset, m_yOffset;

    double    m_stateX;
    double    m_stateY;
    double    m_stateRadius;
    EDA_ANGLE m_stateTheta;
};
