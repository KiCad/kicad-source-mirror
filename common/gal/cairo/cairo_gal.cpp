/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * CAIRO_GAL - Graphics Abstraction Layer for Cairo
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

#include <wx/image.h>
#include <wx/log.h>

#include <gal/cairo/cairo_gal.h>
#include <gal/cairo/cairo_compositor.h>
#include <gal/definitions.h>

#include <limits>

using namespace KIGFX;


const float CAIRO_GAL::LAYER_ALPHA = 0.8;


CAIRO_GAL::CAIRO_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener,
        wxEvtHandler* aPaintListener, const wxString& aName ) :
    wxWindow( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxEXPAND, aName )
{
    parentWindow  = aParent;
    mouseListener = aMouseListener;
    paintListener = aPaintListener;

    // Initialize the flags
    isGrouping          = false;
    isInitialized       = false;
    isDeleteSavedPixels = false;
    validCompositor     = false;
    groupCounter        = 0;

    // Connecting the event handlers
    Connect( wxEVT_PAINT,       wxPaintEventHandler( CAIRO_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION,          wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN,       wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP,         wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK,     wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN,     wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP,       wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK,   wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN,      wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP,        wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK,    wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL,      wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW,    wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
#endif

    SetSize( aParent->GetSize() );
    screenSize = VECTOR2I( aParent->GetSize() );

    cursorPixels = NULL;
    cursorPixelsSaved = NULL;
    initCursor();

    // Grid color settings are different in Cairo and OpenGL
    SetGridColor( COLOR4D( 0.1, 0.1, 0.1, 0.8 ) );

    // Allocate memory for pixel storage
    allocateBitmaps();

    initSurface();
}


CAIRO_GAL::~CAIRO_GAL()
{
    deinitSurface();
    deleteBitmaps();

    delete cursorPixels;
    delete cursorPixelsSaved;

    ClearCache();
}


void CAIRO_GAL::BeginDrawing()
{
    initSurface();

    if( !validCompositor )
        setCompositor();

    compositor->SetMainContext( context );
    compositor->SetBuffer( mainBuffer );

    // Cairo grouping prevents display of overlapping items on the same layer in the lighter color
    cairo_push_group( currentContext );
}


void CAIRO_GAL::EndDrawing()
{
    // Force remaining objects to be drawn
    Flush();

    // Cairo grouping prevents display of overlapping items on the same layer in the lighter color
    cairo_pop_group_to_source( currentContext );
    cairo_paint_with_alpha( currentContext, LAYER_ALPHA );

    // Merge buffers on the screen
    compositor->DrawBuffer( mainBuffer );
    compositor->DrawBuffer( overlayBuffer );

    // This code was taken from the wxCairo example - it's not the most efficient one
    // Here is a good place for optimizations

    // Now translate the raw context data from the format stored
    // by cairo into a format understood by wxImage.
    unsigned char* wxOutputPtr = wxOutput;

    for( size_t count = 0; count < bufferSize; count++ )
    {
        unsigned int value = bitmapBuffer[count];
        *wxOutputPtr++ = ( value >> 16 ) & 0xff;  // Red pixel
        *wxOutputPtr++ = ( value >> 8 ) & 0xff;   // Green pixel
        *wxOutputPtr++ = value & 0xff;            // Blue pixel
    }

    wxImage      img( screenSize.x, screenSize.y, (unsigned char*) wxOutput, true );
    wxBitmap     bmp( img );
    wxClientDC   client_dc( this );
    wxBufferedDC dc;
    dc.Init( &client_dc, bmp );

    // Now it is the time to blit the mouse cursor
    blitCursor( dc );

    deinitSurface();
}


void CAIRO_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    cairo_move_to( currentContext, aStartPoint.x, aStartPoint.y );
    cairo_line_to( currentContext, aEndPoint.x, aEndPoint.y );
    isElementAdded = true;
}


void CAIRO_GAL::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                             double aWidth )
{
    if( isFillEnabled )
    {
        // Filled tracks mode
        SetLineWidth( aWidth );

        cairo_move_to( currentContext, (double) aStartPoint.x, (double) aStartPoint.y );
        cairo_line_to( currentContext, (double) aEndPoint.x, (double) aEndPoint.y );
    }
    else
    {
        // Outline mode for tracks
        VECTOR2D startEndVector = aEndPoint - aStartPoint;
        double   lineAngle      = atan2( startEndVector.y, startEndVector.x );
        double   lineLength     = startEndVector.EuclideanNorm();

        cairo_save( currentContext );

        cairo_translate( currentContext, aStartPoint.x, aStartPoint.y );
        cairo_rotate( currentContext, lineAngle );

        cairo_arc( currentContext, 0.0,        0.0, aWidth / 2.0,  M_PI / 2.0, 3.0 * M_PI / 2.0 );
        cairo_arc( currentContext, lineLength, 0.0, aWidth / 2.0, -M_PI / 2.0, M_PI / 2.0 );

        cairo_move_to( currentContext, 0.0,        aWidth / 2.0 );
        cairo_line_to( currentContext, lineLength, aWidth / 2.0 );

        cairo_move_to( currentContext, 0.0,        -aWidth / 2.0 );
        cairo_line_to( currentContext, lineLength, -aWidth / 2.0 );

        cairo_restore( currentContext );
    }

    isElementAdded = true;
}


