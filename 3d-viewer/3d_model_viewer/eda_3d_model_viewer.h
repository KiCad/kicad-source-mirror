/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file  eda_3d_model_viewer.h
 * @brief Implements a model viewer canvas.
 *
 * The purpose of model viewer is to render 3d models that come in the original data from
 * the files without any transformations.
 */

#ifndef _C3D_MODEL_VIEWER_H_
#define _C3D_MODEL_VIEWER_H_

#include "3d_rendering/track_ball.h"
#include <gal/hidpi_gl_canvas.h>

class S3D_CACHE;
class MODEL_3D;

/**
 *  Implement a canvas based on a wxGLCanvas.
 */
class EDA_3D_MODEL_VIEWER : public HIDPI_GL_CANVAS
{
public:
    /**
     *  Create a new 3D Canvas with a attribute list.
     *
     *  @param aParent the parent creator of this canvas.
     *  @param aGLAttribs openGL attributes created by #OGL_ATT_LIST::GetAttributesList.
     */
    EDA_3D_MODEL_VIEWER( wxWindow* aParent, const wxGLAttributes& aGLAttribs,
                         S3D_CACHE* aCacheManager = nullptr );

    ~EDA_3D_MODEL_VIEWER();

    /**
     * Set this model to be displayed.
     *
     * @param a3DModel 3D model data.
     */
    void Set3DModel( const S3DMODEL& a3DModel );

    /**
     * Set this model to be displayed.
     *
     * N.B. This will not load a model from the internal cache.  Only from on disk.
     *
     * @param aModelPathName 3D model path name.  Must be a file on disk.
     */
    void Set3DModel( const wxString& aModelPathName );

    /**
     * Unload the displayed 3D model.
     */
    void Clear3DModel();

private:
    void ogl_initialize();
    void ogl_set_arrow_material();

    void OnPaint( wxPaintEvent& event );

    void OnEraseBackground( wxEraseEvent& event );

    void OnMouseWheel( wxMouseEvent& event );

#ifdef USE_OSX_MAGNIFY_EVENT
    void   OnMagnify( wxMouseEvent& event );
#endif

    void OnMouseMove( wxMouseEvent& event );

    void OnLeftDown( wxMouseEvent& event );

    void OnLeftUp( wxMouseEvent& event );

    void OnMiddleUp( wxMouseEvent& event );

    void OnMiddleDown( wxMouseEvent& event );

    void OnRightClick( wxMouseEvent& event );

    DECLARE_EVENT_TABLE()

    /// openGL context
    wxGLContext* m_glRC;

    /// Camera used in this canvas
    TRACK_BALL m_trackBallCamera;

    /// Original 3d model data
    const S3DMODEL* m_3d_model;

    /// Class holder for 3d model to display on openGL
    MODEL_3D* m_ogl_3dmodel;

    /// Flag that we have a new model and it need to be reloaded when the paint is called
    bool m_reload_is_needed;

    /// Flag if open gl was initialized
    bool m_ogl_initialized;

    /// factor to convert the model or any other items to keep it in relation to
    /// the +/-RANGE_SCALE_3D
    /// (it is named same as the board render for better understanding proposes)
    double m_BiuTo3dUnits;

    /// Optional cache manager
    S3D_CACHE* m_cacheManager;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_3D_MODEL_VIEWER".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar* m_logTrace;
};

#endif // _C3D_MODEL_VIEWER_H_
