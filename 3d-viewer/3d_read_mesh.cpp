/**
 * @file 3d_read_mesh.cpp
*/

#include "fctsys.h"
#include "common.h"
#include "macros.h"
#include "kicad_string.h"
#include "appl_wxstruct.h"

#include "3d_viewer.h"


int S3D_MASTER::ReadData()
{
    char       line[1024], * text;
    wxFileName fn;
    wxString   FullFilename;
    FILE*      file;
    int        LineNum = 0;

    if( m_Shape3DName.IsEmpty() )
    {
        return 1;
    }


    if( wxFileName::FileExists( m_Shape3DName ) )
        FullFilename = m_Shape3DName;
    else
    {
        fn = m_Shape3DName;
        FullFilename = wxGetApp().FindLibraryPath( fn );

        if( FullFilename.IsEmpty() )
        {
            wxLogDebug( wxT( "3D part library <%s> could not be found." ),
                       GetChars( fn.GetFullPath() ) );
            return -1;
        }
    }

    file = wxFopen( FullFilename, wxT( "rt" ) );

    if( file == NULL )
    {
        return -1;
    }

    // Switch the locale to standard C (needed to print floating point
    // numbers like 1.3)
    SetLocaleTo_C_standard();
    while( GetLine( file, line, &LineNum, 512 ) )
    {
        text = strtok( line, " \t\n\r" );
        if( stricmp( text, "DEF" ) == 0 )
        {
            while( GetLine( file, line, &LineNum, 512 ) )
            {
                text = strtok( line, " \t\n\r" );
                if( text == NULL )
                    continue;
                if( *text == '}' )
                    break;
                if( stricmp( text, "children" ) == 0 )
                {
                    ReadChildren( file, &LineNum );
                }
            }
        }
    }

    fclose( file );
    SetLocaleTo_Default();       // revert to the current locale
    return 0;
}


/*
 * Analyzes the description of the type:
 * DEF yellow material Material (
 * DiffuseColor 1.00000 1.00000 0.00000e 0
 * EmissiveColor 0.00000e 0 0.00000e 0 0.00000e 0
 * SpecularColor 1.00000 1.00000 1.00000
 * AmbientIntensity 1.00000
 * Transparency 0.00000e 0
 * Shininess 1.00000
 *)
 * Or type:
 * USE yellow material
 */
int S3D_MASTER:: ReadMaterial( FILE* file, int* LineNum )
{
    char          line[512], * text, * command;
    wxString      mat_name;
    S3D_MATERIAL* material = NULL;

    command  = strtok( NULL, " \t\n\r" );
    text     = strtok( NULL, " \t\n\r" );
    mat_name = FROM_UTF8( text );
    if( stricmp( command, "USE" ) == 0 )
    {
        for( material = m_Materials; material; material = material->Next() )
        {
            if( material->m_Name == mat_name )
            {
                material->SetMaterial();
                return 1;
            }
        }

        printf( "ReadMaterial error: material not found\n" );
        return 0;
    }

    if( stricmp( command, "DEF" ) == 0 )
    {
        material = new S3D_MATERIAL( this, mat_name );

        Insert( material );

        while( GetLine( file, line, LineNum, 512 ) )
        {
            text = strtok( line, " \t\n\r" );
            if( text == NULL )
                continue;
            if( text[0] == '}' )
            {
                material->SetMaterial();
                return 0;
            }
            if( stricmp( text, "diffuseColor" ) == 0 )
            {
                text = strtok( NULL, " \t\n\r" );
                material->m_DiffuseColor.x = atof( text );
                text = strtok( NULL, " \t\n\r" );
                material->m_DiffuseColor.y = atof( text );
                text = strtok( NULL, " \t\n\r" );
                material->m_DiffuseColor.z = atof( text );
            }
            else if( stricmp( text, "emissiveColor" ) == 0 )
            {
                text = strtok( NULL, " \t\n\r" );
                material->m_EmissiveColor.x = atof( text );
                text = strtok( NULL, " \t\n\r" );
                material->m_EmissiveColor.y = atof( text );
                text = strtok( NULL, " \t\n\r" );
                material->m_EmissiveColor.z = atof( text );
            }
            else if( strnicmp( text, "specularColor", 13 ) == 0 )
            {
                text = strtok( NULL, " \t\n\r" );
                material->m_SpecularColor.x = atof( text );
                text = strtok( NULL, " \t\n\r" );
                material->m_SpecularColor.y = atof( text );
                text = strtok( NULL, " \t\n\r" );
                material->m_SpecularColor.z = atof( text );
            }
            else if( strnicmp( text, "ambientIntensity", 16 ) == 0 )
            {
                text = strtok( NULL, " \t\n\r" );
                material->m_AmbientIntensity = atof( text );
            }
            else if( strnicmp( text, "transparency", 12 ) == 0 )
            {
                text = strtok( NULL, " \t\n\r" );
                material->m_Transparency = atof( text );
            }
            else if( strnicmp( text, "shininess", 9 ) == 0 )
            {
                text = strtok( NULL, " \t\n\r" );
                material->m_Shininess = atof( text );
            }
        }
    }
    return -1;
}


