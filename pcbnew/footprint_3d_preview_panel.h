/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __FOOTPRINT_3D_PREVIEW_PANEL_H
#define __FOOTPRINT_3D_PREVIEW_PANEL_H

#include <memory>

#include <3d_canvas/board_adapter.h>
#include <3d_rendering/track_ball.h>
#include <kiway_holder.h>
#include <widgets/footprint_3d_preview_widget.h>

#include <wx/panel.h>

class BOARD;
class CAMERA;
class EDA_3D_CANVAS;
class FOOTPRINT;
class KIWAY;


class FOOTPRINT_3D_PREVIEW_PANEL : public wxPanel,
                                   public KIWAY_HOLDER,
                                   public FOOTPRINT_3D_PREVIEW_PANEL_BASE
{
public:
    ~FOOTPRINT_3D_PREVIEW_PANEL() override;

    FOOTPRINT_3D_PREVIEW_STATUS DisplayFootprint( const LIB_ID& aFPID ) override;
    void ClearPreview() override;
    void RefreshAll() override;
    void Shutdown() override;
    wxWindow* GetWindow() override { return this; }

    static FOOTPRINT_3D_PREVIEW_PANEL* New( KIWAY* aKiway, wxWindow* aParent );

private:
    FOOTPRINT_3D_PREVIEW_PANEL( KIWAY* aKiway, wxWindow* aParent );

    bool hasRenderableModels( const FOOTPRINT& aFootprint ) const;
    void configureDummyBoard();

private:
    std::unique_ptr<BOARD>     m_dummyBoard;
    std::shared_ptr<FOOTPRINT> m_currentFootprint;
    BOARD_ADAPTER              m_boardAdapter;
    TRACK_BALL                 m_trackBallCamera;
    CAMERA&                    m_currentCamera;
    EDA_3D_CANVAS*             m_canvas;
    bool                       m_shutdown;
};

#endif
