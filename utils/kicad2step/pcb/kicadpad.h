/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file kicadpad.h
 * declares the PAD description object.
 */

#ifndef KICADPAD_H
#define KICADPAD_H

#include <string>
#include <vector>
#include "base.h"


struct KICADDRILL
{
    DOUBLET size;
    bool    oval;
};


class KICADPAD
{
private:
    bool        m_thruhole;
    bool parseDrill( SEXPR::SEXPR* aDrill );

public:
    KICADPAD();
    virtual ~KICADPAD();

    bool Read( SEXPR::SEXPR* aEntry );

    bool IsThruHole()
    {
        return m_thruhole;
    }

    DOUBLET     m_position;
    double      m_rotation; // rotation (radians)
    KICADDRILL  m_drill;
};

#endif  // KICADPAD_H
