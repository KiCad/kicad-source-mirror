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

/**
 * Container to describe the net connectivity of a specific pin on a component.
 */
class SCH_PIN_CONNECTION : public SCH_ITEM
{
public:
    SCH_PIN_CONNECTION( EDA_ITEM* aParent = nullptr );

    wxString GetClass() const override
    {
        return wxT( "SCH_PIN_CONNECTION" );
    }

    wxString GetDefaultNetName( const SCH_SHEET_PATH aPath );

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
               GR_DRAWMODE aDrawMode, COLOR4D aColor = COLOR4D::UNSPECIFIED ) override
    {
    }

    void Move( const wxPoint& aMoveVector ) override {}

    void MirrorY( int aYaxis_position ) override {}

    void MirrorX( int aXaxis_position ) override {}

    void Rotate( wxPoint aPosition ) override {}

    wxPoint GetPosition() const override;

    void SetPosition( const wxPoint& aPosition ) override {}

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override {}
#endif

    LIB_PIN* m_pin;

    SCH_COMPONENT* m_comp;

    /// The name that this pin connection will drive onto a net
    std::map<const SCH_SHEET_PATH, wxString> m_net_name_map;
};

#endif
