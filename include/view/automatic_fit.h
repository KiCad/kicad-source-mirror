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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <math/box2.h>
#include <math/vector2d.h>

#include <optional>


namespace KIGFX
{

/**
 * What the last zoom to fit showed, so a canvas that changes size can fit it again.
 *
 * The view counts as still at its automatic fit while its scale and centre are exactly those the
 * fit left. Any zoom or pan, by the user or by a tool, changes one of them and so ends it without
 * having to be reported here.
 */
class AUTOMATIC_FIT
{
public:
    /// Note a fit of @a aBox that left the view at @a aScale and @a aCenter
    void Record( const BOX2I& aBox, std::optional<double> aMarginScale, double aScale, const VECTOR2D& aCenter )
    {
        m_valid = true;
        m_box = aBox;
        m_marginScale = aMarginScale;
        m_scale = aScale;
        m_center = aCenter;
    }

    void Clear() { m_valid = false; }

    /// True while a view at @a aScale and @a aCenter shows exactly what the last fit left
    bool Holds( double aScale, const VECTOR2D& aCenter ) const
    {
        return m_valid && aScale == m_scale && aCenter == m_center;
    }

    const BOX2I& GetBox() const { return m_box; }

    std::optional<double> GetMarginScale() const { return m_marginScale; }

private:
    bool                  m_valid = false;
    BOX2I                 m_box;
    std::optional<double> m_marginScale;
    double                m_scale = 0.0;
    VECTOR2D              m_center;
};

} // namespace KIGFX
