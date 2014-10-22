/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef FRAME_T_H_
#define FRAME_T_H_

/**
 * Enum FRAME_T
 * is the set of EDA_BASE_FRAME derivatives, typically stored in
 * EDA_BASE_FRAME::m_Ident.
 */
enum FRAME_T
{
    FRAME_SCH,
    FRAME_SCH_LIB_EDITOR,
    FRAME_SCH_VIEWER,
    FRAME_SCH_VIEWER_MODAL,

    FRAME_PCB,
    FRAME_PCB_MODULE_EDITOR,
    FRAME_PCB_MODULE_VIEWER,
    FRAME_PCB_MODULE_VIEWER_MODAL,
    FRAME_PCB_FOOTPRINT_WIZARD_MODAL,
    FRAME_PCB_DISPLAY3D,

    FRAME_CVPCB,
    FRAME_CVPCB_DISPLAY,

    FRAME_GERBER,

    FRAME_PL_EDITOR,

    FRAME_BM2CMP,

    FRAME_CALC,

    KIWAY_PLAYER_COUNT,         // counts subset of FRAME_T's which are KIWAY_PLAYER derivatives

    // C++ project manager is not a KIWAY_PLAYER
    KICAD_MAIN_FRAME_T = KIWAY_PLAYER_COUNT,

    FRAME_T_COUNT
};

    //TEXT_EDITOR_FRAME_T,


#endif  // FRAME_T_H_
