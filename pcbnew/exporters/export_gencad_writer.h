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

#include <stdio.h> // for FILE

#include <wx/string.h>

#include <math/vector2d.h>

class BOARD;
class FOOTPRINT;

/**
 * Export board to GenCAD file format.
 *
 * @note This exporter **only** supports GenCAD version 1.4.
 */
class GENCAD_EXPORTER
{
public:
    GENCAD_EXPORTER( BOARD* aBoard ):
        m_board( aBoard ),
        m_file( nullptr ),
        m_flipBottomPads( false ),
        m_useUniquePins( false ),
        m_useIndividualShapes( false ),
        m_storeOriginCoords( false )
    {
    }

    /**
     * Export a GenCAD file.
     *
     * @param aFullFileName is the full filename to create.
     * @return true on success
     */
    bool WriteFile( const wxString& aFullFileName );

    /// Set the coordinates offset when exporting items.
    void SetPlotOffet( VECTOR2I aOffset ) { m_gencadOffset = aOffset; }

    /// Flip pad shapes on the bottom side.
    void FlipBottomPads( bool aFlip ) { m_flipBottomPads = aFlip; }

    /// Make pin names unique.
    void UsePinNamesUnique( bool aUnique ) { m_useUniquePins = aUnique; }

    /// Make pad shapes unique.
    void UseIndividualShapes( bool aUnique ) { m_useIndividualShapes = aUnique; }

    /// Store origin coordinate in GenCAD file.
    void StoreOriginCoordsInFile( bool aStore ) { m_storeOriginCoords = aStore; }

private:
    /// Creates the header section
    bool createHeaderInfoData();
    void createArtworksSection();

    /**
     * Create the "$TRACKS" section.
     *
     *  This section gives the list of widths (tools) used in tracks and vias
     *
     *  Each track name is build using "TRACK" + track width.
     *  For instance for a width = 120 : name = "TRACK120".
     */

    void createTracksInfoData();
    void createBoardSection();

    /**
     * Create the $COMPONENTS GenCAD section.
     *
     * GenCAD $COMPONENTS are the footprint placements.  Bottom side components are difficult to handle
     * because shapes must be mirrored or flipped.  Silk screen layers need to be handled correctly and
     * so on. Also it seems that *no one* follows the specs...
     */
    void createComponentsSection();

    /**
     * Create the $DEVICES section.
     *
     * This is a list of footprints properties. Footprint shapes are in $SHAPES section.
     */

    void createDevicesSection();

    /**
     * Create the $ROUTES section.
     *
     * This section handles tracks and vias
     *
     * @todo Add zones to GenCAD output.
     */
    void createRoutesSection();
    void createSignalsSection();


    /**
     * Create the footprint shape list.
     *
     * Since footprint shape is customizable after the placement we cannot share them.
     * Instead we opt for the one footprint one shape one component one device approach.
     */
    void createShapesSection();
    void createPadsShapesSection();

    /**
     * Create the shape of a footprint (SHAPE section)
     *
     *  The shape is always given "normal" orientation.  It's almost guaranteed that the silk
     * layer will be imported wrong but the shape also contains the pads.
     */
    void footprintWriteShape( FOOTPRINT* aFootprint, const wxString& aShapeName );
    const wxString getShapeName( FOOTPRINT* aFootprint );

    /**
     * Helper functions to calculate coordinates of footprints in GenCAD values.
     *
     * The GenCAD Y axis from bottom to top,
     */
    double mapXTo( int aX );
    double mapYTo( int aY );


private:
    BOARD* m_board;
    wxString m_fullFileName;
    FILE* m_file;

    // Export options
    bool m_flipBottomPads;
    bool m_useUniquePins;
    bool m_useIndividualShapes;
    bool m_storeOriginCoords;

    // These are the export origin (the auxiliary axis)
    VECTOR2I m_gencadOffset;
};
