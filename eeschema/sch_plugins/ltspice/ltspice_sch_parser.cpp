/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Parses the datastructure produced by the LTSPICE_SCHEMATIC into a KiCad
 * schematic file.
 */

#include <sch_plugins/ltspice/ltspice_sch_parser.h>
#include <sch_plugins/ltspice/ltspice_schematic.h>
#include <sch_io_mgr.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_line.h>
#include <lib_shape.h>
#include <sch_label.h>
#include <sch_edit_frame.h>
#include <sch_shape.h>
#include <sch_bus_entry.h>


void LTSPICE_SCH_PARSER::Parse( SCH_SHEET_PATH* aSheet,
                                std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs,
                                const std::vector<wxString>& aAsyFileNames )
{
    // Center created objects in Kicad page
    BOX2I bbox;

    for( const LTSPICE_SCHEMATIC::LT_ASC& asc : outLT_ASCs )
        bbox.Merge( asc.BoundingBox );

    m_originOffset = { 0, 0 };
    bbox.SetOrigin( ToKicadCoords( bbox.GetOrigin() ) );
    bbox.SetSize( ToKicadCoords( bbox.GetSize() ) );

    VECTOR2I pageSize = aSheet->LastScreen()->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
    int      grid = schIUScale.MilsToIU( 50 );
    int      margin = grid * 10;

    m_originOffset = ( pageSize / 2 ) - bbox.GetCenter();

    if( bbox.GetWidth() > pageSize.x - margin )
        m_originOffset.x = margin - bbox.GetLeft();

    if( bbox.GetHeight() > pageSize.y - margin )
        m_originOffset.y = margin - bbox.GetTop();

    m_originOffset = ( m_originOffset / grid ) * grid;

    readIncludes( outLT_ASCs );
    CreateKicadSYMBOLs( aSheet, outLT_ASCs, aAsyFileNames );
    CreateKicadSCH_ITEMs( aSheet, outLT_ASCs );
}


void LTSPICE_SCH_PARSER::readIncludes( std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs )
{
    wxString ltSubDir = m_lt_schematic->GetLTspiceDataDir().GetFullPath() + wxS( "lib/sub/" );
    wxString path;

    for( const LTSPICE_SCHEMATIC::LT_ASC& asc : outLT_ASCs )
    {
        for( const LTSPICE_SCHEMATIC::TEXT& lt_text : asc.Texts )
        {
            for( wxString& line : wxSplit( lt_text.Value, '\n' ) )
            {
                if( line.StartsWith( wxS( ".inc " ), &path ) )
                {
                    path.Replace( '\\', '/' );
                    wxFileName fileName( path );

                    if( fileName.IsAbsolute() )
                    {
                        m_includes[ fileName.GetName() ] = fileName.GetFullPath();
                    }
                    else
                    {
                        wxFileName absolute( ltSubDir + path );
                        m_includes[ absolute.GetName() ] = absolute.GetFullPath();
                    }
                }
            }
        }
    }
}


void LTSPICE_SCH_PARSER::CreateLines( LIB_SYMBOL* aSymbol, LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol,
                                      int aIndex, LIB_SHAPE* shape )
{
    LTSPICE_SCHEMATIC::LINE& lt_line = aLTSymbol.Lines[aIndex];

    shape->AddPoint( ToInvertedKicadCoords( lt_line.End ) );
    shape->AddPoint( ToInvertedKicadCoords( lt_line.Start ) );
    shape->SetStroke( getStroke( lt_line.LineWidth, lt_line.LineStyle ) );
}


void LTSPICE_SCH_PARSER::CreateLines( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                      SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::LINE& lt_line = aLTSymbol.Lines[aIndex];
    SCH_SHAPE*               shape = new SCH_SHAPE( SHAPE_T::POLY );

    shape->AddPoint( ToKicadCoords( lt_line.End ) );
    shape->AddPoint( ToKicadCoords( lt_line.Start ) );
    shape->SetStroke( getStroke( lt_line.LineWidth, lt_line.LineStyle ) );

    shape->Move( ToKicadCoords( aLTSymbol.Offset ) + m_originOffset );
    RotateMirrorShape( aLTSymbol, shape );

    aSheet->LastScreen()->Append( shape );
}


