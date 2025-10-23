/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file GERBER_plotter.cpp
 * @brief specialized plotter for GERBER files format
 */

#include <string_utils.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_rect.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <trigo.h>
#include <wx/log.h>
#include <cstdio>
#include <fmt/format.h>

#include <build_version.h>

#include <plotters/plotter_gerber.h>
#include <plotters/gbr_plotter_aperture_macros.h>

#include <gbr_metadata.h>


// if GBR_USE_MACROS is defined, pads having a shape that is not a Gerber primitive
// will use a macro when possible
// Old code will be removed only after many tests
//
// Note also: setting m_gerberDisableApertMacros to true disable all aperture macros
// in Gerber files
//
#define GBR_USE_MACROS_FOR_CHAMFERED_ROUND_RECT
#define GBR_USE_MACROS_FOR_CHAMFERED_RECT
#define GBR_USE_MACROS_FOR_ROUNDRECT
#define GBR_USE_MACROS_FOR_TRAPEZOID
#define GBR_USE_MACROS_FOR_ROTATED_OVAL
#define GBR_USE_MACROS_FOR_ROTATED_RECT
#define GBR_USE_MACROS_FOR_CUSTOM_PAD

// max count of corners to create a aperture macro for a custom shape.
// provided just in case a aperture macro type free polygon creates issues
// when the number of corners is too high.
// (1 corner = up to 24 chars)
// Gerber doc say max corners 5000. We use a slightly smaller value.
// if a custom shape needs more than GBR_MACRO_FOR_CUSTOM_PAD_MAX_CORNER_COUNT, it
// will be plot using a region.
#define GBR_MACRO_FOR_CUSTOM_PAD_MAX_CORNER_COUNT 4990
#define AM_FREEPOLY_BASENAME "FreePoly"


// A helper function to compare 2 polygons: polygons are similar if they have the same
// number of vertices and each vertex coordinate are similar, i.e. if the difference
// between coordinates is small ( <= margin to accept rounding issues coming from polygon
// geometric transforms like rotation
static bool polyCompare( const std::vector<VECTOR2I>& aPolygon,
                         const std::vector<VECTOR2I>& aTestPolygon )
{
    // fast test: polygon sizes must be the same:
    if( aTestPolygon.size() != aPolygon.size() )
        return false;

    const int margin = 2;

    for( size_t jj = 0; jj < aPolygon.size(); jj++ )
    {
        if( std::abs( aPolygon[jj].x - aTestPolygon[jj].x ) > margin ||
            std::abs( aPolygon[jj].y - aTestPolygon[jj].y ) > margin )
            return false;
    }

    return true;
}


GERBER_PLOTTER::GERBER_PLOTTER()
{
    workFile  = nullptr;
    finalFile = nullptr;
    m_currentApertureIdx = -1;
    m_apertureAttribute = 0;

    // number of digits after the point (number of digits of the mantissa
    // Be careful: the Gerber coordinates are stored in an integer
    // so 6 digits (inches) or 5 digits (mm) is a good value
    // To avoid overflow, 7 digits (inches) or 6 digits is a max.
    // with lower values than 6 digits (inches) or 5 digits (mm),
    // Creating self-intersecting polygons from non-intersecting polygons
    // happen easily.
    m_gerberUnitInch = false;
    m_gerberUnitFmt = 6;
    m_useX2format = true;
    m_useNetAttributes = true;
    m_gerberDisableApertMacros = false;

    m_hasApertureRoundRect = false;     // true is at least one round rect aperture is in use
    m_hasApertureRotOval = false;       // true is at least one oval rotated aperture is in use
    m_hasApertureRotRect = false;       // true is at least one rect. rotated aperture is in use
    m_hasApertureOutline4P = false;       // true is at least one rotated rect or trapezoid pad
                                        // aperture is in use
    m_hasApertureChamferedRect = false; // true is at least one chamfered rect
                                        // (no rounded corner) is in use
}


void GERBER_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                                  double aScale, bool aMirror )
{
    wxASSERT( aMirror == false );
    m_plotMirror = false;
    m_plotOffset = aOffset;
    wxASSERT( aScale == 1 );              // aScale parameter is not used in Gerber
    m_plotScale = 1;                      // Plot scale is *always* 1.0

    m_IUsPerDecimil = aIusPerDecimil;

    // gives now a default value to iuPerDeviceUnit (because the units of the caller is now known)
    // which could be modified later by calling SetGerberCoordinatesFormat()
    m_iuPerDeviceUnit = pow( 10.0, m_gerberUnitFmt ) / ( m_IUsPerDecimil * 10000.0 );

    // We don't handle the filmbox, and it's more useful to keep the
    // origin at the origin
    m_paperSize.x = 0;
    m_paperSize.y = 0;
}


void GERBER_PLOTTER::SetGerberCoordinatesFormat( int aResolution, bool aUseInches )
{
    m_gerberUnitInch = aUseInches;
    m_gerberUnitFmt = aResolution;

    m_iuPerDeviceUnit = pow( 10.0, m_gerberUnitFmt ) / ( m_IUsPerDecimil * 10000.0 );

    if( ! m_gerberUnitInch )
        m_iuPerDeviceUnit *= 25.4;     // gerber output in mm
}


void GERBER_PLOTTER::emitDcode( const VECTOR2D& pt, int dcode )
{
    fmt::println( m_outputFile, "X{}Y{}D{:02d}*", KiROUND( pt.x ), KiROUND( pt.y ), dcode );
}


void GERBER_PLOTTER::ClearAllAttributes()
{
    // Remove all attributes from object attributes dictionary (TO. and TA commands)
    if( m_useX2format )
        fmt::println( m_outputFile, "%TD*%" );
    else
        fmt::println( m_outputFile, "G04 #@! TD*" );

    m_objectAttributesDictionary.clear();
}


void GERBER_PLOTTER::clearNetAttribute()
{
    // disable a Gerber net attribute (exists only in X2 with net attributes mode).
    if( m_objectAttributesDictionary.empty() )     // No net attribute or not X2 mode
        return;

    // Remove all net attributes from object attributes dictionary
    if( m_useX2format )
        fmt::println( m_outputFile, "%TD*%" );
    else
        fmt::println( m_outputFile, "G04 #@! TD*" );

    m_objectAttributesDictionary.clear();
}


void GERBER_PLOTTER::StartBlock( void* aData )
{
    // Currently, it is the same as EndBlock(): clear all aperture net attributes
    EndBlock( aData );
}


void GERBER_PLOTTER::EndBlock( void* aData )
{
    // Remove all net attributes from object attributes dictionary
    clearNetAttribute();
}


void GERBER_PLOTTER::formatNetAttribute( GBR_NETLIST_METADATA* aData )
{
    // print a Gerber net attribute record.
    // it is added to the object attributes dictionary
    // On file, only modified or new attributes are printed.
    if( aData == nullptr )
        return;

    if( !m_useNetAttributes )
        return;

    bool useX1StructuredComment = !m_useX2format;

    bool clearDict;
    std::string short_attribute_string;

    if( !FormatNetAttribute( short_attribute_string, m_objectAttributesDictionary,
                        aData, clearDict, useX1StructuredComment ) )
        return;

    if( clearDict )
        clearNetAttribute();

    if( !short_attribute_string.empty() )
        fmt::print( m_outputFile, "{}", short_attribute_string );

    if( m_useX2format && !aData->m_ExtraData.IsEmpty() )
    {
        std::string extra_data = TO_UTF8( aData->m_ExtraData );
        fmt::print( m_outputFile, "{}", extra_data );
    }
}


