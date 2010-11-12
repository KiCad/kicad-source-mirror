/////////////////////////////////////////////////////////////////////////////

// Name:        3d_canvas.cpp
/////////////////////////////////////////////////////////////////////////////
#include "fctsys.h"
#include "trigo.h"

#include "wx/image.h"

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#include "wx/dataobj.h"
#include "wx/clipbrd.h"
#include <wx/wupdlock.h>

#include "gestfich.h"

#include "3d_viewer.h"
#include "trackball.h"

/* Tool and button Bitmaps */
#include "bitmaps.h"


// -----------------
// helper function (from wxWidgets, opengl/cube.cpp sample
// -----------------
void CheckGLError()
{
    GLenum errLast = GL_NO_ERROR;

    for ( ;; )
    {
        GLenum err = glGetError();
        if ( err == GL_NO_ERROR )
            return;

        // normally the error is reset by the call to glGetError() but if
        // glGetError() itself returns an error, we risk looping forever here
        // so check that we get a different error than the last time
        if ( err == errLast )
        {
            wxLogError(wxT("OpenGL error state couldn't be reset."));
            return;
        }

        errLast = err;

        wxLogError(wxT("OpenGL error %d"), err);
    }
}

/*
 * Pcb3D_GLCanvas implementation
 */

BEGIN_EVENT_TABLE( Pcb3D_GLCanvas, wxGLCanvas )
EVT_PAINT( Pcb3D_GLCanvas::OnPaint )

// key event:
EVT_CHAR( Pcb3D_GLCanvas::OnChar )

// mouse events
EVT_RIGHT_DOWN( Pcb3D_GLCanvas::OnRightClick )
EVT_MOUSEWHEEL( Pcb3D_GLCanvas::OnMouseWheel )
EVT_MOTION( Pcb3D_GLCanvas::OnMouseMove )

// other events
EVT_ERASE_BACKGROUND( Pcb3D_GLCanvas::OnEraseBackground )
EVT_MENU_RANGE( ID_POPUP_3D_VIEW_START, ID_POPUP_3D_VIEW_END,
                Pcb3D_GLCanvas::OnPopUpMenu )
END_EVENT_TABLE()

Pcb3D_GLCanvas::Pcb3D_GLCanvas( WinEDA3D_DrawFrame* parent ) :
#if wxCHECK_VERSION( 2, 9, 0 )
    wxGLCanvas( parent, -1, NULL, wxDefaultPosition, wxDefaultSize,
                wxFULL_REPAINT_ON_RESIZE )
#else
    wxGLCanvas( parent, -1, wxDefaultPosition, wxDefaultSize,
                wxFULL_REPAINT_ON_RESIZE )
#endif
{
    m_init   = FALSE;
    m_gllist = 0;
    m_Parent = parent;
    m_ortho = false;

#if wxCHECK_VERSION( 2, 9, 0 )

    // Explicitly create a new rendering context instance for this canvas.
    m_glRC = new wxGLContext( this );
#endif

    DisplayStatus();
}


Pcb3D_GLCanvas::~Pcb3D_GLCanvas()
{
    ClearLists();
    m_init = FALSE;
#if wxCHECK_VERSION( 2, 9, 0 )
    delete m_glRC;
#endif
}


void Pcb3D_GLCanvas::ClearLists()
{
    if( m_gllist > 0 )
        glDeleteLists( m_gllist, 1 );
    m_gllist = 0;
}


void Pcb3D_GLCanvas::OnChar( wxKeyEvent& event )
{
    SetView3D( event.GetKeyCode() );
    event.Skip();
}


