/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "sch_easyeda_parser.h"

#include <sch_io/sch_io_mgr.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_label.h>
#include <sch_junction.h>
#include <sch_edit_frame.h>
#include <sch_shape.h>
#include <sch_bitmap.h>
#include <string_utils.h>
#include <bezier_curves.h>
#include <wx/base64.h>
#include <wx/mstream.h>
#include <core/map_helpers.h>
#include <gfx_import_utils.h>
#include <import_gfx/svg_import_plugin.h>
#include <import_gfx/graphics_importer_lib_symbol.h>
#include <import_gfx/graphics_importer_sch.h>


// clang-format off
static const std::vector<wxString> c_attributesWhitelist = { "Datasheet",
                                                             "Manufacturer Part",
                                                             "Manufacturer",
                                                             "BOM_Manufacturer Part",
                                                             "BOM_Manufacturer",
                                                             "Supplier Part",
                                                             "Supplier",
                                                             "BOM_Supplier Part",
                                                             "BOM_Supplier",
                                                             "LCSC Part Name" };
// clang-format on


SCH_EASYEDA_PARSER::SCH_EASYEDA_PARSER( SCHEMATIC*         aSchematic,
                                        PROGRESS_REPORTER* aProgressReporter )
{
    m_schematic = aSchematic;
}


SCH_EASYEDA_PARSER::~SCH_EASYEDA_PARSER()
{
}


static std::vector<std::vector<wxString>> RegexMatchAll( wxRegEx& aRegex, const wxString& aString )
{
    std::vector<std::vector<wxString>> allMatches;

    size_t start = 0;
    size_t len = 0;
    size_t prevstart = 0;

    wxString str = aString;

    while( aRegex.Matches( str ) )
    {
        std::vector<wxString> matches;
        aRegex.GetMatch( &start, &len );

        for( size_t i = 0; i < aRegex.GetMatchCount(); i++ )
            matches.emplace_back( aRegex.GetMatch( str, i ) );

        allMatches.emplace_back( matches );

        prevstart = start + len;
        str = str.Mid( prevstart );
    }

    return allMatches;
}


/**
 * EasyEDA image transformations are in the form of:
 *
 * scale(1,-1) translate(-555,1025) rotate(270,425,-785)
 *
 * Order of operations is from end to start, similar to CSS.
 */
static std::vector<std::pair<wxString, std::vector<double>>>
ParseImageTransform( const wxString& transformData )
{
    wxRegEx transformRegex( "(rotate|translate|scale)\\(([\\w\\s,\\.\\-]*)\\)", wxRE_ICASE );
    std::vector<std::vector<wxString>> allMatches = RegexMatchAll( transformRegex, transformData );

    std::vector<std::pair<wxString, std::vector<double>>> transformCmds;

    for( int cmdId = allMatches.size() - 1; cmdId >= 0; cmdId-- )
    {
        std::vector<wxString>& groups = allMatches[cmdId];

        if( groups.size() != 3 )
            continue;

        const wxString& cmdName = groups[1].Strip( wxString::both ).Lower();
        const wxString& cmdArgsStr = groups[2];

        wxArrayString       cmdParts = wxSplit( cmdArgsStr, ',', '\0' );
        std::vector<double> cmdArgs;

        for( const wxString& cmdPart : cmdParts )
        {
            double arg = 0;
            wxASSERT( cmdPart.Strip( wxString::both ).ToCDouble( &arg ) );

            cmdArgs.push_back( arg );
        }

        transformCmds.emplace_back( cmdName, cmdArgs );
    }

    return transformCmds;
}


static LIB_ID EasyEdaToKiCadLibID( const wxString& aLibName, const wxString& aLibReference )
{
    wxString libReference = EscapeString( aLibReference, CTX_LIBID );

    wxString key = !aLibName.empty() ? ( aLibName + ':' + libReference ) : libReference;

    LIB_ID libId;
    libId.Parse( key, true );

    return libId;
}


static LINE_STYLE ConvertStrokeStyle( const wxString& aStyle )
{
    if( aStyle == wxS( "0" ) )
        return LINE_STYLE::SOLID;
    else if( aStyle == wxS( "1" ) )
        return LINE_STYLE::DASH;
    else if( aStyle == wxS( "2" ) )
        return LINE_STYLE::DOT;

    return LINE_STYLE::DEFAULT;
}


static ELECTRICAL_PINTYPE ConvertElecType( const wxString& aType )
{
    if( aType == wxS( "0" ) )
        return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
    else if( aType == wxS( "1" ) )
        return ELECTRICAL_PINTYPE::PT_INPUT;
    else if( aType == wxS( "2" ) )
        return ELECTRICAL_PINTYPE::PT_OUTPUT;
    else if( aType == wxS( "3" ) )
        return ELECTRICAL_PINTYPE::PT_BIDI;
    else if( aType == wxS( "4" ) )
        return ELECTRICAL_PINTYPE::PT_PASSIVE;

    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}


