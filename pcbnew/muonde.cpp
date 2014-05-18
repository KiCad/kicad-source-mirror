/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file muonde.cpp
 * @brief Microwave pcb layout code.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <trigo.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <richio.h>
#include <filter_reader.h>
#include <gr_basic.h>
#include <pcbcommon.h>
#include <macros.h>
#include <base_units.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <pcbnew.h>


#define COEFF_COUNT 6

static std::vector< double > PolyEdges;
static double  ShapeScaleX, ShapeScaleY;
static wxSize  ShapeSize;
static int     PolyShapeType;


static void Exit_Self( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void gen_arc( std::vector <wxPoint>& aBuffer,
                     wxPoint                aStartPoint,
                     wxPoint                aCenter,
                     int                    a_ArcAngle );
static void ShowBoundingBoxMicroWaveInductor( EDA_DRAW_PANEL* apanel,
                                              wxDC*           aDC,
                                              const wxPoint&  aPosition,
                                              bool            aErase );


int         BuildCornersList_S_Shape( std::vector <wxPoint>& aBuffer,
                                      wxPoint aStartPoint, wxPoint aEndPoint,
                                      int aLength, int aWidth );

class SELFPCB
{
public:
    int     forme;          // Shape: coil, spiral, etc ..
    wxPoint m_Start;
    wxPoint m_End;
    wxSize  m_Size;
    int     lng;                        // Trace length.
    int     m_Width;                    // Trace width.
};

static SELFPCB Mself;
static int     Self_On;


/* This function shows on screen the bounding box of the inductor that will be
 * created at the end of the build inductor process
 */
static void ShowBoundingBoxMicroWaveInductor( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                              const wxPoint& aPosition, bool aErase )
{
    /* Calculate the orientation and size of the box containing the inductor:
     * the box is a rectangle with height = lenght/2
     * the shape is defined by a rectangle, nor necessary horizontal or vertical
     */
    GRSetDrawMode( aDC, GR_XOR );

    wxPoint poly[5];
    wxPoint pt    = Mself.m_End - Mself.m_Start;
    double  angle = -ArcTangente( pt.y, pt.x );
    int     len   = KiROUND( EuclideanNorm( pt ) );

    // calculate corners
    pt.x = 0; pt.y = len / 4;
    RotatePoint( &pt, angle );
    poly[0] = Mself.m_Start + pt;
    poly[1] = Mself.m_End + pt;
    pt.x    = 0; pt.y = -len / 4;
    RotatePoint( &pt, angle );
    poly[2] = Mself.m_End + pt;
    poly[3] = Mself.m_Start + pt;
    poly[4] = poly[0];

    if( aErase )
    {
        GRPoly( aPanel->GetClipBox(), aDC, 5, poly, false, 0, YELLOW, YELLOW );
    }

    Mself.m_End = aPanel->GetParent()->GetCrossHairPosition();
    pt    = Mself.m_End - Mself.m_Start;
    angle = -ArcTangente( pt.y, pt.x );
    len   = KiROUND( EuclideanNorm( pt ) );

    // calculate new corners
    pt.x = 0; pt.y = len / 4;
    RotatePoint( &pt, angle );
    poly[0] = Mself.m_Start + pt;
    poly[1] = Mself.m_End + pt;
    pt.x    = 0; pt.y = -len / 4;
    RotatePoint( &pt, angle );
    poly[2] = Mself.m_End + pt;
    poly[3] = Mself.m_Start + pt;
    poly[4] = poly[0];

    GRPoly( aPanel->GetClipBox(), aDC, 5, poly, false, 0, YELLOW, YELLOW );
}


void Exit_Self( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    if( Self_On )
    {
        Self_On = 0;
        Panel->CallMouseCapture( DC, wxDefaultPosition, 0 );
    }
}


void PCB_EDIT_FRAME::Begin_Self( wxDC* DC )
{
    if( Self_On )
    {
        Genere_Self( DC );
        return;
    }

    Mself.m_Start = GetCrossHairPosition();
    Mself.m_End   = Mself.m_Start;

    Self_On = 1;

    // Update the initial coordinates.
    GetScreen()->m_O_Curseur = GetCrossHairPosition();
    UpdateStatusBar();

    m_canvas->SetMouseCapture( ShowBoundingBoxMicroWaveInductor, Exit_Self );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
}


MODULE* PCB_EDIT_FRAME::Genere_Self( wxDC* DC )
{
    D_PAD*   pad;
    int      ll;
    wxString msg;

    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
    m_canvas->SetMouseCapture( NULL, NULL );

    if( Self_On == 0 )
    {
        DisplayError( this, wxT( "Starting point not init.." ) );
        return NULL;
    }

    Self_On = 0;

    Mself.m_End = GetCrossHairPosition();

    wxPoint pt = Mself.m_End - Mself.m_Start;
    int     min_len = KiROUND( EuclideanNorm( pt ) );
    Mself.lng = min_len;

    // Enter the desired length.
    msg = StringFromValue( g_UserUnit, Mself.lng );
    wxTextEntryDialog dlg( this, _( "Length:" ), _( "Length" ), msg );

    if( dlg.ShowModal() != wxID_OK )
        return NULL; // canceled by user

    msg = dlg.GetValue();
    Mself.lng = ValueFromString( g_UserUnit, msg );

    // Control values (ii = minimum length)
    if( Mself.lng < min_len )
    {
        DisplayError( this, _( "Requested length < minimum length" ) );
        return NULL;
    }

    // Calculate the elements.
    Mself.m_Width = GetDesignSettings().GetCurrentTrackWidth();

    std::vector <wxPoint> buffer;
    ll = BuildCornersList_S_Shape( buffer, Mself.m_Start, Mself.m_End, Mself.lng, Mself.m_Width );

    if( !ll )
    {
        DisplayError( this, _( "Requested length too large" ) );
        return NULL;
    }

    // Generate module.
    MODULE* module;
    module = Create_1_Module( wxEmptyString );

    if( module == NULL )
        return NULL;

    // here the module is already in the BOARD, Create_1_Module() does that.
    module->SetFPID( FPID( std::string( "MuSelf" ) ) );
    module->SetAttributes( MOD_VIRTUAL | MOD_CMS );
    module->ClearFlags();
    module->SetPosition( Mself.m_End );

    // Generate segments
    for( unsigned jj = 1; jj < buffer.size(); jj++ )
    {
        EDGE_MODULE* PtSegm;
        PtSegm = new EDGE_MODULE( module );
        PtSegm->SetStart( buffer[jj - 1] );
        PtSegm->SetEnd( buffer[jj] );
        PtSegm->SetWidth( Mself.m_Width );
        PtSegm->SetLayer( module->GetLayer() );
        PtSegm->SetShape( S_SEGMENT );
        PtSegm->SetStart0( PtSegm->GetStart() - module->GetPosition() );
        PtSegm->SetEnd0(   PtSegm->GetEnd()   - module->GetPosition() );
        module->GraphicalItems().PushBack( PtSegm );
    }

    // Place a pad on each end of coil.
    pad = new D_PAD( module );

    module->Pads().PushFront( pad );

    pad->SetPadName( wxT( "1" ) );
    pad->SetPosition( Mself.m_End );
    pad->SetPos0( pad->GetPosition() - module->GetPosition() );

    pad->SetSize( wxSize( Mself.m_Width, Mself.m_Width ) );

    pad->SetLayerMask( GetLayerMask( module->GetLayer() ) );
    pad->SetAttribute( PAD_SMD );
    pad->SetShape( PAD_CIRCLE );

    D_PAD* newpad = new D_PAD( *pad );

    module->Pads().Insert( newpad, pad->Next() );

    pad = newpad;
    pad->SetPadName( wxT( "2" ) );
    pad->SetPosition( Mself.m_Start );
    pad->SetPos0( pad->GetPosition() - module->GetPosition() );

    // Modify text positions.
    SetMsgPanel( module );

    wxPoint refPos( ( Mself.m_Start.x + Mself.m_End.x ) / 2,
                    ( Mself.m_Start.y + Mself.m_End.y ) / 2 );

    wxPoint valPos = refPos;

    refPos.y -= module->Reference().GetSize().y;
    module->Reference().SetTextPosition( refPos );
    valPos.y += module->Value().GetSize().y;
    module->Value().SetTextPosition( valPos );
    module->Reference().SetPos0( module->Reference().GetTextPosition() - module->GetPosition() );
    module->Value().SetPos0( module->Value().GetTextPosition() - module->GetPosition() );

    module->CalculateBoundingBox();
    module->Draw( m_canvas, DC, GR_OR );

    return module;
}


/**
 * Function  gen_arc
 * generates an arc using arc approximation by lines:
 * Center aCenter
 * Angle "angle" (in 0.1 deg)
 * @param  aBuffer = a buffer to store points.
 * @param  aStartPoint = starting point of arc.
 * @param  aCenter = arc centre.
 * @param  a_ArcAngle = arc length in 0.1 degrees.
 */
static void gen_arc( std::vector <wxPoint>& aBuffer,
                     wxPoint                aStartPoint,
                     wxPoint                aCenter,
                     int                    a_ArcAngle )
{
    #define SEGM_COUNT_PER_360DEG 16
    wxPoint first_point = aStartPoint - aCenter;
    int     seg_count   = ( ( abs( a_ArcAngle ) ) * SEGM_COUNT_PER_360DEG ) / 3600;

    if( seg_count == 0 )
        seg_count = 1;

    double increment_angle = (double) a_ArcAngle * M_PI / 1800 / seg_count;

    // Creates nb_seg point to approximate arc by segments:
    for( int ii = 1; ii <= seg_count; ii++ )
    {
        double  rot_angle = increment_angle * ii;
        double  fcos = cos( rot_angle );
        double  fsin = sin( rot_angle );
        wxPoint currpt;

        // Rotate current point:
        currpt.x = KiROUND( ( first_point.x * fcos + first_point.y * fsin ) );
        currpt.y = KiROUND( ( first_point.y * fcos - first_point.x * fsin ) );

        wxPoint corner = aCenter + currpt;
        aBuffer.push_back( corner );
    }
}


/**
 * Function BuildCornersList_S_Shape
 * Create a path like a S-shaped coil
 * @param  aBuffer =  a buffer where to store points (ends of segments)
 * @param  aStartPoint = starting point of the path
 * @param  aEndPoint = ending point of the path
 * @param  aLength = full lenght of the path
 * @param  aWidth = segment width
 */
int BuildCornersList_S_Shape( std::vector <wxPoint>& aBuffer,
                              wxPoint aStartPoint, wxPoint aEndPoint,
                              int aLength, int aWidth )
{
/* We must determine:
 * segm_count = number of segments perpendicular to the direction
 * segm_len = length of a strand
 * radius = radius of rounded parts of the coil
 * stubs_len = length of the 2 stubs( segments parallel to the direction)
 *         connecting the start point to the start point of the S shape
 *         and the ending point to the end point of the S shape
 * The equations are (assuming the area size of the entire shape is Size:
 * Size.x = 2 * radius + segm_len
 * Size.y = (segm_count + 2 ) * 2 * radius + 2 * stubs_len
 * Mself.lng = 2 * delta // connections to the coil
 *             + (segm_count-2) * segm_len      // length of the strands except 1st and last
 *             + (segm_count) * (PI * radius)   // length of rounded
 * segm_len + / 2 - radius * 2)                 // length of 1st and last bit
 *
 * The constraints are:
 * segm_count >= 2
 * radius < m_Size.x
 * Size.y = (radius * 4) + (2 * stubs_len)
 * segm_len > radius * 2
 *
 * The calculation is conducted in the following way:
 * first:
 * segm_count = 2
 * radius = 4 * Size.x (arbitrarily fixed value)
 * Then:
 * Increasing the number of segments to the desired length
 * (radius decreases if necessary)
 */
    wxSize size;

    // This scale factor adjusts the arc length to handle
    // the arc to segment approximation.
    // because we use SEGM_COUNT_PER_360DEG segment to approximate a circle,
    // the trace len must be corrected when calculated using arcs
    // this factor adjust calculations and must be changed if SEGM_COUNT_PER_360DEG is modified
    // because trace using segment is shorter the corresponding arc
    // ADJUST_SIZE is the ratio between tline len and the arc len for an arc
    // of 360/ADJUST_SIZE angle
    #define ADJUST_SIZE 0.988

    wxPoint pt       = aEndPoint - aStartPoint;
    double  angle    = -ArcTangente( pt.y, pt.x );
    int     min_len  = KiROUND( EuclideanNorm( pt ) );
    int     segm_len = 0;           // length of segments
    int     full_len;               // full len of shape (sum of lenght of all segments + arcs)


    /* Note: calculations are made for a vertical coil (more easy calculations)
     * and after points are rotated to their actual position
     * So the main direction is the Y axis.
     * the 2 stubs are on the Y axis
     * the others segments are parallel to the X axis.
     */

    // Calculate the size of area (for a vertical shape)
    size.x = min_len / 2;
    size.y = min_len;

    // Choose a reasonable starting value for the radius of the arcs.
    int radius = std::min( aWidth * 5, size.x / 4 );

    int segm_count;     // number of full len segments
                        // the half size segments (first and last segment) are not counted here
    int stubs_len = 0;  // lenght of first or last segment (half size of others segments)

    for( segm_count = 0; ; segm_count++ )
    {
        stubs_len = ( size.y - ( radius * 2 * (segm_count + 2 ) ) ) / 2;

        if( stubs_len < size.y / 10 ) // Reduce radius.
        {
            stubs_len = size.y / 10;
            radius    = ( size.y - (2 * stubs_len) ) / ( 2 * (segm_count + 2) );

            if( radius < aWidth ) // Radius too small.
            {
                // Unable to create line: Requested length value is too large for room
                return 0;
            }
        }

        segm_len  = size.x - ( radius * 2 );
        full_len  = 2 * stubs_len;               // Length of coil connections.
        full_len += segm_len * segm_count;       // Length of full length segments.
        full_len += KiROUND( ( segm_count + 2 ) * M_PI * ADJUST_SIZE * radius );    // Ard arcs len
        full_len += segm_len - (2 * radius);     // Length of first and last segments
                                                 // (half size segments len = segm_len/2 - radius).

        if( full_len >= aLength )
            break;
    }

    // Adjust len by adjusting segm_len:
    int delta_size = full_len - aLength;

    // reduce len of the segm_count segments + 2 half size segments (= 1 full size segment)
    segm_len -= delta_size / (segm_count + 1);

    // Generate first line (the first stub) and first arc (90 deg arc)
    pt = aStartPoint;
    aBuffer.push_back( pt );
    pt.y += stubs_len;
    aBuffer.push_back( pt );

    wxPoint centre = pt;
    centre.x -= radius;
    gen_arc( aBuffer, pt, centre, -900 );
    pt = aBuffer.back();

    int half_size_seg_len = segm_len / 2 - radius;

    if( half_size_seg_len )
    {
        pt.x -= half_size_seg_len;
        aBuffer.push_back( pt );
    }

    // Create shape.
    int ii;
    int sign = 1;
    segm_count += 1;    // increase segm_count to create the last half_size segment

    for( ii = 0; ii < segm_count; ii++ )
    {
        int arc_angle;

        if( ii & 1 ) // odd order arcs are greater than 0
            sign = -1;
        else
            sign = 1;

        arc_angle = 1800 * sign;
        centre    = pt;
        centre.y += radius;
        gen_arc( aBuffer, pt, centre, arc_angle );
        pt    = aBuffer.back();
        pt.x += segm_len * sign;
        aBuffer.push_back( pt );
    }

    // The last point is false:
    // it is the end of a full size segment, but must be
    // the end of the second half_size segment. Change it.
    sign *= -1;
    aBuffer.back().x = aStartPoint.x + radius * sign;

    // create last arc
    pt        = aBuffer.back();
    centre    = pt;
    centre.y += radius;
    gen_arc( aBuffer, pt, centre, 900 * sign );    pt = aBuffer.back();

    // Rotate point
    angle += 900;

    for( unsigned jj = 0; jj < aBuffer.size(); jj++ )
    {
        RotatePoint( &aBuffer[jj].x, &aBuffer[jj].y, aStartPoint.x, aStartPoint.y, angle );
    }

    // push last point (end point)
    aBuffer.push_back( aEndPoint );

    return 1;
}


MODULE* PCB_EDIT_FRAME::Create_MuWaveBasicShape( const wxString& name, int pad_count )
{
    MODULE*  module;
    int      pad_num = 1;
    wxString Line;

    module = Create_1_Module( name );

    if( module == NULL )
        return NULL;

    #define DEFAULT_SIZE 30
    module->SetTimeStamp( GetNewTimeStamp() );

    module->Value().SetSize( wxSize( DEFAULT_SIZE, DEFAULT_SIZE ) );

    module->Value().SetPos0( wxPoint( 0, -DEFAULT_SIZE ) );

    module->Value().Offset( wxPoint( 0, module->Value().GetPos0().y ) );

    module->Value().SetThickness( DEFAULT_SIZE / 4 );

    module->Reference().SetSize( wxSize( DEFAULT_SIZE, DEFAULT_SIZE ) );

    module->Reference().SetPos0( wxPoint( 0, DEFAULT_SIZE ) );

    module->Reference().Offset( wxPoint( 0, module->Reference().GetPos0().y ) );

    module->Reference().SetThickness( DEFAULT_SIZE / 4 );

    // Create 2 pads used in gaps and stubs.  The gap is between these 2 pads
    // the stub is the pad 2
    while( pad_count-- )
    {
        D_PAD* pad = new D_PAD( module );

        module->Pads().PushFront( pad );

        int tw = GetDesignSettings().GetCurrentTrackWidth();
        pad->SetSize( wxSize( tw, tw ) );

        pad->SetPosition( module->GetPosition() );
        pad->SetShape( PAD_RECT );
        pad->SetAttribute( PAD_SMD );
        pad->SetLayerMask( LAYER_FRONT );

        Line.Printf( wxT( "%d" ), pad_num );
        pad->SetPadName( Line );
        pad_num++;
    }

    return module;
}


MODULE* PCB_EDIT_FRAME::Create_MuWaveComponent( int shape_type )
{
    int      oX;
    D_PAD*   pad;
    MODULE*  module;
    wxString msg, cmp_name;
    int      pad_count = 2;
    int      angle     = 0;

    // Enter the size of the gap or stub
    int      gap_size = GetDesignSettings().GetCurrentTrackWidth();

    switch( shape_type )
    {
    case 0:
        msg = _( "Gap" );
        cmp_name = wxT( "GAP" );
        break;

    case 1:
        msg = _( "Stub" );
        cmp_name  = wxT( "STUB" );
        pad_count = 2;
        break;

    case 2:
        msg = _( "Arc Stub" );
        cmp_name  = wxT( "ASTUB" );
        pad_count = 1;
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    wxString          value = StringFromValue( g_UserUnit, gap_size );
    wxTextEntryDialog dlg( this, msg, _( "Create microwave module" ), value );

    if( dlg.ShowModal() != wxID_OK )
    {
        m_canvas->MoveCursorToCrossHair();
        return NULL; // cancelled by user
    }

    value    = dlg.GetValue();
    gap_size = ValueFromString( g_UserUnit, value );

    bool abort = false;

    if( shape_type == 2 )
    {
        double            fcoeff = 10.0, fval;
        msg.Printf( wxT( "%3.1f" ), angle / fcoeff );
        wxTextEntryDialog angledlg( this, _( "Angle (0.1deg):" ),
                                    _( "Create microwave module" ), msg );

        if( angledlg.ShowModal() != wxID_OK )
        {
            m_canvas->MoveCursorToCrossHair();
            return NULL; // cancelled by user
        }

        msg = angledlg.GetValue();

        if( !msg.ToDouble( &fval ) )
        {
            DisplayError( this, _( "Incorrect number, abort" ) );
            abort = true;
        }

        angle = std::abs( KiROUND( fval * fcoeff ) );

        if( angle > 1800 )
            angle = 1800;
    }

    if( abort )
    {
        m_canvas->MoveCursorToCrossHair();
        return NULL;
    }

    module = Create_MuWaveBasicShape( cmp_name, pad_count );
    pad    = module->Pads();

    switch( shape_type )
    {
    case 0:     //Gap :
        oX = -( gap_size + pad->GetSize().x ) / 2;
        pad->SetX0( oX );

        pad->SetX( pad->GetPos0().x + pad->GetPosition().x );

        pad = pad->Next();

        pad->SetX0( oX + gap_size + pad->GetSize().x );
        pad->SetX( pad->GetPos0().x + pad->GetPosition().x );
        break;

    case 1:     //Stub :
        pad->SetPadName( wxT( "1" ) );
        pad = pad->Next();
        pad->SetY0( -( gap_size + pad->GetSize().y ) / 2 );
        pad->SetSize( wxSize( pad->GetSize().x, gap_size ) );
        pad->SetY( pad->GetPos0().y + pad->GetPosition().y );
        break;

    case 2:     // Arc Stub created by a polygonal approach:
    {
        EDGE_MODULE* edge = new EDGE_MODULE( module );
        module->GraphicalItems().PushFront( edge );

        edge->SetShape( S_POLYGON );
        edge->SetLayer( LAYER_N_FRONT );

        int numPoints = angle / 50 + 3;     // Note: angles are in 0.1 degrees
        std::vector<wxPoint> polyPoints = edge->GetPolyPoints();
        polyPoints.reserve( numPoints );

        edge->m_Start0.y = -pad->GetSize().y / 2;

        polyPoints.push_back( wxPoint( 0, 0 ) );

        int theta = -angle / 2;

        for( int ii = 1; ii<numPoints - 1; ii++ )
        {
            wxPoint pt( 0, -gap_size );

            RotatePoint( &pt.x, &pt.y, theta );

            polyPoints.push_back( pt );

            theta += 50;

            if( theta > angle / 2 )
                theta = angle / 2;
        }

        // Close the polygon:
        polyPoints.push_back( polyPoints[0] );
    }
        break;

    default:
        break;
    }

    module->CalculateBoundingBox();
    GetBoard()->m_Status_Pcb = 0;
    OnModify();
    return module;
}


/**************** Polygon Shapes ***********************/

enum id_mw_cmd {
    ID_READ_SHAPE_FILE = 1000
};


/* Setting polynomial form parameters
 */
class WinEDA_SetParamShapeFrame : public wxDialog
{
private:
    PCB_EDIT_FRAME*  m_Parent;
    wxRadioBox*      m_ShapeOptionCtrl;
    EDA_SIZE_CTRL*   m_SizeCtrl;

public:
    WinEDA_SetParamShapeFrame( PCB_EDIT_FRAME* parent, const wxPoint& pos );
    ~WinEDA_SetParamShapeFrame() { };

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    /**
     * Function ReadDataShapeDescr
     * read a description shape file
     *  File format is
     *  Unit=MM
     *  XScale=271.501
     *  YScale=1.00133
     *
     *  $COORD
     *  0                      0.6112600148417837
     *  0.001851851851851852   0.6104800531118608
     *  ....
     *  $ENDCOORD
     *
     *  Each line is the X Y coord (normalized units from 0 to 1)
     */
    void ReadDataShapeDescr( wxCommandEvent& event );
    void AcceptOptions( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( WinEDA_SetParamShapeFrame, wxDialog )
    EVT_BUTTON( wxID_OK, WinEDA_SetParamShapeFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SetParamShapeFrame::OnCancelClick )
    EVT_BUTTON( ID_READ_SHAPE_FILE, WinEDA_SetParamShapeFrame::ReadDataShapeDescr )
END_EVENT_TABLE()


WinEDA_SetParamShapeFrame::WinEDA_SetParamShapeFrame( PCB_EDIT_FRAME* parent,
                                                      const wxPoint&  framepos ) :
    wxDialog( parent, -1, _( "Complex shape" ), framepos, wxSize( 350, 280 ),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    m_Parent = parent;

    PolyEdges.clear();

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    wxButton* Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, ID_READ_SHAPE_FILE,
                           _( "Read Shape Description File..." ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    wxString shapelist[3] =
    {
        _( "Normal" ), _( "Symmetrical" ),
        _( "Mirrored" )
    };

    m_ShapeOptionCtrl = new wxRadioBox( this, -1, _( "Shape Option" ),
                                        wxDefaultPosition, wxDefaultSize, 3,
                                        shapelist, 1,
                                        wxRA_SPECIFY_COLS );
    LeftBoxSizer->Add( m_ShapeOptionCtrl, 0, wxGROW | wxALL, 5 );

    m_SizeCtrl = new EDA_SIZE_CTRL( this, _( "Size" ), ShapeSize, g_UserUnit, LeftBoxSizer );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void WinEDA_SetParamShapeFrame::OnCancelClick( wxCommandEvent& event )
{
    PolyEdges.clear();
    EndModal( -1 );
}


void WinEDA_SetParamShapeFrame::OnOkClick( wxCommandEvent& event )
{
    ShapeSize     = m_SizeCtrl->GetValue();
    PolyShapeType = m_ShapeOptionCtrl->GetSelection();
    EndModal( 1 );
}


void WinEDA_SetParamShapeFrame::ReadDataShapeDescr( wxCommandEvent& event )
{
    wxString FullFileName;
    wxString ext, mask;
    FILE*    File;
    char*    Line;
    double   unitconv = 10000;
    char*    param1, * param2;

    ext  = wxT( ".txt" );
    mask = wxT( "*" ) + ext;
    FullFileName = EDA_FileSelector( _( "Read descr shape file" ),
                                     wxEmptyString,
                                     FullFileName,
                                     ext,
                                     mask,
                                     this,
                                     wxFD_OPEN,
                                     true );
    if( FullFileName.IsEmpty() )
        return;

    File = wxFopen( FullFileName, wxT( "rt" ) );

    if( File == NULL )
    {
        DisplayError( this, _( "File not found" ) );
        return;
    }

    FILE_LINE_READER fileReader( File, FullFileName );

    FILTER_READER reader( fileReader );

    LOCALE_IO   toggle;

    while( reader.ReadLine() )
    {
        Line = reader.Line();
        param1 = strtok( Line, " =\n\r" );
        param2 = strtok( NULL, " \t\n\r" );

        if( strnicmp( param1, "Unit", 4 ) == 0 )
        {
            if( strnicmp( param2, "inch", 4 ) == 0 )
                unitconv = 10000;

            if( strnicmp( param2, "mm", 2 ) == 0 )
                unitconv = 10000 / 25.4;
        }

        if( strnicmp( param1, "$ENDCOORD", 8 ) == 0 )
            break;

        if( strnicmp( param1, "$COORD", 6 ) == 0 )
        {
            while( reader.ReadLine() )
            {
                Line = reader.Line();
                param1 = strtok( Line, " \t\n\r" );
                param2 = strtok( NULL, " \t\n\r" );

                if( strnicmp( param1, "$ENDCOORD", 8 ) == 0 )
                    break;

                PolyEdges.push_back( atof( param1 ) );
                PolyEdges.push_back( atof( param2 ) );
            }
        }

        if( strnicmp( Line, "XScale", 6 ) == 0 )
        {
            ShapeScaleX = atof( param2 );
        }

        if( strnicmp( Line, "YScale", 6 ) == 0 )
        {
            ShapeScaleY = atof( param2 );
        }
    }

    ShapeScaleX *= unitconv;
    ShapeScaleY *= unitconv;

    m_SizeCtrl->SetValue( (int) ShapeScaleX, (int) ShapeScaleY );
}


MODULE* PCB_EDIT_FRAME::Create_MuWavePolygonShape()
{
    D_PAD*       pad1, * pad2;
    MODULE*      module;
    wxString     cmp_name;
    int          pad_count = 2;
    EDGE_MODULE* edge;

    WinEDA_SetParamShapeFrame* frame = new WinEDA_SetParamShapeFrame( this, wxPoint( -1, -1 ) );

    int ok = frame->ShowModal();

    frame->Destroy();

    m_canvas->MoveCursorToCrossHair();

    if( ok != 1 )
    {
        PolyEdges.clear();
    }

    if( PolyShapeType == 2 )  // mirrored
        ShapeScaleY = -ShapeScaleY;

    ShapeSize.x = KiROUND( ShapeScaleX );
    ShapeSize.y = KiROUND( ShapeScaleY );

    if( ( ShapeSize.x ) == 0 || ( ShapeSize.y == 0 ) )
    {
        DisplayError( this, _( "Shape has a null size!" ) );
        return NULL;
    }

    if( PolyEdges.size() == 0 )
    {
        DisplayError( this, _( "Shape has no points!" ) );
        return NULL;
    }

    cmp_name = wxT( "POLY" );

    module = Create_MuWaveBasicShape( cmp_name, pad_count );
    pad1   = module->Pads();

    pad1->SetX0( -ShapeSize.x / 2 );
    pad1->SetX( pad1->GetPos0().x + pad1->GetPosition().x );

    pad2 = (D_PAD*) pad1->Next();
    pad2->SetX0( pad1->GetPos0().x + ShapeSize.x );
    pad2->SetX( pad2->GetPos0().x + pad2->GetPosition().x );

    edge = new EDGE_MODULE( module );

    module->GraphicalItems().PushFront( edge );

    edge->SetShape( S_POLYGON );
    edge->SetLayer( LAYER_N_FRONT );

    std::vector<wxPoint> polyPoints = edge->GetPolyPoints();
    polyPoints.reserve( 2 * PolyEdges.size() + 2 );

    // Init start point coord:
    polyPoints.push_back( wxPoint( pad1->GetPos0().x, 0 ) );

    wxPoint first_coordinate, last_coordinate;

    for( unsigned ii = 0; ii < PolyEdges.size(); ii++ )  // Copy points
    {
        last_coordinate.x = KiROUND( PolyEdges[ii] * ShapeScaleX ) + pad1->GetPos0().x;
        last_coordinate.y = -KiROUND( PolyEdges[ii] * ShapeScaleY );
        polyPoints.push_back( last_coordinate );
    }

    first_coordinate.y = polyPoints[1].y;

    switch( PolyShapeType )
    {
    case 0:     // Single
    case 2:     // Single mirrored

        // Init end point coord:
        pad2->SetX0( last_coordinate.x );
        polyPoints.push_back( wxPoint( last_coordinate.x, 0 ) );

        pad1->SetSize( wxSize( std::abs( first_coordinate.y ),
                       std::abs( first_coordinate.y ) ) );
        pad2->SetSize( wxSize( std::abs( last_coordinate.y ),
                       std::abs( last_coordinate.y ) ) );

        pad1->SetY0( first_coordinate.y / 2 );
        pad2->SetY0( last_coordinate.y / 2 );

        pad1->SetY( pad1->GetPos0().y + module->GetPosition().y );
        pad2->SetY( pad2->GetPos0().y + module->GetPosition().y );
        break;

    case 1:     // Symmetric
        for( int ndx = polyPoints.size() - 1; ndx>=0; --ndx )
        {
            wxPoint pt = polyPoints[ndx];

            pt.y = -pt.y;   // mirror about X axis

            polyPoints.push_back( pt );
        }

        pad1->SetSize( wxSize( 2 * std::abs( first_coordinate.y ),
                       2 * std::abs( first_coordinate.y ) ) );
        pad2->SetSize( wxSize( 2 * std::abs( last_coordinate.y ),
                       2 * std::abs( last_coordinate.y ) ) );
        break;
    }

    PolyEdges.clear();
    module->CalculateBoundingBox();
    GetBoard()->m_Status_Pcb = 0;
    OnModify();
    return module;
}


void PCB_EDIT_FRAME::Edit_Gap( wxDC* DC, MODULE* aModule )
{
    int      gap_size, oX;
    D_PAD*   pad, * next_pad;
    wxString msg;

    if( aModule == NULL )
        return;

    // Test if module is a gap type (name begins with GAP, and has 2 pads).
    msg = aModule->GetReference().Left( 3 );

    if( msg != wxT( "GAP" ) )
        return;

    pad = aModule->Pads();

    if( pad == NULL )
    {
        DisplayError( this, _( "No pad for this module" ) );
        return;
    }

    next_pad = (D_PAD*) pad->Next();

    if( next_pad == NULL )
    {
        DisplayError( this, _( "Only one pad for this module" ) );
        return;
    }

    aModule->Draw( m_canvas, DC, GR_XOR );

    // Calculate the current dimension.
    gap_size = next_pad->GetPos0().x - pad->GetPos0().x - pad->GetSize().x;

    // Entrer the desired length of the gap.
    msg = StringFromValue( g_UserUnit, gap_size );
    wxTextEntryDialog dlg( this, _( "Gap:" ), _( "Create Microwave Gap" ), msg );

    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    msg = dlg.GetValue();
    gap_size = ValueFromString( g_UserUnit, msg );

    // Updating sizes of pads forming the gap.
    int tw = GetDesignSettings().GetCurrentTrackWidth();
    pad->SetSize( wxSize( tw, tw ) );

    pad->SetY0( 0 );
    oX = -( gap_size + pad->GetSize().x ) / 2;
    pad->SetX0( oX );

    wxPoint padpos = pad->GetPos0() + aModule->GetPosition();

    RotatePoint( &padpos.x, &padpos.y,
                 aModule->GetPosition().x, aModule->GetPosition().y, aModule->GetOrientation() );

    pad->SetPosition( padpos );

    tw = GetDesignSettings().GetCurrentTrackWidth();
    next_pad->SetSize( wxSize( tw, tw ) );

    next_pad->SetY0( 0 );
    next_pad->SetX0( oX + gap_size + next_pad->GetSize().x );

    padpos = next_pad->GetPos0() + aModule->GetPosition();

    RotatePoint( &padpos.x, &padpos.y,
                 aModule->GetPosition().x, aModule->GetPosition().y, aModule->GetOrientation() );

    next_pad->SetPosition( padpos );

    aModule->Draw( m_canvas, DC, GR_OR );
}
