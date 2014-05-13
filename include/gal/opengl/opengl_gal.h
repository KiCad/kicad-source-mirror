/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Graphics Abstraction Layer (GAL) for OpenGL
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

#ifndef OPENGLGAL_H_
#define OPENGLGAL_H_

// GAL imports
#include <gal/graphics_abstraction_layer.h>
#include <gal/opengl/shader.h>
#include <gal/opengl/vertex_manager.h>
#include <gal/opengl/vertex_item.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/opengl_compositor.h>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include <cmath>
#include <iterator>
#include <vector>
#include <algorithm>
#include <memory>
#include <map>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/shared_array.hpp>

#include <stdlib.h>
#include <iostream>
#include <fstream>

#ifndef CALLBACK
#define CALLBACK
#endif

namespace KIGFX
{
class SHADER;

/**
 * @brief Class OpenGL_GAL is the OpenGL implementation of the Graphics Abstraction Layer.
 *
 * This is a direct OpenGL-implementation and uses low-level graphics primitives like triangles
 * and quads. The purpuse is to provide a fast graphics interface, that takes advantage of modern
 * graphics card GPUs. All methods here benefit thus from the hardware acceleration.
 */
class OPENGL_GAL : public GAL, public wxGLCanvas
{
public:

    /**
     * @brief Constructor OPENGL_GAL
     *
     * @param aParent is the wxWidgets immediate wxWindow parent of this object.
     *
     * @param aMouseListener is the wxEvtHandler that should receive the mouse events,
     *  this can be can be any wxWindow, but is often a wxFrame container.
     *
     * @param aPaintListener is the wxEvtHandler that should receive the paint
     *  event.  This can be any wxWindow, but is often a derived instance
     *  of this class or a containing wxFrame.  The "paint event" here is
     *  a wxCommandEvent holding EVT_GAL_REDRAW, as sent by PostPaint().
     *
     * @param aName is the name of this window for use by wxWindow::FindWindowByName()
     */
    OPENGL_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener = NULL,
                wxEvtHandler* aPaintListener = NULL, const wxString& aName = wxT( "GLCanvas" ) );

    virtual ~OPENGL_GAL();

    // ---------------
    // Drawing methods
    // ---------------

    /// @copydoc GAL::BeginDrawing()
    virtual void BeginDrawing();

    /// @copydoc GAL::EndDrawing()
    virtual void EndDrawing();

    /// @copydoc GAL::DrawLine()
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawSegment()
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                              double aWidth );

    /// @copydoc GAL::DrawCircle()
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius );

    /// @copydoc GAL::DrawArc()
    virtual void DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                          double aStartAngle, double aEndAngle );

    /// @copydoc GAL::DrawRectangle()
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawPolyline()
    virtual void DrawPolyline( std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawPolygon()
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawCurve()
    virtual void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint );

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen( int aWidth, int aHeight );

    /// @brief Shows/hides the GAL canvas
    virtual bool Show( bool aShow );

    /// @copydoc GAL::Flush()
    virtual void Flush();

    /// @copydoc GAL::ClearScreen()
    virtual void ClearScreen();

    // --------------
    // Transformation
    // --------------

    /// @copydoc GAL::Transform()
    virtual void Transform( const MATRIX3x3D& aTransformation );

    /// @copydoc GAL::Rotate()
    virtual void Rotate( double aAngle );

    /// @copydoc GAL::Translate()
    virtual void Translate( const VECTOR2D& aTranslation );

    /// @copydoc GAL::Scale()
    virtual void Scale( const VECTOR2D& aScale );

    /// @copydoc GAL::Save()
    virtual void Save();

    /// @copydoc GAL::Restore()
    virtual void Restore();

    // --------------------------------------------
    // Group methods
    // ---------------------------------------------

    /// @copydoc GAL::BeginGroup()
    virtual int BeginGroup();

    /// @copydoc GAL::EndGroup()
    virtual void EndGroup();

    /// @copydoc GAL::DrawGroup()
    virtual void DrawGroup( int aGroupNumber );

    /// @copydoc GAL::ChangeGroupColor()
    virtual void ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor );

    /// @copydoc GAL::ChangeGroupDepth()
    virtual void ChangeGroupDepth( int aGroupNumber, int aDepth );

    /// @copydoc GAL::DeleteGroup()
    virtual void DeleteGroup( int aGroupNumber );

    /// @copydoc GAL::ClearCache()
    virtual void ClearCache();

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @copydoc GAL::SaveScreen()
    virtual void SaveScreen();

    /// @copydoc GAL::RestoreScreen()
    virtual void RestoreScreen();

    /// @copydoc GAL::SetTarget()
    virtual void SetTarget( RENDER_TARGET aTarget );

    /// @copydoc GAL::GetTarget()
    virtual RENDER_TARGET GetTarget() const;

    /// @copydoc GAL::ClearTarget()
    virtual void ClearTarget( RENDER_TARGET aTarget );

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::DrawCursor()
    virtual void DrawCursor( const VECTOR2D& aCursorPosition );

    /**
     * @brief Function PostPaint
     * posts an event to m_paint_listener.  A post is used so that the actual drawing
     * function can use a device context type that is not specific to the wxEVT_PAINT event.
     */
    void PostPaint()
    {
        if( paintListener )
        {
            wxPaintEvent redrawEvent;
            wxPostEvent( paintListener, redrawEvent );
        }
    }

    void SetMouseListener( wxEvtHandler* aMouseListener )
    {
        mouseListener = aMouseListener;
    }

    void SetPaintListener( wxEvtHandler* aPaintListener )
    {
        paintListener = aPaintListener;
    }

    ///< Parameters passed to the GLU tesselator
    typedef struct
    {
        /// Manager used for storing new vertices
        VERTEX_MANAGER* vboManager;

        /// Intersect points, that have to be freed after tessellation
        std::deque< boost::shared_array<GLdouble> >& intersectPoints;
    } TessParams;

