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
 * @file pcb_line.h
 */

#ifndef PCB_LINE_H_
#define PCB_LINE_H_

#include <wx/wx.h>

#include <pcb_component.h>

namespace PCAD2KICAD {

// Line , routes and drawings
class PCB_LINE : public PCB_COMPONENT
{
public:
    int m_width;
    int m_toX;
    int m_toY;

    PCB_LINE( PCB_CALLBACKS* aCallbacks, BOARD* aBoard );
    ~PCB_LINE();

    virtual void    Parse( XNODE*       aNode,
                           int          aLayer,
                           wxString     aDefaultMeasurementUnit,
                           wxString     aActualConversion );
    virtual void    SetPosOffset( int aX_offs, int aY_offs ) override;
    virtual void    Flip() override;
    void            AddToModule( MODULE* aModule ) override;
    void            AddToBoard() override;
};

} // namespace PCAD2KICAD

#endif    // PCB_LINE_H_
