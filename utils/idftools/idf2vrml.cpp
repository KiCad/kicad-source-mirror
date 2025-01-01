/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 Cirilo Bernardo
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

/*
 *  This program takes an IDF base name, loads the board outline
 *  and component outine files, and creates a single VRML file.
 *  The VRML file can be used to visually verify the IDF files
 *  before sending them to a mechanical designer. The output scale
 *  is 10:1; this scale was chosen because VRML was originally
 *  intended to describe large virtual worlds and rounding errors
 *  would be more likely if we used a 1:1 scale.
 */

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/log.h>
#include <wx/string.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cerrno>
#include <list>
#include <utility>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <boost/ptr_container/ptr_map.hpp>

#include <wx_filename.h>

#include "idf_helpers.h"
#include "idf_common.h"
#include "idf_parser.h"
#include "streamwrapper.h"
#include "vrml_layer.h"

#ifndef MIN_ANG
#define MIN_ANG 0.01
#endif

class IDF2VRML : public wxAppConsole
{
public:
    virtual bool OnInit() override;
    virtual int OnRun() override;
    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;

private:
    double m_ScaleFactor;
    bool   m_Compact;
    bool   m_NoOutlineSubs;
    wxString m_filename;
};

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_OPTION, "f", NULL, "input file name",
        wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
    { wxCMD_LINE_OPTION, "s", NULL, "scale factor",
        wxCMD_LINE_VAL_DOUBLE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "k", NULL, "produce KiCad-friendly VRML output; default is compact VRML",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "d", NULL, "suppress substitution of default outlines",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "z", NULL, "suppress rendering of zero-height outlines",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "m", NULL, "print object mapping to stdout for debugging",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "h", NULL, "display this message",
        wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
};


wxIMPLEMENT_APP_CONSOLE( IDF2VRML );

bool nozeroheights;
bool showObjectMapping;

bool IDF2VRML::OnInit()
{
    m_ScaleFactor = 1.0;
    m_Compact = true;
    m_NoOutlineSubs = false;
    nozeroheights = false;
    showObjectMapping = false;

    if( !wxAppConsole::OnInit() )
        return false;

    return true;
}


void IDF2VRML::OnInitCmdLine( wxCmdLineParser& parser )
{
    parser.SetDesc( cmdLineDesc );
    parser.SetSwitchChars( wxT( "-" ) );
    return;
}


bool IDF2VRML::OnCmdLineParsed( wxCmdLineParser& parser )
{
    if( parser.Found( wxT( "k" ) ) )
        m_Compact = false;

    double scale;

    if( parser.Found( wxT( "s" ), &scale ) )
        m_ScaleFactor = scale;

    wxString fname;

    if( parser.Found( wxT( "f" ), &fname ) )
        m_filename = fname;

    if( parser.Found( wxT( "d" ) ) )
        m_NoOutlineSubs = true;

    if( parser.Found( wxT( "z" ) ) )
        nozeroheights = true;

    if( parser.Found( wxT( "m" ) ) )
        showObjectMapping = true;

    return true;
}

using namespace boost;

// define colors
struct VRML_COLOR
{
    double diff[3];
    double emis[3];
    double spec[3];
    double ambi;
    double tran;
    double shin;
};

struct VRML_IDS
{
    int colorIndex;
    std::string objectName;
    bool used;
    bool bottom;
    double dX, dY, dZ, dA;

    VRML_IDS()
    {
        colorIndex = 0;
        used = false;
        bottom = false;
        dX = 0.0;
        dY = 0.0;
        dZ = 0.0;
        dA = 0.0;
    }
};

