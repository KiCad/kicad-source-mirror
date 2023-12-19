/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _SCH_SEXPR_PLUGIN_COMMON_H_
#define _SCH_SEXPR_PLUGIN_COMMON_H_

#include <eda_shape.h>
#include <kiid.h>
#include <lib_pin.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>


//class COLOR4D;
class KIID;
class OUTPUTFORMATTER;
class STROKE_PARAMS;


/**
 * Fill token formatting helper.
 */
extern void formatFill( OUTPUTFORMATTER* aFormatter, int aNestLevel, FILL_T aFillMode,
                        const COLOR4D& aFillColor );

extern const char* getPinElectricalTypeToken( ELECTRICAL_PINTYPE aType );

extern const char* getPinShapeToken( GRAPHIC_PINSHAPE aShape );

extern EDA_ANGLE getPinAngle( PIN_ORIENTATION aOrientation );

extern const char* getSheetPinShapeToken( LABEL_FLAG_SHAPE aShape );

extern EDA_ANGLE getSheetPinAngle( SHEET_SIDE aSide );

extern const char* getTextTypeToken( KICAD_T aType );

extern void formatArc( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aArc,
                       bool aIsPrivate, const STROKE_PARAMS& aStroke, FILL_T aFillMode,
                       const COLOR4D& aFillColor, const KIID& aUuid = niluuid );

extern void formatCircle( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aCircle,
                          bool aIsPrivate, const STROKE_PARAMS& aStroke, FILL_T aFillMode,
                          const COLOR4D& aFillColor, const KIID& aUuid = niluuid );

extern void formatRect( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aRect,
                        bool aIsPrivate, const STROKE_PARAMS& aStroke, FILL_T aFillMode,
                        const COLOR4D& aFillColor, const KIID& aUuid = niluuid );

extern void formatBezier( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aBezier,
                          bool aIsPrivate, const STROKE_PARAMS& aStroke, FILL_T aFillMode,
                          const COLOR4D& aFillColor, const KIID& aUuid = niluuid );

extern void formatPoly( OUTPUTFORMATTER* aFormatter, int aNestLevel, EDA_SHAPE* aPolyLine,
                        bool aIsPrivate, const STROKE_PARAMS& aStroke, FILL_T aFillMode,
                        const COLOR4D& aFillColor, const KIID& aUuid = niluuid );

#endif    // _SCH_SEXPR_PLUGIN_COMMON_H_
