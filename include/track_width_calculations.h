/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACK_WIDTH_CALCULATIONS_H
#define TRACK_WIDTH_CALCULATIONS_H

/**
 * Conductor current/temperature relationships based on the IPC-2152 data, fit to closed-form
 * equations by Douglas Brooks and Johannes Adam.
 *
 * The historical IPC-2221 charts (originally derived from 1956 NBS measurements) were superseded
 * by IPC-2152 in 2009.  IPC-2152 publishes only charts, not equations, so the equations fit to
 * that data by Brooks & Adam are used here.  The fit has the form
 *
 *     dT = K * I^a * W^b * Th^c
 *
 * where dT is the temperature rise in degrees C, I the current in amperes, W the track width in
 * mils and Th the track thickness in mils.  Solving for the current or the width gives the public
 * helpers below.
 *
 * The coefficients differ for external and internal layers.  IPC-2152 found internal traces cool
 * about as well as external ones, reversing the IPC-2221 assumption that internal traces run far
 * hotter, so the historical "derate internal by a factor of two" rule is not used.
 *
 * Source: D. G. Brooks and J. Adam, "Trace Currents and Temperatures Revisited," 2015 (fitting the
 * IPC-2152 data, Table 3-1).  The external coefficients apply to all copper weights; the internal
 * fits vary by copper weight and are selected from the nearest nominal weight using the supplied
 * thickness.
 */
namespace TRACK_WIDTH_CALCULATIONS
{
    /**
     * Compute the maximum current a track can carry for a given temperature rise.
     *
     * @param aWidthMils Track width in mils.
     * @param aThicknessMils Track (copper) thickness in mils.
     * @param aDeltaT_C Allowed temperature rise above ambient in degrees C.
     * @param aUseInternalLayer True for an internal layer, false for an external layer.
     * @return Maximum current in amperes.
     */
    double CurrentFromWidth( double aWidthMils, double aThicknessMils, double aDeltaT_C,
                             bool aUseInternalLayer );

    /**
     * Compute the track width required to carry a given current for a given temperature rise.
     *
     * @param aCurrentA Track current in amperes.
     * @param aThicknessMils Track (copper) thickness in mils.
     * @param aDeltaT_C Allowed temperature rise above ambient in degrees C.
     * @param aUseInternalLayer True for an internal layer, false for an external layer.
     * @return Required track width in mils.
     */
    double WidthFromCurrent( double aCurrentA, double aThicknessMils, double aDeltaT_C,
                             bool aUseInternalLayer );
}

#endif // TRACK_WIDTH_CALCULATIONS_H
