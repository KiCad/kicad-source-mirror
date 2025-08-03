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

#include "nl_3d_viewer_plugin_impl.h"

// 3d-viewer
#include <3d-viewer/3d_rendering/track_ball.h>
#include <3d-viewer/3d_canvas/eda_3d_canvas.h>

// KiCad includes
#include <tool/action_manager.h>
#include <tool/tool_manager.h>
#include <tool/tools_holder.h>

// stdlib
#include <map>
#include <string>
#include <vector>

#include <wx/mstream.h>

/**
 * Flag to enable the NL_3D_VIEWER_PLUGIN debug tracing.
 *
 * Use "KI_TRACE_NL_3D_VIEWER_PLUGIN" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* NL_3D_VIEWER_PLUGIN_IMPL::m_logTrace = wxT( "KI_TRACE_NL_3D_VIEWER_PLUGIN" );


/**
 * Template to compare two glm::mat<T> values for equality within a required epsilon.
 *
 * @param aFirst value to compare.
 * @param aSecond value to compare.
 * @param aEpsilon allowed error.
 * @return true if the values considered equal within the specified epsilon, otherwise false.
 */
template <glm::length_t L, glm::length_t C, class T, glm::qualifier Q>
bool equals( glm::mat<L, C, T, Q> const& aFirst, glm::mat<L, C, T, Q> const& aSecond,
             T aEpsilon = static_cast<T>( FLT_EPSILON * 10 ) )
{
    T const* first = glm::value_ptr( aFirst );
    T const* second = glm::value_ptr( aSecond );

    for( glm::length_t j = 0; j < L * C; ++j )
    {
        if( !equals( first[j], second[j], aEpsilon ) )
        {
            return false;
        }
    }

    return true;
}


NL_3D_VIEWER_PLUGIN_IMPL::NL_3D_VIEWER_PLUGIN_IMPL( EDA_3D_CANVAS* aCanvas,
                                                    const std::string& aProfileHint ) :
        NAV_3D( false, false ),
        m_canvas( aCanvas ),
        m_capIsMoving( false ),
        m_newWidth( 0.0 )
{
    m_camera = dynamic_cast<TRACK_BALL*>( m_canvas->GetCamera() );

    PutProfileHint( aProfileHint );
}


NL_3D_VIEWER_PLUGIN_IMPL::~NL_3D_VIEWER_PLUGIN_IMPL()
{
    EnableNavigation( false );
}


void NL_3D_VIEWER_PLUGIN_IMPL::SetFocus( bool aFocus )
{
    wxLogTrace( m_logTrace, wxT( "NL_3D_VIEWER_PLUGIN_IMPL::SetFocus %d" ), aFocus );
    NAV_3D::Write( navlib::focus_k, aFocus );
}


EDA_3D_CANVAS* NL_3D_VIEWER_PLUGIN_IMPL::GetCanvas() const
{
    return m_canvas;
}


void NL_3D_VIEWER_PLUGIN_IMPL::Connect()
{
   EnableNavigation(true);
   PutFrameTimingSource(TimingSource::SpaceMouse);
   exportCommandsAndImages();
}


CATEGORY_STORE::iterator add_category( std::string aCategoryPath, CATEGORY_STORE& aCategoryStore )
{
    using TDx::SpaceMouse::CCategory;

    CATEGORY_STORE::iterator parent_iter = aCategoryStore.begin();
    std::string::size_type   pos = aCategoryPath.find_last_of( '.' );

    if( pos != std::string::npos )
    {
        std::string parentPath = aCategoryPath.substr( 0, pos );
        parent_iter = aCategoryStore.find( parentPath );

        if( parent_iter == aCategoryStore.end() )
        {
            parent_iter = add_category( parentPath, aCategoryStore );
        }
    }

    std::string                name = aCategoryPath.substr( pos + 1 );
    std::unique_ptr<CCategory> categoryNode =
            std::make_unique<CCategory>( aCategoryPath.c_str(), name.c_str() );

    CATEGORY_STORE::iterator iter = aCategoryStore.insert(
            aCategoryStore.end(), CATEGORY_STORE::value_type( aCategoryPath, categoryNode.get() ) );

    parent_iter->second->push_back( std::move( categoryNode ) );
    return iter;
}


