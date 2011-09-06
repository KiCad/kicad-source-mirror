/* file debug_kbool_key_file_fct.cpp
 */
#include <vector>

#include "fctsys.h"
#include "kicad_string.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "zones.h"
#include "PolyLine.h"

#include "debug_kbool_key_file_fct.h"

#if defined (CREATE_KBOOL_KEY_FILES) || (CREATE_KBOOL_KEY_FILES_FIRST_PASS)

// Helper class to handle a coordinate
struct kfcoord
{
    int x, y;
};

static FILE*       kdebugFile;
static char sDate_Time[256];
static vector <kfcoord> s_EntityCoordinates;

void CreateKeyFile()
{
    wxString   datetimestr;
    wxDateTime datetime = wxDateTime::Now();
    datetime.SetCountry( wxDateTime::Country_Default );
    datetimestr = datetime.FormatISODate( )
                + wxT("  ")
                + datetime.FormatISOTime( );
    strcpy(sDate_Time, TO_UTF8(datetimestr) );

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
        fprintf( kdebugFile, "UNITS;\n# Internal pcbnew units are in 0.0001 inch\n" );
        fprintf( kdebugFile, "USERUNITS 1; PHYSUNITS 1;\n\n" );
    }
    else
    {
        wxMessageBox( wxT( "CreateKeyFile() cannot create output file" ) );
    }

    s_EntityCoordinates.clear();
}


void CloseKeyFile()
{
    if( kdebugFile )
    {
        fprintf( kdebugFile, "\nENDLIB;\n" );
        fclose( kdebugFile );
    }
    s_EntityCoordinates.clear();
}


const char* sCurrEntityName = NULL;

void OpenKeyFileEntity( const char* aName )
{
    if( kdebugFile )
    {
        fprintf( kdebugFile, "\nBGNSTR; # Begin of structure\n" );
        fprintf( kdebugFile, "CREATION {%s}; # creation time\n",sDate_Time);
        fprintf( kdebugFile, "LASTMOD  {%s}; # last modification time\n",sDate_Time);
        fprintf( kdebugFile, "STRNAME %s;\n", aName );
    }
    sCurrEntityName = aName;
    s_EntityCoordinates.clear();
}


void CloseKeyFileEntity()
{
    if( kdebugFile )
        fprintf( kdebugFile, "\nENDSTR %s;\n", sCurrEntityName );
}

/* start a polygon entity in key file
*/
void StartKeyFilePolygon( int aLayer)
{
    s_EntityCoordinates.clear();
    fprintf( kdebugFile, "\nBOUNDARY; LAYER %d;  DATATYPE 0;\n", aLayer );
}

/* add a polygon corner to the current polygon entity in key file
*/
void AddKeyFilePointXY( int aXcoord, int aYcoord)
{
    kfcoord coord;
    coord.x = aXcoord;
    coord.y = aYcoord;
    s_EntityCoordinates.push_back( coord );
}


/* Close a polygon entity in key file
 * write the entire polygon data to the file
*/
void EndKeyFilePolygon()
{
    // Polygon must be closed: test for that and close it if needed
    if( s_EntityCoordinates.size() )
    {
        if( s_EntityCoordinates.back().x != s_EntityCoordinates[0].x
            || s_EntityCoordinates.back().y != s_EntityCoordinates[0].y )
            s_EntityCoordinates.push_back( s_EntityCoordinates[0] );
    }

    fprintf( kdebugFile, "   XY %d;\n", s_EntityCoordinates.size() );

    for( unsigned ii = 0; ii < s_EntityCoordinates.size(); ii ++ )
        fprintf( kdebugFile, "   X %d; Y %d;\n",
            s_EntityCoordinates[ii].x, s_EntityCoordinates[ii].y );
    fprintf( kdebugFile, "ENDEL;\n" );
    s_EntityCoordinates.clear();
}

void CopyPolygonsFromFilledPolysListToKeyFile( ZONE_CONTAINER* aZone, int aLayer )
{
    if( !kdebugFile )
        return;

    unsigned corners_count = aZone->m_FilledPolysList.size();
    unsigned ic    = 0;
    while( ic < corners_count )
    {

        // write polygon:
        StartKeyFilePolygon( aLayer );
        for( ; ic < corners_count; ic++ )
        {
            CPolyPt* corner = &aZone->m_FilledPolysList[ic];
            AddKeyFilePointXY( corner->x, corner->y );
            if( corner->end_contour )
            {
                ic++;
                break;
            }
        }
        EndKeyFilePolygon();
    }
}

#endif
