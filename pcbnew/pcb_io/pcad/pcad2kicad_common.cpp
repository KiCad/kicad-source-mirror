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
 * @file pcad2kicad_common.cpp
 */

#include <pcad/pcad2kicad_common.h>

#include <common.h>
#include <math/util.h>      // for KiROUND
#include <trigo.h>
#include <xnode.h>

#include <wx/regex.h>

namespace PCAD2KICAD {

// PCAD stroke font average ratio of width to size
const double TEXT_WIDTH_TO_SIZE_AVERAGE = 0.5;

// PCAD proportions of stroke font
const double STROKE_HEIGHT_TO_SIZE = 0.656;
const double STROKE_WIDTH_TO_SIZE = 0.69;

// TrueType font
const double TRUETYPE_HEIGHT_TO_SIZE = 0.585;
const double TRUETYPE_WIDTH_TO_SIZE = 0.585;
const double TRUETYPE_THICK_PER_HEIGHT = 0.073;
const double TRUETYPE_BOLD_THICK_MUL = 1.6;
const long TRUETYPE_BOLD_MIN_WEIGHT = 700;


wxString GetWord( wxString* aStr )
{
    wxString result = wxEmptyString;

    *aStr = aStr->Trim( false );

    if( aStr->Len() == 0 )
        return result;

    if( (*aStr)[0] == wxT( '"' ) )
    {
        result  += (*aStr)[0];
        *aStr   = aStr->Mid( 1 ); // remove Frot apostrofe

        while( aStr->Len() > 0 && (*aStr)[0] != wxT( '"' ) )
        {
            result  += (*aStr)[0];
            *aStr   = aStr->Mid( 1 );
        }

        if( aStr->Len() > 0 && (*aStr)[0] == wxT( '"' ) )
        {
            result  += (*aStr)[0];
            *aStr   = aStr->Mid( 1 ); // remove ending apostrophe
        }
    }
    else
    {
        while( aStr->Len() > 0
               && !( (*aStr)[0] == wxT( ' ' )
                     || (*aStr)[0] == wxT( '(' )
                     || (*aStr)[0] == wxT( ')' ) ) )
        {
            result  += (*aStr)[0];
            *aStr   = aStr->Mid( 1 );
        }
    }

    result.Trim( true );
    result.Trim( false );

    return result;
}


XNODE* FindPinMap( XNODE* aNode )
{
    XNODE* result, * lNode;

    result  = nullptr;
    lNode   = FindNode( aNode, wxT( "attachedPattern" ) );

    if( lNode )
        result = FindNode( lNode, wxT( "padPinMap" ) );

    return result;
}


double StrToDoublePrecisionUnits( const wxString& aStr, char aAxe,
                                  const wxString& aActualConversion )
{
    wxString    ls;
    double      i;
    char        u;

    ls = aStr;
    ls.Trim( true );
    ls.Trim( false );

    if( ls.Len() > 0 )
    {
        u = ls[ls.Len() - 1];

        while( ls.Len() > 0
               && !( ls[ls.Len() - 1] == wxT( '.' )
                     || ls[ls.Len() - 1] == wxT( ',' )
                     || (ls[ls.Len() - 1] >= wxT( '0' ) && ls[ls.Len() - 1] <= wxT( '9' ) ) ) )
        {
            ls = ls.Left( ls.Len() - 1 );
        }

        while( ls.Len() > 0
               && !( ls[0] == wxT( '-' )
                     || ls[0] == wxT( '+' )
                     || ls[0] == wxT( '.' )
                     || ls[0] == wxT( ',' )
                     || (ls[0] >= wxT( '0' ) && ls[0] <= wxT( '9' ) ) ) )
        {
            ls = ls.Mid( 1 );
        }

        if( u == wxT( 'm' ) )
        {
            ls.ToCDouble( &i );

#ifdef PCAD2KICAD_SCALE_SCH_TO_INCH_GRID
            if( aActualConversion == wxT( "SCH" )
                || aActualConversion == wxT( "SCHLIB" ) )
                i = i * (0.0254 / 0.025);
#endif

            i = pcbIUScale.mmToIU( i );
        }
        else
        {
            ls.ToCDouble( &i );
            i *= pcbIUScale.IU_PER_MILS;
        }
    }
    else
    {
        i = 0.0;
    }

    if( ( aActualConversion == wxT( "PCB" ) || aActualConversion == wxT( "SCH" ) )
        && aAxe == wxT( 'Y' ) )
        return -i;
    else
        return i; // Y axe is mirrored compared to P-Cad
}


int StrToIntUnits( const wxString& aStr, char aAxe, const wxString& aActualConversion )
{
    return KiROUND( StrToDoublePrecisionUnits( aStr, aAxe, aActualConversion ) );
}


wxString GetAndCutWordWithMeasureUnits( wxString* aStr, const wxString& aDefaultMeasurementUnit )
{
    wxString result;

    aStr->Trim( false );
    result = wxEmptyString;

    // value
    while( aStr->Len() > 0 && (*aStr)[0] != wxT( ' ' ) )
    {
        result  += (*aStr)[0];
        *aStr   = aStr->Mid( 1 );
    }

    aStr->Trim( false );

    // if there is also measurement unit
    while( aStr->Len() > 0
           && ( ( (*aStr)[0] >= wxT( 'a' ) && (*aStr)[0] <= wxT( 'z' ) )
                || ( (*aStr)[0] >= wxT( 'A' ) && (*aStr)[0] <= wxT( 'Z' ) ) ) )
    {
        result  += (*aStr)[0];
        *aStr   = aStr->Mid( 1 );
    }

    // and if not, add default....
    if( result.Len() > 0
        && ( result[result.Len() - 1] == wxT( '.' )
             || result[result.Len() - 1] == wxT( ',' )
             || (result[result.Len() - 1] >= wxT( '0' )
                 && result[result.Len() - 1] <= wxT( '9' ) ) ) )
    {
        result += aDefaultMeasurementUnit;
    }

    return result;
}


int StrToInt1Units( const wxString& aStr )
{
    double num, precision = 10;

    aStr.ToCDouble( &num );
    return KiROUND( num * precision );
}


wxString ValidateName( const wxString& aName )
{
    wxString retv = aName;
    retv.Replace( wxT( " " ), wxT( "_" ) );

    return retv;
}


wxString ValidateReference( const wxString& aRef )
{
    wxRegEx reRef;
    reRef.Compile( wxT( "^[[:digit:]][[:digit:]]*$" ) );

    wxString retv = aRef;

    if( reRef.Matches( retv ) )
        retv.Prepend( wxT( '.' ) );

    return retv;
}


void SetWidth( const wxString& aStr, const wxString& aDefaultMeasurementUnit, int* aWidth,
               const wxString& aActualConversion )
{
    wxString tmp = aStr;

    *aWidth = StrToIntUnits( GetAndCutWordWithMeasureUnits( &tmp, aDefaultMeasurementUnit ),
                             wxT( ' ' ), aActualConversion );
}


void SetHeight( const wxString& aStr, const wxString& aDefaultMeasurementUnit, int* aHeight,
                const wxString& aActualConversion )
{
    wxString tmp = aStr;

    *aHeight = StrToIntUnits( GetAndCutWordWithMeasureUnits( &tmp, aDefaultMeasurementUnit ),
                              wxT( ' ' ), aActualConversion );
}


void SetPosition( const wxString& aStr, const wxString& aDefaultMeasurementUnit, int* aX, int* aY,
                  const wxString& aActualConversion )
{
    wxString tmp = aStr;

    *aX = StrToIntUnits( GetAndCutWordWithMeasureUnits( &tmp, aDefaultMeasurementUnit ),
                         wxT( 'X' ), aActualConversion );
    *aY = StrToIntUnits( GetAndCutWordWithMeasureUnits( &tmp, aDefaultMeasurementUnit ),
                         wxT( 'Y' ), aActualConversion );
}


void SetDoublePrecisionPosition( const wxString& aStr, const wxString& aDefaultMeasurementUnit,
                                 double* aX, double* aY, const wxString& aActualConversion )
{
    wxString tmp = aStr;

    *aX = StrToDoublePrecisionUnits(
            GetAndCutWordWithMeasureUnits( &tmp, aDefaultMeasurementUnit ), wxT( 'X' ),
            aActualConversion );
    *aY = StrToDoublePrecisionUnits(
            GetAndCutWordWithMeasureUnits( &tmp, aDefaultMeasurementUnit ), wxT( 'Y' ),
            aActualConversion );
}


TTEXT_JUSTIFY GetJustifyIdentificator( const wxString& aJustify )
{
    TTEXT_JUSTIFY id;

    if( aJustify.IsSameAs( wxT( "LowerCenter" ), false ) )
        id = LowerCenter;
    else if( aJustify.IsSameAs( wxT( "LowerRight" ), false ) )
        id = LowerRight;
    else if( aJustify.IsSameAs( wxT( "UpperLeft" ), false ) )
        id = UpperLeft;
    else if( aJustify.IsSameAs( wxT( "UpperCenter" ), false ) )
        id = UpperCenter;
    else if( aJustify.IsSameAs( wxT( "UpperRight" ), false ) )
        id = UpperRight;
    else if( aJustify.IsSameAs( wxT( "Left" ), false ) )
        id = Left;
    else if( aJustify.IsSameAs( wxT( "Center" ), false ) )
        id = Center;
    else if( aJustify.IsSameAs( wxT( "Right" ), false ) )
        id = Right;
    else
        id = LowerLeft;

    return id;
}


void SetTextParameters( XNODE* aNode, TTEXTVALUE* aTextValue,
                        const wxString& aDefaultMeasurementUnit, const wxString& aActualConversion )
{
    XNODE*   tNode;
    wxString str;

    tNode = FindNode( aNode, wxT( "pt" ) );

    if( tNode )
        SetPosition( tNode->GetNodeContent(), aDefaultMeasurementUnit, &aTextValue->textPositionX,
                     &aTextValue->textPositionY, aActualConversion );

    tNode = FindNode( aNode, wxT( "rotation" ) );

    if( tNode )
    {
        str = tNode->GetNodeContent();
        str.Trim( false );
        aTextValue->textRotation = EDA_ANGLE( StrToInt1Units( str ), TENTHS_OF_A_DEGREE_T );
    }
    else
    {
        aTextValue->textRotation = ANGLE_0;
    }

    str = FindNodeGetContent( aNode, wxT( "isVisible" ) );

    if( str.IsSameAs( wxT( "True" ), false ) )
        aTextValue->textIsVisible = 1;
    else
        aTextValue->textIsVisible = 0;

    str = FindNodeGetContent( aNode, wxT( "justify" ) );
    aTextValue->justify = GetJustifyIdentificator( str );

    str = FindNodeGetContent( aNode, wxT( "isFlipped" ) );

    if( str.IsSameAs( wxT( "True" ), false ) )
        aTextValue->mirror = 1;
    else
        aTextValue->mirror = 0;

    tNode = FindNode( aNode, wxT( "textStyleRef" ) );

    if( tNode )
        SetFontProperty( tNode, aTextValue, aDefaultMeasurementUnit, aActualConversion );
}


void SetFontProperty( XNODE* aNode, TTEXTVALUE* aTextValue, const wxString& aDefaultMeasurementUnit,
                      const wxString& aActualConversion )
{
    wxString n, propValue;

    aNode->GetAttribute( wxT( "Name" ), &n );

    while( aNode->GetName() != wxT( "www.lura.sk" ) )
        aNode = aNode->GetParent();

    aNode = FindNode( aNode, wxT( "library" ) );

    if( aNode )
        aNode = FindNode( aNode, wxT( "textStyleDef" ) );

    while( aNode )
    {
        aNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );

        if( propValue == n )
            break;

        aNode = aNode->GetNext();
    }

