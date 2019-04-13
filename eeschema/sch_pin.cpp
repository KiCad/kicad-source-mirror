/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lib_pin.h>
#include <sch_component.h>
#include <sch_pin.h>
#include <sch_sheet_path.h>


SCH_PIN::SCH_PIN( LIB_PIN* aLibPin, SCH_COMPONENT* aParentComponent ) :
    SCH_ITEM( nullptr, SCH_PIN_T ),
    m_pin( aLibPin ),
    m_comp( aParentComponent )
{
    SetPosition( aLibPin->GetPosition() );
    m_isDangling = true;
}


SCH_PIN::SCH_PIN( const SCH_PIN& aPin ) :
    SCH_ITEM( aPin )
{
    m_pin = aPin.m_pin;
    m_comp = aPin.m_comp;
    m_position = aPin.m_position;
    m_isDangling = aPin.m_isDangling;
}


wxString SCH_PIN::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString tmp;

#ifdef DEBUG
    tmp.Printf( "SCH_PIN for %s %s",
                m_comp->GetSelectMenuText( aUnits ),
                m_pin->GetSelectMenuText( aUnits ) );
#else
    tmp.Printf( "%s %s",
                m_comp->GetSelectMenuText( aUnits ),
               m_pin->GetSelectMenuText( aUnits ) );
#endif

    return tmp;
}


wxString SCH_PIN::GetDefaultNetName( const SCH_SHEET_PATH aPath )
{
    if( m_pin->IsPowerConnection() )
        return m_pin->GetName();

    std::lock_guard<std::mutex> lock( m_netmap_mutex );

    if( m_net_name_map.count( aPath ) > 0 )
        return m_net_name_map.at( aPath );

    wxString name = "Net-(";

    name << m_comp->GetRef( &aPath );

    // TODO(JE) do we need adoptTimestamp?
    if( /* adoptTimestamp && */ name.Last() == '?' )
        name << m_comp->GetTimeStamp();

    name << "-Pad" << m_pin->GetNumber() << ")";

    m_net_name_map[ aPath ] = name;

    return name;
}


wxPoint SCH_PIN::GetTransformedPosition() const
{
    auto t = m_comp->GetTransform();
    return ( t.TransformCoordinate( GetPosition() ) +
             m_comp->GetPosition() );
}
