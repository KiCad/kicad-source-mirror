/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Włostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __ZONE_FILLER_H
#define __ZONE_FILLER_H

#include <vector>
#include <class_zone.h>

class WX_PROGRESS_REPORTER;
class BOARD;
class COMMIT;
class SHAPE_POLY_SET;
class SHAPE_LINE_CHAIN;


class ZONE_FILLER
{
public:
    ZONE_FILLER( BOARD* aBoard, COMMIT* aCommit = nullptr );
    ~ZONE_FILLER();

    void InstallNewProgressReporter( wxWindow* aParent, const wxString& aTitle, int aNumPhases );
    bool Fill( const std::vector<ZONE_CONTAINER*>& aZones, bool aCheck = false );

private:

    void addKnockout( D_PAD* aPad, int aGap, SHAPE_POLY_SET& aHoles );

    void addKnockout( BOARD_ITEM* aItem, int aGap, bool aIgnoreLineWidth, SHAPE_POLY_SET& aHoles );

    void knockoutThermalReliefs( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aFill );

    void buildCopperItemClearances( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aHoles );

    /**
     * Function computeRawFilledArea
     * Add non copper areas polygons (pads and tracks with clearance)
     * to a filled copper area
     * used in BuildFilledSolidAreasPolygons when calculating filled areas in a zone
     * Non copper areas are pads and track and their clearance area
     * The filled copper area must be computed before
     * BuildFilledSolidAreasPolygons() call this function just after creating the
     *  filled copper area polygon (without clearance areas
     * @param aPcb: the current board
     */
    void computeRawFilledArea( const ZONE_CONTAINER* aZone,
                               const SHAPE_POLY_SET& aSmoothedOutline,
                               std::set<VECTOR2I>* aPreserveCorners,
                               SHAPE_POLY_SET& aRawPolys, SHAPE_POLY_SET& aFinalPolys );

    /**
     * Function buildThermalSpokes
     * Constructs a list of all thermal spokes for the given zone.
     */
    void buildThermalSpokes( const ZONE_CONTAINER* aZone, std::deque<SHAPE_LINE_CHAIN>& aSpokes );

    /**
     * Build the filled solid areas polygons from zone outlines (stored in m_Poly)
     * The solid areas can be more than one on copper layers, and do not have holes
     *  ( holes are linked by overlapping segments to the main outline)
     * in order to have drawable (and plottable) filled polygons.
     * @return true if OK, false if the solid polygons cannot be built
     * @param aZone is the zone to fill
     * @param aRawPolys: A reference to a SHAPE_POLY_SET buffer to store
     * filled solid areas polygons (with holes)
     * @param aFinalPolys: A reference to a SHAPE_POLY_SET buffer to store polygons with no holes
     * (holes are linked to main outline by overlapping segments, and these polygons are shrinked
     * by aZone->GetMinThickness() / 2 to be drawn with a outline thickness = aZone->GetMinThickness()
     * aFinalPolys are polygons that will be drawn on screen and plotted
     */
    bool fillSingleZone( ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys,
                         SHAPE_POLY_SET& aFinalPolys );

    /**
     * for zones having the ZONE_FILL_MODE::ZFM_HATCH_PATTERN, create a grid pattern
     * in filled areas of aZone, giving to the filled polygons a fill style like a grid
     * @param aZone is the zone to modify
     * @param aRawPolys: A reference to a SHAPE_POLY_SET buffer containing the initial
     * filled areas, and after adding the grid pattern, the modified filled areas with holes
     */
    void addHatchFillTypeOnZone( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys );

    BOARD* m_board;
    SHAPE_POLY_SET m_boardOutline;      // The board outlines, if exists
    bool m_brdOutlinesValid;            // true if m_boardOutline can be calculated
                                        // false if not (not closed outlines for instance)
    COMMIT* m_commit;
    WX_PROGRESS_REPORTER* m_progressReporter;
    std::unique_ptr<WX_PROGRESS_REPORTER> m_uniqueReporter;

    // m_high_def can be used to define a high definition arc to polygon approximation
    int m_high_def;

    // m_low_def can be used to define a low definition arc to polygon approximation
    // Used when converting some pad shapes that can accept lower resolution, vias and track ends.
    // Rect pads use m_low_def to reduce the number of segments. For these shapes a low def
    // gives a good shape, because the arc is small (90 degrees) and a small part of the shape.
    int m_low_def;
};

#endif
