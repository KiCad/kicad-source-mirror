/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright (C) 2019 KiCad Developers, see change_log.txt for contributors.
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
    SCH_ITEM( aParentComponent, SCH_PIN_T ),
    m_libPin( aLibPin )
{
    SetPosition( aLibPin->GetPosition() );
    m_isDangling = true;
}


SCH_PIN::SCH_PIN( const SCH_PIN& aPin ) :
        SCH_ITEM( aPin )
{
    m_libPin = aPin.m_libPin;
    m_position = aPin.m_position;
    m_isDangling = aPin.m_isDangling;
}


SCH_PIN& SCH_PIN::operator=( const SCH_PIN& aPin )
{
    SCH_ITEM::operator=( aPin );

    m_libPin = aPin.m_libPin;
    m_position = aPin.m_position;
    m_isDangling = aPin.m_isDangling;

    return *this;
}


bool SCH_PIN::Matches( wxFindReplaceData& aSearchData, void* aAuxData )
{
    return m_libPin->Matches( aSearchData, aAuxData );
}


SCH_COMPONENT* SCH_PIN::GetParentComponent() const
{
    return static_cast<SCH_COMPONENT*>( GetParent() );
}


wxString SCH_PIN::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( "%s %s",
                             GetParentComponent()->GetSelectMenuText( aUnits ),
                             m_libPin->GetSelectMenuText( aUnits ) );
}


void SCH_PIN::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    m_libPin->GetMsgPanelInfo( aUnits, aList, GetParentComponent() );
}


wxString SCH_PIN::GetDefaultNetName( const SCH_SHEET_PATH aPath )
{
    if( m_libPin->IsPowerConnection() )
        return m_libPin->GetName();

    std::lock_guard<std::mutex> lock( m_netmap_mutex );

    if( m_net_name_map.count( aPath ) > 0 )
        return m_net_name_map.at( aPath );

    wxString name = "Net-(";

    name << GetParentComponent()->GetRef( &aPath );

    bool annotated = true;

    // Add timestamp for uninitialized components
    if( name.Last() == '?' )
    {
        name << GetParentComponent()->GetTimeStamp();
        annotated = false;
    }

    name << "-Pad" << m_libPin->GetNumber() << ")";

    if( annotated )
        m_net_name_map[ aPath ] = name;

    return name;
}


wxPoint SCH_PIN::GetTransformedPosition() const
{
    TRANSFORM t = GetParentComponent()->GetTransform();
    return ( t.TransformCoordinate( GetPosition() ) + GetParentComponent()->GetPosition() );
}


const EDA_RECT SCH_PIN::GetBoundingBox() const
{
    TRANSFORM t = GetParentComponent()->GetTransform();
    EDA_RECT  r = m_libPin->GetBoundingBox();

    r.RevertYAxis();

    r = t.TransformCoordinate( r );
    r.Offset( GetParentComponent()->GetPosition() );

    return r;
}


bool SCH_PIN::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();
    return rect.Inflate( aAccuracy ).Contains( aPosition );
}