VECTOR2I HelperGeneratePowerPortGraphics( LIB_SYMBOL* aKsymbol, EASYEDA::POWER_FLAG_STYLE aStyle,
                                          REPORTER* aReporter )
{
    if( aStyle == EASYEDA::POWER_FLAG_STYLE::CIRCLE || aStyle == EASYEDA::POWER_FLAG_STYLE::ARROW )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 50 ) } );
        aKsymbol->AddDrawItem( line1, false );

        if( aStyle == EASYEDA::POWER_FLAG_STYLE::CIRCLE )
        {
            SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
            circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 5 ), LINE_STYLE::SOLID ) );
            circle->SetPosition( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 75 ) } );
            circle->SetEnd( circle->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 25 ), 0 ) );
            aKsymbol->AddDrawItem( circle, false );
        }
        else
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( 50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 25 ), schIUScale.MilsToIU( 50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( 50 ) } );
            aKsymbol->AddDrawItem( line2, false );
        }

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
    else if( aStyle == EASYEDA::POWER_FLAG_STYLE::WAVE )
    {
        SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line->AddPoint( { 0, 0 } );
        line->AddPoint( { 0, schIUScale.MilsToIU( 72 ) } );
        aKsymbol->AddDrawItem( line, false );

        SCH_SHAPE* bezier = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
        bezier->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 5 ), LINE_STYLE::SOLID ) );
        bezier->SetStart( { schIUScale.MilsToIU( 30 ), schIUScale.MilsToIU( 50 ) } );
        bezier->SetBezierC1( { schIUScale.MilsToIU( 30 ), schIUScale.MilsToIU( 87 ) } );
        bezier->SetBezierC2( { schIUScale.MilsToIU( -30 ), schIUScale.MilsToIU( 63 ) } );
        bezier->SetEnd( { schIUScale.MilsToIU( -30 ), schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( bezier, false );

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
    else if( aStyle == EASYEDA::POWER_FLAG_STYLE::POWER_GROUND
             || aStyle == EASYEDA::POWER_FLAG_STYLE::SIGNAL_GROUND
             || aStyle == EASYEDA::POWER_FLAG_STYLE::EARTH
             || aStyle == EASYEDA::POWER_FLAG_STYLE::GOST_ARROW )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( line1, false );

        if( aStyle == EASYEDA::POWER_FLAG_STYLE::POWER_GROUND )
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) } );
            aKsymbol->AddDrawItem( line2, false );

            SCH_SHAPE* line3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line3->AddPoint( { schIUScale.MilsToIU( -70 ), schIUScale.MilsToIU( 120 ) } );
            line3->AddPoint( { schIUScale.MilsToIU( 70 ), schIUScale.MilsToIU( 120 ) } );
            aKsymbol->AddDrawItem( line3, false );

            SCH_SHAPE* line4 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line4->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line4->AddPoint( { schIUScale.MilsToIU( -40 ), schIUScale.MilsToIU( 140 ) } );
            line4->AddPoint( { schIUScale.MilsToIU( 40 ), schIUScale.MilsToIU( 140 ) } );
            aKsymbol->AddDrawItem( line4, false );

            SCH_SHAPE* line5 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line5->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line5->AddPoint( { schIUScale.MilsToIU( -10 ), schIUScale.MilsToIU( 160 ) } );
            line5->AddPoint( { schIUScale.MilsToIU( 10 ), schIUScale.MilsToIU( 160 ) } );
            aKsymbol->AddDrawItem( line5, false );
        }
        else if( aStyle == EASYEDA::POWER_FLAG_STYLE::SIGNAL_GROUND )
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 160 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            aKsymbol->AddDrawItem( line2, false );
        }
        else if( aStyle == EASYEDA::POWER_FLAG_STYLE::EARTH )
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -150 ), schIUScale.MilsToIU( 200 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 200 ) } );
            aKsymbol->AddDrawItem( line2, false );

            SCH_SHAPE* line3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line3->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 100 ) } );
            line3->AddPoint( { schIUScale.MilsToIU( -50 ), schIUScale.MilsToIU( 200 ) } );
            aKsymbol->AddDrawItem( line3, false );
        }
        else // EASYEDA::POWER_FLAG_STYLE::GOST_ARROW
        {
            SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
            line2->AddPoint( { schIUScale.MilsToIU( -25 ), schIUScale.MilsToIU( 50 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 100 ) } );
            line2->AddPoint( { schIUScale.MilsToIU( 25 ), schIUScale.MilsToIU( 50 ) } );
            aKsymbol->AddDrawItem( line2, false );

            return { 0, schIUScale.MilsToIU( 150 ) }; // special case
        }

        return { 0, schIUScale.MilsToIU( 250 ) };
    }
    else if( aStyle == EASYEDA::POWER_FLAG_STYLE::GOST_POWER_GROUND
             || aStyle == EASYEDA::POWER_FLAG_STYLE::GOST_EARTH )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 160 ) } );
        aKsymbol->AddDrawItem( line1, false );

        SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 160 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 160 ) } );
        aKsymbol->AddDrawItem( line2, false );

        SCH_SHAPE* line3 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line3->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line3->AddPoint( { schIUScale.MilsToIU( -60 ), schIUScale.MilsToIU( 200 ) } );
        line3->AddPoint( { schIUScale.MilsToIU( 60 ), schIUScale.MilsToIU( 200 ) } );
        aKsymbol->AddDrawItem( line3, false );

        SCH_SHAPE* line4 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line4->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line4->AddPoint( { schIUScale.MilsToIU( -20 ), schIUScale.MilsToIU( 240 ) } );
        line4->AddPoint( { schIUScale.MilsToIU( 20 ), schIUScale.MilsToIU( 240 ) } );
        aKsymbol->AddDrawItem( line4, false );

        if( aStyle == EASYEDA::POWER_FLAG_STYLE::GOST_POWER_GROUND )
            return { 0, schIUScale.MilsToIU( -300 ) };

        SCH_SHAPE* circle = new SCH_SHAPE( SHAPE_T::CIRCLE, LAYER_DEVICE );
        circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        circle->SetPosition( { schIUScale.MilsToIU( 0 ), schIUScale.MilsToIU( 160 ) } );
        circle->SetEnd( circle->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 120 ), 0 ) );
        aKsymbol->AddDrawItem( circle, false );

        return { 0, schIUScale.MilsToIU( 350 ) };
    }
    else if( aStyle == EASYEDA::POWER_FLAG_STYLE::GOST_BAR )
    {
        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 200 ) } );
        aKsymbol->AddDrawItem( line1, false );

        SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -100 ), schIUScale.MilsToIU( 200 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 200 ) } );
        aKsymbol->AddDrawItem( line2, false );

        return { 0, schIUScale.MilsToIU( 250 ) };
    }
    else
    {
        if( aStyle != EASYEDA::POWER_FLAG_STYLE::BAR )
        {
            aReporter->Report( _( "Power Port with unknown style imported as 'Bar' type." ),
                               RPT_SEVERITY_WARNING );
        }

        SCH_SHAPE* line1 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line1->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line1->AddPoint( { 0, 0 } );
        line1->AddPoint( { 0, schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( line1, false );

        SCH_SHAPE* line2 = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
        line2->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        line2->AddPoint( { schIUScale.MilsToIU( -50 ), schIUScale.MilsToIU( 100 ) } );
        line2->AddPoint( { schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 100 ) } );
        aKsymbol->AddDrawItem( line2, false );

        return { 0, schIUScale.MilsToIU( 150 ) };
    }
}


