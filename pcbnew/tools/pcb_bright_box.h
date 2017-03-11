/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see change_log.txt for contributors.
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

#ifndef __PCB_BRIGHT_BOX_H
#define __PCB_BRIGHT_BOX_H

#include <preview_items/bright_box.h>

/**
 * Class PCB_BRIGHT_BOX
 *
 * Draws a decoration to indicate a brightened item.
 */
class PCB_BRIGHT_BOX : public BRIGHT_BOX
{
public:
    PCB_BRIGHT_BOX();
    ~PCB_BRIGHT_BOX() {}

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

private:
    static const double PCB_LINE_WIDTH;

};

#endif
