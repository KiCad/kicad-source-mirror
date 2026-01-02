/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_issue20173_paste_subsheet.cpp
 *
 * Test for issue #20173: Copy/paste subsheet causes original sheet's references to be corrupted.
 *
 * The test verifies that when a subsheet is copy/pasted to create a second instance sharing
 * the same screen file, the original sheet's symbol references are preserved correctly.
 *
 * Bug scenario:
 * - Start with a root sheet containing subsheet CH1 with symbols R1, R2, R3, R4, D1, D2
 * - Copy/paste CH1 to create CH2 (which shares the same screen as CH1)
 * - CH2 gets new references: R5...R8, D3, D4 (via annotation)
 * - BUG: When navigating back to CH1, references show R5...R8, D3, D4 instead of R1...R4, D1, D2
 *
 * Root cause: When a shared screen is used, symbol instances for the original path may be
 * incorrectly modified or lost during the paste operation.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ISSUE20173_FIXTURE
{
    ISSUE20173_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Helper function to get all symbol references for a given sheet path.
 * Returns a map of symbol UUID -> reference designator.
 */
std::map<KIID, wxString> GetSymbolReferences( const SCH_SHEET_PATH& aPath )
{
    std::map<KIID, wxString> refs;

    if( !aPath.LastScreen() )
        return refs;

    for( SCH_ITEM* item : aPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        // Skip power symbols (references starting with #)
        wxString ref = symbol->GetRef( &aPath );

        if( !ref.IsEmpty() && ref[0] != '#' )
            refs[symbol->m_Uuid] = ref;
    }

    return refs;
}


/**
 * Test that simulates the paste flow more closely, including instance pruning.
 *
 * This test specifically checks whether the PruneOrphanedSymbolInstances call
 * after paste incorrectly removes the original sheet's instances.
 */
BOOST_FIXTURE_TEST_CASE( Issue20173PruningAfterPaste, ISSUE20173_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue20173/issue20173", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Find the ch1 subsheet
    SCH_SHEET_PATH ch1Path;
    SCH_SHEET*     ch1Sheet = nullptr;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 )
        {
            ch1Path = path;
            ch1Sheet = path.Last();
            break;
        }
    }

    BOOST_REQUIRE( ch1Sheet != nullptr );

    SCH_SCREEN* sharedScreen = ch1Sheet->GetScreen();
    BOOST_REQUIRE( sharedScreen != nullptr );

    // Set up proper instances for ch1 (in case the loaded data has stale UUIDs)
    std::map<KIID, wxString> originalRefs;

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = symbol->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref.IsEmpty() || ref[0] == '#' )
            continue;

        symbol->SetRef( &ch1Path, ref );
        originalRefs[symbol->m_Uuid] = ref;
    }

    // Create ch2 with shared screen (simulating paste)
    SCH_SHEET* ch2Sheet = new SCH_SHEET( m_schematic.get() );
    ch2Sheet->GetField( FIELD_T::SHEET_NAME )->SetText( "ch2" );
    ch2Sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( ch1Sheet->GetFileName() );
    ch2Sheet->SetScreen( sharedScreen );
    ch2Sheet->SetPosition( ch1Sheet->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 1000 ), 0 ) );

    // Add to root and refresh hierarchy
    m_schematic->RootScreen()->Append( ch2Sheet );
    m_schematic->RefreshHierarchy();

    // Get updated hierarchy
    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    SCH_SHEET_PATH ch2Path;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 && path.Last() == ch2Sheet )
        {
            ch2Path = path;
            break;
        }
    }

    BOOST_REQUIRE( ch2Path.size() > 1 );

    // Add ch2 instances with different references
    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        wxString    ch1Ref = symbol->GetRef( &ch1Path );

        if( ch1Ref.IsEmpty() || ch1Ref[0] == '#' )
            continue;

        // Create ch2 reference (R1 -> R101)
        wxString ch2Ref = ch1Ref;

        if( ch2Ref.length() > 1 )
        {
            wxString prefix = ch2Ref.substr( 0, 1 );
            long     num = 0;

            if( ch2Ref.substr( 1 ).ToLong( &num ) )
                ch2Ref = prefix + wxString::Format( "%ld", num + 100 );
        }

        symbol->SetRef( &ch2Path, ch2Ref );
    }

    // Verify instances are set up correctly before pruning
    BOOST_TEST_MESSAGE( "Before pruning:" );

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &ch1Path )[0] == '#' )
            continue;

        BOOST_TEST_MESSAGE( "  " << symbol->m_Uuid.AsString() << " ch1=" << symbol->GetRef( &ch1Path )
                                 << " ch2=" << symbol->GetRef( &ch2Path ) );
        break;
    }

    // Now call the pruning that happens after paste
    // This is what SCH_EDITOR_CONTROL::Paste does at line 2615
    SCH_SCREENS allScreens( m_schematic->Root() );
    allScreens.PruneOrphanedSymbolInstances( m_schematic->Project().GetProjectName(), sheets );

    BOOST_TEST_MESSAGE( "After pruning:" );

    // Verify ch1 instances are still intact after pruning
    bool ch1InstancesIntact = true;

    for( const auto& [uuid, originalRef] : originalRefs )
    {
        // Find the symbol
        SCH_SYMBOL* symbol = nullptr;

        for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( item->m_Uuid == uuid )
            {
                symbol = static_cast<SCH_SYMBOL*>( item );
                break;
            }
        }

        if( !symbol )
            continue;

        wxString ch1RefAfter = symbol->GetRef( &ch1Path );
        wxString ch2RefAfter = symbol->GetRef( &ch2Path );

        BOOST_TEST_MESSAGE( "  " << uuid.AsString() << " ch1=" << ch1RefAfter << " ch2="
                                 << ch2RefAfter << " (original=" << originalRef << ")" );

        if( ch1RefAfter != originalRef )
        {
            BOOST_TEST_MESSAGE( "    MISMATCH: ch1 reference changed from " << originalRef << " to "
                                                                            << ch1RefAfter );
            ch1InstancesIntact = false;
        }
    }

    BOOST_CHECK_MESSAGE( ch1InstancesIntact,
                         "CH1 instances should be preserved after paste and pruning" );
}