void SCH_EASYEDA_PARSER::ParseSymbolShapes( LIB_SYMBOL*                  aSymbol,
                                            std::map<wxString, wxString> paramMap,
                                            wxArrayString                aShapes )
{
    for( wxString shapeStr : aShapes )
    {
        wxArrayString arr = wxSplit( shapeStr, '~', '\0' );

        wxString elType = arr[0];
        if( elType == wxS( "PL" ) || elType == wxS( "PG" ) )
        {
            wxArrayString  ptArr = wxSplit( arr[1], ' ', '\0' );
            wxString       strokeColor = arr[2];
            double         lineWidth = Convert( arr[3] );
            LINE_STYLE     strokeStyle = ConvertStrokeStyle( arr[4] );
            wxString       fillColor = arr[5].Lower();
            //bool           locked = arr[7] != wxS( "0" );

            SHAPE_LINE_CHAIN chain;

            for( size_t i = 1; i < ptArr.size(); i += 2 )
            {
                chain.Append(
                        RelPosSym( VECTOR2I( Convert( ptArr[i - 1] ), Convert( ptArr[i] ) ) ) );
            }

            auto line = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

            if( elType == wxS( "PG" ) )
                chain.SetClosed( true );

            if( chain.PointCount() < 2 )
                continue;

            for( int i = 0; i < chain.PointCount(); i++ )
                line->AddPoint( chain.CPoint( i ) );

            if( chain.IsClosed() )
                line->AddPoint( chain.CPoint( 0 ) );

            line->SetUnit( 0 );
            line->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

            if( fillColor != wxS( "none" ) )
            {
                line->SetFilled( true );

                if( fillColor == strokeColor )
                    line->SetFillMode( FILL_T::FILLED_SHAPE );
                else
                    line->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
            }

            aSymbol->AddDrawItem( line.release() );
        }
        else if( elType == wxS( "PT" ) ) // Freedraw
        {
            wxString   pointsData = arr[1];
            wxString   strokeColor = arr[2];
            double     lineWidth = Convert( arr[3] );
            LINE_STYLE strokeStyle = ConvertStrokeStyle( arr[4] );
            wxString   fillColor = arr[5].Lower();
            //bool       locked = arr[7] != wxS( "0" );

            std::vector<SHAPE_LINE_CHAIN> lineChains =
                    ParseLineChains( pointsData, schIUScale.MilsToIU( 10 ), false );

            for( SHAPE_LINE_CHAIN outline : lineChains )
            {
                auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

                if( outline.IsClosed() )
                    outline.Append( outline.CPoint( 0 ), true );

                for( const VECTOR2I& pt : outline.CPoints() )
                    shape->AddPoint( pt );

                shape->SetUnit( 0 );
                shape->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

                if( fillColor != wxS( "none" ) )
                {
                    shape->SetFilled( true );

                    if( fillColor == strokeColor )
                        shape->SetFillMode( FILL_T::FILLED_SHAPE );
                    else
                        shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
                }

                aSymbol->AddDrawItem( shape.release() );
            }
        }
        else if( elType == wxS( "Pimage" ) )
        {
            //double   angle = Convert( arr[4] ); // TODO
            VECTOR2D start( Convert( arr[6] ), Convert( arr[7] ) );
            VECTOR2D size( Convert( arr[8] ), Convert( arr[9] ) );
            wxString imageUrl = arr[10];

            if( imageUrl.BeforeFirst( ':' ) == wxS( "data" ) )
            {
                wxArrayString paramsArr =
                        wxSplit( imageUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                wxString data = imageUrl.AfterFirst( ',' );

                if( paramsArr.size() > 0 )
                {
                    wxString       mimeType = paramsArr[0];
                    wxMemoryBuffer buf = wxBase64Decode( data );

                    if( mimeType == wxS( "image/svg+xml" ) )
                    {
                        VECTOR2D offset = RelPosSym( start );

                        SVG_IMPORT_PLUGIN            svgImportPlugin;
                        GRAPHICS_IMPORTER_LIB_SYMBOL libsymImporter( aSymbol, 0 );

                        svgImportPlugin.SetImporter( &libsymImporter );
                        svgImportPlugin.LoadFromMemory( buf );

                        VECTOR2D imSize( svgImportPlugin.GetImageWidth(),
                                         svgImportPlugin.GetImageHeight() );

                        VECTOR2D pixelScale( schIUScale.IUTomm( ScaleSize( size.x ) ) / imSize.x,
                                             schIUScale.IUTomm( ScaleSize( size.y ) ) / imSize.y );

                        libsymImporter.SetScale( pixelScale );

                        VECTOR2D offsetMM( schIUScale.IUTomm( offset.x ),
                                           schIUScale.IUTomm( offset.y ) );

                        libsymImporter.SetImportOffsetMM( offsetMM );

                        svgImportPlugin.Import();

                        for( std::unique_ptr<EDA_ITEM>& item : libsymImporter.GetItems() )
                            aSymbol->AddDrawItem( static_cast<SCH_ITEM*>( item.release() ) );
                    }
                    else
                    {
                        wxMemoryInputStream memis( buf.GetData(), buf.GetDataLen() );

                        wxImage::SetDefaultLoadFlags( wxImage::GetDefaultLoadFlags()
                                                      & ~wxImage::Load_Verbose );
                        wxImage img;
                        if( img.LoadFile( memis, mimeType ) )
                        {
                            int    dimMul = img.GetWidth() * img.GetHeight();
                            double maxPixels = 30000;

                            if( dimMul > maxPixels )
                            {
                                double scale = sqrt( maxPixels / dimMul );
                                img.Rescale( img.GetWidth() * scale, img.GetHeight() * scale );
                            }

                            VECTOR2D pixelScale( ScaleSize( size.x ) / img.GetWidth(),
                                                 ScaleSize( size.y ) / img.GetHeight() );

                            ConvertImageToLibShapes( aSymbol, 0, img, pixelScale,
                                                     RelPosSym( start ) );
                        }
                    }
                }
            }
        }
        else if( elType == wxS( "A" ) )
        {
            wxString   data = arr[1];
            wxString   strokeColor = arr[3];
            double     lineWidth = Convert( arr[4] );
            LINE_STYLE strokeStyle = ConvertStrokeStyle( arr[5] );
            wxString   fillColor = arr[6].Lower();
            //bool       locked = arr[8] != wxS( "0" );

            std::vector<SHAPE_LINE_CHAIN> chains =
                    ParseLineChains( data, schIUScale.MilsToIU( 10 ), false );

            auto transform = []( VECTOR2I aVec )
            {
                return VECTOR2I( aVec.x, aVec.y );
            };

            for( const SHAPE_LINE_CHAIN& chain : chains )
            {
                for( int i = 0; i <= chain.PointCount() && i != -1; i = chain.NextShape( i ) )
                {
                    if( chain.IsArcStart( i ) )
                    {
                        SHAPE_ARC arc = chain.Arc( chain.ArcIndex( i ) );

                        std::unique_ptr<SCH_SHAPE> shape =
                                std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE );

                        shape->SetArcGeometry( transform( arc.GetP0() ),
                                               transform( arc.GetArcMid() ),
                                               transform( arc.GetP1() ) );

                        shape->SetUnit( 0 );
                        shape->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

                        if( fillColor != wxS( "none" ) )
                        {
                            shape->SetFilled( true );

                            if( fillColor == strokeColor )
                                shape->SetFillMode( FILL_T::FILLED_SHAPE );
                            else
                                shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
                        }

                        aSymbol->AddDrawItem( shape.release() );
                    }
                    else
                    {
                        SEG seg = chain.CSegment( i );

                        std::unique_ptr<SCH_SHAPE> shape =
                                std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

                        shape->AddPoint( transform( seg.A ) );
                        shape->AddPoint( transform( seg.B ) );

                        shape->SetUnit( 0 );
                        shape->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

                        aSymbol->AddDrawItem( shape.release() );
                    }
                }
            }
        }
        else if( elType == wxS( "R" ) )
        {
            VECTOR2D start( Convert( arr[1] ), Convert( arr[2] ) );

            VECTOR2D cr;
            cr.x = !arr[3].empty() ? Convert( arr[3] ) : 0,
            cr.y = !arr[4].empty() ? Convert( arr[4] ) : 0; // TODO: corner radius

            VECTOR2D   size( Convert( arr[5] ), Convert( arr[6] ) );
            wxString   strokeColor = arr[7];
            double     lineWidth = Convert( arr[8] );
            LINE_STYLE strokeStyle = ConvertStrokeStyle( arr[9] );
            wxString   fillColor = arr[10].Lower();
            //bool       locked = arr[12] != wxS( "0" );

            //if( cr.x == 0 )
            {
                auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );

                rect->SetStart( RelPosSym( start ) );
                rect->SetEnd( RelPosSym( start + size ) );

                rect->SetUnit( 0 );
                rect->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

                if( fillColor != wxS( "none" ) )
                {
                    rect->SetFilled( true );

                    if( fillColor == strokeColor )
                        rect->SetFillMode( FILL_T::FILLED_SHAPE );
                    else
                        rect->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
                }

                aSymbol->AddDrawItem( rect.release() );
            }
            // TODO: rounded rectangles
        }
        else if( elType == wxS( "E" ) )
        {
            auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );

            VECTOR2D   center( Convert( arr[1] ), Convert( arr[2] ) );
            VECTOR2D   radius( Convert( arr[3] ), Convert( arr[4] ) ); // TODO: corner radius
            wxString   strokeColor = arr[5];
            double     lineWidth = Convert( arr[6] );
            LINE_STYLE strokeStyle = ConvertStrokeStyle( arr[7] );
            wxString   fillColor = arr[8].Lower();
            //bool       locked = arr[10] != wxS( "0" );

            circle->SetCenter( RelPosSym( center ) );
            circle->SetEnd( RelPosSym( center + VECTOR2I( radius.x, 0 ) ) );

            circle->SetUnit( 0 );
            circle->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

            if( fillColor != wxS( "none" ) )
            {
                circle->SetFilled( true );

                if( fillColor == strokeColor )
                    circle->SetFillMode( FILL_T::FILLED_SHAPE );
                else
                    circle->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
            }

            aSymbol->AddDrawItem( circle.release() );
        }
        else if( elType == wxS( "P" ) )
        {
            wxString sepShapeStr = shapeStr;
            sepShapeStr.Replace( wxS( "^^" ), wxS( "\n" ) );

            wxArrayString segments = wxSplit( sepShapeStr, '\n', '\0' );
            wxArrayString mainParts = wxSplit( segments[0], '~', '\0' );
            wxArrayString pinDotParts = wxSplit( segments[1], '~', '\0' );
            wxArrayString pinPathColorParts = wxSplit( segments[2], '~', '\0' );
            wxArrayString pinNameParts = wxSplit( segments[3], '~', '\0' );
            wxArrayString pinNumParts = wxSplit( segments[4], '~', '\0' );

            //bool               show = mainParts[1].Lower() != wxS( "none" );
            ELECTRICAL_PINTYPE elecType = ConvertElecType( mainParts[2] );
            wxString           pinNumber = mainParts[3];
            VECTOR2D           pinPos( Convert( mainParts[4] ), Convert( mainParts[5] ) );
            //int                pinRot = Convert( mainParts[6] );

            VECTOR2D pinDotPos( Convert( pinDotParts[0] ), Convert( pinDotParts[1] ) );

            bool     nameVisible = pinNameParts[0] != wxS( "0" );
            wxString pinName = pinNameParts[4];

            bool     numVisible = pinNumParts[0] != wxS( "0" );
            // wxString ptSize = pinNumParts[4];

            VECTOR2D startPoint;
            bool     vertical = false;
            double   pinLen = 0;

            // M360,290h10 or M 420 300 h -5
            wxString lineData = pinPathColorParts[0];
            wxRegEx  regex( wxS( "^M\\s*([-\\d.]+)[,\\s]([-\\d.]+)\\s*([h|v])\\s*([-\\d.]+)\\s*$" ) );

            if( regex.Matches( lineData ) )
            {
                startPoint.x = Convert( regex.GetMatch( lineData, 1 ) );
                startPoint.y = Convert( regex.GetMatch( lineData, 2 ) );

                vertical = regex.GetMatch( lineData, 3 ).Contains( wxS( "v" ) );
                pinLen = Convert( regex.GetMatch( lineData, 4 ) );
            }

            int pinRotation = 0;

            if( !vertical )
            {
                if( startPoint.x == pinPos.x && pinLen < 0 )
                    pinRotation = 180;
                else if( startPoint.x == pinPos.x && pinLen > 0 )
                    pinRotation = 0;
                else if( startPoint.x != pinPos.x && pinLen < 0 )
                    pinRotation = 0;
                else if( startPoint.x != pinPos.x && pinLen > 0 )
                    pinRotation = 180;
            }
            else
            {
                if( startPoint.y == pinPos.y && pinLen < 0 )
                    pinRotation = 90;
                else if( startPoint.y == pinPos.y && pinLen > 0 )
                    pinRotation = 270;
                else if( startPoint.y != pinPos.y && pinLen < 0 )
                    pinRotation = 270;
                else if( startPoint.y != pinPos.y && pinLen > 0 )
                    pinRotation = 90;
            }

            PIN_ORIENTATION orient = PIN_ORIENTATION::PIN_RIGHT;

            if( pinRotation == 0 )
                orient = PIN_ORIENTATION::PIN_RIGHT;
            else if( pinRotation == 90 )
                orient = PIN_ORIENTATION::PIN_UP;
            else if( pinRotation == 180 )
                orient = PIN_ORIENTATION::PIN_LEFT;
            else if( pinRotation == 270 )
                orient = PIN_ORIENTATION::PIN_DOWN;

            int pinUnit = 0;
            int kPinLen = ScaleSize( std::abs( pinLen ) );

            if( segments.size() > 5 )
            {
                wxArrayString dotParts = wxSplit( segments[5], '~', '\0' );
                wxArrayString clockParts = wxSplit( segments[6], '~', '\0' );

                if( dotParts.size() == 3 && clockParts.size() == 2 )
                {
                    if( dotParts[0] == wxS( "1" ) )
                    {
                        VECTOR2D dotPos( Convert( dotParts[1] ), Convert( dotParts[2] ) );

                        auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );

                        circle->SetCenter( RelPosSym( dotPos ) );
                        circle->SetEnd( RelPosSym(
                                pinPos + ( dotPos - pinPos ).Resize( std::abs( pinLen ) ) ) );

                        circle->SetUnit( 0 );

                        aSymbol->AddDrawItem( circle.release() );
                    }

                    if( clockParts[0] == wxS( "1" ) )
                    {
                        std::vector<SHAPE_LINE_CHAIN> lineChains =
                                ParseLineChains( clockParts[1], schIUScale.MilsToIU( 10 ), false );

                        for( SHAPE_LINE_CHAIN outline : lineChains )
                        {
                            auto shape = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

                            if( outline.IsClosed() )
                                outline.Append( outline.CPoint( 0 ), true );

                            for( const VECTOR2I& pt : outline.CPoints() )
                                shape->AddPoint( pt );

                            shape->SetUnit( 0 );

                            aSymbol->AddDrawItem( shape.release() );
                        }
                    }
                }
            }

            std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( aSymbol );

            pin->SetName( pinName );
            pin->SetNumber( pinNumber );
            pin->SetOrientation( orient );
            pin->SetType( elecType );
            pin->SetLength( kPinLen );
            pin->SetPosition( RelPosSym( pinPos ) );
            pin->SetUnit( pinUnit );

            if( pin->GetNumberTextSize() * int( pinNumber.size() ) > kPinLen )
                pin->SetNumberTextSize( kPinLen / pinNumber.size() );

            if( !nameVisible )
                pin->SetNameTextSize( schIUScale.MilsToIU( 1 ) );

            if( !numVisible )
                pin->SetNumberTextSize( schIUScale.MilsToIU( 1 ) );

            aSymbol->AddDrawItem( pin.release() );
        }
        else if( elType == wxS( "T" ) )
        {
            wxString textType = arr[1];
            VECTOR2D pos( Convert( arr[2] ), Convert( arr[3] ) );
            int      angle = Convert( arr[4] );
            // wxString color = arr[5];
            wxString fontname = arr[6];
            wxString fontSize = arr[7];
            wxString baselineAlign = arr[10];
            wxString textStr = arr[12];
            bool     visible = arr[13] != wxS( "0" );

            textStr.Replace( wxS( "\\n" ), wxS( "\n" ) );
            textStr = UnescapeHTML( textStr );

            wxString halignStr = arr[14]; // Empty, start, middle, end, inherit

            bool      added = false;
            EDA_TEXT* textItem = nullptr;

            if( textType == wxS( "P" ) )
            {
                textItem = &aSymbol->GetReferenceField();
                textItem->SetTextPos( RelPosSym( pos ) );
                textItem->SetText( textStr );
            }
            else if( textType == wxS( "N" ) )
            {
                textItem = &aSymbol->GetValueField();
                textItem->SetTextPos( RelPosSym( pos ) );
                textItem->SetText( textStr );
            }
            else
            {
                textItem = new SCH_TEXT( RelPosSym( pos ), textStr, LAYER_DEVICE );
                added = true;
            }

            textItem->SetTextAngleDegrees( ( 360 - angle ) % 360 );
            textItem->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

            if( halignStr == wxS( "middle" ) )
                textItem->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            else if( halignStr == wxS( "end" ) )
                textItem->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            else
                textItem->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            textItem->SetFont( KIFONT::FONT::GetFont( fontname ) );
            textItem->SetVisible( visible );

            double ptSize = 7;

            if( !fontSize.IsEmpty() )
            {
                if( fontSize.EndsWith( wxS( "pt" ) ) )
                    ptSize = Convert( fontSize.BeforeFirst( 'p' ) );
                else if( fontSize.IsNumber() )
                    ptSize = Convert( fontSize );
            }

            double ktextSize = ScaleSize( ptSize );

            if( textStr.Contains( wxS( "\n" ) ) )
                ktextSize *= 0.8;
            else
                ktextSize *= 0.95;

            textItem->SetTextSize( VECTOR2I( ktextSize, ktextSize ) );

            TransformTextToBaseline( textItem, baselineAlign );

            if( added )
                aSymbol->AddDrawItem( dynamic_cast<SCH_ITEM*>( textItem ) );
        }
    }
}