bool GERBER_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    m_hasApertureRoundRect = false;     // true is at least one round rect aperture is in use
    m_hasApertureRotOval = false;       // true is at least one oval rotated aperture is in use
    m_hasApertureRotRect = false;       // true is at least one rect. rotated aperture is in use
    m_hasApertureOutline4P = false;     // true is at least one rotated rect/trapezoid aperture
                                        // is in use
    m_hasApertureChamferedRect = false; // true is at least one chamfered rect is in use
    m_am_freepoly_list.ClearList();

    wxASSERT( m_outputFile );

    finalFile = m_outputFile;     // the actual gerber file will be created later

    // Create a temp file in system temp to avoid potential network share buffer issues for
    // the final read and save.
    m_workFilename = wxFileName::CreateTempFileName( "" );
    workFile = wxFopen( m_workFilename, wxT( "wt" ) );
    m_outputFile = workFile;
    wxASSERT( m_outputFile );

    if( m_outputFile == nullptr )
        return false;

    for( unsigned ii = 0; ii < m_headerExtraLines.GetCount(); ii++ )
    {
        if( ! m_headerExtraLines[ii].IsEmpty() )
            fmt::println( m_outputFile, "{}", TO_UTF8( m_headerExtraLines[ii] ) );
    }

    // Set coordinate format to 3.6 or 4.5 absolute, leading zero omitted
    // the number of digits for the integer part of coordinates is needed
    // in gerber format, but is not very important when omitting leading zeros
    // It is fixed here to 3 (inch) or 4 (mm), but is not actually used
    int leadingDigitCount = m_gerberUnitInch ? 3 : 4;

    fmt::println( m_outputFile, "%FSLAX{}{}Y{}{}*%",
             leadingDigitCount, m_gerberUnitFmt,
             leadingDigitCount, m_gerberUnitFmt );
    fmt::println( m_outputFile,
             "G04 Gerber Fmt {}.{}, Leading zero omitted, Abs format (unit {})*",
             leadingDigitCount, m_gerberUnitFmt,
             m_gerberUnitInch ? "inch" : "mm" );

    wxString Title = m_creator + wxT( " " ) + GetBuildVersion();

    // In gerber files, ASCII7 chars only are allowed.
    // So use a ISO date format (using a space as separator between date and time),
    // not a localized date format
    wxDateTime date = wxDateTime::Now();
    fmt::println( m_outputFile, "G04 Created by KiCad ({}) date {}*",
             TO_UTF8( Title ), TO_UTF8( date.FormatISOCombined( ' ') ) );

    /* Mass parameter: unit = IN/MM */
    if( m_gerberUnitInch )
        fmt::println( m_outputFile, "%MOIN*%" );
    else
        fmt::println( m_outputFile, "%MOMM*%" );

    // Be sure the usual dark polarity is selected:
    fmt::println( m_outputFile, "%LPD*%" );

    // Set initial interpolation mode: always G01 (linear):
    fmt::println( m_outputFile, "G01*" );

    // Add aperture list start point
    fmt::println( m_outputFile, "G04 APERTURE LIST*" );

    // Give a minimal value to the default pen size, used to plot items in sketch mode
    if( m_renderSettings )
    {
        const int pen_min = 0.1 * m_IUsPerDecimil * 10000 / 25.4;   // for min width = 0.1 mm
        m_renderSettings->SetDefaultPenWidth( std::max( m_renderSettings->GetDefaultPenWidth(),
                                                        pen_min ) );
    }

    return true;
}


bool GERBER_PLOTTER::EndPlot()
{
    char     line[1024];

    wxASSERT( m_outputFile );

    /* Outfile is actually a temporary file i.e. workFile */
    fmt::println( m_outputFile, "M02*" );
    fflush( m_outputFile );

    fclose( workFile );
    workFile   = wxFopen( m_workFilename, wxT( "rt" ));
    wxASSERT( workFile );
    m_outputFile = finalFile;

    // Placement of apertures in RS274X
    while( fgets( line, 1024, workFile ) )
    {
        fmt::print( m_outputFile, "{}", line );

        char* substr = strtok( line, "\n\r" );

        if( substr && strcmp( substr, "G04 APERTURE LIST*" ) == 0 )
        {
            // Add aperture list macro:
            if( m_hasApertureRoundRect || m_hasApertureRotOval ||
                m_hasApertureOutline4P || m_hasApertureRotRect ||
                m_hasApertureChamferedRect || m_am_freepoly_list.AmCount() )
            {
                fmt::println( m_outputFile, "G04 Aperture macros list*" );

                if( m_hasApertureRoundRect )
                    fmt::print( m_outputFile, APER_MACRO_ROUNDRECT_HEADER );

                if( m_hasApertureRotOval )
                    fmt::print( m_outputFile, APER_MACRO_SHAPE_OVAL_HEADER );

                if( m_hasApertureRotRect )
                    fmt::print( m_outputFile, APER_MACRO_ROT_RECT_HEADER );

                if( m_hasApertureOutline4P )
                    fmt::print( m_outputFile, APER_MACRO_OUTLINE4P_HEADER );

                if( m_hasApertureChamferedRect )
                {
                    fmt::print( m_outputFile, APER_MACRO_OUTLINE5P_HEADER );
                    fmt::print( m_outputFile, APER_MACRO_OUTLINE6P_HEADER );
                    fmt::print( m_outputFile, APER_MACRO_OUTLINE7P_HEADER );
                    fmt::print( m_outputFile, APER_MACRO_OUTLINE8P_HEADER );
                }

                if( m_am_freepoly_list.AmCount() )
                {
                    // aperture sizes are in inch or mm, regardless the
                    // coordinates format
                    double fscale = 0.0001 * m_plotScale / m_IUsPerDecimil; // inches

                    if(! m_gerberUnitInch )
                        fscale *= 25.4;     // size in mm

                    m_am_freepoly_list.Format( m_outputFile, fscale );
                }

                fmt::println( m_outputFile, "G04 Aperture macros list end*" );
            }

            writeApertureList();
            fmt::println( m_outputFile, "G04 APERTURE END LIST*" );
        }
    }

    fclose( workFile );
    fclose( finalFile );
    ::wxRemoveFile( m_workFilename );
    m_outputFile = nullptr;

    return true;
}


void GERBER_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    if( aWidth == DO_NOT_SET_LINE_WIDTH )
        return;
    else if( aWidth == USE_DEFAULT_LINE_WIDTH )
        aWidth =  m_renderSettings->GetDefaultPenWidth();

    wxASSERT_MSG( aWidth >= 0, "Plotter called to set negative pen width" );

    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );
    int           aperture_attribute = 0;
    std::string   custom_attribute = "";
    if( gbr_metadata )
    {
        aperture_attribute = gbr_metadata->GetApertureAttrib();
        custom_attribute = gbr_metadata->GetCustomAttribute();
    }

    selectAperture( VECTOR2I( aWidth, aWidth ), 0, ANGLE_0, APERTURE::AT_PLOTTING,
                    aperture_attribute, custom_attribute );
    m_currentPenWidth = aWidth;
}


int GERBER_PLOTTER::GetOrCreateAperture( const VECTOR2I& aSize, int aRadius,
                                         const EDA_ANGLE& aRotation, APERTURE::APERTURE_TYPE aType,
                                         int                aApertureAttribute,
                                         const std::string& aCustomAttribute )
{
    int last_D_code = 9;

    // Search an existing aperture
    for( int idx = 0; idx < (int)m_apertures.size(); ++idx )
    {
        APERTURE* tool = &m_apertures[idx];
        last_D_code = tool->m_DCode;

        if( ( tool->m_Type == aType ) && ( tool->m_Size == aSize ) && ( tool->m_Radius == aRadius )
            && ( tool->m_Rotation == aRotation )
            && ( tool->m_ApertureAttribute == aApertureAttribute )
            && ( tool->m_CustomAttribute == aCustomAttribute ) )
        {
            return idx;
        }
    }

    // Allocate a new aperture
    APERTURE new_tool;
    new_tool.m_Size     = aSize;
    new_tool.m_Type     = aType;
    new_tool.m_Radius   = aRadius;
    new_tool.m_Rotation = aRotation;
    new_tool.m_DCode    = last_D_code + 1;
    new_tool.m_ApertureAttribute = aApertureAttribute;
    new_tool.m_CustomAttribute = aCustomAttribute;

    m_apertures.push_back( std::move( new_tool ) );

    return m_apertures.size() - 1;
}


