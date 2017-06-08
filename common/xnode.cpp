/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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


#include <xnode.h>
#include <macros.h>

typedef wxXmlAttribute   XATTR;


void XNODE::Format( OUTPUTFORMATTER* out, int nestLevel )
{
    switch( GetType() )
    {
    case wxXML_ELEMENT_NODE:
        out->Print( nestLevel, "(%s", out->Quotew( GetName() ).c_str() );
        FormatContents( out, nestLevel );
        if( GetNext() )
            out->Print( 0, ")\n" );
        else
            out->Print( 0, ")" );
        break;

    default:
        FormatContents( out, nestLevel );
    }
}


void XNODE::FormatContents( OUTPUTFORMATTER* out, int nestLevel )
{
    // output attributes first if they exist
    for( XATTR* attr = (XATTR*) GetAttributes();  attr;  attr = (XATTR*) attr->GetNext() )
    {
        out->Print( 0, " (%s %s)",
            // attr names should never need quoting, no spaces, we designed the file.
            out->Quotew( attr->GetName()  ).c_str(),
            out->Quotew( attr->GetValue() ).c_str()
            );
    }

    // we only expect to have used one of two types here:
    switch( GetType() )
    {
    case wxXML_ELEMENT_NODE:

        // output children if they exist.
        for( XNODE* kid = (XNODE*) GetChildren();  kid;  kid = (XNODE*) kid->GetNext() )
        {
            if( kid->GetType() != wxXML_TEXT_NODE )
            {
                if( kid == GetChildren() )
                    out->Print( 0, "\n" );
                kid->Format( out, nestLevel+1 );
            }
            else
            {
                kid->Format( out, 0 );
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

// EOF