LIB_SYMBOL* SCH_EASYEDA_PARSER::ParseSymbol( const VECTOR2D&              aOrigin,
                                             std::map<wxString, wxString> aParams,
                                             wxArrayString                aShapes )
{
    std::unique_ptr<LIB_SYMBOL> ksymbol = std::make_unique<LIB_SYMBOL>( wxEmptyString );

    m_relOrigin = aOrigin;

    std::optional<wxString> valOpt;
    wxString                symbolName = wxS( "Unknown" );

    if( ( valOpt = get_opt( aParams, wxS( "name" ) ) ) )
        symbolName = *valOpt;
    else if( ( valOpt = get_opt( aParams, wxS( "spiceSymbolName" ) ) ) )
        symbolName = *valOpt;

    wxString symbolPrefix;

    if( ( valOpt = get_opt( aParams, wxS( "pre" ) ) ) )
        symbolPrefix = *valOpt;
    else if( ( valOpt = get_opt( aParams, wxS( "spicePre" ) ) ) )
        symbolPrefix = *valOpt;

    LIB_ID libId = EasyEdaToKiCadLibID( wxEmptyString, symbolName );

    ksymbol->SetLibId( libId );
    ksymbol->SetName( symbolName );

    ksymbol->GetReferenceField().SetText( symbolPrefix );
    ksymbol->GetValueField().SetText( symbolName );

    for( wxString attrName : c_attributesWhitelist )
    {
        wxString srcName = attrName;

        if( srcName == wxS( "Datasheet" ) )
            srcName = wxS( "link" );

        if( ( valOpt = get_opt( aParams, srcName ) ) )
        {
            if( valOpt->empty() )
                continue;

            SCH_FIELD* fd = ksymbol->FindFieldCaseInsensitive( attrName );

            if( !fd )
            {
                fd = new SCH_FIELD( ksymbol.get(), FIELD_T::USER, attrName );
                ksymbol->AddField( fd );
            }

            fd->SetText( *valOpt );
            fd->SetVisible( false );
        }
    }

    ParseSymbolShapes( ksymbol.get(), aParams, aShapes );

    return ksymbol.release();
}