int GERBER_PLOTTER::GetOrCreateAperture( const std::vector<VECTOR2I>& aCorners,
                                         const EDA_ANGLE& aRotation, APERTURE::APERTURE_TYPE aType,
                                         int                aApertureAttribute,
                                         const std::string& aCustomAttribute )
{
    int last_D_code = 9;

    // For APERTURE::AM_FREE_POLYGON aperture macros, we need to create the macro
    // on the fly, because due to the fact the vertex count is not a constant we
    // cannot create a static definition.
    if( APERTURE::AM_FREE_POLYGON == aType )
    {
        int idx = m_am_freepoly_list.FindAm( aCorners );

        if( idx < 0 )
            m_am_freepoly_list.Append( aCorners );
    }

    // Search an existing aperture
    for( int idx = 0; idx < (int)m_apertures.size(); ++idx )
    {
        APERTURE* tool = &m_apertures[idx];

        last_D_code = tool->m_DCode;

        if( ( tool->m_Type == aType ) && ( tool->m_Corners.size() == aCorners.size() )
            && ( tool->m_Rotation == aRotation )
            && ( tool->m_ApertureAttribute == aApertureAttribute )
            && ( tool->m_CustomAttribute == aCustomAttribute ) )
        {
            // A candidate is found. the corner lists must be similar
            bool is_same = polyCompare( tool->m_Corners, aCorners );

            if( is_same )
                return idx;
        }
    }

    // Allocate a new aperture
    APERTURE new_tool;

    new_tool.m_Corners  = aCorners;
    new_tool.m_Size     = VECTOR2I( 0, 0 );   // Not used
    new_tool.m_Type     = aType;
    new_tool.m_Radius   = 0;             // Not used
    new_tool.m_Rotation = aRotation;
    new_tool.m_DCode    = last_D_code + 1;
    new_tool.m_ApertureAttribute = aApertureAttribute;
    new_tool.m_CustomAttribute = aCustomAttribute;

    m_apertures.push_back( std::move( new_tool ) );

    return m_apertures.size() - 1;
}


void GERBER_PLOTTER::selectAperture( const VECTOR2I& aSize, int aRadius, const EDA_ANGLE& aRotation,
                                     APERTURE::APERTURE_TYPE aType, int aApertureAttribute,
                                     const std::string& aCustomAttribute )
{
    bool change = ( m_currentApertureIdx < 0 ) ||
                  ( m_apertures[m_currentApertureIdx].m_Type != aType ) ||
                  ( m_apertures[m_currentApertureIdx].m_Size != aSize ) ||
                  ( m_apertures[m_currentApertureIdx].m_Radius != aRadius ) ||
                  ( m_apertures[m_currentApertureIdx].m_Rotation != aRotation );

    if( !change )
    {
        change = ( m_apertures[m_currentApertureIdx].m_ApertureAttribute != aApertureAttribute )
                 || ( m_apertures[m_currentApertureIdx].m_CustomAttribute != aCustomAttribute );
    }

    if( change )
    {
        // Pick an existing aperture or create a new one
        m_currentApertureIdx = GetOrCreateAperture( aSize, aRadius, aRotation, aType,
                                                    aApertureAttribute, aCustomAttribute );
        fmt::println( m_outputFile, "D{}*", m_apertures[m_currentApertureIdx].m_DCode );
    }
}


void GERBER_PLOTTER::selectAperture( const std::vector<VECTOR2I>& aCorners,
                                     const EDA_ANGLE& aRotation, APERTURE::APERTURE_TYPE aType,
                                     int aApertureAttribute, const std::string& aCustomAttribute )
{
    bool change = ( m_currentApertureIdx < 0 ) ||
                  ( m_apertures[m_currentApertureIdx].m_Type != aType ) ||
                  ( m_apertures[m_currentApertureIdx].m_Corners.size() != aCorners.size() ) ||
                  ( m_apertures[m_currentApertureIdx].m_Rotation != aRotation );

    if( !change )   // Compare corner lists
    {
        for( size_t ii = 0; ii < aCorners.size(); ii++ )
        {
            if( aCorners[ii] != m_apertures[m_currentApertureIdx].m_Corners[ii] )
            {
                change = true;
                break;
            }
        }
    }

    if( !change )
    {
        change = ( m_apertures[m_currentApertureIdx].m_ApertureAttribute != aApertureAttribute )
                 || ( m_apertures[m_currentApertureIdx].m_CustomAttribute != aCustomAttribute );
    }

    if( change )
    {
        // Pick an existing aperture or create a new one
        m_currentApertureIdx = GetOrCreateAperture( aCorners, aRotation, aType, aApertureAttribute,
                                                    aCustomAttribute );
        fmt::println( m_outputFile, "D{}*", m_apertures[m_currentApertureIdx].m_DCode );
    }
}


void GERBER_PLOTTER::selectApertureWithAttributes( const VECTOR2I& aPos, GBR_METADATA* aGbrMetadata,
                                                   VECTOR2I aSize, int aRadius,
                                                   const EDA_ANGLE&        aAngle,
                                                   APERTURE::APERTURE_TYPE aType )
{
    VECTOR2D pos_dev = userToDeviceCoordinates( aPos );

    int         aperture_attribute = 0;
    std::string custom_attribute = "";

    if( aGbrMetadata )
    {
        aperture_attribute = aGbrMetadata->GetApertureAttrib();
        custom_attribute = aGbrMetadata->GetCustomAttribute();
    }

    selectAperture( aSize, aRadius, aAngle, aType, aperture_attribute, custom_attribute );

    if( aGbrMetadata )
        formatNetAttribute( &aGbrMetadata->m_NetlistMetadata );

    emitDcode( pos_dev, 3 );
}


