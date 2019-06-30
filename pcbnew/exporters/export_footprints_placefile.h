/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EXPORT_FOOTPRINTS_PLACEFILE_H
#define EXPORT_FOOTPRINTS_PLACEFILE_H


#include <class_board.h>
#include <class_module.h>

/**
 * The ASCII format of the kicad place file is:
 *      ### Module positions - created on 04/12/2012 15:24:24 ###
 *      ### Printed by Pcbnew version pcbnew (2012-11-30 BZR 3828)-testing
 *      ## Unit = inches, Angle = deg.
 * or
 *      ## Unit = mm, Angle = deg.
 *      ## Side : top
 * or
 *      ## Side : bottom
 * or
 *      ## Side : all
 *      # Ref    Val              Package             PosX       PosY        Rot     Side
 *      C123     0,1uF/50V        SM0603              1.6024    -2.6280     180.0    Front
 *      C124     0,1uF/50V        SM0603              1.6063    -2.7579     180.0    Front
 *      C125     0,1uF/50V        SM0603              1.6010    -2.8310     180.0    Front
 *      ## End
 */


class PLACE_FILE_EXPORTER
{
public:

    /** Create a PLACE_FILE_EXPORTER
     * @param aBoard is the board
     * @param aUnitsMM is the unit option: true foo mm, false for inches
     * @param aForceSmdItems true to force not virtual and not flagged smd footprints
     * but having only smd pads to be in list
     * @param aTopSide true to generate top side info
     * @param aBottomSide true to generate bottom side info
     * @param aFormatCSV true to generate a csv format info, false to generate a ascii info
     */
    PLACE_FILE_EXPORTER( BOARD* aBoard,  bool aUnitsMM,
                         bool aForceSmdItems, bool aTopSide, bool aBottomSide, bool aFormatCSV );

    /**
     * build a string filled with the position data
     */
    std::string GenPositionData();

    /**
     * build a string filled with the pad report data
     * This report does not used options aForceSmdItems,aTopSide, aBottomSide
     * and aFormatCSV.
     * All footprints and their pads on board are reported.
     */
    std::string GenReportData();

    /** @return the footprint count found on board by GenPositionData()
     * must be called only after GenPositionData() is run
     */
    int GetFootprintCount() { return m_fpCount; }

    /**
     * @return the list of  not virtual footprints with MOD_CMS flag not set
     * but having only smd pads.
     * This list can be used to force this flag.
     * it is filled only if forceSmdItems is true
     */
    std::vector<MODULE*>& GetSmdFootprintsNotLabeledSMD()
    {
        return m_smdFootprintsNotLabeledSMD;
    }

    // Use standard board side name. do not translate them,
    // they are keywords in place file
    static std::string GetFrontSideName() { return std::string( "top" ); }
    static std::string GetBackSideName() { return std::string( "bottom" ); }

private:
    BOARD* m_board;
    bool m_unitsMM;         // true for mm, false for inches
    bool m_forceSmdItems;   // If true, non virtual fp with the flag MOD_CMD not set but
                            // having only smd pads will be in list
                            // and will be added in m_smdFootprintsNotLabeledSMD
    int m_side;             // PCB_BACK_SIDE, PCB_FRONT_SIDE, PCB_BOTH_SIDES
    bool m_formatCSV;       // true for csv format, false for ascii (utf8) format
    int m_fpCount;          // Number of footprints in list, for info
    wxPoint m_place_Offset; // Offset for coordinates in generated data.

    // A list of footprints with MOD_CMS flag not set but having only smd pads.
    // This list can be used to force this flag.
    std::vector<MODULE*> m_smdFootprintsNotLabeledSMD;
};

#endif      // #ifndef EXPORT_FOOTPRINTS_PLACEFILE_H
