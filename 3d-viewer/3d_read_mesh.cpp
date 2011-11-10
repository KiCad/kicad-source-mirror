/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
    {
        FullFilename = m_Shape3DName;
    }
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

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
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


int S3D_MASTER::ReadMaterial( FILE* file, int* LineNum )
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

/**
 * Function ReadCoordList
 * reads 3D coordinate lists like:
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
 *  text_buffer contains the first line of this node :
 *     "coord Coordinate { point ["
 */
void ReadCoordsList( FILE* file, char* text_buffer, std::vector< double >& aList, int* LineNum )
{
    unsigned int ii = 0, jj = 0;
    char*        text;
    bool         HasData   = false;
    bool         StartData = false;
    bool         EndNode   = false;
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
                StartData = true;
                jj = 0;
                string_num[jj] = 0;
                break;

            case '}':
                EndNode = true;
                break;

            case ']':
            case '\t':
            case ' ':
            case ',':
                jj = 0;

                if( !StartData || !HasData )
                    break;

                aList.push_back( atof( string_num ) );
                string_num[jj] = 0;
                ii++;

                HasData = false;

                if( *text == ']' )
                {
                    StartData = false;
                }

                break;

            default:
                if( !StartData )
                    break;

                if( jj >= sizeof( string_num ) )
                    break;

                string_num[jj] = *text;
                jj++;
                string_num[jj] = 0;
                HasData = true;
                break;
            }

            text++;
        }
    }
}


int S3D_MASTER::ReadGeometry( FILE* file, int* LineNum )
{
    char    line[1024], buffer[1024], * text;
    int     err    = 1;
    std::vector< double > points;
    std::vector< double > list;

    while( GetLine( file, line, LineNum, 512 ) )
    {
        strcpy( buffer, line );
        text = strtok( buffer, " \t\n\r" );

        if( *text == '}' )
        {
            err = 0;
            break;
        }

        if( stricmp( text, "normalPerVertex" ) == 0 )
        {
            text = strtok( NULL, " ,\t\n\r" );

            if( stricmp( text, "true" ) == 0 )
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

            if( stricmp( text, "true" ) == 0 )
            {
            }
            else
            {
            }
            continue;
        }

        if( stricmp( text, "normal" ) == 0 )
        {
            ReadCoordsList( file, line, list, LineNum );
            list.clear();
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
            ReadCoordsList( file, line, list, LineNum );
            list.clear();
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
            ReadCoordsList( file, line, points, LineNum );
        }
        else if( stricmp( text, "coordIndex" ) == 0 )
        {
            if( points.size() < 3 || points.size() % 3 != 0 )
            {
                wxLogError( wxT( "3D geometry read error <%s> at line %d." ),
                            GetChars( FROM_UTF8( text ) ), *LineNum );
                err = 1;
                break;
            }

            std::vector< int > coordIndex;
            std::vector< S3D_Vertex > vertices;

            while( GetLine( file, line, LineNum, 512 ) )
            {
                int jj;
                text = strtok( line, " ,\t\n\r" );

                while( text )
                {
                    if( *text == ']' )
                        break;

                    jj = atoi( text );

                    if( jj < 0 )
                    {
                        for( jj = 0; jj < (int) coordIndex.size(); jj++ )
                        {
                            int kk = coordIndex[jj] * 3;

                            if( (kk < 0) || ((kk + 3) > points.size()) )
                            {
                                wxLogError( wxT( "3D geometry index read error <%s> at line %d." ),
                                            GetChars( FROM_UTF8( text ) ), *LineNum );
                                err = 1;
                                break;
                            }

                            S3D_Vertex vertex;
                            vertex.x = points[kk];
                            vertex.y = points[kk + 1];
                            vertex.z = points[kk + 2];
                            vertices.push_back( vertex );
                        }

                        Set_Object_Coords( vertices );
                        Set_Object_Data( vertices );
                        vertices.clear();
                        coordIndex.clear();
                    }
                    else
                    {
                        coordIndex.push_back( jj );
                    }

                    text = strtok( NULL, " ,\t\n\r" );
                }

                if( text && (*text == ']') )
                    break;
            }
        }
        else
        {
            wxLogError( wxT( "3D geometry read error <%s> at line %d." ),
                        GetChars( FROM_UTF8( text ) ), *LineNum );
            err = 1;
            break;
        }
    }

    return err;
}


int Struct3D_Shape::ReadData( FILE* file, int* LineNum )
{
    char line[512];

    while( GetLine( file, line, LineNum, 512 ) )
    {
    }

    return -1;
}
