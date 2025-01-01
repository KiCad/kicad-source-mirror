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
 * @file camera.h
 * @brief Define an abstract camera.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <gal/gal.h>
#include <plugins/3dapi/xv3d_types.h>
#include <wx/gdicmn.h>  // for wxSize
#include <vector>
#include <3d_enums.h>

enum class PROJECTION_TYPE
{
    ORTHO = 0,
    PERSPECTIVE
};

/**
 * Frustum is a implementation based on a tutorial by
 * http://www.lighthouse3d.com/tutorials/view-frustum-culling/
 */
struct CAMERA_FRUSTUM
{
    SFVEC3F nc;
    SFVEC3F fc;
    SFVEC3F ntl; ///< Near Top Left
    SFVEC3F ntr; ///< Near Top Right
    SFVEC3F nbl; ///< Near Bottom Left
    SFVEC3F nbr; ///< Near Bottom Right
    SFVEC3F ftl; ///< Far Top Left
    SFVEC3F ftr; ///< Far Top Right
    SFVEC3F fbl; ///< Far Bottom Left
    SFVEC3F fbr; ///< Far Bottom Right
    float nearD, farD, ratio, angle, tang;
    float nw, nh, fw, fh;

    CAMERA_FRUSTUM() :
        nc( { 0.0, 0.0, 0.0 } ),
        fc( { 0.0, 0.0, 0.0 } ),
        ntl( { 0.0, 0.0, 0.0 } ),
        ntr( { 0.0, 0.0, 0.0 } ),
        nbl( { 0.0, 0.0, 0.0 } ),
        nbr( { 0.0, 0.0, 0.0 } ),
        ftl( { 0.0, 0.0, 0.0 } ),
        ftr( { 0.0, 0.0, 0.0 } ),
        fbl( { 0.0, 0.0, 0.0 } ),
        fbr( { 0.0, 0.0, 0.0 } ),
        nearD( 0.0 ),
        farD( 0.0 ),
        ratio( 1.0 ),
        angle( 0.0 ),
        tang( 0.0 ),
        nw( 0.0 ),
        nh( 0.0 ),
        fw( 0.0 ),
        fh( 0.0 )
    {
    }
};


enum class CAMERA_INTERPOLATION
{
    LINEAR,
    EASING_IN_OUT, // Quadratic
    BEZIER,
};


/**
 *  A class used to derive camera objects from.
 *
 *  It must be derived by other classes to implement a real camera object.
 */
class GAL_API CAMERA
{
public:
    static const float DEFAULT_MIN_ZOOM;
    static const float DEFAULT_MAX_ZOOM;

    /**
     * Initialize a camera.
     *
     * @param aInitialDistance Initial Z-distance to the board
     */
    explicit CAMERA( float aInitialDistance );
    explicit CAMERA( SFVEC3F aInitPos, SFVEC3F aLookat, PROJECTION_TYPE aProjectionType );

    virtual ~CAMERA()
    {
    }

    /**
     *  Get the rotation matrix to be applied in a transformation camera.
     *
     *  @return the rotation matrix of the camera
     */
    glm::mat4 GetRotationMatrix() const;

    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetViewMatrix_Inv() const;

    const glm::mat4& GetProjectionMatrix() const;
    const glm::mat4& GetProjectionMatrixInv() const;

    const SFVEC3F& GetRight()    const { return m_right; }
    const SFVEC3F& GetUp()       const { return m_up; }
    const SFVEC3F& GetDir()      const { return m_dir; }
    const SFVEC3F& GetPos()      const { return m_pos; }
    const SFVEC2F& GetFocalLen() const { return m_focalLen; }
    float GetNear() const { return m_frustum.nearD; }
    float GetFar() const { return m_frustum.farD; }
    const CAMERA_FRUSTUM& GetFrustum() const { return m_frustum; }
    const SFVEC3F&        GetLookAtPos() const { return m_lookat_pos; }

