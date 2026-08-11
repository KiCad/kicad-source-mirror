/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#include <sch_io/pads/pads_sch_binary_builder.h>

#include <bitmap_base.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <connection_graph.h>
#include <embedded_files.h>
#include <font/font.h>
#include <page_info.h>
#include <pin_type.h>
#include <sch_bus_entry.h>
#include <sch_bitmap.h>
#include <sch_field.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <sch_io/ole_image.h>
#include <stroke_params.h>
#include <title_block.h>

#include <ki_exception.h>
#include <wildcards_and_files_ext.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <wx/buffer.h>
#include <wx/image.h>
#include <wx/log.h>

namespace PADS_SCH_BINARY
{
namespace
{

    int toIU( int64_t aHalfMils )
    {
        return schIUScale.MilsToIU( static_cast<double>( aHalfMils ) / 2.0 );
    }


    VECTOR2I localPoint( const SOURCE_POINT& aPoint )
    {
        return { toIU( aPoint.x ), -toIU( aPoint.y ) };
    }


    VECTOR2I pagePoint( const SOURCE_POINT& aPoint, int aPageHeight )
    {
        return { toIU( aPoint.x ), aPageHeight - toIU( aPoint.y ) };
    }


    std::unique_ptr<SCH_BITMAP> makeEmbeddedImage( const MODEL_EMBEDDED_IMAGE& aSource, int aPageHeight,
                                                   std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        if( aSource.size.x <= 0 || aSource.size.y <= 0 )
        {
            aDiagnostics.emplace_back( RPT_SEVERITY_WARNING, aSource.source,
                                       wxS( "embedded OLE image has a degenerate page box and was skipped" ) );
            return nullptr;
        }

        auto             bitmap = std::make_unique<SCH_BITMAP>( pagePoint( aSource.position, aPageHeight ) );
        REFERENCE_IMAGE& refImage = bitmap->GetReferenceImage();
        bool             decoded = false;
        wxMemoryBuffer   buffer;

        switch( aSource.type )
        {
        case MODEL_EMBEDDED_IMAGE_TYPE::BMP:
            buffer.AppendData( aSource.data.data(), aSource.data.size() );
            {
                wxLogNull noLog;
                decoded = refImage.ReadImageFile( buffer );
            }
            break;

        case MODEL_EMBEDDED_IMAGE_TYPE::DIB:
            if( OleMakeBmpFromDib( aSource.data, buffer ) )
            {
                wxLogNull noLog;
                decoded = refImage.ReadImageFile( buffer );
            }
            break;

        case MODEL_EMBEDDED_IMAGE_TYPE::WMF:
        {
            wxImage image;
            int width = std::clamp<int64_t>( std::abs( static_cast<int64_t>( aSource.extent[2] ) - aSource.extent[0] ),
                                             1, 4096 );
            int height = std::clamp<int64_t>( std::abs( static_cast<int64_t>( aSource.extent[3] ) - aSource.extent[1] ),
                                              1, 4096 );

            if( OleRenderWmf( aSource.data, width, height, image ) )
            {
                // Clamp before rounding; llround on an out-of-range double is undefined
                double scaledHeight = static_cast<double>( width ) * aSource.size.y / aSource.size.x;
                int    fittedHeight =
                        static_cast<int>( std::llround( std::clamp( scaledHeight, 1.0, 4096.0 ) ) );
                image.Rescale( width, fittedHeight, wxIMAGE_QUALITY_HIGH );
                decoded = refImage.SetImage( image );
            }

            break;
        }

        case MODEL_EMBEDDED_IMAGE_TYPE::UNSUPPORTED: break;
        }

        if( !decoded )
        {
            aDiagnostics.emplace_back( RPT_SEVERITY_WARNING, aSource.source,
                                       wxS( "embedded OLE image could not be rasterized and was skipped" ) );
            return nullptr;
        }

        const int targetWidth = toIU( aSource.size.x );
        const int targetHeight = toIU( aSource.size.y );

        if( targetWidth <= 0 || targetHeight <= 0 )
            THROW_IO_ERROR( FormatParserError( aSource.source, wxS( "embedded image has invalid page size" ) ) );

        const wxImage* decodedImage = refImage.GetImage().GetImageData();

        if( decodedImage && decodedImage->IsOk() )
        {
            const int fittedHeight =
                    std::max( 1, static_cast<int>( std::llround( static_cast<double>( decodedImage->GetWidth() )
                                                                 * targetHeight / targetWidth ) ) );

            if( fittedHeight != decodedImage->GetHeight() )
            {
                wxImage fitted = decodedImage->Copy();
                fitted.Rescale( fitted.GetWidth(), fittedHeight, wxIMAGE_QUALITY_HIGH );
                refImage.SetImage( fitted );
            }
        }

        refImage.SetWidth( targetWidth );

        const VECTOR2I center = bitmap->GetPosition();

        if( aSource.mirrorHorizontal )
            bitmap->MirrorHorizontally( center.x );

        if( aSource.mirrorVertical )
            bitmap->MirrorVertically( center.y );

        return bitmap;
    }


    LINE_STYLE lineStyle( MODEL_LINE_STYLE aStyle )
    {
        switch( aStyle )
        {
        case MODEL_LINE_STYLE::DEFAULT:
        case MODEL_LINE_STYLE::SOLID: return LINE_STYLE::SOLID;
        case MODEL_LINE_STYLE::DASH: return LINE_STYLE::DASH;
        case MODEL_LINE_STYLE::DOT: return LINE_STYLE::DOT;
        case MODEL_LINE_STYLE::DASH_DOT: return LINE_STYLE::DASHDOT;
        }

        return LINE_STYLE::SOLID;
    }


    FILL_T fillStyle( MODEL_FILL_STYLE aFill )
    {
        switch( aFill )
        {
        case MODEL_FILL_STYLE::NONE: return FILL_T::NO_FILL;
        case MODEL_FILL_STYLE::FILLED: return FILL_T::FILLED_SHAPE;
        case MODEL_FILL_STYLE::HATCHED: return FILL_T::HATCH;
        }

        return FILL_T::NO_FILL;
    }


    ELECTRICAL_PINTYPE pinType( uint32_t aType )
    {
        switch( aType )
        {
        case 0: return ELECTRICAL_PINTYPE::PT_PASSIVE;
        case 1: return ELECTRICAL_PINTYPE::PT_INPUT;
        case 2: return ELECTRICAL_PINTYPE::PT_OUTPUT;
        case 3: return ELECTRICAL_PINTYPE::PT_BIDI;
        case 4: return ELECTRICAL_PINTYPE::PT_TRISTATE;
        case 5: return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
        case 6: return ELECTRICAL_PINTYPE::PT_OPENEMITTER;
        case 7: return ELECTRICAL_PINTYPE::PT_POWER_IN;
        default: return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
        }
    }


    GRAPHIC_PINSHAPE pinShape( uint32_t aStyle )
    {
        switch( aStyle )
        {
        case 1: return GRAPHIC_PINSHAPE::INVERTED;
        case 2: return GRAPHIC_PINSHAPE::CLOCK;
        case 3: return GRAPHIC_PINSHAPE::INVERTED_CLOCK;
        default: return GRAPHIC_PINSHAPE::LINE;
        }
    }


    PIN_ORIENTATION pinOrientation( const MODEL_PIN_DEFINITION& aPin )
    {
        const int  angle = NormalizeAngle( aPin.angle );
        const bool verticalDecal = aPin.decalName.text.Contains( wxS( "VRT" ) );

        if( verticalDecal )
            return aPin.side == 2 ? PIN_ORIENTATION::PIN_UP : PIN_ORIENTATION::PIN_DOWN;

        switch( angle )
        {
        case 900: return aPin.side >= 2 ? PIN_ORIENTATION::PIN_DOWN : PIN_ORIENTATION::PIN_UP;
        case 1800: return ( aPin.side & 1 ) != 0 ? PIN_ORIENTATION::PIN_RIGHT : PIN_ORIENTATION::PIN_LEFT;
        case 2700: return aPin.side >= 2 ? PIN_ORIENTATION::PIN_UP : PIN_ORIENTATION::PIN_DOWN;
        default: return ( aPin.side & 1 ) != 0 ? PIN_ORIENTATION::PIN_LEFT : PIN_ORIENTATION::PIN_RIGHT;
        }
    }


    GR_TEXT_H_ALIGN_T horizontalJustification( MODEL_JUSTIFICATION aJustification )
    {
        switch( aJustification )
        {
        case MODEL_JUSTIFICATION::CENTER: return GR_TEXT_H_ALIGN_CENTER;
        case MODEL_JUSTIFICATION::RIGHT: return GR_TEXT_H_ALIGN_RIGHT;
        default: return GR_TEXT_H_ALIGN_LEFT;
        }
    }


    GR_TEXT_V_ALIGN_T verticalJustification( MODEL_JUSTIFICATION aJustification )
    {
        switch( aJustification )
        {
        case MODEL_JUSTIFICATION::LEFT: return GR_TEXT_V_ALIGN_TOP;
        case MODEL_JUSTIFICATION::RIGHT: return GR_TEXT_V_ALIGN_BOTTOM;
        default: return GR_TEXT_V_ALIGN_CENTER;
        }
    }


    void applyTextPresentation( EDA_TEXT* aText, const MODEL_TEXT_PRESENTATION& aPresentation, bool aVisible,
                                std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        if( aPresentation.height > 0 )
            aText->SetTextSize( { toIU( aPresentation.height ), toIU( aPresentation.height ) } );

        aText->SetHorizJustify( horizontalJustification( aPresentation.horizontalJustification ) );
        aText->SetVertJustify( verticalJustification( aPresentation.verticalJustification ) );
        aText->SetBold( aPresentation.bold );
        aText->SetItalic( aPresentation.italic );
        aText->SetVisible( aVisible );

        if( aPresentation.width > 0 )
            aText->SetTextThickness( toIU( aPresentation.width ) );

        if( !aPresentation.font.text.IsEmpty() && aPresentation.font.text != wxS( "Default Font" ) )
            aText->SetFont(
                    KIFONT::FONT::GetFont( aPresentation.font.text, aPresentation.bold, aPresentation.italic ) );

        if( aPresentation.underline )
        {
            aDiagnostics.push_back( MakePropertyDiagnostic( RPT_SEVERITY_WARNING, aPresentation.source,
                                                            wxS( "underline" ), PROPERTY_DISPOSITION::UNSUPPORTED,
                                                            wxS( "PADS underline presentation is unsupported" ) ) );
        }
    }


