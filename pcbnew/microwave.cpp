/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <confirm.h>
#include <trigo.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <richio.h>
#include <filter_reader.h>
#include <base_units.h>
#include <validators.h>
#include <dialog_text_entry.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <microwave/microwave_inductor.h>
#include <pcbnew.h>

static std::vector< wxRealPoint > PolyEdges;
static double  ShapeScaleX, ShapeScaleY;
static wxSize  ShapeSize;
static int     PolyShapeType;


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

        module->Add( pad, ADD_INSERT );

        int tw = GetDesignSettings().GetCurrentTrackWidth();
        pad->SetSize( wxSize( tw, tw ) );

        pad->SetPosition( module->GetPosition() );
        pad->SetShape( PAD_SHAPE_RECT );
        pad->SetAttribute( PAD_ATTRIB_SMD );
        pad->SetLayerSet( F_Cu );

        Line.Printf( wxT( "%d" ), pad_num );
        pad->SetName( Line );
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
        msg = _( "Gap Size:" );
        cmp_name = "muwave_gap";
        text_size = gap_size;
        break;

    case 1:
        msg = _( "Stub Size:" );
        cmp_name  = "muwave_stub";
        text_size = gap_size;
        pad_count = 2;
        break;

    case 2:
        msg = _( "Arc Stub Radius Value:" );
        cmp_name  = "muwave_arcstub";
        pad_count = 1;
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    wxString value = StringFromValue( GetUserUnits(), gap_size );
    WX_TEXT_ENTRY_DIALOG dlg( this, msg, _( "Create microwave module" ), value );

    if( dlg.ShowModal() != wxID_OK )
        return NULL; // cancelled by user

    value    = dlg.GetValue();
    gap_size = ValueFromString( GetUserUnits(), value );

    bool abort = false;

    if( shape_type == 2 )
    {
        double            fcoeff = 10.0, fval;
        msg.Printf( wxT( "%3.1f" ), angle / fcoeff );
        WX_TEXT_ENTRY_DIALOG angledlg( this, _( "Angle in degrees:" ),
                                       _( "Create microwave module" ), msg );

        if( angledlg.ShowModal() != wxID_OK )
            return NULL; // cancelled by user

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
        return NULL;

    module = CreateMuWaveBaseFootprint( cmp_name, text_size, pad_count );
    auto it = module->Pads().begin();
    pad = *it;

    switch( shape_type )
    {
    case 0:     //Gap :
        oX = -( gap_size + pad->GetSize().x ) / 2;
        pad->SetX0( oX );

        pad->SetX( pad->GetPos0().x + pad->GetPosition().x );

        pad = *( it + 1 );

        pad->SetX0( oX + gap_size + pad->GetSize().x );
        pad->SetX( pad->GetPos0().x + pad->GetPosition().x );
        break;

    case 1:     //Stub :
        pad->SetName( wxT( "1" ) );
        pad = *( it + 1 );
        pad->SetY0( -( gap_size + pad->GetSize().y ) / 2 );
        pad->SetSize( wxSize( pad->GetSize().x, gap_size ) );
        pad->SetY( pad->GetPos0().y + pad->GetPosition().y );
        break;

    case 2:     // Arc Stub created by a polygonal approach:
    {
        pad->SetShape( PAD_SHAPE_CUSTOM );
        pad->SetAnchorPadShape( PAD_SHAPE_RECT );

        int numPoints = (angle / 50) + 3;     // Note: angles are in 0.1 degrees
        std::vector<wxPoint> polyPoints;
        polyPoints.reserve( numPoints );

        polyPoints.emplace_back( wxPoint( 0, 0 ) );

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

        pad->AddPrimitive( polyPoints, 0 );  // add a polygonal basic shape
    }
        break;

    default:
        break;
    }

    module->CalculateBoundingBox();
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

    wxString shapelist[] = { _( "Normal" ), _( "Symmetrical" ), _( "Mirrored" ) };

    m_ShapeOptionCtrl = new wxRadioBox( this, -1, _( "Shape Option" ),
                                        wxDefaultPosition, wxDefaultSize, 3,
                                        shapelist, 1,
                                        wxRA_SPECIFY_COLS );
    LeftBoxSizer->Add( m_ShapeOptionCtrl, 0, wxGROW | wxALL, 5 );

    m_SizeCtrl = new EDA_SIZE_CTRL( this, _( "Size" ), ShapeSize, parent->GetUserUnits(),
                                    LeftBoxSizer );

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
    wxString mask = wxFileSelectorDefaultWildcardStr;

    wxString FullFileName = EDA_FILE_SELECTOR( _( "Read descr shape file" ), lastpath,
                                               FullFileName, wxEmptyString, mask, this,
                                               wxFD_OPEN, true );
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

    MWAVE_POLYGONAL_SHAPE_DLG dlg( this, wxDefaultPosition );

    int ret = dlg.ShowModal();

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

    auto it = module->Pads().begin();

    pad1 = *it;
    pad1->SetX0( offset.x );
    pad1->SetX( pad1->GetPos0().x );

    pad2 = *( ++it );
    pad2->SetX0( offset.x + ShapeSize.x );
    pad2->SetX( pad2->GetPos0().x );

    // Add a polygonal edge (corners will be added later) on copper layer
    edge = new EDGE_MODULE( module );
    edge->SetShape( S_POLYGON );
    edge->SetLayer( F_Cu );

    module->Add( edge, ADD_INSERT );

    // Get the corner buffer of the polygonal edge
    std::vector<wxPoint> polyPoints;
    polyPoints.reserve( PolyEdges.size() + 2 );

    // Init start point coord:
    polyPoints.emplace_back( wxPoint( offset.x, 0 ) );

    wxPoint last_coordinate;

    for( wxRealPoint& pt: PolyEdges )  // Copy points
    {
        last_coordinate.x = KiROUND( pt.x * ShapeScaleX );
        last_coordinate.y = -KiROUND( pt.y * ShapeScaleY );
        last_coordinate += offset;
        polyPoints.push_back( last_coordinate );
    }

    // finish the polygonal shape
    if( last_coordinate.y != 0 )
        polyPoints.emplace_back( wxPoint( last_coordinate.x, 0 ) );

    switch( PolyShapeType )
    {
    case 0:     // shape from file
    case 2:     // shape from file, mirrored (the mirror is already done)
        break;

    case 1:     // Symmetric shape: add the symmetric (mirrored) shape
        for( int ndx = (int) polyPoints.size() - 1; ndx >= 0; --ndx )
        {
            wxPoint pt = polyPoints[ndx];
            pt.y = -pt.y;   // mirror about X axis
            polyPoints.push_back( pt );
        }
        break;
    }

    edge->SetPolyPoints( polyPoints );
    // Set the polygon outline thickness to 0, only the polygonal shape is filled
    // without extra thickness
    edge->SetWidth( 0 );
    PolyEdges.clear();
    module->CalculateBoundingBox();
    OnModify();
    return module;
}
