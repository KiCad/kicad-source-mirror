/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file easypc_classes_root.h
 * @brief The design document  and the design parameter tables the importers use.
 */

#ifndef EASYPC_CLASSES_ROOT_H
#define EASYPC_CLASSES_ROOT_H

#include <cstdint>
#include <map>
#include <vector>

#include <io/easypc/easypc_classes_styles.h>


namespace EASYPC
{

/// A point read as two int32 values
struct POINT32
{
    int32_t X = 0;
    int32_t Y = 0;
};


POINT32 ReadPoint( ARCHIVE& aAr );


/// Load and drop an embedded member of a registered class; aDesignOwned marks its design as owner
LOAD_TASK SkipMember( ARCHIVE& aAr, const char* aClass, bool aDesignOwned = false );


/**
 * A member stored inside the design.  The design is its owner until its owner tag is read, so a version test
 * before that tag uses the header version.
 */
struct DESIGN_MEMBER : SOURCE_DESIGN_DATA
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override;

    bool DesignOwned = false;
    bool OwnerRead = false;
};


/// A map of DRC switch id to value
struct SOURCE_CHECK_PARAMETERS : DESIGN_MEMBER
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    bool IsChecking( int32_t aId ) const
    {
        auto it = Values.find( aId );
        return it != Values.end() && it->second != 0;
    }

    std::map<int32_t, int32_t> Values;
};


/// Indices into the design parameter table
enum DESIGN_PARAM_INDEX
{
    DP_MIN_ANNULAR_RING = 4,
    DP_MIN_PASTE_SIZE = 6,
    DP_MIN_TRACK_WIDTH = 7,
    DP_MIN_TESTPOINT_SIZE = 24,
    DP_MIN_TESTPOINT_SPACE = 25,
    DP_MIN_HOLE_SIZE = 27,
    DP_MIN_VIA_ANNULAR_RING = 28,
    DP_MIRROR_PAD_STYLE_EXCEPTIONS = 30,
    DP_MIN_VIA_HOLE_SIZE = 31,
    DP_VIA_SMD_PAD_SPACE = 35,
    DP_MIN_TEXT_SIZE = 37,
    DP_MIN_SOLDER_MASK_WIDTH = 38,
    DP_MIN_SOLDER_MASK_TO_TRACK = 39
};


struct SOURCE_DESIGN_PARAMETERS : DESIGN_MEMBER
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    /// The value at aIndex, or aDefault where the stored array is shorter, as the accessors do
    int32_t Param( int aIndex, int32_t aDefault = 0 ) const
    {
        return aIndex >= 0 && aIndex < static_cast<int>( Params.size() ) ? Params[aIndex] : aDefault;
    }

    std::vector<int32_t> Params;
};


struct SOURCE_DEFAULTS : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT*  TextStyle = nullptr;
    wxString NetClass;
};


/**
 * The document: OLE items, then the design.  A field an older version does not store keeps its default.  The
 * design is not in the load array; a null parent tag means it.
 */
struct SOURCE_DESIGN : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override { return Version; }

    int32_t Version = 0;
    int32_t Product = PRODUCT_EASYPC;

    OBJECT*          Areas = nullptr;
    OBJECT*          Boards = nullptr;
    OBJECT*          Coppers = nullptr;
    OBJECT*          Layers = nullptr;
    OBJECT*          NetClasses = nullptr;
    OBJECT*          Symbols = nullptr;
    OBJECT*          Nets = nullptr;
    OBJECT*          Pads = nullptr;
    OBJECT*          Texts = nullptr;
    OBJECT*          Spacings = nullptr; ///< legacy or current design spacings, depending on format
    OBJECT*          Attributes = nullptr;
    OBJECT*          ComponentInstances = nullptr;
    SOURCE_DEFAULTS* Defaults = nullptr;
    OBJECT*          Buses = nullptr;
    OBJECT*          SymbolInstances = nullptr;
    int32_t          IsolationGap = 0x9ec;
    int32_t          ThermalRelief = 0x9ec;
    bool             ThermalAngled = false;
    POINT32          Origin;
    OBJECT*          Groups = nullptr;
    bool             AdjustTextRotation = false;
    OBJECT*          ValuePositions = nullptr;

    SOURCE_DESIGN_PARAMETERS DesignParameters; ///< after 8000
    SOURCE_CHECK_PARAMETERS  DrcParameters;    ///< from 13000
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_ROOT_H
