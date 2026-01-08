/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#ifndef EXPORTER_STEP_H
#define EXPORTER_STEP_H

#include <geometry/shape_poly_set.h>
#include <gal/color4d.h>
#include <jobs/job_export_pcb_3d.h>     // For EXPORTER_STEP_PARAMS
#include <layer_ids.h>
#include <lset.h>
#include <reporter.h>


class PCBMODEL;
class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class PCB_TRACK;
class FILENAME_RESOLVER;
class STEP_PCB_MODEL;


class EXPORTER_STEP
{
public:
    EXPORTER_STEP( BOARD* aBoard, const EXPORTER_STEP_PARAMS& aParams, REPORTER* aReporter );
    ~EXPORTER_STEP();

    bool Export();

    wxString m_outputFile;

private:
    bool buildBoard3DShapes();
    bool buildFootprint3DShapes( FOOTPRINT* aFootprint, const VECTOR2D& aOrigin, SHAPE_POLY_SET* aClipPolygon );
    bool buildTrack3DShape( PCB_TRACK* aTrack, const VECTOR2D& aOrigin );
    void buildZones3DShape( VECTOR2D aOrigin, bool aSolderMaskOnly = false );
    bool buildGraphic3DShape( BOARD_ITEM* aItem, const VECTOR2D& aOrigin );
    void initOutputVariant();

    /**
     * Check if a copper layer is within a backdrill layer span (inclusive).
     * @param aLayer The layer to check
     * @param aStartLayer The backdrill start layer
     * @param aEndLayer The backdrill end layer
     * @return true if the layer is within the span (inclusive)
     */
    bool isLayerInBackdrillSpan( PCB_LAYER_ID aLayer, PCB_LAYER_ID aStartLayer,
                                 PCB_LAYER_ID aEndLayer ) const;

    EXPORTER_STEP_PARAMS m_params;
    std::unique_ptr<FILENAME_RESOLVER> m_resolver;

private:
    REPORTER*       m_reporter;

    BOARD*          m_board;
    std::unique_ptr<STEP_PCB_MODEL> m_pcbModel;

    /// the name of the project (board short filename (no path, no ext)
    /// used to identify items in step file
    wxString        m_pcbBaseName;

    // wxString key is netname
    std::map<PCB_LAYER_ID, std::map<wxString, SHAPE_POLY_SET>> m_poly_shapes;
    std::map<PCB_LAYER_ID, SHAPE_POLY_SET>                     m_poly_holes;

    LSET m_layersToExport;

    KIGFX::COLOR4D m_copperColor;
    KIGFX::COLOR4D m_padColor;

    int m_platingThickness; // plating thickness for TH pads/vias
};

#endif
