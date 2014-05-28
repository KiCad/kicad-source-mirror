/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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
 *  This program creates an outline for a horizontal or vertically
 *  oriented axial or radial leaded cylinder with dimensions based
 *  on the user's input.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <list>
#include <utility>
#include <clocale>

using namespace std;

void make_vcyl( bool inch, bool axial, double dia, double length,
                double z, double wireDia );

void make_hcyl( bool inch, bool axial, double dia, double length,
                double z, double wireDia );

void writeAxialCyl( FILE* fp, bool inch, double dia, double length, double wireDia, double pitch );

void writeRadialCyl( FILE* fp, bool inch, double dia, double length, double wireDia,
                     double pitch, double lead );

int main( int argc, char **argv )
{
    // IDF implicitly requires the C locale
    setlocale( LC_ALL, "C" );

    if( argc == 1 )
    {
        cout << "idfcyl: This program generates an outline for a cylindrical component.\n";
        cout << "        The cylinder may be horizontal or vertical.\n";
        cout << "        A horizontal cylinder may have wires at one or both ends.\n";
        cout << "        A vertical cylinder may have at most one wire which may be\n";
        cout << "        placed on the left or right side.\n\n";
        cout << "Input:\n";
        cout << "        Unit: mm, in (millimeters or inches)\n";
        cout << "        Orientation: V (vertical)\n";
        cout << "        Lead type: X, R (axial, radial)\n";
        cout << "        Diameter of body\n";
        cout << "        Length of body\n";
        cout << "        Board offset\n";
        cout << "        *   Wire diameter\n";
        cout << "        *   Pitch\n";
        cout << "        **  Wire side: L, R (left, right)\n";
        cout << "        *** Lead length\n";
        cout << "        File name (must end in *.idf)\n\n";
        cout << "        NOTES:\n";
        cout << "            *   only required for horizontal orientation or\n";
        cout << "                vertical orientation with axial leads\n\n";
        cout << "            **  only required for vertical orientation with axial leads\n\n";
        cout << "            *** only required for horizontal orientation with radial leads\n\n";
    }

    char orientation = '\0';
    bool inch  = false; // default mm
    double dia = 0.0;
    double length = 0.0;
    double extraZ = 0.0;
    double wireDia = 0.0;
    bool axial = false;

    stringstream tstr;
    string line;

    line.clear();
    while( line.compare( "mm" ) && line.compare( "in" ) )
    {
        cout << "* Units (mm,in): ";
        line.clear();
        std::getline( cin, line );
    }

    if( line.compare( "mm" ) )
        inch = true;

    line.clear();
    while( line.compare( "H" ) && line.compare( "h" )
        && line.compare( "V" ) && line.compare( "v" ) )
    {
        cout << "* Orientation (H,V): ";
        line.clear();
        std::getline( cin, line );
    }

    if( line.compare( "H" ) && line.compare( "h" ) )
        orientation = 'v';
    else
        orientation = 'h';

    bool ok = false;

    while( !ok )
    {
        cout << "* Axial or Radial (X,R): ";

        line.clear();
        std::getline( cin, line );

        if( !line.compare( "x" ) || !line.compare( "X" ) )
        {
            axial = true;
            ok = true;
        }
        else if( !line.compare( "r" ) || !line.compare( "R" ) )
        {
            axial = false;
            ok = true;
        }
    }

    // cylinder dimensions
    ok = false;
    while( !ok )
    {
        cout << "* Diameter: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> dia;
        if( !tstr.fail() && dia > 0.0 )
            ok = true;
    }

    ok = false;
    while( !ok )
    {
        cout << "* Length: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> length;
        if( !tstr.fail() && length > 0.0 )
            ok = true;
    }

    ok = false;
    while( !ok )
    {
        cout << "* Board offset: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> extraZ;
        if( !tstr.fail() && extraZ >= 0.0 )
            ok = true;
    }

    ok = false;
    while( ( axial || orientation == 'h' ) && !ok )
    {
        cout << "* Wire diameter: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> wireDia;
        if( !tstr.fail() && wireDia > 0.0 )
        {
            if( wireDia < dia )
                ok = true;
            else
                cout << "* WARNING: wire diameter must be < cylinder diameter\n";
        }
    }

    switch( orientation )
    {
        case 'v':
            make_vcyl( inch, axial, dia, length, extraZ, wireDia );
            break;
        case 'h':
            make_hcyl( inch, axial, dia, length, extraZ, wireDia );
            break;
        default:
            break;
    }

    setlocale( LC_ALL, "" );
    return 0;
}


