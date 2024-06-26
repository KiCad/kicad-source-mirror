/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/shape_poly_set.h>
#include <gal/color4d.h>
#include <layer_ids.h>

// Default value to chain 2 shapes when creating the board outlines
// from shapes on Edges.Cut layer
#define BOARD_DEFAULT_CHAINING_EPSILON 0.01

class PCBMODEL;
class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class PCB_TRACK;
class FILENAME_RESOLVER;
class STEP_PCB_MODEL;

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
            m_exportBoardBody( true ),
            m_exportComponents( true ),
            m_exportTracksVias( false ),
            m_exportPads( false ),
            m_exportZones( false ),
            m_exportInnerCopper( false ),
            m_exportSilkscreen( false ),
            m_exportSoldermask( false ),
            m_fuseShapes( false ),
            m_optimizeStep( true ),
            m_format( FORMAT::STEP )
    {};

    enum class FORMAT
    {
        STEP,
        BREP,
        XAO,
        GLB
    };

    wxString m_outputFile;
    wxString m_netFilter;

    VECTOR2D m_origin;

    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_includeUnspecified;
    bool     m_includeDNP;
    bool     m_substModels;
    double   m_BoardOutlinesChainingEpsilon;
    bool     m_boardOnly;
    bool     m_exportBoardBody;
    bool     m_exportComponents;
    bool     m_exportTracksVias;
    bool     m_exportPads;
    bool     m_exportZones;
    bool     m_exportInnerCopper;
    bool     m_exportSilkscreen;
    bool     m_exportSoldermask;
    bool     m_fuseShapes;
    bool     m_optimizeStep;
    FORMAT   m_format;

    wxString GetDefaultExportExtension();
    wxString GetFormatName();
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

private:
    bool buildBoard3DShapes();
    bool buildFootprint3DShapes( FOOTPRINT* aFootprint, VECTOR2D aOrigin );
    bool buildTrack3DShape( PCB_TRACK* aTrack, VECTOR2D aOrigin );
    void buildZones3DShape( VECTOR2D aOrigin );
    bool buildGraphic3DShape( BOARD_ITEM* aItem, VECTOR2D aOrigin );

    EXPORTER_STEP_PARAMS m_params;
    std::unique_ptr<FILENAME_RESOLVER> m_resolver;

private:
    bool            m_error;
    bool            m_fail;
    bool            m_warn;

    BOARD*          m_board;
    std::unique_ptr<STEP_PCB_MODEL> m_pcbModel;

    /// the name of the project (board short filename (no path, no ext)
    /// used to identify items in step file
    wxString        m_pcbBaseName;

    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> m_poly_shapes;
    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> m_poly_holes;

    LSET m_layersToExport;

    KIGFX::COLOR4D m_copperColor;
    KIGFX::COLOR4D m_padColor;

    int m_platingThickness; // plating thickness for TH pads/vias
};

#endif
