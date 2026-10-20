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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @brief Loads the asc file and asy files.
 */

#include "sch_io/ltspice/ltspice_schematic.h"

#include <sch_io/ltspice/sch_io_ltspice_parser.h>
#include <sch_screen.h>
#include <wx/log.h>
#include <wx/dir.h>
#include <wx/tokenzr.h>
#include <wildcards_and_files_ext.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <page_info.h>
#include <project.h>
#include <lib_id.h>
#include <richio.h>
#include <string_utils.h>
#include <algorithm>


// Files are found in this order: foo/amp.asy, then bar/amp.asy.
// The map is then:
//   "foo/amp" -> foo/amp.asy
//   "amp"     -> foo/amp.asy
//   "bar/amp" -> bar/amp.asy
//
// "foo/amp" returns "amp", because "amp" is foo/amp.asy.
// "bar/amp" returns "bar/amp", because "amp" is foo/amp.asy, not bar/amp.asy.
static wxString getShortestKey( const std::map<wxString, wxString>& aMap, const wxString& aName )
{
    wxString key = aName.Lower();
    key.Replace( '\\', '/' );

    if( aMap.contains( key ) )
    {
        while( key.Contains( '/' ) )
        {
            wxString shorter = key.AfterFirst( '/' );

            if( !aMap.contains( shorter ) || aMap.at( shorter ) != aMap.at( key ) )
                break;

            key = shorter;
        }

        return key;
    }
    else
    {
        key = key.AfterLast( '/' );

        if( aMap.contains( key ) )
            return key;
    }

    return {};
}


wxString LTSPICE_SCHEMATIC::GetSchematicFileName( const wxString& aAscKey ) const
{
    wxString name = getShortestKey( m_ascNamesMap, aAscKey );

    if( name.IsEmpty() )
        name = aAscKey.Lower();

    ReplaceIllegalFileNameChars( name, '_' );

    return name + '.' + FILEEXT::KiCadSchematicFileExtension;
}


wxString LTSPICE_SCHEMATIC::GetSymbolName( const wxString& aSymbolName ) const
{
    wxString name = getShortestKey( m_asyNamesMap, aSymbolName );

    return LIB_ID::FixIllegalChars( name, false ).wx_str();
}