void CAIRO_GAL::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    // A circle is drawn using an arc
    cairo_new_sub_path( currentContext );
    cairo_arc( currentContext, aCenterPoint.x, aCenterPoint.y, aRadius, 0.0, 2 * M_PI );

    isElementAdded = true;
}


void CAIRO_GAL::DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                         double aEndAngle )
{
    SWAP( aStartAngle, >, aEndAngle );

    cairo_new_sub_path( currentContext );
    cairo_arc( currentContext, aCenterPoint.x, aCenterPoint.y, aRadius, aStartAngle, aEndAngle );

    isElementAdded = true;
}


void CAIRO_GAL::DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    // Calculate the diagonal points
    VECTOR2D diagonalPointA( aEndPoint.x,  aStartPoint.y );
    VECTOR2D diagonalPointB( aStartPoint.x, aEndPoint.y );

    // The path is composed from 4 segments
    cairo_move_to( currentContext, aStartPoint.x, aStartPoint.y );
    cairo_line_to( currentContext, diagonalPointA.x, diagonalPointA.y );
    cairo_line_to( currentContext, aEndPoint.x, aEndPoint.y );
    cairo_line_to( currentContext, diagonalPointB.x, diagonalPointB.y );
    cairo_close_path( currentContext );

    isElementAdded = true;
}


void CAIRO_GAL::DrawPolyline( std::deque<VECTOR2D>& aPointList )
{
    // Iterate over the point list and draw the segments
    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();

    cairo_move_to( currentContext, it->x, it->y );

    for( ++it; it != aPointList.end(); ++it )
    {
        cairo_line_to( currentContext, it->x, it->y );
    }

    isElementAdded = true;
}


void CAIRO_GAL::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    // Iterate over the point list and draw the polygon
    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();

    cairo_move_to( currentContext, it->x, it->y );

    for( ++it; it != aPointList.end(); ++it )
    {
        cairo_line_to( currentContext, it->x, it->y );
    }

    isElementAdded = true;
}


void CAIRO_GAL::DrawCurve( const VECTOR2D& aStartPoint, const VECTOR2D& aControlPointA,
                           const VECTOR2D& aControlPointB, const VECTOR2D& aEndPoint )
{
    cairo_move_to( currentContext, aStartPoint.x, aStartPoint.y );
    cairo_curve_to( currentContext, aControlPointA.x, aControlPointA.y, aControlPointB.x,
                    aControlPointB.y, aEndPoint.x, aEndPoint.y );
    cairo_line_to( currentContext, aEndPoint.x, aEndPoint.y );

    isElementAdded = true;
}


void CAIRO_GAL::ResizeScreen( int aWidth, int aHeight )
{
    screenSize = VECTOR2I( aWidth, aHeight );

    // Recreate the bitmaps
    deleteBitmaps();
    allocateBitmaps();

    if( validCompositor )
        compositor->Resize( aWidth, aHeight );

    validCompositor = false;

    SetSize( wxSize( aWidth, aHeight ) );
}


bool CAIRO_GAL::Show( bool aShow )
{
    bool s = wxWindow::Show( aShow );

    if( aShow )
        wxWindow::Raise();

    return s;
}


void CAIRO_GAL::Flush()
{
    storePath();
}


void CAIRO_GAL::ClearScreen( const COLOR4D& aColor )
{
    backgroundColor = aColor;
    cairo_set_source_rgb( currentContext, aColor.r, aColor.g, aColor.b );
    cairo_rectangle( currentContext, 0.0, 0.0, screenSize.x, screenSize.y );
    cairo_fill( currentContext );
}