    std::unique_ptr<SCH_SHAPE> makeShape( const MODEL_GRAPHIC& aGraphic, bool aPageCoordinates, int aPageHeight )
    {
        if( aGraphic.kind == MODEL_GRAPHIC_KIND::TEXT || aGraphic.points.size() < 2 )
            THROW_IO_ERROR( FormatParserError( aGraphic.source, wxS( "graphic has inconsistent geometry" ) ) );

        auto convert = [&]( const SOURCE_POINT& aPoint )
        {
            return aPageCoordinates ? pagePoint( aPoint, aPageHeight ) : localPoint( aPoint );
        };

        SHAPE_T shapeType = SHAPE_T::POLY;

        switch( aGraphic.kind )
        {
        case MODEL_GRAPHIC_KIND::RECTANGLE: shapeType = SHAPE_T::RECTANGLE; break;
        case MODEL_GRAPHIC_KIND::CIRCLE: shapeType = SHAPE_T::CIRCLE; break;
        case MODEL_GRAPHIC_KIND::ARC: shapeType = SHAPE_T::ARC; break;
        default: shapeType = SHAPE_T::POLY; break;
        }

        auto shape = std::make_unique<SCH_SHAPE>( shapeType, aPageCoordinates ? LAYER_NOTES : LAYER_DEVICE );

        if( aGraphic.kind == MODEL_GRAPHIC_KIND::RECTANGLE && aGraphic.points.size() >= 2 )
        {
            shape->SetStart( convert( aGraphic.points[0] ) );
            shape->SetEnd( convert( aGraphic.points[1] ) );
        }
        else if( aGraphic.kind == MODEL_GRAPHIC_KIND::CIRCLE && aGraphic.points.size() >= 2 )
        {
            SOURCE_POINT center;
            center.x = ( aGraphic.points[0].x + aGraphic.points[1].x ) / 2;
            center.y = ( aGraphic.points[0].y + aGraphic.points[1].y ) / 2;
            shape->SetStart( convert( center ) );
            shape->SetEnd( convert( aGraphic.points[0] ) );
        }
        else if( aGraphic.kind == MODEL_GRAPHIC_KIND::ARC && aGraphic.points.size() >= 2 )
        {
            const SOURCE_POINT& start = aGraphic.points.front();
            const SOURCE_POINT& end = aGraphic.points.back();
            double              startAngle = std::atan2( static_cast<double>( start.y - aGraphic.arcCenter.y ),
                                                         static_cast<double>( start.x - aGraphic.arcCenter.x ) );
            double              sweep = std::abs( aGraphic.arcSweepAngle ) * M_PI / 1800.0;

            if( aGraphic.arcClockwise )
                sweep = -sweep;

            double       radius = std::hypot( static_cast<double>( start.x - aGraphic.arcCenter.x ),
                                              static_cast<double>( start.y - aGraphic.arcCenter.y ) );
            SOURCE_POINT mid;
            mid.x = aGraphic.arcCenter.x + std::llround( radius * std::cos( startAngle + sweep / 2.0 ) );
            mid.y = aGraphic.arcCenter.y + std::llround( radius * std::sin( startAngle + sweep / 2.0 ) );
            shape->SetArcGeometry( convert( start ), convert( mid ), convert( end ) );
        }
        else
        {
            for( const SOURCE_POINT& point : aGraphic.points )
                shape->AddPoint( convert( point ) );
        }

        shape->SetStroke( STROKE_PARAMS( toIU( aGraphic.strokeWidth ), lineStyle( aGraphic.lineStyle ) ) );
        shape->SetFillMode( fillStyle( aGraphic.fill ) );
        return shape;
    }


    std::unique_ptr<SCH_TEXT> makeGraphicText( const MODEL_GRAPHIC&            aGraphic,
                                               std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        if( aGraphic.points.empty() )
            THROW_IO_ERROR( FormatParserError( aGraphic.source, wxS( "symbol text has no position" ) ) );

        auto text =
                std::make_unique<SCH_TEXT>( localPoint( aGraphic.points.front() ), aGraphic.text.text, LAYER_DEVICE );
        text->SetTextAngle( EDA_ANGLE( aGraphic.angle, TENTHS_OF_A_DEGREE_T ) );
        applyTextPresentation( text.get(), aGraphic.presentation, aGraphic.presentation.visible, aDiagnostics );
        return text;
    }


    std::unique_ptr<SCH_TEXT> makePageText( const SOURCE_STRING& aText, const SOURCE_POINT& aPosition, int aAngle,
                                            const MODEL_TEXT_PRESENTATION& aPresentation, int aPageHeight,
                                            std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        auto text = std::make_unique<SCH_TEXT>( pagePoint( aPosition, aPageHeight ), aText.text, LAYER_NOTES );
        text->SetTextAngle( EDA_ANGLE( aAngle, TENTHS_OF_A_DEGREE_T ) );
        applyTextPresentation( text.get(), aPresentation, aPresentation.visible, aDiagnostics );
        return text;
    }


    size_t appendPageGraphic( SCH_SCREEN* aScreen, const MODEL_GRAPHIC& aGraphic, int aPageHeight,
                              std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        if( aGraphic.kind == MODEL_GRAPHIC_KIND::TEXT )
        {
            if( aGraphic.points.empty() )
                THROW_IO_ERROR( FormatParserError( aGraphic.source, wxS( "page text has no position" ) ) );

            std::unique_ptr<SCH_TEXT> text = makePageText( aGraphic.text, aGraphic.points.front(), aGraphic.angle,
                                                           aGraphic.presentation, aPageHeight, aDiagnostics );
            aScreen->Append( text.get() );
            text.release();
            return 1;
        }

        if( ( aGraphic.kind == MODEL_GRAPHIC_KIND::LINE || aGraphic.kind == MODEL_GRAPHIC_KIND::POLYLINE )
            && aGraphic.fill == MODEL_FILL_STYLE::NONE )
        {
            if( aGraphic.points.size() < 2 )
                THROW_IO_ERROR(
                        FormatParserError( aGraphic.source, wxS( "page polyline has inconsistent geometry" ) ) );

            for( size_t point = 1; point < aGraphic.points.size(); ++point )
            {
                auto line =
                        std::make_unique<SCH_LINE>( pagePoint( aGraphic.points[point - 1], aPageHeight ), LAYER_NOTES );
                line->SetEndPoint( pagePoint( aGraphic.points[point], aPageHeight ) );
                line->SetStroke( STROKE_PARAMS( toIU( aGraphic.strokeWidth ), lineStyle( aGraphic.lineStyle ) ) );
                aScreen->Append( line.get() );
                line.release();
            }

            return aGraphic.points.size() - 1;
        }

        std::unique_ptr<SCH_SHAPE> shape = makeShape( aGraphic, true, aPageHeight );

        if( aGraphic.fill == MODEL_FILL_STYLE::FILLED )
            shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );

        aScreen->Append( shape.get() );
        shape.release();
        return 1;
    }


    std::unique_ptr<LIB_SYMBOL> makePadsPowerLibrary( const MODEL_LABEL& aLabel )
    {
        wxString name;

        if( aLabel.kind == MODEL_LABEL_KIND::GROUND )
            name = aLabel.symbolVariant == 1   ? wxS( "PADS_GNDA" )
                   : aLabel.symbolVariant == 2 ? wxS( "PADS_GNDCH" )
                                               : wxS( "PADS_GND" );
        else
            name = wxString::Format( wxS( "PADS_POWER_%u" ), aLabel.symbolVariant );

        auto symbol = std::make_unique<LIB_SYMBOL>( name );
        symbol->SetGlobalPower();
        symbol->SetShowPinNumbers( false );
        symbol->SetShowPinNames( false );

        auto mil = []( int aMils )
        {
            return schIUScale.MilsToIU( aMils );
        };
        auto addLine = [&]( std::initializer_list<VECTOR2I> aPoints )
        {
            auto line = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

            for( const VECTOR2I& point : aPoints )
                line->AddPoint( point );

            line->SetStroke( STROKE_PARAMS( mil( 10 ), LINE_STYLE::SOLID ) );
            symbol->AddDrawItem( line.release() );
        };
        auto addTriangle = [&]( int aStemLength )
        {
            addLine( { { 0, 0 }, { 0, mil( aStemLength ) } } );
            auto triangle = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );
            triangle->AddPoint( { 0, mil( aStemLength ) } );
            triangle->AddPoint( { -mil( 50 ), mil( 100 ) } );
            triangle->AddPoint( { mil( 50 ), mil( 100 ) } );
            triangle->AddPoint( { 0, mil( aStemLength ) } );
            triangle->SetStroke( STROKE_PARAMS( mil( 10 ), LINE_STYLE::SOLID ) );
            triangle->SetFillMode( FILL_T::FILLED_SHAPE );
            symbol->AddDrawItem( triangle.release() );
        };

