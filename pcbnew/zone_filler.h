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
#include <zone.h>

class WX_PROGRESS_REPORTER;
class BOARD;
class COMMIT;
class SHAPE_POLY_SET;
class SHAPE_LINE_CHAIN;


class ZONE_FILLER
{
public:
    ZONE_FILLER( BOARD* aBoard, COMMIT* aCommit );
    ~ZONE_FILLER();

    void SetProgressReporter( PROGRESS_REPORTER* aReporter );
    void InstallNewProgressReporter( wxWindow* aParent, const wxString& aTitle, int aNumPhases );

    /**
     * Fills the given list of zones.  Invalidates connectivity - it is up to the caller to obtain
     * a lock on the connectivity data before calling Fill to prevent access to stale data by other
     * coroutines (for example, ratsnest redraw).  This will generally be required if a UI-based
     * progress reporter has been installed.
     */
    bool Fill( std::vector<ZONE*>& aZones, bool aCheck = false, wxWindow* aParent = nullptr );

    bool IsDebug() const { return m_debugZoneFiller; }

private:

    void addKnockout( PAD* aPad, PCB_LAYER_ID aLayer, int aGap, SHAPE_POLY_SET& aHoles );

    void addKnockout( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, int aGap, bool aIgnoreLineWidth,
                      SHAPE_POLY_SET& aHoles );

    void addHoleKnockout( PAD* aPad, int aGap, SHAPE_POLY_SET& aHoles );

    void knockoutThermalReliefs( const ZONE* aZone, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aFill );

    void buildCopperItemClearances( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                    SHAPE_POLY_SET& aHoles );

    void subtractHigherPriorityZones( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                      SHAPE_POLY_SET& aRawFill );

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
    bool computeRawFilledArea( const ZONE* aZone, PCB_LAYER_ID aLayer, PCB_LAYER_ID aDebugLayer,
                               const SHAPE_POLY_SET& aSmoothedOutline,
                               const SHAPE_POLY_SET& aMaxExtents, SHAPE_POLY_SET& aRawPolys );

    /**
     * Function buildThermalSpokes
     * Constructs a list of all thermal spokes for the given zone.
     */
    void buildThermalSpokes( const ZONE* aZone, PCB_LAYER_ID aLayer,
                             std::deque<SHAPE_LINE_CHAIN>& aSpokes );

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
     * (holes are linked to main outline by overlapping segments, and these polygons are shrunk
     * by aZone->GetMinThickness() / 2 to be drawn with a outline thickness = aZone->GetMinThickness()
     * aFinalPolys are polygons that will be drawn on screen and plotted
     */
    bool fillSingleZone( ZONE* aZone, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aRawPolys,
                         SHAPE_POLY_SET& aFinalPolys );

    /**
     * for zones having the ZONE_FILL_MODE::ZONE_FILL_MODE::HATCH_PATTERN, create a grid pattern
     * in filled areas of aZone, giving to the filled polygons a fill style like a grid
     * @param aZone is the zone to modify
     * @param aRawPolys: A reference to a SHAPE_POLY_SET buffer containing the initial
     * filled areas, and after adding the grid pattern, the modified filled areas with holes
     */
    bool addHatchFillTypeOnZone( const ZONE* aZone, PCB_LAYER_ID aLayer, PCB_LAYER_ID aDebugLayer,
                                 SHAPE_POLY_SET& aRawPolys );

    BOARD*                m_board;
    SHAPE_POLY_SET        m_boardOutline;       // the board outlines, if exists
    bool                  m_brdOutlinesValid;   // true if m_boardOutline is well-formed
    COMMIT*               m_commit;
    PROGRESS_REPORTER*    m_progressReporter;

    std::unique_ptr<WX_PROGRESS_REPORTER> m_uniqueReporter;

    int                   m_maxError;
    int                   m_worstClearance;

    bool                  m_debugZoneFiller;
};

#endif
