/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <pcb_io/pcb_io.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <io/pads/pads_unit_converter.h>
#include "pads_layer_mapper.h"

#include <map>
#include <string>
#include <vector>

class BOARD;
class PCB_SHAPE;
class SHAPE_ARC;
class SHAPE_LINE_CHAIN;

namespace PADS_IO
{
class PARSER;
struct ARC_POINT;
}

class PCB_IO_PADS : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    PCB_IO_PADS();
    ~PCB_IO_PADS() override;

    const IO_FILE_DESC GetBoardFileDesc() const override;
    const IO_FILE_DESC GetLibraryDesc() const override;
    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties, PROJECT* aProject ) override;

    /**
     * Return the automapped layers. Non-static because copper layer mapping depends on the
     * current board, so the callback is bound to a PCB_IO_PADS instance.
     */
    std::map<wxString, PCB_LAYER_ID> DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

private:
    int         scaleSize( double aVal ) const;
    int         scaleCoord( double aVal, bool aIsX ) const;

    /// Resolve a PADS decal/part UNITS letter to a nm-per-unit scale factor, or 0.0 to use
    /// the file's primary unit. The PADS letters are counterintuitive: `I` is Imperial
    /// (mils, not inches), `M` is Metric (mm, not mils).
    double      decalUnitScale( const std::string& aUnits ) const;
    PCB_LAYER_ID getMappedLayer( int aPadsLayer ) const;
    void        ensureNet( const std::string& aNetName );

    /// Append ARC_POINT vertices to a chain, expanding full-circle entries to 36-segment
    /// polygons.
    void appendArcPoints( SHAPE_LINE_CHAIN& aChain,
                          const std::vector<PADS_IO::ARC_POINT>& aPts );

    /// Configure a PCB_SHAPE as an arc from two consecutive PADS points, applying the
    /// Y-axis winding fix.
    void setPcbShapeArc( PCB_SHAPE* aShape, const PADS_IO::ARC_POINT& aPrev,
                         const PADS_IO::ARC_POINT& aCurr );

    /// Build a SHAPE_ARC from two consecutive PADS points. The midpoint is computed in PADS
    /// space before the Y-axis flip so the 3-point constructor gets the correct winding.
    SHAPE_ARC makeMidpointArc( const PADS_IO::ARC_POINT& aPrev,
                               const PADS_IO::ARC_POINT& aCurr, int aWidth );

    void        loadBoardSetup();
    void        loadNets();
    void        loadFootprints();
    void        loadReuseBlockGroups();
    void        loadTestPoints();
    void        loadTexts();
    void        loadTracksAndVias();
    void        loadCopperShapes();
    void        loadClusterGroups();
    void        loadZones();
    void        loadBoardOutline();
    void        loadDimensions();
    void        loadKeepouts();
    void        loadGraphicLines();
    void        generateDrcRules( const wxString& aFileName );
    void        reportStatistics();
    void        clearLoadingState();

    std::map<wxString, PCB_LAYER_ID>            m_layer_map;      ///< PADS layer names to KiCad layers

    // Valid only during LoadBoard, cleared by clearLoadingState().
    BOARD*                                      m_loadBoard = nullptr;
    const PADS_IO::PARSER*                      m_parser = nullptr;
    PADS_UNIT_CONVERTER                         m_unitConverter;
    PADS_LAYER_MAPPER                           m_layerMapper;
    std::vector<PADS_LAYER_INFO>                m_layerInfos;
    double                                      m_scaleFactor = 0.0;
    double                                      m_originX = 0.0;
    double                                      m_originY = 0.0;
    std::map<std::string, std::string>          m_pinToNetMap;
    std::map<std::string, std::string>          m_partToBlockMap;
    int                                         m_testPointIndex = 1;
    int                                         m_minObjectSize = 1000;
};
