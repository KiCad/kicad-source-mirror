/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcb_polygon.h
 */

#ifndef PCB_POLYGON_H_
#define PCB_POLYGON_H_

#include <wx/wx.h>

#include <pcb_component.h>

namespace PCAD2KICAD {

//WX_DEFINE_ARRAY( wxRealPoint*, VERTICES_ARRAY );
WX_DEFINE_ARRAY( VERTICES_ARRAY*, ISLANDS_ARRAY );

class PCB_POLYGON : public PCB_COMPONENT
{
public:
    int             m_width;
    int             m_priority;
    VERTICES_ARRAY  m_outline; // collection of boundary/outline lines - objects
    ISLANDS_ARRAY   m_islands;
    ISLANDS_ARRAY   m_cutouts;

    PCB_POLYGON( PCB_CALLBACKS* aCallbacks, BOARD* aBoard, int aPCadLayer );
    ~PCB_POLYGON();

    virtual bool Parse( XNODE*          aNode,
                        wxString        aDefaultMeasurementUnit,
                        wxString        aActualConversion,
                        wxStatusBar*    aStatusBar );

    virtual void    SetPosOffset( int aX_offs, int aY_offs ) override;
    void            AddToModule( MODULE* aModule ) override;
    void            AddToBoard() override;

// protected:
    void            AssignNet( wxString aNetName );
    void            SetOutline( VERTICES_ARRAY* aOutline );

    void            FormPolygon( XNODE*   aNode, VERTICES_ARRAY* aPolygon,
                                 wxString aDefaultMeasurementUnit, wxString actualConversion );
protected:
    bool            m_filled;
};

} // namespace PCAD2KICAD

#endif    // PCB_POLYGON_H_
