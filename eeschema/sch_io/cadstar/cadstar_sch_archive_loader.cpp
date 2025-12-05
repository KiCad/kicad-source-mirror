/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
 * @file cadstar_sch_archive_loader.cpp
 * @brief Loads a csa file into a KiCad SCHEMATIC object
 */

#include <sch_io/cadstar/cadstar_sch_archive_loader.h>
#include <io/cadstar/cadstar_parts_lib_parser.h>

#include <bus_alias.h>
#include <core/mirror.h>
#include <core/kicad_algo.h>
#include <eda_text.h>
#include <macros.h>
#include <progress_reporter.h>
#include <string_utils.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h> //SYMBOL_ORIENTATION_T
#include <sch_io/sch_io_mgr.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_label.h>
#include <schematic.h>
#include <sim/sim_model.h>
#include <trigo.h>
#include <wildcards_and_files_ext.h>


const wxString PartNameFieldName = "Part Name";
const wxString PartNumberFieldName = "Part Number";
const wxString PartVersionFieldName = "Part Version";
const wxString PartAcceptanceFieldName = "Part Acceptance";


wxString CADSTAR_SCH_ARCHIVE_LOADER::CreateLibName( const wxFileName& aFileName,
                                                    const SCH_SHEET* aRootSheet )
{
    wxString libName = aFileName.GetName();

    if( libName.IsEmpty() && aRootSheet )
    {
        wxFileName fn( aRootSheet->GetFileName() );
        libName = fn.GetName();
    }

    if( libName.IsEmpty() )
        libName = "noname";

    libName = LIB_ID::FixIllegalChars( libName, true ).wx_str();

    return libName;
}


std::vector<LIB_SYMBOL*> CADSTAR_SCH_ARCHIVE_LOADER::LoadPartsLib( const wxString& aFilename )
{
    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 3 ); // (0) Read csa, (1) Parse csa, (3) Load lib

    Parse();

    CADSTAR_PARTS_LIB_PARSER p;

    if( !p.CheckFileHeader( aFilename.utf8_string() ) )
        THROW_IO_ERROR( _( "File does not appear to be a CADSTAR parts Library file" ) );

    // TODO: we could add progress reporting for reading .lib
    CADSTAR_PARTS_LIB_MODEL csLib = p.ReadFile( aFilename.utf8_string() );

    if( m_progressReporter )
    {
        m_progressReporter->BeginPhase( 2 );
        long numSteps = csLib.m_PartEntries.size();
        m_progressReporter->SetMaxProgress( numSteps );
    }

    std::vector<LIB_SYMBOL*> retVal;

    for( const CADSTAR_PART_ENTRY& part : csLib.m_PartEntries )
    {
        std::unique_ptr<LIB_SYMBOL> loadedPart = loadLibPart( part );

        checkPoint();

        if( loadedPart )
            retVal.push_back( loadedPart.release() );
    }

    return retVal;
}


std::unique_ptr<LIB_SYMBOL>
CADSTAR_SCH_ARCHIVE_LOADER::loadLibPart( const CADSTAR_PART_ENTRY& aPart )
{
    wxString                    escapedPartName = EscapeString( aPart.m_Name, CTX_LIBID );
    std::unique_ptr<LIB_SYMBOL> retSym;

    int unit = 0;

    for( const CADSTAR_PART_SYMBOL_ENTRY& sym : aPart.m_Symbols )
    {
        ++unit;
        wxString  alternateName = sym.m_SymbolAlternateName.value_or( "" );
        SYMDEF_ID symbolID = getSymDefFromName( sym.m_SymbolName, alternateName );

        if( !Library.SymbolDefinitions.count( symbolID ) )
        {
            m_reporter->Report( wxString::Format( _( "Unable to find symbol %s, referenced by part "
                                                     "%s. The part was not loaded." ),
                                                  generateLibName( sym.m_SymbolName, alternateName ),
                                                  aPart.m_Name ),
                                RPT_SEVERITY_ERROR );

            return nullptr;
        }

        // Load the graphical symbol for this gate
        std::unique_ptr<LIB_SYMBOL> kiSymDef( loadSymdef( symbolID )->Duplicate() );

        if( (int)sym.m_Pins.size() != kiSymDef->GetPinCount() )
        {
            m_reporter->Report( wxString::Format( _( "Inconsistent pin numbers in symbol %s "
                                                     "compared to the one defined in part %s. The "
                                                     "part was not loaded." ),
                                                  generateLibName( sym.m_SymbolName, alternateName ),
                                                  aPart.m_Name ),
                                RPT_SEVERITY_ERROR );

            return nullptr;
        }

        wxASSERT( m_symDefTerminalsMap.count( symbolID ) ); //loadSymDef should have populated this


        // Update the pin numbers to match those defined in the Cadstar part
        for( auto& [storedPinNum, termID] : m_symDefTerminalsMap[symbolID] )
        {
            wxCHECK( termID > 0 && sym.m_Pins.size() >= size_t( termID ), nullptr );
            SCH_PIN* pin = kiSymDef->GetPin( storedPinNum );
            size_t   termIdx = size_t( termID ) - 1;

            // For now leave numerical pin number. Otherwise, when loading the
            // .cpa file we won't be able to link up to the footprint pads, but if
            // we solve this, we could then load alphanumeric pin numbers as below:
            //
            // if( aPart.m_PinNamesMap.count( termID ) )
            //  partPinNum = wxString( aPart.m_PinNamesMap.at( termID ) );
            //
            wxString partPinNum = wxString::Format( "%ld", sym.m_Pins[termIdx].m_Identifier );
            pin->SetNumber( partPinNum );

            if( aPart.m_PinNamesMap.count( termID ) )
                pin->SetName( HandleTextOverbar( aPart.m_PinNamesMap.at( termID ) ) );
            else if( aPart.m_PinLabelsMap.count( termID ) )
                pin->SetName( HandleTextOverbar( aPart.m_PinLabelsMap.at( termID ) ) );

            pin->SetType( getKiCadPinType( sym.m_Pins[termIdx].m_Type ) );

            // @todo: Load pin/gate swapping information once kicad supports this
        }

        if( unit == 1 )
        {
            wxCHECK( kiSymDef->GetUnitCount() == 1, nullptr );
            // The first unit can just be moved to the part symbol
            retSym = std::move( kiSymDef );

            retSym->SetUnitCount( aPart.m_Symbols.size(), true );

            retSym->SetName( escapedPartName );
            retSym->GetReferenceField().SetText( aPart.m_ComponentStem );
            retSym->GetValueField().SetText( aPart.m_Value.value_or( "" ) );
            addNewFieldToSymbol( PartNameFieldName, retSym )->SetText( aPart.m_Name );
            retSym->SetDescription( aPart.m_Description.value_or( "" ) );

            auto addFieldIfHasValue =
                    [&]( const wxString& name, const std::optional<std::string>& value )
                    {
                        if( value.has_value() )
                            addNewFieldToSymbol( name, retSym )->SetText( value.value() );
                    };

            addFieldIfHasValue( PartNumberFieldName,        aPart.m_Number );
            addFieldIfHasValue( PartVersionFieldName,       aPart.m_Version );
            addFieldIfHasValue( PartAcceptanceFieldName,    aPart.m_AcceptancePartName );

            setFootprintOnSymbol( retSym, aPart.m_Pcb_component,
                                  aPart.m_Pcb_alternate.value_or( "" ) );

            if( aPart.m_SpiceModel.has_value() )
            {
                wxString modelVal = wxString::Format( "model=\"%s\"", aPart.m_SpiceModel.value() );
                addNewFieldToSymbol( SIM_DEVICE_FIELD, retSym )->SetText( "SPICE" );
                addNewFieldToSymbol( SIM_PARAMS_FIELD, retSym )->SetText( modelVal );
            }

            // Load all part attributes, regardless of original cadstar type, to the symbol

            // @todo some cadstar part attributes have a "read-only" flag. We should load this
            // when KiCad supports read-only fields.

            for( auto& [fieldName, value] : aPart.m_UserAttributes )
                addNewFieldToSymbol( fieldName, retSym )->SetText( value );

            for( auto& [fieldName, attrValue] : aPart.m_SchAttributes )
                addNewFieldToSymbol( fieldName, retSym )->SetText( attrValue.m_Value );

            for( auto& [fieldName, attrValue] : aPart.m_PcbAttributes )
                addNewFieldToSymbol( fieldName, retSym )->SetText( attrValue.m_Value );

            for( auto& [fieldName, attrValue] : aPart.m_SchAndPcbAttributes )
                addNewFieldToSymbol( fieldName, retSym )->SetText( attrValue.m_Value );

            for( auto& [fieldName, attrValue] : aPart.m_PartAttributes )
                addNewFieldToSymbol( fieldName, retSym )->SetText( attrValue.m_Value );

            // Load all hidden pins onto the first unit of the symbol in KiCad
            // We load them in a spiral sequence, starting at the center of the symbol BBOX
            VECTOR2I symCenter = retSym->GetBodyBoundingBox( unit, 0, false, false ).GetCenter();
            symCenter.y = -symCenter.y; // need to invert the y coord for lib symbols.

            VECTOR2I delta( 0, 1 );
            VECTOR2I direction( 0, -1 );
            int spacing = schIUScale.MilsToIU( 50 ); // for now, place on a 50mil grid

            for( auto& [signalName, csPinVector] : aPart.m_HiddenPins )
            {
                for(  const CADSTAR_PART_PIN& csPin : csPinVector )
                {
                    std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( retSym.get() );

                    long pinNum = csPin.m_Identifier;
                    pin->SetNumber( wxString::Format( "%ld", pinNum ) );
                    pin->SetName( signalName );
                    pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );

                    pin->SetVisible( false );

                    // Generate the coordinate for the pin. We don't want overlapping pins
                    // and ideally close to the center of the symbol, so we load pins in a
                    // spiral sequence around the center
                    if( delta.x == delta.y
                        || ( delta.x < 0 && delta.x == -delta.y )
                        || ( delta.x > 0 && delta.x == 1 - delta.y ) )
                    {
                        // change direction
                        direction = { -direction.y, direction.x };
                    }

                    delta += direction;
                    VECTOR2I offset = delta * spacing;
                    pin->SetPosition( symCenter + offset );
                    pin->SetLength( 0 ); //CADSTAR Pins are just a point (have no length)
                    pin->SetShape( GRAPHIC_PINSHAPE::LINE );
                    pin->SetUnit( unit );
                    retSym->AddDrawItem( pin.release() );
                }
            }
        }
        else
        {                 // Source:   Dest:
            copySymbolItems( kiSymDef, retSym, unit, false /* aOverrideFields */ );
        }


        retSym->SetShowPinNames( aPart.m_PinsVisible );
        retSym->SetShowPinNumbers( aPart.m_PinsVisible );
    }


    return retSym;
}