void CAIRO_GAL::SetIsFill( bool aIsFillEnabled )
{
    storePath();
    isFillEnabled = aIsFillEnabled;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_FILL;
        groupElement.boolArgument = aIsFillEnabled;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL::SetIsStroke( bool aIsStrokeEnabled )
{
    storePath();
    isStrokeEnabled = aIsStrokeEnabled;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_STROKE;
        groupElement.boolArgument = aIsStrokeEnabled;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL::SetStrokeColor( const COLOR4D& aColor )
{
    storePath();
    strokeColor = aColor;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_STROKECOLOR;
        groupElement.arguments[0] = strokeColor.r;
        groupElement.arguments[1] = strokeColor.g;
        groupElement.arguments[2] = strokeColor.b;
        groupElement.arguments[3] = strokeColor.a;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL::SetFillColor( const COLOR4D& aColor )
{
    storePath();
    fillColor = aColor;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_FILLCOLOR;
        groupElement.arguments[0] = fillColor.r;
        groupElement.arguments[1] = fillColor.g;
        groupElement.arguments[2] = fillColor.b;
        groupElement.arguments[3] = fillColor.a;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL::SetLineWidth( double aLineWidth )
{
    storePath();

    lineWidth = aLineWidth;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_LINE_WIDTH;
        groupElement.arguments[0] = aLineWidth;
        currentGroup->push_back( groupElement );
    }
    else
    {
        // Make lines appear at least 1 pixel wide, no matter of zoom
        double x = 1.0, y = 1.0;
        cairo_device_to_user_distance( currentContext, &x, &y );
        double minWidth = std::min( fabs( x ), fabs( y ) );
        cairo_set_line_width( currentContext, std::max( aLineWidth, minWidth ) );
    }
}


void CAIRO_GAL::SetLayerDepth( double aLayerDepth )
{
    super::SetLayerDepth( aLayerDepth );

    if( isInitialized )
    {
        storePath();

        cairo_pop_group_to_source( currentContext );
        cairo_paint_with_alpha( currentContext, LAYER_ALPHA );

        cairo_push_group( currentContext );
    }
}


void CAIRO_GAL::Transform( const MATRIX3x3D& aTransformation )
{
    cairo_matrix_t cairoTransformation;

    cairo_matrix_init( &cairoTransformation,
                       aTransformation.m_data[0][0],
                       aTransformation.m_data[1][0],
                       aTransformation.m_data[0][1],
                       aTransformation.m_data[1][1],
                       aTransformation.m_data[0][2],
                       aTransformation.m_data[1][2] );

    cairo_transform( currentContext, &cairoTransformation );
}


void CAIRO_GAL::Rotate( double aAngle )
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_ROTATE;
        groupElement.arguments[0] = aAngle;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_rotate( currentContext, aAngle );
    }
}


void CAIRO_GAL::Translate( const VECTOR2D& aTranslation )
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_TRANSLATE;
        groupElement.arguments[0] = aTranslation.x;
        groupElement.arguments[1] = aTranslation.y;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_translate( currentContext, aTranslation.x, aTranslation.y );
    }
}


void CAIRO_GAL::Scale( const VECTOR2D& aScale )
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SCALE;
        groupElement.arguments[0] = aScale.x;
        groupElement.arguments[1] = aScale.y;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_scale( currentContext, aScale.x, aScale.y );
    }
}


void CAIRO_GAL::Save()
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SAVE;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_save( currentContext );
    }
}


void CAIRO_GAL::Restore()
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_RESTORE;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_restore( currentContext );
    }
}


int CAIRO_GAL::BeginGroup()
{
    initSurface();

    // If the grouping is started: the actual path is stored in the group, when
    // a attribute was changed or when grouping stops with the end group method.
    storePath();

    GROUP group;
    int groupNumber = getNewGroupNumber();
    groups.insert( std::make_pair( groupNumber, group ) );
    currentGroup = &groups[groupNumber];
    isGrouping   = true;

    return groupNumber;
}


void CAIRO_GAL::EndGroup()
{
    storePath();
    isGrouping = false;

    deinitSurface();
}