    /**
     *  Set the rotation matrix to be applied in a transformation camera, without
     *  making any new calculations on camera.
     *
     *  @param aRotation is the total rotation matrix of the camera.
     */
    void SetRotationMatrix( const glm::mat4& aRotation );

    /**
     *  Set the affine matrix to be applied to a transformation camera.
     *
     *  @param aViewMatrix is the affine matrix of the camera. The affine matrix
     *                     maps coordinates in the world frame to those in the
     *                     camera frame.
     */
    void SetViewMatrix( glm::mat4 aViewMatrix );

    void SetBoardLookAtPos( const SFVEC3F& aBoardPos );

    void SetLookAtPos_T1( const SFVEC3F& aLookAtPos )
    {
        m_lookat_pos_t1 = aLookAtPos;
    }

    const SFVEC3F& GetLookAtPos_T1() const { return m_lookat_pos_t1; }

    const SFVEC3F& GetCameraPos() const { return m_camera_pos; }
    const SFVEC3F& GetCameraInitPos() const { return m_camera_pos_init; }

    float GetCameraMinDimension() const;

    /**
     *  Calculate a new mouse drag position
     */
    virtual void Drag( const wxPoint& aNewMousePosition )  = 0;

    virtual void Pan( const wxPoint& aNewMousePosition )  = 0;

    virtual void Pan( const SFVEC3F& aDeltaOffsetInc )  = 0;

    virtual void Pan_T1( const SFVEC3F& aDeltaOffsetInc )  = 0;

    /**
     *  Reset the camera to initial state
     */
    virtual void Reset();
    virtual void Reset_T1();

    void ResetXYpos();
    void ResetXYpos_T1();

    /**
     *  Get the current mouse position.
     */
    const wxPoint& GetCurMousePosition() { return m_lastPosition; }

    /**
     *  Update the current mouse position without make any new calculations on camera.
     */
    void SetCurMousePosition( const wxPoint& aPosition );

    void ToggleProjection();
    PROJECTION_TYPE GetProjection() { return m_projectionType; }
    void SetProjection( PROJECTION_TYPE aProjection ) { m_projectionType = aProjection; }

    /**
     * Update the windows size of the camera.
     *
     * @return true if the windows size changed since last time.
     */
    bool SetCurWindowSize( const wxSize& aSize );

    void ZoomReset();

    bool Zoom( float aFactor );

    bool Zoom_T1( float aFactor );

    float GetZoom() const { return m_zoom; }

    float GetMinZoom() { return m_minZoom; }
    void  SetMinZoom( float minZoom )
    {
        m_minZoom = minZoom;
        zoomChanged();
    }

    float GetMaxZoom() { return m_maxZoom; }
    void  SetMaxZoom( float maxZoom )
    {
        m_maxZoom = maxZoom;
        zoomChanged();
    }

    bool ViewCommand_T1( VIEW3D_TYPE aRequestedView );

    /**
     * Rotates the camera in screen plane.
     */
    void RotateScreen( float aAngleInRadians );

    void RotateX( float aAngleInRadians );
    void RotateY( float aAngleInRadians );
    void RotateZ( float aAngleInRadians );

    void RotateX_T1( float aAngleInRadians );
    void RotateY_T1( float aAngleInRadians );
    void RotateZ_T1( float aAngleInRadians );

    /**
     * This will set T0 and T1 with the current values.
     */
    virtual void SetT0_and_T1_current_T();

    /**
     * It will update the matrix to interpolate between T0 and T1 values.
     *
     * @param t the interpolation time, between 0.0f and 1.0f (it will clamp if >1).
     */
    virtual void Interpolate( float t );

    void SetInterpolateMode( CAMERA_INTERPOLATION aInterpolateMode )
    {
        m_interpolation_mode = aInterpolateMode;
    }

    /**
     *  @return true if some of the parameters in camera was changed, it will reset the flag.
     */
    bool ParametersChanged();

