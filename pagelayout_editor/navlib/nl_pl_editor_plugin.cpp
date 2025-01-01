/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 3Dconnexion
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

#include "nl_pl_editor_plugin.h"
#include "nl_pl_editor_plugin_impl.h"

#include <advanced_config.h>
#include <kiplatform/drivers.h>


NL_PL_EDITOR_PLUGIN::NL_PL_EDITOR_PLUGIN()
{
    if( ADVANCED_CFG::GetCfg().m_Use3DConnexionDriver
        && KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion() )
    {
        m_impl = std::make_unique<NL_PL_EDITOR_PLUGIN_IMPL>();
    }
}


NL_PL_EDITOR_PLUGIN::~NL_PL_EDITOR_PLUGIN() = default;


void NL_PL_EDITOR_PLUGIN::SetFocus( bool focus )
{
    if( m_impl )
        m_impl->SetFocus( focus );
}


void NL_PL_EDITOR_PLUGIN::SetCanvas( EDA_DRAW_PANEL_GAL* aViewport )
{
    if( m_impl )
        m_impl->SetCanvas( aViewport );
}