void LTSPICE_SCH_PARSER::CreateKicadSYMBOLs( SCH_SHEET_PATH* aSheet,
                                             std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs,
                                             const std::vector<wxString>& aAsyFiles )
{
    for( LTSPICE_SCHEMATIC::LT_ASC& lt_asc : outLT_ASCs )
    {
        std::vector<LTSPICE_SCHEMATIC::LT_SYMBOL> symbols = lt_asc.Symbols;
        std::map<wxString, LIB_SYMBOL*>           existingSymbol;
        std::map<wxString, SCH_SYMBOL*>           existingSchematicSymbol;

        for( LTSPICE_SCHEMATIC::LT_SYMBOL& lt_symbol : symbols )
        {
            if( !alg::contains( aAsyFiles, lt_symbol.Name ) )
            {
                LIB_SYMBOL* lib_symbol;

                if( existingSymbol.count( lt_symbol.Name ) == 0 )
                {
                    lib_symbol = new LIB_SYMBOL( lt_symbol.Name );

                    CreateSymbol( lt_symbol, lib_symbol );

                    existingSymbol.emplace( lt_symbol.Name, lib_symbol );
                }
                else
                {
                    lib_symbol = existingSymbol[lt_symbol.Name];
                }

                LIB_ID      libId( wxS( "ltspice" ), lt_symbol.Name );
                SCH_SYMBOL* sch_symbol = new SCH_SYMBOL( *lib_symbol, libId, aSheet, 1 );

                CreateFields( lt_symbol, sch_symbol, aSheet );

                for( int j = 0; j < (int) lt_symbol.Wires.size(); j++ )
                    CreateWires( lt_symbol, j, aSheet );

                sch_symbol->Move( ToKicadCoords( lt_symbol.Offset ) + m_originOffset );
                RotateMirror( lt_symbol, sch_symbol );

                aSheet->LastScreen()->Append( sch_symbol );
            }
            else
            {
                for( int j = 0; j < (int) lt_symbol.Lines.size(); j++ )
                    CreateLines( lt_symbol, j, aSheet );

                for( int j = 0; j < (int) lt_symbol.Circles.size(); j++ )
                    CreateCircle( lt_symbol, j, aSheet );

                for( int j = 0; j < (int) lt_symbol.Arcs.size(); j++ )
                    CreateArc( lt_symbol, j, aSheet );

                for( int j = 0; j < (int) lt_symbol.Rectangles.size(); j++ )
                    CreateRect( lt_symbol, j, aSheet );

                // Calculating bounding box
                BOX2I bbox;

                std::vector<LTSPICE_FILE>    tempVector;
                LTSPICE_SCHEMATIC::LT_SYMBOL tempSymbol;
                LTSPICE_FILE                 tempAsyFile( lt_symbol.Name + ".asy", { 0, 0 } );
                LTSPICE_SCHEMATIC::LT_ASC    dummyAsc;

                tempVector.push_back( tempAsyFile );

                tempSymbol = m_lt_schematic->SymbolBuilder( lt_symbol.Name, dummyAsc );

                LIB_SYMBOL* tempLibSymbol = new LIB_SYMBOL( lt_symbol.Name );
                CreateSymbol( tempSymbol, tempLibSymbol );

                bbox = tempLibSymbol->GetBoundingBox();

                int topLeftX = lt_symbol.Offset.x + ToLtSpiceCoords( bbox.GetOrigin().x );
                int topLeftY = lt_symbol.Offset.y + ToLtSpiceCoords( bbox.GetOrigin().y );
                int botRightX = lt_symbol.Offset.x
                                   + ToLtSpiceCoords( bbox.GetOrigin().x )
                                   + ToLtSpiceCoords( bbox.GetSize().x );
                int botRightY = lt_symbol.Offset.y
                                   + ToLtSpiceCoords( bbox.GetOrigin().y )
                                   + ToLtSpiceCoords( bbox.GetSize().y );

                for( LTSPICE_SCHEMATIC::LT_PIN& pin : lt_symbol.Pins )
                {
                    VECTOR2I pinPos = pin.PinLocation;

                    for( LTSPICE_SCHEMATIC::WIRE& wire : lt_asc.Wires )
                    {
                        if( wire.Start == ( pinPos + lt_symbol.Offset ) )
                        {
                            //wire is vertical
                            if( wire.End.x == ( pinPos + lt_symbol.Offset ).x )
                            {
                                if( wire.End.y <= topLeftY )
                                    wire.Start = VECTOR2I( wire.Start.x, topLeftY + 3 );
                                else if( wire.End.y >= botRightY )
                                    wire.Start = VECTOR2I( wire.Start.x, botRightY );
                                else if( wire.End.y < botRightY && wire.End.y > topLeftY )
                                    wire.Start = VECTOR2I( topLeftX, wire.Start.y );
                            }
                            //wire is horizontal
                            else if( wire.End.y == ( pinPos + lt_symbol.Offset ).y )
                            {
                                if( wire.End.x <= topLeftX )
                                    wire.Start = VECTOR2I( topLeftX, wire.Start.y );
                                else if( wire.End.x >= botRightX )
                                    wire.Start = VECTOR2I( botRightX, wire.Start.y );
                                else if( wire.End.x < botRightX && wire.End.x > topLeftX )
                                    wire.Start = VECTOR2I( botRightX, wire.Start.y );
                            }
                        }
                        else if( wire.End == ( pinPos + lt_symbol.Offset ) )
                        {
                            //wire is Vertical
                            if( wire.Start.x == ( pinPos + lt_symbol.Offset ).x )
                            {
                                if( wire.Start.y <= topLeftY )
                                    wire.End = VECTOR2I( wire.End.x, topLeftY );
                                else if( wire.Start.y > botRightY )
                                    wire.End = VECTOR2I( wire.End.x, botRightY );
                                else if( wire.Start.y < botRightY && wire.End.y > topLeftY )
                                    wire.End = VECTOR2I( wire.End.x, botRightY );
                            }
                            //wire is Horizontal
                            else if( wire.Start.y == ( pinPos + lt_symbol.Offset ).y )
                            {
                                if( wire.Start.x <= topLeftX )
                                    wire.End = VECTOR2I( topLeftX, wire.End.y );
                                else if( wire.Start.x >= botRightX )
                                    wire.End = VECTOR2I( botRightX, wire.End.y );
                                else if( wire.Start.x < botRightX && wire.Start.x > topLeftX )
                                    wire.End = VECTOR2I( botRightX, wire.End.y );
                            }
                        }
                    }
                }
            }
        }
    }
}


void LTSPICE_SCH_PARSER::CreateSymbol( LTSPICE_SCHEMATIC::LT_SYMBOL& aLtSymbol,
                                       LIB_SYMBOL*                   aLibSymbol )
{
    for( int j = 0; j < (int) aLtSymbol.Lines.size(); j++ )
    {
        LIB_SHAPE* line = new LIB_SHAPE( aLibSymbol, SHAPE_T::POLY );

        CreateLines( aLibSymbol, aLtSymbol, j, line );
        aLibSymbol->AddDrawItem( line );
    }

    for( int j = 0; j < (int) aLtSymbol.Circles.size(); j++ )
    {
        LIB_SHAPE* circle = new LIB_SHAPE( aLibSymbol, SHAPE_T::CIRCLE );

        CreateCircle( aLtSymbol, j, circle );
        aLibSymbol->AddDrawItem( circle );
    }

    for( int j = 0; j < (int) aLtSymbol.Arcs.size(); j++ )
    {
        LIB_SHAPE* arc = new LIB_SHAPE( aLibSymbol, SHAPE_T::ARC );

        CreateArc( aLtSymbol, j, arc );
        aLibSymbol->AddDrawItem( arc );
    }

    for( int j = 0; j < (int) aLtSymbol.Rectangles.size(); j++ )
    {
        LIB_SHAPE* rectangle = new LIB_SHAPE( aLibSymbol, SHAPE_T::RECTANGLE );

        CreateRect( aLtSymbol, j, rectangle );
        aLibSymbol->AddDrawItem( rectangle );
    }

    for( int j = 0; j < (int) aLtSymbol.Pins.size(); j++ )
    {
        LIB_PIN* pin = new LIB_PIN( aLibSymbol );

        CreatePin( aLtSymbol, j, pin );
        aLibSymbol->AddDrawItem( pin );
    }

    aLibSymbol->SetShowPinNumbers( false );
}