void CAIRO_GAL::DrawGroup( int aGroupNumber )
{
    // This method implements a small Virtual Machine - all stored commands
    // are executed; nested calling is also possible

    storePath();

    for( GROUP::iterator it = groups[aGroupNumber].begin();
         it != groups[aGroupNumber].end(); ++it )
    {
        switch( it->command )
        {
        case CMD_SET_FILL:
            isFillEnabled = it->boolArgument;
            break;

        case CMD_SET_STROKE:
            isStrokeEnabled = it->boolArgument;
            break;

        case CMD_SET_FILLCOLOR:
            fillColor = COLOR4D( it->arguments[0], it->arguments[1], it->arguments[2],
                                 it->arguments[3] );
            break;

        case CMD_SET_STROKECOLOR:
            strokeColor = COLOR4D( it->arguments[0], it->arguments[1], it->arguments[2],
                                   it->arguments[3] );
            break;

        case CMD_SET_LINE_WIDTH:
            {
                // Make lines appear at least 1 pixel wide, no matter of zoom
                double x = 1.0, y = 1.0;
                cairo_device_to_user_distance( currentContext, &x, &y );
                double minWidth = std::min( fabs( x ), fabs( y ) );
                cairo_set_line_width( currentContext, std::max( it->arguments[0], minWidth ) );
            }
            break;


        case CMD_STROKE_PATH:
            cairo_set_source_rgb( currentContext, strokeColor.r, strokeColor.g, strokeColor.b );
            cairo_append_path( currentContext, it->cairoPath );
            cairo_stroke( currentContext );
            break;

        case CMD_FILL_PATH:
            cairo_set_source_rgb( currentContext, fillColor.r, fillColor.g, fillColor.b );
            cairo_append_path( currentContext, it->cairoPath );
            cairo_fill( currentContext );
            break;

        case CMD_TRANSFORM:
            cairo_matrix_t matrix;
            cairo_matrix_init( &matrix, it->arguments[0], it->arguments[1], it->arguments[2],
                               it->arguments[3], it->arguments[4], it->arguments[5] );
            cairo_transform( currentContext, &matrix );
            break;

        case CMD_ROTATE:
            cairo_rotate( currentContext, it->arguments[0] );
            break;

        case CMD_TRANSLATE:
            cairo_translate( currentContext, it->arguments[0], it->arguments[1] );
            break;

        case CMD_SCALE:
            cairo_scale( currentContext, it->arguments[0], it->arguments[1] );
            break;

        case CMD_SAVE:
            cairo_save( currentContext );
            break;

        case CMD_RESTORE:
            cairo_restore( currentContext );
            break;

        case CMD_CALL_GROUP:
            DrawGroup( it->intArgument );
            break;
        }
    }
}


void CAIRO_GAL::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    storePath();

    for( GROUP::iterator it = groups[aGroupNumber].begin();
         it != groups[aGroupNumber].end(); ++it )
    {
        if( it->command == CMD_SET_FILLCOLOR || it->command == CMD_SET_STROKECOLOR )
        {
            it->arguments[0] = aNewColor.r;
            it->arguments[1] = aNewColor.g;
            it->arguments[2] = aNewColor.b;
            it->arguments[3] = aNewColor.a;
        }
    }
}


void CAIRO_GAL::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    // Cairo does not have any possibilities to change the depth coordinate of stored items,
    // it depends only on the order of drawing
}


void CAIRO_GAL::DeleteGroup( int aGroupNumber )
{
    storePath();

    // Delete the Cairo paths
    std::deque<GROUP_ELEMENT>::iterator it, end;

    for( it = groups[aGroupNumber].begin(), end = groups[aGroupNumber].end(); it != end; ++it )
    {
        if( it->command == CMD_FILL_PATH || it->command == CMD_STROKE_PATH )
        {
            cairo_path_destroy( it->cairoPath );
        }
    }

    // Delete the group
    groups.erase( aGroupNumber );
}


void CAIRO_GAL::ClearCache()
{
    for( int i = groups.size() - 1; i >= 0; --i )
    {
        DeleteGroup( i );
    }
}


void CAIRO_GAL::SaveScreen()
{
    // Copy the current bitmap to the backup buffer
    int offset = 0;

    for( int j = 0; j < screenSize.y; j++ )
    {
        for( int i = 0; i < stride; i++ )
        {
            bitmapBufferBackup[offset + i] = bitmapBuffer[offset + i];
            offset += stride;
        }
    }
}


void CAIRO_GAL::RestoreScreen()
{
    int offset = 0;

    for( int j = 0; j < screenSize.y; j++ )
    {
        for( int i = 0; i < stride; i++ )
        {
            bitmapBuffer[offset + i] = bitmapBufferBackup[offset + i];
            offset += stride;
        }
    }
}


