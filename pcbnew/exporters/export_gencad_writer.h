/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

class BOARD;
class wxString;
struct aFile;

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

    /** Export a genCAD file
     * @param aFullFileName is the full filenam to create
     * @return true on success
     */
    bool WriteFile( const wxString& aFullFileName );

    /// Set the coordinates offet when exporting items
    void SetPlotOffet( VECTOR2I aOffset ) { GencadOffset = aOffset; }

    /// Flip pad shapes on the bottom side
    void FlipBottomPads( bool aFlip ) { m_flipBottomPads = aFlip; }

    /// Make pin names unique
    void UsePinNamesUnique( bool aUnique ) { m_useUniquePins = aUnique; }

    /// Make pad shapes unique
    void UseIndividualShapes( bool aUnique ) { m_useIndividualShapes = aUnique; }

    ///Store coord origin in genCAD file
    void StoreOriginCoordsInFile( bool aStore ) { m_storeOriginCoords = aStore; }

private:
    /// Creates the header section
    bool CreateHeaderInfoData();
    void CreateArtworksSection();
    void CreateTracksInfoData();
    void CreateBoardSection();
    void CreateComponentsSection();
    void CreateDevicesSection();
    void CreateRoutesSection();
    void CreateSignalsSection();
    void CreateShapesSection();
    void CreatePadsShapesSection();
    void FootprintWriteShape( FOOTPRINT* aFootprint, const wxString& aShapeName );
    const wxString getShapeName( FOOTPRINT* aFootprint );

    double MapXTo( int aX );
    double MapYTo( int aY );


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
    VECTOR2I GencadOffset;
};
