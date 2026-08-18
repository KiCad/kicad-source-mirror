/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PADS_PCB_CONVERTER_H_
#define PADS_PCB_CONVERTER_H_

#include <map>
#include <string>
#include <vector>

#include <math/vector2d.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <io/pads/pads_unit_converter.h>
#include <zones.h>

#include "pads_layer_mapper.h"
#include "pads_parser.h"

class BOARD;
class REPORTER;
class SHAPE_ARC;
class SHAPE_LINE_CHAIN;
class ZONE;

/**
 * Builds KiCad board objects from the PADS_IO model structs.
 *
 * The ASCII and binary PADS readers fill the same model, so everything downstream of the
 * reader is shared: the unit and coordinate transform, the layer mapping, the net table and
 * the model-to-BOARD conversion itself. Each plugin owns one converter for the duration of
 * a LoadBoard call and drives it with the structs its parser produced.
 *
 * The transform is not usable until SetupLayers(), the units and SetOrigin() are configured,
 * because a PADS coordinate only becomes a KiCad position once the file's units and origin
 * are known.
 */
class PADS_PCB_CONVERTER
{
public:
    /**
     * @param aBoard receives every object the converter creates.
     * @param aReporter collects import warnings, and may be null.
     */
    PADS_PCB_CONVERTER( BOARD* aBoard, REPORTER* aReporter );

    BOARD* GetBoard() const { return m_board; }

    PADS_UNIT_CONVERTER& UnitConverter() { return m_unitConverter; }

    /**
     * Set the nanometers-per-file-unit factor used by the coordinate transform. This must
     * agree with the unit converter's own mode.
     */
    void SetScaleFactor( double aScaleFactor ) { m_scaleFactor = aScaleFactor; }

    double GetScaleFactor() const { return m_scaleFactor; }

    void SetOrigin( double aX, double aY );

    double GetOriginX() const { return m_originX; }

    double GetOriginY() const { return m_originY; }

    /**
     * Move the origin to the center of the board outline bounding box, which places the
     * imported board near the KiCad page origin when the file has no useful origin of its
     * own. Returns false, leaving the origin untouched, if the outlines are degenerate.
     */
    bool SetOriginFromOutlines( const std::vector<PADS_IO::POLYLINE>& aOutlines );

    int ScaleSize( double aVal ) const;

    int ScaleCoord( double aVal, bool aIsX ) const;

    VECTOR2I ScalePoint( double aX, double aY ) const;

    /**
     * Resolve the PADS layer table to KiCad layers and apply the copper layer count to the
     * board.
     *
     * @param aPadsLayers is the layer table as read from the file.
     * @param aPadsLayerCount is the file's copper layer count, which selects the bottom
     *        copper layer and seeds the layer mapper.
     * @param aMappingHandler is the plugin's layer remapping callback, which may be empty.
     * @param aResolveUnknownTypeByName parses the layer name, then falls back to a
     *        documentation layer, for layers whose function neither the file nor the layer
     *        number declares. Only the binary format leaves layers in that state.
     */
    void SetupLayers( const std::vector<PADS_IO::LAYER_INFO>& aPadsLayers, int aPadsLayerCount,
                      const LAYER_MAPPING_HANDLER& aMappingHandler, bool aResolveUnknownTypeByName );

    const std::vector<PADS_LAYER_INFO>& GetLayerInfos() const { return m_layerInfos; }

    PCB_LAYER_ID GetMappedLayer( int aPadsLayer ) const;

    /**
     * Build the board stackup from the PADS physical layer data, if that data carries real
     * thicknesses or dielectric constants. Requires SetupLayers() and the units to be
     * configured, since the layer thicknesses are scaled here.
     */
    void BuildStackup( const std::vector<PADS_IO::LAYER_INFO>& aPadsLayers );

    void EnsureNet( const std::string& aNetName );

    /**
     * Append PADS vertices to a chain, converting arc entries to polylines and expanding a
     * lone full-circle entry to a 36-segment polygon.
     */
    void AppendArcPoints( SHAPE_LINE_CHAIN& aChain, const std::vector<PADS_IO::ARC_POINT>& aPts ) const;

    /**
     * Build a SHAPE_ARC from two consecutive PADS points. The midpoint is computed in PADS
     * space before the Y-axis flip so the 3-point constructor gets the correct winding.
     */
    SHAPE_ARC MakeMidpointArc( const PADS_IO::ARC_POINT& aPrev, const PADS_IO::ARC_POINT& aCurr,
                               int aWidth ) const;

    void LoadTexts( const std::vector<PADS_IO::TEXT>& aTexts );

    void LoadKeepouts( const std::vector<PADS_IO::KEEPOUT>& aKeepouts );

    void LoadDimensions( const std::vector<PADS_IO::DIMENSION>& aDimensions );

    /**
     * Apply the cutout or fill settings of @p aPour to a zone whose outline the caller has
     * already built.
     *
     * @param aMaxPriority is the largest priority in the pour set, used to invert the PADS
     *        convention where the lowest number fills on top.
     */
    void ApplyPourSettings( ZONE* aZone, const PADS_IO::POUR& aPour, int aMaxPriority,
                            const PADS_IO::PARAMETERS& aParams );

    /**
     * Write a .kicad_dru beside @p aFileName carrying one clearance rule per differential
     * pair with a gap. No file is written when no pair yields a rule.
     */
    void WriteDiffPairRules( const wxString& aFileName,
                             const std::vector<PADS_IO::DIFF_PAIR_DEF>& aDiffPairs );

    void ReportStatistics();

private:
    BOARD*    m_board;
    REPORTER* m_reporter;

    PADS_UNIT_CONVERTER m_unitConverter;
    double              m_scaleFactor = 0.0;
    double              m_originX = 0.0;
    double              m_originY = 0.0;

    PADS_LAYER_MAPPER                m_layerMapper;
    std::vector<PADS_LAYER_INFO>     m_layerInfos;
    std::map<wxString, PCB_LAYER_ID> m_layerMap;
    int                              m_copperLayerCount = 2;
};

#endif // PADS_PCB_CONVERTER_H_
