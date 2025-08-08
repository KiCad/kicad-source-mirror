/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Chetan Subhash Shinde<chetanshinde2001@gmail.com>
 * Copyright (C) 2023 CERN
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
 * @brief Loads the asc file and asy files.
 */

#include "sch_io/ltspice/ltspice_schematic.h"

#include <sch_io/ltspice/sch_io_ltspice_parser.h>
#include <sch_screen.h>
#include <wx/log.h>
#include <wx/dir.h>
#include <wildcards_and_files_ext.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <project.h>
#include <richio.h>
#include <algorithm>


void LTSPICE_SCHEMATIC::Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                              const wxFileName& aLibraryFileName, REPORTER* aReporter )
{
    std::map<wxString, wxString> mapOfAscFiles;
    std::map<wxString, wxString> mapOfAsyFiles;

    // Library paths to search (Give highest priority to files contained in same directory)
    GetAscAndAsyFilePaths( aLibraryFileName.GetPath(), false, mapOfAscFiles, mapOfAsyFiles );

    // TODO: Custom paths go here (non-recursive)

    // Default LTspice libs
    GetAscAndAsyFilePaths( m_ltspiceDataDir.GetPathWithSep() + wxS( "sub" ), true, mapOfAscFiles,
                           mapOfAsyFiles );

    GetAscAndAsyFilePaths( m_ltspiceDataDir.GetPathWithSep() + wxS( "sym" ), true, mapOfAscFiles,
                           mapOfAsyFiles );

    m_schematic = aSchematic;

    std::queue<wxString> ascFileQueue;
    ascFileQueue.push( aLibraryFileName.GetName().Lower() );

    LTSPICE_FILE rootAscFile( ascFileQueue.front(), { 0, 0 } );

    rootAscFile.Sheet = aRootSheet;
    rootAscFile.Screen = new SCH_SCREEN();

    int parentSheetIndex = 0;

    // Asc files who are subschematic in nature
    std::vector<LTSPICE_FILE> ascFiles;

    ascFiles.push_back( rootAscFile );

    while( !ascFileQueue.empty() )
    {
        SCH_SCREEN* screen = nullptr;

        // Reading the .asc file
        wxString ascFilePath = mapOfAscFiles[ ascFileQueue.front() ];
        wxString buffer = SafeReadFile( ascFilePath, "r" );

        std::vector<LTSPICE_FILE> newSubSchematicElements = GetSchematicElements( buffer );

        std::erase_if( newSubSchematicElements,
                        [&mapOfAscFiles]( const LTSPICE_FILE& ii )
                        {
                            return mapOfAscFiles[ii.ElementName].IsEmpty();
                        } );

        for( LTSPICE_FILE& newSubSchematicElement : newSubSchematicElements )
        {
            wxString asyName = newSubSchematicElement.ElementName;
            auto     it = mapOfAsyFiles.find( asyName );

            if( it == mapOfAsyFiles.end() )
                continue;

            wxString asyBuffer = SafeReadFile( it->second, "r" );

            if( IsAsySubsheet( asyBuffer ) )
            {
                if( !screen )
                    screen = new SCH_SCREEN( m_schematic );

                newSubSchematicElement.ParentIndex = parentSheetIndex;
                newSubSchematicElement.Screen = screen;
                newSubSchematicElement.Sheet = new SCH_SHEET();

                ascFileQueue.push( newSubSchematicElement.ElementName );
                ascFiles.push_back( newSubSchematicElement );
            }
        }

        ascFileQueue.pop();

        parentSheetIndex++;
    }

    for( unsigned int i = 0; i < ascFiles.size(); i++ )
    {
        // Reading the .asc file
        wxString buffer = SafeReadFile( mapOfAscFiles[ascFiles[i].ElementName], wxS( "r" ) );

        // Getting the keywords to read
        {
            std::vector<LTSPICE_FILE> sourceFiles = GetSchematicElements( buffer );

            m_fileCache[ wxS( "asyFiles" ) ] = ReadAsyFiles( sourceFiles, mapOfAsyFiles );
            m_fileCache[ wxS( "ascFiles" ) ][ wxS( "parentFile" ) ] = buffer;

            for( const LTSPICE_FILE& file : sourceFiles )
                wxASSERT( file.Sheet == nullptr && file.Screen == nullptr );
        }

        SCH_SHEET_PATH     curSheetPath;
        SCH_IO_LTSPICE_PARSER parser( this );

        if( i > 0 )
        {
            SCH_SHEET* curSheet = ascFiles[i].Sheet;
            std::map   tempAsyMap = ReadAsyFile( ascFiles[i], mapOfAsyFiles );
            wxString   ascFileName = ascFiles[i].ElementName;
            LT_ASC     dummyAsc;
            LT_SYMBOL  tempSymbol = SymbolBuilder( ascFileName, tempAsyMap[ascFileName], dummyAsc );
            LIB_SYMBOL tempLibSymbol( ascFiles[i].ElementName );

            parser.CreateSymbol( tempSymbol, &tempLibSymbol );

            BOX2I bbox = tempLibSymbol.GetBoundingBox();

            curSheet->SetSize( bbox.GetSize() );
            curSheet->SetPosition( parser.ToKicadCoords( ascFiles[i].Offset ) + bbox.GetOrigin() );
            curSheet->SetParent( ascFiles[ascFiles[i].ParentIndex].Sheet );

            SCH_FIELD* sheetNameField = curSheet->GetField( FIELD_T::SHEET_NAME );
            SCH_FIELD* fileNameSheet = curSheet->GetField( FIELD_T::SHEET_FILENAME );
            wxString   sheetName = wxString::Format( wxS( "%s-subsheet-%d" ),
                                                     ascFiles[i].ElementName,
                                                     i );

            sheetNameField->SetText( sheetName );
            fileNameSheet->SetText( sheetName + ".kicad_sch" );

            curSheet->SetScreen( ascFiles[i].Screen );

            curSheetPath = ascFiles[ascFiles[i].ParentIndex].SheetPath;
            curSheetPath.push_back( curSheet );

            ascFiles[i].SheetPath = curSheetPath;

            ascFiles[ascFiles[i].ParentIndex].Sheet->GetScreen()->Append( curSheet );

            curSheet->GetScreen()->SetFileName( m_schematic->Project().GetProjectPath() + sheetName + ".kicad_sch" );
        }
        else
        {
            SCH_SHEET* curSheet = ascFiles[i].Sheet;

            ascFiles[i].SheetPath.push_back( curSheet );
            curSheetPath = ascFiles[i].SheetPath;
        }

        std::vector<wxString> subSchematicAsyFiles;

        for( const LTSPICE_FILE& ascFile : ascFiles )
            subSchematicAsyFiles.push_back( ascFile.ElementName );

        try
        {
            std::vector<LTSPICE_SCHEMATIC::LT_ASC> lt_ascs = StructureBuilder();
            parser.Parse( &curSheetPath, lt_ascs, subSchematicAsyFiles );
        }
        catch( IO_ERROR& e )
        {
            aReporter->Report( e.What(), RPT_SEVERITY_ERROR );
        }
    }
}