void GERBER_PLOTTER::writeApertureList()
{
    wxASSERT( m_outputFile );

    bool useX1StructuredComment = false;

    if( !m_useX2format )
        useX1StructuredComment = true;

    // Init
    for( APERTURE& tool : m_apertures )
    {
        // aperture sizes are in inch or mm, regardless the
        // coordinates format
        double fscale = 0.0001 * m_plotScale / m_IUsPerDecimil; // inches

        if(! m_gerberUnitInch )
            fscale *= 25.4;     // size in mm

        int attribute = tool.m_ApertureAttribute;

        if( attribute != m_apertureAttribute )
        {
            fmt::print( m_outputFile, "{}", GBR_APERTURE_METADATA::FormatAttribute(
                           (GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB) attribute,
                           useX1StructuredComment, tool.m_CustomAttribute ) );
        }

        fmt::print( m_outputFile, "%ADD{}", tool.m_DCode );

        /* Please note: the Gerber specs for mass parameters say that
           exponential syntax is *not* allowed and the decimal point should
           also be always inserted. So the %g format is ruled out, but %f is fine
           (the # modifier forces the decimal point). Sadly the %f formatter
           can't remove trailing zeros but that's not a problem, since nothing
           forbid it (the file is only slightly longer) */

        switch( tool.m_Type )
        {
        case APERTURE::AT_CIRCLE:
            fmt::println( m_outputFile, "C,{:#f}*%", tool.GetDiameter() * fscale );
            break;

        case APERTURE::AT_RECT:
            fmt::println( m_outputFile, "R,{:#f}X{:#f}*%",
                          tool.m_Size.x * fscale,
                          tool.m_Size.y * fscale );
            break;

        case APERTURE::AT_PLOTTING:
            fmt::println( m_outputFile, "C,{:#f}*%", tool.m_Size.x * fscale );
            break;

        case APERTURE::AT_OVAL:
            fmt::println( m_outputFile, "O,{:#f}X{:#f}*%",
                          tool.m_Size.x * fscale,
                          tool.m_Size.y * fscale );
            break;

        case APERTURE::AT_REGULAR_POLY:
        case APERTURE::AT_REGULAR_POLY3:
        case APERTURE::AT_REGULAR_POLY4:
        case APERTURE::AT_REGULAR_POLY5:
        case APERTURE::AT_REGULAR_POLY6:
        case APERTURE::AT_REGULAR_POLY7:
        case APERTURE::AT_REGULAR_POLY8:
        case APERTURE::AT_REGULAR_POLY9:
        case APERTURE::AT_REGULAR_POLY10:
        case APERTURE::AT_REGULAR_POLY11:
        case APERTURE::AT_REGULAR_POLY12:
            fmt::println( m_outputFile, "P,{:#f}X{}X{:#f}*%",
                          tool.GetDiameter() * fscale,
                          tool.GetRegPolyVerticeCount(),
                          tool.GetRotation().AsDegrees() );
            break;

        case APERTURE::AM_ROUND_RECT:       // Aperture macro for round rect pads
        {
            // The aperture macro needs coordinates of the centers of the 4 corners
            std::vector<VECTOR2I> corners;
            VECTOR2I half_size( tool.m_Size.x/2-tool.m_Radius, tool.m_Size.y/2-tool.m_Radius );

            // Ensure half_size.x and half_size.y > minimal value to avoid shapes
            // with null size (especially the rectangle with coordinates corners)
            // Because the minimal value for a non nul Gerber coord in 10nm
            // in format 4.5, use 10 nm as minimal value.
            // (Even in 4.6 format, use 10 nm, because gerber viewers can have
            // a internal unit bigger than 1 nm)
            const int min_size_value = 10;
            half_size.x = std::max( half_size.x, min_size_value );
            half_size.y = std::max( half_size.y, min_size_value );

            corners.emplace_back( -half_size.x, -half_size.y );
            corners.emplace_back( half_size.x, -half_size.y );
            corners.emplace_back( half_size.x, half_size.y );
            corners.emplace_back( -half_size.x, half_size.y );

            // Rotate the corner coordinates:
            for( int ii = 0; ii < 4; ii++ )
                RotatePoint( corners[ii], -tool.m_Rotation );

            fmt::print( m_outputFile, "{},{:#f}X",
                        APER_MACRO_ROUNDRECT_NAME,
                        tool.m_Radius * fscale );

            // Add each corner
            for( int ii = 0; ii < 4; ii++ )
            {
                fmt::print( m_outputFile, "{:#f}X{:#f}X",
                            corners[ii].x * fscale,
                            corners[ii].y * fscale );
            }

            fmt::println( m_outputFile, "0*%" );
        }
            break;

        case APERTURE::AM_ROT_RECT:         // Aperture macro for rotated rect pads
            fmt::println( m_outputFile, "{},{:#f}X{:#f}X{:#f}*%", APER_MACRO_ROT_RECT_NAME,
                        tool.m_Size.x * fscale,
                        tool.m_Size.y * fscale,
                        tool.m_Rotation.AsDegrees() );
            break;

        case APERTURE::APER_MACRO_OUTLINE4P:    // Aperture macro for trapezoid pads
        case APERTURE::APER_MACRO_OUTLINE5P:    // Aperture macro for chamfered rect pads
        case APERTURE::APER_MACRO_OUTLINE6P:    // Aperture macro for chamfered rect pads
        case APERTURE::APER_MACRO_OUTLINE7P:    // Aperture macro for chamfered rect pads
        case APERTURE::APER_MACRO_OUTLINE8P:    // Aperture macro for chamfered rect pads
            switch( tool.m_Type )
            {
            case APERTURE::APER_MACRO_OUTLINE4P:
                fmt::print( m_outputFile, APER_MACRO_OUTLINE4P_NAME );
                break;
            case APERTURE::APER_MACRO_OUTLINE5P:
                fmt::print( m_outputFile, APER_MACRO_OUTLINE5P_NAME );
                break;
            case APERTURE::APER_MACRO_OUTLINE6P:
                fmt::print( m_outputFile, APER_MACRO_OUTLINE6P_NAME );
                break;
            case APERTURE::APER_MACRO_OUTLINE7P:
                fmt::print( m_outputFile, APER_MACRO_OUTLINE7P_NAME );
                break;
            case APERTURE::APER_MACRO_OUTLINE8P:
                fmt::print( m_outputFile, APER_MACRO_OUTLINE8P_NAME );
                break;
            default:
                break;
            }

            // Add separator after aperture macro name
            fmt::print( m_outputFile, "," );

            // Output all corners (should be 4 to 8 corners)
            // Remember: the Y coordinate must be negated, due to the fact in Pcbnew
            // the Y axis is from top to bottom
            for( const VECTOR2I& corner : tool.m_Corners)
                fmt::print( m_outputFile, "{:#f}X{:#f}X", corner.x * fscale, -corner.y * fscale );

            // close outline and output rotation
            fmt::println( m_outputFile, "{:#f}*%", tool.m_Rotation.AsDegrees() );
            break;

        case APERTURE::AM_ROTATED_OVAL:         // Aperture macro for rotated oval pads
                                                // (not rotated is a primitive)
            // m_Size.x = full length; m_Size.y = width, and the macro aperture expects
            // the position of ends
        {
                // the seg_len is the distance between the 2 circle centers
                int seg_len = tool.m_Size.x - tool.m_Size.y;

                // Center of the circle on the segment start point:
                VECTOR2I start( seg_len/2, 0 );

                // Center of the circle on the segment end point:
                VECTOR2I end( - seg_len/2, 0 );

                RotatePoint( start, tool.m_Rotation );
                RotatePoint( end, tool.m_Rotation );

                fmt::println( m_outputFile, "{},{:#f}X{:#f}X{:#f}X{:#f}X{:#f}X0*%",
                              APER_MACRO_SHAPE_OVAL_NAME,
                              tool.m_Size.y * fscale,              // width
                              start.x * fscale, -start.y * fscale, // X,Y corner start pos
                              end.x * fscale, -end.y * fscale );   // X,Y corner end pos
        }
            break;

        case APERTURE::AM_FREE_POLYGON:
        {
            // Find the aperture macro name in the list of aperture macro
            // created on the fly for this polygon:
            int idx = m_am_freepoly_list.FindAm( tool.m_Corners );

            // Write DCODE id ( "%ADDxx" is already in buffer) and rotation
            // the full line is something like :%ADD12FreePoly1,45.000000*%
            fmt::println( m_outputFile, "{}{},{:#f}*%",
                          AM_FREEPOLY_BASENAME,
                          idx,
                          tool.m_Rotation.AsDegrees() );
            break;
        }
        }

        m_apertureAttribute = attribute;

        // Currently reset the aperture attribute. Perhaps a better optimization
        // is to store the last attribute
        if( attribute )
        {
            if( m_useX2format )
                fmt::println( m_outputFile, "%TD*%" );
            else
                fmt::println( m_outputFile, "G04 #@! TD*" );

            m_apertureAttribute = 0;
        }

    }
}


void GERBER_PLOTTER::PenTo( const VECTOR2I& aPos, char plume )
{
    wxASSERT( m_outputFile );
    VECTOR2D pos_dev = userToDeviceCoordinates( aPos );

    switch( plume )
    {
    case 'Z':
        break;

    case 'U':
        emitDcode( pos_dev, 2 );
        break;

    case 'D':
        emitDcode( pos_dev, 1 );
    }

    m_penState = plume;
}


void GERBER_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                           int aCornerRadius )
{
    if( aCornerRadius > 0 )
        wxFAIL_MSG( wxT( "GERBER_PLOTTER must use PlotPolyAsRegion() for rounded-corner rectangles!" ) );

    std::vector<VECTOR2I> cornerList;

    cornerList.reserve( 5 );

    // Build corners list
    cornerList.push_back( p1 );

    VECTOR2I corner( p1.x, p2.y );
    cornerList.push_back( corner );
    cornerList.push_back( p2 );
    corner.x = p2.x;
    corner.y = p1.y;
    cornerList.push_back( corner );
    cornerList.push_back( p1 );

    PlotPoly( cornerList, fill, width, nullptr );
}


void GERBER_PLOTTER::Circle( const VECTOR2I& aCenter, int aDiameter, FILL_T aFill, int aWidth )
{
    Arc( aCenter, ANGLE_0, ANGLE_180, aDiameter / 2, aFill, aWidth );
    Arc( aCenter, ANGLE_180, ANGLE_180, aDiameter / 2, aFill, aWidth );
}



void GERBER_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                          const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    SetCurrentLineWidth( aWidth );

    double arcLength = std::abs( aRadius * aAngle.AsRadians() );

    if( arcLength < 100 || std::abs( aAngle.AsDegrees() ) < 0.1 )
    {
        // Prevent plotting very short arcs as full circles, especially with 4.5 mm precision.
        // Also reduce the risk of integer overflow issues.
        polyArc( aCenter, aStartAngle, aAngle, aRadius, aFill, GetCurrentLineWidth() );
    }
    else
    {
        EDA_ANGLE endAngle = aStartAngle + aAngle;

        // aFill is not used here.
        plotArc( aCenter, aStartAngle, endAngle, aRadius, false );
    }
}


void GERBER_PLOTTER::plotArc( const SHAPE_ARC& aArc, bool aPlotInRegion )
{
    VECTOR2I  start( aArc.GetP0() );
    VECTOR2I  end( aArc.GetP1() );
    VECTOR2I  center( aArc.GetCenter() );

    if( !aPlotInRegion )
        MoveTo( start);
    else
        LineTo( start );

    VECTOR2D devEnd = userToDeviceCoordinates( end );

    // devRelCenter is the position on arc center relative to the arc start, in Gerber coord.
    // Warning: it is **not** userToDeviceCoordinates( center -  start ) when the plotter
    // has an offset.
    VECTOR2D devRelCenter = userToDeviceCoordinates( center ) - userToDeviceCoordinates( start );

    // We need to know if the arc is CW or CCW in device coordinates, so build this arc.
    SHAPE_ARC deviceArc( userToDeviceCoordinates( start ),
                         userToDeviceCoordinates( aArc.GetArcMid() ),
                         devEnd, 0 );

    fmt::println( m_outputFile, "G75*" );        // Multiquadrant (360 degrees) mode

    if( deviceArc.IsClockwise() )
        fmt::println( m_outputFile, "G02*" );    // Active circular interpolation, CW
    else
        fmt::println( m_outputFile, "G03*" );    // Active circular interpolation, CCW

    fmt::println( m_outputFile, "X{}Y{}I{}J{}D01*",
             KiROUND( devEnd.x ), KiROUND( devEnd.y ),
             KiROUND( devRelCenter.x ), KiROUND( devRelCenter.y ) );

    fmt::println( m_outputFile, "G01*" ); // Back to linear interpolate (perhaps useless here).
}


