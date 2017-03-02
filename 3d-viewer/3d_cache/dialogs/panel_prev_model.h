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

#include "../3d_info.h"
#include <vector>

#include <3d_canvas/eda_3d_canvas.h>

// Define min and max parameter values
#define MAX_SCALE          10000.0
#define MAX_ROTATION       180.0
#define MAX_OFFSET         1000.0

#define SCALE_INCREMENT_FINE    0.02
#define SCALE_INCREMENT     0.1

#define ROTATION_INCREMENT 5             // in degrees, for spin button command
#define ROTATION_INCREMENT_WHEEL 15      // in degrees, for mouse wheel command
#define ROTATION_INCREMENT_WHEEL_FINE 1  // in degrees, for mouse wheel command

#define OFFSET_INCREMENT_MM   0.5
#define OFFSET_INCREMENT_MM_FINE 0.1

#define OFFSET_INCREMENT_MIL   25.0
#define OFFSET_INCREMENT_MIL_FINE   5.0


// Declared classes to create pointers
class S3D_CACHE;
class S3D_FILENAME_RESOLVER;
class BOARD;
class CINFO3D_VISU;
class MODULE;

class PANEL_PREV_3D: public PANEL_PREV_3D_BASE
{
public:
    PANEL_PREV_3D( wxWindow* aParent, S3D_CACHE* aCacheManager,
                     MODULE* aModuleCopy,
                     std::vector<S3D_INFO> *aParentInfoList = NULL );

    ~PANEL_PREV_3D();

private:
    wxString                currentModelFile;   ///< Used to check if the model file was changed
    S3D_FILENAME_RESOLVER   *m_resolver;        ///< Used to get the full path name

    /// The 3D canvas
    EDA_3D_CANVAS   *m_previewPane;

    /// A dummy board used to store the copy moduled
    BOARD           *m_dummyBoard;

    /// The settings that will be used for this 3D viewer canvas
    CINFO3D_VISU    *m_settings3Dviewer;

    /// A pointer to a new copy of the original module
    MODULE          *m_copyModule;

    /// A pointer to the parent S3D_INFO list that we will use to copy to the preview module
    std::vector<S3D_INFO> *m_parentInfoList;

    /// The current selected index of the S3D_INFO list
    int             m_currentSelectedIdx;

    /// Current S3D_INFO that is being edited
    S3D_INFO        m_modelInfo;

    // Methods of the class
private:
    void initPanel();

    /**
     * @brief updateOrientation - it will receive the events from editing the fields
     * @param event
     */
    void updateOrientation( wxCommandEvent &event ) override;

	void onMouseWheelScale( wxMouseEvent& event ) override;
	void onMouseWheelRot( wxMouseEvent& event ) override;
	void onMouseWheelOffset( wxMouseEvent& event ) override;

	void onIncrementRot( wxSpinEvent& event ) override;
	void onDecrementRot( wxSpinEvent& event ) override;
	void onIncrementScale( wxSpinEvent& event ) override;
	void onDecrementScale( wxSpinEvent& event ) override;
	void onIncrementOffset( wxSpinEvent& event ) override;
	void onDecrementOffset( wxSpinEvent& event ) override;

    /**
     * @brief getOrientationVars - gets the transformation from entries and validate it
     * @param aScale: output scale var
     * @param aRotation: output rotation var
     * @param aOffset: output offset var
     */
    void getOrientationVars( SGPOINT& aScale, SGPOINT& aRotation, SGPOINT& aOffset );

    /**
     * @brief updateListOnModelCopy - copy the current shape list to the copy of module that is on
     * the preview dummy board
     */
    void updateListOnModelCopy();


	void onEnterPreviewCanvas( wxMouseEvent& event )
    {
        m_previewPane->SetFocus();
    }

	void View3DISO( wxCommandEvent& event ) override
    {
        m_settings3Dviewer->CameraGet().ToggleProjection();
        m_previewPane->Refresh();
    }

	void View3DLeft( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'X' );
    }

	void View3DFront( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'Y' );
    }

	void View3DTop( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'z' );
    }

	void View3DUpdate( wxCommandEvent& event ) override
    {
        m_previewPane->ReloadRequest();
        m_previewPane->Refresh();
    }

	void View3DRight( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'x' );
    }

	void View3DBack( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'y' );
    }

	void View3DBottom( wxCommandEvent& event ) override
    {
        m_previewPane->SetView3D( 'Z' );
    }

public:
    /**
     * @brief SetModelDataIdx - This will set the index of the INFO list that was set on the parent.
     * So we will update our values to edit based on the index on that list.
     * @param idx - The index that was selected
     * @param aReloadPreviewModule: if need to update the preview module
     */
    void SetModelDataIdx( int idx, bool aReloadPreviewModule = false );

    /**
     * @brief ResetModelData - Clear the values and reload the preview board
     * @param aReloadPreviewModule: if need to update the preview module
     */
    void ResetModelData( bool aReloadPreviewModule = false );

    void UpdateModelName( wxString const& aModel );

    /**
     * @brief verify X,Y and Z scale factors are acceptable (> 0.001 and < 1000.0)
     * @return false if one (or more) value is not acceptable.
     * @param aErrorMessage is a wxString to store error messages, if any
     */
    bool ValidateWithMessage( wxString& aErrorMessage );

    bool Validate() override
    {
        wxString temp;
        return ValidateWithMessage(temp);
    }
};

#endif  // PANEL_PREV_MODEL_H
