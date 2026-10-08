/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/string.h>
#include <lset.h>
#include <layer_ids.h>

class BOARD;
class BOARD_ITEM;
class FOOTPRINT;
class OUTPUTFORMATTER;
class PCB_DIMENSION_BASE;
class PCB_REFERENCE_IMAGE;
class PCB_GROUP;
class PCB_SHAPE;
class PCB_TARGET;
class PCB_POINT;
class PAD;
class PCB_BARCODE;
class PCB_TEXT;
class PCB_TEXTBOX;
class PCB_TABLE;
class PCB_GENERATOR;
class PCB_TRACK;
class ZONE;
class TEARDROP_PARAMETERS;
class SHAPE_LINE_CHAIN;
class EDA_TEXT;
struct ZONE_LAYER_PROPERTIES;

/// Writes a board or footprint in the KiCad 10.0 file format. Extracted from the 10.0.0 release,
/// so it can only emit what that release can read back.
class PCB_WRITER_V10
{
public:
    static constexpr int FORMAT_VERSION = 20260206;

    PCB_WRITER_V10() :
            m_board( nullptr ),
            m_out( nullptr ),
            m_ctl( 0 )
    {
    }

    void SaveBoard( const wxString& aFileName, BOARD* aBoard );

    void SaveFootprintFile( const wxString& aFileName, FOOTPRINT* aFootprint );

private:
    void FormatBoardToFormatter( OUTPUTFORMATTER* aOut, BOARD* aBoard );

    void Format( const BOARD_ITEM* aItem ) const;

    void formatSetup( const BOARD* aBoard ) const;

    void formatGeneral( const BOARD* aBoard ) const;

    void formatBoardLayers( const BOARD* aBoard ) const;

    void formatProperties( const BOARD* aBoard ) const;

    void formatVariants( const BOARD* aBoard ) const;

    void formatHeader( const BOARD* aBoard ) const;

    void formatTeardropParameters( const TEARDROP_PARAMETERS& tdParams ) const;

    void format( const BOARD* aBoard ) const;

    void format( const PCB_DIMENSION_BASE* aDimension ) const;

    void format( const PCB_REFERENCE_IMAGE* aBitmap ) const;

    void format( const PCB_GROUP* aGroup ) const;

    void format( const PCB_SHAPE* aSegment ) const;

    void format( const PCB_TARGET* aTarget ) const;

    void format( const PCB_POINT* aPoint ) const;

    void format( const FOOTPRINT* aFootprint ) const;

    void format( const PAD* aPad ) const;

    void format( const PCB_BARCODE* aBarcode ) const;

    void format( const PCB_TEXT* aText ) const;

    void format( const PCB_TEXTBOX* aTextBox ) const;

    void format( const PCB_TABLE* aTable ) const;

    void format( const PCB_GENERATOR* aGenerator ) const;

    void format( const PCB_TRACK* aTrack ) const;

    void format( const ZONE* aZone ) const;

    void format( const ZONE_LAYER_PROPERTIES& aZoneLayerProperties, int aNestLevel, PCB_LAYER_ID aLayer ) const;

    void formatPolyPts( const SHAPE_LINE_CHAIN& aOutline, const FOOTPRINT* aParentFP = nullptr ) const;

    void formatRenderCache( const EDA_TEXT* aText ) const;

    void formatLayer( PCB_LAYER_ID aLayer, bool aIsKnockout = false ) const;

    void formatLayers( LSET aLayerMask, bool aEnumerateLayers, bool aIsZone = false ) const;

    BOARD*           m_board;
    OUTPUTFORMATTER* m_out;
    int              m_ctl;
};
