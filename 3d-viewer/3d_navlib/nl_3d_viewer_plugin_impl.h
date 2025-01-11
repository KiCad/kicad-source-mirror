/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 3Dconnexion
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file  nl_3d_viewer_plugin_impl.h
 * @brief Declaration of the NL_3D_VIEWER_PLUGIN_IMPL class
 */

#ifndef NL_3D_VIEWER_PLUGIN_IMPL_H_
#define NL_3D_VIEWER_PLUGIN_IMPL_H_

// TDxWare SDK.
#include <SpaceMouse/CNavigation3D.hpp>

// KiCad
#include <include/plugins/3dapi/xv3d_types.h>

// wx
#include <wx/chartype.h>

// glm
#include <glm/glm.hpp>

// Forward declarations.
class EDA_3D_CANVAS;
class TRACK_BALL;

// temporary store for the categories
typedef std::map<std::string, TDx::CCommandTreeNode*> CATEGORY_STORE;

/**
 * Add a category to the store.
 *
 * The function adds category paths of the format "A.B" where B is a sub-category of A.
 *
 * @param aCategoryPath is the std::string representation of the category.
 * @param aCategoryStore is the CATEGORY_STORE instance to add to.
 * @return a CATEGORY_STORE::iterator where the category was added.
 */
CATEGORY_STORE::iterator add_category( std::string aCategoryPath, CATEGORY_STORE& aCategoryStore );

// Convenience typedef.
typedef TDx::SpaceMouse::Navigation3D::CNavigation3D NAV_3D;

/**
 * The class that implements the accessors and mutators required for
 * 3D navigation in an EDA_3D_CANVAS using a SpaceMouse.
 */
class NL_3D_VIEWER_PLUGIN_IMPL : public NAV_3D
{
public:
    /**
     * Initializes a new instance of the NL_3DVIEWER_PLUGIN.
     *
     *  @param aCanvas is the viewport to be navigated.
     *  @param aProfileHint tells the 3DConnexion UI which profile to use.
     */
    NL_3D_VIEWER_PLUGIN_IMPL( EDA_3D_CANVAS* aCanvas, const std::string& aProfileHint );

    virtual ~NL_3D_VIEWER_PLUGIN_IMPL();

    /**
     * Set the connection to the 3Dconnexion driver to the focus state so that
     * 3DMouse data is routed here.
     *
     * @param aFocus is true to set the connection active.
     */
    void SetFocus( bool aFocus = true );

    /**
     * Get the m_canvas pointer.
     */
    EDA_3D_CANVAS* GetCanvas() const;

    /**
     * Connect plugin implementation to the driver.
     */
    void Connect();

private:
    /**
      * Export the invocable actions and images to the 3Dconnexion UI.
      */
    virtual void exportCommandsAndImages();

    long GetCameraMatrix( navlib::matrix_t& aMatrix ) const override;
    long GetPointerPosition( navlib::point_t& aPosition ) const override;
    long GetViewExtents( navlib::box_t& aExtents ) const override;
    long GetViewFOV( double& aFov ) const override;
    long GetViewFrustum( navlib::frustum_t& aFrustum ) const override;
    long GetIsViewPerspective( navlib::bool_t& aPerspective ) const override;
    long SetCameraMatrix( const navlib::matrix_t& aMatrix ) override;
    long SetViewExtents( const navlib::box_t& aExtents ) override;
    long SetViewFOV( double aFov ) override;
    long SetViewFrustum( const navlib::frustum_t& aFrustum ) override;
    long GetModelExtents( navlib::box_t& aExtents ) const override;
    long GetSelectionExtents( navlib::box_t& aExtents ) const override;
    long GetSelectionTransform( navlib::matrix_t& aTransform ) const override;
    long GetIsSelectionEmpty( navlib::bool_t& aEmpty ) const override;
    long SetSelectionTransform( const navlib::matrix_t& aMatrix ) override;
    long GetPivotPosition( navlib::point_t& aPosition ) const override;
    long IsUserPivot( navlib::bool_t& aUserPivot ) const override;
    long SetPivotPosition( const navlib::point_t& aPosition ) override;
    long GetPivotVisible( navlib::bool_t& aVisible ) const override;
    long SetPivotVisible( bool aVisible ) override;
    long GetHitLookAt( navlib::point_t& aPosition ) const override;
    long SetHitAperture( double aAperture ) override;
    long SetHitDirection( const navlib::vector_t& aDirection ) override;
    long SetHitLookFrom( const navlib::point_t& aPosition ) override;
    long SetHitSelectionOnly( bool aSelectionOnly ) override;
    long SetActiveCommand( std::string aCommandId ) override;

    long SetSettingsChanged( long aChangeNumber ) override;
    long SetMotionFlag( bool aValue ) override;
    long SetTransaction( long aValue ) override;
    long SetCameraTarget( const navlib::point_t& aPosition ) override;

    long GetFrontView( navlib::matrix_t& aMatrix ) const override;
    long GetCoordinateSystem( navlib::matrix_t& aMatrix ) const override;
    long GetIsViewRotatable( navlib::bool_t& isRotatable ) const override;


private:
    EDA_3D_CANVAS* m_canvas;
    TRACK_BALL*    m_camera;
    bool           m_capIsMoving;
    double         m_newWidth;
    SFVEC3F        m_rayOrigin;
    SFVEC3F        m_rayDirection;

    // The cached CAMERA affine matrix. This is used to determine if the camera has been
    // moved. The camera matrix is the world to camera affine.
    mutable glm::mat4 m_cameraMatrix;


    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_NL_3DVIEWER_PLUGIN".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar* m_logTrace;
};

#endif // NL_3D_VIEWER_PLUGIN_IMPL_H_
