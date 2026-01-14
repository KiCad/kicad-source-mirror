/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ZONE_FILLER_H
#define ZONE_FILLER_H

#include <map>
#include <mutex>
#include <vector>
#include <zone.h>
#include <geometry/shape_poly_set.h>

class PROGRESS_REPORTER;
class BOARD;
class COMMIT;
class SHAPE_LINE_CHAIN;


class ZONE_FILLER
{
public:
    ZONE_FILLER( BOARD* aBoard, COMMIT* aCommit );
    ~ZONE_FILLER();

    void SetProgressReporter( PROGRESS_REPORTER* aReporter );
    PROGRESS_REPORTER* GetProgressReporter() const { return m_progressReporter; }

    /**
     * Fills the given list of zones.
     *
     * NB: Invalidates connectivity - it is up to the caller to obtain a lock on the connectivity
     * data before calling Fill to prevent access to stale data by other coroutines (for example,
     * ratsnest redraw).  This will generally be required if a UI-based progress reporter has been
     * installed.
     *
     * Caller is also responsible for re-building connectivity afterwards.
     */
    bool Fill( const std::vector<ZONE*>& aZones, bool aCheck = false, wxWindow* aParent = nullptr );

    bool IsDebug() const { return m_debugZoneFiller; }

private:

    void addKnockout( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, int aGap, SHAPE_POLY_SET& aHoles );

    void addKnockout( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, int aGap, bool aIgnoreLineWidth,
                      SHAPE_POLY_SET& aHoles );

    void addHoleKnockout( PAD* aPad, int aGap, SHAPE_POLY_SET& aHoles );

    void knockoutThermalReliefs( const ZONE* aZone, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aFill,
                                 std::vector<BOARD_ITEM*>& aThermalConnectionPads,
                                 std::vector<PAD*>& aNoConnectionPads );

    void buildCopperItemClearances( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                    const std::vector<PAD*>& aNoConnectionPads,
                                    SHAPE_POLY_SET& aHoles,
                                    bool aIncludeZoneClearances = true );

    /**
     * Build clearance knockout holes for higher-priority zones on different nets.
     * Separated from buildCopperItemClearances to allow caching before zone knockouts.
     */
    void buildDifferentNetZoneClearances( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                          SHAPE_POLY_SET& aHoles );

    void subtractHigherPriorityZones( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                      SHAPE_POLY_SET& aRawFill );

    /**
     * Function fillCopperZone
     * Add non copper areas polygons (pads and tracks with clearance)
     * to a filled copper area
     * used in BuildFilledSolidAreasPolygons when calculating filled areas in a zone
     * Non copper areas are pads and track and their clearance area
     * The filled copper area must be computed before
     * BuildFilledSolidAreasPolygons() call this function just after creating the
     *  filled copper area polygon (without clearance areas
     * @param aPcb: the current board
     */
    bool fillCopperZone( const ZONE* aZone, PCB_LAYER_ID aLayer, PCB_LAYER_ID aDebugLayer,
                         const SHAPE_POLY_SET& aSmoothedOutline,
                         const SHAPE_POLY_SET& aMaxExtents, SHAPE_POLY_SET& aFillPolys );

    bool fillNonCopperZone( const ZONE* candidate, PCB_LAYER_ID aLayer,
                            const SHAPE_POLY_SET& aSmoothedOutline, SHAPE_POLY_SET& aFillPolys );
    /**
     * Function buildThermalSpokes
     * Constructs a list of all thermal spokes for the given zone.
     */
    void buildThermalSpokes( const ZONE* box, PCB_LAYER_ID aLayer,
                             const std::vector<BOARD_ITEM*>& aSpokedPadsList,
                             std::deque<SHAPE_LINE_CHAIN>& aSpokes );

    /**
     * Build thermal rings for pads in hatch zones.
     * For circular pads, creates an arc ring; for other shapes, creates an inflated ring.
     * Rings are clipped to the zone boundary.
     *
     * @param aThermalRings Output parameter to collect the thermal ring geometry. Used later
     *                      to drop hatch holes that would isolate the thermal relief.
     */
    void buildHatchZoneThermalRings( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                     const SHAPE_POLY_SET& aSmoothedOutline,
                                     const std::vector<BOARD_ITEM*>& aThermalConnectionPads,
                                     SHAPE_POLY_SET& aFillPolys,
                                     SHAPE_POLY_SET& aThermalRings );

    /**
     * Create strands of zero-width between elements of SHAPE_POLY_SET that are within
     * aDistance of each other.  When we inflate these strands, they will create minimum
     * width bands
     */
    void connect_nearby_polys( SHAPE_POLY_SET& aPolys, double aDistance );

    /**
     * Build the filled solid areas polygons from zone outlines (stored in m_Poly)
     * The solid areas can be more than one on copper layers, and do not have holes
     *  ( holes are linked by overlapping segments to the main outline)
     * in order to have drawable (and plottable) filled polygons.
     * @return true if OK, false if the solid polygons cannot be built
     * @param aZone is the zone to fill
     * @param aFillPolys: A reference to a SHAPE_POLY_SET buffer to store polygons with no holes
     * (holes are linked to main outline by overlapping segments, and these polygons are shrunk
     * by aZone->GetMinThickness() / 2 to be drawn with a outline thickness = aZone->GetMinThickness()
     * aFillPolys are polygons that will be drawn on screen and plotted
     */
    bool fillSingleZone( ZONE* aZone, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aFillPolys );

    /**
     * for zones having the ZONE_FILL_MODE::ZONE_FILL_MODE::HATCH_PATTERN, create a grid pattern
     * in filled areas of aZone, giving to the filled polygons a fill style like a grid
     * @param aZone is the zone to modify
     * @param aFillPolys: A reference to a SHAPE_POLY_SET buffer containing the initial
     * filled areas, and after adding the grid pattern, the modified filled areas with holes
     * @param aThermalRings: Thermal ring geometry used to drop hatch holes that would isolate
     *                       thermal reliefs from the zone fill.
     */
    bool addHatchFillTypeOnZone( const ZONE* aZone, PCB_LAYER_ID aLayer, PCB_LAYER_ID aDebugLayer,
                                 SHAPE_POLY_SET& aFillPolys,
                                 const SHAPE_POLY_SET& aThermalRings );

    /**
     * Refill a zone from cached pre-knockout fill.
     * Used during iterative refill to avoid recomputing thermal reliefs and copper clearances.
     * Only re-applies the higher-priority zone knockout with updated fills.
     */
    bool refillZoneFromCache( ZONE* aZone, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aFillPolys );

    BOARD*                m_board;
    SHAPE_POLY_SET        m_boardOutline;       // the board outlines, if exists
    bool                  m_brdOutlinesValid;   // true if m_boardOutline is well-formed
    COMMIT*               m_commit;
    PROGRESS_REPORTER*    m_progressReporter;

    int                   m_maxError;
    int                   m_worstClearance;

    bool                  m_debugZoneFiller;

    // Cache of pre-knockout fills for iterative refill optimization (issue 21746)
    // Key: (zone pointer, layer), Value: fill polygon before higher-priority zone knockout
    std::map<std::pair<const ZONE*, PCB_LAYER_ID>, SHAPE_POLY_SET> m_preKnockoutFillCache;
    mutable std::mutex                                             m_cacheMutex;
};

#endif