void GERBER_PLOTTER::plotArc( const VECTOR2I& aCenter, const EDA_ANGLE& aStartAngle,
                              const EDA_ANGLE& aEndAngle, double aRadius, bool aPlotInRegion )
{
    VECTOR2I start, end;
    start.x = KiROUND( aCenter.x + aRadius * aStartAngle.Cos() );
    start.y = KiROUND( aCenter.y + aRadius * aStartAngle.Sin() );

    if( !aPlotInRegion )
        MoveTo( start );
    else
        LineTo( start );

    end.x = KiROUND( aCenter.x + aRadius * aEndAngle.Cos() );
    end.y = KiROUND( aCenter.y + aRadius * aEndAngle.Sin() );
    VECTOR2D devEnd = userToDeviceCoordinates( end );

    // devRelCenter is the position on arc center relative to the arc start, in Gerber coord.
    VECTOR2D devRelCenter = userToDeviceCoordinates( aCenter ) - userToDeviceCoordinates( start );

    fmt::println( m_outputFile, "G75*" ); // Multiquadrant (360 degrees) mode

    if( aStartAngle > aEndAngle )
        fmt::println( m_outputFile, "G03*" ); // Active circular interpolation, CCW
    else
        fmt::println( m_outputFile, "G02*" ); // Active circular interpolation, CW

    fmt::println( m_outputFile, "X{}Y{}I{}J{}D01*",
             KiROUND( devEnd.x ), KiROUND( devEnd.y ),
             KiROUND( devRelCenter.x ), KiROUND( devRelCenter.y ) );

    fmt::println( m_outputFile, "G01*" ); // Back to linear interpolate (perhaps useless here).
}


void GERBER_PLOTTER::PlotGerberRegion( const SHAPE_LINE_CHAIN& aPoly, GBR_METADATA* aGbrMetadata )
{
    if( aPoly.PointCount() <= 2 )
        return;

    bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

    if( aGbrMetadata )
    {
        std::string attrib = aGbrMetadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

        if( !attrib.empty() )
        {
            fmt::print( m_outputFile, "{}", attrib );
            clearTA_AperFunction = true;
        }
    }

    PlotPoly( aPoly, FILL_T::FILLED_SHAPE, 0 , aGbrMetadata );

    // Clear the TA attribute, to avoid the next item to inherit it:
    if( clearTA_AperFunction )
    {
        if( m_useX2format )
            fmt::println( m_outputFile, "%TD.AperFunction*%" );
        else
            fmt::println( m_outputFile, "G04 #@! TD.AperFunction*" );
    }
}


void GERBER_PLOTTER::PlotGerberRegion( const std::vector<VECTOR2I>& aCornerList,
                                       GBR_METADATA* aGbrMetadata )
{
    if( aCornerList.size() <= 2 )
        return;

    bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

    if( aGbrMetadata )
    {
        std::string attrib = aGbrMetadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

        if( !attrib.empty() )
        {
            fmt::print( m_outputFile, "{}", attrib );
            clearTA_AperFunction = true;
        }
    }

    PlotPoly( aCornerList, FILL_T::FILLED_SHAPE, 0, aGbrMetadata );

    // Clear the TA attribute, to avoid the next item to inherit it:
    if( clearTA_AperFunction )
    {
        if( m_useX2format )
            fmt::println( m_outputFile, "%TD.AperFunction*%" );
        else
            fmt::println( m_outputFile, "G04 #@! TD.AperFunction*" );
    }
}


void GERBER_PLOTTER::PlotPolyAsRegion( const SHAPE_LINE_CHAIN& aPoly, FILL_T aFill,
                                       int aWidth, GBR_METADATA* aGbrMetadata )
{
    // plot a filled polygon using Gerber region, therefore adding X2 attributes
    // to the solid polygon
    if( aWidth || aFill == FILL_T::NO_FILL )
        PlotPoly( aPoly, FILL_T::NO_FILL, aWidth, aGbrMetadata );

    if( aFill != FILL_T::NO_FILL )
        PlotGerberRegion( aPoly, aGbrMetadata );
}


void GERBER_PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aPoly, FILL_T aFill, int aWidth,
                               void* aData )
{
    if( aPoly.CPoints().size() <= 1 )
        return;

    // Gerber format does not know filled polygons with thick outline
    // Therefore, to plot a filled polygon with outline having a thickness,
    // one should plot outline as thick segments
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( aFill != FILL_T::NO_FILL )
    {
        fmt::println( m_outputFile, "G36*" );

        MoveTo( VECTOR2I( aPoly.CPoint( 0 ) ) );

        fmt::println( m_outputFile, "G01*" ); // Set linear interpolation.

        for( int ii = 1; ii < aPoly.PointCount(); ii++ )
        {
            int arcindex = aPoly.ArcIndex( ii );

            if( arcindex < 0 )
            {
                /// Plain point
                LineTo( VECTOR2I( aPoly.CPoint( ii ) ) );
            }
            else
            {
                const SHAPE_ARC& arc = aPoly.Arc( arcindex );

                plotArc( arc, true );

                // skip points on arcs, since we plot the arc itself
                while( ii+1 < aPoly.PointCount() && arcindex == aPoly.ArcIndex( ii+1 ) )
                    ii++;
            }
        }

        // If the polygon is not closed, close it:
        if( aPoly.CPoint( 0 ) != aPoly.CLastPoint() )
            FinishTo( VECTOR2I( aPoly.CPoint( 0 ) ) );

        fmt::println( m_outputFile, "G37*" );
    }
    else if( aWidth != 0 )    // Draw the polyline/polygon outline
    {
        SetCurrentLineWidth( aWidth, gbr_metadata );

        MoveTo( VECTOR2I( aPoly.CPoint( 0 ) ) );

        for( int ii = 1; ii < aPoly.PointCount(); ii++ )
        {
            int arcindex = aPoly.ArcIndex( ii );

            if( arcindex < 0 )
            {
                /// Plain point
                LineTo( VECTOR2I( aPoly.CPoint( ii ) ) );
            }
            else
            {
                const SHAPE_ARC& arc = aPoly.Arc( arcindex );

                plotArc( arc, true );

                // skip points on arcs, since we plot the arc itself
                while( ii+1 < aPoly.PointCount() && arcindex == aPoly.ArcIndex( ii+1 ) )
                    ii++;
             }
        }

        // Ensure the thick outline is closed for filled polygons
        // (if not filled, could be only a polyline)
        if( ( aPoly.CPoint( 0 ) != aPoly.CLastPoint() ) && ( aPoly.IsClosed() || aFill != FILL_T::NO_FILL ) )
            LineTo( VECTOR2I( aPoly.CPoint( 0 ) ) );

        PenFinish();
    }
}


void GERBER_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                               void * aData )
{
    if( aCornerList.size() <= 1 )
        return;

    // Gerber format does not know filled polygons with thick outline
    // Therefore, to plot a filled polygon with outline having a thickness,
    // one should plot outline as thick segments
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    if( aFill != FILL_T::NO_FILL )
    {
        fmt::println( m_outputFile, "G36*" );

        MoveTo( aCornerList[0] );
        fmt::println( m_outputFile, "G01*" ); // Set linear interpolation.

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // If the polygon is not closed, close it:
        if( aCornerList[0] != aCornerList[aCornerList.size()-1] )
            FinishTo( aCornerList[0] );

        fmt::println( m_outputFile, "G37*" );
    }

    if( aWidth != 0 || aFill == FILL_T::NO_FILL )    // Draw the polyline/polygon outline
    {
        SetCurrentLineWidth( aWidth, gbr_metadata );

        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Ensure the thick outline is closed for filled polygons
        // (if not filled, could be only a polyline)
        if( aFill != FILL_T::NO_FILL && ( aCornerList[aCornerList.size() - 1] != aCornerList[0] ) )
            LineTo( aCornerList[0] );

        PenFinish();
    }
}


