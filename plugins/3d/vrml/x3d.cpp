/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
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

/*
 * Description:
 *  This plugin implements the legacy KiCad X3D parser.
 *  Due to the rare use of X3D models, this plugin is a simple
 *  reimplementation of the legacy x3dmodelparser.cpp and is not
 *  intended to be a compliant X3D implementation.
 */

#include <vector>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include <iostream>

#include "x3d.h"
#include "x3d_ops.h"
#include "x3d_transform.h"
#include "plugins/3dapi/ifsg_all.h"


SCENEGRAPH* X3DPARSER::Load( const wxString& aFileName )
{
    wxFFileInputStream stream( aFileName );
    wxXmlDocument doc;

    if( !stream.IsOk() || !doc.Load( stream ) )
        return nullptr;

    if( doc.GetRoot()->GetName() != wxT( "X3D" ) )
        return nullptr;

    NODE_LIST children; // VRML Grouping Nodes at top level

    if( !getGroupingNodes( doc.GetRoot(), children ) )
        return nullptr;

    X3D_DICT dictionary;    // dictionary for USE/DEF implementation
    X3DNODE* topNode = new X3DTRANSFORM;
    bool ok = false;

    for( NODE_LIST::iterator node_it = children.begin(); node_it != children.end(); ++node_it )
    {
        wxXmlNode* node = *node_it;
        wxString name = node->GetName();

        if( name == wxT( "Transform" ) || name == wxT( "Group" ) )
        {
            // Read a Transform / Group
            ok |= X3D::ReadTransform( node, topNode, dictionary );
        }
        else if( name == wxT( "Switch" ) )
        {
            ok |= X3D::ReadSwitch( node, topNode, dictionary );
        }
    }

    SCENEGRAPH* sp = nullptr;

    if( ok )
        sp = (SCENEGRAPH*) topNode->TranslateToSG( nullptr );

    delete topNode;
    return sp;
}


bool X3DPARSER::getGroupingNodes( wxXmlNode* aNode, std::vector<wxXmlNode*>& aResult )
{
    aResult.clear();
    wxXmlNode* scene = nullptr;

    for( wxXmlNode* child = aNode->GetChildren(); child != nullptr; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "Scene" ) )
        {
            scene = child;
            break;
        }
    }

    if( nullptr == scene )
        return false;

    for( wxXmlNode* child = scene->GetChildren(); child != nullptr; child = child->GetNext() )
    {
        const wxString& name = child->GetName();

        if( name == wxT( "Transform" ) || name == wxT( "Switch" ) || name == wxT( "Group" ) )
            aResult.push_back( child );
    }

    if( aResult.empty() )
        return false;

    return true;
}
