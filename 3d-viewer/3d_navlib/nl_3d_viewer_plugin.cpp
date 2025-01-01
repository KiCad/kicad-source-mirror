/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 3Dconnexion
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "nl_3d_viewer_plugin_impl.h"
#include <advanced_config.h>
#include <kiplatform/drivers.h>


NL_3D_VIEWER_PLUGIN::NL_3D_VIEWER_PLUGIN( EDA_3D_CANVAS* aViewport )
{
    if( ADVANCED_CFG::GetCfg().m_Use3DConnexionDriver
        && KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion() )
    {
        m_impl = std::make_unique<NL_3D_VIEWER_PLUGIN_IMPL>( aViewport, "KiCAD 3D" );
        m_impl->Connect();
    }
}


NL_3D_VIEWER_PLUGIN::~NL_3D_VIEWER_PLUGIN() = default;


void NL_3D_VIEWER_PLUGIN::SetFocus( bool focus )
{
    if( m_impl )
        m_impl->SetFocus( focus );
}