int LTSPICE_SCH_PARSER::ToKicadCoords( int aCoordinate )
{
    return schIUScale.MilsToIU( rescale( 50, aCoordinate, 16 ) );
}


VECTOR2I LTSPICE_SCH_PARSER::ToKicadCoords( const VECTOR2I& aPos )
{
    return VECTOR2I( ToKicadCoords( aPos.x ), ToKicadCoords( aPos.y ) );
}


VECTOR2I LTSPICE_SCH_PARSER::ToInvertedKicadCoords( const VECTOR2I& aPos )
{
    return VECTOR2I( ToKicadCoords( aPos.x ), -ToKicadCoords( aPos.y ) );
}


VECTOR2I LTSPICE_SCH_PARSER::ToKicadFontSize( int aLTFontSize )
{
    auto MILS_SIZE =
            []( int mils )
            {
                return VECTOR2I( schIUScale.MilsToIU( mils ), schIUScale.MilsToIU( mils ) );
            };

    if( aLTFontSize == 1 )      return MILS_SIZE( 36 );
    else if( aLTFontSize == 2 ) return MILS_SIZE( 42 );
    else if( aLTFontSize == 3 ) return MILS_SIZE( 50 );
    else if( aLTFontSize == 4 ) return MILS_SIZE( 60 );
    else if( aLTFontSize == 5 ) return MILS_SIZE( 72 );
    else if( aLTFontSize == 6 ) return MILS_SIZE( 88 );
    else if( aLTFontSize == 7 ) return MILS_SIZE( 108 );
    else                        return ToKicadFontSize( 2 );
}


int LTSPICE_SCH_PARSER::ToLtSpiceCoords( int aCoordinate )
{
    return schIUScale.IUToMils( rescale( 16, aCoordinate, 50 ) );
}


void LTSPICE_SCH_PARSER::RotateMirrorShape( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol,
                                            SCH_SHAPE* aShape )
{
    if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R90 )
    {
        aShape->Rotate( VECTOR2I() );
        aShape->Rotate( VECTOR2I() );
        aShape->Rotate( VECTOR2I() );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R180 )
    {
        aShape->Rotate( VECTOR2I() );
        aShape->Rotate( VECTOR2I() );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R270 )
    {
        aShape->Rotate( VECTOR2I() );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M0 )
    {
        aShape->MirrorVertically( 0 );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M90 )
    {
        aShape->MirrorVertically( 0 );
        aShape->Rotate( VECTOR2I() );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M180 )
    {
        aShape->MirrorHorizontally( 0 );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M270 )
    {
        aShape->MirrorVertically( 0 );
        aShape->Rotate( VECTOR2I() );
        aShape->Rotate( VECTOR2I() );
        aShape->Rotate( VECTOR2I() );
    }
}


void LTSPICE_SCH_PARSER::RotateMirror( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol,
                                       SCH_SYMBOL* aSchSymbol )
{
    if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R0 )
    {
        aSchSymbol->SetOrientation( SYM_ORIENT_0 );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R90 )
    {
        aSchSymbol->SetOrientation( SYM_ORIENT_180 );
        aSchSymbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R180 )
    {
        aSchSymbol->SetOrientation( SYM_ORIENT_180 );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::R270 )
    {
        aSchSymbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M0 )
    {
        aSchSymbol->SetOrientation( SYM_MIRROR_Y );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M90 )
    {
        aSchSymbol->SetOrientation( SYM_MIRROR_Y );
        aSchSymbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M180 )
    {
        aSchSymbol->SetOrientation( SYM_MIRROR_X );
    }
    else if( aLTSymbol.SymbolOrientation == LTSPICE_SCHEMATIC::ORIENTATION::M270 )
    {
        aSchSymbol->SetOrientation( SYM_MIRROR_Y );
        aSchSymbol->SetOrientation( SYM_ROTATE_CLOCKWISE );
    }
}


void LTSPICE_SCH_PARSER::CreateWires( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                      SCH_SHEET_PATH* aSheet )
{
    SCH_LINE* segment = new SCH_LINE();

    segment->SetLineWidth( getLineWidth( LTSPICE_SCHEMATIC::LINEWIDTH::Normal ) );
    segment->SetLineStyle( PLOT_DASH_TYPE::SOLID );

    segment->SetStartPoint( aLTSymbol.Wires[aIndex].Start );
    segment->SetEndPoint( aLTSymbol.Wires[aIndex].End );

    aSheet->LastScreen()->Append( segment );
}