    if( aNode )
    {
        wxString fontType;

        propValue = FindNodeGetContent( aNode, wxT( "textStyleDisplayTType" ) );
        aTextValue->isTrueType = ( propValue.IsSameAs( wxT( "True" ), false ) );

        aNode = FindNode( aNode, wxT( "font" ) );
        fontType = FindNodeGetContent( aNode, wxT( "fontType" ) );

        if( ( aTextValue->isTrueType && !fontType.IsSameAs( wxT( "TrueType" ), false ) ) ||
            ( !aTextValue->isTrueType && !fontType.IsSameAs( wxT( "Stroke" ), false ) ) )
            aNode = aNode->GetNext();

        if( aNode )
        {
            if( aTextValue->isTrueType )
            {
                propValue = FindNodeGetContent( aNode, wxT( "fontItalic" ) );
                aTextValue->isItalic = propValue.IsSameAs( wxT( "True" ), false );

                propValue = FindNodeGetContent( aNode, wxT( "fontWeight" ) );

                if( propValue != wxEmptyString )
                {
                    long fontWeight;

                    propValue.ToLong( &fontWeight );
                    aTextValue->isBold = ( fontWeight >= TRUETYPE_BOLD_MIN_WEIGHT );
                }
            }

            XNODE* lNode;

            lNode = FindNode( aNode, wxT( "fontHeight" ) );

            if( lNode )
                SetHeight( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                           &aTextValue->textHeight, aActualConversion );

            if( aTextValue->isTrueType )
            {
                aTextValue->textstrokeWidth = TRUETYPE_THICK_PER_HEIGHT * aTextValue->textHeight;

                if( aTextValue->isBold )
                    aTextValue->textstrokeWidth *= TRUETYPE_BOLD_THICK_MUL;
            }
            else
            {
                lNode = FindNode( aNode, wxT( "strokeWidth" ) );

                if( lNode )
                    SetWidth( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                              &aTextValue->textstrokeWidth, aActualConversion );
            }
        }
    }
}