/**
 * Test that ClearAnnotation doesn't corrupt REFERENCE field in a way that affects GetRef.
 *
 * ClearAnnotation modifies both the instance reference AND the REFERENCE field text.
 * If GetRef falls back to the field text (when instance doesn't match), this can cause issues.
 */
BOOST_FIXTURE_TEST_CASE( Issue20173ClearAnnotationFieldCorruption, ISSUE20173_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue20173/issue20173", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Find the ch1 subsheet
    SCH_SHEET_PATH ch1Path;
    SCH_SHEET*     ch1Sheet = nullptr;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 )
        {
            ch1Path = path;
            ch1Sheet = path.Last();
            break;
        }
    }

    BOOST_REQUIRE( ch1Sheet != nullptr );

    SCH_SCREEN* sharedScreen = ch1Sheet->GetScreen();
    BOOST_REQUIRE( sharedScreen != nullptr );

    // Set up proper instances for ch1 (fix any stale UUID issues from loading)
    std::map<KIID, wxString> ch1OriginalRefs;

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = symbol->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref.IsEmpty() || ref[0] == '#' )
            continue;

        symbol->SetRef( &ch1Path, ref );
        ch1OriginalRefs[symbol->m_Uuid] = ref;
    }

    // Create ch2 with shared screen
    SCH_SHEET* ch2Sheet = new SCH_SHEET( m_schematic.get() );
    ch2Sheet->GetField( FIELD_T::SHEET_NAME )->SetText( "ch2" );
    ch2Sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( ch1Sheet->GetFileName() );
    ch2Sheet->SetScreen( sharedScreen );

    m_schematic->RootScreen()->Append( ch2Sheet );
    m_schematic->RefreshHierarchy();

    sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    SCH_SHEET_PATH ch2Path;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 && path.Last() == ch2Sheet )
        {
            ch2Path = path;
            break;
        }
    }

    BOOST_REQUIRE( ch2Path.size() > 1 );

    // Add ch2 instances and call ClearAnnotation (simulating paste with !forceKeepAnnotations)
    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        wxString    ch1Ref = symbol->GetRef( &ch1Path );

        if( ch1Ref.IsEmpty() || ch1Ref[0] == '#' )
            continue;

        // Add instance for ch2 (like updatePastedSymbol does)
        symbol->SetRef( &ch2Path, ch1Ref );

        // Now call ClearAnnotation for ch2 path (like updatePastedSymbol does when !forceKeepAnnotations)
        symbol->ClearAnnotation( &ch2Path, false );
    }

    // Verify ch1 references are unchanged after ClearAnnotation on ch2
    bool ch1RefsPreserved = true;

    for( const auto& [uuid, originalRef] : ch1OriginalRefs )
    {
        SCH_SYMBOL* symbol = nullptr;

        for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( item->m_Uuid == uuid )
            {
                symbol = static_cast<SCH_SYMBOL*>( item );
                break;
            }
        }

        if( !symbol )
            continue;

        wxString ch1RefAfter = symbol->GetRef( &ch1Path );

        if( ch1RefAfter != originalRef )
        {
            BOOST_TEST_MESSAGE( "Symbol " << uuid.AsString() << " ch1 reference changed from "
                                          << originalRef << " to " << ch1RefAfter
                                          << " after ClearAnnotation on ch2" );
            ch1RefsPreserved = false;
        }
    }

    BOOST_CHECK_MESSAGE( ch1RefsPreserved,
                         "CH1 references should be preserved after ClearAnnotation on CH2" );
}