void Pcb3D_GLCanvas::SetView3D( int keycode )
{
    int    ii;
    double delta_move = 0.7 * g_Parm_3D_Visu.m_Zoom;

    switch( keycode )
    {
    case WXK_LEFT:
        g_Draw3d_dx -= delta_move;
        break;

    case WXK_RIGHT:
        g_Draw3d_dx += delta_move;
        break;

    case WXK_UP:
        g_Draw3d_dy += delta_move;
        break;

    case WXK_DOWN:
        g_Draw3d_dy -= delta_move;
        break;

    case WXK_HOME:
        g_Parm_3D_Visu.m_Zoom = 1.0;
        g_Draw3d_dx = g_Draw3d_dy = 0;
        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        break;

    case WXK_END:
        break;

    case WXK_F1:
        g_Parm_3D_Visu.m_Zoom /= 1.4;
        if( g_Parm_3D_Visu.m_Zoom <= 0.01 )
            g_Parm_3D_Visu.m_Zoom = 0.01;
        break;

    case WXK_F2:
        g_Parm_3D_Visu.m_Zoom *= 1.4;
        break;

    case '+':
        break;

    case '-':
        break;

    case 'r':
    case 'R':
        g_Draw3d_dx = g_Draw3d_dy = 0;
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        break;

    case 'x':
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        g_Parm_3D_Visu.m_ROTZ = -90;
        g_Parm_3D_Visu.m_ROTX = -90;
        break;

    case 'X':
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        g_Parm_3D_Visu.m_ROTZ = 90;
        g_Parm_3D_Visu.m_ROTX = -90;
        break;

    case 'y':
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        g_Parm_3D_Visu.m_ROTX = -90;
        break;

    case 'Y':
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        g_Parm_3D_Visu.m_ROTX = -90;
        g_Parm_3D_Visu.m_ROTZ = -180;
        break;

    case 'z':
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        break;

    case 'Z':
        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        g_Parm_3D_Visu.m_ROTX = -180;
        break;

    default:
        return;
    }

    DisplayStatus();
    Refresh( FALSE );
}


void Pcb3D_GLCanvas::OnMouseWheel( wxMouseEvent& event )
{
    wxSize size( GetClientSize() );

    if( event.ShiftDown() )
    {
        if( event.GetWheelRotation() < 0 )
        {
            /* up */
            SetView3D( WXK_UP );
        }
        else
        {
            /* down */
            SetView3D( WXK_DOWN );
        }
    }
    else if( event.ControlDown() )
    {
        if( event.GetWheelRotation() > 0 )
        {
            /* right */
            SetView3D( WXK_RIGHT );
        }
        else
        {
            /* left */
            SetView3D( WXK_LEFT );
        }
    }
    else
    {
        if( event.GetWheelRotation() > 0 )
        {
            g_Parm_3D_Visu.m_Zoom /= 1.4;
            if( g_Parm_3D_Visu.m_Zoom <= 0.01 )
                g_Parm_3D_Visu.m_Zoom = 0.01;
        }
        else
            g_Parm_3D_Visu.m_Zoom *= 1.4;
        DisplayStatus();
        Refresh( FALSE );
    }

    g_Parm_3D_Visu.m_Beginx = event.GetX();
    g_Parm_3D_Visu.m_Beginy = event.GetY();
}


void Pcb3D_GLCanvas::OnMouseMove( wxMouseEvent& event )
{
    wxSize size( GetClientSize() );
    double spin_quat[4];

    if( event.Dragging() )
    {
        if( event.LeftIsDown() )
        {
            /* drag in progress, simulate trackball */
            trackball( spin_quat,
                       (2.0 * g_Parm_3D_Visu.m_Beginx - size.x) / size.x,
                       (size.y - 2.0 * g_Parm_3D_Visu.m_Beginy) / size.y,
                       (     2.0 * event.GetX() - size.x) / size.x,
                       ( size.y - 2.0 * event.GetY() ) / size.y );

            add_quats( spin_quat, g_Parm_3D_Visu.m_Quat, g_Parm_3D_Visu.m_Quat );
        }
        else if( event.MiddleIsDown() )
        {
            /* middle button drag -> pan */

            /* Current zoom and an additional factor are taken into account
             * for the amount of panning. */
            const double PAN_FACTOR = 8.0 * g_Parm_3D_Visu.m_Zoom;
            g_Draw3d_dx -= PAN_FACTOR *
                           ( g_Parm_3D_Visu.m_Beginx - event.GetX() ) / size.x;
            g_Draw3d_dy -= PAN_FACTOR *
                           (event.GetY() - g_Parm_3D_Visu.m_Beginy) / size.y;
        }

        /* orientation has changed, redraw mesh */
        DisplayStatus();
        Refresh( FALSE );
    }

    g_Parm_3D_Visu.m_Beginx = event.GetX();
    g_Parm_3D_Visu.m_Beginy = event.GetY();
}