void LTSPICE_SCH_PARSER::CreateKicadSCH_ITEMs( SCH_SHEET_PATH* aSheet,
                                               std::vector<LTSPICE_SCHEMATIC::LT_ASC>& outLT_ASCs )
{
    SCH_SCREEN* screen = aSheet->LastScreen();

    for( LTSPICE_SCHEMATIC::LT_ASC& lt_asc : outLT_ASCs )
    {
        for( int j = 0; j < (int) lt_asc.Lines.size(); j++ )
            CreateLine( lt_asc, j, aSheet );

        for( int j = 0; j < (int) lt_asc.Circles.size(); j++ )
            CreateCircle( lt_asc, j, aSheet );

        for( int j = 0; j < (int) lt_asc.Arcs.size(); j++ )
            CreateArc( lt_asc, j, aSheet );

        for( int j = 0; j < (int) lt_asc.Rectangles.size(); j++ )
            CreateRect( lt_asc, j, aSheet );

        for( int j = 0; j < (int) lt_asc.Bustap.size(); j++ )
            CreateBusEntry( lt_asc, j, aSheet );

        /**
        *
        *NOTE: This current code is used for plotting sch sheet pins on the sheet, we are working on
        * finding how to place the pin on the intersection of line and sheet.
        *
        *
        if( aSubSchematicStructure )
        {
            SCH_HIERLABEL* sheetPin =
                    new SCH_SHEET_PIN( aRootSheet, VECTOR2I(), lt_asc.Flags[index].Value );

            sheetPin->SetText( "PIN" );
            sheetPin->SetPosition( VECTOR2I(
                    schIUScale.MilsToIU( rescale( 50, 592, 16 ) ),
                    schIUScale.MilsToIU( rescale( 50, 133, 16 ) ) ) );
            sheetPin->SetVisible( true );
            aRootSheet->AddPin( (SCH_SHEET_PIN*) sheetPin );
        }*/

        for( int j = 0; j < (int) lt_asc.Wires.size(); j++ )
            CreateWire( lt_asc, j, aSheet, SCH_LAYER_ID::LAYER_WIRE );

        for( int j = 0; j < (int) lt_asc.Iopins.size(); j++ )
            CreatePin( lt_asc, j, aSheet );

        for( const LTSPICE_SCHEMATIC::FLAG& lt_flag : lt_asc.Flags )
        {
            if( lt_flag.Value == wxS( "0" ) )
            {
                screen->Append( CreatePowerSymbol( lt_flag.Offset, lt_flag.Value, lt_flag.FontSize,
                                                   aSheet, lt_asc.Wires ) );
            }
            else
            {
                screen->Append( CreateSCH_LABEL( SCH_LABEL_T, lt_flag.Offset, lt_flag.Value,
                                                 lt_flag.FontSize ) );
            }
        }

        for( const LTSPICE_SCHEMATIC::TEXT& lt_text : lt_asc.Texts )
        {
            screen->Append( CreateSCH_TEXT( lt_text.Offset, lt_text.Value, lt_text.FontSize,
                                            lt_text.Justification ) );
        }

        for( const LTSPICE_SCHEMATIC::DATAFLAG& lt_flag : lt_asc.DataFlags )
        {
            screen->Append( CreateSCH_LABEL( SCH_DIRECTIVE_LABEL_T, lt_flag.Offset,
                                             lt_flag.Expression, lt_flag.FontSize ) );
        }
    }
}


void LTSPICE_SCH_PARSER::CreateBusEntry( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                         SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::BUSTAP& bustap = aAscfile.Bustap[aIndex];

    for( int k = 0; k < (int) aAscfile.Wires.size(); k++ )
    {
        if( ( aAscfile.Wires[k].Start == bustap.Start )
            || ( aAscfile.Wires[k].End == bustap.Start ) )
        {
            CreateWire( aAscfile, k, aSheet, SCH_LAYER_ID::LAYER_BUS );
            aAscfile.Wires.erase( aAscfile.Wires.begin() + k );
        }
    }

    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( ToKicadCoords( { bustap.Start.x,
                                                                            bustap.Start.y - 16 } ) );

    busEntry->SetSize( { ToKicadCoords( 16 ), ToKicadCoords( 16 ) } );

    aSheet->LastScreen()->Append( busEntry );
}


LABEL_FLAG_SHAPE getLabelShape( LTSPICE_SCHEMATIC::POLARITY aPolarity )
{
    if( aPolarity == LTSPICE_SCHEMATIC::POLARITY::INPUT )
        return LABEL_FLAG_SHAPE::L_INPUT;
    else if( aPolarity == LTSPICE_SCHEMATIC::POLARITY::OUTPUT )
        return LABEL_FLAG_SHAPE::L_OUTPUT;
    else
        return LABEL_FLAG_SHAPE::L_BIDI;
}


void LTSPICE_SCH_PARSER::CreatePin( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                    SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::IOPIN& iopin = aAscfile.Iopins[aIndex];
    wxString                  ioPinName;

    for( unsigned int k = 0; k < aAscfile.Flags.size(); k++ )
    {
        if( ( aAscfile.Flags[k].Offset.x == iopin.Location.x )
            && ( aAscfile.Flags[k].Offset.y == iopin.Location.y ) )
        {
            ioPinName = aAscfile.Flags[k].Value;
            aAscfile.Flags.erase( aAscfile.Flags.begin() + k );
        }
    }

    SCH_HIERLABEL* sheetPin =
            new SCH_HIERLABEL( ToKicadCoords( iopin.Location ), ioPinName, SCH_HIER_LABEL_T );

    sheetPin->Move( m_originOffset );

    sheetPin->SetShape( getLabelShape( iopin.Polarity ) );
    aSheet->LastScreen()->Append( sheetPin );
}


void LTSPICE_SCH_PARSER::CreateLine( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                     SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::LINE& lt_line = aAscfile.Lines[aIndex];
    SCH_LINE* line = new SCH_LINE( ToKicadCoords( lt_line.Start ), SCH_LAYER_ID::LAYER_NOTES );

    line->SetEndPoint( ToKicadCoords( lt_line.End ) );
    line->SetStroke( getStroke( lt_line.LineWidth, lt_line.LineStyle ) );
    line->Move( m_originOffset );

    aSheet->LastScreen()->Append( line );
}


void LTSPICE_SCH_PARSER::CreateCircle( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                       SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::CIRCLE& lt_circle = aAscfile.Circles[aIndex];
    SCH_SHAPE*                 circle = new SCH_SHAPE( SHAPE_T::CIRCLE );

    VECTOR2I c = ( lt_circle.TopLeft + lt_circle.BotRight ) / 2;
    int r = ( lt_circle.TopLeft.x - lt_circle.BotRight.x ) / 2;

    circle->SetPosition( ToKicadCoords( c ) );
    circle->SetEnd( ToKicadCoords( c ) + VECTOR2I( abs( ToKicadCoords( r ) ), 0 ) );
    circle->SetStroke( getStroke( lt_circle.LineWidth, lt_circle.LineStyle ) );
    circle->Move( m_originOffset );

    aSheet->LastScreen()->Append( circle );
}


void LTSPICE_SCH_PARSER::CreateArc( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                    SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::ARC& lt_arc = aAscfile.Arcs[aIndex];
    SCH_SHAPE*              arc = new SCH_SHAPE( SHAPE_T::ARC );

    arc->SetCenter( ToKicadCoords( ( lt_arc.TopLeft + lt_arc.BotRight ) / 2 ) );
    arc->SetEnd( ToKicadCoords( lt_arc.ArcEnd ) );
    arc->SetStart( ToKicadCoords( lt_arc.ArcStart ) );
    arc->SetStroke( getStroke( lt_arc.LineWidth, lt_arc.LineStyle ) );
    arc->Move( m_originOffset );

    aSheet->LastScreen()->Append( arc );
}