void LTSPICE_SCHEMATIC::Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet, const wxFileName& aLibraryFileName,
                              REPORTER* aReporter )
{
    m_ascFileCache.clear();
    m_asyFileCache.clear();
    m_ascNamesMap.clear();
    m_asyNamesMap.clear();

    // Library paths to search (Give highest priority to files contained in same directory)
    GetAscAndAsyFilePaths( aLibraryFileName.GetPath(), false, m_ascNamesMap, m_asyNamesMap );

    // TODO: Custom paths go here (non-recursive)

    // Default LTspice libs
    GetAscAndAsyFilePaths( m_ltspiceDataDir.GetPathWithSep() + wxS( "sub" ), true, m_ascNamesMap, m_asyNamesMap );
    GetAscAndAsyFilePaths( m_ltspiceDataDir.GetPathWithSep() + wxS( "sym" ), true, m_ascNamesMap, m_asyNamesMap );

    wxString              rootElementName = aLibraryFileName.GetName().Lower();
    std::vector<wxString> uniqueAscNameKeys;

    std::function<void( const wxString& )> collectHierarchy;
    collectHierarchy = [&]( const wxString& aAscNameKey )
    {
        uniqueAscNameKeys.push_back( aAscNameKey );

        if( !m_ascNamesMap.contains( aAscNameKey ) )
        {
            aReporter->Report( wxString::Format( _( "Missing '%s' schematic file" ), aAscNameKey + wxS( ".asc" ) ),
                               RPT_SEVERITY_ERROR );
            return;
        }

        wxString buffer = SafeReadFile( m_ascNamesMap.at( aAscNameKey ), wxS( "r" ) );
        m_ascFileCache[m_ascNamesMap.at( aAscNameKey )] = buffer;

        std::vector<LTSPICE_FILE> newSubSchematicElements = GetSchematicElements( buffer );

        for( LTSPICE_FILE& newSubSchematicElement : newSubSchematicElements )
        {
            wxString asyKey = getShortestKey( m_asyNamesMap, newSubSchematicElement.ElementName );

            if( alg::contains( uniqueAscNameKeys, asyKey ) )
                continue;

            if( asyKey.IsEmpty() )
                continue;

            wxString asyBuffer = SafeReadFile( m_asyNamesMap.at( asyKey ), wxS( "r" ) );
            m_asyFileCache[m_asyNamesMap.at( asyKey )] = asyBuffer;

            if( IsAsySubsheet( asyBuffer ) )
                collectHierarchy( asyKey );
        }
    };

    collectHierarchy( rootElementName );

    std::map<wxString, LTSPICE_SCHEMATIC::LT_ASC> parsedAscFiles = ParseAscFiles( m_ascFileCache );
    std::map<wxString, SCH_SCREEN*>               mapOfScreens;

    // Create a SCH_SCREEN for each unique .asc file
    for( const wxString& elementName : uniqueAscNameKeys )
    {
        SCH_SCREEN* screen;

        if( elementName == rootElementName )
            screen = aRootSheet->GetScreen();
        else
            screen = new SCH_SCREEN( aSchematic );

        screen->SetFileName( aSchematic->Project().GetProjectPath() + GetSchematicFileName( elementName ) );

        mapOfScreens[elementName] = screen;
    }

    // Create KiCad items for each .asc file
    for( const wxString& elementName : uniqueAscNameKeys )
    {
        SCH_IO_LTSPICE_PARSER parser( this );

        if( !m_ascNamesMap.contains( elementName ) )
            continue;

        if( !parsedAscFiles.contains( m_ascNamesMap[elementName] ) )
            continue;

        LTSPICE_SCHEMATIC::LT_ASC& ascFile = parsedAscFiles[m_ascNamesMap[elementName]];
        SCH_SCREEN*                screen = mapOfScreens[elementName];

        const int grid = schIUScale.MilsToIU( 50 );
        const int margin = grid * 10;
        VECTOR2I  pageSize;
        BOX2I     bbox;

        bbox.Merge( parser.ToKicadCoords( ascFile.BoundingBox.GetOrigin() ) );
        bbox.Merge( parser.ToKicadCoords( ascFile.BoundingBox.GetEnd() ) );

        // Set smallest page size to fit all elements
        for( const PAGE_SIZE_TYPE& sizeType : { PAGE_SIZE_TYPE::A5, PAGE_SIZE_TYPE::A4, PAGE_SIZE_TYPE::A3,
                                                PAGE_SIZE_TYPE::A2, PAGE_SIZE_TYPE::A1, PAGE_SIZE_TYPE::A0 } )
        {
            PAGE_INFO pageInfo( sizeType, false );

            screen->SetPageSettings( pageInfo );
            pageSize = KiROUND( pageInfo.GetSizeIU( schIUScale.IU_PER_MILS ) );

            if( bbox.GetWidth() + margin < pageSize.x && bbox.GetHeight() + margin < pageSize.y )
                break;
        }

        VECTOR2I originOffset = ( ( pageSize / 2 ) - bbox.GetCenter() ) / grid * grid;

        try
        {
            parser.CreateItems( ascFile, screen, uniqueAscNameKeys, mapOfScreens, originOffset );
        }
        catch( IO_ERROR& e )
        {
            aReporter->Report( e.What(), RPT_SEVERITY_ERROR );
        }
    }

    // Fix label casing
    std::set<SCH_SCREEN*> visitedScreens;

    std::function<void( SCH_SCREEN*, const std::map<wxString, wxString>& )> fixLabelCase;
    fixLabelCase = [&]( SCH_SCREEN* aScreen, const std::map<wxString, wxString>& aLabelMap )
    {
        for( SCH_ITEM* item : aScreen->Items() )
        {
            if( SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( item ) )
            {
                SCH_SCREEN* sheetScreen = sheet->GetScreen();

                if( visitedScreens.contains( sheetScreen ) )
                    continue;

                visitedScreens.insert( sheetScreen );

                std::map<wxString, wxString> childLabelMap;

                for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                    childLabelMap[pin->GetText().Lower()] = pin->GetText();

                fixLabelCase( sheetScreen, childLabelMap );
            }
            else if( SCH_LABEL_BASE* lbl = dynamic_cast<SCH_LABEL_BASE*>( item ) )
            {
                if( aLabelMap.contains( lbl->GetText().Lower() ) )
                    lbl->SetText( aLabelMap.at( lbl->GetText().Lower() ) );
            }
        }
    };

    fixLabelCase( aRootSheet->GetScreen(), {} );

    SCH_SHEET_LIST sheets = aSchematic->BuildUnorderedSheetList();

    // Number sheets in LTspice InstName order (parents before children, X2 before X10).
    std::sort( sheets.begin(), sheets.end(),
               []( const SCH_SHEET_PATH& a, const SCH_SHEET_PATH& b )
               {
                   size_t len = std::min( a.size(), b.size() );

                   for( size_t i = 0; i < len; ++i )
                   {
                       SCH_SHEET* sheetA = a.at( i );
                       SCH_SHEET* sheetB = b.at( i );

                       if( sheetA->m_Uuid == sheetB->m_Uuid )
                           continue;

                       const SCH_FIELD* fieldA = sheetA->GetField( SCH_IO_LTSPICE_PARSER::LT_INSTNAME_FIELD );
                       const SCH_FIELD* fieldB = sheetB->GetField( SCH_IO_LTSPICE_PARSER::LT_INSTNAME_FIELD );
                       int              cmp = StrNumCmp( fieldA ? fieldA->GetText() : wxString(),
                                                         fieldB ? fieldB->GetText() : wxString(), true );

                       if( cmp != 0 )
                           return cmp < 0;

                       return sheetA->m_Uuid < sheetB->m_Uuid;
                   }

                   return a.size() < b.size();
               } );

    sheets.SetInitialPageNumbers();

    sheets.SortByHierarchicalPageNumbers( true );
    sheets.CheckForMissingSymbolInstances( aSchematic->Project().GetProjectName() );

    // Update references of all symbols in the schematic to include instance path suffixes ("R1_X4_X1")
    for( SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() <= 1 )
            continue;

        wxString suffix;

        for( size_t i = 1; i < path.size(); ++i )
        {
            suffix += wxS( "_" );

            if( SCH_FIELD* instNameField = path.at( i )->GetField( SCH_IO_LTSPICE_PARSER::LT_INSTNAME_FIELD ) )
                suffix += instNameField->GetText();
        }

        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path );

            symbol->SetRef( &path, ref + suffix );
        }
    }

    for( const SCH_SHEET_PATH& path : sheets )
        path.UpdateAllScreenReferences();

    // Fixing all junctions at the end
    aSchematic->FixupJunctionsAfterImport();
}


