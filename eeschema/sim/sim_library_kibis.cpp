/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sim/sim_library_kibis.h>
#include <sim/sim_model_kibis.h>
#include <sim/spice_grammar.h>
#include <ki_exception.h>
#include <locale_io.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


void SIM_LIBRARY_KIBIS::ReadFile( const std::string& aFilePath, SIM_MODEL::TYPE aType )
{
    SIM_LIBRARY::ReadFile( aFilePath );
    m_kibis = KIBIS( aFilePath );

    if( !m_kibis.m_valid )
    {
        THROW_IO_ERROR( wxString::Format( "Invalid ibis file" ) );
        return;
    }

    unsigned pinNumber = 2;

    if( aType == SIM_MODEL::TYPE::KIBIS_DIFFDEVICE || aType == SIM_MODEL::TYPE::KIBIS_DIFFDEVICE )
        pinNumber = 3;

    for( KIBIS_COMPONENT& kcomp : m_kibis.m_components )
    {
        m_models.push_back( SIM_MODEL::Create( aType, pinNumber ) );
        m_modelNames.emplace_back( kcomp.m_name );

        SIM_MODEL_KIBIS* libcomp = dynamic_cast<SIM_MODEL_KIBIS*>( m_models.back().get() );

        if ( libcomp )
            InitModel( *libcomp, kcomp.m_name );
    }
}


bool SIM_LIBRARY_KIBIS::InitModel( SIM_MODEL_KIBIS& aModel, wxString aCompName )
{
    for( KIBIS_COMPONENT& kcomp : m_kibis.m_components )
    {
        if( kcomp.m_name != aCompName )
            continue;

        aModel.m_componentName = kcomp.m_name;
        aModel.m_ibisPins.clear();

        for( KIBIS_PIN& kpin : kcomp.m_pins )
        {
            aModel.m_ibisPins.emplace_back( kpin.m_pinNumber, kpin.m_signalName );
        }
        return true;
    }
    return false;
}
