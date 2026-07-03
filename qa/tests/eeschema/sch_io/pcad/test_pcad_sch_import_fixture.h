/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TEST_PCAD_SCH_IMPORT_FIXTURE_H
#define TEST_PCAD_SCH_IMPORT_FIXTURE_H

/**
 * @file test_pcad_sch_import_fixture.h
 * Shared fixture and helpers for the P-CAD schematic import test suite.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema/sch_io/pcad/sch_io_pcad.h>

#include <lib_symbol.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <title_block.h>

#include <memory>
#include <set>


struct PCAD_SCH_IMPORT_FIXTURE
{
    PCAD_SCH_IMPORT_FIXTURE() : m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    SCH_IO_PCAD                m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
    SCH_SHEET*                 m_loadedRoot = nullptr;

    std::string GetTestDataDir() { return KI_TEST::GetEeschemaTestDataDir() + "io/pcad/"; }

    void LoadSchematic( const std::string& aFile )
    {
        m_loadedRoot = m_plugin.LoadSchematicFile( GetTestDataDir() + aFile, m_schematic.get() );
        BOOST_REQUIRE( m_loadedRoot != nullptr );
    }

    int CountItemsOfType( KICAD_T aType )
    {
        std::set<SCH_SCREEN*> seenScreens;
        int                   count = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == aType )
                    count++;
            }
        }

        return count;
    }

    int CountImportedScreens()
    {
        std::set<SCH_SCREEN*> seenScreens;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( SCH_SCREEN* screen = sheetPath.LastScreen() )
                seenScreens.insert( screen );
        }

        return static_cast<int>( seenScreens.size() );
    }

    int CountWires()
    {
        int count = 0;

        forEachItemOfType( SCH_LINE_T,
                [&count]( SCH_ITEM* item, const SCH_SHEET_PATH& )
                {
                    if( static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE )
                        count++;
                } );

        return count;
    }

    int CountBusSegments()
    {
        int count = 0;

        forEachItemOfType( SCH_LINE_T,
                [&count]( SCH_ITEM* item, const SCH_SHEET_PATH& )
                {
                    if( static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_BUS )
                        count++;
                } );

        return count;
    }

    /// The embedded library symbol of the first placed symbol matching the reference.
    LIB_SYMBOL* LibSymbolForRefdes( const wxString& aRefdes )
    {
        LIB_SYMBOL* result = nullptr;

        forEachItemOfType( SCH_SYMBOL_T,
                [&]( SCH_ITEM* item, const SCH_SHEET_PATH& sheetPath )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                    if( !result && symbol->GetRef( &sheetPath, false ) == aRefdes )
                        result = symbol->GetLibSymbolRef().get();
                } );

        return result;
    }

    SCH_SYMBOL* SymbolForRefdesUnit( const wxString& aRefdes, int aUnit )
    {
        SCH_SYMBOL* result = nullptr;

        forEachItemOfType( SCH_SYMBOL_T,
                [&]( SCH_ITEM* item, const SCH_SHEET_PATH& sheetPath )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                    if( !result && symbol->GetRef( &sheetPath, false ) == aRefdes
                        && symbol->GetUnit() == aUnit )
                    {
                        result = symbol;
                    }
                } );

        return result;
    }

    /// The library pin with the given number, searched across all units.
    SCH_PIN* LibPin( LIB_SYMBOL* aSymbol, const wxString& aNumber )
    {
        if( !aSymbol )
            return nullptr;

        for( SCH_PIN* pin : aSymbol->GetPins() )
        {
            if( pin->GetNumber() == aNumber )
                return pin;
        }

        return nullptr;
    }

    bool HasLabel( const wxString& aText, KICAD_T aType = SCH_LABEL_T )
    {
        bool found = false;

        forEachItemOfType( aType,
                [&]( SCH_ITEM* item, const SCH_SHEET_PATH& )
                {
                    if( static_cast<SCH_LABEL_BASE*>( item )->GetText() == aText )
                        found = true;
                } );

        return found;
    }

    SCH_TEXT* FindText( const wxString& aText )
    {
        SCH_TEXT* result = nullptr;

        forEachItemOfType( SCH_TEXT_T,
                [&]( SCH_ITEM* item, const SCH_SHEET_PATH& )
                {
                    if( !result && static_cast<SCH_TEXT*>( item )->GetText() == aText )
                        result = static_cast<SCH_TEXT*>( item );
                } );

        return result;
    }

    /// Count wire or graphic lines drawn with the given stroke style anywhere in the design.
    int CountShapesWithStyle( LINE_STYLE aStyle )
    {
        int count = 0;

        forEachItemOfType( SCH_SHAPE_T,
                [&]( SCH_ITEM* item, const SCH_SHEET_PATH& )
                {
                    if( static_cast<SCH_SHAPE*>( item )->GetStroke().GetLineStyle() == aStyle )
                        count++;
                } );

        return count;
    }

    template <typename FUNC>
    void forEachItemOfType( KICAD_T aType, FUNC aFunc )
    {
        std::set<SCH_SCREEN*> seenScreens;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( aType ) )
                aFunc( item, sheetPath );
        }
    }
};

#endif // TEST_PCAD_SCH_IMPORT_FIXTURE_H
