/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  c3d_model_viewer.h
 * @brief Implements a model viewer canvas. The propose of model viewer is to
 * render 3d models that come in the original data from the files without any
 * transformations.
 */

#ifndef _C3D_MODEL_VIEWER_H_
#define _C3D_MODEL_VIEWER_H_

#include "3d_rendering/ctrack_ball.h"
#include <wx/glcanvas.h>

class S3D_CACHE;
class C_OGL_3DMODEL;

/**
 *  Class C3D_MODEL_VIEWER
 *  Implement a canvas based on a wxGLCanvas
 */
class C3D_MODEL_VIEWER : public wxGLCanvas
{

public:


    /**
     *  Creates a new 3D Canvas with a attribute list
     *  @param aParent = the parent creator of this canvas
     *  @param aAttribList = a list of openGL options created by
     *  COGL_ATT_LIST::GetAttributesList
     */
    C3D_MODEL_VIEWER( wxWindow *aParent,
                      const int *aAttribList  = 0,
                      S3D_CACHE *aCacheManager = NULL );

    ~C3D_MODEL_VIEWER();

    /**
     * @brief Set3DModel - Set this model to be displayed
     * @param a3DModel - 3d model data
     */
    void Set3DModel( const S3DMODEL &a3DModel );

    /**
     * @brief Set3DModel - Set this model to be displayed
     * @param aModelPathName - 3d model path name
     */
    void Set3DModel( wxString const& aModelPathName );

    /**
     * @brief Clear3DModel - Unloads the displayed 3d model
     */
    void Clear3DModel();

private:
    void ogl_initialize();
    void ogl_set_arrow_material();

private:

    void OnPaint( wxPaintEvent &event );

    void OnEraseBackground( wxEraseEvent &event );

    void OnMouseWheel( wxMouseEvent &event );

#ifdef USE_OSX_MAGNIFY_EVENT
    void   OnMagnify( wxMouseEvent& event );
#endif

    void OnMouseMove( wxMouseEvent &event );

    void OnLeftDown( wxMouseEvent &event );

    void OnLeftUp( wxMouseEvent &event );

    void OnMiddleUp( wxMouseEvent &event );

    void OnMiddleDown( wxMouseEvent &event );

    void OnRightClick( wxMouseEvent &event );

    DECLARE_EVENT_TABLE();


private:
    /// openGL context
    wxGLContext *m_glRC;

    /// Camera used in this canvas
    CTRACK_BALL m_trackBallCamera;

    /// Original 3d model data
    const S3DMODEL *m_3d_model;

    /// Class holder for 3d model to display on openGL
    C_OGL_3DMODEL  *m_ogl_3dmodel;

    /// Flag that we have a new model and it need to be reloaded when the paint is called
    bool m_reload_is_needed;

    /// Flag if open gl was initialized
    bool m_ogl_initialized;

    /// factor to convert the model or any other items to keep it in relation to
    /// the +/-RANGE_SCALE_3D
    /// (it is named same as the board render for better understanding proposes)
    double m_BiuTo3Dunits;

    /// Optional cache manager
    S3D_CACHE* m_cacheManager;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_3D_MODEL_VIEWER".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar *m_logTrace;
};

#endif // _C3D_MODEL_VIEWER_H_