void CAIRO_GAL::SetTarget( RENDER_TARGET aTarget )
{
    // If the compositor is not set, that means that there is a recaching process going on
    // and we do not need the compositor now
    if( !validCompositor )
        return;

    // Cairo grouping prevents display of overlapping items on the same layer in the lighter color
    if( isInitialized )
    {
        storePath();

        cairo_pop_group_to_source( currentContext );
        cairo_paint_with_alpha( currentContext, LAYER_ALPHA );
    }

    switch( aTarget )
    {
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        compositor->SetBuffer( mainBuffer );
        break;

    case TARGET_OVERLAY:
        compositor->SetBuffer( overlayBuffer );
        break;
    }

    if( isInitialized )
        cairo_push_group( currentContext );

    currentTarget = aTarget;
}


RENDER_TARGET CAIRO_GAL::GetTarget() const
{
    return currentTarget;
}


void CAIRO_GAL::ClearTarget( RENDER_TARGET aTarget )
{
    // Save the current state
    unsigned int currentBuffer = compositor->GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        compositor->SetBuffer( mainBuffer );
        break;

    case TARGET_OVERLAY:
        compositor->SetBuffer( overlayBuffer );
        break;
    }

    compositor->ClearBuffer();

    // Restore the previous state
    compositor->SetBuffer( currentBuffer );
}


void CAIRO_GAL::SetCursorSize( unsigned int aCursorSize )
{
    GAL::SetCursorSize( aCursorSize );
    initCursor();
}


void CAIRO_GAL::DrawCursor( const VECTOR2D& aCursorPosition )
{
    // Now we should only store the position of the mouse cursor
    // The real drawing routines are in blitCursor()
    cursorPosition = VECTOR2D( aCursorPosition.x - cursorSize / 2,
                               aCursorPosition.y - cursorSize / 2 );
}


void CAIRO_GAL::drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    cairo_move_to( currentContext, aStartPoint.x, aStartPoint.y );
    cairo_line_to( currentContext, aEndPoint.x, aEndPoint.y );
    cairo_set_source_rgb( currentContext, gridColor.r, gridColor.g, gridColor.b );
    cairo_stroke( currentContext );
}


void CAIRO_GAL::storePath()
{
    if( isElementAdded )
    {
        isElementAdded = false;

        if( !isGrouping )
        {
            if( isFillEnabled )
            {
                cairo_set_source_rgb( currentContext, fillColor.r, fillColor.g, fillColor.b );
                cairo_fill_preserve( currentContext );
            }

            if( isStrokeEnabled )
            {
                cairo_set_source_rgb( currentContext, strokeColor.r, strokeColor.g,
                                      strokeColor.b );
                cairo_stroke_preserve( currentContext );
            }
        }
        else
        {
            // Copy the actual path, append it to the global path list
            // then check, if the path needs to be stroked/filled and
            // add this command to the group list;
            if( isStrokeEnabled )
            {
                GROUP_ELEMENT groupElement;
                groupElement.cairoPath = cairo_copy_path( currentContext );
                groupElement.command   = CMD_STROKE_PATH;
                currentGroup->push_back( groupElement );
            }

            if( isFillEnabled )
            {
                GROUP_ELEMENT groupElement;
                groupElement.cairoPath = cairo_copy_path( currentContext );
                groupElement.command   = CMD_FILL_PATH;
                currentGroup->push_back( groupElement );
            }
        }

        cairo_new_path( currentContext );
    }
}


void CAIRO_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    PostPaint();
}


void CAIRO_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( mouseListener )
        wxPostEvent( mouseListener, aEvent );
}


void CAIRO_GAL::initCursor()
{
    if( cursorPixels )
        delete cursorPixels;

    if( cursorPixelsSaved )
        delete cursorPixelsSaved;

    cursorPixels      = new wxBitmap( cursorSize, cursorSize );
    cursorPixelsSaved = new wxBitmap( cursorSize, cursorSize );

    wxMemoryDC cursorShape( *cursorPixels );

    cursorShape.SetBackground( *wxTRANSPARENT_BRUSH );
    wxColour color( cursorColor.r * cursorColor.a * 255, cursorColor.g * cursorColor.a * 255,
                    cursorColor.b * cursorColor.a * 255, 255 );
    wxPen pen = wxPen( color );
    cursorShape.SetPen( pen );
    cursorShape.Clear();

    cursorShape.DrawLine( 0, cursorSize / 2, cursorSize, cursorSize / 2 );
    cursorShape.DrawLine( cursorSize / 2, 0, cursorSize / 2, cursorSize );
}