void LTSPICE_SCHEMATIC::GetAscAndAsyFilePaths( const wxDir& aDir, bool aRecursive,
                                               std::map<wxString, wxString>& aMapOfAscFiles,
                                               std::map<wxString, wxString>& aMapOfAsyFiles,
                                               const std::vector<wxString>&  aBaseDirs )
{
    wxString filename;

    {
        bool cont = aDir.GetFirst( &filename, wxEmptyString, wxDIR_FILES | wxDIR_HIDDEN );

        while( cont )
        {
            wxFileName path( aDir.GetName(), filename );

            auto logToMap = [&]( std::map<wxString, wxString>& aMapToLogTo, const wxString& aKey )
            {
                if( aMapToLogTo.count( aKey ) )
                {
                    if( m_reporter )
                    {
                        m_reporter->Report( wxString::Format( _( "File at '%s' was ignored. Using previously "
                                                                 "found file at '%s' instead." ),
                                                              path.GetFullPath(),
                                                              aMapToLogTo.at( aKey ) ) );
                    }
                }
                else
                {
                    aMapToLogTo.emplace( aKey, path.GetFullPath() );
                }
            };

            // Add dir1/dir2/cmp, dir2/cmp, cmp
            wxString extension = path.GetExt().Lower();

            for( size_t i = 0; i < aBaseDirs.size() + 1; i++ )
            {
                wxString alias;

                for( size_t j = i; j < aBaseDirs.size(); j++ )
                    alias << aBaseDirs[j].Lower() << '/';

                alias << path.GetName().Lower();

                if( extension == wxS( "asc" ) )
                    logToMap( aMapOfAscFiles, alias );
                else if( extension == wxS( "asy" ) )
                    logToMap( aMapOfAsyFiles, alias );
            }

            cont = aDir.GetNext( &filename );
        }
    }

    if( aRecursive )
    {
        bool cont = aDir.GetFirst( &filename, wxEmptyString, wxDIR_DIRS | wxDIR_HIDDEN );

        while( cont )
        {
            wxFileName path( aDir.GetName(), filename );
            wxDir      subDir( path.GetFullPath() );

            std::vector<wxString> newBase = aBaseDirs;
            newBase.push_back( filename );

            GetAscAndAsyFilePaths( subDir, true, aMapOfAscFiles, aMapOfAsyFiles, newBase );

            cont = aDir.GetNext( &filename );
        }
    }
}


std::map<wxString, wxString>
LTSPICE_SCHEMATIC::ReadAsyFile( const LTSPICE_FILE& aSourceFile,
                                const std::map<wxString, wxString>& aAsyFileMap )
{
    std::map<wxString, wxString> resultantMap;

    wxString fileName = aSourceFile.ElementName;

    if( aAsyFileMap.count( fileName ) )
        resultantMap[fileName] = SafeReadFile( aAsyFileMap.at( fileName ), wxS( "r" ) );

    return resultantMap;
}


std::map<wxString, wxString>
LTSPICE_SCHEMATIC::ReadAsyFiles( const std::vector<LTSPICE_FILE>& aSourceFiles,
                                 const std::map<wxString, wxString>& aAsyFileMap )
{
    std::map<wxString, wxString> resultantMap;

    for( const LTSPICE_FILE& source : aSourceFiles )
    {
        wxString fileName = source.ElementName;

        if( aAsyFileMap.count( fileName ) )
            resultantMap[fileName] = SafeReadFile( aAsyFileMap.at( fileName ), wxS( "r" ) );
    }

    return resultantMap;
}


