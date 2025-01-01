/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <widgets/unit_binder.h>
#include <pcb_edit_frame.h>
#include <dialog_shim.h>
#include <locale_io.h>
#include <filter_reader.h>
#include <dialogs/dialog_text_entry.h>
#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <microwave/microwave_tool.h>
#include <pad.h>
#include <math/util.h>      // for KiROUND

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/filedlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

static std::vector< wxRealPoint > g_PolyEdges;
static double  g_ShapeScaleX, g_ShapeScaleY;
static wxSize  g_ShapeSize;
static int     g_PolyShapeType;


enum id_mw_cmd {
    ID_READ_SHAPE_FILE = 1000
};


class MWAVE_POLYGONAL_SHAPE_DLG : public DIALOG_SHIM
{
public:
    MWAVE_POLYGONAL_SHAPE_DLG( PCB_EDIT_FRAME* parent, const wxPoint& pos );

    ~MWAVE_POLYGONAL_SHAPE_DLG()
    {
        delete m_sizeX;
        delete m_sizeY;
    };

    bool TransferDataFromWindow() override;

private:
    void OnCancelClick( wxCommandEvent& event );

    /**
     * Read a description shape file.
     *
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

    PCB_EDIT_FRAME*  m_frame;
    wxRadioBox*      m_shapeOptionCtrl;
    UNIT_BINDER*     m_sizeX;
    UNIT_BINDER*     m_sizeY;
};


BEGIN_EVENT_TABLE( MWAVE_POLYGONAL_SHAPE_DLG, DIALOG_SHIM )
    EVT_BUTTON( wxID_CANCEL, MWAVE_POLYGONAL_SHAPE_DLG::OnCancelClick )
    EVT_BUTTON( ID_READ_SHAPE_FILE, MWAVE_POLYGONAL_SHAPE_DLG::ReadDataShapeDescr )
END_EVENT_TABLE()


MWAVE_POLYGONAL_SHAPE_DLG::MWAVE_POLYGONAL_SHAPE_DLG( PCB_EDIT_FRAME* parent,
                                                      const wxPoint&  framepos ) :
    DIALOG_SHIM( parent, -1, _( "Complex Shape" ), framepos, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
    m_sizeX(),
    m_sizeY()
{
    m_frame = parent;

    g_PolyEdges.clear();

    wxBoxSizer* mainBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainBoxSizer );

    // Controls

    wxBoxSizer* topBoxSizer  = new wxBoxSizer( wxHORIZONTAL );
    mainBoxSizer->Add( topBoxSizer, 1, wxGROW | wxALL, 5 );

    wxString shapelist[] = { _( "Normal" ), _( "Symmetrical" ), _( "Mirrored" ) };

    m_shapeOptionCtrl = new wxRadioBox( this, -1, _( "Shape" ),
                                        wxDefaultPosition, wxDefaultSize, 3,
                                        shapelist, 1,
                                        wxRA_SPECIFY_COLS );
    topBoxSizer->Add( m_shapeOptionCtrl, 1, wxGROW | wxALL, 5 );

    auto sizeSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _( "Size" ) ),
                                           wxVERTICAL );
    wxBoxSizer* xSizer = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* ySizer = new wxBoxSizer( wxHORIZONTAL );

    topBoxSizer->Add( sizeSizer, 1, wxGROW | wxALL, 5 );
    sizeSizer->Add( xSizer, 0, 0, 5 );
    sizeSizer->Add( ySizer, 0, 0, 5 );

    wxStaticText* xLabel = new wxStaticText( this, -1, _( "X:" ) );
    wxTextCtrl*   xCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString );
    wxStaticText* xUnits = new wxStaticText( this, -1, _( "units" ) );

    xSizer->Add( xLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    xSizer->Add( xCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );
    xSizer->Add( xUnits, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    m_sizeX = new UNIT_BINDER( m_frame, xLabel, xCtrl, xUnits );

    wxStaticText* yLabel = new wxStaticText( this, -1, _( "Y:" ) );
    wxTextCtrl*   yCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString );
    wxStaticText* yUnits = new wxStaticText( this, -1, _( "units" ) );

    ySizer->Add( yLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    ySizer->Add( yCtrl, 1, wxALIGN_CENTER_VERTICAL, 5 );
    ySizer->Add( yUnits, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    m_sizeY = new UNIT_BINDER( m_frame, yLabel, yCtrl, yUnits );

    // Buttons

    wxBoxSizer* buttonsBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    mainBoxSizer->Add( buttonsBoxSizer, 0, wxALL, 5 );

    wxButton* btn = new wxButton( this, ID_READ_SHAPE_FILE, _( "Read Shape Description File..." ) );
    buttonsBoxSizer->Add( btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10 );
    buttonsBoxSizer->AddStretchSpacer();

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    buttonsBoxSizer->Add( sdbSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    wxButton* sdbSizerOK = new wxButton( this, wxID_OK );
   	sdbSizer->AddButton( sdbSizerOK );
   	wxButton* sdbSizerCancel = new wxButton( this, wxID_CANCEL );
   	sdbSizer->AddButton( sdbSizerCancel );
   	sdbSizer->Realize();

    GetSizer()->SetSizeHints( this );
}


void MWAVE_POLYGONAL_SHAPE_DLG::OnCancelClick( wxCommandEvent& event )
{
    g_PolyEdges.clear();
    event.Skip();
}


bool MWAVE_POLYGONAL_SHAPE_DLG::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    g_ShapeSize.x = m_sizeX->GetValue();
    g_ShapeSize.y = m_sizeY->GetValue();
    g_PolyShapeType = m_shapeOptionCtrl->GetSelection();

    return true;
}


void MWAVE_POLYGONAL_SHAPE_DLG::ReadDataShapeDescr( wxCommandEvent& event )
{
    static wxString s_lastpath;       // To remember the last open path during a session
    wxString fullFileName;
    wxString mask = wxFileSelectorDefaultWildcardStr;

    fullFileName = wxFileSelector( _( "Shape Description File" ), s_lastpath,
                                   fullFileName, wxEmptyString, mask, wxFD_OPEN, this );

    if( fullFileName.IsEmpty() )
        return;

    wxFileName fn( fullFileName );
    s_lastpath = fn.GetPath();
    g_PolyEdges.clear();

    FILE* File = wxFopen( fullFileName, wxT( "rt" ) );

    if( File == nullptr )
    {
        DisplayError( this, _( "File not found" ) );
        return;
    }

    double unitconv = pcbIUScale.IU_PER_MM;
    g_ShapeScaleX = g_ShapeScaleY = 1.0;

    FILE_LINE_READER fileReader( File, fullFileName );
    FILTER_READER reader( fileReader );

    LOCALE_IO   toggle;

    while( reader.ReadLine() )
    {
        char* Line = reader.Line();
        char* param1 = strtok( Line, " =\n\r" );
        char* param2 = strtok( nullptr, " \t\n\r" );

        if( strncasecmp( param1, "Unit", 4 ) == 0 )
        {
            if( strncasecmp( param2, "inch", 4 ) == 0 )
                unitconv = pcbIUScale.IU_PER_MILS*1000;

            if( strncasecmp( param2, "mm", 2 ) == 0 )
                unitconv = pcbIUScale.IU_PER_MM;
        }

        if( strncasecmp( param1, "$ENDCOORD", 8 ) == 0 )
            break;

        if( strncasecmp( param1, "$COORD", 6 ) == 0 )
        {
            while( reader.ReadLine() )
            {
                Line = reader.Line();
                param1 = strtok( Line, " \t\n\r" );
                param2 = strtok( nullptr, " \t\n\r" );

                if( strncasecmp( param1, "$ENDCOORD", 8 ) == 0 )
                    break;

                wxRealPoint coord( atof( param1 ), atof( param2 ) );
                g_PolyEdges.push_back( coord );
            }
        }

        if( strncasecmp( Line, "XScale", 6 ) == 0 )
            g_ShapeScaleX = atof( param2 );

        if( strncasecmp( Line, "YScale", 6 ) == 0 )
            g_ShapeScaleY = atof( param2 );
    }

    g_ShapeScaleX *= unitconv;
    g_ShapeScaleY *= unitconv;

    m_sizeX->SetValue( (int) g_ShapeScaleX );
    m_sizeY->SetValue( (int) g_ShapeScaleY );
}


FOOTPRINT* MICROWAVE_TOOL::createPolygonShape()
{
    PAD*       pad1;
    PAD*       pad2;
    FOOTPRINT* footprint;
    wxString   cmp_name;
    int        pad_count = 2;
    PCB_SHAPE* shape;

    PCB_EDIT_FRAME& editFrame  = *getEditFrame<PCB_EDIT_FRAME>();

    MWAVE_POLYGONAL_SHAPE_DLG dlg( &editFrame, wxDefaultPosition );

    int ret = dlg.ShowModal();

    if( ret != wxID_OK )
    {
        g_PolyEdges.clear();
        return nullptr;
    }

    if( g_PolyShapeType == 2 )  // mirrored
        g_ShapeScaleY = -g_ShapeScaleY;

    g_ShapeSize.x = KiROUND( g_ShapeScaleX );
    g_ShapeSize.y = KiROUND( g_ShapeScaleY );

    if(( g_ShapeSize.x ) == 0 || ( g_ShapeSize.y == 0 ) )
    {
        editFrame.ShowInfoBarError( _( "Shape has a null size." ) );
        return nullptr;
    }

    if( g_PolyEdges.size() == 0 )
    {
        editFrame.ShowInfoBarError( _( "Shape has no points." ) );
        return nullptr;
    }

    cmp_name = wxT( "muwave_polygon" );

    // Create a footprint with 2 pads, orientation = 0, pos 0
    footprint = createBaseFootprint( cmp_name, 0, pad_count );

    // We try to place the footprint anchor to the middle of the shape len
    VECTOR2I offset;
    offset.x = -g_ShapeSize.x / 2;

    auto it = footprint->Pads().begin();

    pad1 = *it;
    pad1->SetX( offset.x );

    pad2 = *( ++it );
    pad2->SetX( offset.x + g_ShapeSize.x );

    // Add a polygonal edge (corners will be added later) on copper layer
    shape = new PCB_SHAPE( footprint, SHAPE_T::POLY );
    shape->SetFilled( true );
    shape->SetLayer( F_Cu );

    footprint->Add( shape, ADD_MODE::INSERT );

    // Get the corner buffer of the polygonal edge
    std::vector<VECTOR2I> polyPoints;
    polyPoints.reserve( g_PolyEdges.size() + 2 );

    // Init start point coord:
    polyPoints.emplace_back( VECTOR2I( offset.x, 0 ) );

    VECTOR2I last_coordinate;

    for( wxRealPoint& pt: g_PolyEdges )  // Copy points
    {
        last_coordinate.x = KiROUND( pt.x * g_ShapeScaleX );
        last_coordinate.y = -KiROUND( pt.y * g_ShapeScaleY );
        last_coordinate += offset;
        polyPoints.push_back( last_coordinate );
    }

    // finish the polygonal shape
    if( last_coordinate.y != 0 )
        polyPoints.emplace_back( VECTOR2I( last_coordinate.x, 0 ) );

    switch( g_PolyShapeType )
    {
    case 0:     // shape from file
    case 2:     // shape from file, mirrored (the mirror is already done)
        break;

    case 1:     // Symmetric shape: add the symmetric (mirrored) shape
        for( int ndx = (int) polyPoints.size() - 1; ndx >= 0; --ndx )
        {
            VECTOR2I pt = polyPoints[ndx];
            pt.y = -pt.y;   // mirror about X axis
            polyPoints.push_back( pt );
        }

        break;
    }

    shape->SetPolyPoints( polyPoints );

    // Set the polygon outline thickness to 0, only the polygonal shape is filled
    // without extra thickness.
    shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
    g_PolyEdges.clear();

    editFrame.OnModify();
    return footprint;
}
