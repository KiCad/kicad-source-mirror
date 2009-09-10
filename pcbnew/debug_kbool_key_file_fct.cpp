/* file debug_kbool_key_file_fct.cpp
 */
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "zones.h"
#include "PolyLine.h"

#include "debug_kbool_key_file_fct.h"

#ifdef CREATE_KBOOL_KEY_FILES

static FILE*       kdebugFile;
static const char * sDate_Time = "2009-09-07  15:59:24";


void CreateKeyFile()
{
    kdebugFile = fopen( KEYFILE_FILENAME, "wt" );
    if( kdebugFile )
    {
        fprintf( kdebugFile, "# KEY file for GDS-II postprocessing tool\n" );
        fprintf( kdebugFile, "#  File = %s\n", KEYFILE_FILENAME );
        fprintf( kdebugFile, "# ====================================================================\n");
        fprintf( kdebugFile, "\nHEADER 5; # version\n");
        fprintf( kdebugFile, "BGNLIB;\n");

        fprintf( kdebugFile, "LASTMOD {%s}; # last modification time\n",sDate_Time );
        fprintf( kdebugFile, "LASTACC {%s}; # last access time\n",sDate_Time );

        fprintf( kdebugFile, "LIBNAME trial;\n" );
        fprintf( kdebugFile, "UNITS;\n# Units are in 0.0001 inch\n" );
        fprintf( kdebugFile, "USERUNITS 1; PHYSUNITS 0.0001;\n\n" );
    }
    else
    {
        wxMessageBox( wxT( "CreateKeyFile() cannot create output file" ) );
    }
}


void CloseKeyFile()
{
    if( kdebugFile )
    {
        fprintf( kdebugFile, "\nENDLIB;\n" );
        fclose( kdebugFile );
    }
}


const char* sCurrEntityName = NULL;
static int s_count;

void OpenEntity( const char* aName )
{
    if( kdebugFile )
    {
        fprintf( kdebugFile, "\nBGNSTR; # Begin of structure\n" );
        fprintf( kdebugFile, "CREATION {%s}; # creation time\n",sDate_Time);
        fprintf( kdebugFile, "LASTMOD  {%s}; # last modification time\n",sDate_Time);
        fprintf( kdebugFile, "STRNAME %s;\n", aName );
    }
    sCurrEntityName = aName;
    s_count = 0;
}


void CloseEntity()
{
    if( kdebugFile )
        fprintf( kdebugFile, "\nENDSTR %s;\n", sCurrEntityName );
}


void StartPolygon(int aCornersCount, int aLayer)
{
    fprintf( kdebugFile, "\nBOUNDARY; LAYER %d;  DATATYPE 0;\n", aLayer );
    fprintf( kdebugFile, "   XY %d;\n", aCornersCount );
    s_count = 0;
}

void EndElement()
{
    if ( s_count == 1 )
        fprintf( kdebugFile, "\n");
    fprintf( kdebugFile, "\nENDEL;\n" );
    s_count = 0;
}

void CopyPolygonsFromFilledPolysListToKeyFile( ZONE_CONTAINER* aZone, int aLayer )
{
    if( !kdebugFile )
        return;

    unsigned corners_count = aZone->m_FilledPolysList.size();
    int      count = 0;
    unsigned ic    = 0;
    CPolyPt* corner;

    while( ic < corners_count )
    {
        // Count corners:
        count = 0;
        for( unsigned ii = ic; ii < corners_count; ii++ )
        {
            corner = &aZone->m_FilledPolysList[ii];
            count++;
            if( corner->end_contour )
                break;
        }

        // write corners:
        StartPolygon( count+1, aLayer );
        corner = &aZone->m_FilledPolysList[ic];
        int startpointX = corner->x;
        int startpointY = corner->y;
        for( ; ic < corners_count; ic++ )
        {
            corner = &aZone->m_FilledPolysList[ic];
            AddPointXY( corner->x, corner->y );
            if( corner->end_contour )
            {
                ic++;
                break;
            }
        }
        // Close polygon:
        AddPointXY( startpointX, startpointY );
        EndElement();
    }
}

void AddPointXY( int aXcoord, int aYcoord)
{
    if ( s_count >= 2 )
    {
        s_count = 0;
        fprintf( kdebugFile, "\n");
    }
    SetLocaleTo_C_standard();
    fprintf( kdebugFile, "   X %d; Y %d;", aXcoord, aYcoord );
    SetLocaleTo_Default( );
    s_count ++;
}

#endif