std::vector<LTSPICE_FILE> LTSPICE_SCHEMATIC::GetSchematicElements( const wxString& aAscFile )
{
    std::vector<LTSPICE_FILE> resultantArray;
    wxArrayString             lines = wxSplit( aAscFile, '\n' );

    for( const wxString& line : lines )
    {
        wxArrayString tokens = wxSplit( line, ' ' );

        if( tokens.size() >= 4 && tokens[0].Upper() == wxS( "SYMBOL" ) )
        {
            wxString elementName( tokens[1] );
            long     posX, posY;

            tokens[2].ToLong( &posX );
            tokens[3].ToLong( &posY );

            elementName.Replace( '\\', '/' );

            LTSPICE_FILE asyFile( elementName, VECTOR2I( (int) posX, (int) posY ) );

            resultantArray.push_back( asyFile );
        }
    }

    return resultantArray;
}


bool LTSPICE_SCHEMATIC::IsAsySubsheet( const wxString& aAsyFile )
{
    wxStringTokenizer lines( aAsyFile, "\n" );

    while( lines.HasMoreTokens() )
    {
        wxStringTokenizer parts( lines.GetNextToken(), " " );

        if( parts.GetNextToken().IsSameAs( wxS( "SYMATTR" ), false )
            && parts.GetNextToken().IsSameAs( wxS( "Prefix" ), false ) )
        {
            return false;
        }
    }

    return true;
}


int LTSPICE_SCHEMATIC::integerCheck( const wxString& aToken, int aLineNumber,
                                     const wxString& aFileName )
{
    long result;

    if( !aToken.ToLong( &result ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Expecting integer at line %d in file %s" ),
                                          aLineNumber,
                                          aFileName ) );
    }

    return (int) result;
}


VECTOR2I LTSPICE_SCHEMATIC::pointCheck( const wxString& aTokenX, const wxString& aTokenY,
                                        int aLineNumber, const wxString& aFileName )
{
    return VECTOR2I( integerCheck( aTokenX, aLineNumber, aFileName ),
                     integerCheck( aTokenY, aLineNumber, aFileName ) );
}


void LTSPICE_SCHEMATIC::tokensSizeRangeCheck( size_t aActualSize, int aExpectedMin,
                                              int aExpectedMax, int aLineNumber,
                                              const wxString& aFileName )
{
    if( (int) aActualSize < aExpectedMin )
    {
        THROW_IO_ERROR( wxString::Format( _( "Expected data missing on line %d in file %s" ),
                                          aLineNumber,
                                          aFileName ) );
    }
    else if( (int) aActualSize > aExpectedMax )
    {
        THROW_IO_ERROR( wxString::Format( _( "Extra data found on line %d in file %s" ),
                                          aLineNumber,
                                          aFileName ) );
    }
}


void LTSPICE_SCHEMATIC::aggregateAttributeValue( wxArrayString& aTokens, int aIndex )
{
    // Merges a value which is across multiple tokens into one token with spaces in between.
    for( int i = aIndex + 1; i < (int) aTokens.GetCount(); i++ )
        aTokens[ aIndex ] += " " + aTokens[i];
}


LTSPICE_SCHEMATIC::LINESTYLE LTSPICE_SCHEMATIC::getLineStyle( int aValue )
{
    std::map<int, LINESTYLE> lineStyleMap;

    lineStyleMap[0] = LINESTYLE::SOLID;
    lineStyleMap[1] = LINESTYLE::DASH;
    lineStyleMap[2] = LINESTYLE::DOT;
    lineStyleMap[3] = LINESTYLE::DASHDOT;
    lineStyleMap[4] = LINESTYLE::DASHDOTDOT;

    if( lineStyleMap.find( aValue ) == lineStyleMap.end() )
        THROW_IO_ERROR( _( "Expecting 0, 1, 2, 3 or 4" ) );

    return lineStyleMap[ aValue ];
}


LTSPICE_SCHEMATIC::LINEWIDTH LTSPICE_SCHEMATIC::getLineWidth( const wxString& aValue )
{
    std::map<wxString, LINEWIDTH> lineWidthMap;

    lineWidthMap["NORMAL"] = LINEWIDTH::Normal;
    lineWidthMap["WIDE"] = LINEWIDTH::Wide;

    if( lineWidthMap.find( aValue.Upper() ) == lineWidthMap.end() )
        THROW_IO_ERROR( _( "Expecting NORMAL or WIDE" ) );

    return lineWidthMap[ aValue.Upper() ];
}


LTSPICE_SCHEMATIC::POLARITY LTSPICE_SCHEMATIC::getPolarity( const wxString& aValue )
{
    std::map<wxString, POLARITY> polarityMap;

    polarityMap["I"] = POLARITY::PIN_INPUT;
    polarityMap["O"] = POLARITY::OUTPUT;
    polarityMap["B"] = POLARITY::BIDIR;
    polarityMap["IN"] = POLARITY::PIN_INPUT;
    polarityMap["OUT"] = POLARITY::OUTPUT;
    polarityMap["BIDIR"] = POLARITY::BIDIR;

    if( polarityMap.find( aValue.Upper() ) == polarityMap.end() )
        THROW_IO_ERROR( _( "Expecting I, O, B, IN, OUT or BIDIR" ) );

    return polarityMap[ aValue.Upper() ];
}