void GERBER_PLOTTER::ThickSegment( const VECTOR2I& start, const VECTOR2I& end, int width, void* aData )
{
    if( GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData ) )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    SetCurrentLineWidth( width, aData );
    PLOTTER::ThickSegment( start, end, DO_NOT_SET_LINE_WIDTH, aData );
}


void GERBER_PLOTTER::ThickArc( const VECTOR2D& aCentre, const EDA_ANGLE& aStartAngle,
                               const EDA_ANGLE& aAngle, double aRadius, int aWidth, void* aData )
{
    if( GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData ) )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    SetCurrentLineWidth( aWidth, aData );
    PLOTTER::ThickArc( aCentre, aStartAngle, aAngle, aRadius, DO_NOT_SET_LINE_WIDTH, aData );
}


void GERBER_PLOTTER::ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width, void* aData )
{
    if( GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData ) )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    SetCurrentLineWidth( width, aData );
    PLOTTER::ThickRect( p1, p2, DO_NOT_SET_LINE_WIDTH, aData );
}


void GERBER_PLOTTER::ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData )
{
    if( GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData ) )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    SetCurrentLineWidth( width, aData );
    PLOTTER::ThickCircle( pos, diametre, DO_NOT_SET_LINE_WIDTH, aData );
}


void GERBER_PLOTTER::FilledCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    // A filled circle is a graphic item, not a pad.
    // So it is drawn, not flashed.
    if( GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData ) )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    // Draw a circle of diameter = diameter/2 with a line thickness = radius,
    // To create a filled circle
    SetCurrentLineWidth( diametre/2, aData );
    Circle( pos, diametre/2, FILL_T::NO_FILL, DO_NOT_SET_LINE_WIDTH );
}


void GERBER_PLOTTER::ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData )
{
    if( GBR_METADATA *gbr_metadata = static_cast<GBR_METADATA*>( aData ) )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    SetCurrentLineWidth( aWidth, aData );
    PLOTTER::ThickPoly( aPoly, DO_NOT_SET_LINE_WIDTH, aData );
}


void GERBER_PLOTTER::FlashPadCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    VECTOR2I      size( diametre, diametre );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    selectApertureWithAttributes( pos, gbr_metadata, size, 0, ANGLE_0, APERTURE::AT_CIRCLE );
}


void GERBER_PLOTTER::FlashPadOval( const VECTOR2I& aPos, const VECTOR2I& aSize,
                                   const EDA_ANGLE& aOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I      size( aSize );
    EDA_ANGLE     orient( aOrient );
    orient.Normalize();
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    // Flash a vertical or horizontal shape (this is a basic aperture).
    if( orient.IsCardinal() )
    {
        if( orient.IsCardinal90() )
            std::swap( size.x, size.y );

        selectApertureWithAttributes( aPos, gbr_metadata, size, 0, ANGLE_0, APERTURE::AT_OVAL );
    }
    else    // Plot pad as region.
            // Only regions and flashed items accept a object attribute TO.P for the pin name
    {
#ifdef GBR_USE_MACROS_FOR_ROTATED_OVAL
        if( !m_gerberDisableApertMacros )
#endif
        {
            m_hasApertureRotOval = true;
            // We are using a aperture macro that expect size.y < size.x
            // i.e draw a horizontal line for rotation = 0.0
            // size.x = length, size.y = width
            if( size.x < size.y )
            {
                std::swap( size.x, size.y );
                orient += ANGLE_90;

                if( orient > ANGLE_180 )
                    orient -= ANGLE_180;
            }

            selectApertureWithAttributes( aPos, gbr_metadata, size, 0, orient,
                                          APERTURE::AM_ROTATED_OVAL );
            return;
        }

        // Draw the oval as round rect pad with a radius = 50% min size)
        // In gerber file, it will be drawn as a region with arcs, and can be
        // detected as pads (similar to a flashed pad)
        FlashPadRoundRect( aPos, aSize, std::min( aSize.x, aSize.y ) / 2, orient, aData );
    }
}


void GERBER_PLOTTER::FlashPadRect( const VECTOR2I& pos, const VECTOR2I& aSize,
                                   const EDA_ANGLE& aOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I      size( aSize );
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    // Horizontal / vertical rect can use a basic aperture (not a macro)
    // so use it for rotation n*90 deg
    if( aOrient.IsCardinal() )
    {
        // Build the not rotated equivalent shape:
        if( aOrient.IsCardinal90() )
            std::swap( size.x, size.y );

        selectApertureWithAttributes( pos, gbr_metadata, size, 0, ANGLE_0, APERTURE::AT_RECT );
    }
    else
    {
#ifdef GBR_USE_MACROS_FOR_ROTATED_RECT
        if( !m_gerberDisableApertMacros )
        {
            m_hasApertureRotRect = true;

            selectApertureWithAttributes( pos, gbr_metadata, size, 0, aOrient,
                                          APERTURE::AM_ROT_RECT );
        }
        else
#endif
        {
            // plot pad shape as Gerber region
            VECTOR2I coord[4];

            // coord[0] is assumed the lower left
            // coord[1] is assumed the upper left
            // coord[2] is assumed the upper right
            // coord[3] is assumed the lower right
            coord[0].x = -size.x/2;   // lower left
            coord[0].y = size.y/2;
            coord[1].x = -size.x/2;   // upper left
            coord[1].y = -size.y/2;
            coord[2].x = size.x/2;    // upper right
            coord[2].y = -size.y/2;
            coord[3].x = size.x/2;    // lower right
            coord[3].y = size.y/2;

            FlashPadTrapez( pos, coord, aOrient, aData );
        }
    }
}


void GERBER_PLOTTER::FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                        int aCornerRadius, const EDA_ANGLE& aOrient, void* aData )

{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

#ifdef GBR_USE_MACROS_FOR_ROUNDRECT
    if( !m_gerberDisableApertMacros )
#endif
    {
        m_hasApertureRoundRect = true;

        selectApertureWithAttributes( aPadPos, gbr_metadata, aSize, aCornerRadius, aOrient,
                                      APERTURE::AM_ROUND_RECT );
        return;
    }

    // A Pad RoundRect is plotted as a Gerber region.
    // Initialize region metadata:
    bool clearTA_AperFunction = false;     // true if a TA.AperFunction is used

    if( gbr_metadata )
    {
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );
        std::string attrib = gbr_metadata->m_ApertureMetadata.FormatAttribute( !m_useX2format );

        if( !attrib.empty() )
        {
            fmt::print( m_outputFile, "{}", attrib );
            clearTA_AperFunction = true;
        }
    }

    // Plot the region using arcs in corners.
    plotRoundRectAsRegion( aPadPos, aSize, aCornerRadius, aOrient );

    // Clear the TA attribute, to avoid the next item to inherit it:
    if( clearTA_AperFunction )
    {
        if( m_useX2format )
            fmt::println( m_outputFile, "%TD.AperFunction*%" );
        else
            fmt::println( m_outputFile, "G04 #@! TD.AperFunction*" );
    }
}