void CAIRO_GAL::blitCursor( wxBufferedDC& clientDC )
{
    if( !isCursorEnabled )
        return;

    wxMemoryDC cursorSave( *cursorPixelsSaved );
    wxMemoryDC cursorShape( *cursorPixels );

    if( !isDeleteSavedPixels )
    {
        // Restore pixels that were overpainted by the previous cursor
        clientDC.Blit( savedCursorPosition.x, savedCursorPosition.y,
                       cursorSize, cursorSize, &cursorSave, 0, 0 );
    }
    else
    {
        isDeleteSavedPixels = false;
    }

    // Store pixels that are going to be overpainted
    VECTOR2D cursorScreen = ToScreen( cursorPosition ) - cursorSize / 2;
    cursorSave.Blit( 0, 0, cursorSize, cursorSize, &clientDC, cursorScreen.x, cursorScreen.y );

    // Draw the cursor
    clientDC.Blit( cursorScreen.x, cursorScreen.y, cursorSize, cursorSize,
                   &cursorShape, 0, 0, wxOR );

    savedCursorPosition.x = (wxCoord) cursorScreen.x;
    savedCursorPosition.y = (wxCoord) cursorScreen.y;
}


void CAIRO_GAL::allocateBitmaps()
{
    // Create buffer, use the system independent Cairo context backend
    stride     = cairo_format_stride_for_width( GAL_FORMAT, screenSize.x );
    bufferSize = stride * screenSize.y;

    bitmapBuffer        = new unsigned int[bufferSize];
    bitmapBufferBackup  = new unsigned int[bufferSize];
    wxOutput            = new unsigned char[bufferSize * 3];
}


void CAIRO_GAL::deleteBitmaps()
{
    delete[] bitmapBuffer;
    delete[] bitmapBufferBackup;
    delete[] wxOutput;
}


void CAIRO_GAL::initSurface()
{
    if( isInitialized )
        return;

    // Create the Cairo surface
    surface = cairo_image_surface_create_for_data( (unsigned char*) bitmapBuffer, GAL_FORMAT,
                                                   screenSize.x, screenSize.y, stride );
    context = cairo_create( surface );
#ifdef __WXDEBUG__
    cairo_status_t status = cairo_status( context );
    wxASSERT_MSG( status == CAIRO_STATUS_SUCCESS, wxT( "Cairo context creation error" ) );
#endif /* __WXDEBUG__ */
    currentContext = context;

    cairo_set_antialias( context, CAIRO_ANTIALIAS_SUBPIXEL );

    // Clear the screen
    ClearScreen( backgroundColor );

    // Compute the world <-> screen transformations
    ComputeWorldScreenMatrix();

    cairo_matrix_init( &cairoWorldScreenMatrix, worldScreenMatrix.m_data[0][0],
                       worldScreenMatrix.m_data[1][0], worldScreenMatrix.m_data[0][1],
                       worldScreenMatrix.m_data[1][1], worldScreenMatrix.m_data[0][2],
                       worldScreenMatrix.m_data[1][2] );

    cairo_set_matrix( context, &cairoWorldScreenMatrix );

    // Start drawing with a new path
    cairo_new_path( context );
    isElementAdded = true;

    cairo_set_line_join( context, CAIRO_LINE_JOIN_ROUND );
    cairo_set_line_cap( context, CAIRO_LINE_CAP_ROUND );

    lineWidth = 0;

    isDeleteSavedPixels = true;
    isInitialized = true;
}


void CAIRO_GAL::deinitSurface()
{
    if( !isInitialized )
        return;

    // Destroy Cairo objects
    cairo_destroy( context );
    cairo_surface_destroy( surface );

    isInitialized = false;
}


void CAIRO_GAL::setCompositor()
{
    // Recreate the compositor with the new Cairo context
    compositor.reset( new CAIRO_COMPOSITOR( &currentContext ) );
    compositor->Resize( screenSize.x, screenSize.y );

    // Prepare buffers
    mainBuffer = compositor->CreateBuffer();
    overlayBuffer = compositor->CreateBuffer();

    validCompositor = true;
}


unsigned int CAIRO_GAL::getNewGroupNumber()
{
    wxASSERT_MSG( groups.size() < std::numeric_limits<unsigned int>::max(),
                  wxT( "There are no free slots to store a group" ) );

    while( groups.find( groupCounter ) != groups.end() )
    {
        groupCounter++;
    }

    return groupCounter++;
}