std::pair<LIB_SYMBOL*, bool> SCH_EASYEDA_PARSER::MakePowerSymbol( const wxString& aFlagTypename,
                                                                  const wxString& aNetname )
{
    std::unique_ptr<LIB_SYMBOL> ksymbol = std::make_unique<LIB_SYMBOL>( wxEmptyString );

    m_relOrigin = VECTOR2D();

    LIB_ID libId = EasyEdaToKiCadLibID( wxEmptyString, aNetname );

    ksymbol->SetGlobalPower();
    ksymbol->SetLibId( libId );
    ksymbol->SetName( aNetname );
    ksymbol->GetReferenceField().SetText( wxS( "#PWR" ) );
    ksymbol->GetReferenceField().SetVisible( false );
    ksymbol->GetValueField().SetText( aNetname );
    ksymbol->GetValueField().SetVisible( true );
    ksymbol->SetDescription( wxString::Format( _( "Power symbol creates a global "
                                                  "label with name '%s'" ),
                                               aNetname ) );
    ksymbol->SetKeyWords( wxS( "power-flag" ) );
    ksymbol->SetShowPinNames( false );
    ksymbol->SetShowPinNumbers( false );

    std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( ksymbol.get() );

    pin->SetName( aNetname );
    pin->SetNumber( wxS( "1" ) );
    pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
    pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    pin->SetLength( 0 );

    ksymbol->AddDrawItem( pin.release() );

    EASYEDA::POWER_FLAG_STYLE flagStyle = EASYEDA::POWER_FLAG_STYLE::POWER_GROUND;

    bool flip = false;

    if( aFlagTypename == wxS( "part_netLabel_gnD" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::POWER_GROUND;
    }
    else if( aFlagTypename == wxS( "part_netLabel_GNd" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::SIGNAL_GROUND;
    }
    else if( aFlagTypename == wxS( "part_netLabel_gNd" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::BAR;
    }
    else if( aFlagTypename == wxS( "part_netLabel_GnD" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::EARTH;
    }
    else if( aFlagTypename == wxS( "part_netLabel_VCC" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::BAR;
        flip = true;
    }
    else if( aFlagTypename == wxS( "part_netLabel_+5V" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::BAR;
        flip = true;
    }
    else if( aFlagTypename == wxS( "part_netLabel_VEE" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::BAR;
    }
    else if( aFlagTypename == wxS( "part_netLabel_-5V" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::BAR;
    }
    else if( aFlagTypename == wxS( "part_netLabel_Bar" ) )
    {
        flagStyle = EASYEDA::POWER_FLAG_STYLE::CIRCLE;
        flip = true;
    }

    VECTOR2I valueFieldPos = HelperGeneratePowerPortGraphics( ksymbol.get(), flagStyle, nullptr );
    ksymbol->GetValueField().SetPosition( valueFieldPos );

    return std::make_pair( ksymbol.release(), flip );
}

void SCH_EASYEDA_PARSER::ParseSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                                         const wxString& aFileName, wxArrayString aShapes )
{
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> loadedSymbols;
    std::map<wxString, int>                         namesCounter;
    std::vector<std::unique_ptr<SCH_ITEM>>          createdItems;

    for( wxString shap : aShapes )
    {
        shap.Replace( wxS( "#@$" ), wxS( "\n" ) );
        wxArrayString parts = wxSplit( shap, '\n', '\0' );

        if( parts.size() < 1 )
            continue;

        wxArrayString arr = wxSplit( parts[0], '~', '\0' );

        if( arr.size() < 1 )
            continue;

        wxString rootType = arr[0];

        if( rootType == wxS( "LIB" ) )
        {
            if( arr.size() < 4 )
                continue;

            VECTOR2D origin( Convert( arr[1] ), Convert( arr[2] ) );

            wxString symbolName = wxString::Format( wxS( "Unknown_%s_%s" ), arr[1], arr[2] );

            wxArrayString paramParts = wxSplit( arr[3], '`', '\0' );

            std::map<wxString, wxString> paramMap;

            for( size_t i = 1; i < paramParts.size(); i += 2 )
            {
                wxString key = paramParts[i - 1];
                wxString value = paramParts[i];

                if( key == wxS( "spiceSymbolName" ) && !value.IsEmpty() )
                    symbolName = value;

                paramMap[key] = value;
            }

            int& serial = namesCounter[symbolName];

            if( serial > 0 )
                symbolName << wxS( "_" ) << serial;

            serial++;

            paramMap[wxS( "spiceSymbolName" )] = symbolName;

            parts.RemoveAt( 0 );

            VECTOR2D pcbOrigin = m_relOrigin;

            LIB_SYMBOL* sym = ParseSymbol( origin, paramMap, parts );
            loadedSymbols.emplace( symbolName, sym );

            wxString referenceStr = sym->GetReferenceField().GetText();

            m_relOrigin = pcbOrigin;
            LIB_ID libId = EasyEdaToKiCadLibID( wxEmptyString, symbolName );

            std::unique_ptr<SCH_SYMBOL> schSym =
                    std::make_unique<SCH_SYMBOL>( *sym, libId, &aSchematic->CurrentSheet(), 0 );

            schSym->SetPosition( RelPos( origin ) );
            schSym->SetRef( &aSchematic->CurrentSheet(), referenceStr );

            createdItems.push_back( std::move( schSym ) );
        }
        else if( rootType == wxS( "F" ) )
        {
            wxString sepShapeStr = parts[0];
            sepShapeStr.Replace( wxS( "^^" ), wxS( "\n" ) );

            wxArrayString segments = wxSplit( sepShapeStr, '\n', '\0' );
            // wxArrayString mainParts = wxSplit( segments[0], '~', '\0' );
            // wxArrayString uselessParts = wxSplit( segments[1], '~', '\0' );
            wxArrayString valueParts = wxSplit( segments[2], '~', '\0' );

            wxString flagTypename = arr[1];
            VECTOR2D pos( Convert( arr[2] ), Convert( arr[3] ) );
            double   angle = Convert( arr[4] );

            wxString netnameValue = valueParts[0];
            VECTOR2D valuePos( Convert( valueParts[2] ), Convert( valueParts[3] ) );
            double   textAngle = Convert( valueParts[4] );
            wxString halignStr = valueParts[5];
            wxString valueFontname = valueParts[7];
            wxString valueFontsize = valueParts[8];

            if( flagTypename == wxS( "part_netLabel_netPort" ) )
            {
                std::unique_ptr<SCH_GLOBALLABEL> label =
                        std::make_unique<SCH_GLOBALLABEL>( RelPos( pos ), netnameValue );

                SPIN_STYLE spin = SPIN_STYLE::LEFT;

                for( double i = angle; i > 0; i -= 90 )
                    spin = spin.RotateCCW();

                // If the shape was mirrored, we can't rely on angle value to determine direction.
                if( segments.size() > 3 )
                {
                    wxArrayString shapeParts = wxSplit( segments[3], '~', '\0' );
                    if( shapeParts[0] == wxS( "PL" ) )
                    {
                        wxArrayString ptArr = wxSplit( shapeParts[1], ' ', '\0' );

                        SHAPE_LINE_CHAIN chain;

                        for( size_t i = 1; i < ptArr.size(); i += 2 )
                        {
                            chain.Append( RelPos(
                                    VECTOR2I( Convert( ptArr[i - 1] ), Convert( ptArr[i] ) ) ) );
                        }

                        chain.Move( -RelPos( pos ) );

                        VECTOR2I shapeCenter = chain.Centre();

                        if( std::abs( shapeCenter.x ) >= std::abs( shapeCenter.y ) )
                        {
                            if( shapeCenter.x >= 0 )
                                spin = SPIN_STYLE::RIGHT;
                            else
                                spin = SPIN_STYLE::LEFT;
                        }
                        else
                        {
                            if( shapeCenter.y >= 0 )
                                spin = SPIN_STYLE::BOTTOM;
                            else
                                spin = SPIN_STYLE::UP;
                        }
                    }
                }

                label->SetSpinStyle( spin );
                label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );

                createdItems.push_back( std::move( label ) );
            }
            else
            {
                auto        pair = MakePowerSymbol( flagTypename, netnameValue );
                LIB_SYMBOL* pwrLibSym = pair.first;
                bool        flip = pair.second;

                LIB_ID libId = EasyEdaToKiCadLibID( wxEmptyString, netnameValue );

                std::unique_ptr<SCH_SYMBOL> schSym = std::make_unique<SCH_SYMBOL>(
                        *pwrLibSym, libId, &aSchematic->CurrentSheet(), 0 );

                if( flip )
                    schSym->SetOrientation( SYM_MIRROR_X );

                if( angle == 0 )
                {
                }
                else if( angle == 90 )
                {
                    schSym->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                }
                if( angle == 180 )
                {
                    schSym->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                    schSym->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                }
                if( angle == 270 )
                {
                    schSym->SetOrientation( SYM_ROTATE_CLOCKWISE );
                }

                schSym->SetPosition( RelPos( pos ) );

                SCH_FIELD* valField = schSym->GetField( FIELD_T::VALUE );

                valField->SetPosition( RelPos( valuePos ) );
                valField->SetTextAngleDegrees( textAngle - angle );

                if( !flip )
                    valField->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                else
                    valField->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

                if( halignStr == wxS( "middle" ) )
                    valField->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                else if( halignStr == wxS( "end" ) )
                    valField->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                else
                    valField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

                if( flip && ( angle == 90 || angle == 270 ) )
                {
                    if( valField->GetVertJustify() == GR_TEXT_V_ALIGN_BOTTOM )
                        valField->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                    else if( valField->GetVertJustify() == GR_TEXT_V_ALIGN_TOP )
                        valField->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

                    if( valField->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                        valField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                    else if( valField->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT )
                        valField->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );

                    if( flagTypename == wxS( "part_netLabel_Bar" ) ) // "Circle"
                        valField->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
                }

                if( angle == 0 && flagTypename == wxS( "part_netLabel_Bar" ) ) // "Circle"
                    valField->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

                valField->SetFont( KIFONT::FONT::GetFont( valueFontname ) );

                double ptSize = 7;

                if( valueFontsize.EndsWith( wxS( "pt" ) ) )
                    ptSize = Convert( valueFontsize.BeforeFirst( 'p' ) );

                double ktextSize = ScaleSize( ptSize );

                if( netnameValue.Contains( wxS( "\n" ) ) )
                    ktextSize *= 0.8;
                else
                    ktextSize *= 0.95;

                valField->SetTextSize( VECTOR2I( ktextSize, ktextSize ) );

                //TransformTextToBaseline( valField, wxS( "" ), true );

                createdItems.push_back( std::move( schSym ) );
            }
        }
        else if( rootType == wxS( "W" ) )
        {
            wxArrayString ptArr = wxSplit( arr[1], ' ', '\0' );
            //wxString      strokeColor = arr[2];
            //double        lineWidth = Convert( arr[3] );
            //LINE_STYLE    strokeStyle = ConvertStrokeStyle( arr[4] );
            //wxString      fillColor = arr[5].Lower();
            //bool          locked = arr[7] != wxS( "0" );

            SHAPE_LINE_CHAIN chain;

            for( size_t i = 1; i < ptArr.size(); i += 2 )
                chain.Append( VECTOR2I( RelPosX( ptArr[i - 1] ), RelPosY( ptArr[i] ) ) );

            for( int segId = 0; segId < chain.SegmentCount(); segId++ )
            {
                const SEG& seg = chain.CSegment( segId );

                std::unique_ptr<SCH_LINE> line = std::make_unique<SCH_LINE>( seg.A, LAYER_WIRE );
                line->SetEndPoint( seg.B );

                createdItems.push_back( std::move( line ) );
            }
        }
        else if( rootType == wxS( "N" ) )
        {
            VECTOR2D pos( Convert( arr[1] ), Convert( arr[2] ) );
            double   angle = Convert( arr[3] );
            wxString netname = arr[5];
            wxString halignStr = arr[7];
            VECTOR2D text_pos( Convert( arr[8] ),
                               Convert( arr[9] ) ); // TODO: detect top/bottom align

            // wxString fontname = arr[10];
            // wxString fontSize = arr[11];

            std::unique_ptr<SCH_LABEL> label =
                    std::make_unique<SCH_LABEL>( RelPos( pos ), netname );

            if( halignStr == wxS( "middle" ) )
                label->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            else if( halignStr == wxS( "end" ) )
                label->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            else
                label->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            for( int left = angle; left > 0; left -= 90 )
                label->Rotate90( false );

            createdItems.push_back( std::move( label ) );
        }
        else if( rootType == wxS( "O" ) )
        {
            VECTOR2D pos( Convert( arr[1] ), Convert( arr[2] ) );

            std::unique_ptr<SCH_NO_CONNECT> noConn =
                    std::make_unique<SCH_NO_CONNECT>( RelPos( pos ) );

            createdItems.push_back( std::move( noConn ) );
        }
        else if( rootType == wxS( "J" ) )
        {
            VECTOR2D pos( Convert( arr[1] ), Convert( arr[2] ) );
            //double   dia = Convert( arr[3] );

            std::unique_ptr<SCH_JUNCTION> junction =
                    std::make_unique<SCH_JUNCTION>( RelPos( pos ) /*, ScaleSizeUnit( dia )*/ );

            createdItems.push_back( std::move( junction ) );
        }
        else if( rootType == wxS( "T" ) )
        {
            VECTOR2D pos( Convert( arr[2] ), Convert( arr[3] ) );
            int      angle = Convert( arr[4] );
            // wxString color = arr[5];
            wxString fontname = arr[6];
            wxString fontSize = arr[7];
            wxString baselineAlign = arr[10];
            wxString textStr = arr[12];

            textStr.Replace( wxS( "\\n" ), wxS( "\n" ) );
            textStr = UnescapeHTML( textStr );

            wxString halignStr = arr[14]; // Empty, start, middle, end, inherit

            std::unique_ptr<SCH_TEXT> textItem =
                    std::make_unique<SCH_TEXT>( RelPos( pos ), textStr );

            textItem->SetTextAngleDegrees( ( 360 - angle ) % 360 );
            textItem->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

            if( halignStr == wxS( "middle" ) )
                textItem->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
            else if( halignStr == wxS( "end" ) )
                textItem->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            else
                textItem->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );

            textItem->SetFont( KIFONT::FONT::GetFont( fontname ) );

            double ptSize = 7;

            if( fontSize.EndsWith( wxS( "pt" ) ) )
                ptSize = Convert( fontSize.BeforeFirst( 'p' ) );

            double ktextSize = ScaleSize( ptSize );

            if( textStr.Contains( wxS( "\n" ) ) )
                ktextSize *= 0.8;
            else
                ktextSize *= 0.95;

            textItem->SetTextSize( VECTOR2I( ktextSize, ktextSize ) );

            TransformTextToBaseline( textItem.get(), baselineAlign );

            createdItems.push_back( std::move( textItem ) );
        }
        else if( rootType == wxS( "R" ) )
        {
            VECTOR2D start( Convert( arr[1] ), Convert( arr[2] ) );

            VECTOR2D cr;
            cr.x = !arr[3].empty() ? Convert( arr[3] ) : 0,
            cr.y = !arr[4].empty() ? Convert( arr[4] ) : 0; // TODO: corner radius

            VECTOR2D   size( Convert( arr[5] ), Convert( arr[6] ) );
            wxString   strokeColor = arr[7];
            double     lineWidth = Convert( arr[8] );
            LINE_STYLE strokeStyle = ConvertStrokeStyle( arr[9] );
            wxString   fillColor = arr[10].Lower();
            //bool       locked = arr[12] != wxS( "0" );

            //if( cr.x == 0 )
            {
                std::unique_ptr<SCH_SHAPE> rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE );

                rect->SetStart( RelPos( start ) );
                rect->SetEnd( RelPos( start + size ) );

                rect->SetStroke( STROKE_PARAMS( ScaleSize( lineWidth ), strokeStyle ) );

                if( fillColor != wxS( "none" ) )
                {
                    rect->SetFilled( true );

                    if( fillColor == strokeColor )
                        rect->SetFillMode( FILL_T::FILLED_SHAPE );
                    else
                        rect->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
                }

                createdItems.push_back( std::move( rect ) );
            }
            /*else
            {
            }*/
        }
        else if( rootType == wxS( "I" ) )
        {
            VECTOR2D start( Convert( arr[1] ), Convert( arr[2] ) );
            VECTOR2D size( Convert( arr[3] ), Convert( arr[4] ) );
            //double   angle = Convert( arr[5] ); // CSS-like transformations are used instead.
            wxString imageUrl = arr[6];
            wxString transformData = arr[9];

            VECTOR2D kstart = RelPos( start );
            VECTOR2D ksize = ScalePos( size );

            std::vector<std::pair<wxString, std::vector<double>>> transformCmds =
                    ParseImageTransform( transformData );

            auto applyTransform = [&]( SCH_ITEM* aSchItem )
            {
                for( const auto& cmd : transformCmds )
                {
                    if( cmd.first == wxS( "rotate" ) )
                    {
                        if( cmd.second.size() != 3 )
                            continue;

                        double   cmdAngle = 360 - cmd.second[0];
                        VECTOR2D cmdAround( cmd.second[1], cmd.second[2] );

                        for( double i = cmdAngle; i > 0; i -= 90 )
                        {
                            if( aSchItem->Type() == SCH_LINE_T )
                            {
                                // Lines need special handling for some reason
                                aSchItem->SetFlags( STARTPOINT );
                                aSchItem->Rotate( RelPos( cmdAround ), false );
                                aSchItem->ClearFlags( STARTPOINT );

                                aSchItem->SetFlags( ENDPOINT );
                                aSchItem->Rotate( RelPos( cmdAround ), false );
                                aSchItem->ClearFlags( ENDPOINT );
                            }
                            else
                            {
                                aSchItem->Rotate( RelPos( cmdAround ), false );
                            }
                        }
                    }
                    else if( cmd.first == wxS( "translate" ) )
                    {
                        if( cmd.second.size() != 2 )
                            continue;

                        VECTOR2D cmdOffset( cmd.second[0], cmd.second[1] );
                        aSchItem->Move( ScalePos( cmdOffset ) );
                    }
                    else if( cmd.first == wxS( "scale" ) )
                    {
                        if( cmd.second.size() != 2 )
                            continue;

                        double cmdScaleX = cmd.second[0];
                        double cmdScaleY = cmd.second[1];

                        // Lines need special handling for some reason
                        if( aSchItem->Type() == SCH_LINE_T )
                            aSchItem->SetFlags( STARTPOINT | ENDPOINT );

                        if( cmdScaleX < 0 && cmdScaleY > 0 )
                        {
                            aSchItem->MirrorHorizontally( 0 );
                        }
                        else if( cmdScaleX > 0 && cmdScaleY < 0 )
                        {
                            aSchItem->MirrorVertically( 0 );
                        }
                        else if( cmdScaleX < 0 && cmdScaleY < 0 )
                        {
                            aSchItem->MirrorHorizontally( 0 );
                            aSchItem->MirrorVertically( 0 );
                        }

                        if( aSchItem->Type() == SCH_LINE_T )
                            aSchItem->ClearFlags( STARTPOINT | ENDPOINT );
                    }
                }
            };

            if( imageUrl.BeforeFirst( ':' ) == wxS( "data" ) )
            {
                wxArrayString paramsArr =
                        wxSplit( imageUrl.AfterFirst( ':' ).BeforeFirst( ',' ), ';', '\0' );

                wxString data = imageUrl.AfterFirst( ',' );

                if( paramsArr.size() > 0 )
                {
                    wxString       mimeType = paramsArr[0];
                    wxMemoryBuffer buf = wxBase64Decode( data );

                    if( mimeType == wxS( "image/svg+xml" ) )
                    {
                        VECTOR2D offset = RelPos( start );

                        SVG_IMPORT_PLUGIN     svgImportPlugin;
                        GRAPHICS_IMPORTER_SCH schImporter;

                        svgImportPlugin.SetImporter( &schImporter );
                        svgImportPlugin.LoadFromMemory( buf );

                        VECTOR2D imSize( svgImportPlugin.GetImageWidth(),
                                         svgImportPlugin.GetImageHeight() );

                        VECTOR2D pixelScale( schIUScale.IUTomm( ScaleSize( size.x ) ) / imSize.x,
                                             schIUScale.IUTomm( ScaleSize( size.y ) ) / imSize.y );

                        schImporter.SetScale( pixelScale );

                        VECTOR2D offsetMM( schIUScale.IUTomm( offset.x ),
                                           schIUScale.IUTomm( offset.y ) );

                        schImporter.SetImportOffsetMM( offsetMM );

                        svgImportPlugin.Import();

                        for( std::unique_ptr<EDA_ITEM>& item : schImporter.GetItems() )
                        {
                            SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item.release() );

                            applyTransform( schItem );

                            createdItems.emplace_back( schItem );
                        }
                    }
                    else
                    {
                        std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>();
                        REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

                        wxImage::SetDefaultLoadFlags( wxImage::GetDefaultLoadFlags()
                                                      & ~wxImage::Load_Verbose );

                        if( refImage.ReadImageFile( buf ) )
                        {
                            VECTOR2D kcenter = kstart + ksize / 2;

                            double scaleFactor = ScaleSize( size.x ) / refImage.GetSize().x;
                            refImage.SetImageScale( scaleFactor );
                            bitmap->SetPosition( kcenter );

                            applyTransform( bitmap.get() );

                            createdItems.push_back( std::move( bitmap ) );
                        }
                    }
                }
            }
        }
    }

    BOX2I sheetBBox;

    for( std::unique_ptr<SCH_ITEM>& ptr : createdItems )
        if( ptr->Type() == SCH_SYMBOL_T )
            sheetBBox.Merge( static_cast<SCH_SYMBOL*>( ptr.get() )->GetBodyBoundingBox() );

    SCH_SCREEN* screen = aRootSheet->GetScreen();
    PAGE_INFO   pageInfo = screen->GetPageSettings();

    int alignGrid = schIUScale.MilsToIU( 50 );

    VECTOR2D offset( -sheetBBox.GetLeft(), -sheetBBox.GetTop() );
    offset.x = KiROUND( offset.x / alignGrid ) * alignGrid;
    offset.y = KiROUND( offset.y / alignGrid ) * alignGrid;

    pageInfo.SetWidthMils( schIUScale.IUToMils( sheetBBox.GetWidth() ) );
    pageInfo.SetHeightMils( schIUScale.IUToMils( sheetBBox.GetHeight() ) );

    screen->SetPageSettings( pageInfo );

    for( std::unique_ptr<SCH_ITEM>& ptr : createdItems )
    {
        ptr->Move( offset );
        screen->Append( ptr.release() );
    }
}