#define NCOLORS 7
VRML_COLOR colors[NCOLORS] =
{
    { { 0, 0.82, 0.247 },       { 0, 0, 0 },    { 0, 0.82, 0.247 },         0.9, 0, 0.1 },
    { { 1, 0, 0 },              { 1, 0, 0 },    { 1, 0, 0 },                0.9, 0, 0.1 },
    { { 0.659, 0, 0.463 },      { 0, 0, 0 },    { 0.659, 0, 0.463 },        0.9, 0, 0.1 },
    { { 0.659, 0.294, 0 },      { 0, 0, 0 },    { 0.659, 0.294, 0 },        0.9, 0, 0.1 },
    { { 0, 0.918, 0.659 },      { 0, 0, 0 },    { 0, 0.918, 0.659 },        0.9, 0, 0.1 },
    { { 0.808, 0.733, 0.071 },  { 0, 0, 0 },    { 0.808, 0.733 , 0.071 },   0.9, 0, 0.1 },
    { { 0.102, 1, 0.984 },      { 0, 0, 0 },    { 0.102, 1, 0.984 },        0.9, 0, 0.1 }
};

bool WriteHeader( IDF3_BOARD& board, std::ostream& file );
bool MakeBoard( IDF3_BOARD& board, std::ostream& file );
bool MakeComponents( IDF3_BOARD& board, std::ostream& file, bool compact );
bool MakeOtherOutlines( IDF3_BOARD& board, std::ostream& file );
bool PopulateVRML( VRML_LAYER& model, const std::list< IDF_OUTLINE* >* items, bool bottom,
                   double scale, double dX = 0.0, double dY = 0.0, double angle = 0.0 );
bool AddSegment( VRML_LAYER& model, IDF_SEGMENT* seg, int icont, int iseg );
bool WriteTriangles( std::ostream& file, VRML_IDS* vID, VRML_LAYER* layer, bool plane,
                     bool top, double top_z, double bottom_z, int precision, bool compact );
inline void TransformPoint( IDF_SEGMENT& seg, double frac, bool bottom,
                            double dX, double dY, double angle );
VRML_IDS* GetColor( boost::ptr_map<const std::string, VRML_IDS>& cmap,
                      int& index, const std::string& uid );


int IDF2VRML::OnRun()
{
    // Essential inputs:
    // 1. IDF file
    // 2. Output scale: internal IDF units are mm, so 1 = 1mm per VRML unit,
    //      0.1 = 1cm per VRML unit, 0.01 = 1m per VRML unit,
    //      1/25.4 = 1in per VRML unit, 1/2.54 = 0.1in per VRML unit (KiCad model)
    // 3. KiCad-friendly output (do not reuse features via DEF+USE)
    //      Render each component to VRML; if the user wants
    //      a KiCad friendly output then we must avoid DEF+USE;
    //      otherwise we employ DEF+USE to minimize file size

    if( m_ScaleFactor < 0.001 || m_ScaleFactor > 10.0 )
    {
        wxLogMessage( wxT( "scale factor out of range (%d); range is 0.001 to 10.0" ),
                      m_ScaleFactor);
        return -1;
    }

    IDF3_BOARD pcb( IDF3::CAD_ELEC );

    wxLogMessage( wxT( "Reading file: '%s'" ), m_filename );

    if( !pcb.ReadFile( m_filename, m_NoOutlineSubs ) )
    {
        wxLogMessage( wxT( "Failed to read IDF data: %s" ), pcb.GetError() );
        return -1;
    }

    // set the scale and output precision ( scale 1 == precision 5)
    pcb.SetUserScale( m_ScaleFactor );

    if( m_ScaleFactor < 0.01 )
        pcb.SetUserPrecision( 8 );
    else if( m_ScaleFactor < 0.1 )
        pcb.SetUserPrecision( 7 );
    else if( m_ScaleFactor < 1.0 )
        pcb.SetUserPrecision( 6 );
    else if( m_ScaleFactor < 10.0 )
        pcb.SetUserPrecision( 5 );
    else
        pcb.SetUserPrecision( 4 );

    // Create the VRML file and write the header
    wxFileName fname( m_filename );
    fname.SetExt( wxT( "wrl" ) );
    fname.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
    wxLogMessage( wxT( "Writing file: '%s'" ), fname.GetFullName() );

    OPEN_OSTREAM( ofile, fname.GetFullPath().ToUTF8() );

    if( ofile.fail() )
        wxLogMessage( wxT( "Could not open file: '%s'" ), fname.GetFullName() );

    ofile.imbue( std::locale( "C" ) );
    ofile << std::fixed; // do not use exponents in VRML output
    WriteHeader( pcb, ofile );

    // STEP 1: Render the PCB alone
    MakeBoard( pcb, ofile );

    // STEP 2: Render the components
    MakeComponents( pcb, ofile, m_Compact );

    // STEP 3: Render the OTHER outlines
    MakeOtherOutlines( pcb, ofile );

    ofile << "]\n}\n";
    CLOSE_STREAM( ofile );

    return 0;
}


