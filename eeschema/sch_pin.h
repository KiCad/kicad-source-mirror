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

#ifndef _SCH_PIN_CONNECTION_H
#define _SCH_PIN_CONNECTION_H

#include <sch_item_struct.h>
#include <sch_connection.h>
#include <sch_sheet_path.h>
#include <lib_pin.h>

class SCH_COMPONENT;

class SCH_PIN : public SCH_ITEM
{
    LIB_PIN*       m_pin;
    SCH_COMPONENT* m_comp;

    wxPoint        m_position;
    bool           m_isDangling;

    /// The name that this pin connection will drive onto a net
    std::map<const SCH_SHEET_PATH, wxString> m_net_name_map;

public:
    SCH_PIN( LIB_PIN* aLibPin, SCH_COMPONENT* aParentComponent );

    SCH_PIN( const SCH_PIN& aPin );

    wxString GetClass() const override
    {
        return wxT( "SCH_PIN" );
    }

    LIB_PIN* GetLibPin() const { return m_pin; }

    SCH_COMPONENT* GetParentComponent() const { return m_comp; }
    void SetParentComponent( SCH_COMPONENT* aComp ) { m_comp = aComp; }

    wxString GetDefaultNetName( const SCH_SHEET_PATH aPath );

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, COLOR4D aColor = COLOR4D::UNSPECIFIED ) override {}

    void Move( const wxPoint& aMoveVector ) override {}

    void MirrorY( int aYaxis_position ) override {}

    void MirrorX( int aXaxis_position ) override {}

    void Rotate( wxPoint aPosition ) override {}

    wxPoint GetPosition() const override { return m_position; }

    void SetPosition( const wxPoint& aPosition ) override { m_position = aPosition; }

    bool IsDangling() const override { return m_isDangling; }

    void SetIsDangling( bool isDangling ) { m_isDangling = isDangling; }


    /*
     * While many of these are currently simply covers for the equivalent LIB_PIN methods,
     * the new EESchema file format will soon allow us to override them at the SCH level.
     */
    bool IsVisible() const { return m_pin->IsVisible(); }

    const wxString& GetName() const { return m_pin->GetName(); }

    const wxString& GetNumber() const { return m_pin->GetNumber(); }

    ELECTRICAL_PINTYPE GetType() const { return m_pin->GetType(); }

    bool IsPowerConnection() const { return m_pin->IsPowerConnection(); }


#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif
};

#endif