void CADSTAR_SCH_ARCHIVE_LOADER::copySymbolItems( std::unique_ptr<LIB_SYMBOL>& aSourceSym,
                                                  std::unique_ptr<LIB_SYMBOL>& aDestSym,
                                                  int aDestUnit, bool aOverrideFields )
{
    // Ensure there are no items on the unit we want to load onto
    for( SCH_ITEM* item : aDestSym->GetUnitDrawItems( aDestUnit, 0 /* aBodyStyle */ ) )
        aDestSym->RemoveDrawItem( item );

    // Copy all draw items
    for( SCH_ITEM* newItem : aSourceSym->GetUnitDrawItems( 1, 0 /* aBodyStyle */ ) )
    {
        SCH_ITEM* itemCopy = static_cast<SCH_ITEM*>( newItem->Clone() );
        itemCopy->SetParent( aDestSym.get() );
        itemCopy->SetUnit( aDestUnit );
        aDestSym->AddDrawItem( itemCopy );
    }

    //Copy / override all fields
    if( aOverrideFields )
    {
        std::vector<SCH_FIELD*> fieldsToCopy;
        aSourceSym->GetFields( fieldsToCopy );

        for( SCH_FIELD* templateField : fieldsToCopy )
        {
            SCH_FIELD* appliedField = addNewFieldToSymbol( templateField->GetName(), aDestSym );
            templateField->Copy( appliedField );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet )
{
    wxCHECK( aSchematic, /* void */ );

    if( m_progressReporter )
        m_progressReporter->SetNumPhases( 3 ); // (0) Read file, (1) Parse file, (2) Load file

    Parse();

    checkDesignLimits(); // Throws if error found

    // Assume the center at 0,0 since we are going to be translating the design afterwards anyway
    m_designCenter = { 0, 0 };

    m_schematic = aSchematic;
    m_rootSheet = aRootSheet;

    if( m_progressReporter )
    {
        m_progressReporter->BeginPhase( 2 );
        long numSteps = 11; // one step for each of below functions + one at the end of import

        // Step 4 is by far the longest - add granularity in reporting
        numSteps += Parts.PartDefinitions.size();

        m_progressReporter->SetMaxProgress( numSteps );
    }

    loadTextVariables(); // Load text variables right at the start to ensure bounding box
                         // calculations work correctly for text items
    checkPoint(); // Step 1
    loadSheets();
    checkPoint(); // Step 2
    loadHierarchicalSheetPins();
    checkPoint(); // Step 3
    loadPartsLibrary();
    checkPoint(); // Step 4, Subdivided into extra steps
    loadSchematicSymbolInstances();
    checkPoint(); // Step 5
    loadBusses();
    checkPoint(); // Step 6
    loadNets();
    checkPoint(); // Step 7
    loadFigures();
    checkPoint(); // Step 8
    loadTexts();
    checkPoint(); // Step 9
    loadDocumentationSymbols();
    checkPoint(); // Step 10

    if( Schematic.VariantHierarchy.Variants.size() > 0 )
    {
        m_reporter->Report( wxString::Format( _( "The CADSTAR design contains variants which has "
                                                 "no KiCad equivalent. Only the master variant "
                                                 "('%s') was loaded." ),
                                              Schematic.VariantHierarchy.Variants.at( "V0" ).Name ),
                            RPT_SEVERITY_WARNING );
    }

    if( Schematic.Groups.size() > 0 )
    {
        m_reporter->Report( _( "The CADSTAR design contains grouped items which has no KiCad "
                               "equivalent. Any grouped items have been ungrouped." ),
                            RPT_SEVERITY_WARNING );
    }

    if( Schematic.ReuseBlocks.size() > 0 )
    {
        m_reporter->Report( _( "The CADSTAR design contains re-use blocks which has no KiCad "
                               "equivalent. The re-use block information has been discarded during "
                               "the import." ),
                            RPT_SEVERITY_WARNING );
    }


    // For all sheets, center all elements and re calculate the page size:
    for( std::pair<LAYER_ID, SCH_SHEET*> sheetPair : m_sheetMap )
    {
        SCH_SHEET* sheet = sheetPair.second;

        // Calculate the new sheet size.
        BOX2I sheetBoundingBox;

        for( SCH_ITEM* item : sheet->GetScreen()->Items() )
        {
            BOX2I bbox;

            // Only use the visible fields of the symbols to calculate their bounding box
            // (hidden fields could be very long and artificially enlarge the sheet bounding box)
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* comp = static_cast<SCH_SYMBOL*>( item );
                bbox = comp->GetBodyAndPinsBoundingBox();

                for( const SCH_FIELD& field : comp->GetFields() )
                {
                    if( field.IsVisible() )
                        bbox.Merge( field.GetBoundingBox() );
                }
            }
            else if( item->Type() == SCH_TEXT_T )
            {
                SCH_TEXT* txtItem = static_cast<SCH_TEXT*>( item );
                wxString  txt = txtItem->GetText();

                if( txt.Contains( "${" ) )
                    continue; // We can't calculate bounding box of text items with variables
                else
                    bbox = txtItem->GetBoundingBox();
            }
            else
            {
                bbox = item->GetBoundingBox();
            }

            sheetBoundingBox.Merge( bbox );
        }

        // Find the screen grid of the original CADSTAR design
        int grid = Assignments.Grids.ScreenGrid.Param1;

        if( Assignments.Grids.ScreenGrid.Type == GRID_TYPE::FRACTIONALGRID )
            grid = grid / Assignments.Grids.ScreenGrid.Param2;
        else if( Assignments.Grids.ScreenGrid.Param2 > grid )
            grid = Assignments.Grids.ScreenGrid.Param2;

        grid = getKiCadLength( grid );

        auto roundToNearestGrid =
                [&]( int aNumber ) -> int
                {
                    int error = aNumber % grid;
                    int absError = sign( error ) * error;

                    if( absError > ( grid / 2 ) )
                        return aNumber + ( sign( error ) * grid ) - error;
                    else
                        return aNumber - error;
                };

        // When exporting to pdf, CADSTAR applies a margin of 3percent of the longest dimension (height
        // or width) to all 4 sides (top, bottom, left right). For the import, we are also rounding
        // the margin to the nearest grid, ensuring all items remain on the grid.
        VECTOR2I targetSheetSize = sheetBoundingBox.GetSize();
        int      longestSide = std::max( targetSheetSize.x, targetSheetSize.y );
        int      margin = ( (double) longestSide * 0.03 );
        margin = roundToNearestGrid( margin );
        targetSheetSize += margin * 2;

        // Update page size always
        PAGE_INFO pageInfo = sheet->GetScreen()->GetPageSettings();
        pageInfo.SetWidthMils( schIUScale.IUToMils( targetSheetSize.x ) );
        pageInfo.SetHeightMils( schIUScale.IUToMils( targetSheetSize.y ) );

        // Set the new sheet size.
        sheet->GetScreen()->SetPageSettings( pageInfo );

        VECTOR2I pageSizeIU = sheet->GetScreen()->GetPageSettings().GetSizeIU( schIUScale.IU_PER_MILS );
        VECTOR2I sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
        VECTOR2I itemsCentre = sheetBoundingBox.Centre();

        // round the translation to nearest point on the grid
        VECTOR2I translation = sheetcentre - itemsCentre;
        translation.x = roundToNearestGrid( translation.x );
        translation.y = roundToNearestGrid( translation.y );

        // Translate the items.
        std::vector<SCH_ITEM*> allItems;

        std::copy( sheet->GetScreen()->Items().begin(), sheet->GetScreen()->Items().end(),
                   std::back_inserter( allItems ) );

        for( SCH_ITEM* item : allItems )
        {
            item->Move( translation );
            item->ClearFlags();
            sheet->GetScreen()->Update( item );
        }
    }

    checkPoint();

    m_reporter->Report( _( "CADSTAR fonts are different to the ones in KiCad. This will likely "
                           "result in alignment issues. Please review the imported text elements "
                           "carefully and correct manually if required." ),
                        RPT_SEVERITY_WARNING );

    m_reporter->Report( _( "The CADSTAR design has been imported successfully.\n"
                           "Please review the import errors and warnings (if any)." ) );
}


void CADSTAR_SCH_ARCHIVE_LOADER::checkDesignLimits()
{
    LONGPOINT designLimit = Assignments.Settings.DesignLimit;

    //Note: can't use getKiCadPoint() due VECTOR2I being int - need long long to make the check
    long long designSizeXkicad = (long long) designLimit.x / KiCadUnitDivider;
    long long designSizeYkicad = (long long) designLimit.y / KiCadUnitDivider;

    // Max size limited by the positive dimension of VECTOR2I (which is an int)
    constexpr long long maxDesignSizekicad = std::numeric_limits<int>::max();

    if( designSizeXkicad > maxDesignSizekicad || designSizeYkicad > maxDesignSizekicad )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "The design is too large and cannot be imported into KiCad. \n"
                   "Please reduce the maximum design size in CADSTAR by navigating to: \n"
                   "Design Tab -> Properties -> Design Options -> Maximum Design Size. \n"
                   "Current Design size: %.2f, %.2f millimeters. \n" //format:allow
                   "Maximum permitted design size: %.2f, %.2f millimeters.\n" ), //format:allow
                (double) designSizeXkicad / SCH_IU_PER_MM,
                (double) designSizeYkicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM ) );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheets()
{
    const std::vector<LAYER_ID>& orphanSheets = findOrphanSheets();
    SCH_SHEET_PATH               rootPath;
    rootPath.push_back( m_rootSheet );
    rootPath.SetPageNumber( wxT( "1" ) );

    if( orphanSheets.size() > 1 )
    {
        int x = 1;
        int y = 1;

        for( LAYER_ID sheetID : orphanSheets )
        {
            VECTOR2I pos( x * schIUScale.MilsToIU( 1000 ), y * schIUScale.MilsToIU( 1000 ) );
            VECTOR2I siz( schIUScale.MilsToIU( 1000 ), schIUScale.MilsToIU( 1000 ) );

            loadSheetAndChildSheets( sheetID, pos, siz, rootPath );

            x += 2;

            if( x > 10 ) // start next row
            {
                x = 1;
                y += 2;
            }
        }
    }
    else if( orphanSheets.size() > 0 )
    {
        LAYER_ID rootSheetID = orphanSheets.at( 0 );

        wxFileName loadedFilePath = wxFileName( Filename );

        std::string filename = wxString::Format( "%s_%02d", loadedFilePath.GetName(),
                                                 getSheetNumber( rootSheetID ) )
                                       .ToStdString();
        ReplaceIllegalFileNameChars( filename );
        filename += wxT( "." ) + wxString( FILEEXT::KiCadSchematicFileExtension );

        wxFileName fn( m_schematic->Project().GetProjectPath() + filename );
        m_rootSheet->GetScreen()->SetFileName( fn.GetFullPath() );

        m_sheetMap.insert( { rootSheetID, m_rootSheet } );
        loadChildSheets( rootSheetID, rootPath );
    }
    else if( Header.Format.Type == "SYMBOL" )
    {
        THROW_IO_ERROR( _( "The selected file is a CADSTAR symbol library. It does not contain a "
                           "schematic design so cannot be imported/opened in this way." ) );
    }
    else
    {
        THROW_IO_ERROR( _( "The CADSTAR schematic might be corrupt: there is no root sheet." ) );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadHierarchicalSheetPins()
{
    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK&   block   = blockPair.second;
        LAYER_ID sheetID = "";

        if( block.Type == BLOCK::TYPE::PARENT )
            sheetID = block.LayerID;
        else if( block.Type == BLOCK::TYPE::CHILD )
            sheetID = block.AssocLayerID;
        else
            continue;

        if( m_sheetMap.find( sheetID ) != m_sheetMap.end() )
        {
            SCH_SHEET* sheet = m_sheetMap.at( sheetID );

            for( std::pair<TERMINAL_ID, TERMINAL> termPair : block.Terminals )
            {
                TERMINAL term = termPair.second;
                wxString name = "YOU SHOULDN'T SEE THIS TEXT. THIS IS A BUG.";

                SCH_HIERLABEL* sheetPin = nullptr;

                if( block.Type == BLOCK::TYPE::PARENT )
                    sheetPin = new SCH_HIERLABEL();
                else if( block.Type == BLOCK::TYPE::CHILD )
                    sheetPin = new SCH_SHEET_PIN( sheet );

                sheetPin->SetText( name );
                sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );
                sheetPin->SetSpinStyle( getSpinStyle( term.OrientAngle, false ) );
                sheetPin->SetPosition( getKiCadPoint( term.Position ) );

                if( sheetPin->Type() == SCH_SHEET_PIN_T )
                    sheet->AddPin( (SCH_SHEET_PIN*) sheetPin );
                else
                    sheet->GetScreen()->Append( sheetPin );

                BLOCK_PIN_ID blockPinID = std::make_pair( block.ID, term.ID );
                m_sheetPinMap.insert( { blockPinID, sheetPin } );
            }
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadPartsLibrary()
{
    for( std::pair<PART_ID, PART> partPair : Parts.PartDefinitions )
    {
        PART_ID partID = partPair.first;
        PART    part = partPair.second;

        wxString    escapedPartName = EscapeString( part.Name, CTX_LIBID );
        LIB_SYMBOL* kiSym = new LIB_SYMBOL( escapedPartName );

        kiSym->SetUnitCount( part.Definition.GateSymbols.size(), true );
        bool ok = true;

        for( std::pair<GATE_ID, PART::DEFINITION::GATE> gatePair : part.Definition.GateSymbols )
        {
            GATE_ID                gateID   = gatePair.first;
            PART::DEFINITION::GATE gate     = gatePair.second;
            SYMDEF_ID              symbolID = getSymDefFromName( gate.Name, gate.Alternate );

            if( symbolID.IsEmpty() )
            {
                m_reporter->Report( wxString::Format( _( "Part definition '%s' references symbol "
                                                         "'%s' (alternate '%s') which could not be "
                                                         "found in the symbol library. The part has "
                                                         "not been loaded into the KiCad library." ),
                                                      part.Name,
                                                      gate.Name,
                                                      gate.Alternate ),
                                    RPT_SEVERITY_WARNING);

                ok = false;
                break;
            }

            m_partSymbolsMap.insert( { { partID, gateID }, symbolID } );
            loadSymbolGateAndPartFields( symbolID, part, gateID, kiSym );
        }

        if( ok && part.Definition.GateSymbols.size() != 0 )
        {
            m_loadedSymbols.push_back( kiSym );
        }
        else
        {
            if( part.Definition.GateSymbols.size() == 0 )
            {
                m_reporter->Report( wxString::Format( _( "Part definition '%s' has an incomplete "
                                                         "definition (no symbol definitions are "
                                                         "associated with it). The part has not "
                                                         "been loaded into the KiCad library." ),
                                                      part.Name ),
                                    RPT_SEVERITY_WARNING );
            }

            // Don't save in the library, but still keep it cached as some of the units might have
            // been loaded correctly (saving us time later on), plus the part definition contains
            // the part name, which is important to load
        }

        m_partMap.insert( { partID, kiSym } );

        checkPoint();
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSchematicSymbolInstances()
{
    for( std::pair<SYMBOL_ID, SYMBOL> symPair : Schematic.Symbols )
    {
        SYMBOL sym = symPair.second;

        if( !sym.VariantID.empty() && sym.VariantParentSymbolID != sym.ID )
            continue; // Only load master Variant

        if( sym.IsComponent )
        {
            if( m_partMap.find( sym.PartRef.RefID ) == m_partMap.end() )
            {
                m_reporter->Report( wxString::Format( _( "Symbol '%s' references part '%s' which "
                                                         "could not be found in the library. The "
                                                         "symbol was not loaded" ),
                                                      sym.ComponentRef.Designator,
                                                      sym.PartRef.RefID ),
                                    RPT_SEVERITY_ERROR );

                continue;
            }

            if( sym.GateID.IsEmpty() )
                sym.GateID = wxT( "A" ); // Assume Gate "A" if unspecified

            PART_GATE_ID partSymbolID = { sym.PartRef.RefID, sym.GateID };
            LIB_SYMBOL*  kiSym = m_partMap.at( sym.PartRef.RefID );
            bool         copy = false;

            // The symbol definition in the part either does not exist for this gate number
            // or is different to the symbol instance. We need to reload the gate for this
            // symbol
            if( m_partSymbolsMap.find( partSymbolID ) == m_partSymbolsMap.end()
                || m_partSymbolsMap.at( partSymbolID ) != sym.SymdefID )
            {
                kiSym = new LIB_SYMBOL( *kiSym ); // Make a copy
                copy = true;
                const PART& part = Parts.PartDefinitions.at( sym.PartRef.RefID );
                loadSymbolGateAndPartFields( sym.SymdefID, part, sym.GateID, kiSym );
            }

            LIB_SYMBOL* scaledPart = getScaledLibPart( kiSym, sym.ScaleRatioNumerator,
                                                       sym.ScaleRatioDenominator );

            EDA_ANGLE   symOrient = ANGLE_0;
            SCH_SYMBOL* symbol = loadSchematicSymbol( sym, *scaledPart, symOrient );

            delete scaledPart;

            if( copy )
                delete kiSym;

            SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );

            sym.ComponentRef.Designator.Replace( wxT( "\n" ), wxT( "\\n" ) );
            sym.ComponentRef.Designator.Replace( wxT( "\r" ), wxT( "\\r" ) );
            sym.ComponentRef.Designator.Replace( wxT( "\t" ), wxT( "\\t" ) );
            sym.ComponentRef.Designator.Replace( wxT( " " ), wxT( "_" ) );

            refField->SetText( sym.ComponentRef.Designator );
            loadSymbolFieldAttribute( sym.ComponentRef.AttrLoc, symOrient, sym.Mirror, refField );

            if( sym.HasPartRef )
            {
                SCH_FIELD* partField = symbol->GetField( PartNameFieldName );

                if( !partField )
                    partField = symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, PartNameFieldName ) );

                wxASSERT( partField->GetName() == PartNameFieldName );

                wxString partname = getPart( sym.PartRef.RefID ).Name;
                partname.Replace( wxT( "\n" ), wxT( "\\n" ) );
                partname.Replace( wxT( "\r" ), wxT( "\\r" ) );
                partname.Replace( wxT( "\t" ), wxT( "\\t" ) );
                partField->SetText( partname );

                loadSymbolFieldAttribute( sym.PartRef.AttrLoc, symOrient, sym.Mirror, partField );

                partField->SetVisible( SymbolPartNameColor.IsVisible );
            }

            for( auto& attr : sym.AttributeValues )
            {
                ATTRIBUTE_VALUE attrVal = attr.second;

                if( attrVal.HasLocation )
                {
                    wxString attrName = getAttributeName( attrVal.AttributeID );
                    SCH_FIELD* attrField = symbol->GetField( attrName );

                    if( !attrField )
                        attrField = symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, attrName ) );

                    wxASSERT( attrField->GetName() == attrName );

                    attrVal.Value.Replace( wxT( "\n" ), wxT( "\\n" ) );
                    attrVal.Value.Replace( wxT( "\r" ), wxT( "\\r" ) );
                    attrVal.Value.Replace( wxT( "\t" ), wxT( "\\t" ) );
                    attrField->SetText( attrVal.Value );

                    loadSymbolFieldAttribute( attrVal.AttributeLocation, symOrient, sym.Mirror,
                                              attrField );
                    attrField->SetVisible( isAttributeVisible( attrVal.AttributeID ) );
                }
            }
        }
        else if( sym.IsSymbolVariant )
        {
            if( Library.SymbolDefinitions.find( sym.SymdefID ) == Library.SymbolDefinitions.end() )
            {
                THROW_IO_ERROR( wxString::Format( _( "Symbol ID '%s' references library symbol "
                                                     "'%s' which could not be found in the "
                                                     "library. Did you export all items of the "
                                                     "design?" ),
                                                  sym.ID,
                                                  sym.PartRef.RefID ) );
            }

            SYMDEF_SCM libSymDef = Library.SymbolDefinitions.at( sym.SymdefID );

            if( libSymDef.Terminals.size() != 1 )
            {
                THROW_IO_ERROR( wxString::Format( _( "Symbol ID '%s' is a signal reference or "
                                                     "global signal but it has too many pins. The "
                                                     "expected number of pins is 1 but %d were "
                                                     "found." ),
                                                  sym.ID,
                                                  libSymDef.Terminals.size() ) );
            }

            if( sym.SymbolVariant.Type == SYMBOLVARIANT::TYPE::GLOBALSIGNAL )
            {
                SYMDEF_ID symID  = sym.SymdefID;
                LIB_SYMBOL* kiPart = nullptr;

                // In CADSTAR "GlobalSignal" is a special type of symbol which defines
                // a Power Symbol. The "Alternate" name defines the default net name of
                // the power symbol but this can be overridden in the design itself.
                wxString libraryNetName = Library.SymbolDefinitions.at( symID ).Alternate;

                // Name of the net that the symbol instance in CADSTAR refers to:
                wxString symbolInstanceNetName = sym.SymbolVariant.Reference;
                symbolInstanceNetName = EscapeString( symbolInstanceNetName, CTX_LIBID );

                // Name of the symbol we will use for saving the part in KiCad
                // Note: In CADSTAR all power symbols will start have the reference name be
                // "GLOBALSIGNAL" followed by the default net name, so it makes sense to save
                // the symbol in KiCad as the default net name as well.
                wxString libPartName = libraryNetName;

                // In CADSTAR power symbol instances can refer to a different net to that defined
                // in the library. This causes problems in KiCad v6 as it breaks connectivity when
                // the user decides to update all symbols from library. We handle this by creating
                // individual versions of the power symbol for each net name.
                if( libPartName != symbolInstanceNetName )
                {
                    libPartName += wxT( " (" ) + symbolInstanceNetName + wxT( ")" );
                }

                if( m_powerSymLibMap.find( libPartName ) == m_powerSymLibMap.end() )
                {
                    const LIB_SYMBOL* templatePart = loadSymdef( symID );
                    wxCHECK( templatePart, /*void*/ );

                    kiPart = new LIB_SYMBOL( *templatePart );
                    kiPart->SetGlobalPower();
                    kiPart->SetName( libPartName );
                    kiPart->GetValueField().SetText( symbolInstanceNetName );
                    kiPart->SetShowPinNames( false );
                    kiPart->SetShowPinNumbers( false );

                    std::vector<SCH_PIN*> pins = kiPart->GetPins();
                    wxCHECK( pins.size() == 1, /*void*/ );

                    pins.at( 0 )->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
                    pins.at( 0 )->SetName( symbolInstanceNetName );

                    if( libSymDef.TextLocations.find( SIGNALNAME_ORIGIN_ATTRID )
                        != libSymDef.TextLocations.end() )
                    {
                        TEXT_LOCATION& txtLoc =
                                libSymDef.TextLocations.at( SIGNALNAME_ORIGIN_ATTRID );

                        VECTOR2I valPos = getKiCadLibraryPoint( txtLoc.Position, libSymDef.Origin );

                        kiPart->GetValueField().SetPosition( valPos );
                        kiPart->GetValueField().SetVisible( true );
                    }
                    else
                    {
                        kiPart->GetValueField().SetVisible( false );
                    }

                    kiPart->GetReferenceField().SetText( "#PWR" );
                    kiPart->GetReferenceField().SetVisible( false );
                    m_loadedSymbols.push_back( kiPart );
                    m_powerSymLibMap.insert( { libPartName, kiPart } );
                }
                else
                {
                    kiPart = m_powerSymLibMap.at( libPartName );
                    wxASSERT( kiPart->GetValueField().GetText() == symbolInstanceNetName );
                }

                LIB_SYMBOL* scaledPart = getScaledLibPart( kiPart, sym.ScaleRatioNumerator,
                                                           sym.ScaleRatioDenominator );

                EDA_ANGLE   returnedOrient = ANGLE_0;
                SCH_SYMBOL* symbol = loadSchematicSymbol( sym, *scaledPart, returnedOrient );
                m_powerSymMap.insert( { sym.ID, symbol } );

                delete scaledPart;
            }
            else if( sym.SymbolVariant.Type == SYMBOLVARIANT::TYPE::SIGNALREF )
            {
                // There should only be one pin and we'll use that to set the position
                TERMINAL& symbolTerminal = libSymDef.Terminals.begin()->second;
                VECTOR2I  terminalPosOffset = symbolTerminal.Position - libSymDef.Origin;
                EDA_ANGLE rotate = getAngle( sym.OrientAngle );

                if( sym.Mirror )
                    rotate += ANGLE_180;

                RotatePoint( terminalPosOffset, -rotate );

                SCH_GLOBALLABEL* netLabel = new SCH_GLOBALLABEL;
                netLabel->SetPosition( getKiCadPoint( (VECTOR2I)sym.Origin + terminalPosOffset ) );
                netLabel->SetText( "***UNKNOWN NET****" ); // This should be later updated when we load the netlist
                netLabel->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 50 ) ) );

                SYMDEF_SCM symbolDef = Library.SymbolDefinitions.at( sym.SymdefID );

                if( symbolDef.TextLocations.count( LINK_ORIGIN_ATTRID ) )
                {
                    TEXT_LOCATION linkOrigin = symbolDef.TextLocations.at( LINK_ORIGIN_ATTRID );
                    applyTextSettings( netLabel, linkOrigin.TextCodeID, linkOrigin.Alignment,
                                       linkOrigin.Justification );
                }

                netLabel->SetSpinStyle( getSpinStyle( sym.OrientAngle, sym.Mirror ) );

                if( libSymDef.Alternate.Lower().Contains( "in" ) )
                    netLabel->SetShape( LABEL_FLAG_SHAPE::L_INPUT );
                else if( libSymDef.Alternate.Lower().Contains( "bi" ) )
                    netLabel->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
                else if( libSymDef.Alternate.Lower().Contains( "out" ) )
                    netLabel->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );
                else
                    netLabel->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );

                SCH_SCREEN* screen = m_sheetMap.at( sym.LayerID )->GetScreen();

                // autoplace intersheet refs
                netLabel->AutoplaceFields( screen, AUTOPLACE_AUTO );

                screen->Append( netLabel );
                m_globalLabelsMap.insert( { sym.ID, netLabel } );
            }
            else
            {
                wxASSERT_MSG( false, "Unknown Symbol Variant." );
            }
        }
        else
        {
            m_reporter->Report( wxString::Format( _( "Symbol ID '%s' is of an unknown type. It is "
                                                     "neither a symbol or a net power / symbol. "
                                                     "The symbol was not loaded." ),
                                                  sym.ID ),
                                RPT_SEVERITY_ERROR );
        }

        if( sym.ScaleRatioDenominator != 1 || sym.ScaleRatioNumerator != 1 )
        {
            wxString symbolName = sym.ComponentRef.Designator;

            if( symbolName.empty() )
                symbolName = wxString::Format( "ID: %s", sym.ID );
            else
                symbolName += sym.GateID;

            m_reporter->Report( wxString::Format( _( "Symbol '%s' is scaled in the original "
                                                     "CADSTAR schematic but this is not supported "
                                                     "in KiCad. When the symbol is reloaded from "
                                                     "the library, it will revert to the original "
                                                     "1:1 scale." ),
                                                  symbolName,
                                                  sym.PartRef.RefID ),
                                RPT_SEVERITY_ERROR );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadBusses()
{
    for( std::pair<BUS_ID, BUS> busPair : Schematic.Buses )
    {
        BUS    bus     = busPair.second;
        bool   firstPt = true;
        VERTEX last;

        if( bus.LayerID != wxT( "NO_SHEET" ) )
        {
            SCH_SCREEN*                screen     = m_sheetMap.at( bus.LayerID )->GetScreen();
            std::shared_ptr<BUS_ALIAS> kiBusAlias = std::make_shared<BUS_ALIAS>();

            kiBusAlias->SetName( bus.Name );
            screen->AddBusAlias( kiBusAlias );
            m_busesMap.insert( { bus.ID, kiBusAlias } );

            SCH_LABEL* label = new SCH_LABEL();

            wxString busname = HandleTextOverbar( bus.Name );

            label->SetText( wxT( "{" ) + busname + wxT( "}" ) );
            label->SetVisible( true );
            screen->Append( label );

            SHAPE_LINE_CHAIN busLineChain; // to compute nearest segment to bus label

            for( const VERTEX& cur : bus.Shape.Vertices )
            {
                busLineChain.Append( getKiCadPoint( cur.End ) );

                if( firstPt )
                {
                    last    = cur;
                    firstPt = false;

                    if( !bus.HasBusLabel )
                    {
                        // Add a bus label on the starting point if the original CADSTAR design
                        // does not have an explicit label
                        label->SetPosition( getKiCadPoint( last.End ) );
                    }

                    continue;
                }


                SCH_LINE* kiBus = new SCH_LINE();

                kiBus->SetStartPoint( getKiCadPoint( last.End ) );
                kiBus->SetEndPoint( getKiCadPoint( cur.End ) );
                kiBus->SetLayer( LAYER_BUS );
                kiBus->SetLineWidth( getLineThickness( bus.LineCodeID ) );
                screen->Append( kiBus );

                last = cur;
            }

            if( bus.HasBusLabel )
            {
                //lets find the closest point in the busline to the label
                VECTOR2I busLabelLoc = getKiCadPoint( bus.BusLabel.Position );
                VECTOR2I nearestPt = busLineChain.NearestPoint( busLabelLoc );

                label->SetPosition( nearestPt );

                applyTextSettings( label, bus.BusLabel.TextCodeID, bus.BusLabel.Alignment,
                                   bus.BusLabel.Justification );

                // Re-set bus name as it might have been "double-escaped" after applyTextSettings
                label->SetText( wxT( "{" ) + busname + wxT( "}" ) );

                // Note orientation of the bus label will be determined in loadNets
                // (the position of the wire will determine how best to place the bus label)
            }
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadNets()
{
    for( std::pair<NET_ID, NET_SCH> netPair : Schematic.Nets )
    {
        NET_SCH                             net     = netPair.second;
        wxString                            netName = net.Name;
        std::map<NETELEMENT_ID, SCH_LABEL*> netlabels;

        if( netName.IsEmpty() )
            netName = wxString::Format( "$%ld", net.SignalNum );

        netName = HandleTextOverbar( netName );

        for( std::pair<NETELEMENT_ID, NET_SCH::SYM_TERM> terminalPair : net.Terminals )
        {
            NET_SCH::SYM_TERM netTerm = terminalPair.second;

            if( m_powerSymMap.find( netTerm.SymbolID ) != m_powerSymMap.end() )
            {
                SCH_FIELD* val = m_powerSymMap.at( netTerm.SymbolID )->GetField( FIELD_T::VALUE );
                val->SetText( netName );
                val->SetBold( false );
                val->SetVisible( false );

                if( netTerm.HasNetLabel )
                {
                    val->SetVisible( true );
                    val->SetPosition( getKiCadPoint( netTerm.NetLabel.Position ) );

                    applyTextSettings( val, netTerm.NetLabel.TextCodeID, netTerm.NetLabel.Alignment,
                                       netTerm.NetLabel.Justification, netTerm.NetLabel.OrientAngle,
                                       netTerm.NetLabel.Mirror  );
                }
            }
            else if( m_globalLabelsMap.find( netTerm.SymbolID ) != m_globalLabelsMap.end() )
            {
                m_globalLabelsMap.at( netTerm.SymbolID )->SetText( netName );

                LAYER_ID sheet = Schematic.Symbols.at( netTerm.SymbolID ).LayerID;

                if( m_sheetMap.count( sheet ) )
                {
                    SCH_SCREEN* screen = m_sheetMap.at( sheet )->GetScreen();

                    // autoplace intersheet refs again since we've changed the name
                    m_globalLabelsMap.at( netTerm.SymbolID )->AutoplaceFields( screen, AUTOPLACE_AUTO );
                }
            }
            else if( !net.Name.IsEmpty() && Schematic.Symbols.count( netTerm.SymbolID )
                     && netTerm.HasNetLabel )
            {
                // This is a named net that connects to a schematic symbol pin - we need to put a label
                SCH_LABEL* label = new SCH_LABEL();
                label->SetText( netName );

                POINT pinLocation = getLocationOfNetElement( net, netTerm.ID );
                label->SetPosition( getKiCadPoint( pinLocation ) );
                label->SetVisible( true );

                applyTextSettings( label, netTerm.NetLabel.TextCodeID, netTerm.NetLabel.Alignment,
                                   netTerm.NetLabel.Justification );

                netlabels.insert( { netTerm.ID, label } );

                LAYER_ID sheet = Schematic.Symbols.at( netTerm.SymbolID ).LayerID;
                m_sheetMap.at( sheet )->GetScreen()->Append( label );
            }
        }

        auto getHierarchicalLabel =
                [&]( const NETELEMENT_ID& aNode ) -> SCH_HIERLABEL*
                {
                    if( aNode.Contains( "BLKT" ) )
                    {
                        NET_SCH::BLOCK_TERM blockTerm = net.BlockTerminals.at( aNode );
                        BLOCK_PIN_ID blockPinID = std::make_pair( blockTerm.BlockID,
                                                                  blockTerm.TerminalID );

                        if( m_sheetPinMap.find( blockPinID ) != m_sheetPinMap.end() )
                            return m_sheetPinMap.at( blockPinID );
                    }

                    return nullptr;
                };

        //Add net name to all hierarchical pins (block terminals in CADSTAR)
        for( std::pair<NETELEMENT_ID, NET_SCH::BLOCK_TERM> blockPair : net.BlockTerminals )
        {
            SCH_HIERLABEL* label = getHierarchicalLabel( blockPair.first );

            if( label )
                label->SetText( netName );
        }

        // Load all bus entries and add net label if required
        for( std::pair<NETELEMENT_ID, NET_SCH::BUS_TERM> busPair : net.BusTerminals )
        {
            NET_SCH::BUS_TERM busTerm = busPair.second;
            BUS               bus     = Schematic.Buses.at( busTerm.BusID );

            if( !alg::contains( m_busesMap.at( bus.ID )->Members(), netName ) )
                m_busesMap.at( bus.ID )->Members().emplace_back( netName );

            SCH_BUS_WIRE_ENTRY* busEntry =
                    new SCH_BUS_WIRE_ENTRY( getKiCadPoint( busTerm.FirstPoint ), false );

            VECTOR2I size =
                    getKiCadPoint( busTerm.SecondPoint ) - getKiCadPoint( busTerm.FirstPoint );
            busEntry->SetSize( VECTOR2I( size.x, size.y ) );

            m_sheetMap.at( bus.LayerID )->GetScreen()->Append( busEntry );

            // Always add a label at bus terminals to ensure connectivity.
            // If the original design does not have a label, just make it very small
            // to keep connectivity but make the design look visually similar to
            // the original.
            SCH_LABEL* label = new SCH_LABEL();
            label->SetText( netName );
            label->SetPosition( getKiCadPoint( busTerm.SecondPoint ) );
            label->SetVisible( true );

            if( busTerm.HasNetLabel )
            {
                applyTextSettings( label, busTerm.NetLabel.TextCodeID, busTerm.NetLabel.Alignment,
                                   busTerm.NetLabel.Justification );
            }
            else
            {
                label->SetTextSize( VECTOR2I( SMALL_LABEL_SIZE, SMALL_LABEL_SIZE ) );
            }

            netlabels.insert( { busTerm.ID, label } );
            m_sheetMap.at( bus.LayerID )->GetScreen()->Append( label );
        }

        for( std::pair<NETELEMENT_ID, NET_SCH::DANGLER> danglerPair : net.Danglers )
        {
            NET_SCH::DANGLER dangler = danglerPair.second;

            SCH_LABEL* label = new SCH_LABEL();
            label->SetPosition( getKiCadPoint( dangler.Position ) );
            label->SetVisible( true );

            if( dangler.HasNetLabel )
            {
                applyTextSettings( label, dangler.NetLabel.TextCodeID, dangler.NetLabel.Alignment,
                                   dangler.NetLabel.Justification );
            }

            label->SetText( netName ); // set text after applying settings to avoid double-escaping
            netlabels.insert( { dangler.ID, label } );

            m_sheetMap.at( dangler.LayerID )->GetScreen()->Append( label );
        }

        for( NET_SCH::CONNECTION_SCH conn : net.Connections )
        {
            if( conn.LayerID == wxT( "NO_SHEET" ) )
                continue; // No point loading virtual connections. KiCad handles that internally

            POINT start = getLocationOfNetElement( net, conn.StartNode );
            POINT end = getLocationOfNetElement( net, conn.EndNode );

            if( start.x == UNDEFINED_VALUE || end.x == UNDEFINED_VALUE )
                continue;

            // Connections in CADSTAR are always implied between symbols even if the route
            // doesn't start and end exactly at the connection points
            if( conn.Path.size() < 1 || conn.Path.front() != start )
                conn.Path.insert( conn.Path.begin(), start );

            if( conn.Path.size() < 2 || conn.Path.back() != end )
                conn.Path.push_back( end );

            bool      firstPt  = true;
            bool      secondPt = false;
            VECTOR2I  last;
            SCH_LINE* wire = nullptr;

            SHAPE_LINE_CHAIN wireChain; // Create a temp. line chain representing the connection

            for( const POINT& pt : conn.Path )
                wireChain.Append( getKiCadPoint( pt ) );

            // AUTO-FIX SHEET PINS
            //--------------------
            // KiCad constrains the sheet pin on the edge of the sheet object whereas in
            // CADSTAR it can be anywhere. Let's find the intersection of the wires with the sheet
            // and place the hierarchical
            std::vector<NETELEMENT_ID> nodes;
            nodes.push_back( conn.StartNode );
            nodes.push_back( conn.EndNode );

            for( const NETELEMENT_ID& node : nodes )
            {
                SCH_HIERLABEL* sheetPin = getHierarchicalLabel( node );

                if( sheetPin )
                {
                    if( sheetPin->Type() == SCH_SHEET_PIN_T
                            && SCH_SHEET::ClassOf( sheetPin->GetParent() ) )
                    {
                        SCH_SHEET* parentSheet   = static_cast<SCH_SHEET*>( sheetPin->GetParent() );
                        VECTOR2I   sheetSize = parentSheet->GetSize();
                        VECTOR2I   sheetPosition = parentSheet->GetPosition();

                        int leftSide  = sheetPosition.x;
                        int rightSide = sheetPosition.x + sheetSize.x;
                        int topSide   = sheetPosition.y;
                        int botSide   = sheetPosition.y + sheetSize.y;

                        SHAPE_LINE_CHAIN sheetEdge;

                        sheetEdge.Append( leftSide, topSide );
                        sheetEdge.Append( rightSide, topSide );
                        sheetEdge.Append( rightSide, botSide );
                        sheetEdge.Append( leftSide, botSide );
                        sheetEdge.Append( leftSide, topSide );

                        SHAPE_LINE_CHAIN::INTERSECTIONS wireToSheetIntersects;

                        if( !wireChain.Intersect( sheetEdge, wireToSheetIntersects ) )
                        {
                            // The block terminal is outside the block shape in the original
                            // CADSTAR design. Since KiCad's Sheet Pin will already be constrained
                            // on the edge, we will simply join to it with a straight line.
                            if( node == conn.StartNode )
                                wireChain = wireChain.Reverse();

                            wireChain.Append( sheetPin->GetPosition() );

                            if( node == conn.StartNode )
                                wireChain = wireChain.Reverse();
                        }
                        else
                        {
                            // The block terminal is either inside or on the shape edge. Lets use
                            // the first intersection point.
                            VECTOR2I intsctPt   = wireToSheetIntersects.at( 0 ).p;
                            int      intsctIndx = wireChain.FindSegment( intsctPt );
                            wxASSERT_MSG( intsctIndx != -1, "Can't find intersecting segment" );

                            if( node == conn.StartNode )
                                wireChain.Replace( 0, intsctIndx, intsctPt );
                            else
                                wireChain.Replace( intsctIndx + 1, /*end index*/ -1, intsctPt );

                            sheetPin->SetPosition( intsctPt );
                        }
                    }
                }
            }

            auto fixNetLabelsAndSheetPins =
                    [&]( const EDA_ANGLE& aWireAngle, NETELEMENT_ID& aNetEleID )
                    {
                        SPIN_STYLE spin = getSpinStyle( aWireAngle );

                        if( netlabels.find( aNetEleID ) != netlabels.end() )
                            netlabels.at( aNetEleID )->SetSpinStyle( spin.MirrorY() );

                        SCH_HIERLABEL* sheetPin = getHierarchicalLabel( aNetEleID );

                        if( sheetPin )
                            sheetPin->SetSpinStyle( spin.MirrorX() );
                    };

            // Now we can load the wires and fix the label orientations
            for( const VECTOR2I& pt : wireChain.CPoints() )
            {
                if( firstPt )
                {
                    last     = pt;
                    firstPt  = false;
                    secondPt = true;
                    continue;
                }

                if( secondPt )
                {
                    secondPt = false;

                    EDA_ANGLE wireAngle( last - pt );
                    fixNetLabelsAndSheetPins( wireAngle, conn.StartNode );
                }

                wire = new SCH_LINE();

                wire->SetStartPoint( last );
                wire->SetEndPoint( pt );
                wire->SetLayer( LAYER_WIRE );

                if( !conn.ConnectionLineCode.IsEmpty() )
                    wire->SetLineWidth( getLineThickness( conn.ConnectionLineCode ) );

                last = pt;

                m_sheetMap.at( conn.LayerID )->GetScreen()->Append( wire );
            }

            //Fix labels on the end wire
            if( wire )
            {
                EDA_ANGLE wireAngle( wire->GetEndPoint() - wire->GetStartPoint() );
                fixNetLabelsAndSheetPins( wireAngle, conn.EndNode );
            }
        }

        for( std::pair<NETELEMENT_ID, NET_SCH::JUNCTION_SCH> juncPair : net.Junctions )
        {
            NET_SCH::JUNCTION_SCH junc = juncPair.second;

            SCH_JUNCTION* kiJunc = new SCH_JUNCTION();

            kiJunc->SetPosition( getKiCadPoint( junc.Location ) );
            m_sheetMap.at( junc.LayerID )->GetScreen()->Append( kiJunc );

            if( junc.HasNetLabel )
            {
                // In CADSTAR the label can be placed anywhere, but in KiCad it has to be placed
                // in the same location as the junction for it to be connected to it.
                SCH_LABEL* label = new SCH_LABEL();
                label->SetText( netName );
                label->SetPosition( getKiCadPoint( junc.Location ) );
                label->SetVisible( true );

                EDA_ANGLE  labelAngle = getAngle( junc.NetLabel.OrientAngle );
                SPIN_STYLE spin = getSpinStyle( labelAngle );

                label->SetSpinStyle( spin );

                m_sheetMap.at( junc.LayerID )->GetScreen()->Append( label );
            }
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadFigures()
{
    for( std::pair<FIGURE_ID, FIGURE> figPair : Schematic.Figures )
    {
        FIGURE fig = figPair.second;

        loadFigure( fig, fig.LayerID, LAYER_NOTES );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadTexts()
{
    for( std::pair<TEXT_ID, TEXT> textPair : Schematic.Texts )
    {
        TEXT txt = textPair.second;

        SCH_TEXT* kiTxt = getKiCadSchText( txt );
        loadItemOntoKiCadSheet( txt.LayerID, kiTxt );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadDocumentationSymbols()
{
    for( std::pair<DOCUMENTATION_SYMBOL_ID, DOCUMENTATION_SYMBOL> docSymPair :
         Schematic.DocumentationSymbols )
    {
        DOCUMENTATION_SYMBOL docSym = docSymPair.second;

        if( Library.SymbolDefinitions.find( docSym.SymdefID ) == Library.SymbolDefinitions.end() )
        {
            m_reporter->Report( wxString::Format( _( "Documentation Symbol '%s' refers to symbol "
                                                     "definition ID '%s' which does not exist in "
                                                     "the library. The symbol was not loaded." ),
                                                  docSym.ID,
                                                  docSym.SymdefID ),
                                RPT_SEVERITY_ERROR );
            continue;
        }

        SYMDEF_SCM docSymDef  = Library.SymbolDefinitions.at( docSym.SymdefID );
        VECTOR2I   moveVector = getKiCadPoint( docSym.Origin ) - getKiCadPoint( docSymDef.Origin );
        EDA_ANGLE  rotationAngle     = getAngle( docSym.OrientAngle );
        double     scalingFactor     = (double) docSym.ScaleRatioNumerator
                                        / (double) docSym.ScaleRatioDenominator;
        VECTOR2I   centreOfTransform = getKiCadPoint( docSymDef.Origin );
        bool       mirrorInvert      = docSym.Mirror;

        for( std::pair<FIGURE_ID, FIGURE> figPair : docSymDef.Figures )
        {
            FIGURE fig = figPair.second;

            loadFigure( fig, docSym.LayerID, LAYER_NOTES, moveVector, rotationAngle, scalingFactor,
                        centreOfTransform, mirrorInvert );
        }

        for( std::pair<TEXT_ID, TEXT> textPair : docSymDef.Texts )
        {
            TEXT txt = textPair.second;

            txt.Mirror = ( txt.Mirror ) ? !mirrorInvert : mirrorInvert;
            txt.OrientAngle = docSym.OrientAngle - txt.OrientAngle;

            SCH_TEXT* kiTxt = getKiCadSchText( txt );

            VECTOR2I newPosition = applyTransform( kiTxt->GetPosition(), moveVector, rotationAngle,
                                                   scalingFactor, centreOfTransform, mirrorInvert );

            int     newTxtWidth     = KiROUND( kiTxt->GetTextWidth() * scalingFactor );
            int     newTxtHeight    = KiROUND( kiTxt->GetTextHeight() * scalingFactor );
            int     newTxtThickness = KiROUND( kiTxt->GetTextThickness() * scalingFactor );

            kiTxt->SetPosition( newPosition );
            kiTxt->SetTextWidth( newTxtWidth );
            kiTxt->SetTextHeight( newTxtHeight );
            kiTxt->SetTextThickness( newTxtThickness );

            loadItemOntoKiCadSheet( docSym.LayerID, kiTxt );
        }
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadTextVariables()
{
    auto findAndReplaceTextField =
            [&]( TEXT_FIELD_NAME aField, wxString aValue )
            {
                if( m_context.TextFieldToValuesMap.find( aField ) != m_context.TextFieldToValuesMap.end() )
                {
                    if( m_context.TextFieldToValuesMap.at( aField ) != aValue )
                    {
                        m_context.TextFieldToValuesMap.at( aField ) = aValue;
                        m_context.InconsistentTextFields.insert( aField );
                        return false;
                    }
                }
                else
                {
                    m_context.TextFieldToValuesMap.insert( { aField, aValue } );
                }

                return true;
            };

    PROJECT* pj = &m_schematic->Project();

    if( pj )
    {
        std::map<wxString, wxString>& txtVars = pj->GetTextVars();

        // Most of the design text fields can be derived from other elements
        if( Schematic.VariantHierarchy.Variants.size() > 0 )
        {
            VARIANT loadedVar = Schematic.VariantHierarchy.Variants.begin()->second;

            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_NAME, loadedVar.Name );
            findAndReplaceTextField( TEXT_FIELD_NAME::VARIANT_DESCRIPTION, loadedVar.Description );
        }

        findAndReplaceTextField( TEXT_FIELD_NAME::DESIGN_TITLE, Header.JobTitle );

        for( std::pair<TEXT_FIELD_NAME, wxString> txtvalue : m_context.TextFieldToValuesMap )
        {
            wxString varName  = CADSTAR_TO_KICAD_FIELDS.at( txtvalue.first );
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }

        for( std::pair<wxString, wxString> txtvalue : m_context.FilenamesToTextMap )
        {
            wxString varName  = txtvalue.first;
            wxString varValue = txtvalue.second;

            txtVars.insert( { varName, varValue } );
        }
    }
    else
    {
        m_reporter->Report( _( "Text Variables could not be set as there is no project attached." ),
                            RPT_SEVERITY_ERROR );
    }
}


SCH_FIELD*
CADSTAR_SCH_ARCHIVE_LOADER::addNewFieldToSymbol( const wxString&              aFieldName,
                                                 std::unique_ptr<LIB_SYMBOL>& aKiCadSymbol )
{
    // First Check if field already exists
    if( SCH_FIELD* existingField = aKiCadSymbol->GetField( aFieldName ) )
        return existingField;

    SCH_FIELD* newfield = new SCH_FIELD( aKiCadSymbol.get(), FIELD_T::USER, aFieldName );
    newfield->SetVisible( false );
    aKiCadSymbol->AddField( newfield );
    /*
    @todo we should load that a field is a URL by checking if it starts with "Link"
    e.g.:
    if( aFieldName.Lower().StartsWith( "link" ) )
        newfield->SetAsURL*/

    return newfield;
}


const LIB_SYMBOL* CADSTAR_SCH_ARCHIVE_LOADER::loadSymdef( const SYMDEF_ID& aSymdefID )
{
    wxCHECK( Library.SymbolDefinitions.find( aSymdefID ) != Library.SymbolDefinitions.end(), nullptr );

    if( m_symDefMap.count( aSymdefID ) )
        return m_symDefMap.at( aSymdefID ).get(); // return a non-owning ptr

    SYMDEF_SCM csSym = Library.SymbolDefinitions.at( aSymdefID );
    std::unique_ptr<LIB_SYMBOL> kiSym = std::make_unique<LIB_SYMBOL>( csSym.BuildLibName() );
    const int                   gateNumber = 1; // Always load to gate "A" - we will change the unit later

    // Load Graphical Figures
    for( std::pair<FIGURE_ID, FIGURE> figPair : csSym.Figures )
    {
        FIGURE         fig = figPair.second;
        int            lineThickness = getLineThickness( fig.LineCodeID );
        LINE_STYLE     linestyle = getLineStyle( fig.LineCodeID );

        if( fig.Shape.Type == SHAPE_TYPE::OPENSHAPE )
        {
            loadLibrarySymbolShapeVertices( fig.Shape.Vertices, csSym.Origin, kiSym.get(),
                                            gateNumber,
                                            lineThickness );
        }
        else
        {
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

            shape->SetPolyShape( fig.Shape.ConvertToPolySet(
                    [&]( const VECTOR2I& aPt )
                    {
                        return getKiCadLibraryPoint( aPt, csSym.Origin );
                    },
                    ARC_ACCURACY ) );

            shape->SetUnit( gateNumber );

            shape->SetStroke( STROKE_PARAMS( lineThickness, linestyle ) );

            if( fig.Shape.Type == SHAPE_TYPE::SOLID )
                shape->SetFillMode( FILL_T::FILLED_SHAPE );
            else if( fig.Shape.Type == SHAPE_TYPE::OUTLINE )
                shape->SetFillMode( FILL_T::NO_FILL );
            else if( fig.Shape.Type == SHAPE_TYPE::HATCHED ) // We don't have an equivalent
                shape->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );

            kiSym->AddDrawItem( shape );
        }
    }

    PINNUM_TO_TERMINAL_MAP pinNumToTerminals;

    // Load Pins
    for( std::pair<TERMINAL_ID, TERMINAL> termPair : csSym.Terminals )
    {
        TERMINAL term = termPair.second;
        wxString pinNum = wxString::Format( "%ld", term.ID );
        wxString pinName = wxEmptyString;
        std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( kiSym.get() );

        // Assume passive pin for now (we will set it later once we load the parts)
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );

        pin->SetPosition( getKiCadLibraryPoint( term.Position, csSym.Origin ) );
        pin->SetLength( 0 ); //CADSTAR Pins are just a point (have no length)
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );
        pin->SetUnit( gateNumber );
        pin->SetNumber( pinNum );
        pin->SetName( pinName );

        // TC0 is the default CADSTAR text size for name/number if none specified
        int pinNumberHeight = getTextHeightFromTextCode( wxT( "TC0" ) );
        int pinNameHeight = getTextHeightFromTextCode( wxT( "TC0" ) );

        if( csSym.PinNumberLocations.count( term.ID ) )
        {
            PIN_NUM_LABEL_LOC pinNumLocation = csSym.PinNumberLocations.at( term.ID );
            pinNumberHeight = getTextHeightFromTextCode( pinNumLocation.TextCodeID );
        }

        if( csSym.PinLabelLocations.count( term.ID ) )
        {
            PIN_NUM_LABEL_LOC pinNameLocation = csSym.PinLabelLocations.at( term.ID );
            pinNameHeight = getTextHeightFromTextCode( pinNameLocation.TextCodeID );
        }

        pin->SetNumberTextSize( pinNumberHeight );
        pin->SetNameTextSize( pinNameHeight );

        pinNumToTerminals.insert( { pin->GetNumber(), term.ID } );
        kiSym->AddDrawItem( pin.release() );
    }

    m_symDefTerminalsMap.insert( { aSymdefID, pinNumToTerminals } );
    fixUpLibraryPins( kiSym.get(), gateNumber );


    // Load Text items
    for( std::pair<TEXT_ID, TEXT> textPair : csSym.Texts )
    {
        TEXT csText = textPair.second;
        VECTOR2I pos = getKiCadLibraryPoint( csText.Position, csSym.Origin );
        auto     libtext = std::make_unique<SCH_TEXT>( pos, csText.Text, LAYER_DEVICE );

        libtext->SetUnit( gateNumber );
        libtext->SetPosition( getKiCadLibraryPoint( csText.Position, csSym.Origin ) );
        libtext->SetMultilineAllowed( true ); // temporarily so that we calculate bbox correctly

        applyTextSettings( libtext.get(), csText.TextCodeID, csText.Alignment, csText.Justification,
                           csText.OrientAngle, csText.Mirror );

        // Split out multi line text items into individual text elements
        if( csText.Text.Contains( "\n" ) )
        {
            wxArrayString strings;
            wxStringSplit( csText.Text, strings, '\n' );

            for( size_t ii = 0; ii < strings.size(); ++ii )
            {
                BOX2I    bbox = libtext->GetTextBox( nullptr, ii );
                VECTOR2I linePos = { bbox.GetLeft(), -bbox.GetBottom() };

                RotatePoint( linePos, libtext->GetTextPos(), -libtext->GetTextAngle() );

                SCH_TEXT* textLine = static_cast<SCH_TEXT*>( libtext->Duplicate( IGNORE_PARENT_GROUP ) );
                textLine->SetText( strings[ii] );
                textLine->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                textLine->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                textLine->SetTextPos( linePos );

                // Multiline text not allowed in LIB_TEXT
                textLine->SetMultilineAllowed( false );
                kiSym->AddDrawItem( textLine );
            }
        }
        else
        {
            // Multiline text not allowed in LIB_TEXT
            libtext->SetMultilineAllowed( false );
            kiSym->AddDrawItem( libtext.release() );
        }
    }

    // CADSTAR uses TC1 when fields don't have explicit text/attribute location
    static const TEXTCODE_ID defaultTextCode = "TC1";

    // Load field locations (Attributes in CADSTAR)

    // Symbol name (e.g. R1)
    if( csSym.TextLocations.count( SYMBOL_NAME_ATTRID ) )
    {
        TEXT_LOCATION& textLoc = csSym.TextLocations.at( SYMBOL_NAME_ATTRID );
        applyToLibraryFieldAttribute( textLoc, csSym.Origin, &kiSym->GetReferenceField() );
    }
    else
    {
        applyTextCodeIfExists( &kiSym->GetReferenceField(), defaultTextCode );
    }

    // Always add the part name field (even if it doesn't have a specific location defined)
    SCH_FIELD* partField = addNewFieldToSymbol( PartNameFieldName, kiSym );
    wxCHECK( partField, nullptr );
    wxASSERT( partField->GetName() == PartNameFieldName );

    if( csSym.TextLocations.count( PART_NAME_ATTRID ) )
    {
        TEXT_LOCATION& textLoc = csSym.TextLocations.at( PART_NAME_ATTRID );
        applyToLibraryFieldAttribute( textLoc, csSym.Origin, partField );
    }
    else
    {
        applyTextCodeIfExists( partField, defaultTextCode );
    }

    partField->SetVisible( SymbolPartNameColor.IsVisible );


    for( auto& [attributeId, textLocation] : csSym.TextLocations )
    {
        if( attributeId == PART_NAME_ATTRID || attributeId == SYMBOL_NAME_ATTRID
            || attributeId == SIGNALNAME_ORIGIN_ATTRID || attributeId == LINK_ORIGIN_ATTRID )
        {
            continue;
        }

        wxString attributeName = getAttributeName( attributeId );
        SCH_FIELD* field = addNewFieldToSymbol( attributeName, kiSym );
        applyToLibraryFieldAttribute( textLocation, csSym.Origin, field );
    }


    for( auto& [attributeId, attrValue] : csSym.AttributeValues )
    {
        if( attributeId == PART_NAME_ATTRID || attributeId == SYMBOL_NAME_ATTRID
            || attributeId == SIGNALNAME_ORIGIN_ATTRID || attributeId == LINK_ORIGIN_ATTRID )
        {
            continue;
        }

        wxString   attributeName = getAttributeName( attributeId );
        SCH_FIELD* field = addNewFieldToSymbol( attributeName, kiSym );

        if( attrValue.HasLocation )
            applyToLibraryFieldAttribute( attrValue.AttributeLocation, csSym.Origin, field );
        else
            applyTextCodeIfExists( field, defaultTextCode );
    }


    m_symDefMap.insert( { aSymdefID, std::move( kiSym ) } );

    return m_symDefMap.at( aSymdefID ).get(); // return a non-owning ptr
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSymbolGateAndPartFields( const SYMDEF_ID& aSymdefID,
                                                              const PART& aCadstarPart,
                                                              const GATE_ID& aGateID,
                                                              LIB_SYMBOL* aSymbol )
{
    wxCHECK( Library.SymbolDefinitions.find( aSymdefID ) != Library.SymbolDefinitions.end(), /*void*/ );

    std::unique_ptr<LIB_SYMBOL> kiSymDef( loadSymdef( aSymdefID )->Duplicate() );
    wxCHECK( kiSymDef, /*void*/ );

    //todo: need to use unique_ptr more. For now just create it here and release at end of function
    std::unique_ptr<LIB_SYMBOL> tempSymbol( aSymbol );

    // Update the pin numbers to match those defined in the Cadstar part
    TERMINAL_TO_PINNUM_MAP pinNumMap;

    for( auto&& [storedPinNum, termID] : m_symDefTerminalsMap[aSymdefID] )
    {
        PART::DEFINITION::PIN csPin = getPartDefinitionPin( aCadstarPart, aGateID, termID );
        SCH_PIN*              pin = kiSymDef->GetPin( storedPinNum );

        wxString pinName = HandleTextOverbar( csPin.Label );
        wxString pinNum = HandleTextOverbar( csPin.Name );

        if( pinNum.IsEmpty() )
        {
            if( !csPin.Identifier.IsEmpty() )
                pinNum = csPin.Identifier;
            else if( csPin.ID == UNDEFINED_VALUE )
                pinNum = wxString::Format( "%ld", termID );
            else
                pinNum = wxString::Format( "%ld", csPin.ID );
        }

        pin->SetType( getKiCadPinType( csPin.Type ) );
        pin->SetNumber( pinNum );
        pin->SetName( pinName );

        pinNumMap.insert( { termID, pinNum } );
    }

    m_pinNumsMap.insert( { aCadstarPart.ID + aGateID, pinNumMap } );

    // COPY ITEMS
    int gateNumber = getKiCadUnitNumberFromGate( aGateID );
    copySymbolItems( kiSymDef, tempSymbol, gateNumber );

    // Hide the value field for now (it might get unhidden if an attribute exists in the cadstar
    // design with the text "Value"
    tempSymbol->GetValueField().SetVisible( false );


    if( SCH_FIELD* partNameField = tempSymbol->GetField( PartNameFieldName ) )
        partNameField->SetText( EscapeFieldText( aCadstarPart.Name ) );

    const POINT& symDefOrigin = Library.SymbolDefinitions.at( aSymdefID ).Origin;
    wxString     footprintRefName = wxEmptyString;
    wxString     footprintAlternateName = wxEmptyString;

    auto loadLibraryField = [&]( const ATTRIBUTE_VALUE& aAttributeVal )
    {
        wxString attrName = getAttributeName( aAttributeVal.AttributeID );

        // Remove invalid field characters
        wxString attributeValue = aAttributeVal.Value;
        attributeValue.Replace( wxT( "\n" ), wxT( "\\n" ) );
        attributeValue.Replace( wxT( "\r" ), wxT( "\\r" ) );
        attributeValue.Replace( wxT( "\t" ), wxT( "\\t" ) );

        //TODO: Handle "links": In cadstar a field can be a "link" if its name starts
        // with the characters "Link ". Need to figure out how to convert them to
        // equivalent in KiCad.

        if( attrName == wxT( "(PartDefinitionNameStem)" ) )
        {
            //Space not allowed in Reference field
            attributeValue.Replace( wxT( " " ), "_" );
            tempSymbol->GetReferenceField().SetText( attributeValue );
            return;
        }
        else if( attrName == wxT( "(PartDescription)" ) )
        {
            tempSymbol->SetDescription( attributeValue );
            return;
        }
        else if( attrName == wxT( "(PartDefinitionReferenceName)" ) )
        {
            footprintRefName = attributeValue;
            return;
        }
        else if( attrName == wxT( "(PartDefinitionAlternateName)" ) )
        {
            footprintAlternateName = attributeValue;
            return;
        }

        bool       attrIsNew = tempSymbol->GetField( attrName ) == nullptr;
        SCH_FIELD* attrField = addNewFieldToSymbol( attrName, tempSymbol );

        wxASSERT( attrField->GetName() == attrName );
        attrField->SetText( aAttributeVal.Value );
        attrField->SetUnit( gateNumber );

        const ATTRIBUTE_ID& attrid = aAttributeVal.AttributeID;
        attrField->SetVisible( isAttributeVisible( attrid ) );

        if( aAttributeVal.HasLocation )
        {
            // Check if the part itself defined a location for the field
            applyToLibraryFieldAttribute( aAttributeVal.AttributeLocation, symDefOrigin,
                                          attrField );
        }
        else if( attrIsNew )
        {
            attrField->SetVisible( false );
            applyTextSettings( attrField, wxT( "TC1" ), ALIGNMENT::NO_ALIGNMENT,
                               JUSTIFICATION::LEFT, false, true );
        }
    };

    // Load all attributes in the Part Definition
    for( auto& [attrId, attrVal] : aCadstarPart.Definition.AttributeValues )
        loadLibraryField( attrVal );

    // Load all attributes in the Part itself.
    for( auto& [attrId, attrVal] : aCadstarPart.AttributeValues )
        loadLibraryField( attrVal );

    setFootprintOnSymbol( tempSymbol, footprintRefName, footprintAlternateName );

    if( aCadstarPart.Definition.HidePinNames )
    {
        tempSymbol->SetShowPinNames( false );
        tempSymbol->SetShowPinNumbers( false );
    }

    // Update aSymbol just to keep lint happy.
    aSymbol = tempSymbol.release();
}


void CADSTAR_SCH_ARCHIVE_LOADER::setFootprintOnSymbol( std::unique_ptr<LIB_SYMBOL>& aKiCadSymbol,
                                                       const wxString& aFootprintName,
                                                       const wxString& aFootprintAlternate )
{
    wxString fpNameInLibrary = generateLibName( aFootprintName, aFootprintAlternate );

    if( !fpNameInLibrary.IsEmpty() )
    {
        wxArrayString fpFilters;
        fpFilters.Add( aFootprintName ); // In cadstar one footprint has several "alternates"

        if( !aFootprintAlternate.IsEmpty() )
            fpFilters.Add( fpNameInLibrary );

        aKiCadSymbol->SetFPFilters( fpFilters );

        LIB_ID libID( m_footprintLibName, fpNameInLibrary );
        aKiCadSymbol->GetFootprintField().SetText( libID.Format() );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadLibrarySymbolShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
                                                                 const VECTOR2I& aSymbolOrigin,
                                                                 LIB_SYMBOL* aSymbol,
                                                                 int aGateNumber,
                                                                 int aLineThickness )
{
    const VERTEX* prev = &aCadstarVertices.at( 0 );
    const VERTEX* cur;

    wxASSERT_MSG( prev->Type == VERTEX_TYPE::VT_POINT, "First vertex should always be a point." );

    for( size_t i = 1; i < aCadstarVertices.size(); i++ )
    {
        cur = &aCadstarVertices.at( i );

        SCH_SHAPE* shape      = nullptr;
        bool       cw         = false;
        VECTOR2I   startPoint = getKiCadLibraryPoint( prev->End, aSymbolOrigin );
        VECTOR2I   endPoint   = getKiCadLibraryPoint( cur->End, aSymbolOrigin );
        VECTOR2I   centerPoint;

        if( cur->Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE
                || cur->Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
        {
            centerPoint = ( startPoint + endPoint ) / 2;
        }
        else
        {
            centerPoint = getKiCadLibraryPoint( cur->Center, aSymbolOrigin );
        }


        switch( cur->Type )
        {
        case VERTEX_TYPE::VT_POINT:
            shape = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );
            shape->AddPoint( startPoint );
            shape->AddPoint( endPoint );
            break;

        case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::CLOCKWISE_ARC:
            cw = true;
            KI_FALLTHROUGH;

        case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::ANTICLOCKWISE_ARC:
            shape = new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE );

            shape->SetPosition( centerPoint );

            if( cw )
            {
                shape->SetStart( endPoint );
                shape->SetEnd( startPoint );
            }
            else
            {
                shape->SetStart( startPoint );
                shape->SetEnd( endPoint );
            }

            break;
        }

        shape->SetUnit( aGateNumber );
        shape->SetStroke( STROKE_PARAMS( aLineThickness, LINE_STYLE::SOLID ) );
        aSymbol->AddDrawItem( shape, false );

        prev = cur;
    }

    aSymbol->GetDrawItems().sort();
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyToLibraryFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                                               const VECTOR2I& aSymbolOrigin,
                                                               SCH_FIELD* aKiCadField )
{
    aKiCadField->SetTextPos( getKiCadLibraryPoint( aCadstarAttrLoc.Position, aSymbolOrigin ) );

    applyTextSettings( aKiCadField, aCadstarAttrLoc.TextCodeID, aCadstarAttrLoc.Alignment,
                       aCadstarAttrLoc.Justification, aCadstarAttrLoc.OrientAngle,
                       aCadstarAttrLoc.Mirror );
}


SCH_SYMBOL* CADSTAR_SCH_ARCHIVE_LOADER::loadSchematicSymbol( const SYMBOL& aCadstarSymbol,
                                                             const LIB_SYMBOL& aKiCadPart,
                                                             EDA_ANGLE& aComponentOrientation )
{
    wxString libName = CreateLibName( m_footprintLibName, m_rootSheet );

    LIB_ID libId;
    libId.SetLibItemName( aKiCadPart.GetName() );
    libId.SetLibNickname( libName );

    int unit = getKiCadUnitNumberFromGate( aCadstarSymbol.GateID );

    SCH_SHEET_PATH sheetpath;
    SCH_SHEET* kiSheet = m_sheetMap.at( aCadstarSymbol.LayerID );
    m_rootSheet->LocatePathOfScreen( kiSheet->GetScreen(), &sheetpath );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( aKiCadPart, libId, &sheetpath, unit );

    if( aCadstarSymbol.IsComponent )
        symbol->SetRef( &sheetpath, aCadstarSymbol.ComponentRef.Designator );

    symbol->SetPosition( getKiCadPoint( aCadstarSymbol.Origin ) );

    EDA_ANGLE compAngle = getAngle( aCadstarSymbol.OrientAngle );
    int       compOrientation = 0;

    if( aCadstarSymbol.Mirror )
    {
        compAngle = -compAngle;
        compOrientation += SYMBOL_ORIENTATION_T::SYM_MIRROR_Y;
    }

    compOrientation += getComponentOrientation( compAngle, aComponentOrientation );
    EDA_ANGLE test1( compAngle );
    EDA_ANGLE test2( aComponentOrientation );

    if( test1.Normalize180() != test2.Normalize180() )
    {
        m_reporter->Report( wxString::Format( _( "Symbol '%s' is rotated by an angle of %.1f " //format:allow
                                                 "degrees in the original CADSTAR design but "
                                                 "KiCad only supports rotation angles multiples "
                                                 "of 90 degrees. The connecting wires will need "
                                                 "manual fixing." ),
                                              aCadstarSymbol.ComponentRef.Designator,
                                              compAngle.AsDegrees() ),
                            RPT_SEVERITY_ERROR );
    }

    symbol->SetOrientation( compOrientation );

    if( m_sheetMap.find( aCadstarSymbol.LayerID ) == m_sheetMap.end() )
    {
        m_reporter->Report( wxString::Format( _( "Symbol '%s' references sheet ID '%s' which does "
                                                 "not exist in the design. The symbol was not "
                                                 "loaded." ),
                                              aCadstarSymbol.ComponentRef.Designator,
                                              aCadstarSymbol.LayerID ),
                            RPT_SEVERITY_ERROR );

        delete symbol;
        return nullptr;
    }

    wxString gate = ( aCadstarSymbol.GateID.IsEmpty() ) ? wxString( wxT( "A" ) ) : aCadstarSymbol.GateID;
    wxString partGateIndex = aCadstarSymbol.PartRef.RefID + gate;

    //Handle pin swaps
    if( m_pinNumsMap.find( partGateIndex ) != m_pinNumsMap.end() )
    {
        TERMINAL_TO_PINNUM_MAP termNumMap = m_pinNumsMap.at( partGateIndex );

        std::map<wxString, SCH_PIN*> pinNumToLibPinMap;

        for( auto& term : termNumMap )
        {
            wxString pinNum = term.second;
            pinNumToLibPinMap.insert( { pinNum,
                                        symbol->GetLibSymbolRef()->GetPin( term.second ) } );
        }

        auto replacePinNumber =
                [&]( wxString aOldPinNum, wxString aNewPinNum )
                {
                    if( aOldPinNum == aNewPinNum )
                        return;

                    SCH_PIN* libpin = pinNumToLibPinMap.at( aOldPinNum );
                    libpin->SetNumber( HandleTextOverbar( aNewPinNum ) );
                };

        //Older versions of Cadstar used pin numbers
        for( auto& pinPair : aCadstarSymbol.PinNumbers )
        {
            SYMBOL::PIN_NUM pin = pinPair.second;

            replacePinNumber( termNumMap.at( pin.TerminalID ),
                              wxString::Format( "%ld", pin.PinNum ) );
        }

        //Newer versions of Cadstar use pin names
        for( auto& pinPair : aCadstarSymbol.PinNames )
        {
            SYMPINNAME_LABEL pin = pinPair.second;
            replacePinNumber( termNumMap.at( pin.TerminalID ), pin.NameOrLabel );
        }

        symbol->UpdatePins();
    }

    kiSheet->GetScreen()->Append( symbol );

    return symbol;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSymbolFieldAttribute( const ATTRIBUTE_LOCATION& aCadstarAttrLoc,
                                                           const EDA_ANGLE& aComponentOrientation,
                                                           bool aIsMirrored,
                                                           SCH_FIELD* aKiCadField )
{
    aKiCadField->SetPosition( getKiCadPoint( aCadstarAttrLoc.Position ) );
    aKiCadField->SetVisible( true );

    ALIGNMENT alignment = aCadstarAttrLoc.Alignment;
    EDA_ANGLE textAngle = getAngle( aCadstarAttrLoc.OrientAngle );

    if( aIsMirrored )
    {
        // We need to change the aligment when the symbol is mirrored based on the text orientation
        // To ensure the anchor point is the same in KiCad.

        int textIsVertical = KiROUND( textAngle.AsDegrees() / 90.0 ) % 2;

        if( textIsVertical )
            alignment = rotate180( alignment );

        alignment = mirrorX( alignment );
    }

    applyTextSettings( aKiCadField, aCadstarAttrLoc.TextCodeID, alignment,
                       aCadstarAttrLoc.Justification,
                       getCadstarAngle( textAngle - aComponentOrientation ),
                       aCadstarAttrLoc.Mirror );
}


int CADSTAR_SCH_ARCHIVE_LOADER::getComponentOrientation( const EDA_ANGLE& aOrientAngle,
                                                         EDA_ANGLE& aReturnedOrientation )
{
    int compOrientation = SYMBOL_ORIENTATION_T::SYM_ORIENT_0;

    EDA_ANGLE oDeg = aOrientAngle;
    oDeg.Normalize180();

    if( oDeg >= -ANGLE_45 && oDeg <= ANGLE_45 )
    {
        compOrientation      = SYMBOL_ORIENTATION_T::SYM_ORIENT_0;
        aReturnedOrientation = ANGLE_0;
    }
    else if( oDeg >= ANGLE_45 && oDeg <= ANGLE_135 )
    {
        compOrientation      = SYMBOL_ORIENTATION_T::SYM_ORIENT_90;
        aReturnedOrientation = ANGLE_90;
    }
    else if( oDeg >= ANGLE_135 || oDeg <= -ANGLE_135 )
    {
        compOrientation      = SYMBOL_ORIENTATION_T::SYM_ORIENT_180;
        aReturnedOrientation = ANGLE_180;
    }
    else
    {
        compOrientation      = SYMBOL_ORIENTATION_T::SYM_ORIENT_270;
        aReturnedOrientation = ANGLE_270;
    }

    return compOrientation;
}


CADSTAR_SCH_ARCHIVE_LOADER::POINT
CADSTAR_SCH_ARCHIVE_LOADER::getLocationOfNetElement( const NET_SCH&       aNet,
                                                     const NETELEMENT_ID& aNetElementID )
{
    // clang-format off
    auto logUnknownNetElementError =
        [&]()
        {
            m_reporter->Report( wxString::Format( _( "Net %s references unknown net element %s. "
                                                     "The net was not properly loaded and may "
                                                     "require manual fixing." ),
                                                  getNetName( aNet ),
                                                  aNetElementID ),
                                RPT_SEVERITY_ERROR );

            return POINT();
        };
    // clang-format on

    if( aNetElementID.Contains( "J" ) ) // Junction
    {
        if( aNet.Junctions.find( aNetElementID ) == aNet.Junctions.end() )
            return logUnknownNetElementError();

        return aNet.Junctions.at( aNetElementID ).Location;
    }
    else if( aNetElementID.Contains( "P" ) ) // Terminal/Pin of a symbol
    {
        if( aNet.Terminals.find( aNetElementID ) == aNet.Terminals.end() )
            return logUnknownNetElementError();

        SYMBOL_ID   symid  = aNet.Terminals.at( aNetElementID ).SymbolID;
        TERMINAL_ID termid = aNet.Terminals.at( aNetElementID ).TerminalID;

        if( Schematic.Symbols.find( symid ) == Schematic.Symbols.end() )
            return logUnknownNetElementError();

        SYMBOL    sym          = Schematic.Symbols.at( symid );
        SYMDEF_ID symdefid     = sym.SymdefID;
        VECTOR2I  symbolOrigin = sym.Origin;

        if( Library.SymbolDefinitions.find( symdefid ) == Library.SymbolDefinitions.end() )
            return logUnknownNetElementError();

        VECTOR2I libpinPosition =
                Library.SymbolDefinitions.at( symdefid ).Terminals.at( termid ).Position;
        VECTOR2I libOrigin = Library.SymbolDefinitions.at( symdefid ).Origin;

        VECTOR2I pinOffset = libpinPosition - libOrigin;
        pinOffset.x = ( pinOffset.x * sym.ScaleRatioNumerator ) / sym.ScaleRatioDenominator;
        pinOffset.y = ( pinOffset.y * sym.ScaleRatioNumerator ) / sym.ScaleRatioDenominator;

        VECTOR2I  pinPosition = symbolOrigin + pinOffset;
        EDA_ANGLE compAngle = getAngle( sym.OrientAngle );

        if( sym.Mirror )
            pinPosition.x = ( 2 * symbolOrigin.x ) - pinPosition.x;

        EDA_ANGLE adjustedOrientation;
        getComponentOrientation( compAngle, adjustedOrientation );

        RotatePoint( pinPosition, symbolOrigin, -adjustedOrientation );

        POINT retval;
        retval.x = pinPosition.x;
        retval.y = pinPosition.y;

        return retval;
    }
    else if( aNetElementID.Contains( "BT" ) ) // Bus Terminal
    {
        if( aNet.BusTerminals.find( aNetElementID ) == aNet.BusTerminals.end() )
            return logUnknownNetElementError();

        return aNet.BusTerminals.at( aNetElementID ).SecondPoint;
    }
    else if( aNetElementID.Contains( "BLKT" ) ) // Block Terminal (sheet hierarchy connection)
    {
        if( aNet.BlockTerminals.find( aNetElementID ) == aNet.BlockTerminals.end() )
            return logUnknownNetElementError();

        BLOCK_ID    blockid = aNet.BlockTerminals.at( aNetElementID ).BlockID;
        TERMINAL_ID termid  = aNet.BlockTerminals.at( aNetElementID ).TerminalID;

        if( Schematic.Blocks.find( blockid ) == Schematic.Blocks.end() )
            return logUnknownNetElementError();

        return Schematic.Blocks.at( blockid ).Terminals.at( termid ).Position;
    }
    else if( aNetElementID.Contains( "D" ) ) // Dangler
    {
        if( aNet.Danglers.find( aNetElementID ) == aNet.Danglers.end() )
            return logUnknownNetElementError();

        return aNet.Danglers.at( aNetElementID ).Position;
    }
    else
    {
        return logUnknownNetElementError();
    }
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getNetName( const NET_SCH& aNet )
{
    wxString netname = aNet.Name;

    if( netname.IsEmpty() )
        netname = wxString::Format( "$%ld", aNet.SignalNum );

    return netname;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
                                                    LINECODE_ID aCadstarLineCodeID,
                                                    LAYER_ID aCadstarSheetID,
                                                    SCH_LAYER_ID aKiCadSchLayerID,
                                                    const VECTOR2I& aMoveVector,
                                                    const EDA_ANGLE& aRotation,
                                                    const double& aScalingFactor,
                                                    const VECTOR2I& aTransformCentre,
                                                    const bool& aMirrorInvert )
{
    int lineWidth = KiROUND( getLineThickness( aCadstarLineCodeID ) * aScalingFactor );
    LINE_STYLE lineStyle = getLineStyle( aCadstarLineCodeID );

    const VERTEX* prev = &aCadstarVertices.at( 0 );
    const VERTEX* cur;

    wxASSERT_MSG( prev->Type == VERTEX_TYPE::VT_POINT,
                  "First vertex should always be a point vertex" );

    auto pointTransform =
            [&]( const VECTOR2I& aV )
            {
                return applyTransform( getKiCadPoint( aV ), aMoveVector, aRotation,
                                       aScalingFactor, aTransformCentre, aMirrorInvert );
            };

    for( size_t ii = 1; ii < aCadstarVertices.size(); ii++ )
    {
        cur = &aCadstarVertices.at( ii );

        VECTOR2I transformedStartPoint = pointTransform( prev->End );
        VECTOR2I transformedEndPoint = pointTransform( cur->End );

        switch( cur->Type )
        {
        case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::CLOCKWISE_ARC:
        case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::ANTICLOCKWISE_ARC:
        {
            SHAPE_ARC tempArc = cur->BuildArc( transformedStartPoint, pointTransform );

            SCH_SHAPE* arcShape = new SCH_SHAPE( SHAPE_T::ARC, LAYER_NOTES, lineWidth );
            arcShape->SetArcGeometry( tempArc.GetP0(), tempArc.GetArcMid(), tempArc.GetP1() );

            loadItemOntoKiCadSheet( aCadstarSheetID, arcShape );
            break;
        }

        case VERTEX_TYPE::VT_POINT:
        {
            SCH_LINE* segment = new SCH_LINE();

            segment->SetLayer( aKiCadSchLayerID );
            segment->SetLineWidth( lineWidth );
            segment->SetLineStyle( lineStyle );

            segment->SetStartPoint( transformedStartPoint );
            segment->SetEndPoint( transformedEndPoint );

            loadItemOntoKiCadSheet( aCadstarSheetID, segment );
            break;
        }

        default:
            wxFAIL_MSG( "Unknown CADSTAR Vertex type" );
        }

        prev = cur;
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadFigure( const FIGURE& aCadstarFigure,
                                             const LAYER_ID& aCadstarSheetIDOverride,
                                             SCH_LAYER_ID aKiCadSchLayerID,
                                             const VECTOR2I& aMoveVector,
                                             const EDA_ANGLE& aRotation,
                                             const double& aScalingFactor,
                                             const VECTOR2I& aTransformCentre,
                                             const bool& aMirrorInvert )
{
    loadShapeVertices( aCadstarFigure.Shape.Vertices, aCadstarFigure.LineCodeID,
                       aCadstarSheetIDOverride, aKiCadSchLayerID, aMoveVector, aRotation,
                       aScalingFactor, aTransformCentre, aMirrorInvert );

    for( const CUTOUT& cutout : aCadstarFigure.Shape.Cutouts )
    {
        loadShapeVertices( cutout.Vertices, aCadstarFigure.LineCodeID, aCadstarSheetIDOverride,
                           aKiCadSchLayerID, aMoveVector, aRotation, aScalingFactor,
                           aTransformCentre, aMirrorInvert );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheetAndChildSheets( const LAYER_ID&       aCadstarSheetID,
                                                          const VECTOR2I&       aPosition,
                                                          const VECTOR2I&       aSheetSize,
                                                          const SCH_SHEET_PATH& aParentSheet )
{
    wxCHECK_MSG( m_sheetMap.find( aCadstarSheetID ) == m_sheetMap.end(), ,
                 "Sheet already loaded!" );

    SCH_SHEET*     sheet = new SCH_SHEET(
        /* aParent */ aParentSheet.Last(),
        /* aPosition */ aPosition,
        /* aSize */ VECTOR2I( aSheetSize ) );
    SCH_SCREEN*    screen = new SCH_SCREEN( m_schematic );
    SCH_SHEET_PATH instance( aParentSheet );

    sheet->SetScreen( screen );

    wxString name = Sheets.SheetNames.at( aCadstarSheetID );

    sheet->GetField( FIELD_T::SHEET_NAME )->SetText( name );

    int         sheetNum = getSheetNumber( aCadstarSheetID );
    wxString    loadedFilename = wxFileName( Filename ).GetName();
    std::string filename = wxString::Format( "%s_%02d", loadedFilename, sheetNum ).ToStdString();

    ReplaceIllegalFileNameChars( filename );
    filename += wxT( "." ) + wxString( FILEEXT::KiCadSchematicFileExtension );

    sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( filename );

    wxFileName fn( m_schematic->Project().GetProjectPath() + filename );
    sheet->GetScreen()->SetFileName( fn.GetFullPath() );
    aParentSheet.Last()->GetScreen()->Append( sheet );
    instance.push_back( sheet );

    wxString pageNumStr = wxString::Format( "%d", getSheetNumber( aCadstarSheetID ) );
    instance.SetPageNumber( pageNumStr );

    sheet->AutoplaceFields( nullptr, AUTOPLACE_AUTO );

    m_sheetMap.insert( { aCadstarSheetID, sheet } );

    loadChildSheets( aCadstarSheetID, instance );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadChildSheets( const LAYER_ID& aCadstarSheetID,
                                                  const SCH_SHEET_PATH& aSheet )
{
    wxCHECK_MSG( m_sheetMap.find( aCadstarSheetID ) != m_sheetMap.end(), ,
                 "FIXME! Parent sheet should be loaded before attempting to load subsheets" );

    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK& block = blockPair.second;

        if( block.LayerID == aCadstarSheetID && block.Type == BLOCK::TYPE::CHILD )
        {
            if( block.AssocLayerID == wxT( "NO_LINK" ) )
            {
                if( block.Figures.size() > 0 )
                {
                    m_reporter->Report( wxString::Format( _( "The block ID %s (Block name: '%s') "
                                                             "is drawn on sheet '%s' but is not "
                                                             "linked to another sheet in the "
                                                             "design. KiCad requires all sheet "
                                                             "symbols to be associated to a sheet, "
                                                             "so the block was not loaded." ),
                                                          block.ID, block.Name,
                                                          Sheets.SheetNames.at( aCadstarSheetID ) ),
                                        RPT_SEVERITY_ERROR );
                }

                continue;
            }

            // In KiCad you can only draw rectangular shapes whereas in Cadstar arbitrary shapes
            // are allowed. We will calculate the extents of the Cadstar shape and draw a rectangle

            std::pair<VECTOR2I, VECTOR2I> blockExtents;

            if( block.Figures.size() > 0 )
            {
                blockExtents = getFigureExtentsKiCad( block.Figures.begin()->second );
            }
            else
            {
                THROW_IO_ERROR( wxString::Format( _( "The CADSTAR schematic might be corrupt: "
                                                     "Block %s references a child sheet but has no "
                                                     "Figure defined." ),
                                                  block.ID ) );
            }

            loadSheetAndChildSheets( block.AssocLayerID, blockExtents.first, blockExtents.second,
                                     aSheet );

            // Hide all KiCad sheet properties (sheet name/filename is not applicable in CADSTAR)
            SCH_SHEET* loadedSheet = m_sheetMap.at( block.AssocLayerID );
            SCH_FIELDS fields = loadedSheet->GetFields();

            for( SCH_FIELD& field : fields )
            {
                field.SetVisible( false );
            }

            if( block.HasBlockLabel )
            {
                //@todo use below code when KiCad supports multi-line fields
                /*
                // Add the block label as a separate field
                SCH_FIELD blockNameField( getKiCadPoint( block.BlockLabel.Position ), 2,
                        loadedSheet, wxString( "Block name" ) );
                blockNameField.SetText( block.Name );
                blockNameField.SetVisible( true );

                applyTextSettings( &blockNameField,
                                    block.BlockLabel.TextCodeID,
                                    block.BlockLabel.Alignment,
                                    block.BlockLabel.Justification,
                                    block.BlockLabel.OrientAngle,
                                    block.BlockLabel.Mirror );

                fields.push_back( blockNameField );*/

                // For now as as a text item (supports multi-line properly)
                SCH_TEXT* kiTxt = new SCH_TEXT();

                kiTxt->SetParent( m_schematic );
                kiTxt->SetPosition( getKiCadPoint( block.BlockLabel.Position ) );
                kiTxt->SetText( block.Name );

                applyTextSettings( kiTxt, block.BlockLabel.TextCodeID, block.BlockLabel.Alignment,
                                   block.BlockLabel.Justification, block.BlockLabel.OrientAngle,
                                   block.BlockLabel.Mirror );

                loadItemOntoKiCadSheet( aCadstarSheetID, kiTxt );
            }

            loadedSheet->SetFields( fields );
        }
    }
}


std::vector<CADSTAR_SCH_ARCHIVE_LOADER::LAYER_ID> CADSTAR_SCH_ARCHIVE_LOADER::findOrphanSheets()
{
    std::vector<LAYER_ID> childSheets, orphanSheets;

    //Find all sheets that are child of another
    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK&    block        = blockPair.second;
        LAYER_ID& assocSheetID = block.AssocLayerID;

        if( block.Type == BLOCK::TYPE::CHILD )
            childSheets.push_back( assocSheetID );
    }

    //Add sheets that do not have a parent
    for( const LAYER_ID& sheetID : Sheets.SheetOrder )
    {
        if( std::find( childSheets.begin(), childSheets.end(), sheetID ) == childSheets.end() )
            orphanSheets.push_back( sheetID );
    }

    return orphanSheets;
}


int CADSTAR_SCH_ARCHIVE_LOADER::getSheetNumber( const LAYER_ID& aCadstarSheetID )
{
    int i = 1;

    for( const LAYER_ID& sheetID : Sheets.SheetOrder )
    {
        if( sheetID == aCadstarSheetID )
            return i;

        ++i;
    }

    return -1;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadItemOntoKiCadSheet( const LAYER_ID& aCadstarSheetID,
                                                         SCH_ITEM* aItem )
{
    wxCHECK_MSG( aItem, /*void*/, wxT( "aItem is null" ) );

    if( aCadstarSheetID == "ALL_SHEETS" )
    {
        SCH_ITEM* duplicateItem = nullptr;

        for( std::pair<LAYER_ID, SHEET_NAME> sheetPair : Sheets.SheetNames )
        {
            LAYER_ID sheetID = sheetPair.first;
            duplicateItem    = aItem->Duplicate( IGNORE_PARENT_GROUP );
            m_sheetMap.at( sheetID )->GetScreen()->Append( aItem->Duplicate( IGNORE_PARENT_GROUP ) );
        }

        //Get rid of the extra copy:
        delete aItem;
        aItem = duplicateItem;
    }
    else if( aCadstarSheetID == "NO_SHEET" )
    {
        wxFAIL_MSG( wxT( "Trying to add an item to NO_SHEET? This might be a documentation symbol." ) );
    }
    else
    {
        if( m_sheetMap.find( aCadstarSheetID ) != m_sheetMap.end() )
        {
            m_sheetMap.at( aCadstarSheetID )->GetScreen()->Append( aItem );
        }
        else
        {
            delete aItem;
            wxFAIL_MSG( wxT( "Unknown Sheet ID." ) );
        }
    }
}


CADSTAR_SCH_ARCHIVE_LOADER::SYMDEF_ID
CADSTAR_SCH_ARCHIVE_LOADER::getSymDefFromName( const wxString& aSymdefName,
                                               const wxString& aSymDefAlternate )
{
    if( m_SymDefNamesCache.size() != Library.SymbolDefinitions.size() )
    {
        // Re-initialise
        m_SymDefNamesCache.clear();
        m_DefaultSymDefNamesCache.clear();

        // Create a lower case cache to avoid searching each time
        for( auto& [id, symdef] : Library.SymbolDefinitions )
        {
            wxString refKey = symdef.ReferenceName.Lower();
            wxString altKey = symdef.Alternate.Lower();

            m_SymDefNamesCache[{ refKey, altKey }] = id;

            // Secondary cache to find symbols just by the Name (e.g. if the alternate
            // does not exist, we still want to return a symbo - the same behaviour
            // as CADSTAR

            if( !m_DefaultSymDefNamesCache.count( refKey ) )
            {
                m_DefaultSymDefNamesCache.insert( { refKey, id } );
            }
            else if( altKey.IsEmpty() )
            {
                // Always use the empty alternate if it exists
                m_DefaultSymDefNamesCache[refKey] = id;
            }
        }
    }

    wxString refKeyToFind = aSymdefName.Lower();
    wxString altKeyToFind = aSymDefAlternate.Lower();

    if( m_SymDefNamesCache.count( { refKeyToFind, altKeyToFind } ) )
    {
        return m_SymDefNamesCache[{ refKeyToFind, altKeyToFind }];
    }
    else if( m_DefaultSymDefNamesCache.count( refKeyToFind ) )
    {
        return m_DefaultSymDefNamesCache[refKeyToFind];
    }

    return SYMDEF_ID();
}


bool CADSTAR_SCH_ARCHIVE_LOADER::isAttributeVisible( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    // Use CADSTAR visibility settings to determine if an attribute is visible
    if( AttrColors.AttributeColors.find( aCadstarAttributeID ) != AttrColors.AttributeColors.end() )
        return AttrColors.AttributeColors.at( aCadstarAttributeID ).IsVisible;

    return false; // If there is no visibility setting, assume not displayed
}


int CADSTAR_SCH_ARCHIVE_LOADER::getLineThickness( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
             schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS ) );

    return getKiCadLength( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Width );
}


LINE_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getLineStyle( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
             LINE_STYLE::SOLID );

    // clang-format off
    switch( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Style )
    {
    case LINESTYLE::DASH:       return LINE_STYLE::DASH;
    case LINESTYLE::DASHDOT:    return LINE_STYLE::DASHDOT;
    case LINESTYLE::DASHDOTDOT: return LINE_STYLE::DASHDOT; //TODO: update in future
    case LINESTYLE::DOT:        return LINE_STYLE::DOT;
    case LINESTYLE::SOLID:      return LINE_STYLE::SOLID;
    default:                    return LINE_STYLE::DEFAULT;
    }
    // clang-format on
}


CADSTAR_SCH_ARCHIVE_LOADER::TEXTCODE
CADSTAR_SCH_ARCHIVE_LOADER::getTextCode( const TEXTCODE_ID& aCadstarTextCodeID )
{
    wxCHECK( Assignments.Codedefs.TextCodes.find( aCadstarTextCodeID )
                     != Assignments.Codedefs.TextCodes.end(),
             TEXTCODE() );

    return Assignments.Codedefs.TextCodes.at( aCadstarTextCodeID );
}


int CADSTAR_SCH_ARCHIVE_LOADER::getTextHeightFromTextCode( const TEXTCODE_ID& aCadstarTextCodeID )
{
    TEXTCODE txtCode = getTextCode( aCadstarTextCodeID );

    return KiROUND( (double) getKiCadLength( txtCode.Height ) * TXT_HEIGHT_RATIO );
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    wxCHECK( Assignments.Codedefs.AttributeNames.find( aCadstarAttributeID )
                     != Assignments.Codedefs.AttributeNames.end(),
             aCadstarAttributeID );

    return Assignments.Codedefs.AttributeNames.at( aCadstarAttributeID ).Name;
}


CADSTAR_SCH_ARCHIVE_LOADER::PART
CADSTAR_SCH_ARCHIVE_LOADER::getPart( const PART_ID& aCadstarPartID )
{
    wxCHECK( Parts.PartDefinitions.find( aCadstarPartID ) != Parts.PartDefinitions.end(), PART() );

    return Parts.PartDefinitions.at( aCadstarPartID );
}


CADSTAR_SCH_ARCHIVE_LOADER::ROUTECODE
CADSTAR_SCH_ARCHIVE_LOADER::getRouteCode( const ROUTECODE_ID& aCadstarRouteCodeID )
{
    wxCHECK( Assignments.Codedefs.RouteCodes.find( aCadstarRouteCodeID )
                     != Assignments.Codedefs.RouteCodes.end(),
             ROUTECODE() );

    return Assignments.Codedefs.RouteCodes.at( aCadstarRouteCodeID );
}


CADSTAR_SCH_ARCHIVE_LOADER::PART::DEFINITION::PIN
CADSTAR_SCH_ARCHIVE_LOADER::getPartDefinitionPin( const PART& aCadstarPart, const GATE_ID& aGateID,
                                                  const TERMINAL_ID& aTerminalID )
{
    for( std::pair<PART_DEFINITION_PIN_ID, PART::DEFINITION::PIN> pinPair :
         aCadstarPart.Definition.Pins )
    {
        PART::DEFINITION::PIN partPin = pinPair.second;

        if( partPin.TerminalGate == aGateID && partPin.TerminalPin == aTerminalID )
            return partPin;
    }

    return PART::DEFINITION::PIN();
}


ELECTRICAL_PINTYPE CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPinType( const CADSTAR_PIN_TYPE& aPinType )
{
    switch( aPinType )
    {
    case CADSTAR_PIN_TYPE::UNCOMMITTED:        return ELECTRICAL_PINTYPE::PT_PASSIVE;
    case CADSTAR_PIN_TYPE::PIN_INPUT:              return ELECTRICAL_PINTYPE::PT_INPUT;
    case CADSTAR_PIN_TYPE::OUTPUT_OR:          return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case CADSTAR_PIN_TYPE::OUTPUT_NOT_OR:      return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case CADSTAR_PIN_TYPE::OUTPUT_NOT_NORM_OR: return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case CADSTAR_PIN_TYPE::POWER:              return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case CADSTAR_PIN_TYPE::GROUND:             return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case CADSTAR_PIN_TYPE::TRISTATE_BIDIR:     return ELECTRICAL_PINTYPE::PT_BIDI;
    case CADSTAR_PIN_TYPE::TRISTATE_INPUT:     return ELECTRICAL_PINTYPE::PT_INPUT;
    case CADSTAR_PIN_TYPE::TRISTATE_DRIVER:    return ELECTRICAL_PINTYPE::PT_OUTPUT;
    }

    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}

int CADSTAR_SCH_ARCHIVE_LOADER::getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID )
{
    if( aCadstarGateID.IsEmpty() )
        return 1;

    return (int) aCadstarGateID.Upper().GetChar( 0 ) - (int) wxUniChar( 'A' ) + 1;
}


SPIN_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getSpinStyle( const long long& aCadstarOrientation,
                                                     bool aMirror )
{
    EDA_ANGLE  orientation = getAngle( aCadstarOrientation );
    SPIN_STYLE spinStyle   = getSpinStyle( orientation );

    if( aMirror )
    {
        spinStyle = spinStyle.RotateCCW();
        spinStyle = spinStyle.RotateCCW();
    }

    return spinStyle;
}


SPIN_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getSpinStyle( const EDA_ANGLE& aOrientation )
{
    SPIN_STYLE spinStyle = SPIN_STYLE::LEFT;

    EDA_ANGLE oDeg = aOrientation;
    oDeg.Normalize180();

    if( oDeg >= -ANGLE_45 && oDeg <= ANGLE_45 )
        spinStyle = SPIN_STYLE::RIGHT;                  // 0deg
    else if( oDeg >= ANGLE_45 && oDeg <= ANGLE_135 )
        spinStyle = SPIN_STYLE::UP;                     // 90deg
    else if( oDeg >= ANGLE_135 || oDeg <= -ANGLE_135 )
        spinStyle = SPIN_STYLE::LEFT;                   // 180deg
    else
        spinStyle = SPIN_STYLE::BOTTOM;                 // 270deg

    return spinStyle;
}


CADSTAR_SCH_ARCHIVE_LOADER::ALIGNMENT
CADSTAR_SCH_ARCHIVE_LOADER::mirrorX( const ALIGNMENT& aCadstarAlignment )
{
    switch( aCadstarAlignment )
    {
    // Change left to right:
    case ALIGNMENT::NO_ALIGNMENT:
    case ALIGNMENT::BOTTOMLEFT:    return ALIGNMENT::BOTTOMRIGHT;
    case ALIGNMENT::CENTERLEFT:    return ALIGNMENT::CENTERRIGHT;
    case ALIGNMENT::TOPLEFT:       return ALIGNMENT::TOPRIGHT;

    //Change right to left:
    case ALIGNMENT::BOTTOMRIGHT:   return ALIGNMENT::BOTTOMLEFT;
    case ALIGNMENT::CENTERRIGHT:   return ALIGNMENT::CENTERLEFT;
    case ALIGNMENT::TOPRIGHT:      return ALIGNMENT::TOPLEFT;

    // Center alignment does not mirror:
    case ALIGNMENT::BOTTOMCENTER:
    case ALIGNMENT::CENTERCENTER:
    case ALIGNMENT::TOPCENTER:     return aCadstarAlignment;

    // Shouldn't be here
    default: wxFAIL_MSG( "Unknown Cadstar Alignment" ); return aCadstarAlignment;
    }
}


CADSTAR_SCH_ARCHIVE_LOADER::ALIGNMENT
CADSTAR_SCH_ARCHIVE_LOADER::rotate180( const ALIGNMENT& aCadstarAlignment )
{
    switch( aCadstarAlignment )
    {
    case ALIGNMENT::NO_ALIGNMENT:
    case ALIGNMENT::BOTTOMLEFT:    return ALIGNMENT::TOPRIGHT;
    case ALIGNMENT::BOTTOMCENTER:  return ALIGNMENT::TOPCENTER;
    case ALIGNMENT::BOTTOMRIGHT:   return ALIGNMENT::TOPLEFT;
    case ALIGNMENT::TOPLEFT:       return ALIGNMENT::BOTTOMRIGHT;
    case ALIGNMENT::TOPCENTER:     return ALIGNMENT::BOTTOMCENTER;
    case ALIGNMENT::TOPRIGHT:      return ALIGNMENT::BOTTOMLEFT;
    case ALIGNMENT::CENTERLEFT:    return ALIGNMENT::CENTERRIGHT;
    case ALIGNMENT::CENTERCENTER:  return ALIGNMENT::CENTERCENTER;
    case ALIGNMENT::CENTERRIGHT:   return ALIGNMENT::CENTERLEFT;

    // Shouldn't be here
    default: wxFAIL_MSG( "Unknown Cadstar Alignment" ); return aCadstarAlignment;
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyTextCodeIfExists( EDA_TEXT*          aKiCadTextItem,
                                                        const TEXTCODE_ID& aCadstarTextCodeID )
{
    // Ensure we have no Cadstar overbar characters
    wxString escapedText = HandleTextOverbar( aKiCadTextItem->GetText() );
    aKiCadTextItem->SetText( escapedText );

    if( !Assignments.Codedefs.TextCodes.count( aCadstarTextCodeID ) )
        return;

    TEXTCODE textCode = getTextCode( aCadstarTextCodeID );
    int      textHeight = KiROUND( (double) getKiCadLength( textCode.Height ) * TXT_HEIGHT_RATIO );
    int      textWidth = getKiCadLength( textCode.Width );

    // The width is zero for all non-cadstar fonts. Using a width equal to 2/3 the height seems
    // to work well for most fonts.
    if( textWidth == 0 )
        textWidth = getKiCadLength( 2LL * textCode.Height / 3LL );

    aKiCadTextItem->SetTextWidth( textWidth );
    aKiCadTextItem->SetTextHeight( textHeight );

#if 0
    // EEschema currently supports only normal vs bold for text thickness.
    aKiCadTextItem->SetTextThickness( getKiCadLength( textCode.LineWidth ) );
#endif

    // Must come after SetTextSize()
    aKiCadTextItem->SetBold( textCode.Font.Modifier1 == FONT_BOLD );
    aKiCadTextItem->SetItalic( textCode.Font.Italic );
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyTextSettings( EDA_TEXT*            aKiCadTextItem,
                                                    const TEXTCODE_ID&   aCadstarTextCodeID,
                                                    const ALIGNMENT&     aCadstarAlignment,
                                                    const JUSTIFICATION& aCadstarJustification,
                                                    const long long      aCadstarOrientAngle,
                                                    bool                 aMirrored )
{
    applyTextCodeIfExists( aKiCadTextItem, aCadstarTextCodeID );
    aKiCadTextItem->SetTextAngle( getAngle( aCadstarOrientAngle ) );

    // Justification ignored for now as not supported in Eeschema, but leaving this code in
    // place for future upgrades.
    // TODO update this when Eeschema supports justification independent of anchor position.
    ALIGNMENT textAlignment = aCadstarAlignment;

    // KiCad mirrors the justification and alignment when the symbol is mirrored but CADSTAR
    // specifies it post-mirroring. In contrast, if the text item itself is mirrored (not
    // supported in KiCad), CADSTAR specifies the alignment and justification pre-mirroring
    if( aMirrored )
        textAlignment = mirrorX( aCadstarAlignment );

    auto setAlignment =
            [&]( EDA_TEXT* aText, ALIGNMENT aAlignment )
            {
                switch( aAlignment )
                {
                case ALIGNMENT::NO_ALIGNMENT: // Bottom left of the first line
                    //No exact KiCad equivalent, so lets move the position of the text
                    FixTextPositionNoAlignment( aText );
                    KI_FALLTHROUGH;
                case ALIGNMENT::BOTTOMLEFT:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                    break;

                case ALIGNMENT::BOTTOMCENTER:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                    break;

                case ALIGNMENT::BOTTOMRIGHT:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                    break;

                case ALIGNMENT::CENTERLEFT:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                    break;

                case ALIGNMENT::CENTERCENTER:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                    break;

                case ALIGNMENT::CENTERRIGHT:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                    break;

                case ALIGNMENT::TOPLEFT:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                    break;

                case ALIGNMENT::TOPCENTER:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                    break;

                case ALIGNMENT::TOPRIGHT:
                    aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                    aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
                    break;
                }
            };

    SPIN_STYLE spin = getSpinStyle( aCadstarOrientAngle, aMirrored );
    EDA_ITEM*  textEdaItem = dynamic_cast<EDA_ITEM*>( aKiCadTextItem );
    wxCHECK( textEdaItem, /* void */ ); // ensure this is a EDA_ITEM

    if( textEdaItem->Type() == SCH_FIELD_T )
    {
        // Spin style not used. All text justifications are permitted. However, only orientations
        // of 0 deg or 90 deg are supported
        EDA_ANGLE angle = aKiCadTextItem->GetTextAngle();
        angle.Normalize();

        int quadrant = KiROUND( angle.AsDegrees() / 90.0 );
        quadrant %= 4;

        switch( quadrant )
        {
        case 0:
            angle = ANGLE_HORIZONTAL;
            break;
        case 1:
            angle = ANGLE_VERTICAL;
            break;
        case 2:
            angle = ANGLE_HORIZONTAL;
            textAlignment = rotate180( textAlignment );
            break;
        case 3:
            angle = ANGLE_VERTICAL;
            textAlignment = rotate180( textAlignment );
            break;
        default:
            wxFAIL_MSG( "Unknown Quadrant" );
        }

        aKiCadTextItem->SetTextAngle( angle );
        setAlignment( aKiCadTextItem, textAlignment );
    }
    else if( textEdaItem->Type() == SCH_TEXT_T )
    {
        // Note spin style in a SCH_TEXT results in a vertical alignment GR_TEXT_V_ALIGN_BOTTOM
        // so need to adjust the location of the text element based on Cadstar's original text
        // alignment (anchor position).
        setAlignment( aKiCadTextItem, textAlignment );
        BOX2I    bb = textEdaItem->GetBoundingBox();
        int      off = static_cast<SCH_TEXT*>( aKiCadTextItem )->GetTextOffset();
        VECTOR2I pos;

        // Change the anchor point of the text item to make it match the same bounding box
        // And correct the error introduced by the text offsetting in KiCad
        switch( spin )
        {
        case SPIN_STYLE::BOTTOM: pos = { bb.GetRight() - off, bb.GetTop()          }; break;
        case SPIN_STYLE::UP:     pos = { bb.GetRight() - off, bb.GetBottom()       }; break;
        case SPIN_STYLE::LEFT:   pos = { bb.GetRight()      , bb.GetBottom() + off }; break;
        case SPIN_STYLE::RIGHT:  pos = { bb.GetLeft()       , bb.GetBottom() + off }; break;
        default:                 wxFAIL_MSG( "Unexpected Spin Style" );               break;
        }

        aKiCadTextItem->SetTextPos( pos );

        switch( spin )
        {
        case SPIN_STYLE::RIGHT:            // Horiz Normal Orientation
            aKiCadTextItem->SetTextAngle( ANGLE_HORIZONTAL );
            aKiCadTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;

        case SPIN_STYLE::UP:               // Vert Orientation UP
            aKiCadTextItem->SetTextAngle( ANGLE_VERTICAL );
            aKiCadTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            break;

        case SPIN_STYLE::LEFT:             // Horiz Orientation - Right justified
            aKiCadTextItem->SetTextAngle( ANGLE_HORIZONTAL );
            aKiCadTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;

        case SPIN_STYLE::BOTTOM:           //  Vert Orientation BOTTOM
            aKiCadTextItem->SetTextAngle( ANGLE_VERTICAL );
            aKiCadTextItem->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
            break;

        default:
            wxFAIL_MSG( "Unexpected Spin Style" );
            break;
        }

        aKiCadTextItem->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    }
    else if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( aKiCadTextItem ) )
    {
        // We don't want to change position of net labels as that would break connectivity
        label->SetSpinStyle( spin );
    }
    else
    {
        wxFAIL_MSG( "Unexpected item type" );
    }
}


SCH_TEXT* CADSTAR_SCH_ARCHIVE_LOADER::getKiCadSchText( const TEXT& aCadstarTextElement )
{
    SCH_TEXT* kiTxt = new SCH_TEXT();

    kiTxt->SetParent( m_schematic ); // set to the schematic for now to avoid asserts
    kiTxt->SetPosition( getKiCadPoint( aCadstarTextElement.Position ) );
    kiTxt->SetText( aCadstarTextElement.Text );

    applyTextSettings( kiTxt, aCadstarTextElement.TextCodeID, aCadstarTextElement.Alignment,
                       aCadstarTextElement.Justification, aCadstarTextElement.OrientAngle,
                       aCadstarTextElement.Mirror );

    return kiTxt;
}


LIB_SYMBOL* CADSTAR_SCH_ARCHIVE_LOADER::getScaledLibPart( const LIB_SYMBOL* aSymbol,
                                                          long long aScalingFactorNumerator,
                                                          long long aScalingFactorDenominator )
{
    LIB_SYMBOL* retval = new LIB_SYMBOL( *aSymbol );

    if( aScalingFactorNumerator == aScalingFactorDenominator )
        return retval; // 1:1 scale, nothing to do

    auto scaleLen =
        [&]( int aLength ) -> int
        {
            return( aLength * aScalingFactorNumerator ) / aScalingFactorDenominator;
        };

    auto scalePt =
        [&]( VECTOR2I aCoord ) -> VECTOR2I
        {
            return VECTOR2I( scaleLen( aCoord.x ), scaleLen( aCoord.y ) );
        };

    auto scaleSize =
        [&]( VECTOR2I aSize ) -> VECTOR2I
        {
            return VECTOR2I( scaleLen( aSize.x ), scaleLen( aSize.y ) );
        };

    LIB_ITEMS_CONTAINER& items = retval->GetDrawItems();

    for( SCH_ITEM& item : items )
    {
        switch( item.Type() )
        {
        case KICAD_T::SCH_SHAPE_T:
        {
            SCH_SHAPE& shape = static_cast<SCH_SHAPE&>( item );

            if( shape.GetShape() == SHAPE_T::ARC )
            {
                shape.SetPosition( scalePt( shape.GetPosition() ) );
                shape.SetStart( scalePt( shape.GetStart() ) );
                shape.SetEnd( scalePt( shape.GetEnd() ) );
            }
            else if( shape.GetShape() == SHAPE_T::POLY )
            {
                SHAPE_LINE_CHAIN& poly = shape.GetPolyShape().Outline( 0 );

                for( size_t ii = 0; ii < poly.GetPointCount(); ++ii )
                    poly.SetPoint( ii, scalePt( poly.CPoint( ii ) ) );
            }
            break;
        }

        case KICAD_T::SCH_PIN_T:
        {
            SCH_PIN& pin = static_cast<SCH_PIN&>( item );

            pin.SetPosition( scalePt( pin.GetPosition() ) );
            pin.SetLength( scaleLen( pin.GetLength() ) );
            break;
        }

        case KICAD_T::SCH_TEXT_T:
        {
            SCH_TEXT& txt = static_cast<SCH_TEXT&>( item );

            txt.SetPosition( scalePt( txt.GetPosition() ) );
            txt.SetTextSize( scaleSize( txt.GetTextSize() ) );
            break;
        }

        default:
            break;
        }
    }

    return retval;
}


void CADSTAR_SCH_ARCHIVE_LOADER::fixUpLibraryPins( LIB_SYMBOL* aSymbolToFix, int aGateNumber )
{
    auto compLambda =
            []( const VECTOR2I& aA, const VECTOR2I& aB )
            {
                return LexicographicalCompare( aA, aB ) < 0;
            };

    // Store a list of vertical or horizontal segments in the symbol
    // Note: Need the custom comparison function to ensure the map is sorted correctly
    std::map<VECTOR2I, SHAPE_LINE_CHAIN, decltype( compLambda )> uniqueSegments( compLambda );

    LIB_ITEMS_CONTAINER::ITERATOR shapeIt = aSymbolToFix->GetDrawItems().begin( SCH_SHAPE_T );

    for( ; shapeIt != aSymbolToFix->GetDrawItems().end( SCH_SHAPE_T ); ++shapeIt )
    {
        SCH_SHAPE& shape = static_cast<SCH_SHAPE&>( *shapeIt );

        if( aGateNumber > 0 && shape.GetUnit() != aGateNumber )
            continue;

        if( shape.GetShape() != SHAPE_T::POLY )
            continue;

        SHAPE_LINE_CHAIN poly = shape.GetPolyShape().Outline( 0 );

        if( poly.GetPointCount() == 2 )
        {
            VECTOR2I pt0 = poly.CPoint( 0 );
            VECTOR2I pt1 = poly.CPoint( 1 );

            if( pt0 != pt1 && uniqueSegments.count( pt0 ) == 0 && uniqueSegments.count( pt1 ) == 0 )
            {
                // we are only interested in vertical or horizontal segments
                if( pt0.x == pt1.x || pt0.y == pt1.y )
                {
                    uniqueSegments.insert( { pt0, poly } );
                    uniqueSegments.insert( { pt1, poly } );
                }
            }
        }
    }

    for( SCH_PIN* pin : aSymbolToFix->GetGraphicalPins( aGateNumber, 0 ) )
    {
        auto setPinOrientation =
                [&]( const EDA_ANGLE& aAngle )
                {
                    EDA_ANGLE angle( aAngle );
                    angle.Normalize180();

                    if( angle >= -ANGLE_45 && angle <= ANGLE_45 )
                        pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT ); // 0 degrees
                    else if( angle >= ANGLE_45 && angle <= ANGLE_135 )
                        pin->SetOrientation( PIN_ORIENTATION::PIN_UP ); // 90 degrees
                    else if( angle >= ANGLE_135 || angle <= -ANGLE_135 )
                        pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT ); // 180 degrees
                    else
                        pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN ); // -90 degrees
                };

        if( uniqueSegments.count( pin->GetPosition() ) )
        {
            SHAPE_LINE_CHAIN& poly = uniqueSegments.at( pin->GetPosition() );

            VECTOR2I otherPt = poly.CPoint( 0 );

            if( otherPt == pin->GetPosition() )
                otherPt = poly.CPoint( 1 );

            VECTOR2I vec( otherPt - pin->GetPosition() );

            pin->SetLength( vec.EuclideanNorm() );
            setPinOrientation( EDA_ANGLE( vec ) );
        }
    }
}


std::pair<VECTOR2I, VECTOR2I>
CADSTAR_SCH_ARCHIVE_LOADER::getFigureExtentsKiCad( const FIGURE& aCadstarFigure )
{
    VECTOR2I upperLeft( Assignments.Settings.DesignLimit.x, 0 );
    VECTOR2I lowerRight( 0, Assignments.Settings.DesignLimit.y );

    for( const VERTEX& v : aCadstarFigure.Shape.Vertices )
    {
        if( upperLeft.x > v.End.x )
            upperLeft.x = v.End.x;

        if( upperLeft.y < v.End.y )
            upperLeft.y = v.End.y;

        if( lowerRight.x < v.End.x )
            lowerRight.x = v.End.x;

        if( lowerRight.y > v.End.y )
            lowerRight.y = v.End.y;
    }

    for( CUTOUT cutout : aCadstarFigure.Shape.Cutouts )
    {
        for( const VERTEX& v : aCadstarFigure.Shape.Vertices )
        {
            if( upperLeft.x > v.End.x )
                upperLeft.x = v.End.x;

            if( upperLeft.y < v.End.y )
                upperLeft.y = v.End.y;

            if( lowerRight.x < v.End.x )
                lowerRight.x = v.End.x;

            if( lowerRight.y > v.End.y )
                lowerRight.y = v.End.y;
        }
    }

    VECTOR2I upperLeftKiCad = getKiCadPoint( upperLeft );
    VECTOR2I lowerRightKiCad = getKiCadPoint( lowerRight );

    VECTOR2I size = lowerRightKiCad - upperLeftKiCad;

    return { upperLeftKiCad, VECTOR2I( abs( size.x ), abs( size.y ) ) };
}


VECTOR2I CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPoint( const VECTOR2I& aCadstarPoint )
{
    VECTOR2I retval;

    retval.x = getKiCadLength( aCadstarPoint.x - m_designCenter.x );
    retval.y = -getKiCadLength( aCadstarPoint.y - m_designCenter.y );

    return retval;
}


VECTOR2I CADSTAR_SCH_ARCHIVE_LOADER::getKiCadLibraryPoint( const VECTOR2I& aCadstarPoint,
                                                           const VECTOR2I& aCadstarCentre )
{
    VECTOR2I retval;

    retval.x = getKiCadLength( aCadstarPoint.x - aCadstarCentre.x );
    retval.y = -getKiCadLength( aCadstarPoint.y - aCadstarCentre.y );

    return retval;
}


VECTOR2I CADSTAR_SCH_ARCHIVE_LOADER::applyTransform( const VECTOR2I& aPoint,
                                                     const VECTOR2I& aMoveVector,
                                                     const EDA_ANGLE& aRotation,
                                                     const double& aScalingFactor,
                                                     const VECTOR2I& aTransformCentre,
                                                     const bool& aMirrorInvert )
{
    VECTOR2I retVal = aPoint;

    if( aScalingFactor != 1.0 )
    {
        //scale point
        retVal -= aTransformCentre;
        retVal.x = KiROUND( retVal.x * aScalingFactor );
        retVal.y = KiROUND( retVal.y * aScalingFactor );
        retVal += aTransformCentre;
    }

    if( aMirrorInvert )
        MIRROR( retVal.x, aTransformCentre.x );

    if( !aRotation.IsZero() )
        RotatePoint( retVal, aTransformCentre, aRotation );

    if( aMoveVector != VECTOR2I{ 0, 0 } )
        retVal += aMoveVector;

    return retVal;
}


double CADSTAR_SCH_ARCHIVE_LOADER::getPolarRadius( const VECTOR2I& aPoint )
{
    return sqrt( ( (double) aPoint.x * (double) aPoint.x )
                 + ( (double) aPoint.y * (double) aPoint.y ) );
}
