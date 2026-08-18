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

#pragma once

#include <pcb_io/pcb_io.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <math/vector2d.h>
#include "pads_pcb_converter.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class BOARD;
class PCB_SHAPE;
class FOOTPRINT;
class PAD;
class EDA_ANGLE;

namespace PADS_IO
{
class BINARY_PARSER;
struct ARC_POINT;
struct PART_DECAL;
struct PAD_STACK_LAYER;
} // namespace PADS_IO

/**
 * PCB I/O plugin for importing the native PADS Layout binary .pcb format, distinct from
 * the ASCII format handled by PCB_IO_PADS. The binary parser populates the same PADS_IO
 * structs, which this wrapper converts to KiCad objects like the ASCII importer does.
 */
class PCB_IO_PADS_BINARY : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    PCB_IO_PADS_BINARY();
    ~PCB_IO_PADS_BINARY() override;

    const IO_FILE_DESC GetBoardFileDesc() const override;
    const IO_FILE_DESC GetLibraryDesc() const override;
    long long          GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool CanReadBoard( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe, const std::map<std::string, UTF8>* aProperties,
                      PROJECT* aProject ) override;

    std::map<wxString, PCB_LAYER_ID>
    DefaultLayerMappingCallback( const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

private:
    /// Shorthands for the converter's transform, which the loaders below use everywhere.
    int scaleSize( double aVal ) const { return m_converter->ScaleSize( aVal ); }
    int scaleCoord( double aVal, bool aIsX ) const { return m_converter->ScaleCoord( aVal, aIsX ); }

    /// Scale a board point to a KiCad position, applying the per-axis origin and the Y-flip.
    VECTOR2I     scalePoint( double aX, double aY ) const { return m_converter->ScalePoint( aX, aY ); }
    PCB_LAYER_ID getMappedLayer( int aPadsLayer ) const { return m_converter->GetMappedLayer( aPadsLayer ); }

    void loadBoardSetup();
    void loadNets();
    void loadFootprints();

    /// Build one terminal's pad on @p aFootprint from decal terminal @p aTermIdx, oriented by
    /// the part orientation @p aPartOrient.
    void buildPad( FOOTPRINT* aFootprint, const PADS_IO::PART_DECAL& aDecal, size_t aTermIdx,
                   const EDA_ANGLE& aPartOrient );

    /// Apply a pad-stack layer's shape, size and finger offset to @p aPad.
    void applyPadShape( PAD* aPad, const PADS_IO::PAD_STACK_LAYER& aLayer, PCB_LAYER_ID aKiCadLayer );

    /// Build one PCB_GROUP per PADS part cluster and add each footprint to the group named
    /// by its part's CLSTID. Must run after loadFootprints().
    void loadClusterGroups();

    void loadBoardOutline();
    void loadGraphicLines();
    void setBoardOutlineArc( PCB_SHAPE* aShape, const PADS_IO::ARC_POINT& aPrev, const PADS_IO::ARC_POINT& aCurr );
    void loadTracksAndVias();
    void loadCopperShapes();
    void loadZones();
    void clearLoadingState();

    // Valid only during LoadBoard, cleared by clearLoadingState().
    BOARD*                              m_loadBoard = nullptr;
    const PADS_IO::BINARY_PARSER*       m_parser = nullptr;
    std::unique_ptr<PADS_PCB_CONVERTER> m_converter;
    int                                 m_minObjectSize = 1000;
    std::map<std::string, std::string>  m_pinToNetMap;

    // One footprint per parser part index, so loadClusterGroups() can resolve the
    // part-index-keyed cluster membership to a footprint.
    std::vector<FOOTPRINT*> m_partFootprints;
};
