/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras@wanadoo.fr
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

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <kicad_string.h>
#include <appl_wxstruct.h>

#include <3d_viewer.h>
#include <info3d_visu.h>
#include "3d_struct.h"
#include "modelparsers.h"


S3D_MODEL_PARSER* S3D_MODEL_PARSER::Create( S3D_MASTER* aMaster,
                                            const wxString aExtension )
{
    if ( aExtension == wxT( "x3d" ) )
    {
        return new X3D_MODEL_PARSER( aMaster );
    }
    else if ( aExtension == wxT( "wrl" ) )
    {
        return new VRML_MODEL_PARSER( aMaster );
    }
    else
    {
        return NULL;
    }
}

const wxString S3D_MASTER::GetShape3DFullFilename()
{

    wxString shapeName;

    // Expand any environment variables embedded in footprint's m_Shape3DName field.
    // To ensure compatibility with most of footprint's m_Shape3DName field,
    // if the m_Shape3DName is not an absolute path the default path
    // given by the environment variable KISYS3DMOD will be used

    if( m_Shape3DName.StartsWith( wxT("${") ) )
        shapeName = wxExpandEnvVars( m_Shape3DName );
    else
        shapeName = m_Shape3DName;

    wxFileName fn( shapeName );

    if( fn.IsAbsolute() || shapeName.StartsWith( wxT(".") ) )
        return shapeName;

    wxString default_path;
    wxGetEnv( wxT( KISYS3DMOD ), &default_path );

    if( default_path.IsEmpty() )
        return shapeName;

    if( !default_path.EndsWith( wxT("/") ) && !default_path.EndsWith( wxT("\\") ) )
        default_path += wxT("/");

    default_path += shapeName;

    return default_path;
}

int S3D_MASTER::ReadData()
{
    if( m_Shape3DName.IsEmpty() )
        return 1;

    wxString filename = GetShape3DFullFilename();

#ifdef __WINDOWS__
    filename.Replace( wxT( "/" ), wxT( "\\" ) );
#else
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    if( !wxFileName::FileExists( filename ) )
    {
        wxLogDebug( wxT( "3D shape '%s' not found, even tried '%s' after env var substitution." ),
                    GetChars( m_Shape3DName ),
                    GetChars( filename )
                    );
        return -1;
    }

    wxFileName fn( filename );

    wxString extension = fn.GetExt();
    S3D_MODEL_PARSER* parser = S3D_MODEL_PARSER::Create( this, extension );

    if( parser )
    {
        parser->Load( filename );
        delete parser;
        return 0;
    }
    else
    {
        wxLogDebug( wxT( "Unknown file type '%s'" ), GetChars( extension ) );
    }

    return -1;
}
