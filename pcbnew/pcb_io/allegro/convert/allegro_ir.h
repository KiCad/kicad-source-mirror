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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <optional>
#include <string>

#include <math/vector2d.h>

#include <convert/allegro_db.h>


namespace ALLEGRO
{

/**
 * The IR namespace contiains "intermediate representations"
 * of Allegro database objects.
 *
 * These are slightly higher-level than the binary format and map
 * more directly to the data you see from extracta.
 *
 * The data contained in them is a combination of fields assumed to exist
 * from presence in Altium and Fabmaster-type ASCII outputs, and data
 * found in the .brd files.
 *
 * These structs have no specific binary layout or ordering.
 */
namespace IR
{
    enum class UNITS
    {
        MIL,
        MM,
    };

    // A!BOARD_NAME!BOARD_UNITS!BOARD_EXTENTS_X1!BOARD_EXTENTS_Y1!BOARD_EXTENTS_X2!BOARD_EXTENTS_Y2!BOARD_THICKNESS!
    struct BOARD : public DB_OBJ
    {
        UNITS    m_Units;
        VECTOR2I m_Extents;
        int      m_Thickness;
    };

    enum class LAYER_SUBCLASS
    {
        UNSPECIFIED,
        TOP,
        BOTTOM,
    };

    enum class ARTWORK_POLARITY
    {
        POSITIVE,
        NEGATIVE,
    };

    // A!LAYER_SORT!LAYER_SUBCLASS!LAYER_ARTWORK!LAYER_USE!LAYER_CONDUCTOR!LAYER_DIELECTRIC_CONSTANT!LAYER_ELECTRICAL_CONDUCTIVITY!LAYER_MATERIAL!LAYER_THERMAL_CONDUCTIVITY!LAYER_THICKNESS!
    struct LAYER
    {
        int              m_SortIndex;
        LAYER_SUBCLASS   m_Subclass;
        ARTWORK_POLARITY m_ArtworkPolarity;
        // LAYER_USE m_LayerUse;
        bool                 m_IsConductor;
        float                m_DielectricConst;
        float                m_Conductivity; // in mho/cm
        std::string          m_Material;
        std::optional<float> m_ThermalConductivity;
        float                m_Thickness;
    };

} // namespace IR
} // namespace ALLEGRO
