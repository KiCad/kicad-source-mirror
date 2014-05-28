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

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <dxf2idf.h>

using namespace std;

int main( int argc, char **argv )
{
    list< string > comments;
    string line;
    stringstream tstr;

    string dname;           // DXF filename
    string gname;           // Geometry Name
    string pname;           // Part Name
    double height;          // extrusion height
    bool   inch = false;    // true = inches, false = mm
    bool   ok;

    if( argc == 1 )
    {
        // no arguments; print out usage information
        cout << "dxf2idf: this program takes line, arc, and circle segments\n";
        cout << "         from a DXF file and creates an IDF component outline file.\n\n";
        cout << "Input:\n";
        cout << "         DXF filename: the input file, must end in '.dxf'\n";
        cout << "         Units: mm, in (millimeters or inches)\n";
        cout << "         Geometry Name: string, as per IDF version 3.0 specification\n";
        cout << "         Part Name: as per IDF version 3.0 specification of Part Number\n";
        cout << "         Height: extruded height of the outline\n";
        cout << "         Comments: all non-empty lines are comments to be added to\n";
        cout << "                   the IDF file. An empty line signifies the end of\n";
        cout << "                   the comment block.\n";
        cout << "         File name: output filename, must end in '.idf'\n\n";
    }

    line.clear();
    while( line.empty() || line.find( ".dxf" ) == string::npos )
    {
        cout << "* DXF filename: ";

        line.clear();
        std::getline( cin, line );
    }
    dname = line;

    line.clear();
    while( line.compare( "mm" ) && line.compare( "in" )
        && line.compare( "MM" ) && line.compare( "IN" ) )
    {
        cout << "* Units (mm,in): ";
        line.clear();
        std::getline( cin, line );
    }

    if( line.compare( "mm" ) && line.compare( "MM" ) )
        inch = true;

    line.clear();
    while( line.empty() )
    {
        cout << "* Geometry name: ";
        line.clear();
        std::getline( cin, line );

        if( line.find( "\"" ) != string::npos )
        {
            cerr << "[INFO] geometry name may not contain quotation marks\n";
            line.clear();
        }
    }
    gname = line;

    line.clear();
    while( line.empty() )
    {
        cout << "* Part name: ";
        line.clear();
        std::getline( cin, line );

        if( line.find( "\"" ) != string::npos )
        {
            cerr << "[INFO] part name may not contain quotation marks\n";
            line.clear();
        }
    }
    pname = line;

    ok = false;
    while( !ok )
    {
        cout << "* Height: ";

        line.clear();
        std::getline( cin, line );

        tstr.clear();
        tstr.str( line );

        tstr >> height;
        if( !tstr.fail() && height > 0.001 )
            ok = true;
    }

    cout << "* COMMENTS: any non-blank line is a comment;\n";
    cout << "            a blank line signifies the end of comments.\n";
    ok = false;
    while( !ok )
    {
        line.clear();
        std::getline( cin, line );

        if( line.empty() )
        {
            ok = true;
        }
        else
        {
            if( line[0] != '#' )
                line.insert( 0, "# " );

            comments.push_back( line );
        }
    }

    line.clear();
    while( line.empty() || line.find( ".idf" ) == string::npos )
    {
        cout << "* File name (*.idf): ";

        line.clear();
        std::getline( cin, line );
    }

    DXF2IDF dxf;

    dxf.ReadDxf( dname.c_str() );

    FILE* fp = fopen( line.c_str(), "w" );

    list< string >::const_iterator scom = comments.begin();
    list< string >::const_iterator ecom = comments.end();

    while( scom != ecom )
    {
        fprintf( fp, "%s\n", (*scom).c_str() );
        ++scom;
    }

    fprintf( fp, ".ELECTRICAL\n" );

    if( inch )
        fprintf( fp, "\"%s\" \"%s\" THOU %d\n", gname.c_str(),
                 pname.c_str(), (int) (height * 1000.0) );
    else
        fprintf( fp, "\"%s\" \"%s\" MM %.3f\n", gname.c_str(),
                 pname.c_str(), height );

    dxf.WriteOutline( fp, inch );

    fprintf( fp, ".END_ELECTRICAL\n" );

    return 0;
}