void LTSPICE_SCH_PARSER::CreateRect( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                     SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::RECTANGLE& lt_rect = aAscfile.Rectangles[aIndex];
    SCH_SHAPE*                    rectangle = new SCH_SHAPE( SHAPE_T::RECTANGLE );

    rectangle->SetPosition( ToKicadCoords( lt_rect.TopLeft ) );
    rectangle->SetEnd( ToKicadCoords( lt_rect.BotRight ) );
    rectangle->SetStroke( getStroke( lt_rect.LineWidth, lt_rect.LineStyle ) );
    rectangle->Move( m_originOffset );

    aSheet->LastScreen()->Append( rectangle );
}


int LTSPICE_SCH_PARSER::getLineWidth( const LTSPICE_SCHEMATIC::LINEWIDTH& aLineWidth )
{
    if( aLineWidth == LTSPICE_SCHEMATIC::LINEWIDTH::Normal )
        return schIUScale.MilsToIU( 6 );
    else if( aLineWidth == LTSPICE_SCHEMATIC::LINEWIDTH::Wide )
        return schIUScale.MilsToIU( 12 );
    else
        return schIUScale.MilsToIU( 6 );
}


PLOT_DASH_TYPE LTSPICE_SCH_PARSER::getLineStyle( const LTSPICE_SCHEMATIC::LINESTYLE& aLineStyle )
{
    switch( aLineStyle )
    {
    case LTSPICE_SCHEMATIC::LINESTYLE::SOLID:      return PLOT_DASH_TYPE::SOLID;
    case LTSPICE_SCHEMATIC::LINESTYLE::DOT:        return PLOT_DASH_TYPE::DOT;
    case LTSPICE_SCHEMATIC::LINESTYLE::DASHDOTDOT: return PLOT_DASH_TYPE::DASHDOTDOT;
    case LTSPICE_SCHEMATIC::LINESTYLE::DASHDOT:    return PLOT_DASH_TYPE::DASHDOT;
    case LTSPICE_SCHEMATIC::LINESTYLE::DASH:       return PLOT_DASH_TYPE::DASH;
    default:                                       return PLOT_DASH_TYPE::SOLID;
    }
}


STROKE_PARAMS LTSPICE_SCH_PARSER::getStroke( const LTSPICE_SCHEMATIC::LINEWIDTH& aLineWidth,
                                             const LTSPICE_SCHEMATIC::LINESTYLE& aLineStyle )
{
    return STROKE_PARAMS( getLineWidth( aLineWidth ), getLineStyle( aLineStyle ) );
}


void LTSPICE_SCH_PARSER::setTextJustification( EDA_TEXT*                        aText,
                                               LTSPICE_SCHEMATIC::JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case LTSPICE_SCHEMATIC::JUSTIFICATION::LEFT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VLEFT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::CENTER:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VCENTER:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::RIGHT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VRIGHT:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::BOTTOM:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VBOTTOM:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::TOP:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VTOP:
        aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
        break;

    default: break;
    }

    switch( aJustification )
    {
    case LTSPICE_SCHEMATIC::JUSTIFICATION::LEFT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::CENTER:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::RIGHT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::BOTTOM:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::TOP:
        aText->SetTextAngle( ANGLE_HORIZONTAL );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::VLEFT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VCENTER:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VRIGHT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VBOTTOM:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VTOP:
        aText->SetTextAngle( ANGLE_VERTICAL );
        break;

    default: break;
    }

    // Center, Left, Right aligns by first line in multiline text
    if( wxSplit( aText->GetText(), '\n', '\0' ).size() > 1 )
    {
        switch( aJustification )
        {
        case LTSPICE_SCHEMATIC::JUSTIFICATION::LEFT:
        case LTSPICE_SCHEMATIC::JUSTIFICATION::CENTER:
        case LTSPICE_SCHEMATIC::JUSTIFICATION::RIGHT:
            aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            aText->Offset( VECTOR2I( 0, -aText->GetTextHeight() / 2 ) );
            break;

        case LTSPICE_SCHEMATIC::JUSTIFICATION::VLEFT:
        case LTSPICE_SCHEMATIC::JUSTIFICATION::VCENTER:
        case LTSPICE_SCHEMATIC::JUSTIFICATION::VRIGHT:
            aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            aText->Offset( VECTOR2I( -aText->GetTextHeight() / 2, 0 ) );
            break;

        default: break;
        }
    }
}


SCH_TEXT* LTSPICE_SCH_PARSER::CreateSCH_TEXT( VECTOR2I aOffset, const wxString& aText,
                                              int aFontSize,
                                              LTSPICE_SCHEMATIC::JUSTIFICATION aJustification )
{
    VECTOR2I  pos = ToKicadCoords( aOffset ) + m_originOffset;
    SCH_TEXT* textItem = new SCH_TEXT( pos, aText, SCH_TEXT_T );

    textItem->SetTextSize( ToKicadFontSize( aFontSize ) );
    textItem->SetVisible( true );
    textItem->SetMultilineAllowed( true );

    setTextJustification( textItem, aJustification );

    return textItem;
}


void LTSPICE_SCH_PARSER::CreateWire( LTSPICE_SCHEMATIC::LT_ASC& aAscfile, int aIndex,
                                     SCH_SHEET_PATH* aSheet, SCH_LAYER_ID aLayer )
{
    SCH_LINE* segment = new SCH_LINE();

    segment->SetLineWidth( getLineWidth( LTSPICE_SCHEMATIC::LINEWIDTH::Normal ) );
    segment->SetLineStyle( PLOT_DASH_TYPE::SOLID );
    segment->SetLayer( aLayer );

    segment->SetStartPoint( ToKicadCoords( aAscfile.Wires[aIndex].Start ) + m_originOffset );
    segment->SetEndPoint( ToKicadCoords( aAscfile.Wires[aIndex].End ) + m_originOffset );

    aSheet->LastScreen()->Append( segment );
}