void SetTextJustify( EDA_TEXT* aText, TTEXT_JUSTIFY aJustify )
{
    switch( aJustify )
    {
    case LowerLeft:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case LowerCenter:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case LowerRight:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    case UpperLeft:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case UpperCenter:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case UpperRight:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    case Left:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case Center:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case Right:
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    }
}


int CalculateTextLengthSize( TTEXTVALUE* aText )
{
    return KiROUND( (double) aText->text.Len() *
                    (double) aText->textHeight * TEXT_WIDTH_TO_SIZE_AVERAGE );
}


void CorrectTextPosition( TTEXTVALUE* aValue )
{
    int cm = aValue->mirror ? -1 : 1;
    int cl = KiROUND( (double) CalculateTextLengthSize( aValue ) / 2.0 );
    int ch = KiROUND( (double) aValue->textHeight / 2.0 );
    int posX = 0;
    int posY = 0;

    if( aValue->justify == LowerLeft || aValue->justify == Left || aValue->justify == UpperLeft )
        posX += cl * cm;
    else if( aValue->justify == LowerRight || aValue->justify == Right ||
             aValue->justify == UpperRight )
        posX -= cl * cm;

    if( aValue->justify == LowerLeft || aValue->justify == LowerCenter ||
        aValue->justify == LowerRight )
        posY -= ch;
    else if( aValue->justify == UpperLeft || aValue->justify == UpperCenter ||
             aValue->justify == UpperRight )
        posY += ch;

    RotatePoint( &posX, &posY, aValue->textRotation );

    aValue->correctedPositionX = aValue->textPositionX + posX;
    aValue->correctedPositionY = aValue->textPositionY + posY;
}


