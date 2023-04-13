/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EXPORTER_STEP_H
#define EXPORTER_STEP_H

#include "step_pcb_model.h"
#include <geometry/shape_poly_set.h>
#include <gal/color4d.h>

// Default value to chain 2 shapes when creating the board outlines
// from shapes on Edges.Cut layer
#define BOARD_DEFAULT_CHAINING_EPSILON 0.01

class PCBMODEL;
class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class PCB_TRACK;
class FILENAME_RESOLVER;

class EXPORTER_STEP_PARAMS
{
public:
    EXPORTER_STEP_PARAMS() :
            m_origin(),
            m_overwrite( false ),
            m_useGridOrigin( false ),
            m_useDrillOrigin( false ),
            m_includeUnspecified( true ),
            m_includeDNP( true ),
            m_substModels( true ),
            m_BoardOutlinesChainingEpsilon( BOARD_DEFAULT_CHAINING_EPSILON ),
            m_boardOnly( false ),
            m_exportTracks( false )
    {};

    wxString m_outputFile;

    VECTOR2D m_origin;

    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_includeUnspecified;
    bool     m_includeDNP;
    bool     m_substModels;
    double   m_BoardOutlinesChainingEpsilon;
    bool     m_boardOnly;
    bool     m_exportTracks;
};

class EXPORTER_STEP
{
public:
    EXPORTER_STEP( BOARD* aBoard, const EXPORTER_STEP_PARAMS& aParams );
    ~EXPORTER_STEP();

    bool Export();

    wxString m_outputFile;

    void SetError() { m_error = true; }
    void SetFail() { m_fail = true; }
    void SetWarn() { m_warn = true; }

    /// Return rue to export tracks and vias on top and bottom copper layers
    bool ExportTracksAndVias() { return m_params.m_exportTracks; }

private:
    bool buildBoard3DShapes();
    bool buildFootprint3DShapes( FOOTPRINT* aFootprint, VECTOR2D aOrigin );
    bool buildTrack3DShape( PCB_TRACK* aTrack, VECTOR2D aOrigin );
    bool buildGraphic3DShape( BOARD_ITEM* aItem, VECTOR2D aOrigin );
    void calculatePcbThickness();

    EXPORTER_STEP_PARAMS m_params;
    std::unique_ptr<FILENAME_RESOLVER> m_resolver;

    bool m_error;
    bool m_fail;
    bool m_warn;

    bool            m_hasDrillOrigin;
    bool            m_hasGridOrigin;
    BOARD*          m_board;
    std::unique_ptr<STEP_PCB_MODEL> m_pcbModel;

    /// the name of the project (board short filename (no path, no ext)
    /// used to identify items in step file
    wxString        m_pcbBaseName;

    double m_boardThickness;

    SHAPE_POLY_SET m_top_copper_shapes;
    SHAPE_POLY_SET m_bottom_copper_shapes;

    KIGFX::COLOR4D m_solderMaskColor;
    KIGFX::COLOR4D m_copperColor;
};

#endif