void GERBER_PLOTTER::plotRoundRectAsRegion( const VECTOR2I& aRectCenter, const VECTOR2I& aSize,
                                            int aCornerRadius, const EDA_ANGLE& aOrient )
{
    // The region outline is generated by 4 sides and 4 90 deg arcs
    //  1 --- 2
    //  |  c  |
    //  4 --- 3

    // Note also in user coordinates the Y axis is from top to bottom
    // for historical reasons.

    // A helper structure to handle outlines coordinates (segments and arcs)
    // in user coordinates
    struct RR_EDGE
    {
        VECTOR2I  m_start;
        VECTOR2I  m_end;
        VECTOR2I  m_center;
        EDA_ANGLE m_arc_angle_start;
    };

    int hsizeX = aSize.x/2;
    int hsizeY = aSize.y/2;

    RR_EDGE curr_edge;
    std::vector<RR_EDGE> rr_outline;

    rr_outline.reserve( 4 );

    // Build outline coordinates, relative to rectangle center, rotation 0:

    // Top left corner 1 (and 4 to 1 left vertical side @ x=-hsizeX)
    curr_edge.m_start.x = -hsizeX;
    curr_edge.m_start.y = hsizeY - aCornerRadius;
    curr_edge.m_end.x = curr_edge.m_start.x;
    curr_edge.m_end.y = -hsizeY + aCornerRadius;
    curr_edge.m_center.x = -hsizeX + aCornerRadius;
    curr_edge.m_center.y = curr_edge.m_end.y;
    curr_edge.m_arc_angle_start = aOrient + ANGLE_180;

    rr_outline.push_back( curr_edge );

    // Top right corner 2 (and 1 to 2 top horizontal side @ y=-hsizeY)
    curr_edge.m_start.x = -hsizeX + aCornerRadius;
    curr_edge.m_start.y = -hsizeY;
    curr_edge.m_end.x = hsizeX - aCornerRadius;
    curr_edge.m_end.y = curr_edge.m_start.y;
    curr_edge.m_center.x = curr_edge.m_end.x;
    curr_edge.m_center.y = -hsizeY + aCornerRadius;
    curr_edge.m_arc_angle_start = aOrient + ANGLE_90;

    rr_outline.push_back( curr_edge );

    // bottom right corner 3 (and 2 to 3 right vertical side @ x=hsizeX)
    curr_edge.m_start.x = hsizeX;
    curr_edge.m_start.y = -hsizeY + aCornerRadius;
    curr_edge.m_end.x =  curr_edge.m_start.x;
    curr_edge.m_end.y = hsizeY - aCornerRadius;
    curr_edge.m_center.x = hsizeX - aCornerRadius;
    curr_edge.m_center.y = curr_edge.m_end.y;
    curr_edge.m_arc_angle_start = aOrient + ANGLE_0;

    rr_outline.push_back( curr_edge );

    // bottom left corner 4 (and 3 to 4 bottom horizontal side @ y=hsizeY)
    curr_edge.m_start.x = hsizeX - aCornerRadius;
    curr_edge.m_start.y = hsizeY;
    curr_edge.m_end.x =  -hsizeX + aCornerRadius;
    curr_edge.m_end.y = curr_edge.m_start.y;
    curr_edge.m_center.x = curr_edge.m_end.x;
    curr_edge.m_center.y = hsizeY - aCornerRadius;
    curr_edge.m_arc_angle_start = aOrient - ANGLE_90;

    rr_outline.push_back( curr_edge );

    // Move relative coordinates to the actual location and rotation:
    VECTOR2I  arc_last_center;
    EDA_ANGLE arc_last_angle = curr_edge.m_arc_angle_start - ANGLE_90;

    for( RR_EDGE& rr_edge: rr_outline )
    {
        RotatePoint( rr_edge.m_start, aOrient );
        RotatePoint( rr_edge.m_end, aOrient );
        RotatePoint( rr_edge.m_center, aOrient );
        rr_edge.m_start += aRectCenter;
        rr_edge.m_end += aRectCenter;
        rr_edge.m_center += aRectCenter;
        arc_last_center = rr_edge.m_center;
    }

    // Ensure the region is a closed polygon, i.e. the end point of last segment
    // (end of arc) is the same as the first point. Rounding issues can create a
    // small difference, mainly for rotated pads.
    // calculate last point (end of last arc):
    VECTOR2I last_pt;
    last_pt.x = arc_last_center.x + KiROUND( aCornerRadius * arc_last_angle.Cos() );
    last_pt.y = arc_last_center.y - KiROUND( aCornerRadius * arc_last_angle.Sin() );

    VECTOR2I first_pt = rr_outline[0].m_start;

#if 0    // For test only:
    if( last_pt != first_pt )
        wxLogMessage( wxS( "first pt %d %d last pt %d %d" ),
                      first_pt.x, first_pt.y, last_pt.x, last_pt.y );
#endif

    fmt::println( m_outputFile, "G36*" );  // Start region
    fmt::println( m_outputFile, "G01*" ); // Set linear interpolation.
    first_pt = last_pt;
    MoveTo( first_pt );             // Start point of region, must be same as end point

    for( RR_EDGE& rr_edge: rr_outline )
    {
        if( aCornerRadius )     // Guard: ensure we do not create arcs with radius = 0
        {
            // LineTo( rr_edge.m_end ); // made in plotArc()
            plotArc( rr_edge.m_center, -rr_edge.m_arc_angle_start,
                     -rr_edge.m_arc_angle_start + ANGLE_90, aCornerRadius, true );
        }
        else
        {
            LineTo( rr_edge.m_end );
        }
    }

    fmt::println( m_outputFile, "G37*" ); // Close region
}


void GERBER_PLOTTER::FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                     const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                     void* aData )
{
    // A Pad custom is plotted as polygon (a region in Gerber language).
    GBR_METADATA gbr_metadata;

    if( aData )
        gbr_metadata = *static_cast<GBR_METADATA*>( aData );

    SHAPE_POLY_SET polyshape = aPolygons->CloneDropTriangulation();

    std::vector<VECTOR2I> cornerList;

    for( int cnt = 0; cnt < polyshape.OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = polyshape.Outline( cnt );

        cornerList.clear();

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

        // Close polygon
        cornerList.push_back( cornerList[0] );

#ifdef GBR_USE_MACROS_FOR_CUSTOM_PAD
        if( m_gerberDisableApertMacros || cornerList.size() > GBR_MACRO_FOR_CUSTOM_PAD_MAX_CORNER_COUNT )
        {
            PlotGerberRegion( cornerList, &gbr_metadata );
        }
        else
        {
            // An AM will be created. the shape must be in position 0,0 and orientation 0
            // to be able to reuse the same AM for pads having the same shape
            for( size_t ii = 0; ii < cornerList.size(); ii++ )
            {
                cornerList[ii] -= aPadPos;
                RotatePoint( cornerList[ii], -aOrient );
            }

            VECTOR2D pos_dev = userToDeviceCoordinates( aPadPos );
            selectAperture( cornerList, aOrient, APERTURE::AM_FREE_POLYGON,
                            gbr_metadata.GetApertureAttrib(),
                            gbr_metadata.GetCustomAttribute() );
            formatNetAttribute( &gbr_metadata.m_NetlistMetadata );

            emitDcode( pos_dev, 3 );
        }
#else
        PlotGerberRegion( cornerList, &gbr_metadata );
#endif
    }
}


void GERBER_PLOTTER::FlashPadChamferRoundRect( const VECTOR2I& aShapePos, const VECTOR2I& aPadSize,
                                               int aCornerRadius, double aChamferRatio,
                                               int aChamferPositions, const EDA_ANGLE& aPadOrient,
                                               void* aData )
{
    GBR_METADATA gbr_metadata;

    if( aData )
        gbr_metadata = *static_cast<GBR_METADATA*>( aData );

    VECTOR2D              pos_device = userToDeviceCoordinates( aShapePos );
    SHAPE_POLY_SET        outline;
    std::vector<VECTOR2I> cornerList;

    bool hasRoundedCorner = aCornerRadius != 0 && aChamferPositions != 15;

#ifdef GBR_USE_MACROS_FOR_CHAMFERED_RECT
    // Round rect shape or Apert Macros disabled
    if( hasRoundedCorner || m_gerberDisableApertMacros )
#endif
    {
        TransformRoundChamferedRectToPolygon( outline, aShapePos, aPadSize, aPadOrient,
                                              aCornerRadius, aChamferRatio, aChamferPositions, 0,
                                              GetPlotterArcHighDef(), ERROR_INSIDE );

        // Build the corner list
        const SHAPE_LINE_CHAIN& corners = outline.Outline(0);

        for( int ii = 0; ii < corners.PointCount(); ii++ )
            cornerList.emplace_back( corners.CPoint( ii ).x, corners.CPoint( ii ).y );

        // Close the polygon
        cornerList.push_back( cornerList[0] );

#ifdef GBR_USE_MACROS_FOR_CHAMFERED_ROUND_RECT
        if( m_gerberDisableApertMacros )
        {
            PlotGerberRegion( cornerList, &gbr_metadata );
        }
        else
        {
            // An AM will be created. the shape must be in position 0,0 and orientation 0
            // to be able to reuse the same AM for pads having the same shape
            for( size_t ii = 0; ii < cornerList.size(); ii++ )
            {
                cornerList[ii] -= aShapePos;
                RotatePoint( cornerList[ii], -aPadOrient );
            }

            selectAperture( cornerList, aPadOrient, APERTURE::AM_FREE_POLYGON,
                            gbr_metadata.GetApertureAttrib(),
                            gbr_metadata.GetCustomAttribute() );
            formatNetAttribute( &gbr_metadata.m_NetlistMetadata );

            emitDcode( pos_device, 3 );
        }
#else
        PlotGerberRegion( cornerList, &gbr_metadata );
#endif

        return;
    }

    // Build the chamfered polygon (4 to 8 corners )
    TransformRoundChamferedRectToPolygon( outline, VECTOR2I( 0, 0 ), aPadSize, ANGLE_0, 0,
                                          aChamferRatio, aChamferPositions, 0,
                                          GetPlotterArcHighDef(), ERROR_INSIDE );

    // Build the corner list
    const SHAPE_LINE_CHAIN& corners = outline.Outline(0);

    // Generate the polygon (4 to 8 corners )
    for( int ii = 0; ii < corners.PointCount(); ii++ )
        cornerList.emplace_back( corners.CPoint( ii ).x, corners.CPoint( ii ).y );

    switch( cornerList.size() )
    {
    case 4:
        m_hasApertureOutline4P = true;
        selectAperture( cornerList, aPadOrient, APERTURE::APER_MACRO_OUTLINE4P,
                        gbr_metadata.GetApertureAttrib(), gbr_metadata.GetCustomAttribute() );
        break;

    case 5:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient, APERTURE::APER_MACRO_OUTLINE5P,
                        gbr_metadata.GetApertureAttrib(), gbr_metadata.GetCustomAttribute() );
        break;

    case 6:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient, APERTURE::APER_MACRO_OUTLINE6P,
                        gbr_metadata.GetApertureAttrib(), gbr_metadata.GetCustomAttribute() );
        break;

    case 7:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient, APERTURE::APER_MACRO_OUTLINE7P,
                        gbr_metadata.GetApertureAttrib(), gbr_metadata.GetCustomAttribute() );
        break;

    case 8:
        m_hasApertureChamferedRect = true;
        selectAperture( cornerList, aPadOrient, APERTURE::APER_MACRO_OUTLINE8P,
                        gbr_metadata.GetApertureAttrib(), gbr_metadata.GetCustomAttribute() );
        break;

    default:
        wxLogMessage( wxS( "FlashPadChamferRoundRect(): Unexpected number of corners (%d)" ),
                      (int)cornerList.size() );
        break;
    }

    formatNetAttribute( &gbr_metadata.m_NetlistMetadata );

    emitDcode( pos_device, 3 );
}


