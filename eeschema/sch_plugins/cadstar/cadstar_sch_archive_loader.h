/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_sch_archive_loader.h
 * @brief Loads a csa file into a KiCad SCHEMATIC object
 */

#ifndef CADSTAR_SCH_ARCHIVE_LOADER_H_
#define CADSTAR_SCH_ARCHIVE_LOADER_H_

#include <sch_plugins/cadstar/cadstar_sch_archive_parser.h>

#include <eda_text.h>
#include <schematic.h>

class CADSTAR_SCH_ARCHIVE_LOADER : public CADSTAR_SCH_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_SCH_ARCHIVE_LOADER( wxString aFilename )
            : CADSTAR_SCH_ARCHIVE_PARSER( aFilename )
    {
        mSchematic      = nullptr;
        mRootSheet      = nullptr;
        mDesignCenter.x = 0;
        mDesignCenter.y = 0;
    }


    ~CADSTAR_SCH_ARCHIVE_LOADER()
    {
    }

    /**
     * @brief Loads a CADSTAR PCB Archive file into the KiCad BOARD object given
     * @param aSchematic Schematic to add the design onto 
     * @param aRootSheet Root sheet to add the design onto 
     */
    void Load( ::SCHEMATIC* aSchematic, ::SCH_SHEET* aRootSheet );


private:
    ::SCHEMATIC* mSchematic;
    ::SCH_SHEET* mRootSheet;
    wxPoint      mDesignCenter;               ///< Used for calculating the required
                                              ///< offset to apply to the Cadstar design
                                              ///< so that it fits in KiCad canvas
    std::set<HATCHCODE_ID> mHatchcodesTested; ///< Used by checkAndLogHatchCode() to
                                              ///< avoid multiple duplicate warnings
    std::map<LAYER_ID, SCH_SHEET*> mSheetMap; ///< Map between Cadstar and KiCad

    void loadSheets();

    //Helper Functions for loading
    void loadSheetAndChildSheets( LAYER_ID aCadstarSheetID, wxPoint aPosition, wxSize aSheetSize,
            SCH_SHEET* aParentSheet );
    void loadChildSheets( LAYER_ID aCadstarSheetID );
    std::vector<LAYER_ID> findOrphanSheets();
    int                   getSheetNumber( LAYER_ID aCadstarSheetID );

    void checkAndLogHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID );

    //Helper Functions for obtaining CADSTAR elements in the parsed structures
    int       getLineThickness( const LINECODE_ID& aCadstarLineCodeID );
    HATCHCODE getHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID );
    PART      getPart( const PART_ID& aCadstarPartID );
    ROUTECODE getRouteCode( const ROUTECODE_ID& aCadstarRouteCodeID );
    TEXTCODE  getTextCode( const TEXTCODE_ID& aCadstarTextCodeID );
    wxString  getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID );
    wxString  getAttributeValue( const ATTRIBUTE_ID&        aCadstarAttributeID,
             const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>& aCadstarAttributeMap );

    // Helper Functions for obtaining individual elements as KiCad elements:
    double getHatchCodeAngleDegrees( const HATCHCODE_ID& aCadstarHatchcodeID );
    int    getKiCadHatchCodeThickness( const HATCHCODE_ID& aCadstarHatchcodeID );
    int    getKiCadHatchCodeGap( const HATCHCODE_ID& aCadstarHatchcodeID );


    void applyTextSettings( const TEXTCODE_ID& aCadstarTextCodeID,
            const ALIGNMENT& aCadstarAlignment, const JUSTIFICATION& aCadstarJustification,
            EDA_TEXT* aKiCadTextItem );

    std::pair<wxPoint, wxSize> getFigureExtentsKiCad( const FIGURE& aCadstarFigure );

    wxPoint getKiCadPoint( wxPoint aCadstarPoint );


    int getKiCadLength( long long aCadstarLength )
    {
        return aCadstarLength * KiCadUnitMultiplier;
    }

    /**
     * @brief 
     * @param aCadstarAngle 
     * @return 
    */
    double getAngleTenthDegree( const long long& aCadstarAngle )
    {
        return (double) aCadstarAngle / 100.0;
    }

    /**
     * @brief 
     * @param aCadstarAngle 
     * @return 
     */
    double getAngleDegrees( const long long& aCadstarAngle )
    {
        return (double) aCadstarAngle / 1000.0;
    }

    /**
     * @brief 
     * @param aPoint 
     * @return Angle in decidegrees of the polar representation of the point, scaled 0..360
     */
    double getPolarAngle( wxPoint aPoint );

}; // CADSTAR_SCH_ARCHIVE_LOADER


#endif // CADSTAR_SCH_ARCHIVE_LOADER_H_