int S3D_MASTER::ReadChildren( FILE* file, int* LineNum )
{
    char line[1024], * text;

    while( GetLine( file, line, LineNum, 512 ) )
    {
        text = strtok( line, " \t\n\r" );
        if( *text == ']' )
            return 0;
        if( *text == ',' )
            continue;

        if( stricmp( text, "Shape" ) == 0 )
        {
            ReadShape( file, LineNum );
        }
        else
        {
            printf( "ReadChildren error line %d <%s> \n", *LineNum, text );
            break;
        }
    }

    return 1;
}


int S3D_MASTER::ReadShape( FILE* file, int* LineNum )
{
    char line[1024], * text;
    int  err = 1;

    while( GetLine( file, line, LineNum, 512 ) )
    {
        text = strtok( line, " \t\n\r" );
        if( *text == '}' )
        {
            err = 0;
            break;
        }

        if( stricmp( text, "appearance" ) == 0 )
        {
            ReadAppearance( file, LineNum );
        }
        else if( stricmp( text, "geometry" ) == 0 )
        {
            ReadGeometry( file, LineNum );
        }
        else
        {
            printf( "ReadShape error line %d <%s> \n", *LineNum, text );
            break;
        }
    }

    return err;
}


int S3D_MASTER::ReadAppearance( FILE* file, int* LineNum )
{
    char line[1024], * text;
    int  err = 1;

    while( GetLine( file, line, LineNum, 512 ) )
    {
        text = strtok( line, " \t\n\r" );
        if( *text == '}' )
        {
            err = 0; break;
        }

        if( stricmp( text, "material" ) == 0 )
        {
            ReadMaterial( file, LineNum );
        }
        else
        {
            printf( "ReadAppearance error line %d <%s> \n", *LineNum, text );
            break;
        }
    }

    return err;
}


#define BUFSIZE 2000

/* Read a coordinate list like:
 *      coord Coordinate { point [
 *        -5.24489 6.57640e-3 -9.42129e-2,
 *        -5.11821 6.57421e-3 0.542654,
 *        -3.45868 0.256565 1.32000 ] }
 *  or:
 *  normal Normal { vector [
 *        0.995171 -6.08102e-6 9.81541e-2,
 *        0.923880 -4.09802e-6 0.382683,
 *        0.707107 -9.38186e-7 0.707107]
 *      }
 *
 *  Return the coordinate list
 *  text_buffer contains the first line of this node :
 *     "coord Coordinate { point ["
 */
double* ReadCoordsList( FILE* file, char* text_buffer, int* bufsize,
                        int* LineNum )
{
    double*      data_list = NULL;
    unsigned int ii = 0, jj = 0, nn = BUFSIZE;
    char*        text;
    bool         HasData   = FALSE;
    bool         StartData = FALSE;
    bool         EndData   = FALSE;
    bool         EndNode   = FALSE;
    char         string_num[512];

    text = text_buffer;
    while( !EndNode )
    {
        if( *text == 0 )  // Needs data !
        {
            text = text_buffer;
            GetLine( file, text_buffer, LineNum, 512 );
        }

        while( !EndNode && *text )
        {
            switch( *text )
            {
            case '[':
                StartData = TRUE;
                jj = 0; string_num[jj] = 0;
                data_list = (double*) MyZMalloc( nn * sizeof(double) );
                break;

            case '}':
                EndNode = TRUE;
                break;

            case ']':
            case '\t':
            case ' ':
            case ',':
                jj = 0;
                if( !StartData || !HasData )
                    break;
                data_list[ii]  = atof( string_num );
                string_num[jj] = 0;
                ii++;
                if( ii >= nn )
                {
                    nn *= 2;
                    data_list =
                        (double*) realloc( data_list, ( nn * sizeof(double) ) );
                }
                HasData = FALSE;
                if( *text == ']' )
                {
                    StartData = FALSE;
                    EndData   = TRUE;
                }
                break;

            default:
                if( !StartData )
                    break;
                if( jj >= sizeof(string_num) )
                    break;
                string_num[jj] = *text;
                jj++; string_num[jj] = 0;
                HasData = TRUE;
                break;
            }

            text++;
        }
    }

    if( data_list )
        data_list = (double*) realloc( data_list, ( ii * sizeof(double) ) );
    if( bufsize )
        *bufsize = ii;
    return data_list;
}


