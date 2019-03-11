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
#include <sch_pin_connection.h>
#include <sch_sheet_path.h>


SCH_PIN_CONNECTION::SCH_PIN_CONNECTION( EDA_ITEM* aParent ) :
    SCH_ITEM( aParent, SCH_PIN_CONNECTION_T )
{
}


wxString SCH_PIN_CONNECTION::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString tmp;

#ifdef DEBUG
    tmp.Printf( _( "SCH_PIN_CONNECTION for %s %s" ),
                GetChars( m_comp->GetSelectMenuText( aUnits ) ),
                GetChars( m_pin->GetSelectMenuText( aUnits ) ) );
#else
    tmp.Printf( _( "%s %s" ),
                GetChars( m_comp->GetSelectMenuText( aUnits ) ),
                GetChars( m_pin->GetSelectMenuText( aUnits ) ) );
#endif

    return tmp;
}


wxString SCH_PIN_CONNECTION::GetDefaultNetName( const SCH_SHEET_PATH aPath )
{
    if( m_pin->IsPowerConnection() )
        return m_pin->GetName();

    wxString name;

    try
    {
        name = m_net_name_map.at( aPath );
    }
    catch( const std::out_of_range& oor )
    {
        name = wxT( "Net-(" );

        name << m_comp->GetRef( &aPath );

        // TODO(JE) do we need adoptTimestamp?
        if( /* adoptTimestamp && */ name.Last() == '?' )
            name << m_comp->GetTimeStamp();

        name << _( "-Pad" ) << m_pin->GetNumber() << _( ")" );

        m_net_name_map[ aPath ] = name;
    }

    return name;
}


wxPoint SCH_PIN_CONNECTION::GetPosition() const
{
    auto pos = m_comp->GetPosition();
    auto transform = m_comp->GetTransform();

    return pos + transform.TransformCoordinate( m_pin->GetPosition() );
}