/**
 * Test that reproduces the actual bug: ClearAnnotation corrupts field text.
 *
 * When ClearAnnotation is called for a specific path (CH2), it incorrectly modifies
 * the REFERENCE field text. If GetRef later falls back to field text (due to
 * mismatched instance paths), it returns the corrupted value.
 */
BOOST_FIXTURE_TEST_CASE( Issue20173FieldTextCorruption, ISSUE20173_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue20173/issue20173", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Find the ch1 subsheet
    SCH_SHEET_PATH ch1Path;
    SCH_SHEET*     ch1Sheet = nullptr;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 )
        {
            ch1Path = path;
            ch1Sheet = path.Last();
            break;
        }
    }

    BOOST_REQUIRE( ch1Sheet != nullptr );

    SCH_SCREEN* sharedScreen = ch1Sheet->GetScreen();
    BOOST_REQUIRE( sharedScreen != nullptr );

    // Capture original field text values (these are what GetRef falls back to)
    std::map<KIID, wxString> originalFieldText;

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = symbol->GetField( FIELD_T::REFERENCE )->GetText();

        if( !ref.IsEmpty() && ref[0] != '#' )
            originalFieldText[symbol->m_Uuid] = ref;
    }

    BOOST_REQUIRE( originalFieldText.size() > 0 );

    BOOST_TEST_MESSAGE( "Original field text values:" );

    for( const auto& [uuid, ref] : originalFieldText )
        BOOST_TEST_MESSAGE( "  " << uuid.AsString() << " -> " << ref );

    // Create ch2 with shared screen (simulating paste)
    SCH_SHEET* ch2Sheet = new SCH_SHEET( m_schematic.get() );
    ch2Sheet->GetField( FIELD_T::SHEET_NAME )->SetText( "ch2" );
    ch2Sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( ch1Sheet->GetFileName() );
    ch2Sheet->SetScreen( sharedScreen );

    m_schematic->RootScreen()->Append( ch2Sheet );
    m_schematic->RefreshHierarchy();

    sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    SCH_SHEET_PATH ch2Path;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 && path.Last() == ch2Sheet )
        {
            ch2Path = path;
            break;
        }
    }

    BOOST_REQUIRE( ch2Path.size() > 1 );

    // Simulate what updatePastedSymbol does:
    // 1. Add instance for new path
    // 2. Call ClearAnnotation for new path
    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        wxString    fieldRef = symbol->GetField( FIELD_T::REFERENCE )->GetText();

        if( fieldRef.IsEmpty() || fieldRef[0] == '#' )
            continue;

        // Add instance for ch2 (like AddHierarchicalReference does)
        symbol->SetRef( &ch2Path, fieldRef );

        // Now call ClearAnnotation for ch2 path (like updatePastedSymbol does)
        symbol->ClearAnnotation( &ch2Path, false );
    }

    // Check if field text is corrupted
    BOOST_TEST_MESSAGE( "Field text after ClearAnnotation on ch2:" );

    bool fieldTextCorrupted = false;

    for( const auto& [uuid, originalRef] : originalFieldText )
    {
        SCH_SYMBOL* symbol = nullptr;

        for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( item->m_Uuid == uuid )
            {
                symbol = static_cast<SCH_SYMBOL*>( item );
                break;
            }
        }

        if( !symbol )
            continue;

        wxString currentFieldText = symbol->GetField( FIELD_T::REFERENCE )->GetText();

        BOOST_TEST_MESSAGE( "  " << uuid.AsString() << " -> " << currentFieldText
                                 << " (was " << originalRef << ")" );

        if( currentFieldText != originalRef )
        {
            fieldTextCorrupted = true;
            BOOST_TEST_MESSAGE( "    CORRUPTED!" );
        }
    }

    // The bug: ClearAnnotation modifies field text even when called for a specific path
    // This causes GetRef to return wrong value when falling back to field text
    BOOST_CHECK_MESSAGE( !fieldTextCorrupted,
                         "Field text should NOT be corrupted by ClearAnnotation on different path" );
}