/* Construct and display a popup menu when the right button is clicked.
 */
void Pcb3D_GLCanvas::OnRightClick( wxMouseEvent& event )
{
    wxPoint     pos;
    wxMenu      PopUpMenu;

    pos.x = event.GetX(); pos.y = event.GetY();
    wxMenuItem* item = new wxMenuItem( &PopUpMenu, ID_POPUP_ZOOMIN,
                                      _( "Zoom +" ) );
    item->SetBitmap( zoom_in_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_ZOOMOUT,
                          _( "Zoom -" ) );
    item->SetBitmap( zoom_out_xpm );
    PopUpMenu.Append( item );

    PopUpMenu.AppendSeparator();
    item = new wxMenuItem( &PopUpMenu, ID_POPUP_VIEW_ZPOS,
                          _( "Top View" ) );
    item->SetBitmap( axis3d_top_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_VIEW_ZNEG,
                          _( "Bottom View" ) );
    item->SetBitmap( axis3d_bottom_xpm );
    PopUpMenu.Append( item );

    PopUpMenu.AppendSeparator();
    item = new wxMenuItem( &PopUpMenu, ID_POPUP_VIEW_XPOS,
                          _( "Right View" ) );
    item->SetBitmap( axis3d_right_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_VIEW_XNEG,
                          _( "Left View" ) );
    item->SetBitmap( axis3d_left_xpm );
    PopUpMenu.Append( item );


    PopUpMenu.AppendSeparator();
    item = new wxMenuItem( &PopUpMenu, ID_POPUP_VIEW_YPOS,
                          _( "Front View" ) );
    item->SetBitmap( axis3d_front_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_VIEW_YNEG,
                          _( "Back View" ) );
    item->SetBitmap( axis3d_back_xpm );
    PopUpMenu.Append( item );

    PopUpMenu.AppendSeparator();
    item = new wxMenuItem( &PopUpMenu, ID_POPUP_MOVE3D_LEFT,
                          _( "Move left <-" ) );
    item->SetBitmap( left_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_MOVE3D_RIGHT,
                          _( "Move right ->" ) );
    item->SetBitmap( right_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_MOVE3D_UP,
                          _( "Move Up ^" ) );
    item->SetBitmap( up_xpm );
    PopUpMenu.Append( item );

    item = new wxMenuItem( &PopUpMenu, ID_POPUP_MOVE3D_DOWN,
                          _( "Move Down" ) );
    item->SetBitmap( down_xpm );
    PopUpMenu.Append( item );

    PopupMenu( &PopUpMenu, pos );
}


void Pcb3D_GLCanvas::OnPopUpMenu( wxCommandEvent& event )
{
    int key = 0;

    switch( event.GetId() )
    {
    case ID_POPUP_ZOOMIN:
        key = WXK_F1;
        break;

    case ID_POPUP_ZOOMOUT:
        key = WXK_F2;
        break;

    case ID_POPUP_VIEW_XPOS:
        key = 'x';
        break;

    case ID_POPUP_VIEW_XNEG:
        key = 'X';
        break;

    case ID_POPUP_VIEW_YPOS:
        key = 'y';
        break;

    case ID_POPUP_VIEW_YNEG:
        key = 'Y';
        break;

    case ID_POPUP_VIEW_ZPOS:
        key = 'z';
        break;

    case ID_POPUP_VIEW_ZNEG:
        key = 'Z';
        break;

    case ID_POPUP_MOVE3D_LEFT:
        key = WXK_LEFT;
        break;

    case ID_POPUP_MOVE3D_RIGHT:
        key = WXK_RIGHT;
        break;

    case ID_POPUP_MOVE3D_UP:
        key = WXK_UP;
        break;

    case ID_POPUP_MOVE3D_DOWN:
        key = WXK_DOWN;
        break;

    default:
        return;
    }

    SetView3D( key );
}