bool WriteHeader( IDF3_BOARD& board, std::ostream& file )
{
    std::string bname = board.GetBoardName();

    if( bname.empty() )
    {
        bname = "BoardWithNoName";
    }
    else
    {
        std::string::iterator ss = bname.begin();
        std::string::iterator se = bname.end();

        while( ss != se )
        {
            if( *ss == '/' || *ss == ' ' || *ss == ':' )
                *ss = '_';

            ++ss;
        }
    }

    file << "#VRML V2.0 utf8\n\n";
    file << "WorldInfo {\n";
    file << "    title \"" << bname << "\"\n}\n\n";
    file << "Transform {\n";
    file << "children [\n";

    return !file.fail();
}


bool MakeBoard( IDF3_BOARD& board, std::ostream& file )
{
    VRML_LAYER vpcb;

    if( board.GetBoardOutlinesSize() < 1 )
    {
        wxLogMessage( wxT( "Cannot proceed; no board outline in IDF object" ) );
        return false;
    }

    double scale = board.GetUserScale();

    // set the arc parameters according to output scale
    int tI;
    double tMin, tMax;
    vpcb.GetArcParams( tI, tMin, tMax );
    vpcb.SetArcParams( tI, tMin * scale, tMax * scale );

    if( !PopulateVRML( vpcb, board.GetBoardOutline()->GetOutlines(), false, board.GetUserScale() ) )
    {
        return false;
    }

    vpcb.EnsureWinding( 0, false );

    int nvcont = vpcb.GetNContours() - 1;

    while( nvcont > 0 )
        vpcb.EnsureWinding( nvcont--, true );

    // Add the drill holes
    const std::list<IDF_DRILL_DATA*>* drills = &board.GetBoardDrills();

    std::list<IDF_DRILL_DATA*>::const_iterator sd = drills->begin();
    std::list<IDF_DRILL_DATA*>::const_iterator ed = drills->end();

    while( sd != ed )
    {
        vpcb.AddCircle( (*sd)->GetDrillXPos() * scale, (*sd)->GetDrillYPos() * scale,
                        (*sd)->GetDrillDia() * scale / 2.0, true );
        ++sd;
    }

    std::map< std::string, IDF3_COMPONENT* >*const comp = board.GetComponents();
    std::map< std::string, IDF3_COMPONENT* >::const_iterator sc = comp->begin();
    std::map< std::string, IDF3_COMPONENT* >::const_iterator ec = comp->end();

    while( sc != ec )
    {
        drills = sc->second->GetDrills();
        sd = drills->begin();
        ed = drills->end();

        while( sd != ed )
        {
            vpcb.AddCircle( (*sd)->GetDrillXPos() * scale, (*sd)->GetDrillYPos() * scale,
                            (*sd)->GetDrillDia() * scale / 2.0, true );
            ++sd;
        }

        ++sc;
    }

    // tesselate and write out
    vpcb.Tesselate( NULL );

    double thick = board.GetBoardThickness() / 2.0 * scale;

    VRML_IDS tvid;
    tvid.colorIndex = 0;

    WriteTriangles( file, &tvid, &vpcb, false, false,
                    thick, -thick, board.GetUserPrecision(), false );

    return true;
}

