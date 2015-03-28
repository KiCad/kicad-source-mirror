/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file vrmlmodelparser.cpp
 */

#include <fctsys.h>
#include <vector>
#include <macros.h>
#include <kicad_string.h>
#include <info3d_visu.h>

#include "3d_struct.h"
#include "modelparsers.h"


VRML_MODEL_PARSER::VRML_MODEL_PARSER( S3D_MASTER* aMaster ) :
    S3D_MODEL_PARSER( aMaster )
{
}

VRML_MODEL_PARSER::~VRML_MODEL_PARSER()
{
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
        if( childs[idx] )
        {
            delete childs[idx];
            childs[idx] = 0;
        }
    }
}

bool VRML_MODEL_PARSER::Load( const wxString& aFilename )
{
    char       line[11 + 1];
    FILE*      file;

    //DBG( printf( "Load %s", GetChars( aFilename ) ) );

    file = wxFopen( aFilename, wxT( "rt" ) );

    if( file == NULL )
        return false;

    if( fgets( line, 11, file ) == NULL )
    {
        fclose( file );
        return false;
    }

    fclose( file );

    childs.clear();

    if( stricmp( line, "#VRML V2.0" ) == 0 )
    {
        VRML2_MODEL_PARSER *vrml2_parser = new VRML2_MODEL_PARSER( this );
        vrml2_parser->Load( aFilename );
        delete vrml2_parser;
        return true;
    }
    else if( stricmp( line, "#VRML V1.0" ) == 0 )
    {
        VRML1_MODEL_PARSER *vrml1_parser = new VRML1_MODEL_PARSER( this );
        vrml1_parser->Load( aFilename );
        delete vrml1_parser;
        return true;
    }

    DBG( printf( "Unknown internal VRML file format: %s\n", line ) );
    return false;
}
