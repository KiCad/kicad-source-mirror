#ifndef _XATTR_H_
#define _XATTR_H_

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "richio.h"
#include <wx/xml/xml.h>


/**
 * Class XNODE
 * holds an XML or S-expression element.  It is used for eXporting
 * a document tree in EITHER XML or S-expression.
 */
class XNODE : public wxXmlNode
{
public:
    XNODE() :
        wxXmlNode()
    {
    }

    XNODE( wxXmlNodeType aType, const wxString& aName, const wxString& aContent = wxEmptyString ) :
        wxXmlNode( NULL, aType, aName, aContent )
    {
    }

    /**
     * Function Format
     * writes this object as UTF8 out to an OUTPUTFORMATTER as an S-expression.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    virtual void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );

    /**
     * Function FormatContents
     * writes the contents of object as UTF8 out to an OUTPUTFORMATTER as an S-expression.
     * This is the same as Format() except that the outer wrapper is not included.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IOError if a system error writing the output, such as a full disk.
     */
    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IOError );
};

#endif  // _XATTR_H_
