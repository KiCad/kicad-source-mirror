/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sim/sim_library_ibis.h>
#include <sim/sim_model_ibis.h>
#include <ki_exception.h>
#include <locale_io.h>
#include <pegtl/contrib/parse_tree.hpp>
#include <sch_pin.h>


void SIM_LIBRARY_IBIS::ReadFile( const wxString& aFilePath, REPORTER& aReporter )
{
    SIM_LIBRARY::ReadFile( aFilePath, aReporter );
    m_kibis = KIBIS( aFilePath.ToStdString(), &aReporter );

    if( !m_kibis.m_valid )
    {
        aReporter.Report( wxString::Format( _( "Invalid IBIS file '%s'" ), aFilePath ),
                          RPT_SEVERITY_ERROR );
        return;
    }

    SCH_PIN pinA( nullptr );
    SCH_PIN pinB( nullptr );
    pinA.SetNumber( wxT( "1" ) );
    pinB.SetNumber( wxT( "2" ) );
    std::vector<SCH_PIN*> pins = { &pinA, &pinB };

    for( KIBIS_COMPONENT& kcomp : m_kibis.m_components )
    {
        m_models.push_back( SIM_MODEL::Create( SIM_MODEL::TYPE::KIBIS_DEVICE, pins, aReporter ) );
        m_modelNames.emplace_back( kcomp.m_name );

        SIM_MODEL_IBIS* libcomp = dynamic_cast<SIM_MODEL_IBIS*>( m_models.back().get() );

        if ( libcomp )
            InitModel( *libcomp, kcomp.m_name );
    }
}


bool SIM_LIBRARY_IBIS::InitModel( SIM_MODEL_IBIS& aModel, wxString aCompName )
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

            if( kpin.isDiffPin() )
                m_diffPins.emplace_back( kcomp.m_name, kpin.m_pinNumber );
        }
        return true;
    }
    return false;
}


bool SIM_LIBRARY_IBIS::isPinDiff( const std::string& aComp, const std::string& aPinNumber ) const
{
    for( std::pair<std::string, std::string> aInfo : m_diffPins )
    {
        if( aInfo.first == aComp && aInfo.second == aPinNumber )
            return true;
    }
    return false;
}