LTSPICE_SCHEMATIC::ORIENTATION LTSPICE_SCHEMATIC::getSymbolRotationOrMirror( const wxString& aValue )
{
    std::map<wxString, ORIENTATION> rotationMirrorMap;

    rotationMirrorMap["R0"] = ORIENTATION::R0;
    rotationMirrorMap["R90"] = ORIENTATION::R90;
    rotationMirrorMap["R180"] = ORIENTATION::R180;
    rotationMirrorMap["R270"] = ORIENTATION::R270;

    rotationMirrorMap["M0"] = ORIENTATION::M0;
    rotationMirrorMap["M90"] = ORIENTATION::M90;
    rotationMirrorMap["M180"] = ORIENTATION::M180;
    rotationMirrorMap["M270"] = ORIENTATION::M270;

    if( rotationMirrorMap.find( aValue.Upper() ) == rotationMirrorMap.end() )
        THROW_IO_ERROR( _( "Expecting R0, R90, R18, R270, M0, M90, M180 or M270" ) );

    return rotationMirrorMap[ aValue.Upper() ];
}


LTSPICE_SCHEMATIC::JUSTIFICATION LTSPICE_SCHEMATIC::getTextJustification( const wxString& aValue )
{
    std::map<wxString, JUSTIFICATION> justificationMap;

    justificationMap["LEFT"] = JUSTIFICATION::LEFT;
    justificationMap["CENTER"] = JUSTIFICATION::CENTER;
    justificationMap["RIGHT"] = JUSTIFICATION::RIGHT;
    justificationMap["VLEFT"] = JUSTIFICATION::VLEFT;
    justificationMap["VRIGHT"] = JUSTIFICATION::VRIGHT;
    justificationMap["VCENTER"] = JUSTIFICATION::VCENTER;
    justificationMap["BOTTOM"] = JUSTIFICATION::BOTTOM;
    justificationMap["TOP"] = JUSTIFICATION::TOP;
    justificationMap["VBOTTOM"] = JUSTIFICATION::VBOTTOM;
    justificationMap["VTOP"] = JUSTIFICATION::VTOP;
    justificationMap["INVISIBLE"] = JUSTIFICATION::INVISIBLE;

    if( justificationMap.find( aValue.Upper() ) == justificationMap.end() )
        THROW_IO_ERROR( _( "Expecting LEFT, CENTER, RIGHT, TOP, BOTTOM, VLEFT, VRIGHT, VCENTER, "
                           "VTOP, VBOTTOM or INVISIBLE" ) );

    return justificationMap[ aValue.Upper() ];
}


LTSPICE_SCHEMATIC::JUSTIFICATION LTSPICE_SCHEMATIC::getPinJustification( const wxString& aValue )
{
    std::map<wxString, JUSTIFICATION> pinJustificationMap;

    pinJustificationMap["BOTTOM"] = JUSTIFICATION::BOTTOM;
    pinJustificationMap["NONE"] = JUSTIFICATION::NONE;
    pinJustificationMap["LEFT"] = JUSTIFICATION::LEFT;
    pinJustificationMap["RIGHT"] = JUSTIFICATION::RIGHT;
    pinJustificationMap["TOP"] = JUSTIFICATION::TOP;
    pinJustificationMap["VBOTTOM"] = JUSTIFICATION::VBOTTOM;
    pinJustificationMap["VLEFT"] = JUSTIFICATION::VLEFT;
    pinJustificationMap["VRIGHT"] = JUSTIFICATION::VRIGHT;
    pinJustificationMap["VCENTER"] = JUSTIFICATION::VCENTER;
    pinJustificationMap["VTOP"] = JUSTIFICATION::VTOP;

    if( pinJustificationMap.find( aValue.Upper() ) == pinJustificationMap.end() )
        THROW_IO_ERROR( _( "Expecting NONE, BOTTOM, TOP, LEFT, RIGHT, VBOTTOM, VTOP, VCENTER, VLEFT or VRIGHT" ) );

    return pinJustificationMap[ aValue.Upper() ];
}


LTSPICE_SCHEMATIC::SYMBOLTYPE LTSPICE_SCHEMATIC::getSymbolType( const wxString& aValue )
{
    std::map<wxString, SYMBOLTYPE> symbolTypeMap;

    symbolTypeMap["CELL"] = SYMBOLTYPE::CELL;
    symbolTypeMap["BLOCK"] = SYMBOLTYPE::BLOCK;

    if( symbolTypeMap.find( aValue.Upper() ) == symbolTypeMap.end() )
        THROW_IO_ERROR( _( "Expecting CELL or BLOCK" ) );

    return symbolTypeMap[ aValue.Upper() ];
}


void LTSPICE_SCHEMATIC::removeCarriageReturn( wxString& elementFromLine )
{
    if( elementFromLine.EndsWith( '\r' ) )
        elementFromLine = elementFromLine.BeforeLast( '\r' );
}


LTSPICE_SCHEMATIC::LT_SYMBOL LTSPICE_SCHEMATIC::SymbolBuilder( const wxString& aAscFileName,
                                                               LT_ASC& aAscFile )
{
    const std::map<wxString, wxString>& asyFiles = m_fileCache[ wxS( "asyFiles" ) ];

    if( !asyFiles.count( aAscFileName.Lower() ) )
        THROW_IO_ERROR( wxString::Format( _( "Symbol '%s.asy' not found" ), aAscFileName ) );

    return SymbolBuilder( aAscFileName, asyFiles.at( aAscFileName.Lower() ), aAscFile );
}

