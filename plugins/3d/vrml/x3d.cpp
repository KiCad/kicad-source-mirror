/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 *  This plugin implements the legacy kicad X3D parser.
 *  Due to the rare use of X3D models, this plugin is a simple
 *  reimplementation of the legacy x3dmodelparser.cpp and is not
 *  intended to be a compliant X3D implementation.
 */

#include <vector>
#include <wx/tokenzr.h>
#include <iostream>

#include "x3d.h"
#include "x3d_ops.h"
#include "x3d_transform.h"
#include "plugins/3dapi/ifsg_all.h"


SCENEGRAPH* X3DPARSER::Load( const wxString& aFileName )
{
    wxXmlDocument doc;

    if( !doc.Load( aFileName ) )
        return NULL;

    if( doc.GetRoot()->GetName() != wxT( "X3D" ) )
        return NULL;

    NODE_LIST children; // VRML Grouping Nodes at top level

    if( !getGroupingNodes( doc.GetRoot(), children ) )
        return NULL;

    X3D_DICT dictionary;    // dictionary for USE/DEF implementation
    X3DNODE* topNode = new X3DTRANSFORM;
    bool ok = false;

    for( NODE_LIST::iterator node_it = children.begin();
         node_it != children.end();
         ++node_it )
    {
        wxXmlNode* node = *node_it;
        wxString name = node->GetName();

        if( name == "Transform" || name == "Group" )
        {
            // Read a Transform / Group
            ok |= X3D::ReadTransform( node, topNode, dictionary );
        }
        else if( name == "Switch" )
        {
            ok |= X3D::ReadSwitch( node, topNode, dictionary );
        }
    }

    SCENEGRAPH* sp = NULL;

    if( ok )
        sp = (SCENEGRAPH*) topNode->TranslateToSG( NULL );

    delete topNode;
    return sp;
}


bool X3DPARSER::getGroupingNodes( wxXmlNode* aNode, std::vector<wxXmlNode*>& aResult )
{
    aResult.clear();
    wxXmlNode* scene = NULL;

    for( wxXmlNode* child = aNode->GetChildren();
         child != NULL;
         child = child->GetNext() )
    {
        if( child->GetName() == "Scene" )
        {
            scene = child;
            break;
        }
    }

    if( NULL == scene )
        return false;

    for( wxXmlNode* child = scene->GetChildren();
         child != NULL;
         child = child->GetNext() )
    {
        wxString name = child->GetName();

        if( name == "Transform" || name == "Switch" || name == "Group" )
            aResult.push_back( child );
    }

    if( aResult.empty() )
        return false;

    return true;
}
