/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  mortoncodes.h
 * @brief Implements Morton Codes
 * https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
 * http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
 */

#ifndef _MORTONCODES_H_
#define _MORTONCODES_H_

#include <cstdint>

uint32_t EncodeMorton2( uint32_t x, uint32_t y );
uint32_t EncodeMorton3( uint32_t x, uint32_t y, uint32_t z );

uint32_t DecodeMorton2X( uint32_t code );
uint32_t DecodeMorton2Y( uint32_t code );

uint32_t DecodeMorton3X( uint32_t code );
uint32_t DecodeMorton3Y( uint32_t code );
uint32_t DecodeMorton3Z( uint32_t code );

#endif // _MORTONCODES_H_
