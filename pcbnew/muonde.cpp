/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <macros.h>
#include <base_units.h>
#include <validators.h> //

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <microwave/microwave_inductor.h>

#include <pcbnew.h>

static std::vector< wxRealPoint > PolyEdges;
static double  ShapeScaleX, ShapeScaleY;
static wxSize  ShapeSize;
static int     PolyShapeType;


static void Exit_Self( EDA_DRAW_PANEL* aPanel, wxDC* aDC );

static void ShowBoundingBoxMicroWaveInductor( EDA_DRAW_PANEL* aPanel,
                                              wxDC*           aDC,
                                              const wxPoint&  aPosition,
                                              bool            aErase );


///> An inductor pattern temporarily used during mu-wave inductor creation
static MWAVE::INDUCTOR_PATTERN s_inductor_pattern;

///> A flag set to true when mu-wave inductor is being created
static bool s_inductorInProgress = false;


/* This function shows on screen the bounding box of the inductor that will be
 * created at the end of the build inductor process
 */
static void ShowBoundingBoxMicroWaveInductor( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                              const wxPoint& aPosition, bool aErase )
{
    /* Calculate the orientation and size of the box containing the inductor:
     * the box is a rectangle with height = length/2
     * the shape is defined by a rectangle, nor necessary horizontal or vertical
     */
    GRSetDrawMode( aDC, GR_XOR );

    wxPoint poly[5];
    wxPoint pt    = s_inductor_pattern.m_End - s_inductor_pattern.m_Start;
    double  angle = -ArcTangente( pt.y, pt.x );
    int     len   = KiROUND( EuclideanNorm( pt ) );

    // calculate corners
    pt.x = 0; pt.y = len / 4;
    RotatePoint( &pt, angle );
    poly[0] = s_inductor_pattern.m_Start + pt;
    poly[1] = s_inductor_pattern.m_End + pt;
    pt.x    = 0; pt.y = -len / 4;
    RotatePoint( &pt, angle );
    poly[2] = s_inductor_pattern.m_End + pt;
    poly[3] = s_inductor_pattern.m_Start + pt;
    poly[4] = poly[0];

    if( aErase )
    {
        GRPoly( aPanel->GetClipBox(), aDC, 5, poly, false, 0, YELLOW, YELLOW );
    }

    s_inductor_pattern.m_End = aPanel->GetParent()->GetCrossHairPosition();
    pt    = s_inductor_pattern.m_End - s_inductor_pattern.m_Start;
    angle = -ArcTangente( pt.y, pt.x );
    len   = KiROUND( EuclideanNorm( pt ) );

    // calculate new corners
    pt.x = 0; pt.y = len / 4;
    RotatePoint( &pt, angle );
    poly[0] = s_inductor_pattern.m_Start + pt;
    poly[1] = s_inductor_pattern.m_End + pt;
    pt.x    = 0; pt.y = -len / 4;
    RotatePoint( &pt, angle );
    poly[2] = s_inductor_pattern.m_End + pt;
    poly[3] = s_inductor_pattern.m_Start + pt;
    poly[4] = poly[0];

    GRPoly( aPanel->GetClipBox(), aDC, 5, poly, false, 0, YELLOW, YELLOW );
}


void Exit_Self( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    if( aPanel->IsMouseCaptured() )
        aPanel->CallMouseCapture( aDC, wxDefaultPosition, false );

    s_inductorInProgress = false;
    aPanel->SetMouseCapture( NULL, NULL );
}


void PCB_EDIT_FRAME::Begin_Self( wxDC* DC )
{
    if( s_inductorInProgress )
    {
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
        m_canvas->SetMouseCapture( NULL, NULL );

        wxString errorMessage;

        // Prepare parameters for inductor
        // s_inductor_pattern.m_Start is already initialized,
        // when s_inductor_pattern.m_Flag == false
        s_inductor_pattern.m_Width = GetDesignSettings().GetCurrentTrackWidth();
        s_inductor_pattern.m_End = GetCrossHairPosition();

        wxASSERT( s_inductorInProgress );
        s_inductorInProgress = false;

        MODULE* footprint = MWAVE::CreateMicrowaveInductor( s_inductor_pattern, this, errorMessage );

        if( footprint )
        {
            SetMsgPanel( footprint );
            footprint->Draw( m_canvas, DC, GR_OR );
        }

        else if( !errorMessage.IsEmpty() )
            DisplayError( this, errorMessage );

        return;
    }

    s_inductor_pattern.m_Start = GetCrossHairPosition();
    s_inductor_pattern.m_End   = s_inductor_pattern.m_Start;

    s_inductorInProgress = true;

    // Update the initial coordinates.
    GetScreen()->m_O_Curseur = GetCrossHairPosition();
    UpdateStatusBar();

    m_canvas->SetMouseCapture( ShowBoundingBoxMicroWaveInductor, Exit_Self );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
}


