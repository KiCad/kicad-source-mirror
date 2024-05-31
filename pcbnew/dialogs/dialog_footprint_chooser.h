/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef DIALOG_FOOTPRINT_CHOOSER_H
#define DIALOG_FOOTPRINT_CHOOSER_H

#include <lib_id.h>
#include "dialog_shim.h"
#include <3d_canvas/board_adapter.h>
#include <3d_rendering/track_ball.h>

class PCB_BASE_FRAME;
class PANEL_FOOTPRINT_CHOOSER;
class EDA_3D_CANVAS;
class BOARD;
class CAMERA;
class TRACK_BALL;
class BITMAP_BUTTON;


class DIALOG_FOOTPRINT_CHOOSER : public DIALOG_SHIM
{
public:
    DIALOG_FOOTPRINT_CHOOSER( PCB_BASE_FRAME* aParent, const LIB_ID& aPreselect,
                              const wxArrayString& aFootprintHistoryList );

    ~DIALOG_FOOTPRINT_CHOOSER();

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId() const;

protected:
	void on3DviewReq( wxCommandEvent& event );
	void onFpViewReq( wxCommandEvent& event );

    // A command event sent by a PANEL_FOOTPRINT_CHOOSER will fire this event:
	void onFpChanged( wxCommandEvent& event );

    void build3DCanvas();

protected:
    PANEL_FOOTPRINT_CHOOSER* m_chooserPanel;
    bool                     m_showFpMode;   // True to show the footprint, false for the 3D model

private:
    PCB_BASE_FRAME*          m_parent;
    BOARD_ADAPTER            m_boardAdapter;
    EDA_3D_CANVAS*           m_preview3DCanvas;
    CAMERA&                  m_currentCamera;
    TRACK_BALL               m_trackBallCamera;
    BOARD*                   m_dummyBoard;
    BITMAP_BUTTON*           m_grButtonFpView;
    BITMAP_BUTTON*           m_grButton3DView;

    /// The 3d viewer Render initial settings (must be saved and restored)
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS m_initialRender;
};

#endif /* DIALOG_FOOTPRINT_CHOOSER_H */
