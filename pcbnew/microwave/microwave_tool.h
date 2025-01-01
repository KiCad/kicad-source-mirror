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

#ifndef TOOLS_MICROWAVE_TOOL_H
#define TOOLS_MICROWAVE_TOOL_H

#include <tools/pcb_tool_base.h>

#include <tool/tool_menu.h>

// Microwave shapes that are created as board footprints when the user requests them.
enum class MICROWAVE_FOOTPRINT_SHAPE
{
    GAP,
    STUB,
    STUB_ARC,
    FUNCTION_SHAPE,
};

/**
 * Parameters for construction of a microwave inductor
 */
struct MICROWAVE_INDUCTOR_PATTERN
{
public:
    VECTOR2I m_Start;
    VECTOR2I m_End;
    int      m_Length;       // full length trace.
    int      m_Width;        // Trace width.
};


/**
 * Tool responsible for adding microwave features to PCBs
 */
class MICROWAVE_TOOL : public PCB_TOOL_BASE
{
public:
    MICROWAVE_TOOL();
    ~MICROWAVE_TOOL();

    ///< React to model/view changes
    void Reset( RESET_REASON aReason ) override;

    ///< Bind handlers to corresponding TOOL_ACTIONs
    void setTransitions() override;

private:
    ///< Main interactive tool
    int addMicrowaveFootprint( const TOOL_EVENT& aEvent );

    ///< Create an inductor between the two points
    void createInductorBetween( const VECTOR2I& aStart, const VECTOR2I& aEnd );

    ///< Draw a microwave inductor interactively
    int drawMicrowaveInductor( const TOOL_EVENT& aEvent );

    /**
     * Create a footprint "GAP" or "STUB" used in micro wave designs.
     *
     * This footprint has 2 pads:
     * PAD_ATTRIB::SMD, rectangular, H size = V size = current track width.
     * the "gap" is isolation created between this 2 pads
     *
     * @param aComponentShape is the component to create.
     * @return the new footprint.
     */
    FOOTPRINT* createFootprint( MICROWAVE_FOOTPRINT_SHAPE aFootprintShape );

    FOOTPRINT* createPolygonShape();

    /**
     * Create an S-shaped coil footprint for microwave applications.
     */
    FOOTPRINT* createMicrowaveInductor( MICROWAVE_INDUCTOR_PATTERN& aPattern,
                                        wxString& aErrorMessage );

    /**
     * Create a basic footprint for micro wave applications.
     *
     * The default pad settings are:
     *  PAD_ATTRIB::SMD, rectangular, H size = V size = current track width.
     *
     * @param aValue is the text value.
     * @param aTextSize is the size of ref and value texts ( <= 0 to use board default values ).
     * @param aPadCount is number of pads.
     * @return the new footprint.
     */
    FOOTPRINT* createBaseFootprint( const wxString& aValue, int aTextSize, int aPadCount );
};


#endif // TOOLS_MICROWAVE_TOOL_H