MODULE* PCB_EDIT_FRAME::CreateMuWaveBaseFootprint( const wxString& aValue,
                                                   int aTextSize, int aPadCount )
{
    MODULE* module = CreateNewModule( aValue );

    if( aTextSize > 0 )
    {
        module->Reference().SetTextSize( wxSize( aTextSize, aTextSize ) );
        module->Reference().SetThickness( aTextSize/5 );
        module->Value().SetTextSize( wxSize( aTextSize, aTextSize ) );
        module->Value().SetThickness( aTextSize/5 );
    }

    // Create 2 pads used in gaps and stubs.  The gap is between these 2 pads
    // the stub is the pad 2
    wxString Line;
    int pad_num = 1;

    while( aPadCount-- )
    {
        D_PAD* pad = new D_PAD( module );

        module->Pads().PushFront( pad );

        int tw = GetDesignSettings().GetCurrentTrackWidth();
        pad->SetSize( wxSize( tw, tw ) );

        pad->SetPosition( module->GetPosition() );
        pad->SetShape( PAD_SHAPE_RECT );
        pad->SetAttribute( PAD_ATTRIB_SMD );
        pad->SetLayerSet( F_Cu );

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
    // Ref and value text size (O = use board default value.
    // will be set to a value depending on the footprint size, if possible
    int      text_size = 0;

    // Enter the size of the gap or stub
    int      gap_size = GetDesignSettings().GetCurrentTrackWidth();

    switch( shape_type )
    {
    case 0:
        msg = _( "Gap" );
        cmp_name = wxT( "muwave_gap" );
        text_size = gap_size;
        break;

    case 1:
        msg = _( "Stub" );
        cmp_name  = wxT( "muwave_stub" );
        text_size = gap_size;
        pad_count = 2;
        break;

    case 2:
        msg = _( "Arc Stub" );
        cmp_name  = wxT( "muwave_arcstub" );
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
        wxTextEntryDialog angledlg( this, _( "Angle in degrees:" ),
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

    module = CreateMuWaveBaseFootprint( cmp_name, text_size, pad_count );
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
        edge->SetLayer( F_Cu );

        int numPoints = (angle / 50) + 3;     // Note: angles are in 0.1 degrees
        std::vector<wxPoint>& polyPoints = edge->GetPolyPoints();
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
class MWAVE_POLYGONAL_SHAPE_DLG : public wxDialog
{
private:
    PCB_EDIT_FRAME*  m_Parent;
    wxRadioBox*      m_ShapeOptionCtrl;
    EDA_SIZE_CTRL*   m_SizeCtrl;

public:
    MWAVE_POLYGONAL_SHAPE_DLG( PCB_EDIT_FRAME* parent, const wxPoint& pos );
    ~MWAVE_POLYGONAL_SHAPE_DLG() { };

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


BEGIN_EVENT_TABLE( MWAVE_POLYGONAL_SHAPE_DLG, wxDialog )
    EVT_BUTTON( wxID_OK, MWAVE_POLYGONAL_SHAPE_DLG::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, MWAVE_POLYGONAL_SHAPE_DLG::OnCancelClick )
    EVT_BUTTON( ID_READ_SHAPE_FILE, MWAVE_POLYGONAL_SHAPE_DLG::ReadDataShapeDescr )
END_EVENT_TABLE()


MWAVE_POLYGONAL_SHAPE_DLG::MWAVE_POLYGONAL_SHAPE_DLG( PCB_EDIT_FRAME* parent,
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
        _( "Normal" ), _( "Symmetrical" ), _( "Mirrored" )
    };

    m_ShapeOptionCtrl = new wxRadioBox( this, -1, _( "Shape Option" ),
                                        wxDefaultPosition, wxDefaultSize, 3,
                                        shapelist, 1,
                                        wxRA_SPECIFY_COLS );
    LeftBoxSizer->Add( m_ShapeOptionCtrl, 0, wxGROW | wxALL, 5 );

    m_SizeCtrl = new EDA_SIZE_CTRL( this, _( "Size" ), ShapeSize, g_UserUnit, LeftBoxSizer );

    GetSizer()->SetSizeHints( this );
}


void MWAVE_POLYGONAL_SHAPE_DLG::OnCancelClick( wxCommandEvent& event )
{
    PolyEdges.clear();
    EndModal( wxID_CANCEL );
}


void MWAVE_POLYGONAL_SHAPE_DLG::OnOkClick( wxCommandEvent& event )
{
    ShapeSize     = m_SizeCtrl->GetValue();
    PolyShapeType = m_ShapeOptionCtrl->GetSelection();
    EndModal( wxID_OK );
}


void MWAVE_POLYGONAL_SHAPE_DLG::ReadDataShapeDescr( wxCommandEvent& event )
{
    static wxString lastpath;       // To remember the last open path during a session
    wxString mask = wxT( "*.*" );

    wxString FullFileName = EDA_FILE_SELECTOR( _( "Read descr shape file" ),
                                               lastpath, FullFileName,
                                               wxEmptyString, mask,
                                               this, wxFD_OPEN, true );
    if( FullFileName.IsEmpty() )
        return;

    wxFileName fn( FullFileName );
    lastpath = fn.GetPath();
    PolyEdges.clear();

    FILE* File = wxFopen( FullFileName, wxT( "rt" ) );

    if( File == NULL )
    {
        DisplayError( this, _( "File not found" ) );
        return;
    }

    double   unitconv = IU_PER_MM;
    ShapeScaleX = ShapeScaleY = 1.0;

    FILE_LINE_READER fileReader( File, FullFileName );
    FILTER_READER reader( fileReader );

    LOCALE_IO   toggle;

    while( reader.ReadLine() )
    {
        char* Line = reader.Line();
        char* param1 = strtok( Line, " =\n\r" );
        char* param2 = strtok( NULL, " \t\n\r" );

        if( strncasecmp( param1, "Unit", 4 ) == 0 )
        {
            if( strncasecmp( param2, "inch", 4 ) == 0 )
                unitconv = IU_PER_MILS*1000;

            if( strncasecmp( param2, "mm", 2 ) == 0 )
                unitconv = IU_PER_MM;
        }

        if( strncasecmp( param1, "$ENDCOORD", 8 ) == 0 )
            break;

        if( strncasecmp( param1, "$COORD", 6 ) == 0 )
        {
            while( reader.ReadLine() )
            {
                Line = reader.Line();
                param1 = strtok( Line, " \t\n\r" );
                param2 = strtok( NULL, " \t\n\r" );

                if( strncasecmp( param1, "$ENDCOORD", 8 ) == 0 )
                    break;

                wxRealPoint coord( atof( param1 ), atof( param2 ) );
                PolyEdges.push_back( coord );
            }
        }

        if( strncasecmp( Line, "XScale", 6 ) == 0 )
            ShapeScaleX = atof( param2 );

        if( strncasecmp( Line, "YScale", 6 ) == 0 )
            ShapeScaleY = atof( param2 );
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

    MWAVE_POLYGONAL_SHAPE_DLG dlg( this, wxPoint( -1, -1 ) );

    int ret = dlg.ShowModal();

    m_canvas->MoveCursorToCrossHair();

    if( ret != wxID_OK )
    {
        PolyEdges.clear();
        return NULL;
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

    cmp_name = wxT( "muwave_polygon" );

    // Create a footprint with 2 pads, orientation = 0, pos 0
    module = CreateMuWaveBaseFootprint( cmp_name, 0, pad_count );

    // We try to place the footprint anchor to the middle of the shape len
    wxPoint offset;
    offset.x = -ShapeSize.x / 2;

    pad1   = module->Pads();
    pad1->SetX0( offset.x );
    pad1->SetX( pad1->GetPos0().x );

    pad2 = pad1->Next();
    pad2->SetX0( offset.x + ShapeSize.x );
    pad2->SetX( pad2->GetPos0().x );

    // Add a polygonal edge (corners will be added later) on copper layer
    edge = new EDGE_MODULE( module );
    edge->SetShape( S_POLYGON );
    edge->SetLayer( F_Cu );

    module->GraphicalItems().PushFront( edge );

    // Get the corner buffer of the polygonal edge
    std::vector<wxPoint>& polyPoints = edge->GetPolyPoints();
    polyPoints.reserve( PolyEdges.size() + 2 );

    // Init start point coord:
    polyPoints.push_back( wxPoint( offset.x, 0 ) );

    wxPoint last_coordinate;

    for( unsigned ii = 0; ii < PolyEdges.size(); ii++ )  // Copy points
    {
        last_coordinate.x = KiROUND( PolyEdges[ii].x * ShapeScaleX );
        last_coordinate.y = -KiROUND( PolyEdges[ii].y * ShapeScaleY );
        last_coordinate += offset;
        polyPoints.push_back( last_coordinate );
    }

    // finish the polygonal shape
    if( last_coordinate.y != 0 )
        polyPoints.push_back( wxPoint( last_coordinate.x, 0 ) );

    switch( PolyShapeType )
    {
    case 0:     // shape from file
    case 2:     // shape from file, mirrored (the mirror is already done)
        break;

    case 1:     // Symmetric shape: add the symmetric (mirrored) shape
        for( int ndx = polyPoints.size() - 1; ndx >= 0; --ndx )
        {
            wxPoint pt = polyPoints[ndx];
            pt.y = -pt.y;   // mirror about X axis
            polyPoints.push_back( pt );
        }
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
        DisplayError( this, _( "No pad for this footprint" ) );
        return;
    }

    next_pad = pad->Next();

    if( next_pad == NULL )
    {
        DisplayError( this, _( "Only one pad for this footprint" ) );
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
