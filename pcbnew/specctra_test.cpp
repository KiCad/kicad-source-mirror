/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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


// unit test the specctra.cpp file.  You can use the beautifiers below to output
// exactly what you read in but beautified and without #comments.  This can
// then be used along with program 'diff' to test the parsing and formatting
// of every element.  You may have to run the first output back through to
// get two files that should match, the 2nd and 3rd outputs.



#include <cstdarg>
#include <cstdio>

#include <specctra.h>
#include <common.h>


using namespace DSN;


int main( int argc, char** argv )
{
//    wxString    filename( wxT("/tmp/fpcroute/Sample_1sided/demo_1sided.dsn") );
//    wxString    filename( wxT("/tmp/testdesigns/test.dsn") );
//    wxString    filename( wxT("/tmp/testdesigns/test.ses") );
    wxString    filename( wxT("/tmp/specctra_big.dsn") );

    SPECCTRA_DB     db;
    bool            failed = false;

    SetLocaleTo_C_standard( );    // Switch the locale to standard C

    if( argc == 2 )
    {
        filename = FROM_UTF8( argv[1] );
    }

    try
    {
//        db.LoadPCB( filename );
        db.LoadSESSION( filename );
    }
    catch( const IO_ERROR& ioe )
    {
        fprintf( stderr, "%s\n", TO_UTF8(ioe.errorText) );
        failed = true;
    }

    if( !failed )
        fprintf( stderr, "loaded OK\n" );

    // export what we read in, making this test program basically a beautifier
    // hose the beautified DSN file to stdout.  If an exception occurred,
    // we will be outputting only a portion of what we wanted to read in.
    db.SetFILE( stdout );

#if 0
    // export a PCB
    DSN::PCB* pcb = db.GetPCB();
    pcb->Format( &db, 0 );

#else
    // export a SESSION file.
    DSN::SESSION* ses = db.GetSESSION();
    ses->Format( &db, 0 );
#endif

    SetLocaleTo_Default( );      // revert to the current locale
}

//-----<dummy code>---------------------------------------------------

// a dummy to satisfy link of specctra_test without pulling in BOARD stuff.
int BOARD::GetCopperLayerCount() const
{
    return 0;
}

// a dummy to satisfy link of specctra_test without pulling in BOARD stuff.
wxString BOARD::GetLayerName( LAYER_NUM aLayer ) const
{
    return wxEmptyString;
}

//-----</dummy code>--------------------------------------------------

//EOF