void make_vcyl( bool inch, bool axial, double dia, double length,
                double z, double wireDia )
{
    bool ok = false;
    bool left = false;
    stringstream tstr;
    string line;

    double pitch = 0.0;

    while( axial && !ok )
    {
        cout << "* Pitch: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> pitch;
        if( !tstr.fail() && pitch > 0.0 )
        {
            if( (pitch - wireDia) <= (dia / 2.0) )
            {
                cout << "* WARNING: Pitch must be > dia/2 + wireDia\n";
            }
            else
            {
                ok = true;
            }
        }
    }

    ok = false;
    while( axial && !ok )
    {
        cout << "* Pin side (L,R): ";

        line.clear();
        std::getline( cin, line );

        if( !line.compare( "l" ) || !line.compare( "L" ) )
        {
            left = true;
            ok   = true;
        }
        else if( !line.compare( "r" ) || !line.compare( "R" ) )
            ok = true;
    }

    line.clear();
    while( line.empty() || line.find( ".idf" ) == string::npos )
    {
        cout << "* File name (*.idf): ";

        line.clear();
        std::getline( cin, line );
    }

    FILE* fp = fopen( line.c_str(), "w" );

    if( !fp )
    {
        cerr << "Could not open output file: " << line << "\n";
        return;
    }

    fprintf( fp, "# cylindrical outline, vertical, " );

    if( !axial )
        fprintf( fp, "radial leads\n" );
    else
        fprintf( fp, "axial lead on %s\n", left ? "left" : "right" );

    fprintf( fp, "# file: \"%s\"\n", line.c_str() );

    if( inch )
    {
        fprintf( fp, "# dia: %d THOU\n", (int) (dia * 1000) );
        fprintf( fp, "# length: %d THOU\n", (int) (length * 1000) );
        fprintf( fp, "# board offset: %d THOU\n", (int) (z * 1000) );

        if( axial )
        {
            fprintf( fp, "# wire dia: %d THOU\n", (int) (wireDia * 1000) );
            fprintf( fp, "# pitch: %d THOU\n", (int) (pitch * 1000) );
        }
    }
    else
    {
        fprintf( fp, "# dia: %.3f mm\n", dia );
        fprintf( fp, "# length: %.3f mm\n", length );
        fprintf( fp, "# board offset: %.3f mm\n", z );

        if( axial )
        {
            fprintf( fp, "# wire dia: %.3f mm\n", wireDia );
            fprintf( fp, "# pitch: %.3f mm\n", pitch );
        }
    }

    fprintf( fp, ".ELECTRICAL\n" );

    if( !axial )
    {
        fprintf( fp, "\"CYLV_%s_RAD\" \"D%.3f_H%.3f_Z%.3f\" ", inch ? "IN" : "MM",
            dia, length, z );
    }
    else
    {
        fprintf( fp, "\"CYLV_%s_AX%s\" \"D%.3f_H%.3f_Z%.3f_WD%.3f_P%.3f\" ", inch ? "IN" : "MM",
                 left ? "L" : "R", dia, length, z, wireDia, pitch );
    }

    if( inch )
        fprintf( fp, "THOU %d\n", (int) ((length + z) * 1000) );
    else
        fprintf( fp, "MM %.3f\n", length + z );

    if( !axial )
    {
        fprintf( fp, "0 0 0 0\n" );

        if( inch )
            fprintf( fp, "0 %d 0 360\n", (int) (dia * 1000) );
        else
            fprintf( fp, "0 %.3f 0 360\n", dia );

        fprintf( fp, ".END_ELECTRICAL\n" );
        fclose( fp );
        return;
    }

    double px[4], py[4];

    // points are:
    // [0] = upper point on cylinder perimeter
    // [1] = lower point on cylinder perimeter
    // [2] = point beneath wire center
    // [3] = point above wire center

    if( inch )
    {
        dia     *= 1000.0;
        pitch   *= 1000.0;
        wireDia *= 1000.0;
    }

    double ang = asin( wireDia / dia );
    px[0] = dia * cos( ang ) / 2.0 - pitch / 2.0;
    px[1] = px[0];
    px[2] = pitch / 2.0;
    px[3] = px[2];

    py[0] = wireDia / 2.0;
    py[1] = -py[0];
    py[2] = py[1];
    py[3] = py[0];

    char li = '0';

    double fullAng = 360.0;

    if( left )
    {
        li = '1';
        fullAng = -360.0;
        for( int i = 0; i < 4; ++i ) px[i] = -px[i];
    }


    if( inch )
    {
        fprintf( fp, "%c %d %d 0\n", li, (int) px[0], (int) py[0] );
        fprintf( fp, "%c %d %d %.3f\n", li, (int) px[1], (int) py[1],
                 fullAng * ( 1 - ang / M_PI ) );
        fprintf( fp, "%c %d %d 0\n", li, (int) px[2], (int) py[2] );
        fprintf( fp, "%c %d %d %s\n", li, (int) px[3], (int) py[3],
                 left ? "-180" : "180" );
        fprintf( fp, "%c %d %d 0\n", li, (int) px[0], (int) py[0] );
    }
    else
    {
        fprintf( fp, "%c %.3f %.3f 0\n", li, px[0], py[0] );
        fprintf( fp, "%c %.3f %.3f %.3f\n", li, px[1], py[1], fullAng * ( 1 - ang / M_PI ) );
        fprintf( fp, "%c %.3f %.3f 0\n", li, px[2], py[2] );
        fprintf( fp, "%c %.3f %.3f %s\n", li, px[3], py[3],
                 left ? "-180" : "180" );
        fprintf( fp, "%c %.3f %.3f 0\n", li, px[0], py[0] );
    }

    fprintf( fp, ".END_ELECTRICAL\n" );
    fclose( fp );
    return;
}