        if( aLabel.kind == MODEL_LABEL_KIND::GROUND )
        {
            switch( aLabel.symbolVariant )
            {
            case 1:
                addLine( { { 0, 0 },
                           { 0, -mil( 50 ) },
                           { -mil( 100 ), -mil( 50 ) },
                           { 0, -mil( 200 ) },
                           { mil( 100 ), -mil( 50 ) },
                           { 0, -mil( 50 ) } } );
                break;

            case 2:
                addLine( { { 0, 0 }, { 0, -mil( 100 ) } } );
                addLine( { { -mil( 100 ), -mil( 100 ) }, { mil( 100 ), -mil( 100 ) } } );
                addLine( { { -mil( 100 ), -mil( 100 ) }, { -mil( 150 ), -mil( 200 ) } } );
                addLine( { { 0, -mil( 100 ) }, { -mil( 50 ), -mil( 200 ) } } );
                addLine( { { mil( 100 ), -mil( 100 ) }, { mil( 50 ), -mil( 200 ) } } );
                break;

            default:
                addLine( { { 0, 0 }, { 0, -mil( 100 ) } } );
                addLine( { { -mil( 100 ), -mil( 100 ) }, { mil( 100 ), -mil( 100 ) } } );
                addLine( { { -mil( 60 ), -mil( 150 ) }, { mil( 60 ), -mil( 150 ) } } );
                addLine( { { -mil( 20 ), -mil( 200 ) }, { mil( 20 ), -mil( 200 ) } } );
                break;
            }
        }
        else if( aLabel.symbolVariant == 1 || aLabel.symbolVariant == 3 )
        {
            addTriangle( 250 );
        }
        else if( aLabel.symbolVariant == 4 )
        {
            addTriangle( 200 );
        }
        else
        {
            addLine( { { 0, 0 }, { 0, mil( 100 ) } } );
            auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );
            circle->SetCenter( { 0, mil( 150 ) } );
            circle->SetEnd( { mil( 50 ), mil( 150 ) } );
            circle->SetStroke( STROKE_PARAMS( mil( 10 ), LINE_STYLE::SOLID ) );
            circle->SetFillMode( FILL_T::NO_FILL );
            symbol->AddDrawItem( circle.release() );
        }

        auto pin = std::make_unique<SCH_PIN>( symbol.get() );
        pin->SetNumber( wxS( "1" ) );
        pin->SetName( name );
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        pin->SetVisible( false );
        pin->SetLength( 0 );
        pin->SetPosition( { 0, 0 } );
        pin->SetOrientation( aLabel.kind == MODEL_LABEL_KIND::GROUND ? PIN_ORIENTATION::PIN_DOWN
                                                                     : PIN_ORIENTATION::PIN_UP );
        symbol->AddDrawItem( pin.release() );
        symbol->GetReferenceField().SetText( wxS( "#PWR" ) );
        symbol->GetReferenceField().SetVisible( false );
        return symbol;
    }


    std::unique_ptr<SCH_SYMBOL> makePowerSymbol( const MODEL_LABEL& aLabel, const SCH_SHEET_PATH& aPath,
                                                 int aPageHeight, size_t aOrdinal,
                                                 std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        int orientation = SYM_ORIENT_180;

        if( aLabel.kind == MODEL_LABEL_KIND::POWER && ( aLabel.symbolVariant == 2 || aLabel.symbolVariant == 3 ) )
        {
            orientation = SYM_ORIENT_0;
        }

        std::unique_ptr<LIB_SYMBOL> library = makePadsPowerLibrary( aLabel );

        if( !library )
            THROW_IO_ERROR( FormatParserError( aLabel.source, wxS( "could not construct power symbol" ) ) );

        auto   symbol = std::make_unique<SCH_SYMBOL>();
        LIB_ID libId;
        libId.SetLibNickname( wxS( "pads_import" ) );
        libId.SetLibItemName( library->GetName() );
        symbol->SetLibId( libId );
        auto libraryCopy = std::make_unique<LIB_SYMBOL>( *library );
        symbol->SetLibSymbol( libraryCopy.release() );
        symbol->SetPosition( pagePoint( aLabel.position, aPageHeight ) );
        symbol->SetOrientation( orientation );
        const wxString reference = wxString::Format( wxS( "#PWR%04zu" ), aOrdinal + 1 );
        symbol->SetRef( &aPath, reference );
        symbol->AddHierarchicalReference( aPath.Path(), reference, 1 );
        symbol->SetValueFieldText( aLabel.text.text, &aPath );

        if( SCH_FIELD* field = symbol->GetField( FIELD_T::REFERENCE ) )
            field->SetVisible( false );

        if( SCH_FIELD* value = symbol->GetField( FIELD_T::VALUE ) )
        {
            value->SetText( aLabel.text.text );
            SOURCE_POINT textPosition = aLabel.position;
            textPosition.x += aLabel.textOffset.x;
            textPosition.y += aLabel.textOffset.y;
            value->SetPosition( pagePoint( textPosition, aPageHeight ) );
            value->SetTextAngle( EDA_ANGLE( aLabel.angle, TENTHS_OF_A_DEGREE_T ) );
            applyTextPresentation( value, aLabel.presentation, aLabel.presentation.visible, aDiagnostics );
        }

        return symbol;
    }


    std::unique_ptr<SCH_PIN> makePin( const MODEL_PIN_DEFINITION& aPin, LIB_SYMBOL* aParent )
    {
        auto pin = std::make_unique<SCH_PIN>( aParent );
        pin->SetNumber( aPin.number.text );
        pin->SetName( aPin.name.text );
        pin->SetPosition( localPoint( aPin.position ) );
        pin->SetOrientation( pinOrientation( aPin ) );
        pin->SetLength( toIU( aPin.length ) );
        pin->SetType( pinType( aPin.electricalType ) );
        pin->SetShape( pinShape( aPin.graphicStyle ) );
        pin->SetVisible( aPin.presentation.visible );

        // toIU already halves, the same as applyTextPresentation; a second /2 here made pin text
        // half the size of every other imported string
        pin->SetNameTextSize( toIU( aPin.namePresentation.height ) );
        pin->SetNumberTextSize( toIU( aPin.numberPresentation.height ) );

        return pin;
    }


    const MODEL_SYMBOL_DEFINITION& definitionById( const PADS_SCH_MODEL& aModel, DEFINITION_ID aId )
    {
        auto definition = std::ranges::find( aModel.definitions, aId, &MODEL_SYMBOL_DEFINITION::id );

        if( definition == aModel.definitions.end() )
            THROW_IO_ERROR( wxS( "resolved definition is missing during schematic staging" ) );

        return *definition;
    }


    const MODEL_PART_TYPE& partById( const PADS_SCH_MODEL& aModel, PART_TYPE_ID aId )
    {
        auto part = std::ranges::find( aModel.partTypes, aId, &MODEL_PART_TYPE::id );

        if( part == aModel.partTypes.end() )
            THROW_IO_ERROR( wxS( "resolved part type is missing during schematic staging" ) );

        return *part;
    }


    const MODEL_PIN_DEFINITION& pinById( const MODEL_SYMBOL_DEFINITION& aDefinition, PIN_ID aId )
    {
        auto pin = std::ranges::find( aDefinition.pins, aId, &MODEL_PIN_DEFINITION::id );

        if( pin == aDefinition.pins.end() )
            THROW_IO_ERROR( wxS( "resolved pin is missing during schematic staging" ) );

        return *pin;
    }


    void addDefinitionUnit( LIB_SYMBOL* aLibrary, const MODEL_SYMBOL_DEFINITION& aDefinition, const MODEL_GATE* aGate,
                            int aUnit, std::vector<PARSER_DIAGNOSTIC>& aDiagnostics,
                            const std::vector<PLACED_PIN_REFERENCE>* aPlacementPins = nullptr,
                            const MODEL_CONNECTOR_PIN*               aConnectorPin = nullptr )
    {
        for( const MODEL_GRAPHIC& graphic : aDefinition.graphics )
        {
            if( graphic.kind == MODEL_GRAPHIC_KIND::TEXT )
            {
                std::unique_ptr<SCH_TEXT> text = makeGraphicText( graphic, aDiagnostics );
                text->SetUnit( aUnit );
                aLibrary->AddDrawItem( text.release() );
            }
            else
            {
                std::unique_ptr<SCH_SHAPE> shape = makeShape( graphic, false, 0 );
                shape->SetUnit( aUnit );
                aLibrary->AddDrawItem( shape.release() );
            }
        }

        auto addPin = [&]( const PIN_REFERENCE& aPinReference, size_t aPinOrdinal )
        {
            std::unique_ptr<SCH_PIN> pin = makePin( pinById( aDefinition, aPinReference.id ), aLibrary );

            if( aGate && aPinOrdinal < aGate->logicalPins.size() )
            {
                const MODEL_GATE_PIN& logicalPin = aGate->logicalPins[aPinOrdinal];
                pin->SetNumber( logicalPin.number.text );
                pin->SetName( logicalPin.name.text );
                pin->SetType( pinType( logicalPin.electricalType ) );
            }

            if( aConnectorPin )
            {
                pin->SetNumber( aConnectorPin->number.text );
                pin->SetName( aConnectorPin->name.text );
                pin->SetType( pinType( aConnectorPin->electricalType ) );
            }

            pin->SetUnit( aUnit );
            aLibrary->AddDrawItem( pin.release() );
        };

        if( aPlacementPins && !aPlacementPins->empty() )
        {
            for( size_t pinOrdinal = 0; pinOrdinal < aPlacementPins->size(); ++pinOrdinal )
                addPin( ( *aPlacementPins )[pinOrdinal], pinOrdinal );
        }
        else if( aGate && !aGate->pins.empty() )
        {
            for( size_t pinOrdinal = 0; pinOrdinal < aGate->pins.size(); ++pinOrdinal )
                addPin( aGate->pins[pinOrdinal], pinOrdinal );
        }
        else
        {
            for( const MODEL_PIN_DEFINITION& sourcePin : aDefinition.pins )
            {
                std::unique_ptr<SCH_PIN> pin = makePin( sourcePin, aLibrary );

                if( aConnectorPin )
                {
                    pin->SetNumber( aConnectorPin->number.text );
                    pin->SetName( aConnectorPin->name.text );
                    pin->SetType( pinType( aConnectorPin->electricalType ) );
                }

                pin->SetUnit( aUnit );
                aLibrary->AddDrawItem( pin.release() );
            }
        }
    }


    std::unique_ptr<LIB_SYMBOL> makeLibrarySymbol( const PADS_SCH_MODEL& aModel, const MODEL_PLACEMENT& aPlacement,
                                                   std::vector<PARSER_DIAGNOSTIC>& aDiagnostics, wxString& aReference,
                                                   int& aUnit )
    {
        const MODEL_PART_TYPE& part = partById( aModel, aPlacement.partType.id );
        wxString               libraryName = part.name.text;
        auto                   library = std::make_unique<LIB_SYMBOL>( libraryName );
        const MODEL_GATE*      connectorGate = nullptr;

        for( const MODEL_GATE& gate : part.gates )
        {
            if( !gate.connectorPins.empty() )
            {
                connectorGate = &gate;
                break;
            }
        }

        if( connectorGate )
        {
            const MODEL_SYMBOL_DEFINITION& definition = definitionById( aModel, aPlacement.definition.id );
            library->SetUnitCount( static_cast<int>( connectorGate->connectorPins.size() ), false );
            library->LockUnits( true );
            aReference = aPlacement.reference.text.BeforeLast( '-' );

            if( aReference.IsEmpty() )
                aReference = aPlacement.reference.text;

            aUnit = static_cast<int>( aPlacement.unit );

            for( size_t index = 0; index < connectorGate->connectorPins.size(); ++index )
            {
                const MODEL_CONNECTOR_PIN& connectorPin = connectorGate->connectorPins[index];
                addDefinitionUnit( library.get(), definition, connectorGate, static_cast<int>( index + 1 ),
                                   aDiagnostics, &aPlacement.pins, &connectorPin );

                if( aPlacement.reference.text.EndsWith( wxS( "-" ) + connectorPin.number.text ) )
                    aUnit = static_cast<int>( index + 1 );
            }
        }
        else if( part.gates.size() > 1 )
        {
            library->SetUnitCount( static_cast<int>( part.gates.size() ), false );
            library->LockUnits( true );
            aReference = aPlacement.reference.text.BeforeLast( '-' );

            if( aReference.IsEmpty() )
                aReference = aPlacement.reference.text;

            for( const MODEL_GATE& gate : part.gates )
            {
                const MODEL_SYMBOL_DEFINITION&           definition = gate.unit == aPlacement.unit
                                                                              ? definitionById( aModel, aPlacement.definition.id )
                                                                              : definitionById( aModel, gate.definition.id );
                const std::vector<PLACED_PIN_REFERENCE>* placementPins =
                        gate.unit == aPlacement.unit ? &aPlacement.pins : nullptr;
                addDefinitionUnit( library.get(), definition, &gate, static_cast<int>( gate.unit ), aDiagnostics,
                                   placementPins );
            }
        }
        else
        {
            const MODEL_SYMBOL_DEFINITION& definition = definitionById( aModel, aPlacement.definition.id );
            const MODEL_GATE*              gate = part.gates.empty() ? nullptr : &part.gates.front();
            addDefinitionUnit( library.get(), definition, gate, 0, aDiagnostics, &aPlacement.pins );
        }

        for( const MODEL_SIGNAL_PIN& signalPin : part.signalPins )
        {
            auto pin = std::make_unique<SCH_PIN>( library.get() );
            pin->SetNumber( signalPin.number.text );
            pin->SetName( signalPin.name.text );
            pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
            pin->SetVisible( false );
            pin->SetLength( 0 );
            library->AddDrawItem( pin.release() );
        }

        library->SetShowPinNames( aPlacement.pinNamesVisible );
        library->SetShowPinNumbers( aPlacement.pinNumbersVisible );
        return library;
    }


    void applyField( SCH_SYMBOL* aSymbol, const MODEL_PLACEMENT& aPlacement, const MODEL_FIELD& aSource,
                     std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        SCH_FIELD* field = nullptr;

        if( aSource.name.text.CmpNoCase( wxS( "REF-DES" ) ) == 0 )
            field = aSymbol->GetField( FIELD_T::REFERENCE );
        else if( aSource.name.text.CmpNoCase( wxS( "PART-TYPE" ) ) == 0
                 || aSource.name.text.CmpNoCase( wxS( "VALUE" ) ) == 0 )
            field = aSymbol->GetField( FIELD_T::VALUE );
        else
        {
            field = aSymbol->GetField( aSource.name.text );

            if( !field )
            {
                SCH_FIELD newField( aSymbol, FIELD_T::USER, aSource.name.text );
                aSymbol->AddField( newField );
                field = aSymbol->GetField( aSource.name.text );
            }
        }

        if( !field )
            THROW_IO_ERROR( FormatParserError( aSource.source, wxS( "could not stage symbol field" ) ) );

        SOURCE_POINT position = aSource.position;

        if( aPlacement.mirrorFlags & 1 )
            position.x = -position.x;

        if( aPlacement.mirrorFlags & 2 )
            position.y = -position.y;

        field->SetText( aSource.value.text );
        field->SetPosition( aSymbol->GetPosition() + localPoint( position ) );
        field->SetTextAngle( EDA_ANGLE( aSource.angle, TENTHS_OF_A_DEGREE_T ) );
        applyTextPresentation( field, aSource.presentation, aSource.visible && aSource.presentation.visible,
                               aDiagnostics );

        if( aPlacement.mirrorFlags & 1 )
            field->SetHorizJustify( GetFlippedAlignment( field->GetHorizJustify() ) );

        if( aPlacement.mirrorFlags & 2 )
            field->SetVertJustify( GetFlippedAlignment( field->GetVertJustify() ) );
    }


    std::unique_ptr<SCH_SYMBOL> makeSymbol( const PADS_SCH_MODEL& aModel, const MODEL_PLACEMENT& aPlacement,
                                            const SCH_SHEET_PATH& aPath, int aPageHeight,
                                            std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        wxString                    reference = aPlacement.reference.text;
        int                         unit = static_cast<int>( aPlacement.unit );
        std::unique_ptr<LIB_SYMBOL> library = makeLibrarySymbol( aModel, aPlacement, aDiagnostics, reference, unit );
        auto                        symbol = std::make_unique<SCH_SYMBOL>();
        LIB_ID                      libId;
        libId.SetLibNickname( wxS( "pads_import" ) );
        libId.SetLibItemName( library->GetName() );
        symbol->SetLibId( libId );
        auto libraryCopy = std::make_unique<LIB_SYMBOL>( *library );
        symbol->SetLibSymbol( libraryCopy.release() );
        symbol->SetPosition( pagePoint( aPlacement.position, aPageHeight ) );

        const int angle = NormalizeAngle( aPlacement.angle );
        int       orientation = SYM_ORIENT_0;

        switch( angle )
        {
        case 900: orientation = SYM_ORIENT_90; break;
        case 1800: orientation = SYM_ORIENT_180; break;
        case 2700: orientation = SYM_ORIENT_270; break;
        default: break;
        }

        if( aPlacement.mirrorFlags & 1 )
            orientation |= SYM_MIRROR_Y;

        if( aPlacement.mirrorFlags & 2 )
            orientation |= SYM_MIRROR_X;

        symbol->SetOrientation( orientation );

        symbol->SetUnit( unit );
        symbol->SetRef( &aPath, reference );
        symbol->AddHierarchicalReference( aPath.Path(), reference, unit );

        for( const MODEL_FIELD& field : aPlacement.fields )
            applyField( symbol.get(), aPlacement, field, aDiagnostics );

        symbol->SetExcludedFromBoard( library->GetPins().empty() );

        return symbol;
    }


    PAGE_INFO pageInfo( const MODEL_SHEET& aSheet )
    {
        PAGE_INFO page;
        page.SetWidthMils( static_cast<int>( aSheet.pageSize.x / 2 ) );
        page.SetHeightMils( static_cast<int>( aSheet.pageSize.y / 2 ) );
        return page;
    }


    void applyTitleBlock( SCH_SCREEN* aScreen, const MODEL_SHEET& aSheet )
    {
        TITLE_BLOCK title;
        title.SetTitle( aSheet.title.text );

        static const std::map<wxString, int> commentFields = {
            { wxS( "designed" ), 0 },   { wxS( "des date" ), 1 },       { wxS( "drawn by" ), 2 },
            { wxS( "checked by" ), 3 }, { wxS( "checked date" ), 4 },   { wxS( "approved" ), 5 },
            { wxS( "app date" ), 6 },   { wxS( "drawing number" ), 7 }, { wxS( "scale" ), 8 }
        };
        std::array<bool, 9>             reservedComments{};
        std::vector<const MODEL_FIELD*> fallbackComments;
        wxString                        companyName;
        wxString                        code;

        for( const MODEL_FIELD& field : aSheet.titleBlockFields )
        {
            const wxString name = field.name.text.Lower();

            if( name == wxS( "title" ) )
                title.SetTitle( field.value.text );
            else if( name == wxS( "revision" ) )
                title.SetRevision( field.value.text );
            else if( name == wxS( "date" ) || name == wxS( "drawn date" ) )
                title.SetDate( field.value.text );
            else if( name == wxS( "company name" ) )
                companyName = field.value.text;
            else if( name == wxS( "code" ) )
                code = field.value.text;
            else if( auto comment = commentFields.find( name ); comment != commentFields.end() )
            {
                reservedComments[comment->second] = true;
                title.SetComment( comment->second, field.value.text );
            }
            else
            {
                fallbackComments.push_back( &field );
            }
        }

        title.SetCompany( companyName.IsEmpty() ? code : companyName );

        auto fallback = fallbackComments.begin();

        for( size_t comment = 0; comment < reservedComments.size() && fallback != fallbackComments.end(); ++comment )
        {
            if( !reservedComments[comment] )
                title.SetComment( comment, ( *fallback++ )->value.text );
        }

        aScreen->SetTitleBlock( title );
    }


    std::string quoteWorksheetText( const wxString& aText )
    {
        std::string result;

        for( char character : std::string( aText.utf8_str() ) )
        {
            if( character == '\\' || character == '"' )
                result.push_back( '\\' );

            if( character == '\n' )
                result += "\\n";
            else if( character != '\r' )
                result.push_back( character );
        }

        return result;
    }


    std::string worksheetVariable( const wxString& aText )
    {
        static const std::map<wxString, std::string> variables = {
            { wxS( "Title" ), "${TITLE}" },           { wxS( "Revision" ), "${REVISION}" },
            { wxS( "Drawn Date" ), "${ISSUE_DATE}" }, { wxS( "Code" ), "${COMPANY}" },
            { wxS( "Designed" ), "${COMMENT1}" },     { wxS( "Des Date" ), "${COMMENT2}" },
            { wxS( "Drawn By" ), "${COMMENT3}" },     { wxS( "Checked By" ), "${COMMENT4}" },
            { wxS( "Checked Date" ), "${COMMENT5}" }, { wxS( "Approved" ), "${COMMENT6}" },
            { wxS( "App Date" ), "${COMMENT7}" },     { wxS( "Drawing Number" ), "${COMMENT8}" },
            { wxS( "Scale" ), "${COMMENT9}" },        { wxS( "Sheet Number" ), "${#}" },
            { wxS( "Number of Sheets" ), "${##}" },   { wxS( "Sheet Name" ), "${SHEETNAME}" },
            { wxS( "Sheet Size" ), "${PAPER}" }
        };
        auto found = variables.find( aText );
        return found == variables.end() ? quoteWorksheetText( aText ) : found->second;
    }


    double worksheetX( const SOURCE_POINT& aPoint, const SOURCE_POINT& aPageSize )
    {
        return static_cast<double>( aPageSize.x - aPoint.x ) * 0.0127;
    }


    double worksheetY( const SOURCE_POINT& aPoint )
    {
        return static_cast<double>( aPoint.y ) * 0.0127;
    }


    void appendWorksheetLine( std::ostringstream& aOutput, const SOURCE_POINT& aStart, const SOURCE_POINT& aEnd,
                              const SOURCE_POINT& aPageSize, int64_t aWidth )
    {
        aOutput << "  (line (name \"\") (start " << worksheetX( aStart, aPageSize ) << ' ' << worksheetY( aStart )
                << ") (end " << worksheetX( aEnd, aPageSize ) << ' ' << worksheetY( aEnd ) << ')';

        if( aWidth > 0 )
            aOutput << " (linewidth " << static_cast<double>( aWidth ) * 0.0127 << ')';

        aOutput << ")\n";
    }


    std::string serializeWorksheet( const MODEL_WORKSHEET& aWorksheet, const MODEL_SHEET& aSheet )
    {
        std::ostringstream output;
        output.imbue( std::locale::classic() );
        output.precision( 8 );
        output << "(kicad_wks (version 20220228) (generator pads_import)\n"
                  "  (setup (textsize 1.27 1.27) (linewidth 0) (textlinewidth 0)"
                  " (left_margin 0) (right_margin 0) (top_margin 0) (bottom_margin 0))\n";

        for( const MODEL_GRAPHIC& graphic : aWorksheet.graphics )
        {
            if( graphic.kind == MODEL_GRAPHIC_KIND::TEXT )
            {
                if( graphic.points.empty() || !graphic.presentation.visible )
                    continue;

                output << "  (tbtext \"" << worksheetVariable( graphic.text.text ) << "\" (name \"\") (pos "
                       << worksheetX( graphic.points.front(), aSheet.pageSize ) << ' '
                       << worksheetY( graphic.points.front() );

                if( graphic.angle != 0 )
                    output << ") (rotate " << static_cast<double>( graphic.angle ) / 10.0;

                output << ") (font (size " << static_cast<double>( graphic.presentation.height ) * 0.0127 << ' '
                       << static_cast<double>( graphic.presentation.height ) * 0.0127 << ')';

                if( graphic.presentation.width > 0 )
                    output << " (linewidth " << static_cast<double>( graphic.presentation.width ) * 0.0127 << ')';

                if( graphic.presentation.bold )
                    output << " bold";

                if( graphic.presentation.italic )
                    output << " italic";

                output << ')';

                if( graphic.presentation.horizontalJustification != MODEL_JUSTIFICATION::LEFT
                    || graphic.presentation.verticalJustification != MODEL_JUSTIFICATION::CENTER )
                {
                    output << " (justify";

                    if( graphic.presentation.horizontalJustification == MODEL_JUSTIFICATION::CENTER )
                        output << " center";
                    else if( graphic.presentation.horizontalJustification == MODEL_JUSTIFICATION::RIGHT )
                        output << " right";

                    if( graphic.presentation.verticalJustification == MODEL_JUSTIFICATION::LEFT )
                        output << " top";
                    else if( graphic.presentation.verticalJustification == MODEL_JUSTIFICATION::RIGHT )
                        output << " bottom";

                    output << ')';
                }

                output << ")\n";
                continue;
            }

            if( graphic.kind == MODEL_GRAPHIC_KIND::CIRCLE && graphic.points.size() >= 2 )
            {
                const SOURCE_POINT& first = graphic.points[0];
                const SOURCE_POINT& second = graphic.points[1];
                const double        centerX = ( static_cast<double>( first.x ) + second.x ) / 2.0;
                const double        centerY = ( static_cast<double>( first.y ) + second.y ) / 2.0;
                const double        radius = std::hypot( static_cast<double>( second.x - first.x ),
                                                         static_cast<double>( second.y - first.y ) )
                                      / 2.0;
                constexpr int segments = 64;

                for( int segment = 0; segment < segments; ++segment )
                {
                    const double firstAngle = 2.0 * M_PI * segment / segments;
                    const double secondAngle = 2.0 * M_PI * ( segment + 1 ) / segments;
                    SOURCE_POINT start{ KiROUND( centerX + radius * std::cos( firstAngle ) ),
                                        KiROUND( centerY + radius * std::sin( firstAngle ) ), graphic.source };
                    SOURCE_POINT end{ KiROUND( centerX + radius * std::cos( secondAngle ) ),
                                      KiROUND( centerY + radius * std::sin( secondAngle ) ), graphic.source };
                    appendWorksheetLine( output, start, end, aSheet.pageSize, graphic.strokeWidth );
                }

                continue;
            }

            if( graphic.kind == MODEL_GRAPHIC_KIND::ARC )
            {
                const double startAngle =
                        std::atan2( static_cast<double>( graphic.arcBoundsStart.y - graphic.arcCenter.y ),
                                    static_cast<double>( graphic.arcBoundsStart.x - graphic.arcCenter.x ) );
                const double sweep = static_cast<double>( graphic.arcSweepAngle ) * M_PI / 1800.0
                                     * ( graphic.arcClockwise ? -1.0 : 1.0 );
                const int    segments = std::max( 1, static_cast<int>( std::ceil( std::abs( sweep ) * 16.0 / M_PI ) ) );
                const double radius =
                        std::hypot( static_cast<double>( graphic.arcBoundsStart.x - graphic.arcCenter.x ),
                                    static_cast<double>( graphic.arcBoundsStart.y - graphic.arcCenter.y ) );

                for( int segment = 0; segment < segments; ++segment )
                {
                    const double angle1 = startAngle + sweep * segment / segments;
                    const double angle2 = startAngle + sweep * ( segment + 1 ) / segments;
                    SOURCE_POINT start{ KiROUND( graphic.arcCenter.x + radius * std::cos( angle1 ) ),
                                        KiROUND( graphic.arcCenter.y + radius * std::sin( angle1 ) ), graphic.source };
                    SOURCE_POINT end{ KiROUND( graphic.arcCenter.x + radius * std::cos( angle2 ) ),
                                      KiROUND( graphic.arcCenter.y + radius * std::sin( angle2 ) ), graphic.source };
                    appendWorksheetLine( output, start, end, aSheet.pageSize, graphic.strokeWidth );
                }

                continue;
            }

            if( graphic.kind == MODEL_GRAPHIC_KIND::RECTANGLE && graphic.points.size() >= 2 )
            {
                const SOURCE_POINT topLeft{ graphic.points[0].x, graphic.points[0].y, graphic.source };
                const SOURCE_POINT topRight{ graphic.points[1].x, graphic.points[0].y, graphic.source };
                const SOURCE_POINT bottomRight{ graphic.points[1].x, graphic.points[1].y, graphic.source };
                const SOURCE_POINT bottomLeft{ graphic.points[0].x, graphic.points[1].y, graphic.source };
                appendWorksheetLine( output, topLeft, topRight, aSheet.pageSize, graphic.strokeWidth );
                appendWorksheetLine( output, topRight, bottomRight, aSheet.pageSize, graphic.strokeWidth );
                appendWorksheetLine( output, bottomRight, bottomLeft, aSheet.pageSize, graphic.strokeWidth );
                appendWorksheetLine( output, bottomLeft, topLeft, aSheet.pageSize, graphic.strokeWidth );
                continue;
            }

            for( size_t point = 1; point < graphic.points.size(); ++point )
            {
                appendWorksheetLine( output, graphic.points[point - 1], graphic.points[point], aSheet.pageSize,
                                     graphic.strokeWidth );
            }
        }

        output << ")\n";
        return output.str();
    }


    wxString sanitizedFilename( const wxString& aName, size_t aOrdinal, std::set<wxString>& aUsed )
    {
        wxString stem = aName;

        for( wxUniChar character : wxS( "<>:\"/\\|?*" ) )
            stem.Replace( wxString( character ), wxS( "_" ) );

        stem.Trim( true ).Trim( false );

        if( stem.IsEmpty() )
            stem = wxString::Format( wxS( "pads_sheet_%zu" ), aOrdinal + 1 );

        wxString candidate = stem + wxS( "." ) + FILEEXT::KiCadSchematicFileExtension;
        size_t   suffix = 2;

        while( aUsed.contains( candidate.Lower() ) )
            candidate = wxString::Format( wxS( "%s_%zu.%s" ), stem, suffix++, FILEEXT::KiCadSchematicFileExtension );

        aUsed.insert( candidate.Lower() );
        return candidate;
    }


    struct STAGED_SCHEMATIC
    {
        SCH_SHEET*                              destinationRoot = nullptr;
        std::unique_ptr<SCH_SCREEN>             replacementScreen;
        std::unique_ptr<SCH_SCREEN>             appendCache;
        EE_RTREE                                appendIndex;
        std::vector<std::unique_ptr<SCH_ITEM>>  appendItems;
        std::unique_ptr<SCH_SCREEN>             topLevelCache;
        EE_RTREE                                topLevelIndex;
        std::vector<std::unique_ptr<SCH_SHEET>> topLevelOwners;
        std::vector<SCH_SHEET*>                 topLevelSheets;
        SCH_SHEET_LIST                          hierarchy;
        std::optional<SCH_SHEET_PATH>           replacementCurrentSheet;
        std::unique_ptr<CONNECTION_GRAPH>       connectionGraph;
        std::unique_ptr<EMBEDDED_FILES>         replacementEmbeddedFiles;
        wxString                                replacementDrawingSheet;
        BUILD_RESULT                            result;

        static void ValidateScreen( const SCH_SCREEN* aScreen )
        {
            if( !aScreen )
                THROW_IO_ERROR( wxS( "staged sheet is missing its screen" ) );

            for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
            {
                const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

                if( !symbol->GetLibSymbolRef() || symbol->GetLibId().GetLibItemName().empty() )
                    THROW_IO_ERROR( wxS( "staged symbol is missing its library link" ) );
            }
        }

        static void ValidateChildSheet( const SCH_SHEET* aSheet, std::set<wxString>& aFilenames )
        {
            if( !aSheet || !aSheet->GetScreen() )
                THROW_IO_ERROR( wxS( "staged child sheet is missing its screen" ) );

            ValidateScreen( aSheet->GetScreen() );

            wxString filename = aSheet->GetField( FIELD_T::SHEET_FILENAME )->GetText();

            if( filename.IsEmpty() || filename.Contains( wxS( "/" ) ) || filename.Contains( wxS( "\\" ) )
                || filename.Contains( wxS( ":" ) ) || filename.Contains( wxS( "*" ) ) )
            {
                THROW_IO_ERROR( wxS( "staged child sheet has an invalid filename" ) );
            }

            if( !aFilenames.insert( filename.Lower() ).second )
                THROW_IO_ERROR( wxS( "staged child sheet filenames are not unique" ) );
        }

        void Validate( bool aAppending ) const
        {
            if( replacementEmbeddedFiles
                && ( aAppending || replacementDrawingSheet.IsEmpty()
                     || !replacementEmbeddedFiles->HasFile( wxS( "pads_import.kicad_wks" ) ) ) )
            {
                THROW_IO_ERROR( wxS( "embedded worksheet staging is incomplete" ) );
            }

            if( !replacementEmbeddedFiles && !replacementDrawingSheet.IsEmpty() )
                THROW_IO_ERROR( wxS( "worksheet path has no staged embedded file" ) );

            if( topLevelCache )
            {
                if( replacementScreen || appendCache || topLevelOwners.empty()
                    || topLevelOwners.size() != topLevelSheets.size() || topLevelSheets.size() != result.counts.sheets )
                {
                    THROW_IO_ERROR( wxS( "top-level sheet staging has invalid ownership" ) );
                }

                for( const std::unique_ptr<SCH_SHEET>& sheet : topLevelOwners )
                {
                    if( !sheet || !sheet->GetScreen() || !sheet->GetScreen()->Items().OfType( SCH_SHEET_T ).empty() )
                        THROW_IO_ERROR( wxS( "staged top-level sheet has invalid content" ) );

                    ValidateScreen( sheet->GetScreen() );
                }

                if( hierarchy.size() != topLevelSheets.size() || !replacementCurrentSheet || !connectionGraph )
                    THROW_IO_ERROR( wxS( "top-level sheet staging has incomplete hierarchy state" ) );

                return;
            }

            if( aAppending && ( replacementScreen || !appendCache ) )
                THROW_IO_ERROR( wxS( "append staging has invalid screen ownership" ) );

            if( !aAppending && ( !destinationRoot || !replacementScreen || !replacementCurrentSheet ) )
                THROW_IO_ERROR( wxS( "replacement staging has no destination root or screen" ) );

            std::set<wxString> filenames;

            if( replacementScreen )
            {
                ValidateScreen( replacementScreen.get() );

                size_t childCount = 0;

                for( SCH_ITEM* item : replacementScreen->Items().OfType( SCH_SHEET_T ) )
                {
                    ValidateChildSheet( static_cast<SCH_SHEET*>( item ), filenames );
                    ++childCount;
                }

                if( result.counts.sheets > 1 && childCount != result.counts.sheets )
                    THROW_IO_ERROR( wxS( "staged replacement hierarchy does not own every source sheet" ) );

                if( result.counts.sheets == 1 && childCount != 0 )
                    THROW_IO_ERROR( wxS( "single-sheet staging unexpectedly contains child sheets" ) );
            }

            size_t appendedSheetCount = 0;

            for( const std::unique_ptr<SCH_ITEM>& item : appendItems )
            {
                if( !item )
                    THROW_IO_ERROR( wxS( "staged schematic contains an empty object" ) );

                if( item->Type() == SCH_SHEET_T )
                {
                    ValidateChildSheet( static_cast<const SCH_SHEET*>( item.get() ), filenames );
                    ++appendedSheetCount;
                }
                else if( item->Type() == SCH_SYMBOL_T )
                {
                    const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item.get() );

                    if( !symbol->GetLibSymbolRef() || symbol->GetLibId().GetLibItemName().empty() )
                        THROW_IO_ERROR( wxS( "staged symbol is missing its library link" ) );
                }
            }

            if( aAppending && result.counts.sheets > 1 && appendedSheetCount != result.counts.sheets )
                THROW_IO_ERROR( wxS( "staged append hierarchy does not own every source sheet" ) );

            if( hierarchy.empty() || !connectionGraph )
                THROW_IO_ERROR( wxS( "staged schematic has incomplete hierarchy state" ) );
        }

        void CommitNoexcept( SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe ) noexcept
        {
            CONNECTION_GRAPH* previousGraph = nullptr;

            if( replacementScreen )
            {
                replacementScreen->IncRefCount();
                SCH_SCREEN* previousScreen = destinationRoot->AdoptImportedScreen( replacementScreen.get() );
                previousGraph = aSchematic->AdoptImportedHierarchy( std::move( hierarchy ), &*replacementCurrentSheet,
                                                                    connectionGraph.get() );
                replacementScreen.release();
                connectionGraph.release();
                previousScreen->DecRefCount();

                if( previousScreen->GetRefCount() == 0 )
                    delete previousScreen;
            }
            else if( topLevelCache )
            {
                aSchematic->Root().GetScreen()->AdoptImportedContent( std::move( topLevelIndex ), *topLevelCache );
                previousGraph = aSchematic->AdoptImportedTopLevelHierarchy(
                        topLevelSheets, std::move( hierarchy ), *replacementCurrentSheet, connectionGraph.get() );
                connectionGraph.release();

                for( std::unique_ptr<SCH_SHEET>& sheet : topLevelOwners )
                    sheet.release();

                for( SCH_SHEET* oldSheet : topLevelSheets )
                    delete oldSheet;
            }
            else
            {
                aAppendToMe->GetScreen()->AdoptImportedContent( std::move( appendIndex ), *appendCache );
                previousGraph =
                        aSchematic->AdoptImportedHierarchy( std::move( hierarchy ), nullptr, connectionGraph.get() );
                connectionGraph.release();

                for( std::unique_ptr<SCH_ITEM>& item : appendItems )
                    item.release();
            }

            if( replacementEmbeddedFiles )
            {
                *aSchematic->GetEmbeddedFiles() = std::move( *replacementEmbeddedFiles );
                aSchematic->Settings().m_SchDrawingSheetFileName.swap( replacementDrawingSheet );
            }

            delete previousGraph;
        }

        void Commit( SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe, const std::function<void()>& aBeforeCommit )
        {
            if( aBeforeCommit )
                aBeforeCommit();

            CommitNoexcept( aSchematic, aAppendToMe );
        }
    };


    void collectDispositionDiagnostics( const std::vector<SOURCE_PROPERTY>& aProperties,
                                        std::vector<PARSER_DIAGNOSTIC>&     aDiagnostics )
    {
        for( const SOURCE_PROPERTY& property : aProperties )
        {
            if( property.disposition == PROPERTY_DISPOSITION::EXACT
                || property.disposition == PROPERTY_DISPOSITION::PRESERVED )
                continue;

            aDiagnostics.push_back( MakePropertyDiagnostic(
                    RPT_SEVERITY_WARNING, property,
                    wxString::Format( wxS( "PADS property '%s' retained with %s disposition" ), property.name.text,
                                      property.disposition == PROPERTY_DISPOSITION::APPROXIMATE
                                              ? wxS( "approximate" )
                                              : wxS( "unsupported" ) ) ) );
        }
    }


    void collectPresentationDiagnostics( const MODEL_TEXT_PRESENTATION&  aPresentation,
                                         std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        collectDispositionDiagnostics( aPresentation.properties, aDiagnostics );
    }


    void validatePropertyDispositions( const PADS_SCH_MODEL& aModel, std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        collectDispositionDiagnostics( aModel.settings.properties, aDiagnostics );

        for( const MODEL_SHEET& sheet : aModel.sheets )
        {
            collectDispositionDiagnostics( sheet.properties, aDiagnostics );

            for( const MODEL_GRAPHIC& graphic : sheet.border )
            {
                collectDispositionDiagnostics( graphic.properties, aDiagnostics );
                collectPresentationDiagnostics( graphic.presentation, aDiagnostics );
            }

            for( const MODEL_FIELD& field : sheet.titleBlockFields )
            {
                collectDispositionDiagnostics( field.properties, aDiagnostics );
                collectPresentationDiagnostics( field.presentation, aDiagnostics );
            }
        }

        for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
        {
            collectDispositionDiagnostics( definition.properties, aDiagnostics );

            for( const MODEL_GRAPHIC& graphic : definition.graphics )
            {
                collectDispositionDiagnostics( graphic.properties, aDiagnostics );
                collectPresentationDiagnostics( graphic.presentation, aDiagnostics );
            }

            for( const MODEL_PIN_DEFINITION& pin : definition.pins )
            {
                collectDispositionDiagnostics( pin.properties, aDiagnostics );
                collectPresentationDiagnostics( pin.presentation, aDiagnostics );
                collectPresentationDiagnostics( pin.namePresentation, aDiagnostics );
                collectPresentationDiagnostics( pin.numberPresentation, aDiagnostics );
            }

            for( const MODEL_FIELD& field : definition.fields )
            {
                collectDispositionDiagnostics( field.properties, aDiagnostics );
                collectPresentationDiagnostics( field.presentation, aDiagnostics );
            }
        }

        for( const MODEL_PART_TYPE& part : aModel.partTypes )
        {
            collectDispositionDiagnostics( part.properties, aDiagnostics );

            for( const MODEL_GATE& gate : part.gates )
                collectDispositionDiagnostics( gate.properties, aDiagnostics );

            for( const MODEL_FIELD& field : part.fields )
            {
                collectDispositionDiagnostics( field.properties, aDiagnostics );
                collectPresentationDiagnostics( field.presentation, aDiagnostics );
            }
        }

        for( const MODEL_PLACEMENT& placement : aModel.placements )
        {
            collectDispositionDiagnostics( placement.properties, aDiagnostics );

            for( const MODEL_FIELD& field : placement.fields )
            {
                collectDispositionDiagnostics( field.properties, aDiagnostics );
                collectPresentationDiagnostics( field.presentation, aDiagnostics );
            }
        }

        for( const MODEL_NET& net : aModel.nets )
        {
            collectDispositionDiagnostics( net.properties, aDiagnostics );

            for( const MODEL_CONNECTION& connection : net.connections )
            {
                collectDispositionDiagnostics( connection.properties, aDiagnostics );

                for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
                    collectDispositionDiagnostics( endpoint.properties, aDiagnostics );
            }
        }

        for( const MODEL_BUS& bus : aModel.buses )
        {
            collectDispositionDiagnostics( bus.properties, aDiagnostics );

            for( const MODEL_BUS_ENTRY& entry : bus.entries )
                collectDispositionDiagnostics( entry.properties, aDiagnostics );
        }

        for( const MODEL_LABEL& label : aModel.labels )
        {
            collectDispositionDiagnostics( label.properties, aDiagnostics );
            collectPresentationDiagnostics( label.presentation, aDiagnostics );
        }

        for( const MODEL_JUNCTION& junction : aModel.junctions )
            collectDispositionDiagnostics( junction.properties, aDiagnostics );

        for( const MODEL_TEXT& text : aModel.texts )
        {
            collectDispositionDiagnostics( text.properties, aDiagnostics );
            collectPresentationDiagnostics( text.presentation, aDiagnostics );
        }

        for( const MODEL_PAGE_GRAPHIC& graphic : aModel.graphics )
        {
            collectDispositionDiagnostics( graphic.graphic.properties, aDiagnostics );
            collectPresentationDiagnostics( graphic.graphic.presentation, aDiagnostics );
        }

        for( const MODEL_WORKSHEET& worksheet : aModel.worksheets )
        {
            for( const MODEL_GRAPHIC& graphic : worksheet.graphics )
            {
                collectDispositionDiagnostics( graphic.properties, aDiagnostics );
                collectPresentationDiagnostics( graphic.presentation, aDiagnostics );
            }
        }

        for( const MODEL_EMBEDDED_IMAGE& image : aModel.images )
            collectDispositionDiagnostics( image.properties, aDiagnostics );

        for( const PRESERVED_CONTROLLER_PAYLOAD& payload : aModel.preservedControllerPayloads )
        {
            aDiagnostics.push_back( MakePropertyDiagnostic(
                    RPT_SEVERITY_WARNING, payload.source, wxS( "controller_payload" ), payload.disposition,
                    wxS( "PADS controller payload retained without schematic construction" ) ) );
        }

        std::set<DIAGNOSTIC_PROPERTY_KEY> parserOwnedProperties;

        for( const PARSER_DIAGNOSTIC& diagnostic : aModel.diagnostics )
        {
            if( std::optional key = DiagnosticPropertyKey( diagnostic ) )
                parserOwnedProperties.insert( std::move( *key ) );
        }

        std::erase_if( aDiagnostics,
                       [&]( const PARSER_DIAGNOSTIC& aBuilderDiagnostic )
                       {
                           std::optional key = DiagnosticPropertyKey( aBuilderDiagnostic );
                           return key && parserOwnedProperties.contains( *key );
                       } );
    }


    struct MODEL_INDEX
    {
        using POINT_KEY = std::tuple<uint32_t, int64_t, int64_t>;

        explicit MODEL_INDEX( const PADS_SCH_MODEL& aModel )
        {
            for( const MODEL_PLACEMENT& placement : aModel.placements )
                placementsBySheet[placement.sheet.id].push_back( &placement );

            for( const MODEL_NET& net : aModel.nets )
            {
                netsById.emplace( net.id, &net );
                netsBySheet[net.sheet.id].push_back( &net );

                for( const MODEL_CONNECTION& connection : net.connections )
                {
                    if( connection.vertices.size() < 2 )
                        continue;

                    endpointAdjacency[{ net.id.Value(), connection.vertices.front().x, connection.vertices.front().y }]
                            .push_back( connection.vertices[1] );
                    endpointAdjacency[{ net.id.Value(), connection.vertices.back().x, connection.vertices.back().y }]
                            .push_back( connection.vertices[connection.vertices.size() - 2] );
                }
            }

            for( const MODEL_BUS& bus : aModel.buses )
                busesBySheet[bus.sheet.id].push_back( &bus );

            for( const MODEL_LABEL& label : aModel.labels )
                labelsBySheet[label.sheet.id].push_back( &label );

            for( const MODEL_JUNCTION& junction : aModel.junctions )
                junctionsBySheet[junction.sheet.id].push_back( &junction );

            for( const MODEL_TEXT& text : aModel.texts )
                textsBySheet[text.sheet.id].push_back( &text );

            for( const MODEL_PAGE_GRAPHIC& graphic : aModel.graphics )
                graphicsBySheet[graphic.sheet.id].push_back( &graphic );

            for( const MODEL_EMBEDDED_IMAGE& image : aModel.images )
                imagesBySheet[image.sheet.id].push_back( &image );
        }

        template <typename T>
        static const std::vector<const T*>& ForSheet( const std::map<SHEET_ID, std::vector<const T*>>& aMap,
                                                      SHEET_ID                                         aSheet )
        {
            static const std::vector<const T*> empty;
            auto                               found = aMap.find( aSheet );
            return found == aMap.end() ? empty : found->second;
        }

        std::map<SHEET_ID, std::vector<const MODEL_PLACEMENT*>>      placementsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_NET*>>            netsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_BUS*>>            busesBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_LABEL*>>          labelsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_JUNCTION*>>       junctionsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_TEXT*>>           textsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_PAGE_GRAPHIC*>>   graphicsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_EMBEDDED_IMAGE*>> imagesBySheet;
        std::map<NET_ID, const MODEL_NET*>                           netsById;
        std::map<POINT_KEY, std::vector<SOURCE_POINT>>               endpointAdjacency;
    };


    void stageSheetContent( STAGED_SCHEMATIC& aStaged, const PADS_SCH_MODEL& aModel, const MODEL_INDEX& aIndex,
                            const MODEL_SHEET& aSourceSheet, SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aPath )
    {
        PAGE_INFO page = pageInfo( aSourceSheet );
        aScreen->SetPageSettings( page );
        applyTitleBlock( aScreen, aSourceSheet );
        const int pageHeight = page.GetHeightIU( schIUScale.IU_PER_MILS );

        for( const MODEL_PLACEMENT* placement : MODEL_INDEX::ForSheet( aIndex.placementsBySheet, aSourceSheet.id ) )
        {
            std::unique_ptr<SCH_SYMBOL> symbol =
                    makeSymbol( aModel, *placement, aPath, pageHeight, aStaged.result.diagnostics );
            aScreen->Append( symbol.get() );
            symbol.release();
            ++aStaged.result.counts.symbols;
        }

        for( const MODEL_GRAPHIC& graphic : aSourceSheet.border )
        {
            if( graphic.kind == MODEL_GRAPHIC_KIND::TEXT )
            {
                appendPageGraphic( aScreen, graphic, pageHeight, aStaged.result.diagnostics );
                ++aStaged.result.counts.texts;
                continue;
            }

            aStaged.result.counts.graphics +=
                    appendPageGraphic( aScreen, graphic, pageHeight, aStaged.result.diagnostics );
        }

        using SEGMENT_KEY = std::tuple<uint32_t, uint32_t, int64_t, int64_t, int64_t, int64_t>;
        std::set<SEGMENT_KEY> busEntrySegments;

        auto segmentKey = []( SHEET_ID aSheet, NET_ID aNet, const SOURCE_POINT& aStart, const SOURCE_POINT& aEnd )
        {
            if( std::tie( aStart.x, aStart.y ) <= std::tie( aEnd.x, aEnd.y ) )
                return SEGMENT_KEY( aSheet.Value(), aNet.Value(), aStart.x, aStart.y, aEnd.x, aEnd.y );

            return SEGMENT_KEY( aSheet.Value(), aNet.Value(), aEnd.x, aEnd.y, aStart.x, aStart.y );
        };

        for( const MODEL_BUS* bus : MODEL_INDEX::ForSheet( aIndex.busesBySheet, aSourceSheet.id ) )
        {
            if( bus->vertices.size() < 2 )
                THROW_IO_ERROR( FormatParserError( bus->source, wxS( "bus has inconsistent geometry" ) ) );

            for( size_t vertex = 1; vertex < bus->vertices.size(); ++vertex )
            {
                auto line = std::make_unique<SCH_LINE>( pagePoint( bus->vertices[vertex - 1], pageHeight ), LAYER_BUS );
                line->SetEndPoint( pagePoint( bus->vertices[vertex], pageHeight ) );
                line->SetStroke( STROKE_PARAMS( toIU( aSourceSheet.defaultBusWidth ), LINE_STYLE::SOLID ) );
                aScreen->Append( line.get() );
                line.release();
                ++aStaged.result.counts.buses;
            }

            wxString busLabel = bus->name.text;

            if( !bus->declaredMembers.empty() )
            {
                busLabel += wxS( "{" );

                for( size_t member = 0; member < bus->declaredMembers.size(); ++member )
                {
                    if( member )
                        busLabel += wxS( " " );

                    busLabel += bus->declaredMembers[member].text;
                }

                busLabel += wxS( "}" );
            }

            auto label = std::make_unique<SCH_LABEL>( pagePoint( bus->vertices.front(), pageHeight ), busLabel );
            aScreen->Append( label.get() );
            label.release();
            ++aStaged.result.counts.labels;

            for( const MODEL_BUS_ENTRY& entry : bus->entries )
            {
                auto ownerNet = aIndex.netsById.find( entry.memberNet.id );

                if( ownerNet == aIndex.netsById.end() )
                    THROW_IO_ERROR( FormatParserError( entry.memberNet.source,
                                                       wxS( "resolved bus-entry net is missing during staging" ) ) );

                auto adjacency = aIndex.endpointAdjacency.find(
                        { entry.memberNet.id.Value(), entry.position.x, entry.position.y } );

                if( adjacency == aIndex.endpointAdjacency.end() )
                    THROW_IO_ERROR( FormatParserError( entry.source, wxS( "bus-entry geometry is unresolved" ) ) );

                if( adjacency->second.size() != 1 )
                    THROW_IO_ERROR( FormatParserError( entry.source, wxS( "bus-entry geometry is ambiguous" ) ) );

                const VECTOR2I start = pagePoint( entry.position, pageHeight );
                const VECTOR2I end = pagePoint( adjacency->second.front(), pageHeight );
                const VECTOR2I delta = end - start;
                const int      span = std::max( std::abs( delta.x ), std::abs( delta.y ) );
                const int      entrySpan = std::min( span, schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE ) );
                const VECTOR2I entryEnd =
                        span == 0 ? end : start + VECTOR2I( delta.x * entrySpan / span, delta.y * entrySpan / span );
                auto entryItem = std::make_unique<SCH_BUS_WIRE_ENTRY>( start );
                entryItem->SetSize( entryEnd - start );
                aScreen->Append( entryItem.get() );
                entryItem.release();

                if( entryEnd != end )
                {
                    auto wire = std::make_unique<SCH_LINE>( entryEnd, LAYER_WIRE );
                    wire->SetEndPoint( end );
                    wire->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
                    aScreen->Append( wire.get() );
                    wire.release();
                    ++aStaged.result.counts.wires;
                }

                busEntrySegments.insert(
                        segmentKey( aSourceSheet.id, entry.memberNet.id, entry.position, adjacency->second.front() ) );
                ++aStaged.result.counts.busEntries;
            }
        }

        for( const MODEL_NET* net : MODEL_INDEX::ForSheet( aIndex.netsBySheet, aSourceSheet.id ) )
        {
            for( const MODEL_CONNECTION& connection : net->connections )
            {
                if( connection.vertices.size() < 2 )
                    THROW_IO_ERROR(
                            FormatParserError( connection.source, wxS( "connection has inconsistent geometry" ) ) );

                for( size_t vertex = 1; vertex < connection.vertices.size(); ++vertex )
                {
                    if( busEntrySegments.contains( segmentKey( aSourceSheet.id, net->id,
                                                               connection.vertices[vertex - 1],
                                                               connection.vertices[vertex] ) ) )
                    {
                        continue;
                    }

                    auto line = std::make_unique<SCH_LINE>( pagePoint( connection.vertices[vertex - 1], pageHeight ),
                                                            LAYER_WIRE );
                    line->SetEndPoint( pagePoint( connection.vertices[vertex], pageHeight ) );
                    line->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );
                    aScreen->Append( line.get() );
                    line.release();
                    ++aStaged.result.counts.wires;
                }

                SOURCE_POINT labelPoint = connection.vertices.front();
                auto         pinEndpoint = std::ranges::find( connection.endpoints, MODEL_ENDPOINT_KIND::PIN,
                                                              &MODEL_CONNECTION_ENDPOINT::kind );

                if( pinEndpoint != connection.endpoints.end() )
                    labelPoint = pinEndpoint->point;
                else if( connection.vertices.size() >= 2
                         && busEntrySegments.contains( segmentKey( aSourceSheet.id, net->id, connection.vertices[0],
                                                                   connection.vertices[1] ) ) )
                    labelPoint = connection.vertices[1];

                const bool hasSourceLabel = std::ranges::any_of(
                        MODEL_INDEX::ForSheet( aIndex.labelsBySheet, aSourceSheet.id ),
                        [&]( const MODEL_LABEL* aLabel )
                        {
                            return ( aLabel->kind == MODEL_LABEL_KIND::GLOBAL || aLabel->kind == MODEL_LABEL_KIND::POWER
                                     || aLabel->kind == MODEL_LABEL_KIND::GROUND )
                                   && aLabel->text.text == net->name.text && aLabel->position.x == labelPoint.x
                                   && aLabel->position.y == labelPoint.y;
                        } );

                if( !hasSourceLabel )
                {
                    auto label =
                            std::make_unique<SCH_GLOBALLABEL>( pagePoint( labelPoint, pageHeight ), net->name.text );
                    label->SetTextSize( VECTOR2I( 1, 1 ) );
                    aScreen->Append( label.get() );
                    label.release();
                }
            }
        }

        for( const MODEL_JUNCTION* junction : MODEL_INDEX::ForSheet( aIndex.junctionsBySheet, aSourceSheet.id ) )
        {
            auto item = std::make_unique<SCH_JUNCTION>( pagePoint( junction->position, pageHeight ) );
            aScreen->Append( item.get() );
            item.release();
            ++aStaged.result.counts.junctions;
        }

        size_t powerOrdinal = 0;

        for( const MODEL_LABEL* labelPointer : MODEL_INDEX::ForSheet( aIndex.labelsBySheet, aSourceSheet.id ) )
        {
            const MODEL_LABEL& label = *labelPointer;

            std::unique_ptr<SCH_ITEM> item;

            switch( label.kind )
            {
            case MODEL_LABEL_KIND::LOCAL:
            case MODEL_LABEL_KIND::BUS:
                item = std::make_unique<SCH_LABEL>( pagePoint( label.position, pageHeight ), label.text.text );
                break;

            case MODEL_LABEL_KIND::GLOBAL:
                item = std::make_unique<SCH_GLOBALLABEL>( pagePoint( label.position, pageHeight ), label.text.text );
                break;

            case MODEL_LABEL_KIND::HIERARCHICAL:
                item = std::make_unique<SCH_HIERLABEL>( pagePoint( label.position, pageHeight ), label.text.text );
                break;

            case MODEL_LABEL_KIND::GROUND:
            case MODEL_LABEL_KIND::POWER:
                item = makePowerSymbol( label, aPath, pageHeight, powerOrdinal++, aStaged.result.diagnostics );
                ++aStaged.result.counts.symbols;
                break;

            case MODEL_LABEL_KIND::UNSUPPORTED:
            {
                auto property =
                        std::ranges::find_if( label.properties,
                                              []( const SOURCE_PROPERTY& aProperty )
                                              {
                                                  return aProperty.name.text == wxS( "unsupported_offpage_decal" );
                                              } );

                if( property == label.properties.end() )
                {
                    aStaged.result.diagnostics.push_back( MakePropertyDiagnostic(
                            RPT_SEVERITY_WARNING, label.source, wxS( "unsupported_offpage_decal" ),
                            PROPERTY_DISPOSITION::UNSUPPORTED,
                            wxS( "PADS unsupported label has no KiCad schematic representation" ) ) );
                }

                continue;
            }
            }

            if( auto* text = dynamic_cast<EDA_TEXT*>( item.get() ) )
            {
                text->SetTextAngle( EDA_ANGLE( label.angle, TENTHS_OF_A_DEGREE_T ) );
                applyTextPresentation( text, label.presentation, label.presentation.visible,
                                       aStaged.result.diagnostics );
            }

            aScreen->Append( item.get() );
            item.release();
            ++aStaged.result.counts.labels;
        }

        for( const MODEL_TEXT* sourceTextPointer : MODEL_INDEX::ForSheet( aIndex.textsBySheet, aSourceSheet.id ) )
        {
            const MODEL_TEXT& sourceText = *sourceTextPointer;

            std::unique_ptr<SCH_TEXT> text =
                    makePageText( sourceText.text, sourceText.position, sourceText.angle, sourceText.presentation,
                                  pageHeight, aStaged.result.diagnostics );
            aScreen->Append( text.get() );
            text.release();
            ++aStaged.result.counts.texts;
        }

        for( const MODEL_PAGE_GRAPHIC* pageGraphicPointer :
             MODEL_INDEX::ForSheet( aIndex.graphicsBySheet, aSourceSheet.id ) )
        {
            const MODEL_PAGE_GRAPHIC& pageGraphic = *pageGraphicPointer;

            if( pageGraphic.graphic.kind == MODEL_GRAPHIC_KIND::TEXT )
            {
                appendPageGraphic( aScreen, pageGraphic.graphic, pageHeight, aStaged.result.diagnostics );
                ++aStaged.result.counts.texts;
            }
            else
            {
                aStaged.result.counts.graphics +=
                        appendPageGraphic( aScreen, pageGraphic.graphic, pageHeight, aStaged.result.diagnostics );
            }
        }

        for( const MODEL_EMBEDDED_IMAGE* sourceImage : MODEL_INDEX::ForSheet( aIndex.imagesBySheet, aSourceSheet.id ) )
        {
            std::unique_ptr<SCH_BITMAP> bitmap =
                    makeEmbeddedImage( *sourceImage, pageHeight, aStaged.result.diagnostics );

            if( !bitmap )
                continue;

            aScreen->Append( bitmap.get() );
            bitmap.release();
            ++aStaged.result.counts.images;
        }
    }

} // namespace


