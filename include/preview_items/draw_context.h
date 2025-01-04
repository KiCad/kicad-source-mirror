/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef PREVIEW_PREVIEW_DRAW_CONTEXT__H_
#define PREVIEW_PREVIEW_DRAW_CONTEXT__H_

#include <gal/painter.h>
#include <math/vector2d.h>

namespace KIGFX
{
class GAL;
class VIEW;

namespace PREVIEW
{
/**
 * A KIGFX::PREVIEW::DRAW_CONTEXT is a wrapper around a GAL and some other
 * settings that makes it easy to draw preview items consistently.
 *
 * This class provides some graphical items that are often used by preview
 * items. Complex items can be composed from these.
 */
class DRAW_CONTEXT
{
public:
    DRAW_CONTEXT( KIGFX::VIEW& aView );

    /**
     * Draw a preview circle on the current layer.
     *
     * @param aOrigin circle origin.
     * @param aRad    circle radius.
     * @param aDeEmphasised draw the circle de-emphasized.
     */
    void DrawCircle( const VECTOR2I& aOrigin, double aRad, bool aDeEmphasised );

    /**
     * Draw a dashed preview circle on the current layer.
     *
     * @param aOrigin       circle origin.
     * @param aRad          circle radius.
     * @param aStepAngle    dash step angle.
     * @param aFillAngle    dash fill angle.
     * @param aDeEmphasised draw the circle de-emphasized.
     */
    void DrawCircleDashed( const VECTOR2I& aOrigin, double aRad, double aStepAngle,
                           double aFillAngle, bool aDeEmphasised );

    /**
     * Draw a simple line on the current layer.
     *
     * @param aStart        line start point.
     * @param aEnd          line end point.
     * @param aDeEmphasised draw the line de-.
     */
    void DrawLine( const VECTOR2I& aStart, const VECTOR2I& aEnd, bool aDeEmphasised );

    /**
     * Draw a dashed line on the current layer.
     *
     * @param aStart        line start point.
     * @param aEnd          line end point.
     * @param aDashStep     dash step distance.
     * @param aDashFill     dash fill distance.
     * @param aDeEmphasised draw the line de-emphasized.
     */
    void DrawLineDashed( const VECTOR2I& aStart, const VECTOR2I& aEn, int aDashStep,
                         int aDashFill, bool aDeEmphasised );

    /**
     * Draw a straight line on the current layer, with a special highlight when
     * the line angle is a multiple of 45 degrees.
     *
     * @param aStart        line start point.
     * @param aEnd          line end point.
     * @param aDeEmphasised draw the line de-emphasized.
     */
    void DrawLineWithAngleHighlight(
            const VECTOR2I& aStart, const VECTOR2I& aEnd, bool aDeEmphasised );

    /**
     * Draw an arc on the current layer, with a special highlight when
     * the line angle is a multiple of 45 degrees.
     *
     * @param aOrigin     the arc centre.
     * @param aRad        the arc radius.
     * @param aStartAngle the arc start angle.
     * @param aEndAngle   the arc end angle.
     */
    void DrawArcWithAngleHighlight( const VECTOR2I& aOrigin, double aRad, double aStartAngle,
                                    double aEndAngle );

private:
    /**
     * @return the colour to use for "special" angles.
     */
    COLOR4D getSpecialAngleColour() const;

    ///< The GAL to draw into
    KIGFX::GAL& m_gal;

    const KIGFX::RENDER_SETTINGS& m_render_settings;

    ///< The current layer to draw onto
    GAL_LAYER_ID m_currLayer;

    /// The line width to use for items
    float m_lineWidth;
};

} // namespace PREVIEW
} // namespace KIGFX

#endif // PREVIEW_PREVIEW_DRAW_CONTEXT__H_