void NL_3D_VIEWER_PLUGIN_IMPL::exportCommandsAndImages()
{
    wxLogTrace( m_logTrace, wxT( "NL_3D_VIEWER_PLUGIN_IMPL::exportCommandsAndImages" ) );

    std::list<TOOL_ACTION*> actions = ACTION_MANAGER::GetActionList();

    if( actions.size() == 0 )
    {
        return;
    }

    using TDx::SpaceMouse::CCommand;
    using TDx::SpaceMouse::CCommandSet;

    // The root action set node
    CCommandSet commandSet( "EDA_3D_CANVAS", "3D Viewer" );

    // Activate the command set
    NAV_3D::PutActiveCommands( commandSet.GetId() );

    // temporary store for the categories
    CATEGORY_STORE categoryStore;

    std::vector<TDx::CImage> vImages;

    // add the action set to the category_store
    categoryStore.insert( categoryStore.end(), CATEGORY_STORE::value_type( ".", &commandSet ) );

    std::list<TOOL_ACTION*>::const_iterator it;

    for( it = actions.begin(); it != actions.end(); ++it )
    {
        const TOOL_ACTION* action = *it;
        std::string        label = action->GetMenuLabel().ToStdString();

        if( label.empty() )
        {
            continue;
        }

        std::string name = action->GetName();

        // Do no export commands for the Pcbnew app.
        if( name.rfind( "pcbnew.", 0 ) == 0 )
        {
            continue;
        }

        std::string              strCategory = action->GetToolName();
        CATEGORY_STORE::iterator iter = categoryStore.find( strCategory );

        if( iter == categoryStore.end() )
        {
            iter = add_category( std::move( strCategory ), categoryStore );
        }

        std::string description = action->GetDescription().ToStdString();

        // Arbitrary 8-bit data stream
        wxMemoryOutputStream imageStream;

        if( action->GetIcon() != BITMAPS::INVALID_BITMAP )
        {
            wxImage image = KiBitmap( action->GetIcon() ).ConvertToImage();
            image.SaveFile( imageStream, wxBitmapType::wxBITMAP_TYPE_PNG );
            image.Destroy();

            if( imageStream.GetSize() )
            {
                wxStreamBuffer* streamBuffer = imageStream.GetOutputStreamBuffer();
                TDx::CImage     tdxImage = TDx::CImage::FromData( "", 0, name.c_str() );
                tdxImage.AssignImage( std::string( reinterpret_cast<const char*>(
                                                           streamBuffer->GetBufferStart() ),
                                                   streamBuffer->GetBufferSize() ),
                                      0 );

                wxLogTrace( m_logTrace, wxT( "Adding image for : %s" ), name );
                vImages.push_back( std::move( tdxImage ) );
            }
        }

        wxLogTrace( m_logTrace, wxT( "Inserting command: %s,  description: %s,  in category:  %s" ),
                    name, description, iter->first );

        iter->second->push_back(
                CCommand( std::move( name ), std::move( label ), std::move( description ) ) );
    }

    NAV_3D::AddCommandSet( commandSet );
    NAV_3D::AddImages( vImages );
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetCameraMatrix( navlib::matrix_t& matrix ) const
{
    // cache the camera matrix so that we can tell if the view has been moved and
    // calculate a delta transform if required.
    m_cameraMatrix = m_camera->GetViewMatrix();

    std::copy_n( glm::value_ptr( glm::inverse( m_cameraMatrix ) ), 16, matrix.m );

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetPointerPosition( navlib::point_t& position ) const
{
    SFVEC3F origin, direction;
    m_camera->MakeRayAtCurrentMousePosition( origin, direction );

    position = { origin.x, origin.y, origin.z };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetViewExtents( navlib::box_t& extents ) const
{
    if( m_camera->GetProjection() == PROJECTION_TYPE::PERSPECTIVE )
    {
        return navlib::make_result_code( navlib::navlib_errc::invalid_operation );
    }

    const CAMERA_FRUSTUM& f = m_camera->GetFrustum();

    double half_width = f.fw / 2.;
    double half_height = f.fh / 2.;
    extents = { -half_width, -half_height, f.nearD, half_width, half_height, f.farD };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetViewFOV( double& aFov ) const
{
    const CAMERA_FRUSTUM& f = m_camera->GetFrustum();

    aFov = glm::radians( f.angle );
    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetViewFrustum( navlib::frustum_t& aFrustum ) const
{
    if( m_camera->GetProjection() != PROJECTION_TYPE::PERSPECTIVE )
    {
        return navlib::make_result_code( navlib::navlib_errc::invalid_operation );
    }

    const CAMERA_FRUSTUM& f = m_camera->GetFrustum();
    double                half_width = f.nw / 2.;
    double                half_height = f.nh / 2.;
    aFrustum = { -half_width, half_width, -half_height, half_height, f.nearD, f.farD };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetIsViewPerspective( navlib::bool_t& perspective ) const
{
    perspective = m_camera->GetProjection() == PROJECTION_TYPE::PERSPECTIVE ? 1 : 0;

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetCameraMatrix( const navlib::matrix_t& aCameraMatrix )
{
    long result = 0;

    glm::mat4 cam, viewMatrix;
    std::copy_n( aCameraMatrix.m, 16, glm::value_ptr( cam ) );
    viewMatrix = glm::inverse( cam );

    glm::mat4 camera = m_camera->GetViewMatrix();

    // The navlib does not move the camera in its z-axis in an orthographic projection
    // as this does not change the viewed object size. However ...

    if( m_camera->GetProjection() == PROJECTION_TYPE::ORTHO )
    {
        // ... the CAMERA class couples zoom and distance to the object: we need to
        // ensure that The CAMERA's z position relative to the lookat_pos is not changed
        // in an orthographic projection.
        glm::vec4 lookat( m_camera->GetLookAtPos(), 1.0f );
        glm::vec4 lookat_new = viewMatrix * lookat;
        glm::vec4 lookat_old = camera * lookat;

        viewMatrix[3].z += lookat_old.z - lookat_new.z;
    }

    if( !equals( camera, m_cameraMatrix ) )
    {
        // Some other input device has moved the camera. Apply only the intended delta
        // transform ...
        m_camera->SetViewMatrix( viewMatrix * glm::inverse( m_cameraMatrix ) * camera );
        m_camera->Update();

        // .., cache the intended camera matrix so that we can calculate the delta
        // transform when needed ...
        m_cameraMatrix = viewMatrix;

        // ... and let the 3DMouse controller know, that something is amiss.
        return navlib::make_result_code( navlib::navlib_errc::error );
    }

    m_camera->SetViewMatrix( viewMatrix );
    m_camera->Update();

    // cache the view matrix so that we know when it has changed.
    m_cameraMatrix = m_camera->GetViewMatrix();

    // The camera has a constraint on the z position so we need to check that ...

    if( !equals( m_cameraMatrix[3].z, viewMatrix[3].z ) )
    {
        // ... and let the 3DMouse controller know, when something is amiss.
        return navlib::make_result_code( navlib::navlib_errc::error );
    }

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetViewExtents( const navlib::box_t& extents )
{
    const CAMERA_FRUSTUM& f = m_camera->GetFrustum();

    float factor = f.nw / ( extents.max_x - extents.min_x );
    float zoom = m_camera->GetZoom() / factor;

    m_camera->Zoom( factor );

    // The camera auto positions the camera to match the zoom values. We need to
    // update our cached camera matrix to match the new z value
    m_cameraMatrix[3].z = m_camera->GetViewMatrix()[3].z;

    // The camera has a constraint on the zoom factor so we need to check that ...
    if( zoom != m_camera->GetZoom() )
    {
        // ... and let the 3DMouse controller know, when something is amiss.
        return navlib::make_result_code( navlib::navlib_errc::error );
    }

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetViewFOV( double fov )
{
    return navlib::make_result_code( navlib::navlib_errc::function_not_supported );
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetViewFrustum( const navlib::frustum_t& frustum )
{
    return navlib::make_result_code( navlib::navlib_errc::permission_denied );
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetModelExtents( navlib::box_t& extents ) const
{
    SFVEC3F min = m_canvas->GetBoardAdapter().GetBBox().Min();
    SFVEC3F max = m_canvas->GetBoardAdapter().GetBBox().Max();

    extents = { min.x, min.y, min.z, max.x, max.y, max.z };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetSelectionExtents( navlib::box_t& extents ) const
{
    return navlib::make_result_code( navlib::navlib_errc::no_data_available );
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetSelectionTransform( navlib::matrix_t& transform ) const
{
    return navlib::make_result_code( navlib::navlib_errc::no_data_available );
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetIsSelectionEmpty( navlib::bool_t& empty ) const
{
    empty = true;

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetSelectionTransform( const navlib::matrix_t& matrix )
{
    return navlib::make_result_code( navlib::navlib_errc::invalid_operation );
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetPivotPosition( navlib::point_t& position ) const
{
    SFVEC3F lap = m_camera->GetLookAtPos();

    position = { lap.x, lap.y, lap.z };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::IsUserPivot( navlib::bool_t& userPivot ) const
{
    userPivot = false;

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetPivotPosition( const navlib::point_t& position )
{
    SFVEC3F pivotPos = SFVEC3F( position.x, position.y, position.z );

    // Set the 3dmouse pivot position.
    m_canvas->Set3dmousePivotPos( pivotPos );

    // Set the camera lookat pos.
    m_camera->SetLookAtPos_T1( pivotPos );

    m_canvas->Request_refresh();

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetPivotVisible( navlib::bool_t& visible ) const
{
    visible = m_canvas->GetRender3dmousePivot();

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetPivotVisible( bool visible )
{
    m_canvas->SetRender3dmousePivot( visible );

    m_canvas->Request_refresh();

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetHitLookAt( navlib::point_t& position ) const
{
    RAY mouseRay;
    mouseRay.Init( m_rayOrigin, m_rayDirection );

    float     hit;
    glm::vec3 vec;

    // Test it with the board bounding box

    if( m_canvas->GetBoardAdapter().GetBBox().Intersect( mouseRay, &hit ) )
    {
        vec = mouseRay.at( hit );
        position = { vec.x, vec.y, vec.z };
        return 0;
    }

    return navlib::make_result_code( navlib::navlib_errc::no_data_available );
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetHitAperture( double aperture )
{
    return navlib::make_result_code( navlib::navlib_errc::function_not_supported );
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetHitDirection( const navlib::vector_t& direction )
{
    m_rayDirection = { direction.x, direction.y, direction.z };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetHitLookFrom( const navlib::point_t& eye )
{
    m_rayOrigin = { eye.x, eye.y, eye.z };

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetHitSelectionOnly( bool onlySelection )
{
    return navlib::make_result_code( navlib::navlib_errc::function_not_supported );
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetActiveCommand( std::string commandId )
{
    if( commandId.empty() )
        return 0;

    std::list<TOOL_ACTION*> actions = ACTION_MANAGER::GetActionList();
    TOOL_ACTION*            context = nullptr;

    for( std::list<TOOL_ACTION*>::const_iterator it = actions.begin(); it != actions.end(); it++ )
    {
        TOOL_ACTION* action = *it;
        std::string  nm = action->GetName();

        if( commandId == nm )
            context = action;
    }

    if( context != nullptr )
    {
        wxWindow* parent = m_canvas->GetParent();

        // Only allow command execution if the window is enabled. i.e. there is not a modal dialog
        // currently active.
        if( parent && parent->IsEnabled() )
        {
            TOOLS_HOLDER* tools_holder = dynamic_cast<TOOLS_HOLDER*>( parent );
            TOOL_MANAGER* tool_manager = tools_holder ? tools_holder->GetToolManager() : nullptr;

            if( !tool_manager )
                return navlib::make_result_code( navlib::navlib_errc::invalid_operation );

            // Get the selection to use to test if the action is enabled
            SELECTION& sel = tool_manager->GetToolHolder()->GetCurrentSelection();

            bool runAction = true;

            if( const ACTION_CONDITIONS* aCond = tool_manager->GetActionManager()->GetCondition( *context ) )
                runAction = aCond->enableCondition( sel );

            if( runAction )
            {
                tool_manager->RunAction( *context );
                m_canvas->Request_refresh();
            }
        }
        else
        {
            return navlib::make_result_code( navlib::navlib_errc::invalid_operation );
        }
    }

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetSettingsChanged( long change )
{
    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetMotionFlag( bool value )
{
    m_capIsMoving = value;

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetTransaction( long value )
{
    if( value != 0L )
    {
    }
    else
    {
        m_canvas->Request_refresh( true );
        wxLogTrace( m_logTrace, wxT( "End of transaction" ) );
    }

    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::SetCameraTarget( const navlib::point_t& position )
{
    return navlib::make_result_code( navlib::navlib_errc::function_not_supported );
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetFrontView( navlib::matrix_t& matrix ) const
{
    matrix = { 1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetCoordinateSystem( navlib::matrix_t& matrix ) const
{
    // Use the right-handed coordinate system X-right, Z-up, Y-in (row vectors)
    matrix = { 1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 };
    return 0;
}


long NL_3D_VIEWER_PLUGIN_IMPL::GetIsViewRotatable( navlib::bool_t& isRotatable ) const
{
    isRotatable = true;
    return 0;
}