void GERBER_PLOTTER::FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                     const EDA_ANGLE& aPadOrient, void* aData )
{
    // polygon corners list
    std::vector<VECTOR2I> cornerList = { aCorners[0], aCorners[1], aCorners[2], aCorners[3] };

    // Draw the polygon and fill the interior as required
    for( unsigned ii = 0; ii < 4; ii++ )
    {
        RotatePoint( cornerList[ii], aPadOrient );
        cornerList[ii] += aPadPos;
    }

    // Close the polygon
    cornerList.push_back( cornerList[0] );

    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );
    GBR_METADATA  metadata;

    if( gbr_metadata )
        metadata = *gbr_metadata;

    // Plot a filled polygon:
#ifdef GBR_USE_MACROS_FOR_TRAPEZOID
    if( !m_gerberDisableApertMacros )
#endif
    {
        m_hasApertureOutline4P = true;
        VECTOR2D pos_dev = userToDeviceCoordinates( aPadPos );

        // polygon corners list
        std::vector<VECTOR2I> corners = { aCorners[0], aCorners[1], aCorners[2], aCorners[3] };
        int                   aperture_attribute = 0;
        std::string           custom_attribute = "";

        if( gbr_metadata )
        {
            aperture_attribute = gbr_metadata->GetApertureAttrib();
            custom_attribute = gbr_metadata->GetCustomAttribute();
        }

        selectAperture( corners, aPadOrient, APERTURE::APER_MACRO_OUTLINE4P, aperture_attribute,
                        custom_attribute );

        if( gbr_metadata )
            formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

        emitDcode( pos_dev, 3 );
        return;
    }

    PlotGerberRegion( cornerList, &metadata );
}


void GERBER_PLOTTER::FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter,
                                          int aCornerCount, const EDA_ANGLE& aOrient,
                                          void* aData )
{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    GBR_METADATA metadata;

    if( gbr_metadata )
        metadata = *gbr_metadata;

    APERTURE::APERTURE_TYPE apert_type =
            ( APERTURE::APERTURE_TYPE )( APERTURE::AT_REGULAR_POLY3 + aCornerCount - 3 );

    wxASSERT( apert_type >= APERTURE::APERTURE_TYPE::AT_REGULAR_POLY3
              && apert_type <= APERTURE::APERTURE_TYPE::AT_REGULAR_POLY12 );

    selectApertureWithAttributes( aShapePos, gbr_metadata, VECTOR2I( 0, 0 ), aDiameter / 2,
                                  aOrient, apert_type );
}


void GERBER_PLOTTER::Text( const VECTOR2I&        aPos,
                           const COLOR4D&         aColor,
                           const wxString&        aText,
                           const EDA_ANGLE&       aOrient,
                           const VECTOR2I&        aSize,
                           enum GR_TEXT_H_ALIGN_T aH_justify,
                           enum GR_TEXT_V_ALIGN_T aV_justify,
                           int                    aWidth,
                           bool                   aItalic,
                           bool                   aBold,
                           bool                   aMultilineAllowed,
                           KIFONT::FONT*          aFont,
                           const KIFONT::METRICS& aFontMetrics,
                           void*                  aData )
{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth,
                   aItalic, aBold, aMultilineAllowed, aFont, aFontMetrics, aData );
}


void GERBER_PLOTTER::PlotText( const VECTOR2I&        aPos,
                               const COLOR4D&         aColor,
                               const wxString&        aText,
                               const TEXT_ATTRIBUTES& aAttributes,
                               KIFONT::FONT*          aFont,
                               const KIFONT::METRICS& aFontMetrics,
                               void*                  aData )
{
    GBR_METADATA* gbr_metadata = static_cast<GBR_METADATA*>( aData );

    if( gbr_metadata )
        formatNetAttribute( &gbr_metadata->m_NetlistMetadata );

    PLOTTER::PlotText( aPos, aColor, aText, aAttributes, aFont, aFontMetrics, aData );
}


void GERBER_PLOTTER::SetLayerPolarity( bool aPositive )
{
    if( aPositive )
        fmt::println( m_outputFile, "%LPD*%" );
    else
        fmt::println( m_outputFile, "%LPC*%" );
}


bool APER_MACRO_FREEPOLY::IsSamePoly( const std::vector<VECTOR2I>& aPolygon ) const
{
    return polyCompare( m_Corners, aPolygon );
}


void APER_MACRO_FREEPOLY::Format( FILE * aOutput, double aIu2GbrMacroUnit )
{
    // Write aperture header
    fmt::println( aOutput, "%AM{}{}*", AM_FREEPOLY_BASENAME, m_Id );
    fmt::print( aOutput, "4,1,{},", (int) m_Corners.size() );

    // Insert a newline after curr_line_count_max coordinates.
    int curr_line_corner_count = 0;
    const int curr_line_count_max = 20;     // <= 0 to disable newlines

    for( size_t ii = 0; ii <= m_Corners.size(); ii++ )
    {
        int jj = ii;

        if( ii >= m_Corners.size() )
            jj = 0;

        // Note: parameter values are always mm or inches
        fmt::print( aOutput, "{:#f},{:#f},",
                 m_Corners[jj].x * aIu2GbrMacroUnit, -m_Corners[jj].y * aIu2GbrMacroUnit );

        if( curr_line_count_max >= 0 && ++curr_line_corner_count >= curr_line_count_max )
        {
            fmt::println( aOutput, "" );
            curr_line_corner_count = 0;
        }
    }

    // output rotation parameter
    fmt::println( aOutput, "$1*%" );
}


void APER_MACRO_FREEPOLY_LIST::Format( FILE * aOutput, double aIu2GbrMacroUnit )
{
    for( int idx = 0; idx < AmCount(); idx++ )
        m_AMList[idx].Format( aOutput, aIu2GbrMacroUnit );
}


void APER_MACRO_FREEPOLY_LIST::Append( const std::vector<VECTOR2I>& aPolygon )
{
    m_AMList.emplace_back( aPolygon, AmCount() );
}


int APER_MACRO_FREEPOLY_LIST::FindAm( const std::vector<VECTOR2I>& aPolygon ) const
{
    for( int idx = 0; idx < AmCount(); idx++ )
    {
        if( m_AMList[idx].IsSamePoly( aPolygon ) )
            return idx;
    }

    return -1;
}
