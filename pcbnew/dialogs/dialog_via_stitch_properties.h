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

#include "dialog_via_stitch_properties_base.h"

#include <memory>

class PCB_BASE_EDIT_FRAME;
class PCB_VIA_STITCH;
class PCB_VIA;
class PCB_TRACK;
class UNIT_BINDER;

namespace KIGFX
{
class ORIGIN_VIEWITEM;
}


/**
 * Modal properties editor for a PCB_VIA_STITCH generator.  Drives an off-board working
 * copy of the generator, refreshes the GAL preview after every change, and only writes
 * the values back to the live generator (via BOARD_COMMIT) when the user clicks OK.
 *
 * Used in two places:
 *   - PCB_VIA_STITCH::ShowPropertiesDialog (E key on a selected stitch zone)
 *   - ZONE_CREATE_HELPER stitch path (right after the outline is closed)
 */
class DIALOG_VIA_STITCH_PROPERTIES : public DIALOG_VIA_STITCH_PROPERTIES_BASE
{
public:
    DIALOG_VIA_STITCH_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame, PCB_VIA_STITCH* aStitch );
    ~DIALOG_VIA_STITCH_PROPERTIES() override;

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnValuesChanged( wxCommandEvent& event ) override;
    void OnViaConfigureClicked( wxCommandEvent& event ) override;
    void OnCancel( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

private:
    /// Pull values out of UI controls into m_workingCopy, regenerate its children, and
    /// re-zoom the preview canvas.
    void updateWorkingCopyFromUI();

    /// Push the GAL preview to reflect the current m_workingCopy state.
    void redrawPreview();

    /**
     * Builds a preview for stitch mode
     *
     * @param aSamples receives the via positions to draw.
     * @return the side length of the drawn tile, in IU.
     */
    int buildStitchPreview( std::vector<VECTOR2I>& aSamples );

    /**
     * Builds a preview for guard mode
     *
     * @param aSamples receives the via positions to draw.
     * @return the side length of the drawn tile, in IU.
     */
    int buildGuardPreview( std::vector<VECTOR2I>& aSamples );

    /// Autozoom the preview canvas onto the last-rendered tile
    void fitPreview();

    /// First-time set-up of m_panelShowPreview — colors, view layers, board attachment.
    void prepareCanvas();

private:
    PCB_BASE_EDIT_FRAME*            m_frame;

    /// The live generator being edited.  Only written back on OK.
    PCB_VIA_STITCH*                 m_stitch;

    /// Off-board clone of m_stitch used to drive the preview without touching the board.
    std::unique_ptr<PCB_VIA_STITCH> m_workingCopy;

    std::unique_ptr<UNIT_BINDER>    m_pitchBinder;
    std::unique_ptr<UNIT_BINDER>    m_posXBinder;
    std::unique_ptr<UNIT_BINDER>    m_posYBinder;

    KIGFX::ORIGIN_VIEWITEM*         m_axisOrigin = nullptr;

    /// Vias currently in the preview canvas, used for both Guard and Stitch mode previews
    std::vector<std::unique_ptr<PCB_VIA>> m_previewVias;

    /// Tracks currently in the preview canvas, only used for Guard mode preview
    std::vector<std::unique_ptr<PCB_TRACK>> m_previewTracks;

    bool                            m_canvasInitialized = false;

    /// Side length of the tile drawn by the last redrawPreview(), in IU; 0 if nothing is
    /// currently drawn.  Kept so a resize can re-fit without rebuilding the vias.
    int                             m_previewExtent = 0;
};
