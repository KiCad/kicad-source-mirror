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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <pad.h>
#include <geometry/shape_poly_set.h>

#include <memory>


/** A pad's resolved artwork on one fabrication layer */
class PAD_LAYER_GEOMETRY
{
public:
    bool                  IsEmpty() const { return m_empty; }
    const PAD&            Pad() const { return *m_pad; }
    VECTOR2I              Margin() const { return m_margin; }
    int                   CornerRadius() const { return m_cornerRadius; }
    const SHAPE_POLY_SET& Contour() const { return m_contour; }
    SHAPE_POLY_SET        Polygon() const;

private:
    friend PAD_LAYER_GEOMETRY ResolvePadLayer( const PAD&, PCB_LAYER_ID, int );
    PAD_LAYER_GEOMETRY( const PAD& aPad, PCB_LAYER_ID aLayer );

    std::unique_ptr<PAD> m_pad;
    SHAPE_POLY_SET       m_contour;
    PCB_LAYER_ID         m_layer;
    VECTOR2I             m_margin;
    bool                 m_empty = false;
    int                  m_maxError = 0;
    int                  m_cornerRadius = 0;
};


/** Per-side mask or paste margin as PlotStandardLayer applies it */
VECTOR2I ResolvePadMargin( const PAD& aPad, PCB_LAYER_ID aLayer );

/** Artwork on one layer as plotted by PlotStandardLayer; keep the plotter independent as the parity oracle */
PAD_LAYER_GEOMETRY ResolvePadLayer( const PAD& aPad, PCB_LAYER_ID aLayer, int aMaxError );
