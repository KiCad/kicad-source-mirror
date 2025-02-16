/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include "xnode.h"

#include <richio.h>
#include <string_utils.h>
#include <io/kicad/kicad_io_utils.h>


void XNODE::Format( OUTPUTFORMATTER* out ) const
{
    switch( GetType() )
    {
    case wxXML_ELEMENT_NODE:
        out->Print( "(%s", TO_UTF8( GetName() ) );
        FormatContents( out );

        if( GetNext() )
            out->Print( 0, ")\n" );
        else
            out->Print( 0, ")" );

        break;

    default:
        FormatContents( out );
    }
}


void XNODE::FormatContents( OUTPUTFORMATTER* out ) const
{
    // output attributes first if they exist
    for( wxXmlAttribute* attr = GetAttributes(); attr; attr = attr->GetNext() )
    {
        out->Print( 0, " (%s %s)",
                    TO_UTF8( attr->GetName() ),
                    out->Quotew( attr->GetValue() ).c_str() );
    }

    // we only expect to have used one of two types here:
    switch( GetType() )
    {
    case wxXML_ELEMENT_NODE:
        for( XNODE* child = GetChildren(); child; child = child->GetNext() )
        {
            if( child->GetType() != wxXML_TEXT_NODE )
            {
                if( child == GetChildren() )
                    out->Print( 0, "\n" );

                child->Format( out );
            }
            else
            {
                child->Format( out );
            }
        }

        break;

    case wxXML_TEXT_NODE:
        out->Print( 0, " %s", out->Quotew( GetContent() ).c_str() );
        break;

    default:
        ;   // not supported
    }
}


wxString XNODE::Format() const
{
    STRING_FORMATTER formatter;
    Format( &formatter );
    KICAD_FORMAT::Prettify( formatter.MutableString(), false );
    return formatter.GetString();
}

// EOF
