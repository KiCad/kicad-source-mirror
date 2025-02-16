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

#ifndef XNODE_H_
#define XNODE_H_

// quiet the deprecated warnings with 3 lines:
#include <wx/defs.h>
#undef wxDEPRECATED
#define wxDEPRECATED(x) x

#include <wx/xml/xml.h>
#include <kicommon.h>

class OUTPUTFORMATTER;

/**
 * An extension of wxXmlNode that can format its contents as KiCad-style s-expressions
 */
class KICOMMON_API XNODE : public wxXmlNode
{
public:
    XNODE() :
        wxXmlNode()
    {
    }

    XNODE( wxXmlNodeType aType, const wxString& aName, const wxString& aContent = wxEmptyString ) :
        wxXmlNode( nullptr, aType, aName, aContent )
    {
    }

    XNODE( XNODE* aParent, wxXmlNodeType aType, const wxString& aName,
           const wxString& aContent = wxEmptyString, wxXmlAttribute* aProperties = nullptr ) :
        wxXmlNode( aParent, aType, aName, aContent, aProperties )
    {
    }

    XNODE* GetChildren() const
    {
        return static_cast<XNODE *>( wxXmlNode::GetChildren() );
    }

    XNODE* GetNext() const
    {
        return static_cast<XNODE *>( wxXmlNode::GetNext() );
    }

    XNODE* GetParent() const
    {
        return static_cast<XNODE *>( wxXmlNode::GetParent() );
    }

    /**
     * Write this object as #UTF8 out to an #OUTPUTFORMATTER as an S-expression.
     *
     * @param out The formatter to write to.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    void Format( OUTPUTFORMATTER* out ) const;

    /**
     * Write the contents of object as #UTF8 out to an #OUTPUTFORMATTER as an S-expression.
     *
     * This is the same as Format() except that the outer wrapper is not included.
     *
     * @param out The formatter to write to.
     * @throw IO_ERROR if a system error writing the output, such as a full disk.
     */
    void FormatContents( OUTPUTFORMATTER* out ) const;

    /**
     * 
     * @return the contents of the object as a UTF-8 string, pretty-printed
     */
    wxString Format() const;

};

#endif  // XNODE_H_