void make_hcyl( bool inch, bool axial, double dia, double length,
                double z, double wireDia )
{
    bool ok = false;
    stringstream tstr;
    string line;

    double pitch = 0.0;
    double lead  = 0.0; // lead length for radial leads

    ok = false;
    while( !ok )
    {
        if( axial )
            cout << "* Axial pitch: ";
        else
            cout << "* Radial pitch: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> pitch;
        if( !tstr.fail() && pitch > 0.0 )
        {
            if( axial )
            {
                if( (pitch - wireDia) <= length )
                {
                    cout << "* WARNING: Axial pitch must be > length + wireDia\n";
                }
                else
                {
                    ok = true;
                }
            }
            else
            {
                if( (pitch + wireDia) >= dia )
                {
                    cout << "* WARNING: Radial pitch must be < dia - wireDia\n";
                }
                else if( pitch <= wireDia )
                {
                    cout << "* WARNING: Radial pitch must be > wireDia\n";
                }
                else
                {
                    ok = true;
                }
            }
        }
    }

    ok = false;
    while( !axial && !ok )
    {
        cout << "* Lead length: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> lead;
        if( !tstr.fail() && lead > 0.0 )
        {
            if( lead < wireDia )
                cout << "* WARNING: lead length must be >= wireDia\n";
            else
                ok = true;
        }
    }

    line.clear();
    while( line.empty() || line.find( ".idf" ) == string::npos )
    {
        cout << "* File name (*.idf): ";

        line.clear();
        std::getline( cin, line );
    }

    FILE* fp = fopen( line.c_str(), "w" );

    if( !fp )
    {
        cerr << "Could not open output file: " << line << "\n";
        return;
    }

    fprintf( fp, "# cylindrical outline, horiz., " );

    fprintf( fp, "%s pins\n", axial ? "axial" : "radial" );

    fprintf( fp, "# file: \"%s\"\n", line.c_str() );

    if( inch )
    {
        fprintf( fp, "# dia: %d THOU\n", (int) (dia * 1000) );
        fprintf( fp, "# length: %d THOU\n", (int) (length * 1000) );
        fprintf( fp, "# extra height: %d THOU\n", (int) (z * 1000) );
        fprintf( fp, "# wire dia: %d THOU\n", (int) (wireDia * 1000) );
        fprintf( fp, "# pitch: %d THOU\n", (int) (pitch * 1000) );
        if( !axial )
            fprintf( fp, "# lead: %d THOU\n", (int) (lead * 1000) );
    }
    else
    {
        fprintf( fp, "# dia: %.3f mm\n", dia );
        fprintf( fp, "# length: %.3f mm\n", length );
        fprintf( fp, "# extra height: %.3f mm\n", z );
        fprintf( fp, "# wire dia: %.3f mm\n", wireDia );
        fprintf( fp, "# pitch: %.3f mm\n", pitch );
        if( !axial )
            fprintf( fp, "# lead: %.3f mm\n", lead );
    }

    fprintf( fp, ".ELECTRICAL\n" );

    if( axial )
    {
        fprintf( fp, "\"CYLH_%s_AXI\" \"D%.3f_H%.3f_Z%.3f_WD%.3f_P%.3f\" ",
                 inch ? "IN" : "MM", dia, length, z, wireDia, pitch );
    }
    else
    {
        fprintf( fp, "\"CYLH_%s_RAD\" \"D%.3f_H%.3f_Z%.3f_WD%.3f_P%.3f_L%.3f\" ",
                 inch ? "IN" : "MM", dia, length, z, wireDia, pitch, lead );
    }

    if( inch )
    {
        fprintf( fp, "THOU %d\n", (int) ((dia + z) * 1000) );
        dia *= 1000.0;
        length *= 1000.0;
        wireDia *= 1000.0;
        pitch *= 1000.0;
        if( !axial )
            lead *= 1000.0;
    }
    else
    {
        fprintf( fp, "MM %.3f\n", dia + z );
    }

    if( axial )
        writeAxialCyl( fp, inch, dia, length, wireDia, pitch );
    else
        writeRadialCyl( fp, inch, dia, length, wireDia, pitch, lead );

    fprintf( fp, ".END_ELECTRICAL\n" );
    fclose( fp );
    return;
    return;
}