bool PopulateVRML( VRML_LAYER& model, const std::list< IDF_OUTLINE* >* items, bool bottom, double scale,
                   double dX, double dY, double angle )
{
    // empty outlines are not unusual so we fail quietly
    if( items->size() < 1 )
        return false;

    int nvcont = 0;
    int iseg   = 0;

    std::list< IDF_OUTLINE* >::const_iterator scont = items->begin();
    std::list< IDF_OUTLINE* >::const_iterator econt = items->end();
    std::list<IDF_SEGMENT*>::iterator sseg;
    std::list<IDF_SEGMENT*>::iterator eseg;

    IDF_SEGMENT lseg;

    while( scont != econt )
    {
        nvcont = model.NewContour();

        if( nvcont < 0 )
        {
            wxLogMessage( wxT( "Cannot create an outline" ) );
            return false;
        }

        if( (*scont)->size() < 1 )
        {
            wxLogMessage( wxT( "Invalid contour: no vertices" ) );
            return false;
        }

        sseg = (*scont)->begin();
        eseg = (*scont)->end();

        iseg = 0;
        while( sseg != eseg )
        {
            lseg = **sseg;
            TransformPoint( lseg, scale, bottom, dX, dY, angle );

            if( !AddSegment( model, &lseg, nvcont, iseg ) )
                return false;

            ++iseg;
            ++sseg;
        }

        ++scont;
    }

    return true;
}


bool AddSegment( VRML_LAYER& model, IDF_SEGMENT* seg, int icont, int iseg )
{
    // note: in all cases we must add all but the last point in the segment
    // to avoid redundant points

    if( seg->angle != 0.0 )
    {
        if( seg->IsCircle() )
        {
            if( iseg != 0 )
            {
                wxLogMessage( wxT( "Adding a circle to an existing vertex list" ) );
                return false;
            }

            return model.AppendCircle( seg->center.x, seg->center.y, seg->radius, icont );
        }
        else
        {
            return model.AppendArc( seg->center.x, seg->center.y, seg->radius,
                                    seg->offsetAngle, seg->angle, icont );
        }
    }

    if( !model.AddVertex( icont, seg->startPoint.x, seg->startPoint.y ) )
        return false;

    return true;
}


bool WriteTriangles( std::ostream& file, VRML_IDS* vID, VRML_LAYER* layer, bool plane,
                     bool top, double top_z, double bottom_z, int precision, bool compact )
{
    if( vID == NULL || layer == NULL )
        return false;

    file << "Transform {\n";

    if( compact && !vID->objectName.empty() )
    {
        file << "translation " << std::setprecision( precision ) << vID->dX;
        file << " " << vID->dY << " ";

        if( vID->bottom )
        {
            file << -vID->dZ << "\n";

            double tx, ty;

            // calculate the rotation axis and angle
            tx = cos( M_PI2 - vID->dA / 2.0 );
            ty = sin( M_PI2 - vID->dA / 2.0 );

            file << "rotation " << std::setprecision( precision );
            file << tx << " " << ty << " 0 ";
            file << std::setprecision(5) << M_PI << "\n";
        }
        else
        {
            file << vID->dZ << "\n";
            file << "rotation 0 0 1 " << std::setprecision(5) << vID->dA << "\n";
        }

        file << "children [\n";

        if( vID->used )
        {
            file << "USE " << vID->objectName << "\n";
            file << "]\n";
            file << "}\n";
            return true;
        }

        file << "DEF " << vID->objectName << " Transform {\n";

        if( !plane && top_z <= bottom_z )
        {
            // the height specification is faulty; make the component
            // a bright red to highlight it
            vID->colorIndex = 1;
            // we don't know the scale, but 5 units is huge in most situations
            top_z = bottom_z + 5.0;
        }

    }

    VRML_COLOR* color = &colors[vID->colorIndex];

    vID->used = true;

    file << "children [\n";
    file << "Group {\n";
    file << "children [\n";
    file << "Shape {\n";
    file << "appearance Appearance {\n";
    file << "material Material {\n";

    // material definition
    file << "diffuseColor " << std::setprecision(3) << color->diff[0] << " ";
    file << color->diff[1] << " " << color->diff[2] << "\n";
    file << "specularColor " << color->spec[0] << " " << color->spec[1];
    file << " " << color->spec[2] << "\n";
    file << "emissiveColor " << color->emis[0] << " " << color->emis[1];
    file << " " << color->emis[2] << "\n";
    file << "ambientIntensity " << color->ambi << "\n";
    file << "transparency " << color->tran << "\n";
    file << "shininess " << color->shin << "\n";

    file << "}\n";
    file << "}\n";
    file << "geometry IndexedFaceSet {\n";
    file << "solid TRUE\n";
    file << "coord Coordinate {\n";
    file << "point [\n";

    // Coordinates (vertices)
    if( plane )
    {
        if( !layer->WriteVertices( top_z, file, precision ) )
        {
            wxLogMessage( wxT( "Errors writing planar vertices to %s\n%s" ),
                          vID->objectName, layer->GetError() );
        }
    }
    else
    {
        if( !layer->Write3DVertices( top_z, bottom_z, file, precision ) )
        {
            wxLogMessage( wxT( "Errors writing 3D vertices to %s\n%s" ),
                          vID->objectName, layer->GetError() );
        }
    }

    file << "\n";

    file << "]\n";
    file << "}\n";
    file << "coordIndex [\n";

    // Indices
    if( plane )
        layer->WriteIndices( top, file );
    else
        layer->Write3DIndices( file );

    file << "\n";
    file << "]\n";
    file << "}\n";
    file << "}\n";
    file << "]\n";
    file << "}\n";
    file << "]\n";
    file << "}\n";

    if( compact && !vID->objectName.empty() )
    {
        file << "]\n";
        file << "}\n";
    }

    return !file.fail();
}

