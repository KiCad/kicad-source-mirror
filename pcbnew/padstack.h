/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_PADSTACK_H
#define KICAD_PADSTACK_H

#include <memory>
#include <optional>
#include <wx/string.h>

#include <api/serializable.h>
#include <zones.h>


/**
 * The set of pad shapes, used with PAD::{Set,Get}Shape()
 *
 * --> DO NOT REORDER, PCB_IO_KICAD_LEGACY is dependent on the integer values <--
 */
enum class PAD_SHAPE : int
{
    CIRCLE,
    RECTANGLE,      // do not use just RECT: it collides in a header on MSYS2
    OVAL,
    TRAPEZOID,
    ROUNDRECT,

    // Rectangle with a chamfered corner ( and with rounded other corners).
    CHAMFERED_RECT,
    CUSTOM            // A shape defined by user, using a set of basic shapes
                      // (thick segments, circles, arcs, polygons).
};

/**
 * The set of pad drill shapes, used with PAD::{Set,Get}DrillShape()
 */
enum class PAD_DRILL_SHAPE
{
    CIRCLE,
    OBLONG,
};

/**
 * The set of pad shapes, used with PAD::{Set,Get}Attribute().
 *
 * The double name is for convenience of Python devs
 */
enum class PAD_ATTRIB
{
    PTH,        ///< Plated through hole pad
    SMD,        ///< Smd pad, appears on the solder paste layer (default)
    CONN,       ///< Like smd, does not appear on the solder paste layer (default)
                ///<   Note: also has a special attribute in Gerber X files
                ///<   Used for edgecard connectors for instance
    NPTH,       ///< like PAD_PTH, but not plated
                ///<   mechanical use only, no connection allowed
};


/**
 * The set of pad properties used in Gerber files (Draw files, and P&P files)
 * to define some properties in fabrication or test files.  Also used by
 * DRC to check some properties.
 */
enum class PAD_PROP
{
    NONE,                  ///< no special fabrication property
    BGA,                   ///< Smd pad, used in BGA footprints
    FIDUCIAL_GLBL,         ///< a fiducial (usually a smd) for the full board
    FIDUCIAL_LOCAL,        ///< a fiducial (usually a smd) local to the parent footprint
    TESTPOINT,             ///< a test point pad
    HEATSINK,              ///< a pad used as heat sink, usually in SMD footprints
    CASTELLATED,           ///< a pad with a castellated through hole
    MECHANICAL,            ///< a pad used for mechanical support
};


class PADSTACK : public SERIALIZABLE
{
public:
    ///! Padstack type, mostly for IPC-7351 naming and attributes
    ///! Note that TYPE::MOUNTING is probably not currently supported by KiCad
    enum class TYPE
    {
        NORMAL,     ///< Padstack for a footprint pad
        VIA,        ///< Padstack for a via
        MOUNTING    ///< A mounting hole (plated or unplated, not associated with a footprint)
    };

    enum class MODE
    {
        NORMAL,           ///< Shape is the same on all layers
        TOP_INNER_BOTTOM, ///< Up to three shapes can be defined (top, inner, bottom)
        CUSTOM            ///< Shapes can be defined on arbitrary layers
    };

    struct OUTER_LAYER_PROPS
    {
        std::optional<int> solder_mask_margin;
        std::optional<int> solder_paste_margin;
        std::optional<double> solder_paste_margin_ratio;
        ZONE_CONNECTION zone_connection;
    };

    struct INNER_LAYER_PROPS
    {
        ZONE_CONNECTION zone_connection;
    };

public:
    PADSTACK();

    virtual ~PADSTACK() = default;

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    ///! Returns the name of this padstack in IPC-7351 format
    wxString Name() const;

private:

    ///! An override for the IPC-7351 padstack name
    wxString m_customName;
};


#endif //KICAD_PADSTACK_H
