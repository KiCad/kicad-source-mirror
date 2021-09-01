/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_plugins/cadstar/cadstar_sch_archive_loader.h>

#include <bus_alias.h>
#include <core/mirror.h>
#include <eda_text.h>
#include <lib_arc.h>
#include <lib_polyline.h>
#include <lib_text.h>
#include <macros.h>
#include <string_utils.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h> //SYMBOL_ORIENTATION_T
#include <sch_io_mgr.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <schematic.h>
#include <trigo.h>
#include <wildcards_and_files_ext.h>


const wxString PartNameFieldName = "Part Name";


void CADSTAR_SCH_ARCHIVE_LOADER::Load( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
        SCH_PLUGIN::SCH_PLUGIN_RELEASER* aSchPlugin, const wxFileName& aLibraryFileName )
{
    Parse();

    LONGPOINT designLimit = Assignments.Settings.DesignLimit;

    //Note: can't use getKiCadPoint() due wxPoint being int - need long long to make the check
    long long designSizeXkicad = (long long) designLimit.x / KiCadUnitDivider;
    long long designSizeYkicad = (long long) designLimit.y / KiCadUnitDivider;

    // Max size limited by the positive dimension of wxPoint (which is an int)
    constexpr long long maxDesignSizekicad = std::numeric_limits<int>::max();

    if( designSizeXkicad > maxDesignSizekicad || designSizeYkicad > maxDesignSizekicad )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "The design is too large and cannot be imported into KiCad. \n"
                   "Please reduce the maximum design size in CADSTAR by navigating to: \n"
                   "Design Tab -> Properties -> Design Options -> Maximum Design Size. \n"
                   "Current Design size: %.2f, %.2f millimeters. \n"
                   "Maximum permitted design size: %.2f, %.2f millimeters.\n" ),
                (double) designSizeXkicad / SCH_IU_PER_MM,
                (double) designSizeYkicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM ) );
    }

    // Assume the center at 0,0 since we are going to be translating the design afterwards anyway
    m_designCenter = { 0, 0 };

    m_schematic       = aSchematic;
    m_rootSheet       = aRootSheet;
    m_plugin          = aSchPlugin;
    m_libraryFileName = aLibraryFileName;

    loadTextVariables(); // Load text variables right at the start to ensure bounding box
                         // calculations work correctly for text items
    loadSheets();
    loadHierarchicalSheetPins();
    loadPartsLibrary();
    loadSchematicSymbolInstances();
    loadBusses();
    loadNets();
    loadFigures();
    loadTexts();
    loadDocumentationSymbols();

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
                            RPT_SEVERITY_WARNING  );
    }


    // For all sheets, center all elements and re calculate the page size:
    for( std::pair<LAYER_ID, SCH_SHEET*> sheetPair : m_sheetMap )
    {
        SCH_SHEET* sheet = sheetPair.second;

        // Calculate the new sheet size.
        EDA_RECT sheetBoundingBox;

        for( auto item : sheet->GetScreen()->Items() )
        {
            EDA_RECT bbox;

            // Only use the visible fields of the symbols to calculate their bounding box
            // (hidden fields could be very long and artificially enlarge the sheet bounding box)
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* comp = static_cast<SCH_SYMBOL*>( item );
                bbox = comp->GetBodyBoundingBox();

                for( const SCH_FIELD& field : comp->GetFields() )
                {
                    if( field.IsVisible() )
                        bbox.Merge( field.GetBoundingBox() );
                }
            }
            else
            {
                bbox = item->GetBoundingBox();
            }

            sheetBoundingBox.Merge( bbox );
        }

        // Find the working grid of the original CADSTAR design
        int grid = Assignments.Grids.WorkingGrid.Param1;

        if( Assignments.Grids.WorkingGrid.Type == GRID_TYPE::FRACTIONALGRID )
            grid = grid / Assignments.Grids.WorkingGrid.Param2;
        else if( Assignments.Grids.WorkingGrid.Param2 > grid )
            grid = Assignments.Grids.WorkingGrid.Param2;

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

        // When exporting to pdf, CADSTAR applies a margin of 3% of the longest dimension (height
        // or width) to all 4 sides (top, bottom, left right). For the import, we are also rounding
        // the margin to the nearest grid, ensuring all items remain on the grid.
        wxSize targetSheetSize = sheetBoundingBox.GetSize();
        int    longestSide = std::max( targetSheetSize.x, targetSheetSize.y );
        int    margin = ( (double) longestSide * 0.03);
        margin = roundToNearestGrid( margin );
        targetSheetSize.IncBy( margin * 2, margin * 2 );

        // Update page size always
        PAGE_INFO pageInfo   = sheet->GetScreen()->GetPageSettings();
        pageInfo.SetWidthMils( Iu2Mils( targetSheetSize.x ) );
        pageInfo.SetHeightMils( Iu2Mils( targetSheetSize.y ) );

        // Set the new sheet size.
        sheet->GetScreen()->SetPageSettings( pageInfo );

        wxSize  pageSizeIU = sheet->GetScreen()->GetPageSettings().GetSizeIU();
        wxPoint sheetcentre( pageSizeIU.x / 2, pageSizeIU.y / 2 );
        wxPoint itemsCentre = sheetBoundingBox.Centre();

        // round the translation to nearest point on the grid
        wxPoint translation = sheetcentre - itemsCentre;
        translation.x = roundToNearestGrid( translation.x );
        translation.y = roundToNearestGrid( translation.y );

        // Translate the items.
        std::vector<SCH_ITEM*> allItems;

        std::copy( sheet->GetScreen()->Items().begin(), sheet->GetScreen()->Items().end(),
                std::back_inserter( allItems ) );

        for( SCH_ITEM* item : allItems )
        {
            item->SetPosition( item->GetPosition() + translation );
            item->ClearFlags();
            sheet->GetScreen()->Update( item );
        }
    }

    m_reporter->Report( _( "The CADSTAR design has been imported successfully.\n"
                           "Please review the import errors and warnings (if any)." ) );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheets()
{
    const std::vector<LAYER_ID>& orphanSheets = findOrphanSheets();
    SCH_SHEET_PATH               rootPath;
    rootPath.push_back( m_rootSheet );
    m_rootSheet->AddInstance( rootPath.Path() );
    m_rootSheet->SetPageNumber( rootPath, wxT( "1" ) );

    if( orphanSheets.size() > 1 )
    {
        int x = 1;
        int y = 1;

        for( LAYER_ID sheetID : orphanSheets )
        {
            wxPoint pos( x * Mils2iu( 1000 ), y * Mils2iu( 1000 ) );
            wxSize  siz( Mils2iu( 1000 ), Mils2iu( 1000 ) );

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

        std::string filename = wxString::Format(
                "%s_%02d", loadedFilePath.GetName(), getSheetNumber( rootSheetID ) )
                                       .ToStdString();
        ReplaceIllegalFileNameChars( &filename );
        filename += wxT( "." ) + KiCadSchematicFileExtension;

        wxFileName fn( m_schematic->Prj().GetProjectPath() + filename );
        m_rootSheet->GetScreen()->SetFileName( fn.GetFullPath() );

        m_sheetMap.insert( { rootSheetID, m_rootSheet } );
        loadChildSheets( rootSheetID, rootPath );
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
                sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );
                sheetPin->SetLabelSpinStyle( getSpinStyle( term.OrientAngle, false ) );
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
        PART_ID partID  = partPair.first;
        PART    part = partPair.second;

        if( part.Definition.GateSymbols.size() == 0 )
            continue;

        wxString    escapedPartName = EscapeString( part.Name, CTX_LIBID );
        LIB_SYMBOL* kiPart = new LIB_SYMBOL( escapedPartName );

        kiPart->SetUnitCount( part.Definition.GateSymbols.size() );
        bool ok = true;

        for( std::pair<GATE_ID, PART::DEFINITION::GATE> gatePair : part.Definition.GateSymbols )
        {
            GATE_ID                gateID   = gatePair.first;
            PART::DEFINITION::GATE gate     = gatePair.second;
            SYMDEF_ID              symbolID = getSymDefFromName( gate.Name, gate.Alternate );
            m_partSymbolsMap.insert( { { partID, gateID }, symbolID } );

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

            loadSymDefIntoLibrary( symbolID, &part, gateID, kiPart );
        }

        if( ok )
        {
            ( *m_plugin )->SaveSymbol( m_libraryFileName.GetFullPath(), kiPart );

            LIB_SYMBOL* loadedPart =
                    ( *m_plugin )->LoadSymbol( m_libraryFileName.GetFullPath(), kiPart->GetName() );

            m_partMap.insert( { partID, loadedPart } );
        }
        else
        {
            // Don't save in the library, but still keep it cached as some of the units might have
            // been loaded correctly (saving us time later on)
            m_partMap.insert( { partID, kiPart } );
        }
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
                                    RPT_SEVERITY_ERROR);

                continue;
            }

            if( sym.GateID.IsEmpty() )
                sym.GateID = wxT( "A" ); // Assume Gate "A" if unspecified

            PART_GATE_ID partSymbolID = { sym.PartRef.RefID, sym.GateID };
            LIB_SYMBOL*  kiPart = m_partMap.at( sym.PartRef.RefID );
            bool         copy = false;

            // The symbol definition in the part either does not exist for this gate number
            // or is different to the symbol instance. We need to load a new symbol
            if( m_partSymbolsMap.find( partSymbolID ) == m_partSymbolsMap.end()
                || m_partSymbolsMap.at( partSymbolID ) != sym.SymdefID )
            {
                kiPart = new LIB_SYMBOL( *kiPart ); // Make a copy
                copy = true;
                const PART& part = Parts.PartDefinitions.at( sym.PartRef.RefID );
                loadSymDefIntoLibrary( sym.SymdefID, &part, sym.GateID, kiPart );
            }

            LIB_SYMBOL* scaledPart = getScaledLibPart( kiPart, sym.ScaleRatioNumerator,
                                                       sym.ScaleRatioDenominator );

            double      symOrientDeciDeg = 0.0;
            SCH_SYMBOL* symbol = loadSchematicSymbol( sym, *scaledPart, symOrientDeciDeg );

            delete scaledPart;

            if( copy )
                delete kiPart;

            SCH_FIELD* refField = symbol->GetField( REFERENCE_FIELD );

            sym.ComponentRef.Designator.Replace( wxT( "\n" ), wxT( "\\n" ) );
            sym.ComponentRef.Designator.Replace( wxT( "\r" ), wxT( "\\r" ) );
            sym.ComponentRef.Designator.Replace( wxT( "\t" ), wxT( "\\t" ) );
            sym.ComponentRef.Designator.Replace( wxT( " " ), wxT( "_" ) );

            refField->SetText( sym.ComponentRef.Designator );
            loadSymbolFieldAttribute( sym.ComponentRef.AttrLoc, symOrientDeciDeg,
                                      sym.Mirror, refField );

            if( sym.HasPartRef )
            {
                SCH_FIELD* partField = symbol->FindField( PartNameFieldName );

                if( !partField )
                {
                    int fieldID = symbol->GetFieldCount();
                    partField = symbol->AddField( SCH_FIELD( wxPoint(), fieldID, symbol,
                                                             PartNameFieldName ) );
                }

                wxASSERT( partField->GetName() == PartNameFieldName );

                wxString partname = getPart( sym.PartRef.RefID ).Name;
                partname.Replace( wxT( "\n" ), wxT( "\\n" ) );
                partname.Replace( wxT( "\r" ), wxT( "\\r" ) );
                partname.Replace( wxT( "\t" ), wxT( "\\t" ) );
                partField->SetText( partname );

                loadSymbolFieldAttribute( sym.PartRef.AttrLoc, symOrientDeciDeg,
                                          sym.Mirror, partField );

                partField->SetVisible( SymbolPartNameColor.IsVisible );
            }

            for( auto attr : sym.AttributeValues )
            {
                ATTRIBUTE_VALUE attrVal = attr.second;

                if( attrVal.HasLocation )
                {
                    wxString attrName = getAttributeName( attrVal.AttributeID );
                    SCH_FIELD* attrField = symbol->FindField( attrName );

                    if( !attrField )
                    {
                        int fieldID = symbol->GetFieldCount();
                        attrField = symbol->AddField( SCH_FIELD( wxPoint(), fieldID,
                                                                 symbol, attrName ) );
                    }

                    wxASSERT( attrField->GetName() == attrName );

                    attrVal.Value.Replace( wxT( "\n" ), wxT( "\\n" ) );
                    attrVal.Value.Replace( wxT( "\r" ), wxT( "\\r" ) );
                    attrVal.Value.Replace( wxT( "\t" ), wxT( "\\t" ) );
                    attrField->SetText( attrVal.Value );

                    loadSymbolFieldAttribute( attrVal.AttributeLocation, symOrientDeciDeg,
                                              sym.Mirror, attrField );
                    attrField->SetVisible( isAttributeVisible( attrVal.AttributeID ) );
                }
            }
        }
        else if( sym.IsSymbolVariant )
        {
            if( Library.SymbolDefinitions.find( sym.SymdefID ) == Library.SymbolDefinitions.end() )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Symbol ID '%s' references library symbol '%s' which could not be "
                           "found in the library. Did you export all items of the design?" ),
                        sym.ID, sym.PartRef.RefID ) );
            }

            SYMDEF_SCM libSymDef = Library.SymbolDefinitions.at( sym.SymdefID );

            if( libSymDef.Terminals.size() != 1 )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Symbol ID '%s' is a signal reference or global signal but it has too "
                           "many pins. The expected number of pins is 1 but %d were found." ),
                        sym.ID, libSymDef.Terminals.size() ) );
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
                    SYMDEF_SCM symbolDef = Library.SymbolDefinitions.at( symID );

                    kiPart = new LIB_SYMBOL( libPartName );
                    kiPart->SetPower();
                    loadSymDefIntoLibrary( symID, nullptr, "A", kiPart );

                    kiPart->GetValueField().SetText( symbolInstanceNetName );

                    if( symbolDef.TextLocations.find( SIGNALNAME_ORIGIN_ATTRID )
                            != symbolDef.TextLocations.end() )
                    {
                        TEXT_LOCATION txtLoc =
                                symbolDef.TextLocations.at( SIGNALNAME_ORIGIN_ATTRID );

                        wxPoint valPos = getKiCadLibraryPoint( txtLoc.Position, symbolDef.Origin );

                        kiPart->GetValueField().SetPosition( valPos );
                        kiPart->GetValueField().SetVisible( true );
                    }
                    else
                    {
                        kiPart->GetValueField().SetVisible( false );
                    }

                    kiPart->GetReferenceField().SetText( "#PWR" );
                    kiPart->GetReferenceField().SetVisible( false );
                    ( *m_plugin )->SaveSymbol( m_libraryFileName.GetFullPath(), kiPart );
                    m_powerSymLibMap.insert( { libPartName, kiPart } );
                }
                else
                {
                    kiPart = m_powerSymLibMap.at( libPartName );
                    wxASSERT( kiPart->GetValueField().GetText() == symbolInstanceNetName );
                }

                LIB_SYMBOL* scaledPart = getScaledLibPart( kiPart, sym.ScaleRatioNumerator,
                                                           sym.ScaleRatioDenominator );

                double returnedOrient = 0.0;
                SCH_SYMBOL* symbol = loadSchematicSymbol( sym, *scaledPart, returnedOrient );
                m_powerSymMap.insert( { sym.ID, symbol } );

                delete scaledPart;
            }
            else if( sym.SymbolVariant.Type == SYMBOLVARIANT::TYPE::SIGNALREF )
            {
                // There should only be one pin and we'll use that to set the position
                TERMINAL& symbolTerminal = libSymDef.Terminals.begin()->second;
                wxPoint   terminalPosOffset = symbolTerminal.Position - libSymDef.Origin;
                double    rotateDeciDegree = getAngleTenthDegree( sym.OrientAngle );

                if( sym.Mirror )
                    rotateDeciDegree += 1800.0;

                RotatePoint( &terminalPosOffset, -rotateDeciDegree );

                SCH_GLOBALLABEL* netLabel = new SCH_GLOBALLABEL;
                netLabel->SetPosition( getKiCadPoint( sym.Origin + terminalPosOffset ) );
                netLabel->SetText( "***UNKNOWN NET****" ); // This should be later updated when we load the netlist
                netLabel->SetTextSize( wxSize( Mils2iu( 50 ), Mils2iu( 50 ) ) );

                SYMDEF_SCM symbolDef = Library.SymbolDefinitions.at( sym.SymdefID );

                if( symbolDef.TextLocations.count( LINK_ORIGIN_ATTRID ) )
                {
                    TEXT_LOCATION linkOrigin = symbolDef.TextLocations.at( LINK_ORIGIN_ATTRID );
                    applyTextSettings( netLabel, linkOrigin.TextCodeID, linkOrigin.Alignment,
                                       linkOrigin.Justification );
                }

                netLabel->SetLabelSpinStyle( getSpinStyle( sym.OrientAngle, sym.Mirror ) );

                if( libSymDef.Alternate.Lower().Contains( "in" ) )
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );
                else if( libSymDef.Alternate.Lower().Contains( "bi" ) )
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );
                else if( libSymDef.Alternate.Lower().Contains( "out" ) )
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );
                else
                    netLabel->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );

                m_sheetMap.at( sym.LayerID )->GetScreen()->Append( netLabel );
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
                symbolName = wxString::Format( "ID: %s", sym.ID);

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
            kiBusAlias->SetParent( screen );
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
                wxPoint  nearestPt   = (wxPoint) busLineChain.NearestPoint( busLabelLoc );

                label->SetPosition( nearestPt );

                applyTextSettings( label,
                                   bus.BusLabel.TextCodeID,
                                   bus.BusLabel.Alignment,
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
                SCH_FIELD* val = m_powerSymMap.at( netTerm.SymbolID )->GetField( VALUE_FIELD );
                val->SetText( netName );
                val->SetBold( false );
                val->SetVisible( false );

                if( netTerm.HasNetLabel )
                {
                    val->SetVisible( true );
                    val->SetPosition( getKiCadPoint( netTerm.NetLabel.Position ) );

                    applyTextSettings( val,
                                       netTerm.NetLabel.TextCodeID,
                                       netTerm.NetLabel.Alignment,
                                       netTerm.NetLabel.Justification,
                                       netTerm.NetLabel.OrientAngle,
                                       netTerm.NetLabel.Mirror  );
                }
            }
            else if( m_globalLabelsMap.find( netTerm.SymbolID ) != m_globalLabelsMap.end() )
            {
                m_globalLabelsMap.at( netTerm.SymbolID )->SetText( netName );
            }
        }

        auto getHierarchicalLabel =
            [&]( NETELEMENT_ID aNode ) -> SCH_HIERLABEL*
            {
                if( aNode.Contains( "BLKT" ) )
                {
                    NET_SCH::BLOCK_TERM blockTerm = net.BlockTerminals.at( aNode );
                    BLOCK_PIN_ID blockPinID = std::make_pair( blockTerm.BlockID,
                                                              blockTerm.TerminalID );

                    if( m_sheetPinMap.find( blockPinID )
                            != m_sheetPinMap.end() )
                    {
                        return m_sheetPinMap.at( blockPinID );
                    }
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

            if( !m_busesMap.at( bus.ID )->Contains( netName ) )
                m_busesMap.at( bus.ID )->AddMember( netName );

            SCH_BUS_WIRE_ENTRY* busEntry =
                    new SCH_BUS_WIRE_ENTRY( getKiCadPoint( busTerm.FirstPoint ), false );

            wxPoint size =
                    getKiCadPoint( busTerm.SecondPoint ) - getKiCadPoint( busTerm.FirstPoint );
            busEntry->SetSize( wxSize( size.x, size.y ) );

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
                applyTextSettings( label,
                                   busTerm.NetLabel.TextCodeID,
                                   busTerm.NetLabel.Alignment,
                                   busTerm.NetLabel.Justification );
            }
            else
            {
                label->SetTextSize( wxSize( SMALL_LABEL_SIZE, SMALL_LABEL_SIZE ) );
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
                applyTextSettings( label,
                                   dangler.NetLabel.TextCodeID,
                                   dangler.NetLabel.Alignment,
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
            wxPoint   last;
            SCH_LINE* wire = nullptr;

            SHAPE_LINE_CHAIN wireChain; // Create a temp. line chain representing the connection

            for( POINT pt : conn.Path )
            {
                wireChain.Append( getKiCadPoint( pt ) );
            }

            // AUTO-FIX SHEET PINS
            //--------------------
            // KiCad constrains the sheet pin on the edge of the sheet object whereas in
            // CADSTAR it can be anywhere. Let's find the intersection of the wires with the sheet
            // and place the hierarchical
            std::vector<NETELEMENT_ID> nodes;
            nodes.push_back( conn.StartNode );
            nodes.push_back( conn.EndNode );

            for( NETELEMENT_ID node : nodes )
            {
                SCH_HIERLABEL* sheetPin = getHierarchicalLabel( node );

                if( sheetPin )
                {
                    if( sheetPin->Type() == SCH_SHEET_PIN_T
                            && SCH_SHEET::ClassOf( sheetPin->GetParent() ) )
                    {
                        SCH_SHEET* parentSheet   = static_cast<SCH_SHEET*>( sheetPin->GetParent() );
                        wxSize     sheetSize     = parentSheet->GetSize();
                        wxPoint    sheetPosition = parentSheet->GetPosition();

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

                            sheetPin->SetPosition( (wxPoint) intsctPt );
                        }
                    }
                }
            }

            auto fixNetLabelsAndSheetPins =
                [&]( double aWireAngleDeciDeg, NETELEMENT_ID& aNetEleID )
                {
                    LABEL_SPIN_STYLE spin = getSpinStyleDeciDeg( aWireAngleDeciDeg );

                    if( netlabels.find( aNetEleID ) != netlabels.end() )
                        netlabels.at( aNetEleID )->SetLabelSpinStyle( spin.MirrorY() );

                    SCH_HIERLABEL* sheetPin = getHierarchicalLabel( aNetEleID );

                    if( sheetPin )
                        sheetPin->SetLabelSpinStyle( spin.MirrorX() );
                };

            // Now we can load the wires and fix the label orientations
            for( const VECTOR2I& pt : wireChain.CPoints() )
            {
                if( firstPt )
                {
                    last     = (wxPoint) pt;
                    firstPt  = false;
                    secondPt = true;
                    continue;
                }

                if( secondPt )
                {
                    secondPt = false;


                    wxPoint kiLast           = last;
                    wxPoint kiCurrent        = (wxPoint) pt;
                    double  wireangleDeciDeg = getPolarAngle( kiLast - kiCurrent );
                    fixNetLabelsAndSheetPins( wireangleDeciDeg, conn.StartNode );
                }

                wire = new SCH_LINE();

                wire->SetStartPoint( last );
                wire->SetEndPoint( (wxPoint) pt );
                wire->SetLayer( LAYER_WIRE );

                if( !conn.ConnectionLineCode.IsEmpty() )
                    wire->SetLineWidth( getLineThickness( conn.ConnectionLineCode ) );

                last = (wxPoint) pt;

                m_sheetMap.at( conn.LayerID )->GetScreen()->Append( wire );
            }

            //Fix labels on the end wire
            if( wire )
            {
                wxPoint kiLast           = wire->GetEndPoint();
                wxPoint kiCurrent        = wire->GetStartPoint();
                double  wireangleDeciDeg = getPolarAngle( kiLast - kiCurrent );
                fixNetLabelsAndSheetPins( wireangleDeciDeg, conn.EndNode );
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

                double labelAngleDeciDeg = getAngleTenthDegree( junc.NetLabel.OrientAngle );
                LABEL_SPIN_STYLE spin = getSpinStyleDeciDeg( labelAngleDeciDeg );
                label->SetLabelSpinStyle( spin );

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
        wxPoint    moveVector = getKiCadPoint( docSym.Origin ) - getKiCadPoint( docSymDef.Origin );
        double     rotationAngle = getAngleTenthDegree( docSym.OrientAngle );
        double     scalingFactor =
                (double) docSym.ScaleRatioNumerator / (double) docSym.ScaleRatioDenominator;
        wxPoint centreOfTransform = getKiCadPoint( docSymDef.Origin );
        bool    mirrorInvert      = docSym.Mirror;

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

            wxPoint newPosition = applyTransform( kiTxt->GetPosition(), moveVector, rotationAngle,
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
    auto findAndReplaceTextField = [&]( TEXT_FIELD_NAME aField, wxString aValue ) {
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

    PROJECT* pj = &m_schematic->Prj();

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


void CADSTAR_SCH_ARCHIVE_LOADER::loadSymDefIntoLibrary( const SYMDEF_ID& aSymdefID,
        const PART* aCadstarPart, const GATE_ID& aGateID, LIB_SYMBOL* aSymbol )
{
    wxCHECK( Library.SymbolDefinitions.find( aSymdefID ) != Library.SymbolDefinitions.end(), );

    SYMDEF_SCM symbol = Library.SymbolDefinitions.at( aSymdefID );
    int        gateNumber = getKiCadUnitNumberFromGate( aGateID );

    for( std::pair<FIGURE_ID, FIGURE> figPair : symbol.Figures )
    {
        FIGURE fig = figPair.second;
        int    lineThickness = getLineThickness( fig.LineCodeID );

        loadLibrarySymbolShapeVertices( fig.Shape.Vertices, symbol.Origin, aSymbol, gateNumber,
                                            lineThickness );

        for( CUTOUT c : fig.Shape.Cutouts )
        {
            loadLibrarySymbolShapeVertices( c.Vertices, symbol.Origin, aSymbol, gateNumber,
                                            lineThickness );
        }
    }

    TERMINAL_TO_PINNUM_MAP pinNumMap;

    for( std::pair<TERMINAL_ID, TERMINAL> termPair : symbol.Terminals )
    {
        TERMINAL term    = termPair.second;
        wxString pinNum  = wxString::Format( "%ld", term.ID );
        wxString pinName = wxEmptyString;
        LIB_PIN* pin = new LIB_PIN( aSymbol );

        if( aCadstarPart )
        {
            PART::DEFINITION::PIN csPin = getPartDefinitionPin( *aCadstarPart, aGateID, term.ID );

            pinName = HandleTextOverbar( csPin.Label );
            pinNum = HandleTextOverbar( csPin.Name );

            if( pinNum.IsEmpty() )
            {
                if( !csPin.Identifier.IsEmpty() )
                    pinNum = csPin.Identifier;
                else if( csPin.ID == UNDEFINED_VALUE )
                    pinNum = wxString::Format( "%ld", term.ID );
                else
                    pinNum = wxString::Format( "%ld", csPin.ID );
            }

            pin->SetType( getKiCadPinType( csPin.Type ) );

            pinNumMap.insert( { term.ID, pinNum } );
        }
        else
        {
            // If no part is defined, we don't know the pin type. Assume passive pin
            pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        }

        pin->SetPosition( getKiCadLibraryPoint( term.Position, symbol.Origin ) );
        pin->SetLength( 0 ); //CADSTAR Pins are just a point (have no length)
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );
        pin->SetUnit( gateNumber );
        pin->SetNumber( pinNum );
        pin->SetName( pinName );

        int pinNumberHeight = getTextHeightFromTextCode( wxT( "TC0" ) ); // TC0 is the default CADSTAR text size for name/number
        int pinNameHeight = getTextHeightFromTextCode( wxT( "TC0" ) );

        if( symbol.PinNumberLocations.count( term.ID ) )
        {
            PIN_NUM_LABEL_LOC pinNumLocation = symbol.PinNumberLocations.at( term.ID );
            pinNumberHeight = getTextHeightFromTextCode( pinNumLocation.TextCodeID );
        }

        if( symbol.PinLabelLocations.count( term.ID ) )
        {
            PIN_NUM_LABEL_LOC pinNameLocation = symbol.PinLabelLocations.at( term.ID );
            pinNameHeight = getTextHeightFromTextCode( pinNameLocation.TextCodeID );
        }

        pin->SetNumberTextSize( pinNumberHeight );
        pin->SetNameTextSize( pinNameHeight );

        if( aSymbol->IsPower() )
        {
            pin->SetVisible( false );
            pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
            pin->SetName( aSymbol->GetName() );
        }

        aSymbol->AddDrawItem( pin );
    }

    fixUpLibraryPins( aSymbol, gateNumber );

    if(aCadstarPart)
        m_pinNumsMap.insert( { aCadstarPart->ID + aGateID, pinNumMap } );

    for( std::pair<TEXT_ID, TEXT> textPair : symbol.Texts )
    {
        TEXT csText = textPair.second;

        LIB_TEXT* libtext = new LIB_TEXT( aSymbol );
        libtext->SetText( csText.Text );
        libtext->SetUnit( gateNumber );
        libtext->SetPosition( getKiCadLibraryPoint( csText.Position, symbol.Origin ) );
        libtext->SetMultilineAllowed( true ); // temporarily so that we calculate bbox correctly

        applyTextSettings( libtext,
                           csText.TextCodeID,
                           csText.Alignment,
                           csText.Justification,
                           csText.OrientAngle,
                           csText.Mirror );

        // Split out multi line text items into individual text elements
        if( csText.Text.Contains( "\n" ) )
        {
            wxArrayString strings;
            wxStringSplit( csText.Text, strings, '\n' );
            wxPoint firstLinePos;

            for( size_t ii = 0; ii < strings.size(); ++ii )
            {
                EDA_RECT bbox = libtext->GetTextBox( ii, true );
                wxPoint  linePos = { bbox.GetLeft(), -bbox.GetBottom() };

                RotatePoint( &linePos, libtext->GetTextPos(), -libtext->GetTextAngle() );

                LIB_TEXT* line = static_cast<LIB_TEXT*>( libtext->Clone() );
                line->SetText( strings[ii] );
                line->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                line->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                line->SetTextPos( linePos );

                // Multiline text not allowed in LIB_TEXT
                line->SetMultilineAllowed( false );
                aSymbol->AddDrawItem( line );
            }

            delete libtext;
        }
        else
        {
            // Multiline text not allowed in LIB_TEXT
            libtext->SetMultilineAllowed( false );
            aSymbol->AddDrawItem( libtext );
        }
    }

    if( symbol.TextLocations.find( SYMBOL_NAME_ATTRID ) != symbol.TextLocations.end() )
    {
        TEXT_LOCATION textLoc = symbol.TextLocations.at( SYMBOL_NAME_ATTRID );
        LIB_FIELD*    field = &aSymbol->GetReferenceField();
        applyToLibraryFieldAttribute( textLoc, symbol.Origin, field );
        field->SetUnit( gateNumber );
    }

    // Hide the value field for now (it might get unhidden if an attribute exists in the cadstar
    // design with the text "Value"
    aSymbol->GetValueField().SetVisible( false );

    if( symbol.TextLocations.find( PART_NAME_ATTRID ) != symbol.TextLocations.end() )
    {
        TEXT_LOCATION textLoc = symbol.TextLocations.at( PART_NAME_ATTRID );
        LIB_FIELD*    field = aSymbol->FindField( PartNameFieldName );

        if( !field )
        {
            int fieldID = aSymbol->GetFieldCount();
            field = new LIB_FIELD( aSymbol, fieldID );
            field->SetName( PartNameFieldName );
            aSymbol->AddField( field );
        }

        wxASSERT( field->GetName() == PartNameFieldName );
        applyToLibraryFieldAttribute( textLoc, symbol.Origin, field );

        if( aCadstarPart )
        {
            wxString partName = aCadstarPart->Name;
            partName.Replace( wxT( "\n" ), wxT( "\\n" ) );
            partName.Replace( wxT( "\r" ), wxT( "\\r" ) );
            partName.Replace( wxT( "\t" ), wxT( "\\t" ) );
            field->SetText( partName );
        }

        field->SetUnit( gateNumber );
        field->SetVisible( SymbolPartNameColor.IsVisible );
    }

    if( aCadstarPart )
    {
        wxString footprintRefName = wxEmptyString;
        wxString footprintAlternateName = wxEmptyString;

        auto loadLibraryField =
            [&]( ATTRIBUTE_VALUE& aAttributeVal )
            {
                wxString attrName = getAttributeName( aAttributeVal.AttributeID );

                // Remove invalid field characters
                aAttributeVal.Value.Replace( wxT( "\n" ), wxT( "\\n" ) );
                aAttributeVal.Value.Replace( wxT( "\r" ), wxT( "\\r" ) );
                aAttributeVal.Value.Replace( wxT( "\t" ), wxT( "\\t" ) );

                //TODO: Handle "links": In cadstar a field can be a "link" if its name starts
                // with the characters "Link ". Need to figure out how to convert them to
                // equivalent in KiCad.

                if( attrName == wxT( "(PartDefinitionNameStem)" ) )
                {
                    //Space not allowed in Reference field
                    aAttributeVal.Value.Replace( wxT( " " ), "_" );
                    aSymbol->GetReferenceField().SetText( aAttributeVal.Value );
                    return;
                }
                else if( attrName == wxT( "(PartDescription)" ) )
                {
                    aSymbol->SetDescription( aAttributeVal.Value );
                    return;
                }
                else if( attrName == wxT( "(PartDefinitionReferenceName)" ) )
                {
                    footprintRefName = aAttributeVal.Value;
                    return;
                }
                else if( attrName == wxT( "(PartDefinitionAlternateName)" ) )
                {
                    footprintAlternateName = aAttributeVal.Value;
                    return;
                }

                LIB_FIELD* attrField = aSymbol->FindField( attrName );

                if( !attrField )
                {
                    int fieldID = aSymbol->GetFieldCount();
                    attrField = new LIB_FIELD( aSymbol, fieldID );
                    attrField->SetName( attrName );
                    aSymbol->AddField( attrField );
                }

                wxASSERT( attrField->GetName() == attrName );
                attrField->SetText( aAttributeVal.Value );
                attrField->SetUnit( gateNumber );

                ATTRIBUTE_ID& attrid = aAttributeVal.AttributeID;
                attrField->SetVisible( isAttributeVisible( attrid ) );

                if( aAttributeVal.HasLocation )
                {
                    // #1 Check if the part itself defined a location for the field
                    applyToLibraryFieldAttribute( aAttributeVal.AttributeLocation, symbol.Origin,
                                                  attrField );
                }
                else if( symbol.TextLocations.find( aAttributeVal.AttributeID )
                         != symbol.TextLocations.end() )
                {
                    // #2 Look in the symbol definition: Text locations
                    TEXT_LOCATION symTxtLoc = symbol.TextLocations.at( aAttributeVal.AttributeID );
                    applyToLibraryFieldAttribute( symTxtLoc, symbol.Origin, attrField );
                }
                else if( symbol.AttributeValues.find( attrid ) != symbol.AttributeValues.end()
                         && symbol.AttributeValues.at( attrid ).HasLocation )
                {
                    // #3 Look in the symbol definition: Attribute values
                    ATTRIBUTE_VALUE symAttrVal = symbol.AttributeValues.at( attrid );
                    applyToLibraryFieldAttribute( symAttrVal.AttributeLocation, symbol.Origin,
                                                  attrField );
                }
                else
                {
                    attrField->SetVisible( false );
                    applyTextSettings( attrField, wxT( "TC1" ), ALIGNMENT::NO_ALIGNMENT,
                                       JUSTIFICATION::LEFT );
                }
            };

        // Load all attributes in the Part Definition
        for( std::pair<ATTRIBUTE_ID,
             ATTRIBUTE_VALUE> attr : aCadstarPart->Definition.AttributeValues )
        {
            ATTRIBUTE_VALUE attrVal = attr.second;
            loadLibraryField( attrVal );
        }

        // Load all attributes in the Part itself.
        for( std::pair<ATTRIBUTE_ID, ATTRIBUTE_VALUE> attr : aCadstarPart->AttributeValues )
        {
            ATTRIBUTE_VALUE attrVal = attr.second;
            loadLibraryField( attrVal );
        }

        wxString      fpNameInLibrary = generateLibName( footprintRefName, footprintAlternateName );
        wxArrayString fpFilters;
        fpFilters.Add( fpNameInLibrary );

        aSymbol->SetFPFilters( fpFilters );

        // Assume that the PCB footprint library name will be the same as the schematic filename
        wxFileName schFilename( Filename );
        wxString   libName = schFilename.GetName();

        aSymbol->GetFootprintField().SetText( libName + wxT( ":" ) + fpNameInLibrary );
    }

    if( aCadstarPart && aCadstarPart->Definition.HidePinNames )
    {
        aSymbol->SetShowPinNames( false );
        aSymbol->SetShowPinNumbers( false );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadLibrarySymbolShapeVertices(
        const std::vector<VERTEX>& aCadstarVertices, wxPoint aSymbolOrigin, LIB_SYMBOL* aSymbol,
        int aGateNumber, int aLineThickness )
{
    const VERTEX* prev = &aCadstarVertices.at( 0 );
    const VERTEX* cur;

    wxASSERT_MSG(
            prev->Type == VERTEX_TYPE::POINT, "First vertex should always be a point vertex" );

    for( size_t i = 1; i < aCadstarVertices.size(); i++ )
    {
        cur = &aCadstarVertices.at( i );

        LIB_ITEM* segment    = nullptr;
        bool      cw         = false;
        wxPoint   startPoint = getKiCadLibraryPoint( prev->End, aSymbolOrigin );
        wxPoint   endPoint   = getKiCadLibraryPoint( cur->End, aSymbolOrigin );
        wxPoint   centerPoint;

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
        case VERTEX_TYPE::POINT:
            segment = new LIB_POLYLINE( aSymbol );
            ( (LIB_POLYLINE*) segment )->AddPoint( startPoint );
            ( (LIB_POLYLINE*) segment )->AddPoint( endPoint );
            break;

        case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::CLOCKWISE_ARC:
            cw = true;
            KI_FALLTHROUGH;

        case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::ANTICLOCKWISE_ARC:
            segment = new LIB_ARC( aSymbol );

            ( (LIB_ARC*) segment )->SetPosition( centerPoint );

            if( cw )
            {
                ( (LIB_ARC*) segment )->SetStart( endPoint );
                ( (LIB_ARC*) segment )->SetEnd( startPoint );
            }
            else
            {
                ( (LIB_ARC*) segment )->SetStart( startPoint );
                ( (LIB_ARC*) segment )->SetEnd( endPoint );
            }

            ( (LIB_ARC*) segment )->CalcRadiusAngles();
            break;
        }

        segment->SetUnit( aGateNumber );
        segment->SetWidth( aLineThickness );
        aSymbol->AddDrawItem( segment );

        prev = cur;
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyToLibraryFieldAttribute(
        const ATTRIBUTE_LOCATION& aCadstarAttrLoc, wxPoint aSymbolOrigin, LIB_FIELD* aKiCadField )
{
    aKiCadField->SetTextPos( getKiCadLibraryPoint( aCadstarAttrLoc.Position, aSymbolOrigin ) );

    applyTextSettings( aKiCadField,
                       aCadstarAttrLoc.TextCodeID,
                       aCadstarAttrLoc.Alignment,
                       aCadstarAttrLoc.Justification,
                       aCadstarAttrLoc.OrientAngle,
                       aCadstarAttrLoc.Mirror );
}


SCH_SYMBOL* CADSTAR_SCH_ARCHIVE_LOADER::loadSchematicSymbol( const SYMBOL& aCadstarSymbol,
                                                             const LIB_SYMBOL& aKiCadPart,
                                                             double& aComponentOrientationDeciDeg )
{
    LIB_ID  libId( m_libraryFileName.GetName(), aKiCadPart.GetName() );
    int     unit = getKiCadUnitNumberFromGate( aCadstarSymbol.GateID );

    SCH_SHEET_PATH sheetpath;
    SCH_SHEET* kiSheet = m_sheetMap.at( aCadstarSymbol.LayerID );
    m_rootSheet->LocatePathOfScreen( kiSheet->GetScreen(), &sheetpath );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( aKiCadPart, libId, &sheetpath, unit );

    if( aCadstarSymbol.IsComponent )
    {
        symbol->SetRef( &sheetpath, aCadstarSymbol.ComponentRef.Designator );
    }

    symbol->SetPosition( getKiCadPoint( aCadstarSymbol.Origin ) );

    double compAngleDeciDeg = getAngleTenthDegree( aCadstarSymbol.OrientAngle );
    int compOrientation  = 0;

    if( aCadstarSymbol.Mirror )
    {
        compAngleDeciDeg = -compAngleDeciDeg;
        compOrientation += SYMBOL_ORIENTATION_T::SYM_MIRROR_Y;
    }

    compOrientation += getComponentOrientation( compAngleDeciDeg, aComponentOrientationDeciDeg );

    if( NormalizeAngle180( compAngleDeciDeg ) != NormalizeAngle180( aComponentOrientationDeciDeg ) )
    {
        m_reporter->Report( wxString::Format( _( "Symbol '%s' is rotated by an angle of %.1f "
                                                 "degrees in the original CADSTAR design but "
                                                 "KiCad only supports rotation angles multiples "
                                                 "of 90 degrees. The connecting wires will need "
                                                 "manual fixing." ),
                                              aCadstarSymbol.ComponentRef.Designator,
                                              compAngleDeciDeg / 10.0 ),
                            RPT_SEVERITY_ERROR);
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

    wxString gate = ( aCadstarSymbol.GateID.IsEmpty() ) ? wxT( "A" ) : aCadstarSymbol.GateID;
    wxString partGateIndex = aCadstarSymbol.PartRef.RefID + gate;

    //Handle pin swaps
    if( m_pinNumsMap.find( partGateIndex ) != m_pinNumsMap.end() )
    {
        TERMINAL_TO_PINNUM_MAP termNumMap = m_pinNumsMap.at( partGateIndex );

        std::map<wxString, LIB_PIN*> pinNumToLibPinMap;

        for( auto& term : termNumMap )
        {
            wxString pinNum = term.second;
            pinNumToLibPinMap.insert( { pinNum,
                    symbol->GetLibSymbolRef()->GetPin( term.second ) } );
        }

        auto replacePinNumber = [&]( wxString aOldPinNum, wxString aNewPinNum )
                                {
                                    if( aOldPinNum == aNewPinNum )
                                        return;

                                    LIB_PIN* libpin = pinNumToLibPinMap.at( aOldPinNum );
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


void CADSTAR_SCH_ARCHIVE_LOADER::loadSymbolFieldAttribute(
        const ATTRIBUTE_LOCATION& aCadstarAttrLoc, const double& aComponentOrientationDeciDeg,
        bool aIsMirrored, SCH_FIELD* aKiCadField )
{
    aKiCadField->SetPosition( getKiCadPoint( aCadstarAttrLoc.Position ) );
    aKiCadField->SetVisible( true );

    ALIGNMENT alignment = aCadstarAttrLoc.Alignment;

    double textAngle = getAngleTenthDegree( aCadstarAttrLoc.OrientAngle );
    long long cadstarAngle = getCadstarAngle( textAngle - aComponentOrientationDeciDeg );

    if( aIsMirrored )
    {
        // In KiCad, the angle of the symbol instance affects the position of the symbol
        // fields because there is a distinction on x-axis and y-axis mirroring
        double angleDeciDeg = NormalizeAnglePos( aComponentOrientationDeciDeg );
        int    quadrant = KiROUND( angleDeciDeg / 900.0 );
        quadrant %= 4;

        switch( quadrant )
        {
        case 1:
        case 3: alignment = rotate180( alignment ); KI_FALLTHROUGH;
        case 0:
        case 2: alignment = mirrorX( alignment ); break;
        default: wxFAIL_MSG( "unknown quadrant" ); break;
        }
    }

    applyTextSettings( aKiCadField,
                       aCadstarAttrLoc.TextCodeID,
                       alignment,
                       aCadstarAttrLoc.Justification,
                       cadstarAngle,
                       aCadstarAttrLoc.Mirror );
}


int CADSTAR_SCH_ARCHIVE_LOADER::getComponentOrientation(
        double aOrientAngleDeciDeg, double& aReturnedOrientationDeciDeg )
{
    int compOrientation = SYMBOL_ORIENTATION_T::SYM_ORIENT_0;

    int oDeg = (int) NormalizeAngle180( aOrientAngleDeciDeg );

    if( oDeg >= -450 && oDeg <= 450 )
    {
        compOrientation             = SYMBOL_ORIENTATION_T::SYM_ORIENT_0;
        aReturnedOrientationDeciDeg = 0.0;
    }
    else if( oDeg >= 450 && oDeg <= 1350 )
    {
        compOrientation             = SYMBOL_ORIENTATION_T::SYM_ORIENT_90;
        aReturnedOrientationDeciDeg = 900.0;
    }
    else if( oDeg >= 1350 || oDeg <= -1350 )
    {
        compOrientation             = SYMBOL_ORIENTATION_T::SYM_ORIENT_180;
        aReturnedOrientationDeciDeg = 1800.0;
    }
    else
    {
        compOrientation             = SYMBOL_ORIENTATION_T::SYM_ORIENT_270;
        aReturnedOrientationDeciDeg = 2700.0;
    }

    return compOrientation;
}


CADSTAR_SCH_ARCHIVE_LOADER::POINT CADSTAR_SCH_ARCHIVE_LOADER::getLocationOfNetElement(
        const NET_SCH& aNet, const NETELEMENT_ID& aNetElementID )
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
        wxPoint   symbolOrigin = sym.Origin;

        if( Library.SymbolDefinitions.find( symdefid ) == Library.SymbolDefinitions.end() )
            return logUnknownNetElementError();

        wxPoint libpinPosition =
                Library.SymbolDefinitions.at( symdefid ).Terminals.at( termid ).Position;
        wxPoint libOrigin   = Library.SymbolDefinitions.at( symdefid ).Origin;

        wxPoint pinOffset = libpinPosition - libOrigin;
        pinOffset.x = ( pinOffset.x * sym.ScaleRatioNumerator ) / sym.ScaleRatioDenominator;
        pinOffset.y = ( pinOffset.y * sym.ScaleRatioNumerator ) / sym.ScaleRatioDenominator;

        wxPoint pinPosition = symbolOrigin + pinOffset;

        double compAngleDeciDeg = getAngleTenthDegree( sym.OrientAngle );

        if( sym.Mirror )
            pinPosition.x = ( 2 * symbolOrigin.x ) - pinPosition.x;

        double adjustedOrientationDecideg;
        getComponentOrientation( compAngleDeciDeg, adjustedOrientationDecideg );

        RotatePoint( &pinPosition, symbolOrigin, -adjustedOrientationDecideg );

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

    return POINT();
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getNetName( const NET_SCH& aNet )
{
    wxString netname = aNet.Name;

    if( netname.IsEmpty() )
        netname = wxString::Format( "$%ld", aNet.SignalNum );

    return netname;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadGraphicStaightSegment( const wxPoint& aStartPoint,
        const wxPoint& aEndPoint, const LINECODE_ID& aCadstarLineCodeID,
        const LAYER_ID& aCadstarSheetID, const SCH_LAYER_ID& aKiCadSchLayerID,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    SCH_LINE* segment = new SCH_LINE();

    segment->SetLayer( aKiCadSchLayerID );
    segment->SetLineWidth( KiROUND( getLineThickness( aCadstarLineCodeID ) * aScalingFactor ) );
    segment->SetLineStyle( getLineStyle( aCadstarLineCodeID ) );

    //Apply transforms
    wxPoint startPoint = applyTransform( aStartPoint, aMoveVector, aRotationAngleDeciDeg,
                                         aScalingFactor, aTransformCentre, aMirrorInvert );
    wxPoint endPoint = applyTransform( aEndPoint, aMoveVector, aRotationAngleDeciDeg,
                                       aScalingFactor, aTransformCentre, aMirrorInvert );

    segment->SetStartPoint( startPoint );
    segment->SetEndPoint( endPoint );

    loadItemOntoKiCadSheet( aCadstarSheetID, segment );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadShapeVertices( const std::vector<VERTEX>& aCadstarVertices,
        LINECODE_ID aCadstarLineCodeID, LAYER_ID aCadstarSheetID, SCH_LAYER_ID aKiCadSchLayerID,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    const VERTEX* prev = &aCadstarVertices.at( 0 );
    const VERTEX* cur;

    wxASSERT_MSG(
            prev->Type == VERTEX_TYPE::POINT, "First vertex should always be a point vertex" );

    for( size_t ii = 1; ii < aCadstarVertices.size(); ii++ )
    {
        cur = &aCadstarVertices.at( ii );

        wxPoint   startPoint  = getKiCadPoint( prev->End );
        wxPoint   endPoint    = getKiCadPoint( cur->End );
        wxPoint   centerPoint = getKiCadPoint( cur->Center );
        bool      cw          = false;

        if( cur->Type == VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE
                || cur->Type == VERTEX_TYPE::CLOCKWISE_SEMICIRCLE )
        {
            centerPoint = ( startPoint + endPoint ) / 2;
        }

        switch( cur->Type )
        {
        case VERTEX_TYPE::CLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::CLOCKWISE_ARC:
            cw = true;
            KI_FALLTHROUGH;
        case VERTEX_TYPE::ANTICLOCKWISE_SEMICIRCLE:
        case VERTEX_TYPE::ANTICLOCKWISE_ARC:
        {
            double arcStartAngle = getPolarAngle( startPoint - centerPoint );
            double arcEndAngle   = getPolarAngle( endPoint - centerPoint );
            double arcAngleDeciDeg = arcEndAngle - arcStartAngle;

            if( cw )
                arcAngleDeciDeg = NormalizeAnglePos( arcAngleDeciDeg );
            else
                arcAngleDeciDeg = NormalizeAngleNeg( arcAngleDeciDeg );

            SHAPE_ARC tempArc( VECTOR2I(centerPoint), VECTOR2I(startPoint),
                               arcAngleDeciDeg / 10.0 );
            SHAPE_LINE_CHAIN arcSegments = tempArc.ConvertToPolyline( Millimeter2iu( 0.1 ) );

            // Load the arc as a series of piece-wise segments

            for( int jj = 0; jj < arcSegments.SegmentCount(); jj++ )
            {
                wxPoint segStart = (wxPoint) arcSegments.Segment( jj ).A;
                wxPoint segEnd   = (wxPoint) arcSegments.Segment( jj ).B;

                loadGraphicStaightSegment( segStart, segEnd, aCadstarLineCodeID, aCadstarSheetID,
                                           aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg,
                                           aScalingFactor, aTransformCentre, aMirrorInvert );
            }
        }
            break;

        case VERTEX_TYPE::POINT:
            loadGraphicStaightSegment( startPoint, endPoint, aCadstarLineCodeID, aCadstarSheetID,
                                       aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg,
                                       aScalingFactor, aTransformCentre, aMirrorInvert );
            break;

        default:
            wxFAIL_MSG( "Unknown CADSTAR Vertex type" );
        }


        prev = cur;
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadFigure( const FIGURE& aCadstarFigure,
        const LAYER_ID& aCadstarSheetIDOverride, SCH_LAYER_ID aKiCadSchLayerID,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    loadShapeVertices( aCadstarFigure.Shape.Vertices, aCadstarFigure.LineCodeID,
                       aCadstarSheetIDOverride, aKiCadSchLayerID, aMoveVector,
                       aRotationAngleDeciDeg, aScalingFactor, aTransformCentre, aMirrorInvert );

    for( CUTOUT cutout : aCadstarFigure.Shape.Cutouts )
    {
        loadShapeVertices( cutout.Vertices, aCadstarFigure.LineCodeID, aCadstarSheetIDOverride,
                           aKiCadSchLayerID, aMoveVector, aRotationAngleDeciDeg, aScalingFactor,
                           aTransformCentre, aMirrorInvert );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheetAndChildSheets(
        LAYER_ID aCadstarSheetID, const wxPoint& aPosition, wxSize aSheetSize,
        const SCH_SHEET_PATH& aParentSheet )
{
    wxCHECK_MSG( m_sheetMap.find( aCadstarSheetID ) == m_sheetMap.end(), ,
                 "Sheet already loaded!" );

    SCH_SHEET*  sheet  = new SCH_SHEET( aParentSheet.Last(), aPosition );
    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
    SCH_SHEET_PATH instance( aParentSheet );

    sheet->SetSize( aSheetSize );
    sheet->SetScreen( screen );

    wxString name = Sheets.SheetNames.at( aCadstarSheetID );

    SCH_FIELD& sheetNameField = sheet->GetFields()[SHEETNAME];
    SCH_FIELD& filenameField  = sheet->GetFields()[SHEETFILENAME];

    sheetNameField.SetText( name );

    int sheetNum = getSheetNumber( aCadstarSheetID );
    wxString  loadedFilename = wxFileName( Filename ).GetName();
    std::string filename = wxString::Format( "%s_%02d", loadedFilename, sheetNum ).ToStdString();

    ReplaceIllegalFileNameChars( &filename );
    filename += wxT( "." ) + KiCadSchematicFileExtension;

    filenameField.SetText( filename );

    wxFileName fn( m_schematic->Prj().GetProjectPath() + filename );
    sheet->GetScreen()->SetFileName( fn.GetFullPath() );
    aParentSheet.Last()->GetScreen()->Append( sheet );
    instance.push_back( sheet );
    sheet->AddInstance( instance.Path() );

    wxString pageNumStr = wxString::Format( "%d", getSheetNumber( aCadstarSheetID ) );
    sheet->SetPageNumber( instance, pageNumStr );

    sheet->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

    m_sheetMap.insert( { aCadstarSheetID, sheet } );

    loadChildSheets( aCadstarSheetID, instance );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadChildSheets( LAYER_ID aCadstarSheetID,
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

            std::pair<wxPoint, wxSize> blockExtents;

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

            if( block.HasBlockLabel )
            {
                // Add the block label as a separate field
                SCH_SHEET* loadedSheet = m_sheetMap.at( block.AssocLayerID );
                SCH_FIELDS fields      = loadedSheet->GetFields();

                for( SCH_FIELD& field : fields )
                {
                    field.SetVisible( false );
                }

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

                fields.push_back( blockNameField );
                loadedSheet->SetFields( fields );
            }
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
    for( LAYER_ID sheetID : Sheets.SheetOrder )
    {
        if( std::find( childSheets.begin(), childSheets.end(), sheetID ) == childSheets.end() )
            orphanSheets.push_back( sheetID );
    }

    return orphanSheets;
}


int CADSTAR_SCH_ARCHIVE_LOADER::getSheetNumber( LAYER_ID aCadstarSheetID )
{
    int i = 1;

    for( LAYER_ID sheetID : Sheets.SheetOrder )
    {
        if( sheetID == aCadstarSheetID )
            return i;

        ++i;
    }

    return -1;
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadItemOntoKiCadSheet( LAYER_ID aCadstarSheetID, SCH_ITEM* aItem )
{
    wxCHECK_MSG( aItem, /*void*/, "aItem is null" );

    if( aCadstarSheetID == "ALL_SHEETS" )
    {
        SCH_ITEM* duplicateItem;

        for( std::pair<LAYER_ID, SHEET_NAME> sheetPair : Sheets.SheetNames )
        {
            LAYER_ID sheetID = sheetPair.first;
            duplicateItem    = aItem->Duplicate();
            m_sheetMap.at( sheetID )->GetScreen()->Append( aItem->Duplicate() );
        }

        //Get rid of the extra copy:
        delete aItem;
        aItem = duplicateItem;
    }
    else if( aCadstarSheetID == "NO_SHEET" )
    {
        wxASSERT_MSG(
                false, "Trying to add an item to NO_SHEET? This might be a documentation symbol." );
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
            wxASSERT_MSG( false, "Unknown Sheet ID." );
        }
    }
}


CADSTAR_SCH_ARCHIVE_LOADER::SYMDEF_ID CADSTAR_SCH_ARCHIVE_LOADER::getSymDefFromName(
        const wxString& aSymdefName, const wxString& aSymDefAlternate )
{
    // Do a case-insensitive comparison
    for( std::pair<SYMDEF_ID, SYMDEF_SCM> symPair : Library.SymbolDefinitions )
    {
        SYMDEF_ID  id     = symPair.first;
        SYMDEF_SCM symdef = symPair.second;

        if( symdef.ReferenceName.Lower() == aSymdefName.Lower()
            && symdef.Alternate.Lower() == aSymDefAlternate.Lower() )
        {
            return id;
        }
    }

    return SYMDEF_ID();
}


bool CADSTAR_SCH_ARCHIVE_LOADER::isAttributeVisible( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    // Use CADSTAR visibility settings to determine if an attribute is visible
    if( AttrColors.AttributeColors.find( aCadstarAttributeID ) != AttrColors.AttributeColors.end() )
    {
        return AttrColors.AttributeColors.at( aCadstarAttributeID ).IsVisible;
    }

    return false; // If there is no visibility setting, assume not displayed
}


int CADSTAR_SCH_ARCHIVE_LOADER::getLineThickness( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
            m_schematic->Settings().m_DefaultWireThickness );

    return getKiCadLength( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Width );
}


PLOT_DASH_TYPE CADSTAR_SCH_ARCHIVE_LOADER::getLineStyle( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
             PLOT_DASH_TYPE::SOLID );

    // clang-format off
    switch( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Style )
    {
    case LINESTYLE::DASH:       return PLOT_DASH_TYPE::DASH;
    case LINESTYLE::DASHDOT:    return PLOT_DASH_TYPE::DASHDOT;
    case LINESTYLE::DASHDOTDOT: return PLOT_DASH_TYPE::DASHDOT; //TODO: update in future
    case LINESTYLE::DOT:        return PLOT_DASH_TYPE::DOT;
    case LINESTYLE::SOLID:      return PLOT_DASH_TYPE::SOLID;
    default:                    return PLOT_DASH_TYPE::DEFAULT;
    }
    // clang-format on

    return PLOT_DASH_TYPE();
}


CADSTAR_SCH_ARCHIVE_LOADER::TEXTCODE CADSTAR_SCH_ARCHIVE_LOADER::getTextCode(
        const TEXTCODE_ID& aCadstarTextCodeID )
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
             wxEmptyString );

    return Assignments.Codedefs.AttributeNames.at( aCadstarAttributeID ).Name;
}


CADSTAR_SCH_ARCHIVE_LOADER::PART CADSTAR_SCH_ARCHIVE_LOADER::getPart(
        const PART_ID& aCadstarPartID )
{
    wxCHECK( Parts.PartDefinitions.find( aCadstarPartID ) != Parts.PartDefinitions.end(), PART() );

    return Parts.PartDefinitions.at( aCadstarPartID );
}


CADSTAR_SCH_ARCHIVE_LOADER::ROUTECODE CADSTAR_SCH_ARCHIVE_LOADER::getRouteCode(
        const ROUTECODE_ID& aCadstarRouteCodeID )
{
    wxCHECK( Assignments.Codedefs.RouteCodes.find( aCadstarRouteCodeID )
                     != Assignments.Codedefs.RouteCodes.end(),
             ROUTECODE() );

    return Assignments.Codedefs.RouteCodes.at( aCadstarRouteCodeID );
}


CADSTAR_SCH_ARCHIVE_LOADER::PART::DEFINITION::PIN CADSTAR_SCH_ARCHIVE_LOADER::getPartDefinitionPin(
        const PART& aCadstarPart, const GATE_ID& aGateID, const TERMINAL_ID& aTerminalID )
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


ELECTRICAL_PINTYPE CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPinType( const PART::PIN_TYPE& aPinType )
{
    switch( aPinType )
    {
    case PART::PIN_TYPE::UNCOMMITTED:        return ELECTRICAL_PINTYPE::PT_PASSIVE;
    case PART::PIN_TYPE::INPUT:              return ELECTRICAL_PINTYPE::PT_INPUT;
    case PART::PIN_TYPE::OUTPUT_OR:          return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
    case PART::PIN_TYPE::OUTPUT_NOT_OR:      return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case PART::PIN_TYPE::OUTPUT_NOT_NORM_OR: return ELECTRICAL_PINTYPE::PT_OUTPUT;
    case PART::PIN_TYPE::POWER:              return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case PART::PIN_TYPE::GROUND:             return ELECTRICAL_PINTYPE::PT_POWER_IN;
    case PART::PIN_TYPE::TRISTATE_BIDIR:     return ELECTRICAL_PINTYPE::PT_BIDI;
    case PART::PIN_TYPE::TRISTATE_INPUT:     return ELECTRICAL_PINTYPE::PT_INPUT;
    case PART::PIN_TYPE::TRISTATE_DRIVER:    return ELECTRICAL_PINTYPE::PT_OUTPUT;
    }

    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}

int CADSTAR_SCH_ARCHIVE_LOADER::getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID )
{
    if( aCadstarGateID.IsEmpty() )
        return 1;

    return (int) aCadstarGateID.Upper().GetChar( 0 ) - (int) wxUniChar( 'A' ) + 1;
}


LABEL_SPIN_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getSpinStyle( const long long& aCadstarOrientation,
                                                           bool aMirror )
{
    double           orientationDeciDegree = getAngleTenthDegree( aCadstarOrientation );
    LABEL_SPIN_STYLE spinStyle             = getSpinStyleDeciDeg( orientationDeciDegree );

    if( aMirror )
    {
        spinStyle = spinStyle.RotateCCW();
        spinStyle = spinStyle.RotateCCW();
    }

    return spinStyle;
}


LABEL_SPIN_STYLE CADSTAR_SCH_ARCHIVE_LOADER::getSpinStyleDeciDeg(
        const double& aOrientationDeciDeg )
{
    LABEL_SPIN_STYLE spinStyle = LABEL_SPIN_STYLE::LEFT;

    int oDeg = (int) NormalizeAngle180( aOrientationDeciDeg );

    if( oDeg >= -450 && oDeg <= 450 )
        spinStyle = LABEL_SPIN_STYLE::RIGHT; // 0deg
    else if( oDeg >= 450 && oDeg <= 1350 )
        spinStyle = LABEL_SPIN_STYLE::UP; // 90deg
    else if( oDeg >= 1350 || oDeg <= -1350 )
        spinStyle = LABEL_SPIN_STYLE::LEFT; // 180deg
    else
        spinStyle = LABEL_SPIN_STYLE::BOTTOM; // 270deg

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


void CADSTAR_SCH_ARCHIVE_LOADER::applyTextSettings( EDA_TEXT*            aKiCadTextItem,
                                                    const TEXTCODE_ID&   aCadstarTextCodeID,
                                                    const ALIGNMENT&     aCadstarAlignment,
                                                    const JUSTIFICATION& aCadstarJustification,
                                                    const long long      aCadstarOrientAngle,
                                                    bool                 aMirrored )
{
    // Justification ignored for now as not supported in Eeschema, but leaving this code in
    // place for future upgrades.
    // TODO update this when Eeschema supports justification independent of anchor position.

    TEXTCODE textCode = getTextCode( aCadstarTextCodeID );
    int      textHeight = KiROUND( (double) getKiCadLength( textCode.Height ) * TXT_HEIGHT_RATIO );
    int      textWidth = getKiCadLength( textCode.Width );

    // Ensure we have no Cadstar overbar characters
    wxString escapedText = HandleTextOverbar( aKiCadTextItem->GetText() );
    aKiCadTextItem->SetText( escapedText );

    // The width is zero for all non-cadstar fonts. Using a width equal to 2/3 the height seems
    // to work well for most fonts.
    if( textWidth == 0 )
        textWidth = getKiCadLength( 2 * textCode.Height / 3 );

    aKiCadTextItem->SetTextWidth( textWidth );
    aKiCadTextItem->SetTextHeight( textHeight );
    aKiCadTextItem->SetTextThickness( getKiCadLength( textCode.LineWidth ) );
    aKiCadTextItem->SetTextAngle( getAngleTenthDegree( aCadstarOrientAngle ) );
    aKiCadTextItem->SetBold( textCode.Font.Modifier1 == FONT_BOLD );
    aKiCadTextItem->SetItalic( textCode.Font.Italic );

    ALIGNMENT textAlignment = aCadstarAlignment;

    // KiCad mirrors the justification and alignment when the symbol is mirrored but CADSTAR
    // specifies it post-mirroring. In contrast, if the text item itself is mirrored (not
    // supported in KiCad), CADSTAR specifies the alignment and justification pre-mirroring
    if( aMirrored )
        textAlignment = mirrorX( aCadstarAlignment );

    auto setAlignment = [&]( EDA_TEXT* aText, ALIGNMENT aAlignment )
                        {
                            switch( aAlignment )
                            {
                            case ALIGNMENT::NO_ALIGNMENT: // Bottom left of the first line
                                //No exact KiCad equivalent, so lets move the position of the text
                                FixTextPositionNoAlignment( aText );
                                KI_FALLTHROUGH;
                            case ALIGNMENT::BOTTOMLEFT:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                                break;

                            case ALIGNMENT::BOTTOMCENTER:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
                                break;

                            case ALIGNMENT::BOTTOMRIGHT:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                                break;

                            case ALIGNMENT::CENTERLEFT:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                                break;

                            case ALIGNMENT::CENTERCENTER:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
                                break;

                            case ALIGNMENT::CENTERRIGHT:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                                break;

                            case ALIGNMENT::TOPLEFT:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                                break;

                            case ALIGNMENT::TOPCENTER:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
                                break;

                            case ALIGNMENT::TOPRIGHT:
                                aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                                aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                                break;
                            }
                        };

    LABEL_SPIN_STYLE spin = getSpinStyle( aCadstarOrientAngle, aMirrored );
    EDA_ITEM* textEdaItem = dynamic_cast<EDA_ITEM*>( aKiCadTextItem );
    wxCHECK( textEdaItem, /* void */ ); // ensure this is a EDA_ITEM

    switch( textEdaItem->Type() )
    {
    // Some KiCad schematic text items only permit a limited amount of angles
    // and text justifications
    case LIB_TEXT_T:
    case SCH_FIELD_T:
    case LIB_FIELD_T:
    {
        // Spin style not used. All text justifications are permitted. However, only orientations
        // of 0 deg or 90 deg are supported
        double angleDeciDeg = NormalizeAnglePos( aKiCadTextItem->GetTextAngle() );
        int    quadrant = KiROUND( angleDeciDeg / 900.0 );
        quadrant %= 4;

        switch( quadrant )
        {
        case 0:
            angleDeciDeg = 0;
            break;
        case 1:
            angleDeciDeg = 900;
            break;
        case 2:
            angleDeciDeg = 0;
            textAlignment = rotate180( textAlignment );
            break;
        case 3:
            angleDeciDeg = 900;
            textAlignment = rotate180( textAlignment );
            break;
        default: wxFAIL_MSG( "Unknown Quadrant" );
        }

        aKiCadTextItem->SetTextAngle( angleDeciDeg );
        setAlignment( aKiCadTextItem, textAlignment );
    }
        return;

    case SCH_TEXT_T:
    {
        // Note spin style in a SCH_TEXT results in a vertical alignment GR_TEXT_VJUSTIFY_BOTTOM
        // so need to adjust the location of the text element based on Cadstar's original text
        // alignment (anchor position).
        setAlignment( aKiCadTextItem, textAlignment );
        EDA_RECT bb = textEdaItem->GetBoundingBox();
        int      off = static_cast<SCH_TEXT*>( aKiCadTextItem )->GetTextOffset();
        wxPoint  pos;

        // Change the anchor point of the text item to make it match the same bounding box
        // And correct the error introduced by the text offsetting in KiCad
        switch( spin )
        {
        case LABEL_SPIN_STYLE::BOTTOM: pos = { bb.GetRight() - off, bb.GetTop()          }; break;
        case LABEL_SPIN_STYLE::UP:     pos = { bb.GetRight() - off, bb.GetBottom()       }; break;
        case LABEL_SPIN_STYLE::LEFT:   pos = { bb.GetRight()      , bb.GetBottom() + off }; break;
        case LABEL_SPIN_STYLE::RIGHT:  pos = { bb.GetLeft()       , bb.GetBottom() + off }; break;
        }

        aKiCadTextItem->SetTextPos( pos );
    }
        KI_FALLTHROUGH;

    // We don't want to change position of net labels as that would break connectivity
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
        static_cast<SCH_TEXT*>( aKiCadTextItem )->SetLabelSpinStyle( spin );
        return;

    default:
        wxFAIL_MSG( "Unexpected item type" );
        return;
    }
}


SCH_TEXT* CADSTAR_SCH_ARCHIVE_LOADER::getKiCadSchText( const TEXT& aCadstarTextElement )
{
    SCH_TEXT* kiTxt = new SCH_TEXT();

    kiTxt->SetParent( m_schematic ); // set to the schematic for now to avoid asserts
    kiTxt->SetPosition( getKiCadPoint( aCadstarTextElement.Position ) );
    kiTxt->SetText( aCadstarTextElement.Text );

    applyTextSettings( kiTxt,
                       aCadstarTextElement.TextCodeID,
                       aCadstarTextElement.Alignment,
                       aCadstarTextElement.Justification,
                       aCadstarTextElement.OrientAngle,
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
        [&]( wxPoint aCoord ) -> wxPoint
        {
            return wxPoint( scaleLen( aCoord.x ), scaleLen( aCoord.y ) );
        };

    auto scaleSize =
        [&]( wxSize aSize ) -> wxSize
        {
            return wxSize( scaleLen( aSize.x ), scaleLen( aSize.y ) );
        };

    LIB_ITEMS_CONTAINER& items = retval->GetDrawItems();

    for( auto& item : items )
    {
        switch( item.Type() )
        {
        case KICAD_T::LIB_ARC_T:
        {
            LIB_ARC& arc = static_cast<LIB_ARC&>( item );
            arc.SetPosition( scalePt( arc.GetPosition() ) );
            arc.SetStart( scalePt( arc.GetStart() ) );
            arc.SetEnd( scalePt( arc.GetEnd() ) );
            arc.CalcRadiusAngles(); // Maybe not needed?
        }
        break;

        case KICAD_T::LIB_POLYLINE_T:
        {
            LIB_POLYLINE& poly = static_cast<LIB_POLYLINE&>( item );

            std::vector<wxPoint> originalPts = poly.GetPolyPoints();
            poly.ClearPoints();

            for( wxPoint& pt : originalPts )
                poly.AddPoint( scalePt( pt ) );
        }
        break;

        case KICAD_T::LIB_PIN_T:
        {
            LIB_PIN& pin = static_cast<LIB_PIN&>( item );

            pin.SetPosition( scalePt( pin.GetPosition() ) );
            pin.SetLength( scaleLen( pin.GetLength() ) );
        }
        break;

        case KICAD_T::LIB_TEXT_T:
        {
            LIB_TEXT& txt = static_cast<LIB_TEXT&>( item );

            txt.SetPosition( scalePt( txt.GetPosition() ) );
            txt.SetTextSize( scaleSize( txt.GetTextSize() ) );
        }
        break;

        default: break;
        }

    }

    return retval;
}


void CADSTAR_SCH_ARCHIVE_LOADER::fixUpLibraryPins( LIB_SYMBOL* aSymbolToFix, int aGateNumber )
{
    // Store a list of segments that are not connected to other segments and are vertical or
    // horizontal.
    std::map<wxPoint, LIB_POLYLINE*> twoPointUniqueSegments;

    LIB_ITEMS_CONTAINER::ITERATOR polylineiter =
            aSymbolToFix->GetDrawItems().begin( LIB_POLYLINE_T );

    for( ; polylineiter != aSymbolToFix->GetDrawItems().end( LIB_POLYLINE_T ); ++polylineiter )
    {
        LIB_POLYLINE& polyline = static_cast<LIB_POLYLINE&>( *polylineiter );

        if( aGateNumber > 0 && polyline.GetUnit() != aGateNumber )
            continue;

        const std::vector<wxPoint>& pts = polyline.GetPolyPoints();

        bool isUnique = true;

        auto removeSegment =
            [&]( LIB_POLYLINE* aLineToRemove )
            {
                twoPointUniqueSegments.erase( aLineToRemove->GetPolyPoints().at( 0 ) );
                twoPointUniqueSegments.erase( aLineToRemove->GetPolyPoints().at( 1 ) );
                isUnique = false;
            };

        if( pts.size() == 2 )
        {
            const wxPoint& pt0 = pts.at( 0 );
            const wxPoint& pt1 = pts.at( 1 );

            if( twoPointUniqueSegments.count( pt0 ) )
                removeSegment( twoPointUniqueSegments.at( pt0 ) );

            if( twoPointUniqueSegments.count( pt1 ) )
                removeSegment( twoPointUniqueSegments.at( pt1 ) );

            if( isUnique && pt0 != pt1 )
            {
                if( pt0.x == pt1.x || pt0.y == pt1.y )
                {
                    twoPointUniqueSegments.insert( { pts.at( 0 ), &polyline } );
                    twoPointUniqueSegments.insert( { pts.at( 1 ), &polyline } );
                }
            }
        }
    }

    LIB_PINS pins;
    aSymbolToFix->GetPins( pins, aGateNumber );

    for( auto& pin : pins )
    {
        auto setPinOrientation =
            [&]( double aAngleRad )
            {
                int oDeg = (int) NormalizeAngle180( RAD2DEG( aAngleRad ) );

                if( oDeg >= -45 && oDeg <= 45 )
                    pin->SetOrientation( 'R' ); // 0 degrees
                else if( oDeg >= 45 && oDeg <= 135 )
                    pin->SetOrientation( 'U' ); // 90 degrees
                else if( oDeg >= 135 || oDeg <= -135 )
                    pin->SetOrientation( 'L' ); // 180 degrees
                else
                    pin->SetOrientation( 'D' ); // -90 degrees
            };

        if( twoPointUniqueSegments.count( pin->GetPosition() ) )
        {
            LIB_POLYLINE* poly = twoPointUniqueSegments.at( pin->GetPosition() );

            wxPoint otherPt = poly->GetPolyPoints().at( 0 );

            if( otherPt == pin->GetPosition() )
                otherPt = poly->GetPolyPoints().at( 1 );

            VECTOR2I vec( otherPt - pin->GetPosition() );

            pin->SetLength( vec.EuclideanNorm() );
            setPinOrientation( vec.Angle() );
        }
    }
}


std::pair<wxPoint, wxSize> CADSTAR_SCH_ARCHIVE_LOADER::getFigureExtentsKiCad(
        const FIGURE& aCadstarFigure )
{
    wxPoint upperLeft( Assignments.Settings.DesignLimit.x, 0 );
    wxPoint lowerRight( 0, Assignments.Settings.DesignLimit.y );

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

    wxPoint upperLeftKiCad  = getKiCadPoint( upperLeft );
    wxPoint lowerRightKiCad = getKiCadPoint( lowerRight );

    wxPoint size = lowerRightKiCad - upperLeftKiCad;

    return { upperLeftKiCad, wxSize( abs( size.x ), abs( size.y ) ) };
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPoint( const wxPoint& aCadstarPoint )
{
    wxPoint retval;

    retval.x = getKiCadLength( aCadstarPoint.x - m_designCenter.x );
    retval.y = -getKiCadLength( aCadstarPoint.y - m_designCenter.y );

    return retval;
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::getKiCadLibraryPoint( const wxPoint& aCadstarPoint,
                                                          const wxPoint& aCadstarCentre )
{
    wxPoint retval;

    retval.x = getKiCadLength( aCadstarPoint.x - aCadstarCentre.x );
    retval.y = getKiCadLength( aCadstarPoint.y - aCadstarCentre.y );

    return retval;
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::applyTransform( const wxPoint& aPoint,
        const wxPoint& aMoveVector, const double& aRotationAngleDeciDeg,
        const double& aScalingFactor, const wxPoint& aTransformCentre, const bool& aMirrorInvert )
{
    wxPoint retVal = aPoint;

    if( aScalingFactor != 1.0 )
    {
        //scale point
        retVal -= aTransformCentre;
        retVal.x = KiROUND( retVal.x * aScalingFactor );
        retVal.y = KiROUND( retVal.y * aScalingFactor );
        retVal += aTransformCentre;
    }

    if( aMirrorInvert )
    {
        MIRROR( retVal.x, aTransformCentre.x );
    }

    if( aRotationAngleDeciDeg != 0.0 )
    {
        RotatePoint( &retVal, aTransformCentre, aRotationAngleDeciDeg );
    }

    if( aMoveVector != wxPoint{ 0, 0 } )
    {
        retVal += aMoveVector;
    }

    return retVal;
}


double CADSTAR_SCH_ARCHIVE_LOADER::getPolarAngle( const wxPoint& aPoint )
{
    return NormalizeAnglePos( ArcTangente( aPoint.y, aPoint.x ) );
}


double CADSTAR_SCH_ARCHIVE_LOADER::getPolarRadius( const wxPoint& aPoint )
{
    return sqrt(
            ( (double) aPoint.x * (double) aPoint.x ) + ( (double) aPoint.y * (double) aPoint.y ) );
}