void SetTextSizeFromStrokeFontHeight( EDA_TEXT* aText, int aTextHeight )
{
    aText->SetTextSize( KiROUND( aTextHeight * STROKE_WIDTH_TO_SIZE, aTextHeight * STROKE_HEIGHT_TO_SIZE ) );
}


void SetTextSizeFromTrueTypeFontHeight( EDA_TEXT* aText, int aTextHeight )
{
    aText->SetTextSize( KiROUND( aTextHeight * TRUETYPE_WIDTH_TO_SIZE, aTextHeight * TRUETYPE_HEIGHT_TO_SIZE ) );
}


XNODE* FindNode( XNODE* aChild, const wxString& aTag )
{
    aChild = aChild->GetChildren();

    while( aChild )
    {
        if( aChild->GetName().IsSameAs( aTag, false ) )
            return aChild;

        aChild = aChild->GetNext();
    }

    return nullptr;
}


wxString FindNodeGetContent( XNODE* aChild, const wxString& aTag )
{
    wxString str = wxEmptyString;

    aChild = FindNode( aChild, aTag );

    if( aChild )
    {
        str = aChild->GetNodeContent();
        str.Trim( false );
        str.Trim( true );
    }

    return str;
}


void InitTTextValue( TTEXTVALUE* aTextValue )
{
    aTextValue->text = wxEmptyString;
    aTextValue->textPositionX   = 0;
    aTextValue->textPositionY   = 0;
    aTextValue->textRotation    = ANGLE_0;
    aTextValue->textHeight      = 0;
    aTextValue->textstrokeWidth = 0;
    aTextValue->textIsVisible   = 0;
    aTextValue->mirror      = 0;
    aTextValue->textUnit    = 0;
    aTextValue->correctedPositionX  = 0;
    aTextValue->correctedPositionY  = 0;
    aTextValue->justify = LowerLeft;
    aTextValue->isBold = false;
    aTextValue->isItalic = false;
    aTextValue->isTrueType = false;
}

} // namespace PCAD2KICAD
