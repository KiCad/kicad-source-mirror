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

#ifndef VIA_CROSS_SECTION_GEOMETRY_H
#define VIA_CROSS_SECTION_GEOMETRY_H

#include <optional>
#include <vector>
#include <padstack.h>

class BOARD_STACKUP;


/**
 * The shapes of a schematic via cross section, worked out from its protection and machining
 * settings with no drawing surface involved, so they can be checked without one.
 *
 * The drawing sits on a grid GRID_W wide and GridHeight() tall, y downwards, that the widget
 * scales to its own size. Layer positions and cut depths are to scale with the board thickness
 * and widths are relative to the hole. Copper and mask thickness are exaggerated.
 *
 * A value left unset in the state is mixed across a selection, and the shapes it produces are
 * marked mixed so they can be drawn hatched.
 */
class VIA_CROSS_SECTION_GEOMETRY
{
public:
    /// A cut at the mouth of the hole. Its width is relative to the hole and its depth to the
    /// board, so the layers it reaches are drawn truthfully.
    struct POST_MACHINING
    {
        std::optional<PAD_DRILL_POST_MACHINING_MODE> m_Mode = PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED;

        double m_WidthRatio = 2.0;    // cut width over hole diameter
        double m_DepthFraction = 0.1; // cut depth over board thickness
    };

    struct STATE
    {
        std::optional<bool> m_TentFront;
        std::optional<bool> m_TentBack;
        std::optional<bool> m_CoverFront;
        std::optional<bool> m_CoverBack;
        std::optional<bool> m_PlugFront;
        std::optional<bool> m_PlugBack;
        std::optional<bool> m_Filled;
        std::optional<bool> m_Capped;

        /// Depth of each copper layer centre and its thickness, as fractions of the board
        /// thickness, front to back. Empty means even spacing and a fixed copper thickness.
        std::vector<double> m_LayerDepths;
        std::vector<double> m_LayerThicknesses;
        int                 m_InnerLayers = 2;

        /// Copper layers the via spans, as ordinals where 0 is the front layer and the inner
        /// count plus one is the back layer. An unset bottom means the back layer.
        int                m_TopLayer = 0;
        std::optional<int> m_BottomLayer;

        /// Which spanned copper layers keep an annular ring. Empty means all of them.
        std::vector<bool> m_RingLayers;

        /// Pad diameter over hole diameter
        double m_PadRatio = 2.8;

        /// Copper layers removed from that side by the backdrill, 0 for no backdrill, and the
        /// backdrill diameter over the hole diameter
        std::optional<int> m_BackdrillFrontLayers = 0;
        std::optional<int> m_BackdrillBackLayers = 0;
        double             m_BackdrillFrontWidthRatio = 1.6;
        double             m_BackdrillBackWidthRatio = 1.6;

        POST_MACHINING m_PostMachiningFront;
        POST_MACHINING m_PostMachiningBack;
    };

    /// The materials, in the order they are painted. Each later body's outline lands on the
    /// ones before it.
    enum class MATERIAL
    {
        SOLDER_MASK,
        CORE,
        PLUG,
        COPPER,
        COVER
    };

    /// What is clipped out of a body. Air is never painted: a backdrill is clipped out of
    /// everything under the mask, and a post machining cut out of the mask and cover as well.
    enum class CLIP
    {
        UNDER_MASK, ///< bores and cuts
        OVER_MASK   ///< cuts only
    };

    struct POINT
    {
        double x;
        double y;
    };

    struct PIECE
    {
        /// A rectangle holds its top left and bottom right corners, a polygon its outline
        bool               m_Rect = true;
        std::vector<POINT> m_Points;
        bool               m_Mixed = false;

        double Left() const { return m_Points[0].x; }
        double Top() const { return m_Points[0].y; }
        double Right() const { return m_Points[1].x; }
        double Bottom() const { return m_Points[1].y; }
    };

    struct BODY
    {
        MATERIAL           m_Material;
        CLIP               m_Clip;
        std::vector<PIECE> m_Pieces;
    };

    static constexpr double GRID_W = 220.0;

    /// Grid height for a board with the given number of inner copper layers
    static double GridHeightFor( int aInnerLayers );

    /// Fill the layer depths and thicknesses from a stackup, or leave them empty when it does
    /// not match the copper layer count. Returns the board thickness in internal units.
    static int SetLayersFromStackup( STATE& aState, const BOARD_STACKUP& aStackup, int aCopperLayerCount );

    explicit VIA_CROSS_SECTION_GEOMETRY( const STATE& aState );

    double GridHeight() const { return m_gridHeight; }
    int    InnerLayers() const { return m_innerLayers; }

    /// Grid height of the centre of a copper layer, by ordinal
    double LayerCentre( int aOrdinal ) const;

    const std::vector<PIECE>& Bores() const { return m_bores; }
    const std::vector<PIECE>& Cuts() const { return m_cuts; }
    const std::vector<BODY>&  Bodies() const { return m_bodies; }

    const BODY& Body( MATERIAL aMaterial ) const;

private:
    void build( const STATE& aState );

    int                 m_innerLayers;
    double              m_gridHeight;
    std::vector<double> m_layerCentres;
    std::vector<PIECE>  m_bores;
    std::vector<PIECE>  m_cuts;
    std::vector<BODY>   m_bodies;
};

#endif