void LTSPICE_SCHEMATIC::GetAscAndAsyFilePaths( const wxDir& aDir, bool aRecursive,
                                               std::map<wxString, wxString>& aMapOfAscFiles,
                                               std::map<wxString, wxString>& aMapOfAsyFiles,
                                               const std::vector<wxString>&  aBaseDirs )
{
    wxString filename;

    if( !aDir.IsOpened() )
        return;

    bool cont = aDir.GetFirst( &filename, wxEmptyString, wxDIR_FILES | wxDIR_HIDDEN );

    while( cont )
    {
        wxFileName path( aDir.GetName(), filename );

        auto logToMap =
                [&]( std::map<wxString, wxString>& aMapToLogTo, const wxString& aKey )
                {
                    if( aMapToLogTo.count( aKey ) )
                    {
                        if( m_reporter )
                        {
                            m_reporter->Report( wxString::Format( _( "File at '%s' was ignored. Using previously "
                                                                     "found file at '%s' instead." ),
                                                                  path.GetFullPath(),
                                                                  aMapToLogTo.at( aKey ) ), RPT_SEVERITY_WARNING );
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

    if( aRecursive )
    {
        cont = aDir.GetFirst( &filename, wxEmptyString, wxDIR_DIRS | wxDIR_HIDDEN );

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


std::map<wxString, wxString> LTSPICE_SCHEMATIC::ReadAsyFiles( const std::vector<LTSPICE_FILE>&    aSourceFiles,
                                                              const std::map<wxString, wxString>& aAsyFileMap )
{
    std::map<wxString, wxString> resultantMap;

    for( const LTSPICE_FILE& source : aSourceFiles )
    {
        wxString fileName = source.ElementName;
        wxString mapKey = getShortestKey( aAsyFileMap, fileName );

        if( !mapKey.IsEmpty() )
            resultantMap[fileName] = SafeReadFile( aAsyFileMap.at( mapKey ), wxS( "r" ) );
    }

    return resultantMap;
}


std::vector<LTSPICE_FILE> LTSPICE_SCHEMATIC::GetSchematicElements( const wxString& aAscFile )
{
    std::vector<LTSPICE_FILE> resultantArray;
    wxArrayString             lines = wxSplit( aAscFile, '\n' );

    for( int i = 0; i < (int) lines.GetCount(); i++ )
    {
        wxArrayString tokens = wxSplit( lines[i], ' ' );

        if( tokens.size() >= 4 && tokens[0].Upper() == wxS( "SYMBOL" ) )
        {
            wxString elementName( tokens[1] );
            long     posX, posY;

            tokens[2].ToLong( &posX );
            tokens[3].ToLong( &posY );

            elementName.Replace( '\\', '/' );

            LTSPICE_FILE asyFile( elementName, VECTOR2I( (int) posX, (int) posY ), i + 1 );

            resultantArray.push_back( asyFile );
        }
    }

    return resultantArray;
}


bool LTSPICE_SCHEMATIC::IsAsySubsheet( const wxString& aAsyFile )
{
    wxStringTokenizer lines( aAsyFile, "\r\n", wxTOKEN_STRTOK );

    while( lines.HasMoreTokens() )
    {
        if( lines.GetNextToken().IsSameAs( wxS( "SymbolType BLOCK" ), false ) )
            return true;
    }

    return false;
}


int LTSPICE_SCHEMATIC::integerCheck( const wxString& aToken, int aLineNumber, const wxString& aFileName )
{
    long result;

    if( !aToken.ToLong( &result ) )
        THROW_IO_ERRORF( _( "Expecting integer at line %d in file %s" ), aLineNumber, aFileName );

    return (int) result;
}


VECTOR2I LTSPICE_SCHEMATIC::pointCheck( const wxString& aTokenX, const wxString& aTokenY, int aLineNumber,
                                        const wxString& aFileName )
{
    return VECTOR2I( integerCheck( aTokenX, aLineNumber, aFileName ),
                     integerCheck( aTokenY, aLineNumber, aFileName ) );
}


void LTSPICE_SCHEMATIC::tokensSizeRangeCheck( size_t aActualSize, int aExpectedMin, int aExpectedMax,
                                              int aLineNumber, const wxString& aFileName )
{
    if( (int) aActualSize < aExpectedMin )
        THROW_IO_ERRORF( _( "Expected data missing on line %d in file %s" ), aLineNumber, aFileName );
    else if( (int) aActualSize > aExpectedMax )
        THROW_IO_ERRORF( _( "Extra data found on line %d in file %s" ), aLineNumber, aFileName );
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


LTSPICE_SCHEMATIC::LT_SYMBOL LTSPICE_SCHEMATIC::ParseSymbol( const wxString& aSymbolName,
                                                             const wxString& aAsyFileContent, LT_ASC& aAscFile )
{
    LT_SYMBOL lt_symbol;
    int       lineNumber = 1;

    lt_symbol.Name = aSymbolName;
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
            tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, aSymbolName );

            wxString lineWidth = tokens[1];
            wxString startPointX = tokens[2];
            wxString startPointY = tokens[3];
            wxString endPointX = tokens[4];
            wxString endPointY = tokens[5];

            LINE lt_line;
            lt_line.LineWidth = getLineWidth( lineWidth );
            lt_line.Start = pointCheck( startPointX, startPointY, lineNumber, aSymbolName );
            lt_line.End = pointCheck( endPointX, endPointY, lineNumber, aSymbolName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 7 )
            {
                wxString lineStyle = tokens[6];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aSymbolName );
            }

            lt_line.LineStyle = getLineStyle( lineStyleNumber );

            lt_symbol.Lines.push_back( lt_line );
        }
        else if( element == "RECTANGLE" )
        {
            tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, aSymbolName );

            wxString lineWidth = tokens[1];
            wxString botRightX = tokens[2];
            wxString botRightY = tokens[3];
            wxString topLeftX = tokens[4];
            wxString topRightY = tokens[5];

            RECTANGLE rect;
            rect.LineWidth = getLineWidth( lineWidth );
            rect.BotRight = pointCheck( botRightX, botRightY, lineNumber, aSymbolName );
            rect.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, aSymbolName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 7 )
            {
                wxString lineStyle = tokens[6];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aSymbolName );
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
            tokensSizeRangeCheck( tokens.size(), 6, 7, lineNumber, aSymbolName );

            wxString lineWidth = tokens[1];
            wxString botRightX = tokens[2];
            wxString botRightY = tokens[3];
            wxString topLeftX = tokens[4];
            wxString topRightY = tokens[5];

            CIRCLE circle;
            circle.LineWidth = getLineWidth( lineWidth );
            circle.BotRight = pointCheck( botRightX, botRightY, lineNumber, aSymbolName );
            circle.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, aSymbolName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 7 )
            {
                wxString lineStyle = tokens[6];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aSymbolName );
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
            tokensSizeRangeCheck( tokens.size(), 10, 11, lineNumber, aSymbolName );

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
            arc.BotRight = pointCheck( botRightX, botRightY, lineNumber, aSymbolName );
            arc.TopLeft = pointCheck( topLeftX, topRightY, lineNumber, aSymbolName );
            arc.ArcStart = pointCheck( arcStartPointX, arcStartPointY, lineNumber, aSymbolName );
            arc.ArcEnd = pointCheck( arcEndPointX, arcEndPointY, lineNumber, aSymbolName );

            int lineStyleNumber = 0;  // default

            if( tokens.size() == 11 )
            {
                wxString lineStyle = tokens[10];
                lineStyleNumber = integerCheck( lineStyle, lineNumber, aSymbolName );
            }

            arc.LineStyle = getLineStyle( lineStyleNumber );

            lt_symbol.Arcs.push_back( arc );
        }
        else if( element == "WINDOW" )
        {
            tokensSizeRangeCheck( tokens.size(), 6, 6, lineNumber, aSymbolName );

            wxString number = tokens[1];
            wxString windowPosX = tokens[2];
            wxString windowPosY = tokens[3];
            wxString justification = tokens[4];
            wxString fontSize = tokens[5];

            LT_WINDOW window;
            window.WindowNumber = integerCheck( number, lineNumber, aSymbolName );
            window.Position = pointCheck( windowPosX, windowPosY, lineNumber, aSymbolName );
            window.Justification = getTextJustification( justification );
            window.FontSize = integerCheck( fontSize, lineNumber, aSymbolName );

            // LTSpice appears to ignore hidden property from .asy files
            if( window.FontSize == 0 )
                window.FontSize = 2;

            lt_symbol.Windows.push_back( window );
        }
        else if( element == "SYMATTR" )
        {
            aggregateAttributeValue( tokens, 2 );

            tokensSizeRangeCheck( tokens.size(), 3, INT_MAX, lineNumber, aSymbolName );

            wxString key = tokens[1];
            wxString value = tokens[2];

            if( value == wxS( "\"\"" ) )
                value = wxEmptyString;

            lt_symbol.SymAttributes[ key.Upper() ] = value;
        }
        else if( element == "PIN" )
        {
            tokensSizeRangeCheck( tokens.size(), 5, 5, lineNumber, aSymbolName );

            wxString pinLocationX = tokens[1];
            wxString pinLocationY = tokens[2];
            wxString Justification = tokens[3];
            wxString nameOffSet = tokens[4];

            LT_PIN pin;
            pin.PinLocation = pointCheck( pinLocationX, pinLocationY, lineNumber,aSymbolName );
            pin.PinJustification = getPinJustification( Justification );
            pin.NameOffSet = integerCheck( nameOffSet, lineNumber, aSymbolName );

            lt_symbol.Pins.push_back( pin );
        }
        else if( element == "PINATTR" )
        {
            aggregateAttributeValue( tokens, 2 );

            tokensSizeRangeCheck( tokens.size(), 3, INT_MAX, lineNumber, aSymbolName );

            wxString name = tokens[1];
            wxString Value = tokens[2];

            lt_symbol.Pins.back().PinAttribute.insert( { name, Value } );
        }
        else if( element == "SYMBOLTYPE" )
        {
            tokensSizeRangeCheck( tokens.size(), 2, 2, lineNumber, aSymbolName );

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
    lt_symbol.AsyKey = aAscFileName.Lower();
    lt_symbol.SymbolType = LTSPICE_SCHEMATIC::SYMBOLTYPE::CELL;
    lt_symbol.SymbolOrientation = LTSPICE_SCHEMATIC::ORIENTATION::R0;

    RECTANGLE rect;
    rect.LineWidth = LINEWIDTH::Wide;
    rect.LineStyle = LINESTYLE::DASH;
    rect.BotRight = VECTOR2I( 50, 50 );
    rect.TopLeft = VECTOR2I( -50, -50 );
    lt_symbol.Rectangles.push_back( rect );

    return lt_symbol;
}


std::map<wxString, LTSPICE_SCHEMATIC::LT_ASC>
LTSPICE_SCHEMATIC::ParseAscFiles( const std::map<wxString, wxString>& aAscFilesContent )
{
    // Initialising Schematic Struct

    std::map<wxString, LT_ASC> ascFiles;

    for( const auto& [ fileName, contents ] : aAscFilesContent )
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
                    wxString key = getShortestKey( m_asyNamesMap, symbolName );

                    if( !m_asyNamesMap.contains( key ) )
                    {
                        wxString fullName = wxFileName( fileName ).GetFullName();

                        THROW_IO_ERROR(
                                wxString::Format( _( "Missing symbol file '%s.asy', referenced from '%s' at line %d" ),
                                                  symbolName, fullName, lineNumber ) );
                    }

                    if( !m_asyFileCache.contains( m_asyNamesMap.at( key ) ) )
                        THROW_IO_ERROR( wxString::Format( _( "Unable to read '%s'" ), m_asyNamesMap.at( key ) ) );

                    LT_SYMBOL lt_symbol =
                            ParseSymbol( symbolName, m_asyFileCache.at( m_asyNamesMap.at( key ) ), ascFile );

                    lt_symbol.AsyKey = key;
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

                LT_SYMBOL* symbol = &ascFile.Symbols.back();
                LT_WINDOW* window = nullptr;
                int        windowNumber = integerCheck( number, lineNumber, fileName );

                // Overwrite an existing window from the symbol definition
                for( LT_WINDOW& candidate : symbol->Windows )
                {
                    if( candidate.WindowNumber == windowNumber )
                    {
                        window = &candidate;
                        break;
                    }
                }

                if( !window )
                {
                    symbol->Windows.emplace_back( LT_WINDOW() );
                    window = &symbol->Windows.back();
                }

                window->WindowNumber = windowNumber;
                window->Position = pointCheck( windowPosX, windowPosY, lineNumber, fileName );
                window->Justification = getTextJustification( justification );
                window->FontSize = integerCheck( fontSize, lineNumber, fileName );

                ascFile.BoundingBox.Merge( symbol->Offset + window->Position );
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

        ascFiles[fileName] = std::move( ascFile );
    }

    return ascFiles;
}


