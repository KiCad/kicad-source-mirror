/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * The set of #EDA_BASE_FRAME derivatives, typically stored in EDA_BASE_FRAME::m_Ident.
 */
enum FRAME_T
{
    FRAME_SCH = 0,
    FRAME_SCH_SYMBOL_EDITOR,
    FRAME_SCH_VIEWER,
    FRAME_SYMBOL_CHOOSER,
    FRAME_SIMULATOR,
    FRAME_SCH_DIFF,
    FRAME_SYM_DIFF,

    FRAME_PCB_EDITOR,
    FRAME_FOOTPRINT_EDITOR,
    FRAME_FOOTPRINT_CHOOSER,
    FRAME_FOOTPRINT_VIEWER,
    FRAME_FOOTPRINT_WIZARD,
    FRAME_PCB_DISPLAY3D,
    FRAME_FOOTPRINT_PREVIEW,
    FRAME_PCB_DIFF,
    FRAME_FOOTPRINT_DIFF,

    FRAME_CVPCB,
    FRAME_CVPCB_DISPLAY,

    FRAME_PYTHON,

    FRAME_GERBER,

    FRAME_PL_EDITOR,

    FRAME_BM2CMP,

    FRAME_CALC,

    KIWAY_PLAYER_COUNT, // counts subset of FRAME_T's which are KIWAY_PLAYER derivatives

    // Kicad project manager is not a KIWAY_PLAYER
    KICAD_MAIN_FRAME_T = KIWAY_PLAYER_COUNT,

    FRAME_T_COUNT,

    PANEL_SYM_DISP_OPTIONS = FRAME_T_COUNT,
    PANEL_SYM_EDIT_GRIDS,
    PANEL_SYM_EDIT_OPTIONS,
    PANEL_SYM_COLORS,
    PANEL_SYM_TOOLBARS,

    PANEL_SCH_DISP_OPTIONS,
    PANEL_SCH_GRIDS,
    PANEL_SCH_EDIT_OPTIONS,
    PANEL_SCH_COLORS,
    PANEL_SCH_TOOLBARS,
    PANEL_SCH_FIELD_NAME_TEMPLATES,
    PANEL_SCH_SIMULATOR,

    PANEL_FP_DISPLAY_OPTIONS,
    PANEL_FP_GRIDS,
    PANEL_FP_EDIT_OPTIONS,
    PANEL_FP_COLORS,
    PANEL_FP_TOOLBARS,
    PANEL_FP_DEFAULT_FIELDS,
    PANEL_FP_DEFAULT_GRAPHICS_VALUES,
    PANEL_FP_ORIGINS_AXES,

    PANEL_PCB_DISPLAY_OPTS,
    PANEL_PCB_GRIDS,
    PANEL_PCB_EDIT_OPTIONS,
    PANEL_PCB_COLORS,
    PANEL_PCB_TOOLBARS,
    PANEL_PCB_ACTION_PLUGINS,
    PANEL_PCB_ORIGINS_AXES,

    PANEL_3DV_DISPLAY_OPTIONS,
    PANEL_3DV_OPENGL,
    PANEL_3DV_RAYTRACING,
    PANEL_3DV_TOOLBARS,

    PANEL_GBR_DISPLAY_OPTIONS,
    PANEL_GBR_EDIT_OPTIONS,
    PANEL_GBR_EXCELLON_OPTIONS,
    PANEL_GBR_GRIDS,
    PANEL_GBR_COLORS,
    PANEL_GBR_TOOLBARS,

    PANEL_DS_DISPLAY_OPTIONS,
    PANEL_DS_GRIDS,
    PANEL_DS_COLORS,
    PANEL_DS_TOOLBARS,

    // Library table dialogs are transient and are never returned
    DIALOG_CONFIGUREPATHS,
    DIALOG_DESIGN_BLOCK_LIBRARY_TABLE,
    DIALOG_SCH_LIBRARY_TABLE,
    DIALOG_PCB_LIBRARY_TABLE
};

//TEXT_EDITOR_FRAME_T,


#endif  // FRAME_T_H_
