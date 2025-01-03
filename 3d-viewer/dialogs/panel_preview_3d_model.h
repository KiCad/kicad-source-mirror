/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#ifndef PANEL_PREVIEW_3D_MODEL_H
#define PANEL_PREVIEW_3D_MODEL_H

#include "panel_preview_3d_model_base.h"

#include <vector>
#include <widgets/unit_binder.h>
#include <tool/tools_holder.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <3d_viewer_id.h>
#include <3d_rendering/track_ball.h>
#include <wx/event.h>

// Define min and max parameter values
#define MAX_SCALE          10000.0
#define MAX_ROTATION       180.0
#define MAX_OFFSET         1000.0

#define SCALE_INCREMENT_FINE       0.02
#define SCALE_INCREMENT            0.1

#define ROTATION_INCREMENT 90      // in degrees
#define ROTATION_INCREMENT_FINE 1  // in degrees

#define OFFSET_INCREMENT_MM        0.5
#define OFFSET_INCREMENT_MM_FINE   0.1

#define OFFSET_INCREMENT_MIL       25.0
#define OFFSET_INCREMENT_MIL_FINE  5.0

wxDECLARE_EVENT( wxCUSTOM_PANEL_SHOWN_EVENT, wxCommandEvent );

// Declared classes to create pointers
class WX_INFOBAR;
class S3D_CACHE;
class FILENAME_RESOLVER;
class BOARD;
class BOARD_ADAPTER;
class FOOTPRINT;
#ifndef __linux__
class NL_FOOTPRINT_PROPERTIES_PLUGIN;
#else
class SPNAV_VIEWER_PLUGIN;
#endif

#define PANEL_PREVIEW_3D_MODEL_ID  wxID_HIGHEST + 1244

class PANEL_PREVIEW_3D_MODEL: public TOOLS_HOLDER, public PANEL_PREVIEW_3D_MODEL_BASE
{
public:
    PANEL_PREVIEW_3D_MODEL( wxWindow* aParent, PCB_BASE_FRAME* aFrame, FOOTPRINT* aFootprint,
                            std::vector<FP_3DMODEL>* aParentModelList );

    ~PANEL_PREVIEW_3D_MODEL();

    /**
     * The TOOL_DISPATCHER needs these to work around some issues in wxWidgets where the menu
     * events aren't captured by the menus themselves.
     */
    void OnMenuEvent( wxMenuEvent& aEvent );

    wxWindow* GetToolCanvas() const override { return m_previewPane; }

    BOARD_ADAPTER& GetAdapter() { return m_boardAdapter; }
    CAMERA& GetCurrentCamera() { return m_currentCamera; }

    /**
     * Set the currently selected index in the model list so that the scale/rotation/offset
     * controls can be updated.
     */
    void SetSelectedModel( int idx );

    /**
     * Copy shapes from the current shape list which are flagged for preview to the copy of
     * footprint that is on the preview dummy board.
     */
    void UpdateDummyFootprint( bool aRelaodRequired = true );

    /**
     * Get the dummy footprint that is used for previewing the 3D model.
     * We use this to hold the temporary 3D model shapes.
     */
    FOOTPRINT* GetDummyFootprint() const { return m_dummyFootprint; }

private:
    /**
     * Load 3D relevant settings from the user configuration
     */
    void loadSettings();

    /**
     * It will receive the events from editing the fields.
     */
    void updateOrientation( wxCommandEvent& event ) override;

	void onMouseWheelScale( wxMouseEvent& event ) override;
	void onMouseWheelRot( wxMouseEvent& event ) override;
	void onMouseWheelOffset( wxMouseEvent& event ) override;

	void onIncrementRot( wxSpinEvent& event ) override
    {
        doIncrementRotation( event, 1.0 );
    }
	void onDecrementRot( wxSpinEvent& event ) override
    {
        doIncrementRotation( event, -1.0 );
    }
	void onIncrementScale( wxSpinEvent& event ) override
    {
        doIncrementScale( event, 1.0 );
    }
	void onDecrementScale( wxSpinEvent& event ) override
    {
        doIncrementScale( event, -1.0 );
    }
	void onIncrementOffset( wxSpinEvent& event ) override
    {
        doIncrementOffset( event, 1.0 );
    }
	void onDecrementOffset( wxSpinEvent& event ) override
    {
        doIncrementOffset( event, -1.0 );
    }

    void onOpacitySlider( wxCommandEvent& event ) override;

    void doIncrementScale( wxSpinEvent& aEvent, double aSign );
    void doIncrementRotation( wxSpinEvent& aEvent, double aSign );
    void doIncrementOffset( wxSpinEvent& aEvent, double aSign );

    void onUnitsChanged( wxCommandEvent& aEvent );
    void onPanelShownEvent( wxCommandEvent& aEvent );

    wxString formatScaleValue( double aValue );
    wxString formatRotationValue( double aValue );
    wxString formatOffsetValue( double aValue );

	void View3DISO( wxCommandEvent& event ) override
    {
	    m_currentCamera.ToggleProjection();
        m_previewPane->Refresh();
    }

    // turn ON or OFF options to show the board body. If OFF, solder paste, soldermask
    // and board body are hidden, to allows a good view of the 3D model and its pads.
    // Useful for 3D model placement
	void setBodyStyleView( wxCommandEvent& event ) override;

	void View3DLeft( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_LEFT );
    }

	void View3DFront( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_FRONT );
    }

	void View3DTop( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_TOP );
    }

	void View3DUpdate( wxCommandEvent& event ) override
    {
        m_previewPane->ReloadRequest();
        m_previewPane->Refresh();
    }

    void View3DSettings( wxCommandEvent& event ) override;

	void View3DRight( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_RIGHT );
    }

	void View3DBack( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_BACK );
    }

	void View3DBottom( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( VIEW3D_TYPE::VIEW3D_BOTTOM );
    }

    void onModify();

private:
    PCB_BASE_FRAME*          m_parentFrame;
    EDA_3D_CANVAS*           m_previewPane;
    WX_INFOBAR*              m_infobar;
    BOARD_ADAPTER            m_boardAdapter;
    CAMERA&                  m_currentCamera;
    TRACK_BALL               m_trackBallCamera;

    BOARD*                   m_dummyBoard;
    FOOTPRINT*               m_dummyFootprint;

    std::vector<FP_3DMODEL>* m_parentModelList;
    int                      m_selected;            /// Index into m_parentInfoList

    EDA_UNITS                m_userUnits;

    /// The 3d viewer Render initial settings (must be saved and restored)
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS          m_initialRender;

#ifndef __linux__
    std::unique_ptr<NL_FOOTPRINT_PROPERTIES_PLUGIN>  m_spaceMouse;
#else
    std::unique_ptr<SPNAV_VIEWER_PLUGIN> m_spaceMouse;
#endif
};

#endif  // PANEL_PREVIEW_3D_MODEL_H