BUILD_RESULT PADS_SCH_BINARY_BUILDER::Build( const PADS_SCH_MODEL& aModel, SCHEMATIC* aSchematic,
                                             SCH_SHEET* aAppendToMe, const wxString& aSourcePath )
{
    if( !aSchematic )
        THROW_IO_ERROR( wxS( "cannot build a PADS schematic without a destination" ) );

    if( aSourcePath.IsEmpty() )
        THROW_IO_ERROR( wxS( "cannot build a PADS schematic without a source filename" ) );

    if( aModel.sheets.empty() )
        THROW_IO_ERROR( FormatParserError( aModel.source, wxS( "schematic model has no sheets" ) ) );

    if( aAppendToMe && !aAppendToMe->GetScreen() )
        THROW_IO_ERROR( wxS( "cannot append a PADS schematic to a sheet without a screen" ) );

    aModel.ValidateOrThrow();
    std::vector<const MODEL_SHEET*> sourceSheets;
    sourceSheets.reserve( aModel.sheets.size() );

    for( const MODEL_SHEET& sheet : aModel.sheets )
        sourceSheets.push_back( &sheet );

    std::ranges::sort( sourceSheets, {}, &MODEL_SHEET::index );

    MODEL_INDEX      modelIndex( aModel );
    STAGED_SCHEMATIC staged;
    staged.result.counts.sheets = aModel.sheets.size();
    staged.connectionGraph = std::make_unique<CONNECTION_GRAPH>( aSchematic );
    validatePropertyDispositions( aModel, staged.result.diagnostics );

    if( !aAppendToMe && !aModel.worksheets.empty() )
    {
        auto worksheet = std::ranges::find( aModel.worksheets, wxS( "DRW5982" ),
                                            []( const MODEL_WORKSHEET& aWorksheet )
                                            {
                                                return aWorksheet.name.text;
                                            } );

        if( worksheet == aModel.worksheets.end() )
            worksheet = aModel.worksheets.begin();

        auto sourceSheet = std::ranges::find( aModel.sheets, worksheet->sheet.id, &MODEL_SHEET::id );

        if( sourceSheet == aModel.sheets.end() )
            THROW_IO_ERROR( FormatParserError( worksheet->source, wxS( "worksheet references an unknown sheet" ) ) );

        const std::string serialized = serializeWorksheet( *worksheet, *sourceSheet );
        auto              embeddedWorksheet = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
        embeddedWorksheet->name = wxS( "pads_import.kicad_wks" );
        embeddedWorksheet->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::WORKSHEET;
        embeddedWorksheet->decompressedData.assign( serialized.begin(), serialized.end() );

        if( EMBEDDED_FILES::CompressAndEncode( *embeddedWorksheet ) != EMBEDDED_FILES::RETURN_CODE::OK )
            THROW_IO_ERROR( FormatParserError( worksheet->source, wxS( "could not embed the PADS worksheet" ) ) );

        embeddedWorksheet->is_valid = true;
        staged.replacementEmbeddedFiles = std::make_unique<EMBEDDED_FILES>();
        staged.replacementEmbeddedFiles->SetFileAddedCallback( aSchematic->GetEmbeddedFiles()->GetFileAddedCallback() );
        staged.replacementEmbeddedFiles->AddFile( embeddedWorksheet );
        staged.replacementDrawingSheet = embeddedWorksheet->GetLink();
    }

    const bool         multiSheet = aModel.sheets.size() > 1;
    std::set<wxString> usedFilenames;
    size_t             firstChildPage = 2;

    if( aAppendToMe )
    {
        for( const SCH_SHEET_PATH& path : aSchematic->BuildSheetListSortedByPageNumbers() )
        {
            unsigned long page = 0;

            if( path.GetPageNumber().ToULong( &page ) )
            {
                if( page >= std::numeric_limits<size_t>::max() )
                    THROW_IO_ERROR( wxS( "existing schematic page number leaves no append range" ) );

                firstChildPage = std::max( firstChildPage, static_cast<size_t>( page ) + 1 );
            }
        }

        if( multiSheet && sourceSheets.size() > std::numeric_limits<size_t>::max() - firstChildPage )
            THROW_IO_ERROR( wxS( "imported hierarchy page range overflows" ) );

        staged.appendCache = std::make_unique<SCH_SCREEN>( aSchematic );

        for( const auto& [name, symbol] : aAppendToMe->GetScreen()->GetLibSymbols() )
        {
            if( !symbol )
                THROW_IO_ERROR( wxString::Format( wxS( "destination library cache entry '%s' is null" ), name ) );

            auto clone = std::make_unique<LIB_SYMBOL>( *symbol );
            staged.appendCache->AddLibSymbol( name, std::move( clone ) );
        }

        for( SCH_ITEM* item : aAppendToMe->GetScreen()->Items().OfType( SCH_SHEET_T ) )
        {
            const SCH_SHEET* child = static_cast<const SCH_SHEET*>( item );
            usedFilenames.insert( child->GetField( FIELD_T::SHEET_FILENAME )->GetText().Lower() );
        }
    }

    if( !aAppendToMe )
    {
        if( !multiSheet )
        {
            staged.destinationRoot = aSchematic->GetTopLevelSheet();

            if( !staged.destinationRoot || !staged.destinationRoot->GetScreen() )
                THROW_IO_ERROR( wxS( "cannot replace a schematic without a top-level sheet and screen" ) );

            staged.replacementScreen = std::make_unique<SCH_SCREEN>( aSchematic );
            staged.replacementScreen->SetImportStagingUuid( staged.destinationRoot->m_Uuid );
            staged.replacementScreen->SetFileName( aSourcePath );
            SCH_SHEET_PATH rootPath;
            rootPath.push_back( staged.destinationRoot );
            staged.hierarchy.push_back( rootPath );
            staged.replacementCurrentSheet.emplace( rootPath );
            stageSheetContent( staged, aModel, modelIndex, *sourceSheets.front(), staged.replacementScreen.get(),
                               rootPath );
        }
        else
        {
            staged.topLevelCache = std::make_unique<SCH_SCREEN>( aSchematic );

            for( size_t index = 0; index < sourceSheets.size(); ++index )
            {
                const MODEL_SHEET& sourceSheet = *sourceSheets[index];
                auto               sheet = std::make_unique<SCH_SHEET>( aSchematic );
                auto               screen = new SCH_SCREEN( aSchematic );
                const_cast<KIID&>( sheet->m_Uuid ) = screen->GetUuid();
                sheet->SetScreen( screen );
                sheet->SetParent( &aSchematic->Root() );
                sheet->GetField( FIELD_T::SHEET_NAME )->SetText( sourceSheet.name.text );
                wxString filename = sanitizedFilename( sourceSheet.name.text, index, usedFilenames );
                sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( filename );
                screen->SetFileName( filename );
                screen->SetPageNumber( wxString::Format( wxS( "%zu" ), index + 1 ) );
                SCH_SHEET_PATH path;
                path.push_back( sheet.get() );
                path.SetPageNumber( wxString::Format( wxS( "%zu" ), index + 1 ) );
                stageSheetContent( staged, aModel, modelIndex, sourceSheet, screen, path );
                staged.hierarchy.push_back( path );

                if( index == 0 )
                    staged.replacementCurrentSheet.emplace( path );

                staged.topLevelIndex.insert( sheet.get() );
                staged.topLevelSheets.push_back( sheet.get() );
                staged.topLevelOwners.push_back( std::move( sheet ) );
            }
        }
    }
    else if( !multiSheet )
    {
        const MODEL_SHEET& sourceSheet = *sourceSheets.front();
        SCH_SCREEN*        temporaryScreen = staged.appendCache.get();
        SCH_SHEET_PATH     path;
        path.push_back( aAppendToMe );
        stageSheetContent( staged, aModel, modelIndex, sourceSheet, temporaryScreen, path );

        std::vector<SCH_ITEM*> temporaryItems;

        for( SCH_ITEM* item : temporaryScreen->Items() )
            temporaryItems.push_back( item );

        for( SCH_ITEM* item : temporaryItems )
        {
            std::unique_ptr<SCH_ITEM> itemOwner( item );

            if( !temporaryScreen->Items().remove( item ) )
            {
                itemOwner.release();
                THROW_IO_ERROR( wxS( "staged append item is missing from its spatial index" ) );
            }

            item->SetParent( aAppendToMe->GetScreen() );
            staged.appendItems.emplace_back( std::move( itemOwner ) );
        }
    }
    else
    {
        SCH_SHEET_PATH rootPath;
        rootPath.push_back( aAppendToMe );

        for( size_t index = 0; index < sourceSheets.size(); ++index )
        {
            const MODEL_SHEET& sourceSheet = *sourceSheets[index];
            VECTOR2I           position( schIUScale.MilsToIU( 500 + static_cast<int>( index % 4 ) * 2500 ),
                                         schIUScale.MilsToIU( 500 + static_cast<int>( index / 4 ) * 2000 ) );
            auto               child = std::make_unique<SCH_SHEET>(
                    aAppendToMe, position, VECTOR2I( schIUScale.MilsToIU( 2000 ), schIUScale.MilsToIU( 1500 ) ) );
            auto childScreen = new SCH_SCREEN( aSchematic );
            child->SetScreen( childScreen );
            child->GetField( FIELD_T::SHEET_NAME )->SetText( sourceSheet.name.text );
            wxString filename = sanitizedFilename( sourceSheet.name.text, index, usedFilenames );
            child->GetField( FIELD_T::SHEET_FILENAME )->SetText( filename );
            childScreen->SetFileName( filename );
            childScreen->SetPageNumber( wxString::Format( wxS( "%zu" ), firstChildPage + index ) );
            SCH_SHEET_PATH childPath( rootPath );
            childPath.push_back( child.get() );
            childPath.SetPageNumber( wxString::Format( wxS( "%zu" ), firstChildPage + index ) );
            stageSheetContent( staged, aModel, modelIndex, sourceSheet, childScreen, childPath );
            child->SetParent( aAppendToMe->GetScreen() );
            staged.appendItems.emplace_back( std::move( child ) );
        }
    }

    if( aAppendToMe )
    {
        SCH_SHEET_LIST currentHierarchy = aSchematic->Hierarchy();
        staged.hierarchy.assign( currentHierarchy.begin(), currentHierarchy.end() );

        for( SCH_ITEM* item : aAppendToMe->GetScreen()->Items() )
            staged.appendIndex.insert( item );

        for( const std::unique_ptr<SCH_ITEM>& item : staged.appendItems )
        {
            staged.appendIndex.insert( item.get() );

            if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET_PATH path;
                path.push_back( aAppendToMe );
                path.push_back( static_cast<SCH_SHEET*>( item.get() ) );
                staged.hierarchy.push_back( path );
            }
        }
    }

    staged.Validate( aAppendToMe != nullptr );
    BUILD_RESULT result = staged.result;
    staged.Commit( aSchematic, aAppendToMe, m_beforeCommit );
    return result;
}

} // namespace PADS_SCH_BINARY
