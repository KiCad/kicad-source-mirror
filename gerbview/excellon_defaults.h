/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#pragma once

// Default format for coordinates: they are the default values, not the actual values
// defaut format is 3:3 in mm and 2:4 in inch
//
// number of digits in mantissa:
#define FMT_MANTISSA_MM     3
#define FMT_MANTISSA_INCH   4
// number of digits, integer part:
#define FMT_INTEGER_MM      3
#define FMT_INTEGER_INCH    2


/**
 * management of default values used to read a Excellon (.nc) drill file
 * Some important parameters are not defined in drill files, and some others
 * can be missing in poor drill files.
 * These default values are used when parameter is not found in file
 */
struct EXCELLON_DEFAULTS
{
    bool m_UnitsMM;         // false = inch, true = mm
    bool m_LeadingZero;     // True = LZ false = TZ
    int m_MmIntegerLen;     // number of digits for the integer part of a coordinate in mm
    int m_MmMantissaLen;    // number of digits for the mantissa part of a coordinate in mm
    int m_InchIntegerLen;   // number of digits for the integer part of a coordinate in inch
    int m_InchMantissaLen;  // number of digits for the mantissa part of a coordinate in inch

    EXCELLON_DEFAULTS() { ResetToDefaults(); }

    void ResetToDefaults()
    {
        m_UnitsMM = false;
        m_LeadingZero = true;
        m_MmIntegerLen = FMT_INTEGER_MM;
        m_MmMantissaLen = FMT_MANTISSA_MM;
        m_InchIntegerLen = FMT_INTEGER_INCH;
        m_InchMantissaLen = FMT_MANTISSA_INCH;
    }
};