SCH_SYMBOL* LTSPICE_SCH_PARSER::CreatePowerSymbol( const VECTOR2I& aOffset, const wxString& aValue,
                                                   int aFontSize, SCH_SHEET_PATH* aSheet,
                                                   std::vector<LTSPICE_SCHEMATIC::WIRE>& aWires )
{
    LIB_SYMBOL* lib_symbol = new LIB_SYMBOL( wxS( "GND" ) );
    LIB_SHAPE*  shape = new LIB_SHAPE( lib_symbol, SHAPE_T::POLY );

    shape->AddPoint( ToInvertedKicadCoords( { 16, 0 } ) );
    shape->AddPoint( ToInvertedKicadCoords( { -16, 0 } ) );
    shape->AddPoint( ToInvertedKicadCoords( { 0, 15 } ) );
    shape->AddPoint( ToInvertedKicadCoords( { 16, 0 } ) );
    shape->AddPoint( ToInvertedKicadCoords( { -16, 0 } ) );
    shape->AddPoint( ToInvertedKicadCoords( { 0, 15 } ) );

    shape->SetStroke( STROKE_PARAMS( getLineWidth( LTSPICE_SCHEMATIC::LINEWIDTH::Normal ),
                                     PLOT_DASH_TYPE::SOLID ) );

    lib_symbol->AddDrawItem( shape );
    lib_symbol->SetPower();

    LIB_PIN* pin = new LIB_PIN( lib_symbol );

    pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    pin->SetPosition( ToInvertedKicadCoords( { 0, 0 } ) );
    pin->SetLength( 5 );
    pin->SetShape( GRAPHIC_PINSHAPE::LINE );
    lib_symbol->AddDrawItem( pin );

    LIB_ID libId( wxS( "ltspice" ), wxS( "GND" ) );
    SCH_SYMBOL* sch_symbol = new SCH_SYMBOL( *lib_symbol, libId, aSheet, 1 );

    sch_symbol->SetRef( aSheet, wxString::Format( wxS( "#GND%03d" ), m_powerSymbolIndex++ ) );
    sch_symbol->GetField( REFERENCE_FIELD )->SetVisible( false );
    sch_symbol->SetValueFieldText( wxS( "0" ) );
    sch_symbol->GetField( VALUE_FIELD )->SetTextSize( ToKicadFontSize( aFontSize ) );
    sch_symbol->GetField( VALUE_FIELD )->SetVisible( false );

    sch_symbol->Move( ToKicadCoords( aOffset ) + m_originOffset );

    for( LTSPICE_SCHEMATIC::WIRE& wire : aWires )
    {
        if( aOffset == wire.Start )
        {
            if( wire.Start.x == wire.End.x )
            {
                if( wire.Start.y < wire.End.y )
                {
                    sch_symbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                    sch_symbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                }
            }
            else
            {
                if( wire.Start.x < wire.End.x )
                    sch_symbol->SetOrientation( SYM_ROTATE_CLOCKWISE );
                else if( wire.Start.x > wire.End.x )
                    sch_symbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
            }
        }
        else if( aOffset == wire.End )
        {
            if( wire.Start.x == wire.End.x )
            {
                if( wire.Start.y > wire.End.y )
                {
                    sch_symbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                    sch_symbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                }
            }
            else
            {
                if( wire.Start.x < wire.End.x )
                    sch_symbol->SetOrientation( SYM_ROTATE_COUNTERCLOCKWISE );
                else if( wire.Start.x > wire.End.x )
                    sch_symbol->SetOrientation( SYM_ROTATE_CLOCKWISE );
            }
        }
    }

    return sch_symbol;
}


SCH_LABEL_BASE* LTSPICE_SCH_PARSER::CreateSCH_LABEL( KICAD_T aType, const VECTOR2I& aOffset,
                                                     const wxString& aValue, int aFontSize )
{
    SCH_LABEL_BASE* label = nullptr;

    if( aType == SCH_LABEL_T )
    {
        label = new SCH_LABEL();

        label->SetText( aValue );
        label->SetTextSize( ToKicadFontSize( aFontSize ) );
        label->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );
    }
    else if( aType == SCH_DIRECTIVE_LABEL_T )
    {
        label = new SCH_DIRECTIVE_LABEL();

        label->SetTextSpinStyle( TEXT_SPIN_STYLE::RIGHT );

        SCH_FIELD field( { 0, 0 }, -1, label, wxS( "DATAFLAG" ) );
        field.SetText( aValue );
        field.SetTextSize( ToKicadFontSize( aFontSize ) );
        field.SetVisible( true );

        label->AddField( field );
        label->AutoplaceFields( nullptr, false );
    }
    else
    {
        UNIMPLEMENTED_FOR( aType );
    }

    if( label )
    {
        label->SetPosition( ToKicadCoords( aOffset ) + m_originOffset );
        label->SetVisible( true );
    }

    return label;
}


