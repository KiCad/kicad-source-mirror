#ifndef XATTR_H_
#define XATTR_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 KiCAd Developers, see change_log.txt for contributors.
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

#include <richio.h>

// quiet the deprecated warnings with 3 lines:
#include <wx/defs.h>
#undef wxDEPRECATED
#define wxDEPRECATED(x) x

#include <wx/xml/xml.h>


/**
 * Class XNODE
 * holds an XML or S-expression element.  It is used for eXporting
 * a document tree in EITHER XML or S-expression.
 */
class XNODE : public wxXmlNode
{
public:
    //-----<overloads>---------------------------------------------------------
    XNODE() :
        wxXmlNode()
    {
    }

    XNODE( wxXmlNodeType aType, const wxString& aName, const wxString& aContent = wxEmptyString ) :
        wxXmlNode( NULL, aType, aName, aContent )
    {
    }

    XNODE( XNODE* aParent, wxXmlNodeType aType, const wxString& aName,
           const wxString& aContent = wxEmptyString, wxXmlAttribute* aProperties = NULL ) :
        wxXmlNode( aParent, aType, aName, aContent, aProperties )
    {
    }

    XNODE* GetChildren() const
    {
        return (XNODE* )wxXmlNode::GetChildren();
    }

    XNODE* GetNext() const
    {
        return (XNODE* )wxXmlNode::GetNext();
    }

    XNODE* GetParent() const
    {
        return (XNODE* )wxXmlNode::GetParent();
    }
    //-----</overloads>--------------------------------------------------------

    /**
     * Function Format
     * writes this object as UTF8 out to an OUTPUTFORMATTER as an S-expression.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    virtual void Format( OUTPUTFORMATTER* out, int nestLevel ) throw( IO_ERROR );

    /**
     * Function FormatContents
     * writes the contents of object as UTF8 out to an OUTPUTFORMATTER as an S-expression.
     * This is the same as Format() except that the outer wrapper is not included.
     * @param out The formatter to write to.
     * @param nestLevel A multiple of the number of spaces to preceed the output with.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    virtual void FormatContents( OUTPUTFORMATTER* out, int nestLevel ) throw( IO_ERROR );

};

#endif  // XATTR_H_