protected:
    virtual void drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

private:
    /// Super class definition
    typedef GAL super;

    static const int    CIRCLE_POINTS   = 64;   ///< The number of points for circle approximation
    static const int    CURVE_POINTS    = 32;   ///< The number of points for curve approximation

    wxClientDC*             clientDC;               ///< Drawing context
    wxGLContext*            glContext;              ///< OpenGL context of wxWidgets
    wxWindow*               parentWindow;           ///< Parent window
    wxEvtHandler*           mouseListener;
    wxEvtHandler*           paintListener;

    // Vertex buffer objects related fields
    typedef std::map< unsigned int, boost::shared_ptr<VERTEX_ITEM> > GROUPS_MAP;
    GROUPS_MAP              groups;                 ///< Stores informations about VBO objects (groups)
    unsigned int            groupCounter;           ///< Counter used for generating keys for groups
    VERTEX_MANAGER*         currentManager;         ///< Currently used VERTEX_MANAGER (for storing VERTEX_ITEMs)
    VERTEX_MANAGER          cachedManager;          ///< Container for storing cached VERTEX_ITEMs
    VERTEX_MANAGER          nonCachedManager;       ///< Container for storing non-cached VERTEX_ITEMs
    VERTEX_MANAGER          overlayManager;         ///< Container for storing overlaid VERTEX_ITEMs

    // Framebuffer & compositing
    OPENGL_COMPOSITOR       compositor;             ///< Handles multiple rendering targets
    unsigned int            mainBuffer;             ///< Main rendering target
    unsigned int            overlayBuffer;          ///< Auxiliary rendering target (for menus etc.)
    RENDER_TARGET           currentTarget;          ///< Current rendering target

    // Shader
    SHADER                  shader;         ///< There is only one shader used for different objects

    // Internal flags
    bool                    isGlewInitialized;          ///< Is GLEW initialized?
    bool                    isFramebufferInitialized;   ///< Are the framebuffers initialized?
    bool                    isShaderInitialized;        ///< Was the shader initialized?
    bool                    isGrouping;                 ///< Was a group started?

    // Polygon tesselation
    /// The tessellator
    GLUtesselator*          tesselator;
    /// Storage for intersecting points
    std::deque< boost::shared_array<GLdouble> > tessIntersects;

    /**
     * @brief Draw a quad for the line.
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     */
    void drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /**
     * @brief Draw a semicircle. Depending on settings (isStrokeEnabled & isFilledEnabled) it runs
     * the proper function (drawStrokedSemiCircle or drawFilledSemiCircle).
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semicircle.
     * @param aAngle is the angle of the semicircle.
     *
     */
    void drawSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle );

    /**
     * @brief Draw a filled semicircle.
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semicircle.
     * @param aAngle is the angle of the semicircle.
     *
     */
    void drawFilledSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle );

    /**
     * @brief Draw a stroked semicircle.
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semicircle.
     * @param aAngle is the angle of the semicircle.
     *
     */
    void drawStrokedSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle );

    // Event handling
    /**
     * @brief This is the OnPaint event handler.
     *
     * @param aEvent is the OnPaint event.
     */
    void onPaint( wxPaintEvent& aEvent );

    /**
     * @brief Skip the mouse event to the parent.
     *
     * @param aEvent is the mouse event.
     */
    void skipMouseEvent( wxMouseEvent& aEvent );

    /// Initialize GLEW
    void initGlew();

    /**
     * @brief Blits cursor into the current screen.
     */
    void blitCursor();

    /**
     * @brief Returns a valid key that can be used as a new group number.
     *
     * @return An unique group number that is not used by any other group.
     */
    unsigned int getNewGroupNumber();
};
} // namespace KIGFX

#endif  // OPENGLGAL_H_
