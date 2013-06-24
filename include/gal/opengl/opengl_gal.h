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
#include <GL/glew.h>

// OpenGL mathematics library
#define GLM_FORCE_RADIANS
#include <gal/opengl/glm/gtc/matrix_transform.hpp>

#include <gal/opengl/vbo_container.h>
#include <gal/opengl/shader.h>

// wxWidgets imports
#include <wx/wx.h>
#include <wx/glcanvas.h>

// STL imports
#include <cmath>
#include <iterator>
#include <vector>
#include <algorithm>

#include <stdlib.h>
#include <iostream>
#include <fstream>


namespace KiGfx
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
     *
     * @param isUseShaders is a flag, that  indicates, if shaders should be used
     *        for higher quality rendering.
     *
     */
    OPENGL_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener = NULL,
                wxEvtHandler* aPaintListener = NULL, bool isUseShaders = false,
                const wxString& aName = wxT( "GLCanvas" ) );

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
    virtual void DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth );

    /// @copydoc GAL::DrawPolyline()
    virtual void DrawPolyline( std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawCircle()
    virtual void DrawCircle( const VECTOR2D& aCenterPoint, double aRadius );

    /// @copydoc GAL::DrawArc()
    virtual void
    DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle, double aEndAngle );

    /// @copydoc GAL::DrawRectangle()
    virtual void DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /// @copydoc GAL::DrawPolygon()
    virtual void DrawPolygon( const std::deque<VECTOR2D>& aPointList );

    /// @copydoc GAL::DrawCurve()
    virtual void DrawCurve( const VECTOR2D& startPoint, const VECTOR2D& controlPointA,
                            const VECTOR2D& controlPointB, const VECTOR2D& endPoint );

    // --------------
    // Screen methods
    // --------------

    /// @brief Resizes the canvas.
    virtual void ResizeScreen ( int aWidth, int aHeight );

    /// @brief Shows/hides the GAL canvas
    virtual bool Show( bool aShow );

    /// @copydoc GAL::Flush()
    virtual void Flush();

    /// @copydoc GAL::ClearScreen()
    virtual void ClearScreen();

    // -----------------
    // Attribute setting
    // -----------------

    /// @copydoc GAL::SetIsFill()
    virtual void SetIsFill( bool aIsFillEnabled )
    {
        isFillEnabled = aIsFillEnabled;
    }

    /// @copydoc GAL::SetIsStroke()
    virtual void SetIsStroke( bool aIsStrokeEnabled )
    {
        isStrokeEnabled = aIsStrokeEnabled;
    }

    /// @copydoc GAL::SetFillColor()
    virtual void SetFillColor( const COLOR4D& aColor );

    /// @copydoc GAL::SetStrokeColor()
    virtual void SetStrokeColor( const COLOR4D& aColor );

    /// @copydoc GAL::GetStrokeColor()
    COLOR4D      GetStrokeColor();

    /// @copydoc GAL::SetBackgroundColor()
    virtual void SetBackgroundColor( const COLOR4D& aColor );

    /// @copydoc GAL::SetLineCap()
    virtual void SetLineCap( LineCap aLineCap )
    {
        lineCap = aLineCap;
    }

    /// @copydoc GAL::SetLineJoin()
    virtual void SetLineJoin( LineJoin aLineJoin )
    {
        lineJoin = aLineJoin;
    }

    /// @copydoc GAL::SetLineWidth()
    virtual void SetLineWidth( double aLineWidth );

    /// @copydoc GAL::GetLineWidth()
    double       GetLineWidth();

    /// @copydoc GAL::SetLayerDepth()
    virtual void SetLayerDepth( double aLayerDepth )
    {
        super::SetLayerDepth( aLayerDepth );
    }

    // --------------
    // Transformation
    // --------------

    /// @copydoc GAL::Transform()
    virtual void Transform( MATRIX3x3D aTransformation );

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

    /// @copydoc GAL::DeleteGroup()
    virtual void DeleteGroup( int aGroupNumber );

    /// @copydoc GAL::ClearCache()
    virtual void ClearCache();

    // --------------------------------------------------------
    // Handling the world <-> screen transformation
    // --------------------------------------------------------

    /// @copydoc GAL::ComputeWorldScreenMatrix()
    virtual void ComputeWorldScreenMatrix();

    /// @copydoc GAL::GetWorldScreenMatrix()
    MATRIX3x3D GetWorldScreenMatrix();

    /// @copydoc GAL::SetWorldScreenMatrix()
    void SetWorldScreenMatrix( MATRIX3x3D aMatrix );

    /// @copydoc GAL::SetWorldUnitLength()
    void SetWorldUnitLength( double aWorldUnitLength );

    /// @copydoc GAL::SetScreenDPI()
    void SetScreenDPI( double aScreenDPI );

    /// @copydoc GAL::SetLookAtPoint()
    void SetLookAtPoint( const VECTOR2D& aPoint );

    /// @copydoc GAL::GetLookAtPoint()
    VECTOR2D GetLookAtPoint();

    /// @copydoc GAL::SetZoomFactor()
    void SetZoomFactor( double aZoomFactor );

    /// @copydoc GAL::GetZoomFactor()
    double GetZoomFactor();

    /// @copydoc GAL::SaveScreen()
    virtual void SaveScreen();

    /// @copydoc GAL::RestoreScreen()
    virtual void RestoreScreen();

    // -------
    // Cursor
    // -------

    /// @copydoc GAL::ComputeCursorToWorld()
    virtual VECTOR2D ComputeCursorToWorld( const VECTOR2D& aCursorPosition );

    /// @copydoc GAL::SetIsCursorEnabled()
    void SetIsCursorEnabled( bool aIsCursorEnabled );

    /// @copydoc GAL::DrawCursor()
    virtual void DrawCursor( VECTOR2D aCursorPosition );

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

    void SetShaderPath( const std::string& aPath )
    {
        shaderPath = aPath;
    }

    ///< Parameters passed to the GLU tesselator
    typedef struct
    {
        VBO_ITEM* vboItem;                        ///< VBO_ITEM for storing new vertices
        std::vector<GLdouble*>& intersectPoints;  ///< Intersect points, that have to be freed
    } TessParams;

