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
 * @file pcb_pad.h
 */

#ifndef PCB_PAD_H_
#define PCB_PAD_H_

#include <wx/wx.h>

#include <pcb_component.h>
#include <pcb_pad_shape.h>

namespace PCAD2KICAD {

class PCB_PAD : public PCB_COMPONENT
{
public:
    int m_number;
    int m_hole;
    bool m_isHolePlated;
    PCB_PAD_SHAPES_ARRAY m_shapes;

    PCB_PAD( PCB_CALLBACKS* aCallbacks, BOARD* aBoard );
    ~PCB_PAD();

    virtual void    Parse( XNODE*       aNode,
                           wxString     aDefaultMeasurementUnit,
                           wxString     aActualConversion );
    virtual void    Flip() override;
    void            AddToModule( MODULE* aModule, int aRotation, bool aEncapsulatedPad );
    void            AddToBoard() override;

private:
    wxString m_defaultPinDes;
};

} // namespace PCAD2KICAD

#endif    // PCB_PAD_H_