int S3D_MASTER::ReadGeometry( FILE* file, int* LineNum )
{
    char    line[1024], buffer[1024], * text;
    int     err    = 1;
    int     nn     = BUFSIZE;
    double* points = NULL;
    int*    index  = NULL;

    while( GetLine( file, line, LineNum, 512 ) )
    {
        strcpy( buffer, line );
        text = strtok( buffer, " \t\n\r" );
        if( *text == '}' )
        {
            err = 0; break;
        }

        if( stricmp( text, "normalPerVertex" ) == 0 )
        {
            text = strtok( NULL, " ,\t\n\r" );
            if( stricmp( text, "TRUE" ) == 0 )
            {
            }
            else
            {
            }
            continue;
        }

        if( stricmp( text, "colorPerVertex" ) == 0 )
        {
            text = strtok( NULL, " ,\t\n\r" );
            if( stricmp( text, "TRUE" ) == 0 )
            {
            }
            else
            {
            }
            continue;
        }

        if( stricmp( text, "normal" ) == 0 )
        {
            int     coord_number;
            double* buf_points = ReadCoordsList( file, line, &coord_number,
                                                 LineNum );
            // Do something if needed
            free( buf_points );
            continue;
        }
        if( stricmp( text, "normalIndex" ) == 0 )
        {
            while( GetLine( file, line, LineNum, 512 ) )
            {
                text = strtok( line, " ,\t\n\r" );
                while( text )
                {
                    if( *text == ']' )
                        break;
                    text = strtok( NULL, " ,\t\n\r" );
                }

                if( text && (*text == ']') )
                    break;
            }

            continue;
        }

        if( stricmp( text, "color" ) == 0 )
        {
            int     coord_number;
            double* buf_points = ReadCoordsList( file, line, &coord_number,
                                                 LineNum );
            // Do something if needed
            free( buf_points );
            continue;
        }
        if( stricmp( text, "colorIndex" ) == 0 )
        {
            while( GetLine( file, line, LineNum, 512 ) )
            {
                text = strtok( line, " ,\t\n\r" );
                while( text )
                {
                    if( *text == ']' )
                        break;
                    text = strtok( NULL, " ,\t\n\r" );
                }

                if( text && (*text == ']') )
                    break;
            }

            continue;
        }

        if( stricmp( text, "coord" ) == 0 )
        {
            int coord_number;
            points = ReadCoordsList( file, line, &coord_number, LineNum );
        }
        else if( stricmp( text, "coordIndex" ) == 0 )
        {
            index = (int*) MyMalloc( nn * sizeof(int) );
            S3D_Vertex* coords =
                (S3D_Vertex*) MyMalloc( nn * sizeof(S3D_Vertex) );
            while( GetLine( file, line, LineNum, 512 ) )
            {
                int coord_count = 0, jj;
                text = strtok( line, " ,\t\n\r" );
                while( text )
                {
                    if( *text == ']' )
                        break;
                    jj = atoi( text );
                    if( jj < 0 )
                    {
                        S3D_Vertex* curr_coord = coords;
                        for( jj = 0; jj < coord_count; jj++ )
                        {
                            int kk = index[jj] * 3;
                            curr_coord->x = points[kk];
                            curr_coord->y = points[kk + 1];
                            curr_coord->z = points[kk + 2];
                            curr_coord++;
                        }

                        Set_Object_Coords( coords, coord_count );
                        Set_Object_Data( coords, coord_count );
                        coord_count = 0;
                    }
                    else
                    {
                        index[coord_count++] = jj;
                    }
                    text = strtok( NULL, " ,\t\n\r" );
                }

                if( text && (*text == ']') )
                    break;
            }

            free( index );
            free( coords );
        }
        else
        {
            printf( "ReadGeometry error line %d <%s> \n", *LineNum, text );
            break;
        }
    }

    if( points )
        free( points );

    return err;
}


int Struct3D_Shape:: ReadData( FILE* file, int* LineNum )
{
    char line[512];

    while( GetLine( file, line, LineNum, 512 ) )
    {
    }

    return -1;
}
