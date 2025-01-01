/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
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

/**
 * @file units.i
 * @brief unit conversion code
 */

// Unit conversion, between internal units and mm or mils

%pythoncode
%{
    def ToMM(iu):
        if type(iu) in [int,float]:
            return float(iu) / float(pcbIUScale.IU_PER_MM)
        elif type(iu) in [wxPoint,wxSize,VECTOR2I,VECTOR2L]:
            return tuple(map(ToMM,iu))
        else:
            raise TypeError("ToMM() expects int, float, wxPoint, wxSize, VECTOR2I or VECTOR2L, instead got type " + str(type(iu)))

    def FromMM(mm):
        if type(mm) in [int,float]:
            return int(float(mm) * float(pcbIUScale.IU_PER_MM))
        elif type(mm) in [wxPoint,wxSize,VECTOR2I,VECTOR2L]:
            return tuple(map(FromMM,mm))
        else:
            raise TypeError("FromMM() expects int, float, wxPoint, wxSize, VECTOR2I or VECTOR2L, instead got type " + str(type(mm)))

    def ToMils(iu):
        if type(iu) in [int,float]:
            return float(iu) / float(pcbIUScale.IU_PER_MILS)
        elif type(iu) in [wxPoint,wxSize,VECTOR2I,VECTOR2L]:
            return tuple(map(ToMils,iu))
        else:
            raise TypeError("ToMils() expects int, float, wxPoint, wxSize, VECTOR2I or VECTOR2L, instead got type " + str(type(iu)))

    def FromMils(mils):
        if type(mils) in [int,float]:
            return int(float(mils)*float(pcbIUScale.IU_PER_MILS))
        elif type(mils) in [wxPoint,wxSize,VECTOR2I,VECTOR2L]:
            return tuple(map(FromMils,mils))
        else:
            raise TypeError("FromMils() expects int, float, wxPoint, wxSize, VECTOR2I or VECTOR2L, instead got type " + str(type(mils)))

    def PutOnGridMM(value, gridSizeMM):
        thresh = FromMM(gridSizeMM)
        return round(value/thresh)*thresh

    def PutOnGridMils(value, gridSizeMils):
        thresh = FromMils(gridSizeMils)
        return round(value/thresh)*thresh

    def wxSizeMM(mmx,mmy):
        return wxSize(FromMM(mmx),FromMM(mmy))

    def wxSizeMils(mmx,mmy):
        return wxSize(FromMils(mmx),FromMils(mmy))

    def wxPointMM(mmx,mmy):
        return wxPoint(FromMM(mmx),FromMM(mmy))

    def wxPointMils(mmx,mmy):
        return wxPoint(FromMils(mmx),FromMils(mmy))

    def VECTOR2I_MM(mmx,mmy):
        return VECTOR2I(FromMM(mmx),FromMM(mmy))

    def VECTOR2I_Mils(mmx,mmy):
        return VECTOR2I(FromMils(mmx),FromMils(mmy))

    def wxRectMM(x,y,wx,wy):
        x = int(FromMM(x))
        y = int(FromMM(y))
        wx = int(FromMM(wx))
        wy = int (FromMM(wy))
        return wxRect(x,y,wx,wy)

    def wxRectMils(x,y,wx,wy):
        x = int(FromMils(x))
        y = int(FromMils(y))
        wx = int(FromMils(wx))
        wy = int(FromMils(wy))
        return wxRect(x,y,wx,wy)
%}
