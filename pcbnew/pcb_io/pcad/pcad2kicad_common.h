/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
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
 * @file pcad2kicad_common.h
 */

#ifndef PCAD2KICAD_COMMON_H_
#define PCAD2KICAD_COMMON_H_

#include <eda_text.h>

class wxString;
class XNODE;

namespace PCAD2KICAD
{

#define PCAD2KICAD_SCALE_SCH_TO_INCH_GRID

enum TTEXT_JUSTIFY
{
   LowerLeft,
   LowerCenter,
   LowerRight,
   UpperLeft,
   UpperCenter,
   UpperRight,
   Left,
   Center,
   Right
};

struct TTEXTVALUE
{
    wxString      text;
    int           textPositionX, textPositionY;
    int           textHeight, textstrokeWidth;
    EDA_ANGLE     textRotation;
    int           textIsVisible, mirror, textUnit;
    int           correctedPositionX, correctedPositionY;
    TTEXT_JUSTIFY justify;
    bool          isBold;
    bool          isItalic;
    bool          isTrueType;
};

extern wxString     GetWord( wxString* aStr );
extern XNODE*       FindPinMap( XNODE* aNode );
extern int          StrToIntUnits( const wxString& aStr, char aAxe,
                                   const wxString& aActualConversion );
extern wxString     GetAndCutWordWithMeasureUnits( wxString*       aStr,
                                                   const wxString& aDefaultMeasurementUnit );
extern int          StrToInt1Units( const wxString& aStr );
extern wxString     ValidateName( const wxString& aName );
extern wxString     ValidateReference( const wxString& aRef );
extern void         SetWidth( const wxString& aStr,
                              const wxString& aDefaultMeasurementUnit,
                              int*            aWidth,
                              const wxString& aActualConversion );
extern void         SetPosition( const wxString& aStr,
                                 const wxString& aDefaultMeasurementUnit,
                                 int*            aX,
                                 int*            aY,
                                 const wxString& aActualConversion );
extern void         SetDoublePrecisionPosition( const wxString& aStr,
                                                const wxString& aDefaultMeasurementUnit,
                                                double*         aX,
                                                double*         aY,
                                                const wxString& aActualConversion );
extern TTEXT_JUSTIFY GetJustifyIdentificator( const wxString& aJustify );
extern void         SetTextParameters( XNODE*          aNode,
                                       TTEXTVALUE*     aTextValue,
                                       const wxString& aDefaultMeasurementUnit,
                                       const wxString& aActualConversion );
extern void         SetFontProperty( XNODE*          aNode,
                                     TTEXTVALUE*     aTextValue,
                                     const wxString& aDefaultMeasurementUnit,
                                     const wxString& aActualConversion );
extern void         SetTextJustify( EDA_TEXT* aText, TTEXT_JUSTIFY aJustify );
extern int          CalculateTextLengthSize( TTEXTVALUE* aText );
extern void         CorrectTextPosition( TTEXTVALUE* aValue );
extern void         SetTextSizeFromStrokeFontHeight( EDA_TEXT* aText, int aTextHeight );
extern void         SetTextSizeFromTrueTypeFontHeight( EDA_TEXT* aText, int aTextHeight );
extern XNODE*       FindNode( XNODE* aChild, const wxString& aTag );
extern wxString     FindNodeGetContent( XNODE* aChild, const wxString& aTag );
extern void         InitTTextValue( TTEXTVALUE* aTextValue );

} // namespace PCAD2KICAD

#endif    // PCAD2KICAD_COMMON_H_