inline void TransformPoint( IDF_SEGMENT& seg, double frac, bool bottom,
                            double dX, double dY, double angle )
{
    dX *= frac;
    dY *= frac;

    if( bottom )
    {
        // mirror points on the Y axis
        seg.startPoint.x = -seg.startPoint.x;
        seg.endPoint.x = -seg.endPoint.x;
        seg.center.x = -seg.center.x;
        angle = -angle;
    }

    seg.startPoint.x *= frac;
    seg.startPoint.y *= frac;
    seg.endPoint.x *= frac;
    seg.endPoint.y *= frac;
    seg.center.x *= frac;
    seg.center.y *= frac;

    double tsin = 0.0;
    double tcos = 0.0;

    if( angle > MIN_ANG || angle < -MIN_ANG )
    {
        double ta = angle * M_PI / 180.0;
        double tx, ty;

        tsin = sin( ta );
        tcos = cos( ta );

        tx = seg.startPoint.x * tcos - seg.startPoint.y * tsin;
        ty = seg.startPoint.x * tsin + seg.startPoint.y * tcos;
        seg.startPoint.x = tx;
        seg.startPoint.y = ty;

        tx = seg.endPoint.x * tcos - seg.endPoint.y * tsin;
        ty = seg.endPoint.x * tsin + seg.endPoint.y * tcos;
        seg.endPoint.x = tx;
        seg.endPoint.y = ty;

        if( seg.angle != 0 )
        {
            tx = seg.center.x * tcos - seg.center.y * tsin;
            ty = seg.center.x * tsin + seg.center.y * tcos;
            seg.center.x = tx;
            seg.center.y = ty;
        }
    }

    seg.startPoint.x += dX;
    seg.startPoint.y += dY;
    seg.endPoint.x += dX;
    seg.endPoint.y += dY;
    seg.center.x += dX;
    seg.center.y += dY;

    if( seg.angle != 0 )
    {
        seg.radius *= frac;

        if( bottom )
        {
            if( !seg.IsCircle() )
            {
                seg.angle = -seg.angle;
                if( seg.offsetAngle > 0.0 )
                    seg.offsetAngle = 180 - seg.offsetAngle;
                else
                    seg.offsetAngle = -seg.offsetAngle - 180;
            }
        }

        if( angle > MIN_ANG || angle < -MIN_ANG )
            seg.offsetAngle += angle;
    }

    return;
}

