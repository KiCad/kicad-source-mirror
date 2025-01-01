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

#include "nl_pcbnew_plugin.h"
#include "nl_pcbnew_plugin_impl.h"
#include <advanced_config.h>
#include <kiplatform/drivers.h>

NL_PCBNEW_PLUGIN::NL_PCBNEW_PLUGIN( PCB_DRAW_PANEL_GAL* aViewport ) : m_impl( nullptr )
{
    if( ADVANCED_CFG::GetCfg().m_Use3DConnexionDriver
        && KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion() )
    {
        m_impl = std::make_unique<NL_PCBNEW_PLUGIN_IMPL>( aViewport );
    }
}


NL_PCBNEW_PLUGIN::~NL_PCBNEW_PLUGIN()
{}


void NL_PCBNEW_PLUGIN::SetFocus( bool focus )
{
    if( m_impl )
        m_impl->SetFocus( focus );
}