protected:
    virtual void DrawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

private:
    /// Super class definition
    typedef GAL super;

    static const int    CIRCLE_POINTS   = 64;   ///< The number of points for circle approximation
    static const int    CURVE_POINTS    = 32;   ///< The number of points for curve approximation
    static const double MITER_LIMIT     = 1.5;  ///< Limit for mitered edges ( * lineWidth )

    /// This factor is used to for correct merging of antialiased edges,
    /// a very small value is required
    static const double DEPTH_ADJUST_FACTOR = ( 1.0 / (1 << 23) );

    wxClientDC*     clientDC;               ///< Drawing context
    wxGLContext*    glContext;              ///< OpenGL context of wxWidgets
    wxWindow*       parentWindow;           ///< Parent window
    wxEvtHandler*   mouseListener;
    wxEvtHandler*   paintListener;

    // Display lists (used in shaderless mode)
    VBO_CONTAINER*        precomputedContainer;   ///< Container for storing display lists
    GLuint                displayListCircle;      ///< Circle display list
    VBO_ITEM*             verticesCircle;
    GLuint                displayListSemiCircle;  ///< Semi circle display list
    VBO_ITEM*             verticesSemiCircle;

    // Vertex buffer objects related fields
    std::deque<VBO_ITEM*> vboItems;               ///< Stores informations about VBO objects
    VBO_ITEM*             curVboItem;             ///< Currently used VBO_ITEM (for grouping)
    VBO_CONTAINER*        vboContainer;           ///< Container for storing VBO_ITEMs
    GLuint                vboVertices;            ///< Currently used vertices VBO handle
    GLuint                vboIndices;             ///< Currently used indices VBO handle
    bool                  vboNeedsUpdate;         ///< Flag indicating if VBO should be rebuilt
    glm::mat4             transform;              ///< Current transformation matrix
    std::stack<glm::mat4> transformStack;         ///< Stack of transformation matrices
    int                   indicesSize;            ///< Number of indices to be drawn
    GLuint*               indicesPtr;             ///< Pointer to mapped GPU memory

    double                curvePoints[12];        ///< Coefficients for curves
    // FIXME to be removed:
    std::deque<VECTOR2D>  unitCirclePoints;       ///< List of the points on a unit circle

    // Polygon tesselation
    GLUtesselator*        tesselator;             ///< Pointer to the tesselator
    std::vector<GLdouble*> tessIntersects;        ///< Storage of intersecting points

    // Shader
    // Possible types of shaders
    typedef enum
    {
        SHADER_NONE             = 0,
        SHADER_LINE,
        SHADER_FILLED_CIRCLE,
        SHADER_STROKED_CIRCLE,
    } SHADER_TYPE;

    SHADER                shader;           ///< There is only one shader used for different objects
    int                   shaderAttrib;     ///< Location of shader attributes (for glVertexAttribPointer)
    std::string           shaderPath;       ///< Location of shader files

    // Cursor
    int             cursorSize;             ///< Size of the cursor in pixels
    GLubyte*        cursorShape;            ///< Cursor pixel storage
    GLubyte*        cursorSave;             ///< Saved cursor pixels
    bool            isDeleteSavedPixels;    ///< Flag for deleting saved pixels
    VECTOR2D        savedCursorPosition;    ///< Last saved cursor position

    // Frame buffer
    GLuint          frameBuffer;            ///< Main FBO handle
    GLuint          depthBuffer;            ///< Depth buffer handle
    GLuint          texture;                ///< Main texture handle
    GLuint          frameBufferBackup;      ///< Backup FBO handle
    GLuint          depthBufferBackup;      ///< Backup depth buffer handle
    GLuint          textureBackup;          ///< Backup texture handle

    // Internal flags
    bool            isCreated;
    bool            isGlewInitialized;          ///< Is GLEW initialized?
    bool            isFrameBufferInitialized;   ///< Are the frame buffers initialized?
    bool            isVboInitialized;
    bool            isShaderInitialized;        ///< Was the shader initialized?
    bool            isShaderEnabled;            ///< Are the shaders enabled?
    bool            isUseShader;                ///< Should the shaders be used?
    bool            isGrouping;                 ///< Was a group started?

    /**
     * @brief Draw a semi circle (used for line caps).
     *
     * @param aCenterPoint is the center point.
     * @param aRadius is the radius of the semi-circle.
     * @param aAngle is the angle of the semi-circle.
     *
     */
    void drawSemiCircle( const VECTOR2D& aCenterPoint, double aRadius, double aAngle );

    /// Compute the points of a unit circle.
    void computeUnitCircle();

    /// Compute the points of a unit semi circle.
    void computeUnitSemiCircle();

    /// Compute the points of a unit arc.
    // void computeUnitArcs();  // TODO not used

    // Event handling
    /**
     * @brief This is the window creation event handler.
     *
     * @param aEvent is the window creation event.
     */
    void onCreate( wxWindowCreateEvent& aEvent );

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

    /// Initialize GLEW.
    void initGlew();

    /**
     * @brief Initialize the cursor.
     *
     * @param aCursorSize is the cursor size in pixels (screen coordinates).
     */
    void initCursor( int aCursorSize );

    /**
     * @brief Blit the main texture to the screen.
     *
     * @param aIsClearFrameBuffer if true, the frame buffer is cleared as well.
     */
    void blitMainTexture( bool aIsClearFrameBuffer );

    /// @brief Initialize the frame buffers for main contents and backup storage.
    void initFrameBuffers();

    /**
     * @brief Generate a frame buffer for the screen contents.
     *
     * @param aFrameBuffer is the pointer to the frame buffer handle.
     * @param aDepthBuffer is the pointer to the depth buffer handle.
     * @param aTexture is the pointer to the texture handle.
     */
    void generateFrameBuffer( GLuint* aFrameBuffer, GLuint* aDepthBuffer, GLuint* aTexture );

    /**
     * @brief Delete the frame buffer for the screen contents.
     *
     * @param aFrameBuffer is the pointer to the frame buffer handle.
     * @param aDepthBuffer is the pointer to the depth buffer handle.
     * @param aTexture is the pointer to the texture handle.
     */
    void deleteFrameBuffer( GLuint* aFrameBuffer, GLuint* aDepthBuffer, GLuint* aTexture );

    /**
     * @brief Initializes everything needed to use vertex buffer objects.
     */
    void initVertexBufferObjects();

    /**
     * @brief Deinitializes everything when vertex buffer objects are not used anymore.
     */
    void deleteVertexBufferObjects();

    /**
     * @brief Rebuilds vertex buffer object using stored VBO_ITEMS and sends it to
     * the graphics card memory.
     */
    void rebuildVbo();

    /**
     * @brief Draw a quad for the line.
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     */
    inline void drawLineQuad( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    /**
     * @brief Draw the line cap
     *
     * @param aStartPoint is the start point of the line.
     * @param aEndPoint is the end point of the line.
     * @param aDepthOffset is the relative depth of the line cap.
     */
    inline void drawLineCap( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );

    ///< OpenGL replacement functions (that are working both in immediate and VBO modes)
    /**
     * @brief Starts drawing in immediate mode or does nothing if an item's caching has started.
     * @param aMode specifies the primitive or primitives that will be created.
     */
    inline void begin( GLenum aMode )
    {
        if( !isGrouping )
            glBegin( aMode );
    }

    /**
     * @brief Ends drawing in immediate mode or does nothing if an item's caching has started.
     */
    inline void end()
    {
        if( !isGrouping )
            glEnd();
    }

    /**
     * @brief Adds vertex to the current item or draws it in immediate mode.
     * @param aX is X coordinate.
     * @param aY is Y coordinate.
     * @param aZ is Z coordinate.
     */
    inline void vertex3( double aX, double aY, double aZ )
    {
        if( isGrouping )
        {
            // New vertex coordinates for VBO
            VBO_VERTEX vertex = { aX, aY, aZ };
            curVboItem->PushVertex( &vertex );
        }
        else
        {
            glVertex3d( aX, aY, aZ );
        }
    }

    /**
     * @brief Function that replaces glTranslate and behaves according to isGrouping variable.
     * In case isGrouping==false, it is simply glTranslate, in other case it
     * modifies transformation matrix.
     *
     * @param aX is translation in X axis direction.
     * @param aY is translation in Y axis direction.
     * @param aZ is translation in Z axis direction.
     */
    inline void translate3( double aX, double aY, double aZ )
    {
        if( isGrouping )
        {
            transform = glm::translate( transform, glm::vec3( aX, aY, aZ ) );
        }
        else
        {
            glTranslated( aX, aY, aZ );
        }
    }

    /**
     * @brief Function that replaces glColor and behaves according to isGrouping variable.
     * In case isGrouping==false, it is simply glColor, in other case it
     * modifies color used by current VBO_ITEM.
     *
     * @param aR is red component.
     * @param aG is green component.
     * @param aB is blue component.
     * @param aA is alpha component.
     */
    inline void color4( double aRed, double aGreen, double aBlue, double aAlpha )
    {
        if( isGrouping )
        {
            vboContainer->UseColor( aRed, aGreen, aBlue, aAlpha );
        }
        else
        {
            glColor4d( aRed, aGreen, aBlue, aAlpha );
        }
    }

    /**
     * @brief Function that replaces glColor and behaves according to isGrouping variable.
     * In case isGrouping==false, it is simply glColor, in other case it
     * modifies color used by current VBO_ITEM.
     *
     * @param aColor is the new color.
     */
    inline void color4( const COLOR4D& aColor )
    {
        if( isGrouping )
        {
            vboContainer->UseColor( aColor );
        }
        else
        {
            glColor4d( aColor.r, aColor.g, aColor.b, aColor.a);
        }
    }

    /**
     * @brief Function that sets shader and its parameters for the currently used VBO_ITEM.
     * It should be used before adding any vertices that have to be shaded.
     * @param aShader is the type of shader used for vertices.
     * @param aParam[1..3] are shader's parameters. Their meaning depends on the type of used shader.
     * For more information you may check shaders' source code.
     */
    inline void setShader( SHADER_TYPE aShader, GLfloat aParam1 = 0.0f,
                           GLfloat aParam2 = 0.0f, GLfloat aParam3 = 0.0f )
    {
        if( isUseShader && isGrouping )
        {
            const GLfloat shader[] = { aShader, aParam1, aParam2, aParam3 };

            vboContainer->UseShader( shader );
        }
    }
};
} // namespace KiGfx

#endif // OPENGLGAL_H_