void LTSPICE_SCH_PARSER::CreateFields( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol,
                                       SCH_SYMBOL* aSymbol, SCH_SHEET_PATH* aSheet )
{
    wxString libPath = m_lt_schematic->GetLTspiceDataDir().GetFullPath() + wxS( "lib/" );
    wxString symbolName = aLTSymbol.Name.Upper();
    wxString type = aLTSymbol.SymAttributes[ wxS( "TYPE" ) ].Upper();
    wxString prefix = aLTSymbol.SymAttributes[ wxS( "PREFIX" ) ].Upper();
    wxString instName = aLTSymbol.SymAttributes[ wxS( "INSTNAME" ) ].Upper();
    wxString value = aLTSymbol.SymAttributes[ wxS( "VALUE" ) ];

    aSymbol->SetRef( aSheet, instName );
    aSymbol->SetValueFieldText( value );

    auto setupNonInferredPassive =
            [&]( const wxString& aPrefix )
            {
                SCH_FIELD deviceField( { 0, 0 }, -1, aSymbol, wxS( "Sim.Device" ) );
                deviceField.SetText( aPrefix );
                aSymbol->AddField( deviceField );

                SCH_FIELD paramsField( { 0, 0 }, -1, aSymbol, wxS( "Sim.Params" ) );
                paramsField.SetText( aPrefix + wxS( "=${VALUE}" ) );
                aSymbol->AddField( paramsField );
            };

    if( symbolName == wxS( "RES" ) )
    {
        if( !instName.StartsWith( 'R' ) )
            setupNonInferredPassive( wxS( "R" ) );
    }
    else if( symbolName == wxS( "CAP" ) )
    {
        if( !instName.StartsWith( 'C' ) )
            setupNonInferredPassive( wxS( "C" ) );
    }
    else if( symbolName == wxS( "IND" ) )
    {
        if( !instName.StartsWith( 'L' ) )
            setupNonInferredPassive( wxS( "L" ) );
    }
    else if( symbolName == wxS( "VOLTAGE" ) || symbolName == wxS( "CURRENT" ) )
    {
        // inference had better work....
    }
    else
    {
        wxString libFile = aLTSymbol.SymAttributes[ wxS( "MODELFILE" ) ];

        if( prefix == wxS( "X" ) )
        {
            // A prefix of X overrides the simulation model for other symbols (npn, etc.)
            type = wxS( "X" );
        }
        else if( libFile.IsEmpty() )
        {
            if( type.IsEmpty() )
                type = symbolName;

            if( type == "DIODE" )
                libFile = libPath + wxS( "cmp/standard.dio" );
            else if( type == "NPN" || type == "PNP" )
                libFile = libPath + wxS( "cmp/standard.bjt" );
            else if( type == "NJF" || type == "PJF" )
                libFile = libPath + wxS( "cmp/standard.jft" );
            else if( type == "NMOS" || type == "PMOS" )
                libFile = libPath + wxS( "cmp/standard.mos" );
        }

        if( libFile.IsEmpty() )
            libFile = m_includes[ value ];

        if( !libFile.IsEmpty() )
        {
            SCH_FIELD libField( { 0, 0 }, -1, aSymbol, wxS( "Sim.Library" ) );
            libField.SetText( libFile );
            aSymbol->AddField( libField );
        }

        SCH_FIELD nameField( { 0, 0 }, -1, aSymbol, wxS( "Sim.Name" ) );
        nameField.SetText( wxS( "${VALUE}" ) );
        aSymbol->AddField( nameField );

        if( type == wxS( "X" ) )
        {
            SCH_FIELD deviceField( { 0, 0 }, -1, aSymbol, wxS( "Sim.Device" ) );
            deviceField.SetText( wxS( "SUBCKT" ) );
            aSymbol->AddField( deviceField );
        }

        wxString spiceLine = aLTSymbol.SymAttributes[ wxS( "SPICELINE" ) ];

        if( !spiceLine.IsEmpty() )
        {
            SCH_FIELD paramsField( { 0, 0 }, -1, aSymbol, wxS( "Sim.Params" ) );
            paramsField.SetText( spiceLine );
            aSymbol->AddField( paramsField );
        }
    }

    for( LTSPICE_SCHEMATIC::LT_WINDOW& lt_window : aLTSymbol.Windows )
    {
        SCH_FIELD* field = nullptr;

        switch( lt_window.WindowNumber )
        {
        case -1: /* PartNum    */                                                     break;
        case 0:  /* InstName   */ field = aSymbol->GetField( REFERENCE_FIELD );       break;
        case 1:  /* Type       */                                                     break;
        case 2:  /* RefName    */                                                     break;
        case 3:  /* Value      */ field = aSymbol->GetField( VALUE_FIELD );           break;

        case 5:  /* QArea      */                                                     break;

        case 8:  /* Width      */                                                     break;
        case 9:  /* Length     */                                                     break;
        case 10: /* Multi      */                                                     break;

        case 16: /* Nec        */                                                     break;

        case 38: /* SpiceModel */ field = aSymbol->FindField( wxS( "Sim.Name" ) );    break;
        case 39: /* SpiceLine  */ field = aSymbol->FindField( wxS( "Sim.Params" ) );  break;
        case 40: /* SpiceLine2 */                                                     break;

        /*
           47   Def_Sub

           50   Digital_Timing_Model
           51   Digital_Extracts
           52   Digital_IO_Model
           53   Digital_Line
           54   Digital_Primitive
           55   Digital_MNTYMXDLY
           56   Digital_IO_LEVEL
           57   Digital_StdCell
           58   Digital_File

          105   Cell
          106   W/L
          107   PSIZE
          108   NSIZE
          109   sheets
          110   sh#
          111   Em_Scale
          112   Epi
          113   Sinker
          114   Multi5

          118   AQ
          119   AQSUB
          120   ZSIZE
          121   ESR
          123   Value2
          124   COUPLE
          125   Voltage
          126   Area1
          127   Area2
          128   Area3
          129   Area4
          130   Multi1
          131   Multi2
          132   Multi3
          133   Multi4
          134   DArea
          135   DPerim
          136   CArea
          137   CPerim
          138   Shrink
          139   Gate_Resize

          142   BP
          143   BN
          144   Sim_Level

          146   G_Voltage

          150   SpiceLine3

          153   D_VOLTAGES

          156   Version
          157   Comment
          158   XDef_Sub
          159   LVS_Area

          162   User1
          163   User2
          164   User3
          165   User4
          166   User5
          167   Root
          168   Class
          169   Geometry
          170   WL_Delimiter

          175   T1
          176   T2

          184   DsgnName
          185   Designer

          190   RTN
          191   PWR
          192   BW

          201   CAPROWS
          202   CAPCOLS
          203   NF
          204   SLICES
          205   CUR
          206   TEMPRISE
          207   STRIPS
          208   WEM
          209   LEM
          210   BASES
          211   COLS
          212   XDef_Tub
          */
        default: break;
        }

        if( field )
        {
            field->SetPosition( ToKicadCoords( lt_window.Position ) );
            field->SetTextSize( ToKicadFontSize( lt_window.FontSize ) );

            if( lt_window.FontSize == 0 )
                field->SetVisible( false );

            setTextJustification( field, lt_window.Justification );
        }
    }
}


void LTSPICE_SCH_PARSER::CreateRect( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                     LIB_SHAPE* aRectangle )
{
    LTSPICE_SCHEMATIC::RECTANGLE& lt_rect = aLTSymbol.Rectangles[aIndex];

    aRectangle->SetPosition( ToInvertedKicadCoords( lt_rect.BotRight ) );
    aRectangle->SetEnd( ToInvertedKicadCoords( lt_rect.TopLeft ) );
    aRectangle->SetStroke( getStroke(  lt_rect.LineWidth, lt_rect.LineStyle ) );

    if( aLTSymbol.SymAttributes[wxS( "Prefix" )] == wxS( "X" ) )
        aRectangle->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
}