    /**
     *  @return true if some of the parameters in camera was changed, it will NOT reset the flag.
     */
    bool ParametersChangedQuery() const { return m_parametersChanged; }

    /**
     * Make a ray based on a windows screen position.
     *
     * @param aWindowPos the windows buffer position.
     * @param aOutOrigin out origin position of the ray.
     * @param aOutDirection out direction
     */
    void MakeRay( const SFVEC2I& aWindowPos, SFVEC3F& aOutOrigin, SFVEC3F& aOutDirection ) const;

    /**
     * Make a ray based on a windows screen position, it will interpolate based on the
     * \a aWindowPos.
     *
     * @param aWindowPos the windows buffer position (float value).
     * @param aOutOrigin out origin position of the ray.
     * @param aOutDirection out direction.
     */
    void MakeRay( const SFVEC2F& aWindowPos, SFVEC3F& aOutOrigin, SFVEC3F& aOutDirection ) const;

    /**
     * Make a ray based on the latest mouse position.
     *
     * @param aOutOrigin out origin position of the ray.
     * @param aOutDirection out direction.
     */
    void MakeRayAtCurrentMousePosition( SFVEC3F& aOutOrigin, SFVEC3F& aOutDirection ) const;

    /**
     * Update the camera.
     */
    void Update() { updateFrustum(); }

protected:
    void zoomChanged();
    void rebuildProjection();
    void updateFrustum();
    void updateViewMatrix();

    void updateRotationMatrix();

    /**
     * 3D zoom value -- Z-distance is scaled by it
     */
    float m_zoom;
    float m_zoom_t0;
    float m_zoom_t1;

    /**
     * Possible 3D zoom range
     */
    float m_minZoom;
    float m_maxZoom;

    /**
     * The window size that this camera is working.
     */
    SFVEC2I m_windowSize;

    /**
     * The last mouse position in the screen
     */
    wxPoint m_lastPosition;

    glm::mat4 m_rotationMatrix;
    glm::mat4 m_rotationMatrixAux;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_viewMatrixInverse;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_projectionMatrixInv;
    PROJECTION_TYPE m_projectionType;

    CAMERA_FRUSTUM m_frustum;

    SFVEC3F m_right;
    SFVEC3F m_up;
    SFVEC3F m_dir;
    SFVEC3F m_pos;

    SFVEC2F m_focalLen;

    SFVEC3F m_camera_pos_init;
    SFVEC3F m_camera_pos;
    SFVEC3F m_camera_pos_t0;
    SFVEC3F m_camera_pos_t1;

    SFVEC3F m_lookat_pos;
    SFVEC3F m_lookat_pos_t0;
    SFVEC3F m_lookat_pos_t1;
    SFVEC3F m_board_lookat_pos_init;    ///< Default boardlookat position (the board center).

    SFVEC3F m_rotate_aux;               ///< Stores the rotation angle auxiliary.
    SFVEC3F m_rotate_aux_t0;
    SFVEC3F m_rotate_aux_t1;

    CAMERA_INTERPOLATION m_interpolation_mode;

    /**
     * Precalc values array used to calc ray for each pixel (constant for the same window size).
     */
    std::vector< float > m_scr_nX;
    std::vector< float > m_scr_nY;

    /**
     * Precalc values array used to calc ray for each pixel, for X and Y axis of each new
     * camera position.
     */
    std::vector< SFVEC3F > m_right_nX;
    std::vector< SFVEC3F > m_up_nY;

    /**
     * Set to true if any of the parameters in the camera was changed
     */
    bool m_parametersChanged;

    /**
     * Trace mask used to enable or disable the trace output of this class.
     *
     * The debug output can be turned on by setting the WXTRACE environment variable to
     * "KI_TRACE_CAMERA".  See the wxWidgets documentation on wxLogTrace for
     * more information.
     */
    static const wxChar* m_logTrace;
};

#endif // CAMERA_H