/**
 * Test that symbols on a shared screen maintain independent instances for each sheet path.
 *
 * This tests the fundamental requirement that symbols can have different reference
 * designators in different sheet instances, even when sharing the same screen.
 */
BOOST_FIXTURE_TEST_CASE( Issue20173SharedScreenInstances, ISSUE20173_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue20173/issue20173", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Find the ch1 subsheet
    SCH_SHEET_PATH ch1Path;
    SCH_SHEET*     ch1Sheet = nullptr;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 )
        {
            ch1Path = path;
            ch1Sheet = path.Last();
            break;
        }
    }

    BOOST_REQUIRE( ch1Sheet != nullptr );

    SCH_SCREEN* sharedScreen = ch1Sheet->GetScreen();
    BOOST_REQUIRE( sharedScreen != nullptr );

    // Get root path for adding new sheet
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( &m_schematic->Root() );

    // Create ch2 with shared screen
    SCH_SHEET* ch2Sheet = new SCH_SHEET( m_schematic.get() );
    ch2Sheet->GetField( FIELD_T::SHEET_NAME )->SetText( "ch2" );
    ch2Sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( ch1Sheet->GetFileName() );
    ch2Sheet->SetScreen( sharedScreen );
    ch2Sheet->SetPosition( ch1Sheet->GetPosition() + VECTOR2I( schIUScale.MilsToIU( 1000 ), 0 ) );

    // Manually add ch2 to root screen
    m_schematic->RootScreen()->Append( ch2Sheet );
    m_schematic->RefreshHierarchy();

    // Rebuild sheets after adding ch2
    sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    // Find ch2 path from the hierarchy (don't construct manually)
    SCH_SHEET_PATH ch2Path;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() > 1 && path.Last() == ch2Sheet )
        {
            ch2Path = path;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( ch2Path.size() > 1, "Should find ch2 in hierarchy" );

    // First, ensure all symbols have proper instances for ch1Path
    // (The loaded schematic might have stale instance data with old sheet UUIDs)
    int symbolIndex = 0;

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        // Skip power symbols
        wxString currentRef = symbol->GetField( FIELD_T::REFERENCE )->GetText();

        if( currentRef.IsEmpty() || currentRef[0] == '#' )
            continue;

        // Set up proper ch1 reference using the current field text
        symbol->SetRef( &ch1Path, currentRef );
        symbolIndex++;
    }

    BOOST_TEST_MESSAGE( "Set up " << symbolIndex << " symbols with ch1 instances" );

    // Now set different references for ch2
    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxString ch1Ref = symbol->GetRef( &ch1Path );

        // Skip power symbols
        if( ch1Ref.IsEmpty() || ch1Ref[0] == '#' )
            continue;

        // Add a different reference for ch2 (R1 -> R101, D1 -> D101, etc.)
        wxString ch2Ref = ch1Ref;

        if( ch2Ref.length() > 1 )
        {
            wxString prefix = ch2Ref.substr( 0, 1 );
            long     num = 0;

            if( ch2Ref.substr( 1 ).ToLong( &num ) )
                ch2Ref = prefix + wxString::Format( "%ld", num + 100 );
        }

        symbol->SetRef( &ch2Path, ch2Ref );

        // Verify ch1 reference is unchanged
        wxString ch1RefAfter = symbol->GetRef( &ch1Path );
        BOOST_CHECK_MESSAGE( ch1Ref == ch1RefAfter,
                             "Setting ch2 reference should not change ch1 reference for symbol "
                                     << symbol->m_Uuid.AsString() );

        // Verify ch2 has its own reference
        wxString ch2RefAfter = symbol->GetRef( &ch2Path );
        BOOST_CHECK_MESSAGE( ch2Ref == ch2RefAfter,
                             "ch2 should have its own reference for symbol "
                                     << symbol->m_Uuid.AsString() );
    }

    // Debug: Print ch1 and ch2 paths
    BOOST_TEST_MESSAGE( "ch1Path: " << ch1Path.Path().AsString() );
    BOOST_TEST_MESSAGE( "ch2Path: " << ch2Path.Path().AsString() );

    // Debug: Print instances for first non-power symbol before UpdateAllScreenReferences
    BOOST_TEST_MESSAGE( "Before UpdateAllScreenReferences:" );

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &ch1Path )[0] == '#' )
            continue;

        BOOST_TEST_MESSAGE( "Symbol " << symbol->m_Uuid.AsString() << " instances:" );

        for( const SCH_SYMBOL_INSTANCE& inst : symbol->GetInstances() )
        {
            BOOST_TEST_MESSAGE( "  Path: " << inst.m_Path.AsString() << " -> " << inst.m_Reference );
        }

        BOOST_TEST_MESSAGE( "  Field text: " << symbol->GetField( FIELD_T::REFERENCE )->GetText() );
        BOOST_TEST_MESSAGE( "  GetRef(ch1): " << symbol->GetRef( &ch1Path ) );
        BOOST_TEST_MESSAGE( "  GetRef(ch2): " << symbol->GetRef( &ch2Path ) );
        break;  // Only print first symbol for brevity
    }

    // Now test UpdateAllScreenReferences doesn't corrupt the other instance
    ch2Path.UpdateAllScreenReferences();

    BOOST_TEST_MESSAGE( "After ch2Path.UpdateAllScreenReferences():" );

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &ch1Path )[0] == '#' )
            continue;

        BOOST_TEST_MESSAGE( "  Field text: " << symbol->GetField( FIELD_T::REFERENCE )->GetText() );
        BOOST_TEST_MESSAGE( "  GetRef(ch1): " << symbol->GetRef( &ch1Path ) );
        BOOST_TEST_MESSAGE( "  GetRef(ch2): " << symbol->GetRef( &ch2Path ) );
        break;
    }

    ch1Path.UpdateAllScreenReferences();

    BOOST_TEST_MESSAGE( "After ch1Path.UpdateAllScreenReferences():" );

    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &ch1Path )[0] == '#' )
            continue;

        BOOST_TEST_MESSAGE( "  Field text: " << symbol->GetField( FIELD_T::REFERENCE )->GetText() );
        BOOST_TEST_MESSAGE( "  GetRef(ch1): " << symbol->GetRef( &ch1Path ) );
        BOOST_TEST_MESSAGE( "  GetRef(ch2): " << symbol->GetRef( &ch2Path ) );
        break;
    }

    // Verify references are still correct after switching
    for( SCH_ITEM* item : sharedScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &ch1Path )[0] == '#' )
            continue;

        // Check that instances exist and are different
        wxString ch1Ref = symbol->GetRef( &ch1Path );
        wxString ch2Ref = symbol->GetRef( &ch2Path );

        // Since we set ch2 = ch1 + 100, they should be different
        BOOST_CHECK_MESSAGE( ch1Ref != ch2Ref,
                             "ch1 and ch2 should have different references after navigation" );
    }
}