void Pcb3D_GLCanvas::DisplayStatus()
{
    wxString msg;

    msg.Printf( wxT( "dx %3.2f" ), g_Draw3d_dx );
    m_Parent->SetStatusText( msg, 1 );

    msg.Printf( wxT( "dy %3.2f" ), g_Draw3d_dy );
    m_Parent->SetStatusText( msg, 2 );

    msg.Printf( wxT( "View: %3.1f" ), 45 * g_Parm_3D_Visu.m_Zoom );
    m_Parent->SetStatusText( msg, 3 );
}


void Pcb3D_GLCanvas::OnPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );

    Redraw();
    event.Skip();
}


void Pcb3D_GLCanvas::OnEraseBackground( wxEraseEvent& event )
{
    // Do nothing, to avoid flashing.
}


/* Initialize broad parameters for OpenGL */
void Pcb3D_GLCanvas::InitGL()
{
    wxSize size = GetClientSize();

    if( !m_init )
    {
        m_init = TRUE;
        g_Parm_3D_Visu.m_Zoom = 1.0;
        ZBottom = 1.0; ZTop = 10.0;

        glDisable( GL_CULL_FACE );      // show back faces

        glEnable( GL_DEPTH_TEST );      // Enable z-buferring

        glEnable( GL_LINE_SMOOTH );
        glEnable( GL_COLOR_MATERIAL );
        glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

        /* speedups */
        glEnable( GL_DITHER );
        glShadeModel( GL_SMOOTH );
        glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );
        glHint( GL_POLYGON_SMOOTH_HINT, GL_FASTEST );

        /* blend */
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }

    // set viewing projection

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

#define MAX_VIEW_ANGLE 160.0 / 45.0
    if( g_Parm_3D_Visu.m_Zoom > MAX_VIEW_ANGLE )
        g_Parm_3D_Visu.m_Zoom = MAX_VIEW_ANGLE;

     if( ModeIsOrtho() )
     {
         // OrthoReductionFactor is chosen so as to provide roughly the same size as Perspective View
         const double orthoReductionFactor = 400/g_Parm_3D_Visu.m_Zoom;
         // Initialize Projection Matrix for Ortographic View
         glOrtho(-size.x/orthoReductionFactor, size.x/orthoReductionFactor, -size.y/orthoReductionFactor, size.y/orthoReductionFactor, 1, 10);
     }
     else
     {
         // Ratio width / height of the window display
         double ratio_HV = (double) size.x / size.y;

         // Initialize Projection Matrix for Perspective View
         gluPerspective( 45.0 * g_Parm_3D_Visu.m_Zoom, ratio_HV, 1, 10 );
     }


    // position viewer
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0F, 0.0F, -( ZBottom + ZTop) / 2 );

    // clear color and depth buffers
    glClearColor( g_Parm_3D_Visu.m_BgColor.m_Red,
                  g_Parm_3D_Visu.m_BgColor.m_Green,
                  g_Parm_3D_Visu.m_BgColor.m_Blue, 1 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // Setup light souces:
    SetLights();

    CheckGLError();
}


