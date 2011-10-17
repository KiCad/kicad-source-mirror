/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file gerbview/pcbplot.h
 */

#ifndef PCBPLOT_H
#define PCBPLOT_H

/* Shared Config keys for plot and print */
#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )
#define OPTKEY_PRINT_X_FINESCALE_ADJ wxT( "PrintXFineScaleAdj" )
#define OPTKEY_PRINT_Y_FINESCALE_ADJ wxT( "PrintYFineScaleAdj" )
#define OPTKEY_PRINT_SCALE           wxT( "PrintScale" )
#define OPTKEY_PRINT_PAGE_FRAME      wxT( "PrintPageFrame" )
#define OPTKEY_PRINT_MONOCHROME_MODE wxT( "PrintMonochrome" )

/* Plot Options : */
struct PCB_Plot_Options
{
    bool Exclude_Edges_Pcb;
    int PlotLine_Width;
    bool Plot_Frame_Ref;       // True to plot/print frame references
    int Plot_Mode;
    bool Plot_Set_MIROIR;
    bool Sel_Rotate_Window;
    int HPGL_Pen_Num;
    int HPGL_Pen_Speed;
    int HPGL_Pen_Diam;
    int HPGL_Pen_Recouvrement;
    bool HPGL_Org_Centre;      // true if, HPGL originally the center of the node
    int PlotPSColorOpt;        // True for color Postscript output
    bool Plot_PS_Negative;     // True to create a negative board ps plot

    /* id for plot format (see enum PlotFormat in plot_common.h) */
    int PlotFormat;
    int PlotOrient;
    int PlotScaleOpt;
    int DrillShapeOpt;
    double         Scale;
    double         ScaleAdjX;
    double         ScaleAdjY;
};
extern PCB_Plot_Options g_pcb_plot_options;

#endif  // ifndef PCBPLOT_H