bool MakeComponents( IDF3_BOARD& board, std::ostream& file, bool compact )
{
    int cidx = 2;   // color index; start at 2 since 0,1 are special (board, NOGEOM_NOPART)

    VRML_LAYER vpcb;

    double scale = board.GetUserScale();
    double thick = board.GetBoardThickness() / 2.0;

    // set the arc parameters according to output scale
    int tI;
    double tMin, tMax;
    vpcb.GetArcParams( tI, tMin, tMax );
    vpcb.SetArcParams( tI, tMin * scale, tMax * scale );

    // Add the component outlines
    const std::map< std::string, IDF3_COMPONENT* >*const comp = board.GetComponents();
    std::map< std::string, IDF3_COMPONENT* >::const_iterator sc = comp->begin();
    std::map< std::string, IDF3_COMPONENT* >::const_iterator ec = comp->end();

    std::list< IDF3_COMP_OUTLINE_DATA* >::const_iterator so;
    std::list< IDF3_COMP_OUTLINE_DATA* >::const_iterator eo;

    double vX, vY, vA;
    double tX, tY, tZ, tA;
    double top, bot;
    bool   bottom;
    IDF3::IDF_LAYER lyr;

    boost::ptr_map< const std::string, VRML_IDS> cmap;  // map colors by outline UID
    VRML_IDS* vcp;
    IDF3_COMP_OUTLINE* pout;

    while( sc != ec )
    {
        sc->second->GetPosition( vX, vY, vA, lyr );

        if( lyr == IDF3::LYR_BOTTOM )
            bottom = true;
        else
            bottom = false;

        so = sc->second->GetOutlinesData()->begin();
        eo = sc->second->GetOutlinesData()->end();

        while( so != eo )
        {
            if( (*so)->GetOutline()->GetThickness() < 0.00000001 && nozeroheights )
            {
                vpcb.Clear();
                ++so;
                continue;
            }

            (*so)->GetOffsets( tX, tY, tZ, tA );
            tX += vX;
            tY += vY;
            tA += vA;

            if( ( pout = (IDF3_COMP_OUTLINE*)((*so)->GetOutline()) ) != nullptr )
            {
                vcp = GetColor( cmap, cidx, pout->GetUID() );
            }
            else
            {
                vpcb.Clear();
                ++so;
                continue;
            }

            if( !compact )
            {
                if( !PopulateVRML( vpcb, (*so)->GetOutline()->GetOutlines(), bottom,
                    board.GetUserScale(), tX, tY, tA ) )
                {
                    return false;
                }
            }
            else
            {
                if( !vcp->used && !PopulateVRML( vpcb, (*so)->GetOutline()->GetOutlines(), false,
                    board.GetUserScale() ) )
                {
                    return false;
                }

                vcp->dX = tX * scale;
                vcp->dY = tY * scale;
                vcp->dZ = tZ * scale;
                vcp->dA = tA * M_PI / 180.0;
            }

            if( !compact || !vcp->used )
            {
                vpcb.EnsureWinding( 0, false );

                int nvcont = vpcb.GetNContours() - 1;

                while( nvcont > 0 )
                    vpcb.EnsureWinding( nvcont--, true );

                vpcb.Tesselate( NULL );
            }

            if( !compact )
            {
                if( bottom )
                {
                    top = -thick - tZ;
                    bot = (top - (*so)->GetOutline()->GetThickness() ) * scale;
                    top *= scale;
                }
                else
                {
                    bot = thick + tZ;
                    top = (bot + (*so)->GetOutline()->GetThickness() ) * scale;
                    bot *= scale;
                }
            }
            else
            {
                bot = thick;
                top = (bot + (*so)->GetOutline()->GetThickness() ) * scale;
                bot *= scale;
            }

            vcp = GetColor( cmap, cidx, ((IDF3_COMP_OUTLINE*)((*so)->GetOutline()))->GetUID() );
            vcp->bottom = bottom;

            // note: this can happen because IDF allows some negative heights/thicknesses
            if( bot > top )
                std::swap( bot, top );

            WriteTriangles( file, vcp, &vpcb, false,
                            false, top, bot, board.GetUserPrecision(), compact );

            vpcb.Clear();
            ++so;
        }

        ++sc;
    }

    return true;
}