void writeAxialCyl( FILE* fp, bool inch, double dia, double length,
                   double wireDia, double pitch )
{
    double x1, y1;
    double x2, y2;

    x1 = -length / 2.0;
    x2 = -pitch / 2.0;
    y1 = dia / 2.0;
    y2 = wireDia / 2.0;

    if( inch )
    {
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) y1 );
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) x2, (int) y2 );
        fprintf( fp, "0 %d %d 180\n", (int) x2, (int) -y2 );
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) -y2 );
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) -y1 );
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) -y1 );
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) -y2 );
        fprintf( fp, "0 %d %d 0\n", (int) -x2, (int) -y2 );
        fprintf( fp, "0 %d %d 180\n", (int) -x2, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) y1 );
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) y1 );
    }
    else
    {
        fprintf( fp, "0 %.3f %.3f 0\n", x1, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x2, y2 );
        fprintf( fp, "0 %.3f %.3f 180\n", x2, -y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, -y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, -y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, -y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, -y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x2, -y2 );
        fprintf( fp, "0 %.3f %.3f 180\n", -x2, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, y1 );
    }

    return;
}

void writeRadialCyl( FILE* fp, bool inch, double dia, double length,
                    double wireDia, double pitch, double lead )
{
    double x1, y1;
    double x2, y2;
    double x3;

    // center is between the mounting holes
    // which are on a horizontal line
    y1 = lead + length;
    y2 = lead;
    x1 = dia / 2.0;
    x2 = ( pitch + wireDia ) /2.0;
    x3 = x2 - wireDia;

    if( inch )
    {
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) y1 );
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) -x2, (int) y2 );
        fprintf( fp, "0 %d 0 0\n", (int) -x2 );
        fprintf( fp, "0 %d 0 180\n", (int) -x3 );
        fprintf( fp, "0 %d %d 0\n", (int) -x3, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) x3, (int) y2 );
        fprintf( fp, "0 %d 0 0\n", (int) x3 );
        fprintf( fp, "0 %d 0 180\n", (int) x2 );
        fprintf( fp, "0 %d %d 0\n", (int) x2, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) y2 );
        fprintf( fp, "0 %d %d 0\n", (int) x1, (int) y1 );
        fprintf( fp, "0 %d %d 0\n", (int) -x1, (int) y1 );
    }
    else
    {
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x2, y2 );
        fprintf( fp, "0 %.3f 0 0\n", -x2 );
        fprintf( fp, "0 %.3f 0 180\n", -x3 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x3, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x3, y2 );
        fprintf( fp, "0 %.3f 0 0\n", x3 );
        fprintf( fp, "0 %.3f 0 180\n", x2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x2, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, y1 );
    }

    return;
}
