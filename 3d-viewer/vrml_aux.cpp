/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file vrml_aux.cpp
 */

#include "vrml_aux.h"

char SkipGetChar ( FILE* File )
{
    char c;
    bool re_parse;

    if( (c = fgetc( File )) == EOF )
    {
        //DBG( printf( "EOF\n" ) );
        return EOF;
    }

    //DBG( printf( "c %c 0x%02X\n", c, c ) );

    do
    {
        re_parse = false;

        if ((c == ' ') || (c == '\t') || (c == '{') || (c == '['))
        {
            //DBG( printf( "Skipping space \\t or { or [\n" ) );
            do
            {
                if( (c = fgetc( File )) == EOF )
                {
                     //DBG( printf( "EOF\n" ) );

                    return EOF;
                }
            }
            while((c == ' ') || (c == '\t') || (c == '{') || (c == '['));
        }

        if ((c == '#') || (c == '\n') ||  (c == '\r') || (c == 0) || (c == ','))
        {
            if (c == '#')
            {
                //DBG( printf( "Skipping # \\n or \\r or 0, 0x%02X\n", c ) );
                do
                {
                    if( (c = fgetc( File )) == EOF )
                    {
                        //DBG( printf( "EOF\n" ) );
                        return EOF;
                    }
                }
                while((c != '\n') &&  (c != '\r') && (c != 0) && (c != ','));
            }
            else
            {
                if( (c = fgetc( File )) == EOF )
                {
                    //DBG( printf( "EOF\n" ) );
                    return EOF;
                }
            }

            re_parse = true;
        }
    }while(re_parse == true);

    return c;
}


char* GetNextTag( FILE* File, char* tag )
{

    char c = SkipGetChar( File );

    if (c == EOF)
    {
        return NULL;
    }
    tag[0] = c;
    tag[1] = 0;
    //DBG( printf( "tag[0] %c\n", tag[0] ) );
    if( (c != '}') && (c != ']') )
    {
        char *dst = &tag[1];
        while( fscanf( File, "%c",  dst) )
        {
            if( (*dst == ' ') || (*dst == '[') || (*dst == '{') ||
                (*dst == '\t') || (*dst == '\n')|| (*dst == '\r') )
            {
                *dst = 0;
                break;
            }
            dst++;
        }


        //DBG( printf( "tag %s\n", tag ) );
        c = SkipGetChar( File );

        if (c != EOF)
        {
            // Puts again the read char in the buffer
            ungetc( c, File );
        }
    }

    return tag;
}


int read_NotImplemented( FILE* File, char closeChar)
{
    char c;
    //DBG( printf( "look for %c\n", closeChar) );
    while( (c = fgetc( File )) != EOF )
    {
        if( c == '{' )
        {
            //DBG( printf( "{\n") );
            read_NotImplemented( File, '}' );
        } else if( c == '[' )
        {
            //DBG( printf( "[\n") );
            read_NotImplemented( File, ']' );
        } else if( c == closeChar )
        {
            //DBG( printf( "%c\n", closeChar) );
            return 0;
        }
    }

    DBG( printf( "  NotImplemented failed\n" ) );
    return -1;
}


int parseVertexList( FILE* File, std::vector< glm::vec3 > &dst_vector)
{
    //DBG( printf( "      parseVertexList\n" ) );

    dst_vector.clear();

    glm::vec3 vertex;
    while( parseVertex ( File, vertex ) == 3 )
    {
        dst_vector.push_back( vertex );
    }

    return 0;
}


int parseVertex( FILE* File, glm::vec3 &dst_vertex )
{
    float a,b,c;
    int ret = fscanf( File, "%e %e %e", &a, &b, &c );

    dst_vertex.x = a;
    dst_vertex.y = b;
    dst_vertex.z = c;

    char s = SkipGetChar( File );

    if (s != EOF)
    {
        // Puts again the read char in the buffer
        ungetc( s, File );
    }
    //DBG( printf( "ret%d(%.9f,%.9f,%.9f)", ret, a,b,c) );

    return ret;
}


int parseFloat( FILE* File, float *dst_float )
{
    float value;
    int ret = fscanf( File, "%e", &value );
    *dst_float = value;

    return ret;
}
