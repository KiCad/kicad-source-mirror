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

#include <lib_id.h>
#include <lib_symbol.h>
#include <connection_graph.h>
#include <page_info.h>
#include <pin_type.h>
#include <sch_bus_entry.h>
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
#include <sch_io/pads/pads_sch_symbol_builder.h>
#include <stroke_params.h>
#include <title_block.h>

#include <ki_exception.h>
#include <wildcards_and_files_ext.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

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


    PIN_ORIENTATION pinOrientation( int aAngle )
    {
        switch( NormalizeAngle( aAngle ) )
        {
        case 900: return PIN_ORIENTATION::PIN_UP;
        case 1800: return PIN_ORIENTATION::PIN_LEFT;
        case 2700: return PIN_ORIENTATION::PIN_DOWN;
        default: return PIN_ORIENTATION::PIN_RIGHT;
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
        {
            aDiagnostics.push_back( MakePropertyDiagnostic(
                    RPT_SEVERITY_WARNING, aPresentation.source, wxS( "font_substitution" ),
                    PROPERTY_DISPOSITION::APPROXIMATE,
                    wxString::Format( wxS( "PADS font '%s' is not embedded; using KiCad stroke font" ),
                                      aPresentation.font.text ) ) );
        }

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
            shape->SetStart( convert( aGraphic.points[0] ) );
            shape->SetEnd( convert( aGraphic.points[1] ) );
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


    std::unique_ptr<SCH_SYMBOL> makePowerSymbol( const MODEL_LABEL& aLabel, const SCH_SHEET_PATH& aPath,
                                                 int aPageHeight, size_t aOrdinal,
                                                 std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        PADS_SCH::PARAMETERS              params;
        PADS_SCH::PADS_SCH_SYMBOL_BUILDER symbolBuilder( params );
        const std::string                 style = aLabel.kind == MODEL_LABEL_KIND::GROUND ? "GND" : "VCC";
        std::unique_ptr<LIB_SYMBOL>       library( symbolBuilder.BuildKiCadPowerSymbol( style ) );

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
        symbol->SetOrientation( NormalizeAngle( aLabel.angle ) == 900    ? SYM_ORIENT_90
                                : NormalizeAngle( aLabel.angle ) == 1800 ? SYM_ORIENT_180
                                : NormalizeAngle( aLabel.angle ) == 2700 ? SYM_ORIENT_270
                                                                         : SYM_ORIENT_0 );
        const wxString reference = wxString::Format( wxS( "#PWR%04zu" ), aOrdinal + 1 );
        symbol->SetRef( &aPath, reference );
        symbol->AddHierarchicalReference( aPath.Path(), reference, 1 );
        symbol->SetValueFieldText( aLabel.text.text, &aPath );

        if( SCH_FIELD* value = symbol->GetField( FIELD_T::VALUE ) )
        {
            value->SetText( aLabel.text.text );
            value->SetPosition( symbol->GetPosition() );
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
        pin->SetOrientation( pinOrientation( aPin.angle ) );
        pin->SetLength( toIU( aPin.length ) );
        pin->SetType( pinType( aPin.electricalType ) );
        pin->SetShape( pinShape( aPin.graphicStyle ) );
        pin->SetVisible( aPin.presentation.visible );

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
                            const MODEL_CONNECTOR_PIN* aConnectorPin = nullptr )
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

        if( aGate && !aGate->pins.empty() )
        {
            for( const PIN_REFERENCE& pinReference : aGate->pins )
            {
                std::unique_ptr<SCH_PIN> pin = makePin( pinById( aDefinition, pinReference.id ), aLibrary );

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
                                   aDiagnostics, &connectorPin );

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
                const MODEL_SYMBOL_DEFINITION& definition = gate.unit == aPlacement.unit
                                                                    ? definitionById( aModel, aPlacement.definition.id )
                                                                    : definitionById( aModel, gate.definition.id );
                addDefinitionUnit( library.get(), definition, &gate, static_cast<int>( gate.unit ), aDiagnostics );
            }
        }
        else
        {
            const MODEL_SYMBOL_DEFINITION& definition = definitionById( aModel, aPlacement.definition.id );
            const MODEL_GATE*              gate = part.gates.empty() ? nullptr : &part.gates.front();
            addDefinitionUnit( library.get(), definition, gate, 0, aDiagnostics );
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

        library->SetShowPinNames( true );
        library->SetShowPinNumbers( true );
        return library;
    }


    void applyField( SCH_SYMBOL* aSymbol, const MODEL_FIELD& aSource, std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
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

        field->SetText( aSource.value.text );
        field->SetPosition( aSymbol->GetPosition() + localPoint( aSource.position ) );
        field->SetTextAngle( EDA_ANGLE( aSource.angle, TENTHS_OF_A_DEGREE_T ) );
        applyTextPresentation( field, aSource.presentation, aSource.visible && aSource.presentation.visible,
                               aDiagnostics );
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

        int orientation = SYM_ORIENT_0;

        switch( NormalizeAngle( aPlacement.angle ) )
        {
        case 900: orientation = SYM_ORIENT_90; break;
        case 1800: orientation = SYM_ORIENT_180; break;
        case 2700: orientation = SYM_ORIENT_270; break;
        default: break;
        }

        if( aPlacement.mirrored )
            orientation |= SYM_MIRROR_X;

        symbol->SetOrientation( orientation );
        symbol->SetUnit( unit );
        symbol->SetRef( &aPath, reference );
        symbol->AddHierarchicalReference( aPath.Path(), reference, unit );

        for( const MODEL_FIELD& field : aPlacement.fields )
            applyField( symbol.get(), field, aDiagnostics );

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
        int commentIndex = 0;

        for( const MODEL_FIELD& field : aSheet.titleBlockFields )
        {
            if( field.name.text.CmpNoCase( wxS( "Title" ) ) == 0 )
                title.SetTitle( field.value.text );
            else if( field.name.text.CmpNoCase( wxS( "Revision" ) ) == 0 )
                title.SetRevision( field.value.text );
            else if( field.name.text.CmpNoCase( wxS( "Date" ) ) == 0
                     || field.name.text.CmpNoCase( wxS( "Drawn Date" ) ) == 0 )
                title.SetDate( field.value.text );
            else if( field.name.text.CmpNoCase( wxS( "Company Name" ) ) == 0 )
                title.SetCompany( field.value.text );
            else
                title.SetComment( commentIndex++,
                                  wxString::Format( wxS( "%s: %s" ), field.name.text, field.value.text ) );
        }

        aScreen->SetTitleBlock( title );
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
        SCH_SHEET*                             destinationRoot = nullptr;
        std::unique_ptr<SCH_SCREEN>            replacementScreen;
        std::unique_ptr<SCH_SCREEN>            appendCache;
        EE_RTREE                               appendIndex;
        std::vector<std::unique_ptr<SCH_ITEM>> appendItems;
        SCH_SHEET_LIST                         hierarchy;
        std::optional<SCH_SHEET_PATH>          replacementCurrentSheet;
        std::unique_ptr<CONNECTION_GRAPH>      connectionGraph;
        BUILD_RESULT                           result;

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
                previousGraph = aSchematic->AdoptImportedHierarchy( std::move( hierarchy ),
                                                                    &*replacementCurrentSheet,
                                                                    connectionGraph.get() );
                replacementScreen.release();
                connectionGraph.release();
                previousScreen->DecRefCount();

                if( previousScreen->GetRefCount() == 0 )
                    delete previousScreen;
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
            if( property.disposition == PROPERTY_DISPOSITION::EXACT )
                continue;

            aDiagnostics.push_back( MakePropertyDiagnostic(
                    RPT_SEVERITY_WARNING, property,
                    wxString::Format( wxS( "PADS property '%s' retained with %s disposition" ), property.name.text,
                                      property.disposition == PROPERTY_DISPOSITION::APPROXIMATE ? wxS( "approximate" )
                                      : property.disposition == PROPERTY_DISPOSITION::PRESERVED
                                              ? wxS( "preserved" )
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
        }

        template <typename T>
        static const std::vector<const T*>& ForSheet( const std::map<SHEET_ID, std::vector<const T*>>& aMap,
                                                      SHEET_ID                                         aSheet )
        {
            static const std::vector<const T*> empty;
            auto                               found = aMap.find( aSheet );
            return found == aMap.end() ? empty : found->second;
        }

        std::map<SHEET_ID, std::vector<const MODEL_PLACEMENT*>>    placementsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_NET*>>          netsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_BUS*>>          busesBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_LABEL*>>        labelsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_JUNCTION*>>     junctionsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_TEXT*>>         textsBySheet;
        std::map<SHEET_ID, std::vector<const MODEL_PAGE_GRAPHIC*>> graphicsBySheet;
        std::map<NET_ID, const MODEL_NET*>                         netsById;
        std::map<POINT_KEY, std::vector<SOURCE_POINT>>             endpointAdjacency;
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

            if( !bus->memberNets.empty() )
            {
                busLabel += wxS( "{" );

                for( size_t member = 0; member < bus->memberNets.size(); ++member )
                {
                    auto net = aIndex.netsById.find( bus->memberNets[member].id );

                    if( net == aIndex.netsById.end() )
                        THROW_IO_ERROR( FormatParserError( bus->memberNets[member].source,
                                                           wxS( "resolved bus member is missing during staging" ) ) );

                    if( member )
                        busLabel += wxS( " " );

                    busLabel += net->second->name.text;
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
                auto           entryItem = std::make_unique<SCH_BUS_WIRE_ENTRY>( start );
                entryItem->SetSize( end - start );
                aScreen->Append( entryItem.get() );
                entryItem.release();
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
                    line->SetStroke( STROKE_PARAMS( toIU( aSourceSheet.defaultLineWidth ), LINE_STYLE::SOLID ) );
                    aScreen->Append( line.get() );
                    line.release();
                    ++aStaged.result.counts.wires;
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
                auto property = std::ranges::find_if( label.properties,
                                                      []( const SOURCE_PROPERTY& aProperty )
                                                      {
                                                          return aProperty.name.text == wxS( "unsupported_label_kind" );
                                                      } );

                if( property == label.properties.end() )
                {
                    aStaged.result.diagnostics.push_back( MakePropertyDiagnostic(
                            RPT_SEVERITY_WARNING, label.source, wxS( "unsupported_label_kind" ),
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
    MODEL_INDEX      modelIndex( aModel );
    STAGED_SCHEMATIC staged;
    staged.result.counts.sheets = aModel.sheets.size();
    staged.connectionGraph = std::make_unique<CONNECTION_GRAPH>( aSchematic );
    validatePropertyDispositions( aModel, staged.result.diagnostics );

    const bool         multiSheet = aModel.sheets.size() > 1;
    std::set<wxString> usedFilenames;

    if( aAppendToMe )
    {
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
        staged.destinationRoot = aSchematic->GetTopLevelSheet();

        if( !staged.destinationRoot || !staged.destinationRoot->GetScreen() )
            THROW_IO_ERROR( wxS( "cannot replace a schematic without a top-level sheet and screen" ) );

        staged.replacementScreen = std::make_unique<SCH_SCREEN>( aSchematic );
        staged.replacementScreen->SetImportStagingUuid( staged.destinationRoot->m_Uuid );
        staged.replacementScreen->SetFileName( aSourcePath );
        SCH_SCREEN* rootScreen = staged.replacementScreen.get();

        SCH_SHEET_PATH rootPath;
        rootPath.push_back( staged.destinationRoot );
        staged.hierarchy.push_back( rootPath );
        staged.replacementCurrentSheet.emplace( rootPath );

        if( !multiSheet )
        {
            stageSheetContent( staged, aModel, modelIndex, aModel.sheets.front(), rootScreen, rootPath );
        }
        else
        {
            for( size_t index = 0; index < aModel.sheets.size(); ++index )
            {
                const MODEL_SHEET& sourceSheet = aModel.sheets[index];
                VECTOR2I           position( schIUScale.MilsToIU( 500 + static_cast<int>( index % 4 ) * 2500 ),
                                             schIUScale.MilsToIU( 500 + static_cast<int>( index / 4 ) * 2000 ) );
                auto               child = std::make_unique<SCH_SHEET>(
                        staged.destinationRoot, position,
                        VECTOR2I( schIUScale.MilsToIU( 2000 ), schIUScale.MilsToIU( 1500 ) ) );
                auto childScreen = new SCH_SCREEN( aSchematic );
                child->SetScreen( childScreen );
                child->GetField( FIELD_T::SHEET_NAME )->SetText( sourceSheet.name.text );
                wxString filename = sanitizedFilename( sourceSheet.name.text, index, usedFilenames );
                child->GetField( FIELD_T::SHEET_FILENAME )->SetText( filename );
                childScreen->SetFileName( filename );
                childScreen->SetPageNumber( wxString::Format( wxS( "%zu" ), index + 1 ) );
                SCH_SHEET_PATH childPath( rootPath );
                childPath.push_back( child.get() );
                childPath.SetPageNumber( wxString::Format( wxS( "%zu" ), index + 1 ) );
                stageSheetContent( staged, aModel, modelIndex, sourceSheet, childScreen, childPath );
                staged.hierarchy.push_back( childPath );
                rootScreen->Append( child.get() );
                child.release();
            }
        }
    }
    else if( !multiSheet )
    {
        const MODEL_SHEET& sourceSheet = aModel.sheets.front();
        SCH_SCREEN*        temporaryScreen = staged.appendCache.get();
        SCH_SHEET_PATH path;
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

        for( size_t index = 0; index < aModel.sheets.size(); ++index )
        {
            const MODEL_SHEET& sourceSheet = aModel.sheets[index];
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
            childScreen->SetPageNumber( wxString::Format( wxS( "%zu" ), index + 1 ) );
            SCH_SHEET_PATH childPath( rootPath );
            childPath.push_back( child.get() );
            childPath.SetPageNumber( wxString::Format( wxS( "%zu" ), index + 1 ) );
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
