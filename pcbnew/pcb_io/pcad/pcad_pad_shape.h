/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef PCAD_PAD_SHAPE_H
#define PCAD_PAD_SHAPE_H

#include <pcad/pcad_pcb_component.h>

class BOARD;
class wxString;
class XNODE;

namespace PCAD2KICAD {

class PCAD_PAD_SHAPE : public PCAD_PCB_COMPONENT
{
public:
    PCAD_PAD_SHAPE( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard );
    ~PCAD_PAD_SHAPE();

    virtual void Parse( XNODE* aNode, const wxString& aDefaultUnits,
                        const wxString& aActualConversion );

    void AddToBoard( FOOTPRINT* aFootprint = nullptr ) override {}

public:
    wxString    m_Shape;
    int         m_Width;
    int         m_Height;
};

} // namespace PCAD2KICAD

#endif    // PCAD_PAD_SHAPE_H