LTSPICE_SCHEMATIC::LT_SYMBOL LTSPICE_SCHEMATIC::SymbolBuilder( const wxString& aAscFileName,
                                                               const wxString& aAsyFileContent,
                                                               LT_ASC& aAscFile )
{
    LT_SYMBOL lt_symbol;
    int       lineNumber = 1;

    lt_symbol.Name = aAscFileName;
    lt_symbol.SymbolType = LTSPICE_SCHEMATIC::SYMBOLTYPE::CELL;
    lt_symbol.SymbolOrientation = LTSPICE_SCHEMATIC::ORIENTATION::R0;

    for( wxString line : wxSplit( aAsyFileContent, '\n' ) )
    {
        removeCarriageReturn( line );

        wxArrayString tokens = wxSplit( line, ' ' );

        if( tokens.IsEmpty() )
            continue;

        wxString element = tokens[0].Upper();

        if( element == "LINE" )
        {
            tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, aAscFileName );

            wxString lineWidth = tokens[1];
            wxString startPointX = tokens[2];
            wxString startPointY = tokens[3];
            wxString endPointX = tokens[4];
            wxString endPointY = tokens[5];

            LINE lt_line;
            lt_line.LineWidth = getLineWidth( lineWidth );
            lt_line.Start = pointCheck( startPointX, startPointY, lineNumber, aAscFileName );
            lt_line.End = pointCheck( endPointX, endPointY, lineNumber, aAscFileName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 7 )
            {
                wxString lineStyle = tokens[6];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aAscFileName );
            }

            lt_line.LineStyle = getLineStyle( lineStyleNumber );

            lt_symbol.Lines.push_back( lt_line );
        }
        else if( element == "RECTANGLE" )
        {
            tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, aAscFileName );

            wxString lineWidth = tokens[1];
            wxString botRightX = tokens[2];
            wxString botRightY = tokens[3];
            wxString topLeftX = tokens[4];
            wxString topRightY = tokens[5];

            RECTANGLE rect;
            rect.LineWidth = getLineWidth( lineWidth );
            rect.BotRight = pointCheck( botRightX, botRightY, lineNumber, aAscFileName );
            rect.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, aAscFileName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 7 )
            {
                wxString lineStyle = tokens[6];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aAscFileName );
            }

            rect.LineStyle = getLineStyle( lineStyleNumber );

            lt_symbol.Rectangles.push_back( rect );
        }
        else if( element == "CIRCLE" )
        {
            /**
             *  The Circle is enclosed in the square which is represented by bottomRight and
             *  topLeft coordinates.
             */
            tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, aAscFileName );

            wxString lineWidth = tokens[1];
            wxString botRightX = tokens[2];
            wxString botRightY = tokens[3];
            wxString topLeftX = tokens[4];
            wxString topRightY = tokens[5];

            CIRCLE circle;
            circle.LineWidth = getLineWidth( lineWidth );
            circle.BotRight = pointCheck( botRightX, botRightY, lineNumber, aAscFileName );
            circle.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, aAscFileName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 7 )
            {
                wxString lineStyle = tokens[6];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aAscFileName );
            }

            circle.LineStyle = getLineStyle( lineStyleNumber );

            lt_symbol.Circles.push_back( circle );
        }
        else if( element == "ARC" )
        {
            /**
             *  The Arc is enclosed in the square given by above coordinates and its start and end
             *  coordinates are given.
             *  The arc is drawn counterclockwise from the starting point to the ending point.
             */
            tokensSizeRangeCheck( tokens.size(), 10, 11, lineNumber, aAscFileName );

            wxString lineWidth = tokens[1];
            wxString botRightX = tokens[2];
            wxString botRightY = tokens[3];
            wxString topLeftX = tokens[4];
            wxString topRightY = tokens[5];
            wxString arcStartPointX = tokens[6];
            wxString arcStartPointY = tokens[7];
            wxString arcEndPointX = tokens[8];
            wxString arcEndPointY = tokens[9];

            ARC arc;
            arc.LineWidth = getLineWidth( lineWidth );
            arc.BotRight = pointCheck( botRightX, botRightY, lineNumber, aAscFileName );
            arc.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, aAscFileName );
            arc.ArcStart = pointCheck( arcStartPointX, arcStartPointY, lineNumber, aAscFileName );
            arc.ArcEnd = pointCheck( arcEndPointX, arcEndPointY, lineNumber, aAscFileName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 11 )
            {
                wxString lineStyle = tokens[10];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aAscFileName );
            }

            arc.LineStyle = getLineStyle( lineStyleNumber );

            lt_symbol.Arcs.push_back( arc );
        }
        else if( element == "WINDOW" )
        {
            tokensSizeRangeCheck( tokens.size(), 6, 6, lineNumber, aAscFileName );

            wxString number = tokens[1];
            wxString windowPosX = tokens[2];
            wxString windowPosY = tokens[3];
            wxString justification = tokens[4];
            wxString fontSize = tokens[5];

            LT_WINDOW window;
            window.WindowNumber = integerCheck( number, lineNumber, aAscFileName );
            window.Position = pointCheck( windowPosX, windowPosY, lineNumber, aAscFileName );
            window.Justification = getTextJustification( justification );
            window.FontSize = integerCheck( fontSize, lineNumber, aAscFileName );

            // LTSpice appears to ignore hidden property from .asy files
            if( window.FontSize == 0 )
                window.FontSize = 2;

            lt_symbol.Windows.push_back( window );
        }
        else if( element == "SYMATTR" )
        {
            aggregateAttributeValue( tokens, 2 );

            tokensSizeRangeCheck( tokens.size(), 3, INT_MAX, lineNumber, aAscFileName );

            wxString key = tokens[1];
            wxString value = tokens[2];

            if( value == wxS( "\"\"" ) )
                value = wxEmptyString;

            lt_symbol.SymAttributes[ key.Upper() ] = value;
        }
        else if( element == "PIN" )
        {
            tokensSizeRangeCheck( tokens.size(), 5, 5, lineNumber, aAscFileName );

            wxString pinLocationX = tokens[1];
            wxString pinLocationY = tokens[2];
            wxString Justification = tokens[3];
            wxString nameOffSet = tokens[4];

            LT_PIN pin;
            pin.PinLocation = pointCheck( pinLocationX, pinLocationY, lineNumber,aAscFileName );
            pin.PinJustification = getPinJustification( Justification );
            pin.NameOffSet = integerCheck( nameOffSet, lineNumber, aAscFileName );

            lt_symbol.Pins.push_back( pin );
        }
        else if( element == "PINATTR" )
        {
            aggregateAttributeValue( tokens, 2 );

            tokensSizeRangeCheck( tokens.size(), 3, INT_MAX, lineNumber, aAscFileName );

            wxString name = tokens[1];
            wxString Value = tokens[2];

            lt_symbol.Pins.back().PinAttribute.insert( { name, Value } );
        }
        else if( element == "SYMBOLTYPE" )
        {
            tokensSizeRangeCheck( tokens.size(), 2, 2, lineNumber, aAscFileName );

            wxString symbolType = tokens[1];

            lt_symbol.SymbolType = getSymbolType( symbolType );
        }

        lineNumber++;
    }

    return lt_symbol;
}


