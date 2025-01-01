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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_TYPES_H
#define SIM_TYPES_H

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
    SPT_Y_AXIS_MASK   = 0x00FF,

    // X axis
    SPT_TIME          = 0x0100,
    SPT_LIN_FREQUENCY = 0x0200,
    SPT_LOG_FREQUENCY = 0x0400,
    SPT_SWEEP         = 0x0800,
    SPT_X_AXIS_MASK   = 0xFF00,

    SPT_UNKNOWN       = 0x0000
};

#endif /* SIM_TYPES_H */
