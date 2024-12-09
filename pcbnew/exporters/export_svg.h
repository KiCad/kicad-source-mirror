/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCB_PLOT_SVG_H
#define PCB_PLOT_SVG_H

struct PCB_PLOT_SVG_OPTIONS
{
    wxString m_outputFile;
    wxString m_colorTheme;

    bool m_mirror;
    bool m_blackAndWhite;
    bool m_plotFrame;
    bool m_negative;

    int m_pageSizeMode;

    LSEQ m_printMaskLayer;
    bool m_sketchPadsOnFabLayers;
    bool m_hideDNPFPsOnFabLayers;
    bool m_sketchDNPFPsOnFabLayers;
    bool m_crossoutDNPFPsOnFabLayers;

    // How holes in pads/vias are plotted:
    // 0 = no hole, 1 = small shape, 2 = actual shape
    // Not used in some plotters (Gerber)
    int m_drillShapeOption;

    // coord format: 4 digits in mantissa (units always in mm). This is a good choice.
    unsigned int m_precision = 4;
};

class EXPORT_SVG
{
public:
    static bool Plot( BOARD* aBoard, const PCB_PLOT_SVG_OPTIONS& aSvgPlotOptions );
};

#endif