LTSPICE_SCHEMATIC::LT_SYMBOL LTSPICE_SCHEMATIC::MakeDummySymbol( const wxString& aAscFileName )
{
    LT_SYMBOL lt_symbol;

    lt_symbol.Name = aAscFileName;
    lt_symbol.SymbolType = LTSPICE_SCHEMATIC::SYMBOLTYPE::CELL;
    lt_symbol.SymbolOrientation = LTSPICE_SCHEMATIC::ORIENTATION::R0;

    RECTANGLE rect;
    rect.LineWidth = getLineWidth( "NORMAL" );
    rect.LineStyle = LINESTYLE::SOLID;
    rect.BotRight = VECTOR2I( 20, 20 );
    rect.TopLeft = VECTOR2I( -20, -20 );

    lt_symbol.Rectangles.push_back( rect );

    return lt_symbol;
}


std::vector<LTSPICE_SCHEMATIC::LT_ASC> LTSPICE_SCHEMATIC::StructureBuilder()
{
    // Initialising Symbol Struct

    std::vector<LT_ASC> ascFiles;

    for( const auto& [ fileName, contents ] : m_fileCache[ wxS( "ascFiles" ) ] )
    {
        LT_ASC                 ascFile;
        std::vector<LT_SYMBOL> symbolArray;

        ascFile.SheetSize = VECTOR2I( 0, 0 );
        ascFile.Symbols = symbolArray;
        ascFile.Version = 0;
        ascFile.VersionMinor = 0;
        ascFile.SheetNumber = 0;

        int lineNumber = 1;

        for( wxString line : wxSplit( contents, '\n' ) )
        {
            removeCarriageReturn( line );

            wxArrayString tokens = wxSplit( line, ' ' );

            if( tokens.IsEmpty() )
                continue;

            wxString element = tokens[0].Upper();

            if( element == "SHEET" )
            {
                tokensSizeRangeCheck( tokens.size(), 4, 4, lineNumber, fileName );

                wxString sheetNumber = tokens[1];
                wxString sheetWidth = tokens[2];
                wxString sheetHeight = tokens[3];

                ascFile.SheetNumber = integerCheck( sheetNumber, lineNumber, fileName );
                ascFile.SheetSize = pointCheck( sheetWidth, sheetHeight, lineNumber, fileName );
            }
            else if( element == "SYMBOL" )
            {
                tokensSizeRangeCheck( tokens.size(), 5, 5, lineNumber, fileName );

                wxString symbolName = tokens[1];
                wxString posX = tokens[2];
                wxString posY = tokens[3];
                wxString rotate_mirror_option = tokens[4];

                symbolName.Replace( '\\', '/' );

                try
                {
                    LT_SYMBOL lt_symbol = SymbolBuilder( symbolName, ascFile );
                    lt_symbol.Offset = pointCheck( posX, posY, lineNumber, fileName );
                    lt_symbol.SymbolOrientation = getSymbolRotationOrMirror( rotate_mirror_option );

                    ascFile.Symbols.push_back( lt_symbol );
                    ascFile.BoundingBox.Merge( lt_symbol.Offset );
                }
                catch( IO_ERROR& e )
                {
                    if( m_reporter )
                        m_reporter->Report( e.What(), RPT_SEVERITY_ERROR );

                    // Use dummy symbol
                    LT_SYMBOL lt_symbol = MakeDummySymbol( symbolName );
                    lt_symbol.Offset = pointCheck( posX, posY, lineNumber, fileName );
                    lt_symbol.SymbolOrientation = getSymbolRotationOrMirror( rotate_mirror_option );

                    ascFile.Symbols.push_back( lt_symbol );
                    ascFile.BoundingBox.Merge( lt_symbol.Offset );
                }
            }
            else if( element == "WIRE" )
            {
                tokensSizeRangeCheck( tokens.size(), 5, 5, lineNumber, fileName );

                wxString startPointX = tokens[1];
                wxString startPointY = tokens[2];
                wxString endPointX = tokens[3];
                wxString endPointY = tokens[4];

                WIRE wire;
                wire.Start = pointCheck( startPointX, startPointY, lineNumber, fileName );
                wire.End = pointCheck( endPointX, endPointY, lineNumber, fileName );

                ascFile.Wires.push_back( wire );
                ascFile.BoundingBox.Merge( wire.Start );
                ascFile.BoundingBox.Merge( wire.End );
            }
            else if( element == "FLAG" )
            {
                tokensSizeRangeCheck( tokens.size(), 4, 4, lineNumber, fileName );

                wxString posX = tokens[1];
                wxString posY = tokens[2];
                wxString name = tokens[3];

                FLAG flag;
                flag.Offset = pointCheck( posX, posY, lineNumber, fileName );
                flag.Value = name;
                flag.FontSize = 2;

                ascFile.Flags.push_back( flag );
                ascFile.BoundingBox.Merge( flag.Offset );
            }
            else if( element == "DATAFLAG" )
            {
                tokensSizeRangeCheck( tokens.size(), 4, 4, lineNumber, fileName );

                wxString posX = tokens[1];
                wxString posY = tokens[2];
                wxString expression = tokens[3];

                DATAFLAG flag;
                flag.Offset = pointCheck( posX, posY, lineNumber, fileName );
                flag.Expression = expression;
                flag.FontSize = 2;

                ascFile.DataFlags.push_back( flag );
                ascFile.BoundingBox.Merge( flag.Offset );
            }
            else if( element == "WINDOW" )
            {
                tokensSizeRangeCheck( tokens.size(), 6, 6, lineNumber, fileName );

                wxString number = tokens[1];
                wxString windowPosX = tokens[2];
                wxString windowPosY = tokens[3];
                wxString justification = tokens[4];
                wxString fontSize = tokens[5];

                int        windowNumber = integerCheck( number, lineNumber, fileName );
                LT_WINDOW* window = nullptr;

                // Overwrite an existing window from the symbol definition
                for( LT_WINDOW& candidate : ascFile.Symbols.back().Windows )
                {
                    if( candidate.WindowNumber == windowNumber )
                    {
                        window = &candidate;
                        break;
                    }
                }

                if( !window )
                {
                    ascFile.Symbols.back().Windows.emplace_back( LT_WINDOW() );
                    window = &ascFile.Symbols.back().Windows.back();
                }

                window->WindowNumber = windowNumber;
                window->Position = pointCheck( windowPosX, windowPosY, lineNumber, fileName );
                window->Justification = getTextJustification( justification );
                window->FontSize = integerCheck( fontSize, lineNumber, fileName );

                ascFile.BoundingBox.Merge( window->Position );
            }
            else if( element == "SYMATTR" )
            {
                tokensSizeRangeCheck( tokens.size(), 3, INT_MAX, lineNumber, fileName );

                aggregateAttributeValue( tokens, 2 );

                wxString name = tokens[1];
                wxString value = tokens[2];

                if( value == wxS( "\"\"" ) )
                    value = wxEmptyString;

                ascFile.Symbols.back().SymAttributes[ name.Upper() ] = value;
            }
            else if( element == "LINE" )
            {
                tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, fileName );

                wxString lineWidth = tokens[1];
                wxString startPointX = tokens[2];
                wxString startPointY = tokens[3];
                wxString endPointX = tokens[4];
                wxString endPointY = tokens[5];

                LINE lt_line;
                lt_line.LineWidth = getLineWidth( lineWidth );
                lt_line.Start = pointCheck( startPointX, startPointY, lineNumber, fileName );
                lt_line.End = pointCheck( endPointX, endPointY, lineNumber, fileName );

                int lineStyleNumber = 0;  // default

                if( tokens.size() == 7 )
                {
                    wxString lineStyle = tokens[6];
                    lineStyleNumber = integerCheck( lineStyle, lineNumber, fileName );
                }

                lt_line.LineStyle = getLineStyle( lineStyleNumber );

                ascFile.Lines.push_back( lt_line );
                ascFile.BoundingBox.Merge( lt_line.Start );
                ascFile.BoundingBox.Merge( lt_line.End );
            }
            else if( element == "RECTANGLE" )
            {
                tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, fileName );

                // wxString lineWidth = tokens[1];
                wxString botRightX = tokens[2];
                wxString botRightY = tokens[3];
                wxString topLeftX = tokens[4];
                wxString topRightY = tokens[5];

                RECTANGLE rect;
                rect.LineWidth = getLineWidth( tokens[1] );
                rect.BotRight = pointCheck( botRightX, botRightY, lineNumber, fileName );
                rect.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, fileName );

                int lineStyleNumber = 0;  // default

                if( tokens.size() == 7 )
                {
                    wxString lineStyle = tokens[6];
                    lineStyleNumber = integerCheck( lineStyle, lineNumber, fileName );
                }

                rect.LineStyle = getLineStyle( lineStyleNumber );

                ascFile.Rectangles.push_back( rect );
                ascFile.BoundingBox.Merge( rect.TopLeft );
                ascFile.BoundingBox.Merge( rect.BotRight );
            }
            else if( element == "CIRCLE" )
            {
                tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, fileName );

                wxString lineWidth = tokens[1];
                wxString botRightX = tokens[2];
                wxString botRightY = tokens[3];
                wxString topLeftX = tokens[4];
                wxString topRightY = tokens[5];

                CIRCLE circle;
                circle.LineWidth = getLineWidth( lineWidth );
                circle.BotRight = pointCheck( botRightX, botRightY, lineNumber, fileName );
                circle.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, fileName );

                int lineStyleNumber = 0;  // default

                if( tokens.size() == 7 )
                {
                    wxString lineStyle = tokens[6];
                    lineStyleNumber = integerCheck( lineStyle, lineNumber, fileName );
                }

                circle.LineStyle = getLineStyle( lineStyleNumber );

                ascFile.Circles.push_back( circle );
                ascFile.BoundingBox.Merge( circle.TopLeft );
                ascFile.BoundingBox.Merge( circle.BotRight );
            }
            else if( element == "ARC" )
            {
                /**
                 *  The Arc is enclosed in the square given by above coordinates and its start and
                 *  end coordinates are given.
                 *  The arc is drawn counterclockwise from the starting point to the ending point.
                 */
                tokensSizeRangeCheck( tokens.size(), 10, 11, lineNumber, fileName );

                wxString lineWidth = tokens[1];
                wxString botRightX = tokens[2];
                wxString botRightY = tokens[3];
                wxString topLeftX = tokens[4];
                wxString topRightY = tokens[5];
                wxString arcStartPointX = tokens[6];
                wxString arcStartPointY = tokens[7];
                wxString arcEndPointX = tokens[8];
                wxString arcEndPointY = tokens[9];

                ARC arc;
                arc.LineWidth = getLineWidth( lineWidth );
                arc.BotRight = pointCheck( botRightX, botRightY, lineNumber, fileName );
                arc.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, fileName );
                arc.ArcEnd = pointCheck( arcStartPointX, arcStartPointY, lineNumber, fileName );
                arc.ArcStart = pointCheck( arcEndPointX, arcEndPointY, lineNumber, fileName );

                int lineStyleNumber = 0;  // default

                if( tokens.size() == 11 )
                {
                    wxString lineStyle = tokens[10];
                    lineStyleNumber = integerCheck( lineStyle, lineNumber, fileName );
                }

                arc.LineStyle = getLineStyle( lineStyleNumber );

                ascFile.Arcs.push_back( arc );
                ascFile.BoundingBox.Merge( arc.TopLeft );
                ascFile.BoundingBox.Merge( arc.BotRight );
            }
            else if( element == "IOPIN" )
            {
                tokensSizeRangeCheck( tokens.size(), 4, 4, lineNumber, fileName );

                wxString pinLocationX = tokens[1];
                wxString pinLocationY = tokens[2];
                wxString pinPolarity = tokens[3];

                IOPIN iopin;
                iopin.Location = pointCheck( pinLocationX, pinLocationY, lineNumber, fileName );
                iopin.Polarity = getPolarity( pinPolarity );

                ascFile.Iopins.push_back( iopin );
                ascFile.BoundingBox.Merge( iopin.Location );
            }
            else if( element == "TEXT" )
            {
                aggregateAttributeValue( tokens, 5 );

                tokensSizeRangeCheck( tokens.size(), 6, INT_MAX, lineNumber, fileName );

                wxString positionX = tokens[1];
                wxString positionY = tokens[2];
                wxString justification = tokens[3];
                wxString fontSize = tokens[4];
                wxString value = tokens[5];

                TEXT text;
                text.Offset = pointCheck( positionX, positionY, lineNumber, fileName );
                text.Justification = getTextJustification( justification );
                text.FontSize = integerCheck( fontSize, lineNumber, fileName );

                if( value.StartsWith( wxS( "!" ), &text.Value ) )
                    text.Value.Replace( wxS( "! " ), wxS( "\n" ) );  // replace subsequent ! with \n
                else if( value.StartsWith( wxS( ";" ), &text.Value ) )
                    text.Value.Replace( wxS( "; " ), wxS( "\n" ) );  // replace subsequent ; with \n
                else
                    text.Value = value;

                text.Value.Replace( wxS( "\\n" ), wxS( "\n" ) );

                ascFile.Texts.push_back( text );
                ascFile.BoundingBox.Merge( text.Offset );
            }
            else if( element == "BUSTAP" )
            {
                tokensSizeRangeCheck( tokens.size(), 5, 5, lineNumber, fileName );

                wxString startPointX = tokens[1];
                wxString startPointY = tokens[2];
                wxString endPointX = tokens[3];
                wxString endPointY = tokens[4];

                BUSTAP bustap;
                bustap.Start = pointCheck( startPointX, startPointY, lineNumber, fileName );
                bustap.End = pointCheck( endPointX, endPointY, lineNumber, fileName );

                ascFile.Bustap.push_back( bustap );
                ascFile.BoundingBox.Merge( bustap.Start );
                ascFile.BoundingBox.Merge( bustap.End );
            }
            else if( element == "VERSION" )
            {
                wxString versionNumber = tokens[1];
                if( versionNumber.Contains( '.' ) )
                {
                    wxString majorStr = versionNumber.BeforeFirst( '.' );
                    wxString minorStr = versionNumber.AfterFirst( '.' );
                    ascFile.Version = integerCheck( majorStr, lineNumber, fileName );
                    ascFile.VersionMinor = integerCheck( minorStr, lineNumber, fileName );
                }
                else
                {
                    ascFile.Version = integerCheck( versionNumber, lineNumber, fileName );
                }
            }

            lineNumber++;
        }

        ascFiles.push_back( std::move( ascFile ) );
    }

    return ascFiles;
}


