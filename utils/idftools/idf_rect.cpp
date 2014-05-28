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

void writeLeaded( FILE* fp, double width, double length, double height,
                  double wireDia, double pitch, bool inch );

void writeLeadless( FILE* fp, double width, double length,
                    double height, double chamfer, bool inch );

int main( int argc, char **argv )
{
    // IDF implicitly requires the C locale
    setlocale( LC_ALL, "C" );

    if( argc == 1 )
    {
        cout << "idfrect: This program generates an outline for a rectangular component.\n";
        cout << "         The component may have a single lead (axial) or a chamfer on the\n";
        cout << "         upper left corner.\n";
        cout << "Input:\n";
        cout << "        Unit: mm, in (millimeters or inches)\n";
        cout << "        Width:\n";
        cout << "        Length:\n";
        cout << "        Height:\n";
        cout << "        Chamfer: length of the 45 deg. chamfer\n";
        cout << "        *  Leaded: Y,N (lead is always to the right)\n";
        cout << "        ** Wire diameter\n";
        cout << "        ** Pitch\n";
        cout << "        File name (must end in *.idf)\n\n";
        cout << "        NOTES:\n";
        cout << "            *   only required if chamfer = 0\n\n";
        cout << "            **  only required for leaded components\n\n";
    }

    bool inch  = false; // default mm
    double width = 0.0;
    double length = 0.0;
    double height = 0.0;
    double wireDia = 0.0;
    double pitch = 0.0;
    double chamfer = 0.0;
    bool leaded = false;
    bool ok = false;

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

    ok = false;
    while( !ok )
    {
        cout << "* Width: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> width;
        if( !tstr.fail() && width >= 0.001 )
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
        cout << "* Height: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> height;
        if( !tstr.fail() && height >= 0.001 )
            ok = true;
    }

    ok = false;
    while( !ok )
    {
        cout << "* Chamfer (0 for none): ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> chamfer;
        if( !tstr.fail() && chamfer >= 0.0 )
        {
            if( chamfer > width / 3.0 || chamfer > length / 3.0 )
                cout << "* WARNING: chamfer must be <= MIN( width, length )/3\n";
            else
                ok = true;
        }
    }

    if( chamfer < 1e-6 )
    {
        ok = false;
        while( !ok )
        {
            cout << "* Leaded: Y, N: ";

            line.clear();
            std::getline( cin, line );

            if( !line.compare( "Y" ) || !line.compare( "y" ) )
            {
                leaded = true;
                ok = true;
            }
            else if( !line.compare( "N" ) || !line.compare( "n" ) )
            {
                leaded = false;
                ok = true;
            }
        }
    }

    ok = false;
    while( leaded && !ok )
    {
        cout << "* Wire dia.: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> wireDia;
        if( !tstr.fail() && wireDia >= 0.001 )
        {
            if( wireDia >= length )
                cout << "* WARNING: wire diameter must be < length\n";
            else
                ok = true;
        }
    }

    ok = false;
    while( leaded && !ok )
    {
        cout << "* Pitch: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> pitch;
        if( !tstr.fail() && pitch >= 0.001 )
        {
            if( pitch <= ( length + wireDia ) / 2.0 )
                cout << "* WARNING: pitch must be > (length + wireDia)/2\n";
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
    }
    else
    {
        fprintf( fp, "# rectangular outline%s\n", leaded ? ", leaded" : "" );
        fprintf( fp, "# file: \"%s\"\n", line.c_str() );

        if( inch )
        {
            width *= 1000.0;
            length *= 1000.0;
            height *= 1000.0;
            wireDia *= 1000.0;
            pitch *= 1000.0;
            chamfer *= 1000.0;

            fprintf( fp, "# width: %d THOU\n", (int) width );
            fprintf( fp, "# length: %d THOU\n", (int) length );
            fprintf( fp, "# height: %d THOU\n", (int) height );

            if( leaded )
            {
                fprintf( fp, "# wire dia: %d THOU\n", (int) wireDia );
                fprintf( fp, "# pitch: %d THOU\n", (int) pitch );
            }
            else
            {
                fprintf( fp, "# chamfer: %d THOU\n", (int) chamfer );
            }

            fprintf( fp, ".ELECTRICAL\n" );
            fprintf( fp, "\"RECT%sIN\" \"W%d_L%d_H%d", leaded ? "L" : "",
                     (int) width, (int) length, (int) height );

            if( leaded )
                fprintf( fp, "_D%d_P%d\" ", (int) wireDia, (int) pitch );
            else
                fprintf( fp, "_C%d\" ", (int) chamfer );

            fprintf( fp, "THOU %d\n", (int) height );
        }
        else
        {
            fprintf( fp, "# width: %.3f mm\n", width );
            fprintf( fp, "# length: %.3f mm\n", length );
            fprintf( fp, "# height: %.3f mm\n", height );

            if( leaded )
            {
                fprintf( fp, "# wire dia: %.3f mm\n", wireDia );
                fprintf( fp, "# pitch: %.3f mm\n", pitch );
            }
            else
            {
                fprintf( fp, "# chamfer: %.3f mm\n", chamfer );
            }

            fprintf( fp, ".ELECTRICAL\n" );
            fprintf( fp, "\"RECT%sMM\" \"W%.3f_L%.3f_H%.3f_", leaded ? "L" : "",
                     width, length, height );

            if( leaded )
                fprintf( fp, "D%.3f_P%.3f\" ", wireDia, pitch );
            else
                fprintf( fp, "C%.3f\" ", chamfer );

            fprintf( fp, "MM %.3f\n", height );
        }

        if( leaded )
            writeLeaded( fp, width, length, height, wireDia, pitch, inch );
        else
            writeLeadless( fp, width, length, height, chamfer, inch );

        fprintf( fp, ".END_ELECTRICAL\n" );
        fclose( fp );
    }

    setlocale( LC_ALL, "" );
    return 0;
}


void writeLeaded( FILE* fp, double width, double length,
                  double height, double wireDia, double pitch, bool inch )
{
    if( inch )
    {
        int x1, x2, x3;
        int y1, y2;

        x1 = pitch / 2.0;
        x2 = width / 2.0 - x1;
        x3 = x2 - width;

        y1 = wireDia / 2.0;
        y2 = length / 2.0;

        fprintf( fp, "0 %d %d 0\n", x1, y1 );
        fprintf( fp, "0 %d %d 0\n", x2, y1 );
        fprintf( fp, "0 %d %d 0\n", x2, y2 );
        fprintf( fp, "0 %d %d 0\n", x3, y2 );
        fprintf( fp, "0 %d %d 0\n", x3, -y2 );
        fprintf( fp, "0 %d %d 0\n", x2, -y2 );
        fprintf( fp, "0 %d %d 0\n", x2, -y1 );
        fprintf( fp, "0 %d %d 0\n", x1, -y1 );
        fprintf( fp, "0 %d %d 180\n", x1, y1 );
    }
    else
    {
        double x1, x2, x3;
        double y1, y2;

        x1 = pitch / 2.0;
        x2 = width / 2.0 - x1;
        x3 = x2 - width;

        y1 = wireDia / 2.0;
        y2 = length / 2.0;

        fprintf( fp, "0 %.3f %.3f 0\n", x1, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", x2, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", x2, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x3, y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x3, -y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x2, -y2 );
        fprintf( fp, "0 %.3f %.3f 0\n", x2, -y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", x1, -y1 );
        fprintf( fp, "0 %.3f %.3f 180\n", x1, y1 );
    }

    return;
}

void writeLeadless( FILE* fp, double width, double length,
                    double height, double chamfer, bool inch )
{
    if( chamfer < 0.001 )
    {
        if( inch )
        {
            int x = width / 2.0;
            int y = length / 2.0;

            fprintf( fp, "0 %d %d 0\n", x, y );
            fprintf( fp, "0 %d %d 0\n", -x, y );
            fprintf( fp, "0 %d %d 0\n", -x, -y );
            fprintf( fp, "0 %d %d 0\n", x, -y );
            fprintf( fp, "0 %d %d 0\n", x, y );
        }
        else
        {
            double x = width / 2.0;
            double y = length / 2.0;

            fprintf( fp, "0 %.3f %.3f 0\n", x, y );
            fprintf( fp, "0 %.3f %.3f 0\n", -x, y );
            fprintf( fp, "0 %.3f %.3f 0\n", -x, -y );
            fprintf( fp, "0 %.3f %.3f 0\n", x, -y );
            fprintf( fp, "0 %.3f %.3f 0\n", x, y );
        }

        return;
    }

    if( inch )
    {
        int x = width / 2.0;
        int y = length / 2.0;
        int x1 = x - chamfer;
        int y1 = y - chamfer;

        fprintf( fp, "0 %d %d 0\n", x, y );
        fprintf( fp, "0 %d %d 0\n", -x1, y );
        fprintf( fp, "0 %d %d 0\n", -x, y1 );
        fprintf( fp, "0 %d %d 0\n", -x, -y );
        fprintf( fp, "0 %d %d 0\n", x, -y );
        fprintf( fp, "0 %d %d 0\n", x, y );
    }
    else
    {
        double x = width / 2.0;
        double y = length / 2.0;
        double x1 = x - chamfer;
        double y1 = y - chamfer;

        fprintf( fp, "0 %.3f %.3f 0\n", x, y );
        fprintf( fp, "0 %.3f %.3f 0\n", -x1, y );
        fprintf( fp, "0 %.3f %.3f 0\n", -x, y1 );
        fprintf( fp, "0 %.3f %.3f 0\n", -x, -y );
        fprintf( fp, "0 %.3f %.3f 0\n", x, -y );
        fprintf( fp, "0 %.3f %.3f 0\n", x, y );
    }

    return;
}
