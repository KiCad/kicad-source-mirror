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

#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>
#include <wx/log.h>

#include <gal/cairo/cairo_gal.h>

using namespace KiGfx;

CAIRO_GAL::CAIRO_GAL( wxWindow* aParent, wxEvtHandler* aMouseListener,
                      wxEvtHandler* aPaintListener, const wxString& aName ) :
                      wxWindow( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxEXPAND, aName )
{
    // Default values
    fillColor   = COLOR4D( 0, 0, 0, 1 );
    strokeColor = COLOR4D( 1, 1, 1, 1 );
    screenSize  = VECTOR2D( 20, 20 );   // window will be soon resized

    parentWindow  = aParent;
    mouseListener = aMouseListener;
    paintListener = aPaintListener;

    isGrouping = false;
    zoomFactor = 1.0;

    SetSize( aParent->GetSize() );

    // Connecting the event handlers
    Connect( wxEVT_SIZE, wxSizeEventHandler( CAIRO_GAL::onSize ) );
    Connect( wxEVT_PAINT, wxPaintEventHandler( CAIRO_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );

    // Initialize line attributes map
    lineCapMap[LINE_CAP_BUTT]    = CAIRO_LINE_CAP_BUTT;
    lineCapMap[LINE_CAP_ROUND]   = CAIRO_LINE_CAP_ROUND;
    lineCapMap[LINE_CAP_SQUARED] = CAIRO_LINE_CAP_SQUARE;

    lineJoinMap[LINE_JOIN_BEVEL] = CAIRO_LINE_JOIN_BEVEL;
    lineJoinMap[LINE_JOIN_ROUND] = CAIRO_LINE_JOIN_ROUND;
    lineJoinMap[LINE_JOIN_MITER] = CAIRO_LINE_JOIN_MITER;

    isDeleteSavedPixels = false;

    isGrouping = false;

    // Initialize the cursor shape
    SetCursorColor( COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    initCursor( 21 );

    // Allocate memory
    allocateBitmaps();

    // Set grid defaults
    SetGridColor( COLOR4D( 0.5, 0.5, 0.5, 0.3 ) );
    SetCoarseGrid( 10 );
    SetGridLineWidth( 0.5 );

    Refresh();
}


CAIRO_GAL::~CAIRO_GAL()
{
    // TODO Deleting of list contents like groups and paths
    deleteBitmaps();
}


void CAIRO_GAL::onPaint( wxPaintEvent& aEvent )
{
    PostPaint();
}


void CAIRO_GAL::ResizeScreen( int aWidth, int aHeight )
{
    deleteBitmaps();

    screenSize  = VECTOR2D( aWidth, aHeight );

    // Recreate the bitmaps
    allocateBitmaps();

    SetSize( wxSize( aWidth, aHeight ) );

    PostPaint();
}


void CAIRO_GAL::onSize( wxSizeEvent& aEvent )
{
    ResizeScreen( aEvent.GetSize().x, aEvent.GetSize().y );
    PostPaint();
}


void CAIRO_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( mouseListener )
        wxPostEvent( mouseListener, aEvent );
}


void CAIRO_GAL::BeginDrawing() throw( int )
{
    // The size of the client area needs to be greater than zero
    clientRectangle = parentWindow->GetClientRect();

    if( clientRectangle.width == 0 || clientRectangle.height == 0 )
        throw EXCEPTION_ZERO_CLIENT_RECTANGLE;

    // clientDC = new wxClientDC( this );

    // Create the CAIRO surface
    cairoSurface = cairo_image_surface_create_for_data( (unsigned char*) bitmapBuffer,
                                                        CAIRO_FORMAT_RGB24, clientRectangle.width,
                                                        clientRectangle.height, stride );
    cairoImage = cairo_create( cairoSurface );

    // -----------------------------------------------------------------

    cairo_set_antialias( cairoImage, CAIRO_ANTIALIAS_SUBPIXEL );

    // Clear the screen
    ClearScreen();

    // Compute the world <-> screen transformations
    ComputeWorldScreenMatrix();

    cairo_matrix_init( &cairoWorldScreenMatrix, worldScreenMatrix.m_data[0][0],
                       worldScreenMatrix.m_data[1][0], worldScreenMatrix.m_data[0][1],
                       worldScreenMatrix.m_data[1][1], worldScreenMatrix.m_data[0][2],
                       worldScreenMatrix.m_data[1][2] );

    cairo_set_matrix( cairoImage, &cairoWorldScreenMatrix );

    isSetAttributes = false;

    // Start drawing with a new path
    cairo_new_path( cairoImage );
    isElementAdded = true;

    cairo_set_line_join( cairoImage, lineJoinMap[lineJoin] );
    cairo_set_line_cap( cairoImage, lineCapMap[lineCap] );

    lineWidth = 0;

    isDeleteSavedPixels = true;
}


void CAIRO_GAL::EndDrawing()
{
    // Force remaining objects to be drawn
    Flush();

    // FIXME Accelerate support for wxWidgets 2.8.10

#if wxCHECK_VERSION( 2, 9, 0 )
    // Copy the cairo image contents to the wxBitmap
    wxNativePixelData pixelData( *wxBitmap_ );

    if( !pixelData )
    {
        wxLogError( wxString::FromUTF8( "Can't access pixel data!" ) );
        return;
    }

    wxNativePixelData::Iterator pixelIterator( pixelData );

    int offset = 0;

    // Copy the cairo image to the wxDC bitmap
    for( int j = 0; j < screenSize.y; j++ )
    {
        offset = j * (int) screenSize.x;

        for( int column = 0; column < clientRectangle.width; column++ )
        {
            unsigned int value    = bitmapBuffer[offset + column];
            pixelIterator.Red()   = value >> 16;
            pixelIterator.Green() = value >> 8;
            pixelIterator.Blue()  = value;
            pixelIterator++;
        }

        pixelIterator.MoveTo( pixelData, 0, j );
    }

    // Blit the contents to the screen
    wxClientDC   client_dc( this );
    wxBufferedDC dc( &client_dc );
    dc.DrawBitmap( *wxBitmap_, 0, 0 );

#elif wxCHECK_VERSION( 2, 8, 0 )

    // This code was taken from the wxCairo example - it's not the most efficient one
    // Here is a good place for optimizations

    // Now translate the raw image data from the format stored
    // by cairo into a format understood by wxImage.
    unsigned char* wxOutputPtr = wxOutput;
    for( size_t count = 0; count < bufferSize; count++ )
    {
        unsigned int value = bitmapBuffer[count];
        // Red pixel
        *wxOutputPtr++ = (value >> 16) & 0xff;
        // Green pixel
        *wxOutputPtr++ = (value >> 8) & 0xff;
        // Blue pixel
        *wxOutputPtr++ = (value >> 0) & 0xff;
    }

    wxImage      img( (int) screenSize.x, (int) screenSize.y, (unsigned char*) wxOutput, true);
    wxBitmap     bmp( img );
    wxClientDC   client_dc( this );
    wxBufferedDC dc;
    // client_dc.DrawBitmap(bmp, 0, 0, false);
    dc.Init( &client_dc, bmp );

#else
#error "need wxWidgets-2.8 as a minimum"
#endif

    // Destroy Cairo objects
    cairo_destroy( cairoImage );
    cairo_surface_destroy( cairoSurface );
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


void CAIRO_GAL::DrawLine( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    cairo_move_to( cairoImage, aStartPoint.x, aStartPoint.y );
    cairo_line_to( cairoImage, aEndPoint.x, aEndPoint.y );
    isElementAdded = true;
}


void CAIRO_GAL::DrawCircle( VECTOR2D aCenterPoint, double aRadius )
{
    // A circle is drawn using an arc
    cairo_new_sub_path( cairoImage );
    cairo_arc( cairoImage, aCenterPoint.x, aCenterPoint.y, aRadius, 0.0, 2 * M_PI );
    isElementAdded = true;
}


void CAIRO_GAL::DrawArc( VECTOR2D aCenterPoint, double aRadius, double aStartAngle,
                         double aEndAngle )
{
    cairo_new_sub_path( cairoImage );
    cairo_arc( cairoImage, aCenterPoint.x, aCenterPoint.y, aRadius, aStartAngle, aEndAngle );
    isElementAdded = true;
}


void CAIRO_GAL::DrawPolyline( std::deque<VECTOR2D>& aPointList )
{
    bool isFirstPoint = true;

    // Iterate over the point list and draw the segments
    for( std::deque<VECTOR2D>::iterator it = aPointList.begin(); it != aPointList.end(); ++it )
    {
        if( isFirstPoint )
        {
            cairo_move_to( cairoImage, it->x, it->y );
            isFirstPoint = false;
        }
        else
        {
            cairo_line_to( cairoImage, it->x, it->y );
        }
    }

    isElementAdded = true;
}


void CAIRO_GAL::DrawPolygon( const std::deque<VECTOR2D>& aPointList )
{
    bool isFirstPoint = true;

    // Iterate over the point list and draw the polygon
    for( std::deque<VECTOR2D>::const_iterator it = aPointList.begin(); it != aPointList.end(); ++it )
    {
        if( isFirstPoint )
        {
            cairo_move_to( cairoImage, it->x, it->y );
            isFirstPoint = false;
        }
        else
        {
            cairo_line_to( cairoImage, it->x, it->y );
        }
    }

    isElementAdded = true;
}


void CAIRO_GAL::DrawRectangle( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    // Calculate the diagonal points
    VECTOR2D diagonalPointA( aEndPoint.x, aStartPoint.y );
    VECTOR2D diagonalPointB( aStartPoint.x, aEndPoint.y );

    // The path is composed from 4 segments
    cairo_move_to( cairoImage, aStartPoint.x, aStartPoint.y );
    cairo_line_to( cairoImage, diagonalPointA.x, diagonalPointA.y );
    cairo_line_to( cairoImage, aEndPoint.x, aEndPoint.y );
    cairo_line_to( cairoImage, diagonalPointB.x, diagonalPointB.y );
    cairo_close_path( cairoImage );

    isElementAdded = true;
}


void CAIRO_GAL::DrawCurve( VECTOR2D aStartPoint, VECTOR2D aControlPointA,
                           VECTOR2D aControlPointB, VECTOR2D aEndPoint )
{
    cairo_move_to( cairoImage, aStartPoint.x, aStartPoint.y );
    cairo_curve_to( cairoImage, aControlPointA.x, aControlPointA.y, aControlPointB.x,
                    aControlPointB.y, aEndPoint.x, aEndPoint.y );
    cairo_line_to( cairoImage, aEndPoint.x, aEndPoint.y );
    isElementAdded = true;
}


void CAIRO_GAL::SetBackgroundColor( COLOR4D aColor )
{
    backgroundColor = aColor;
}


void CAIRO_GAL::SetIsFill( bool aIsFillEnabled )
{
    storePath();
    isFillEnabled = aIsFillEnabled;

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SET_FILL;
        groupElement.boolArgument = aIsFillEnabled;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::SetIsStroke( bool aIsStrokeEnabled )
{
    storePath();
    isStrokeEnabled = aIsStrokeEnabled;

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SET_STROKE;
        groupElement.boolArgument = aIsStrokeEnabled;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::SetStrokeColor( COLOR4D aColor )
{
    storePath();

    strokeColor = aColor;

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SET_STROKECOLOR;
        groupElement.arguments[0] = strokeColor.r;
        groupElement.arguments[1] = strokeColor.g;
        groupElement.arguments[2] = strokeColor.b;
        groupElement.arguments[3] = strokeColor.a;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::SetFillColor( COLOR4D aColor )
{
    storePath();
    fillColor = aColor;

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SET_FILLCOLOR;
        groupElement.arguments[0] = fillColor.r;
        groupElement.arguments[1] = fillColor.g;
        groupElement.arguments[2] = fillColor.b;
        groupElement.arguments[3] = fillColor.a;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::SetLineWidth( double aLineWidth )
{
    storePath();

    lineWidth = aLineWidth;
    cairo_set_line_width( cairoImage, aLineWidth );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SET_LINE_WIDTH;
        groupElement.arguments[0] = aLineWidth;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::SetLineCap( LineCap aLineCap )
{
    storePath();

    lineCap = aLineCap;

    cairo_set_line_cap( cairoImage, lineCapMap[aLineCap] );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command     = CMD_SET_LINE_CAP;
        groupElement.intArgument = (int) aLineCap;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::SetLineJoin( LineJoin aLineJoin )
{
    storePath();

    lineJoin = aLineJoin;

    cairo_set_line_join( cairoImage, lineJoinMap[aLineJoin] );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command     = CMD_SET_LINE_JOIN;
        groupElement.intArgument = (int) aLineJoin;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::ClearScreen()
{
    // Clear screen
    cairo_set_source_rgba( cairoImage,
                           backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0 );
    cairo_rectangle( cairoImage, 0.0, 0.0, screenSize.x, screenSize.y );
    cairo_fill( cairoImage );
}


void CAIRO_GAL::Transform( MATRIX3x3D aTransformation )
{
    cairo_matrix_t cairoTransformation;

    cairo_matrix_init( &cairoTransformation,
                       aTransformation.m_data[0][0],
                       aTransformation.m_data[1][0],
                       aTransformation.m_data[0][1],
                       aTransformation.m_data[1][1],
                       aTransformation.m_data[0][2],
                       aTransformation.m_data[1][2] );

    cairo_transform( cairoImage, &cairoTransformation );
}


void CAIRO_GAL::Rotate( double aAngle )
{
    storePath();

    cairo_rotate( cairoImage, aAngle );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_ROTATE;
        groupElement.arguments[0] = aAngle;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::Translate( VECTOR2D aTranslation )
{
    storePath();

    cairo_translate( cairoImage, aTranslation.x, aTranslation.y );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_TRANSLATE;
        groupElement.arguments[0] = aTranslation.x;
        groupElement.arguments[1] = aTranslation.y;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::Scale( VECTOR2D aScale )
{
    storePath();

    cairo_scale( cairoImage, aScale.x, aScale.y );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SCALE;
        groupElement.arguments[0] = aScale.x;
        groupElement.arguments[1] = aScale.y;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::Save()
{
    storePath();

    cairo_save( cairoImage );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_SAVE;
        groups.back().push_back( groupElement );
    }
}


void CAIRO_GAL::Restore()
{
    storePath();

    cairo_restore( cairoImage );

    if( isGrouping )
    {
        GroupElement groupElement;
        groupElement.command = CMD_RESTORE;
        groups.back().push_back( groupElement );
    }
}


int CAIRO_GAL::BeginGroup()
{
    // If the grouping is started: the actual path is stored in the group, when
    // a attribute was changed or when grouping stops with the end group method.
    storePath();
    Group group;
    groups.push_back( group );
    isGrouping = true;
    return groups.size() - 1;
}


void CAIRO_GAL::EndGroup()
{
    storePath();
    isGrouping = false;
}


void CAIRO_GAL::DeleteGroup( int aGroupNumber )
{
    storePath();

    // Delete the Cairo paths
    for( std::deque<GroupElement>::iterator it = groups[aGroupNumber].begin();
         it != groups[aGroupNumber].end(); ++it )
    {
        if( it->command == CMD_FILL_PATH || it->command == CMD_STROKE_PATH )
        {
            cairo_path_destroy( it->cairoPath );
        }
    }

    // Delete the group
    groups.erase( groups.begin() + aGroupNumber );
}


void CAIRO_GAL::DrawGroup( int aGroupNumber )
{
    // This method implements a small Virtual Machine - all stored commands
    // are executed; nested calling is also possible

    storePath();

    for( Group::iterator it = groups[aGroupNumber].begin();
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
            cairo_set_line_width( cairoImage, it->arguments[0] );
            break;

        case CMD_SET_LINE_JOIN:
            cairo_set_line_join( cairoImage, lineJoinMap[(LineJoin) ( it->intArgument )] );
            break;

        case CMD_SET_LINE_CAP:
            cairo_set_line_cap( cairoImage, lineCapMap[(LineCap) ( it->intArgument )] );
            break;

        case CMD_STROKE_PATH:
            cairo_set_source_rgba( cairoImage, strokeColor.r, strokeColor.g, strokeColor.b,
                                   strokeColor.a );
            cairo_append_path( cairoImage, it->cairoPath );
            cairo_stroke( cairoImage );
            break;

        case CMD_FILL_PATH:
            cairo_set_source_rgba( cairoImage, fillColor.r, fillColor.g, fillColor.b,
                                   fillColor.a );
            cairo_append_path( cairoImage, it->cairoPath );
            cairo_fill( cairoImage );
            break;

        case CMD_TRANSFORM:
            cairo_matrix_t matrix;
            cairo_matrix_init( &matrix, it->arguments[0], it->arguments[1], it->arguments[2],
                               it->arguments[3], it->arguments[4], it->arguments[5] );
            cairo_transform( cairoImage, &matrix );
            break;

        case CMD_ROTATE:
            cairo_rotate( cairoImage, it->arguments[0] );
            break;

        case CMD_TRANSLATE:
            cairo_translate( cairoImage, it->arguments[0], it->arguments[1] );
            break;

        case CMD_SCALE:
            cairo_scale( cairoImage, it->arguments[0], it->arguments[1] );
            break;

        case CMD_SAVE:
            cairo_save( cairoImage );
            break;

        case CMD_RESTORE:
            cairo_restore( cairoImage );
            break;

        case CMD_CALL_GROUP:
            DrawGroup( it->intArgument );
            break;
        }
    }
}


void CAIRO_GAL::Flush()
{
    storePath();
}


void CAIRO_GAL::ComputeWorldScreenMatrix()
{
    ComputeWorldScale();

    worldScreenMatrix.SetIdentity();

    MATRIX3x3D translation;
    translation.SetIdentity();
    translation.SetTranslation( 0.5 * screenSize );

    MATRIX3x3D scale;
    scale.SetIdentity();
    scale.SetScale( VECTOR2D( worldScale, worldScale ) );

    MATRIX3x3D lookat;
    lookat.SetIdentity();
    lookat.SetTranslation( -lookAtPoint );

    worldScreenMatrix = translation * scale * lookat * worldScreenMatrix;
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
                cairo_set_source_rgba( cairoImage, fillColor.r, fillColor.g, fillColor.b,
                                       fillColor.a );
                cairo_fill_preserve( cairoImage );
            }

            if( isStrokeEnabled )
            {
                cairo_set_source_rgba( cairoImage, strokeColor.r, strokeColor.g, strokeColor.b,
                                       strokeColor.a );
                cairo_stroke_preserve( cairoImage );
            }
        }
        else
        {
            // Copy the actual path, append it to the global path list
            // then check, if the path needs to be stroked/filled and
            // add this command to the group list;

            cairo_path_t* path = cairo_copy_path( cairoImage );
            pathList.push_back( path );

            if( isStrokeEnabled )
            {
                GroupElement groupElement;
                groupElement.cairoPath = path;
                groupElement.command   = CMD_STROKE_PATH;
                groups.back().push_back( groupElement );
            }

            if( isFillEnabled )
            {
                GroupElement groupElement;
                groupElement.cairoPath = path;
                groupElement.command   = CMD_FILL_PATH;
                groups.back().push_back( groupElement );
            }
        }

        cairo_new_path( cairoImage );
    }
}


// ---------------
// Cursor handling
// ---------------


void CAIRO_GAL::initCursor( int aCursorSize )
{
    cursorPixels      = new wxBitmap( aCursorSize, aCursorSize );
    cursorPixelsSaved = new wxBitmap( aCursorSize, aCursorSize );
    cursorSize        = aCursorSize;

    wxMemoryDC cursorShape( *cursorPixels );

    cursorShape.SetBackground( *wxTRANSPARENT_BRUSH );
    wxColour color( cursorColor.r * cursorColor.a * 255, cursorColor.g * cursorColor.a * 255,
                    cursorColor.b * cursorColor.a * 255, 255 );
    wxPen    pen = wxPen( color );
    cursorShape.SetPen( pen );
    cursorShape.Clear();

    cursorShape.DrawLine( 0, aCursorSize / 2, aCursorSize, aCursorSize / 2 );
    cursorShape.DrawLine( aCursorSize / 2, 0, aCursorSize / 2, aCursorSize );
}


VECTOR2D CAIRO_GAL::ComputeCursorToWorld( VECTOR2D aCursorPosition )
{

    MATRIX3x3D inverseMatrix = worldScreenMatrix.Inverse();
    VECTOR2D   cursorPositionWorld = inverseMatrix * aCursorPosition;

    return cursorPositionWorld;
}


void CAIRO_GAL::DrawCursor( VECTOR2D aCursorPosition )
{
    if( !IsShownOnScreen() )
        return;

    wxClientDC clientDC( this );
    wxMemoryDC cursorSave( *cursorPixelsSaved );
    wxMemoryDC cursorShape( *cursorPixels );

    // Snap to grid
    VECTOR2D cursorPositionWorld = ComputeCursorToWorld( aCursorPosition );

    cursorPositionWorld.x = round( cursorPositionWorld.x / gridSize.x ) * gridSize.x;
    cursorPositionWorld.y = round( cursorPositionWorld.y / gridSize.y ) * gridSize.y;
    aCursorPosition       = worldScreenMatrix * cursorPositionWorld;
    aCursorPosition       = aCursorPosition - VECTOR2D( cursorSize / 2, cursorSize / 2 );

    if( !isDeleteSavedPixels )
    {
        clientDC.Blit( savedCursorPosition.x, savedCursorPosition.y, cursorSize, cursorSize,
                       &cursorSave, 0, 0 );
    }
    else
    {
        isDeleteSavedPixels = false;
    }

    cursorSave.Blit( 0, 0, cursorSize, cursorSize, &clientDC, aCursorPosition.x,
                     aCursorPosition.y );

    clientDC.Blit( aCursorPosition.x, aCursorPosition.y, cursorSize, cursorSize, &cursorShape, 0,
                   0, wxOR );

    savedCursorPosition.x = (wxCoord) aCursorPosition.x;
    savedCursorPosition.y = (wxCoord) aCursorPosition.y;
}


void CAIRO_GAL::DrawGridLine( VECTOR2D aStartPoint, VECTOR2D aEndPoint )
{
    cairo_move_to( cairoImage, aStartPoint.x, aStartPoint.y );
    cairo_line_to( cairoImage, aEndPoint.x, aEndPoint.y );
    cairo_set_source_rgba( cairoImage, gridColor.r, gridColor.g, gridColor.b, gridColor.a );
    cairo_stroke( cairoImage );
}


void CAIRO_GAL::allocateBitmaps()
{
    // Create buffer, use the system independent CAIRO image back end
    stride     = cairo_format_stride_for_width( CAIRO_FORMAT_RGB24, screenSize.x );
    bufferSize = stride * screenSize.y;

    bitmapBuffer       	= new unsigned int[bufferSize];
    bitmapBufferBackup 	= new unsigned int[bufferSize];
    wxOutput            = new unsigned char[bufferSize * 4];
    wxBitmap_           = new wxBitmap( screenSize.x, screenSize.y, SCREEN_DEPTH );
}


void CAIRO_GAL::deleteBitmaps()
{
    delete[] bitmapBuffer;
    delete[] bitmapBufferBackup;
    delete[] wxOutput;
    delete   wxBitmap_;
}


bool CAIRO_GAL::Show( bool aShow )
{
    bool s = wxWindow::Show( aShow );

    if( aShow )
        wxWindow::Raise();

    return s;
}