/* Initialize OpenGL light sources. */
void Pcb3D_GLCanvas::SetLights()
{
    double  light;
    GLfloat light_color[4];

    /* set viewing projection */
    light_color[3] = 1.0;
    GLfloat Z_axis_pos[4]    = { 0.0, 0.0, 3.0, 0.0 };
    GLfloat lowZ_axis_pos[4] = { 0.0, 0.0, -3.0, 0.5 };

    /* activate light */
    light = 1.0;
    light_color[0] = light_color[1] = light_color[2] = light;
    glLightfv( GL_LIGHT0, GL_POSITION, Z_axis_pos );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, light_color );
    light = 0.3;
    light_color[0] = light_color[1] = light_color[2] = light;
    glLightfv( GL_LIGHT1, GL_POSITION, lowZ_axis_pos );
    glLightfv( GL_LIGHT1, GL_DIFFUSE, light_color );
    glEnable( GL_LIGHT0 );      // White spot on Z axis
    glEnable( GL_LIGHT1 );      // White spot on Z axis ( bottom)
    glEnable( GL_LIGHTING );
}


/* Create a Screenshot of the current 3D view.
 *  Output file format is png or jpeg, or image is copied to the clipboard
 */
void Pcb3D_GLCanvas::TakeScreenshot( wxCommandEvent& event )
{
    wxFileName fn( m_Parent->m_Parent->GetScreen()->m_FileName );
    wxString   FullFileName;
    wxString   file_ext, mask;
    bool       fmt_is_jpeg = FALSE;

    if( event.GetId() == ID_MENU_SCREENCOPY_JPEG )
        fmt_is_jpeg = TRUE;
    if( event.GetId() != ID_TOOL_SCREENCOPY_TOCLIBBOARD )
    {
        file_ext     = fmt_is_jpeg ? wxT( "jpg" ) : wxT( "png" );
        mask         = wxT( "*." ) + file_ext;
        FullFileName = m_Parent->m_Parent->GetScreen()->m_FileName;
        fn.SetExt( file_ext );

        FullFileName =
            EDA_FileSelector( _( "3D Image filename:" ), wxEmptyString,
                              fn.GetFullName(), file_ext, mask, this,
                              wxFD_SAVE, TRUE );

        if( FullFileName.IsEmpty() )
            return;
    }

    wxSize image_size = GetClientSize();
    struct vieport_params
    {
        GLint originx;
        GLint originy;
        GLint x;
        GLint y;
    } viewport;

    // Build image from the 3D buffer
    wxWindowUpdateLocker noUpdates( this );
    glGetIntegerv( GL_VIEWPORT, (GLint*) &viewport );

    unsigned char*       pixelbuffer = (unsigned char*) malloc( viewport.x * viewport.y * 3 );
    unsigned char*       alphabuffer = (unsigned char*) malloc( viewport.x * viewport.y );
    wxImage image( viewport.x, viewport.y );

    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glReadBuffer( GL_BACK_LEFT );
    glReadPixels( viewport.originx, viewport.originy,
                  viewport.x, viewport.y,
                  GL_RGB, GL_UNSIGNED_BYTE, pixelbuffer );
    glReadPixels( viewport.originx, viewport.originy,
                  viewport.x, viewport.y,
                  GL_ALPHA, GL_UNSIGNED_BYTE, alphabuffer );


    image.SetData( pixelbuffer );
    image.SetAlpha( alphabuffer );
    image = image.Mirror( false );
    wxBitmap bitmap( image );

    if( event.GetId() == ID_TOOL_SCREENCOPY_TOCLIBBOARD )
    {
        wxBitmapDataObject* dobjBmp = new wxBitmapDataObject;
        dobjBmp->SetBitmap( bitmap );
        if( wxTheClipboard->Open() )
        {
            if( !wxTheClipboard->SetData( dobjBmp ) )
                wxMessageBox( _( "Failed to copy image to clipboard" ) );

            wxTheClipboard->Flush();    /* the data in clipboard will stay
                                         * available after the
                                         * application exits */
            wxTheClipboard->Close();
        }
    }
    else
    {
        wxImage image = bitmap.ConvertToImage();

        if( !image.SaveFile( FullFileName,
                             fmt_is_jpeg ? wxBITMAP_TYPE_JPEG :
                             wxBITMAP_TYPE_PNG ) )
            wxMessageBox( _( "Can't save file" ) );

        image.Destroy();
    }
}
