/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef FIX_BOARD_SHAPE_H_
#define FIX_BOARD_SHAPE_H_

#include <memory>
#include <vector>
#include <pcb_shape.h>

/**
 * Connects shapes to each other, making continious contours (adjacent shapes will have a common vertex)
 * aChainingEpsilon is the max distance between vertices of different shapes to connect.
 * Modifies original shapes
 */
void ConnectBoardShapes( std::vector<PCB_SHAPE*>& aShapeList, int aChainingEpsilon );

#endif // FIX_BOARD_SHAPE_H_