void LTSPICE_SCH_PARSER::CreateRect( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                     SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::RECTANGLE& lt_rect = aLTSymbol.Rectangles[aIndex];
    SCH_SHAPE*                    rectangle = new SCH_SHAPE( SHAPE_T::RECTANGLE );

    rectangle->SetPosition( ToInvertedKicadCoords( lt_rect.BotRight ) );
    rectangle->SetEnd( ToInvertedKicadCoords( lt_rect.TopLeft ) );
    rectangle->SetStroke( getStroke( lt_rect.LineWidth, lt_rect.LineStyle ) );

    rectangle->Move( aLTSymbol.Offset );
    RotateMirrorShape( aLTSymbol, rectangle );

    aSheet->LastScreen()->Append( rectangle );
}


void LTSPICE_SCH_PARSER::CreatePin( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                    LIB_PIN* aPin )
{
    LTSPICE_SCHEMATIC::LT_PIN& lt_pin = aLTSymbol.Pins[aIndex];
    wxString                   device = aLTSymbol.Name.Lower();

    if( aLTSymbol.Pins.size() == 2 && ( device == wxS( "res" )
                                     || device == wxS( "cap" )
                                     || device == wxS( "ind" ) ) )
    {
        // drop A/B pin names from simple LRCs as they're not terribly useful (and prevent
        // other pin names on the net from driving the net name).
    }
    else
    {
        aPin->SetName( lt_pin.PinAttribute[ wxS( "PinName" ) ] );

        if( lt_pin.PinJustification == LTSPICE_SCHEMATIC::JUSTIFICATION::NONE )
            aPin->SetNameTextSize( 0 );
    }

    aPin->SetNumber( wxString::Format( wxS( "%d" ), aIndex + 1 ) );
    aPin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
    aPin->SetPosition( ToInvertedKicadCoords( lt_pin.PinLocation ) );
    aPin->SetLength( 5 );
    aPin->SetShape( GRAPHIC_PINSHAPE::LINE );

    switch( lt_pin.PinJustification )
    {
    case LTSPICE_SCHEMATIC::JUSTIFICATION::LEFT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VLEFT:
        aPin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::RIGHT:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VRIGHT:
        aPin->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::BOTTOM:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VBOTTOM:
        aPin->SetOrientation( PIN_ORIENTATION::PIN_UP );
        break;

    case LTSPICE_SCHEMATIC::JUSTIFICATION::TOP:
    case LTSPICE_SCHEMATIC::JUSTIFICATION::VTOP:
        aPin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );
        break;

    default: break;
    }
}


void LTSPICE_SCH_PARSER::CreateArc( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                    LIB_SHAPE* aArc )
{
    LTSPICE_SCHEMATIC::ARC& lt_arc = aLTSymbol.Arcs[aIndex];

    aArc->SetCenter( ToInvertedKicadCoords( ( lt_arc.TopLeft + lt_arc.BotRight ) / 2 ) );
    aArc->SetEnd( ToInvertedKicadCoords( lt_arc.ArcEnd ) );
    aArc->SetStart( ToInvertedKicadCoords( lt_arc.ArcStart ) );
    aArc->SetStroke( getStroke( lt_arc.LineWidth, lt_arc.LineStyle ) );
}


void LTSPICE_SCH_PARSER::CreateArc( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                    SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::ARC& lt_arc = aLTSymbol.Arcs[aIndex];
    SCH_SHAPE*              arc = new SCH_SHAPE( SHAPE_T::ARC );

    arc->SetCenter( ToInvertedKicadCoords( ( lt_arc.TopLeft + lt_arc.BotRight ) / 2 ) );
    arc->SetEnd( ToInvertedKicadCoords( lt_arc.ArcEnd ) );
    arc->SetStart( ToInvertedKicadCoords( lt_arc.ArcStart ) );
    arc->SetStroke( getStroke( lt_arc.LineWidth, lt_arc.LineStyle ) );

    arc->Move( ToKicadCoords( aLTSymbol.Offset ) + m_originOffset );
    RotateMirrorShape( aLTSymbol, arc );

    aSheet->LastScreen()->Append( arc );
}


void LTSPICE_SCH_PARSER::CreateCircle( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                       SCH_SHEET_PATH* aSheet )
{
    LTSPICE_SCHEMATIC::CIRCLE& lt_circle = aLTSymbol.Circles[aIndex];
    SCH_SHAPE*                 circle = new SCH_SHAPE( SHAPE_T::CIRCLE );

    VECTOR2I c = ( lt_circle.TopLeft + lt_circle.BotRight ) / 2;
    int r = ( lt_circle.TopLeft.x - lt_circle.BotRight.x ) / 2;

    circle->SetPosition( ToInvertedKicadCoords( c ) );
    circle->SetEnd( ToInvertedKicadCoords( c ) + VECTOR2I( abs( ToKicadCoords( r ) ), 0 ) );
    circle->SetStroke( getStroke( lt_circle.LineWidth, lt_circle.LineStyle ) );

    circle->Move( aLTSymbol.Offset );
    RotateMirrorShape( aLTSymbol, circle );

    aSheet->LastScreen()->Append( circle );
}


void LTSPICE_SCH_PARSER::CreateCircle( LTSPICE_SCHEMATIC::LT_SYMBOL& aLTSymbol, int aIndex,
                                       LIB_SHAPE* aCircle )
{
    LTSPICE_SCHEMATIC::CIRCLE& lt_circle = aLTSymbol.Circles[aIndex];

    VECTOR2I c = ( lt_circle.TopLeft + lt_circle.BotRight ) / 2;
    int r = ( lt_circle.TopLeft.x - lt_circle.BotRight.x ) / 2;

    aCircle->SetPosition( ToInvertedKicadCoords( c ) );
    aCircle->SetEnd( ToInvertedKicadCoords( c ) + VECTOR2I( abs( ToKicadCoords( r ) ), 0 ) );
    aCircle->SetStroke( getStroke( lt_circle.LineWidth, lt_circle.LineStyle ) );
}


