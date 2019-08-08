/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2015-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file panel_prev_model.h
 * @brief Defines a panel which is to be added to a wxFileDialog via
 * SetExtraControl();
 * The panel shows a preview of the module being edited and provides controls
 * to set the offset/rotation/scale of each model 3d shape as per KiCad's
 * current behavior. The panel may also be used in the 3D configuration dialog
 * to tune the positioning of the models without invoking a file selector dialog.
 */

#ifndef PANEL_PREV_MODEL_H
#define PANEL_PREV_MODEL_H

#include "panel_prev_3d_base.h"

#include <vector>

#include <3d_canvas/eda_3d_canvas.h>

// Define min and max parameter values
#define MAX_SCALE          10000.0
#define MAX_ROTATION       180.0
#define MAX_OFFSET         1000.0

#define SCALE_INCREMENT_FINE       0.02
#define SCALE_INCREMENT            0.1

#define ROTATION_INCREMENT 90            // in degrees, for spin button command
#define ROTATION_INCREMENT_WHEEL 90      // in degrees, for mouse wheel command
#define ROTATION_INCREMENT_WHEEL_FINE 1  // in degrees, for mouse wheel command

#define OFFSET_INCREMENT_MM        0.5
#define OFFSET_INCREMENT_MM_FINE   0.1

#define OFFSET_INCREMENT_MIL       25.0
#define OFFSET_INCREMENT_MIL_FINE  5.0


// Declared classes to create pointers
class S3D_CACHE;
class FILENAME_RESOLVER;
class BOARD;
class CINFO3D_VISU;
class MODULE;
class COLORS_DESIGN_SETTINGS;

class PANEL_PREV_3D: public PANEL_PREV_3D_BASE
{
public:
    PANEL_PREV_3D( wxWindow* aParent, PCB_BASE_FRAME* aFrame, MODULE* aModule,
                   std::vector<MODULE_3D_SETTINGS> *aParentModelList );

    ~PANEL_PREV_3D();

private:
    EDA_3D_CANVAS*                   m_previewPane;
    CINFO3D_VISU*                    m_settings3Dviewer;

    BOARD*                           m_dummyBoard;
    MODULE*                          m_dummyModule;

    std::vector<MODULE_3D_SETTINGS>* m_parentModelList;
    int                              m_selected;   /// Index into m_parentInfoList

    EDA_UNITS_T                      m_userUnits;

    // Methods of the class
private:
    void initPanel();

    /**
     * Load 3D relevant settings from the user configuration
     */
    void loadCommonSettings();

    /**
     * @brief updateOrientation - it will receive the events from editing the fields
     * @param event
     */
    void updateOrientation( wxCommandEvent &event ) override;

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

    void doIncrementScale( wxSpinEvent& aEvent, double aSign );
    void doIncrementRotation( wxSpinEvent& aEvent, double aSign );
    void doIncrementOffset( wxSpinEvent& aEvent, double aSign );

    wxString formatScaleValue( double aValue );
    wxString formatRotationValue( double aValue );
    wxString formatOffsetValue( double aValue );

	void View3DISO( wxCommandEvent& event ) override
    {
        m_settings3Dviewer->CameraGet().ToggleProjection();
        m_previewPane->Refresh();
    }

	void View3DLeft( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( GR_KB_SHIFT + 'X' );
    }

	void View3DFront( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'Y' );
    }

	void View3DTop( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'Z' );
    }

	void View3DUpdate( wxCommandEvent& event ) override
    {
        m_previewPane->ReloadRequest();
        m_previewPane->Refresh();
    }

	void View3DRight( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'X' );
    }

	void View3DBack( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( GR_KB_SHIFT + 'Y' );
    }

	void View3DBottom( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( GR_KB_SHIFT + 'Z' );
    }

public:
    /**
     * @brief SetModelDataIdx - Sets the currently selected index in the model list so that
     * the scale/rotation/offset controls can be updated.
     */
    void SetSelectedModel( int idx );

    /**
     * @brief UpdateModelInfoList - copy shapes from the current shape list which are flagged
     * for preview to the copy of module that is on the preview dummy board
     */
    void UpdateDummyModule( bool aRelaodRequired = true );
};

#endif  // PANEL_PREV_MODEL_H
