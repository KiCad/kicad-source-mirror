/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 3Dconnexion
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "nl_3d_viewer_plugin.h"

#if defined( KICAD_USE_3DCONNEXION )
#include "nl_3d_viewer_plugin_impl.h"


NL_3D_VIEWER_PLUGIN::NL_3D_VIEWER_PLUGIN( EDA_3D_CANVAS* aViewport ) :
        m_impl( new NL_3D_VIEWER_PLUGIN_IMPL( aViewport ) )
{
}


NL_3D_VIEWER_PLUGIN::~NL_3D_VIEWER_PLUGIN()
{
    delete m_impl;
}


void NL_3D_VIEWER_PLUGIN::SetFocus( bool focus )
{
    m_impl->SetFocus( focus );
}
#else


NL_3D_VIEWER_PLUGIN::NL_3D_VIEWER_PLUGIN( EDA_3D_CANVAS* aViewport ) : m_impl( nullptr )
{
}


void NL_3D_VIEWER_PLUGIN::SetFocus( bool focus )
{
}


NL_3D_VIEWER_PLUGIN::~NL_3D_VIEWER_PLUGIN()
{
}
#endif
