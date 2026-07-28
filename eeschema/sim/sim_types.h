/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <cmath>
#include <limits>

///< Possible simulation types
enum SIM_TYPE
{
    ST_UNKNOWN,
    ST_AC,
    ST_DC,
    ST_DISTO,
    ST_NOISE,
    ST_OP,
    ST_PZ,
    ST_SENS,
    ST_TF,
    ST_TRAN,
    ST_SP,
    ST_FFT,
    ST_LAST
};

///< Possible trace types
enum SIM_TRACE_TYPE
{
    // Y axis
    SPT_VOLTAGE       = 0x0001,
    SPT_CURRENT       = 0x0002,
    SPT_AC_PHASE      = 0x0004,
    SPT_AC_GAIN       = 0x0008,
    SPT_POWER         = 0x0010,
    SPT_SP_AMP        = 0x0020,
    SPT_SP_SMITH      = 0x0040,
    SPT_SP_MASK       = SPT_SP_AMP | SPT_AC_PHASE | SPT_SP_SMITH,
    SPT_Y_AXIS_MASK   = 0x00FF,

    // X axis
    SPT_TIME          = 0x0100,
    SPT_LIN_FREQUENCY = 0x0200,
    SPT_LOG_FREQUENCY = 0x0400,
    SPT_SWEEP         = 0x0800,
    SPT_X_AXIS_MASK   = 0xFF00,

    SPT_UNKNOWN       = 0x0000
};


/**
 * Convert a linear magnitude to decibels.
 *
 * A node carrying no signal is a routine AC result, not an error, and its zero magnitude has no
 * logarithm.  Reporting NaN lets consumers leave a gap rather than draw the node at some finite
 * level it never had.
 *
 * @param aMagnitude is a linear magnitude, normally non-negative.
 * @return the magnitude in dB, or NaN if it has none.
 */
inline double MagnitudeToDb( double aMagnitude )
{
    // Also catches a NaN magnitude, which has no dB value either
    if( !( aMagnitude > 0.0 ) )
        return std::numeric_limits<double>::quiet_NaN();

    return 20.0 * std::log10( aMagnitude );
}

#endif /* SIM_TYPES_H */