VRML_IDS* GetColor( boost::ptr_map<const std::string, VRML_IDS>& cmap, int& index, const std::string& uid )
{
    static int refnum = 0;

    if( index < 2 )
        index = 2;   // 0 and 1 are special (BOARD, UID=NOGEOM_NOPART)

    boost::ptr_map<const std::string, VRML_IDS>::iterator cit = cmap.find( uid );

    if( cit == cmap.end() )
    {
        VRML_IDS* id = new VRML_IDS;

        if( !uid.compare( "NOGEOM_NOPART" ) )
            id->colorIndex = 1;
        else
            id->colorIndex = index++;

        std::ostringstream ostr;
        ostr << "OBJECTn" << refnum++;
        id->objectName = ostr.str();

        if( showObjectMapping )
            wxLogMessage( wxT( "* %s = '%s'" ), ostr.str(), uid );

        cmap.insert( uid, id );

        if( index >= NCOLORS )
            index = 2;

        return id;
    }

    return cit->second;
}


bool MakeOtherOutlines( IDF3_BOARD& board, std::ostream& file )
{
    int cidx = 2;   // color index; start at 2 since 0,1 are special (board, NOGEOM_NOPART)

    VRML_LAYER vpcb;

    double scale = board.GetUserScale();
    double thick = board.GetBoardThickness() / 2.0;

    // set the arc parameters according to output scale
    int tI;
    double tMin, tMax;
    vpcb.GetArcParams( tI, tMin, tMax );
    vpcb.SetArcParams( tI, tMin * scale, tMax * scale );

    // Add the component outlines
    const std::map< std::string, OTHER_OUTLINE* >*const comp = board.GetOtherOutlines();
    std::map< std::string, OTHER_OUTLINE* >::const_iterator sc = comp->begin();
    std::map< std::string, OTHER_OUTLINE* >::const_iterator ec = comp->end();

    double top, bot;
    bool   bottom;
    int nvcont;

    boost::ptr_map< const std::string, VRML_IDS> cmap;  // map colors by outline UID
    VRML_IDS* vcp;
    OTHER_OUTLINE* pout;

    while( sc != ec )
    {
        pout = sc->second;

        if( pout->GetThickness() < 0.00000001 && nozeroheights )
        {
            vpcb.Clear();
            ++sc;
            continue;
        }

        vcp = GetColor( cmap, cidx, pout->GetOutlineIdentifier() );

        if( !PopulateVRML( vpcb, pout->GetOutlines(), false,
            board.GetUserScale(), 0, 0, 0 ) )
        {
            return false;
        }

        vpcb.EnsureWinding( 0, false );

        nvcont = vpcb.GetNContours() - 1;

        while( nvcont > 0 )
            vpcb.EnsureWinding( nvcont--, true );

        vpcb.Tesselate( NULL );

        if( pout->GetSide() == IDF3::LYR_BOTTOM )
            bottom = true;
        else
            bottom = false;

        if( bottom )
        {
            top = -thick;
            bot = ( top - pout->GetThickness() ) * scale;
            top *= scale;
        }
        else
        {
            bot = thick;
            top = (bot + pout->GetThickness() ) * scale;
            bot *= scale;
        }

        // note: this can happen because IDF allows some negative heights/thicknesses
        if( bot > top )
            std::swap( bot, top );

        vcp->bottom = bottom;
        WriteTriangles( file, vcp, &vpcb, false,
                        false, top, bot, board.GetUserPrecision(), false );

        vpcb.Clear();
        ++sc;
    }

    return true;
}
