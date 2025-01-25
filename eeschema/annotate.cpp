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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>

#include <confirm.h>
#include <project/project_file.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <sch_commit.h>
#include <erc/erc_settings.h>
#include <sch_reference_list.h>
#include <tools/sch_selection.h>
#include <tools/sch_selection_tool.h>
#include <tool/tool_manager.h>
#include <dialog_erc.h>

void SCH_EDIT_FRAME::mapExistingAnnotation( std::map<wxString, wxString>& aMap )
{
    SCH_REFERENCE_LIST references;
    Schematic().Hierarchy().GetSymbols( references );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_SYMBOL*     symbol = references[ i ].GetSymbol();
        SCH_SHEET_PATH* curr_sheetpath = &references[ i ].GetSheetPath();
        KIID_PATH       curr_full_uuid = curr_sheetpath->Path();

        curr_full_uuid.push_back( symbol->m_Uuid );

        wxString ref = symbol->GetRef( curr_sheetpath, true );

        if( symbol->IsAnnotated( curr_sheetpath ) )
            aMap[ curr_full_uuid.AsString() ] = ref;
    }
}


void SCH_EDIT_FRAME::DeleteAnnotation( ANNOTATE_SCOPE_T aAnnotateScope, bool aRecursive,
                                       REPORTER& aReporter )
{

    SCH_SHEET_LIST sheets = Schematic().Hierarchy();
    SCH_SCREEN*    screen = GetScreen();
    SCH_SHEET_PATH currentSheet = GetCurrentSheet();
    SCH_COMMIT     commit( this );

    auto clearSymbolAnnotation =
            [&]( EDA_ITEM* aItem, SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet, bool aResetPrefixes )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( aItem );
                commit.Modify( aItem, aScreen );

                // aSheet == nullptr means all sheets
                if( !aSheet || symbol->IsAnnotated( aSheet ) )
                {
                    wxString msg;

                    if( symbol->GetUnitCount() > 1 )
                    {
                        msg.Printf( _( "Cleared annotation for %s (unit %s)." ),
                                    symbol->GetValue( true, aSheet, false ),
                                    symbol->SubReference( symbol->GetUnit(), false ) );
                    }
                    else
                    {
                        msg.Printf( _( "Cleared annotation for %s." ),
                                    symbol->GetValue( true, aSheet, false ) );
                    }

                    symbol->ClearAnnotation( aSheet, aResetPrefixes );
                    aReporter.Report( msg, RPT_SEVERITY_ACTION );
                }
            };

    auto clearSheetAnnotation =
            [&]( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet, bool aResetPrefixes )
            {
                for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
                    clearSymbolAnnotation( item, aScreen, aSheet, aResetPrefixes );
            };

    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
    {
        for( SCH_SHEET_PATH& sheet : sheets )
            clearSheetAnnotation( sheet.LastScreen(), &sheet, false );

        break;
    }
    case ANNOTATE_CURRENT_SHEET:
    {
        clearSheetAnnotation( screen, &currentSheet, false );

        if( aRecursive )
        {
            SCH_SHEET_LIST subSheets;

            std::vector<SCH_ITEM*> tempSubSheets;
            currentSheet.LastScreen()->GetSheets( &tempSubSheets );

            for( SCH_ITEM* item : tempSubSheets )
            {
                SCH_SHEET_PATH subSheetPath = currentSheet;
                subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

                sheets.GetSheetsWithinPath( subSheets, subSheetPath );
            }

            for( SCH_SHEET_PATH sheet : subSheets )
                clearSheetAnnotation( sheet.LastScreen(), &sheet, false );
        }

        break;
    }

    case ANNOTATE_SELECTION:
    {
        SCH_SELECTION_TOOL* selTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();
        SCH_SELECTION&      selection = selTool->RequestSelection();
        SCH_SHEET_LIST      selectedSheets;

        for( EDA_ITEM* item : selection.Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
                clearSymbolAnnotation( item, screen, &currentSheet, false );

            if( item->Type() == SCH_SHEET_T && aRecursive )
            {
                SCH_SHEET_PATH subSheetPath = currentSheet;
                subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

                sheets.GetSheetsWithinPath( selectedSheets, subSheetPath );
            }
        }

        for( SCH_SHEET_PATH sheet : selectedSheets )
            clearSheetAnnotation( sheet.LastScreen(), &sheet, false );

        break;
    }
    }

    // Update the references for the sheet that is currently being displayed.
    GetCurrentSheet().UpdateAllScreenReferences();

    wxWindow* erc_dlg = wxWindow::FindWindowByName( DIALOG_ERC_WINDOW_NAME );

    if( erc_dlg )
        static_cast<DIALOG_ERC*>( erc_dlg )->UpdateAnnotationWarning();

    commit.Push( _( "Delete Annotation" ) );

    // Must go after OnModify() so the connectivity graph has been updated
    UpdateNetHighlightStatus();
}


std::unordered_set<SCH_SYMBOL*> getInferredSymbols( const SCH_SELECTION& aSelection )
{
    std::unordered_set<SCH_SYMBOL*> symbols;

    for( EDA_ITEM* item : aSelection )
    {
        switch( item->Type() )
        {
        case SCH_FIELD_T:
        {
            SCH_FIELD*  field = static_cast<SCH_FIELD*>( item );

            if( field->GetId() == FIELD_T::REFERENCE && field->GetParent()->Type() == SCH_SYMBOL_T )
                symbols.insert( static_cast<SCH_SYMBOL*>( field->GetParent() ) );

            break;
        }

        case SCH_SYMBOL_T:
            symbols.insert( static_cast<SCH_SYMBOL*>( item ) );
            break;

        default:
            break;
        }
    }

    return symbols;
}


void SCH_EDIT_FRAME::AnnotateSymbols( SCH_COMMIT* aCommit, ANNOTATE_SCOPE_T  aAnnotateScope,
                                      ANNOTATE_ORDER_T aSortOption, ANNOTATE_ALGO_T aAlgoOption,
                                      bool aRecursive, int aStartNumber, bool aResetAnnotation,
                                      bool aRepairTimestamps, REPORTER& aReporter )
{
    SCH_SELECTION_TOOL* selTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selTool->GetSelection();

    SCH_REFERENCE_LIST  references;
    SCH_SCREENS         screens( Schematic().Root() );
    SCH_SHEET_LIST      sheets = Schematic().Hierarchy();
    SCH_SHEET_PATH      currentSheet = GetCurrentSheet();


    // Store the selected sheets relative to the full hierarchy so we get the correct sheet numbers
    SCH_SHEET_LIST selectedSheets;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET_PATH subSheetPath = currentSheet;
            subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

            sheets.GetSheetsWithinPath( selectedSheets, subSheetPath );
        }
    }


    // Like above, store subsheets relative to full hierarchy for recursive annotation from current
    // sheet
    SCH_SHEET_LIST subSheets;

    std::vector<SCH_ITEM*> tempSubSheets;
    currentSheet.LastScreen()->GetSheets( &tempSubSheets );

    for( SCH_ITEM* item : tempSubSheets )
    {
        SCH_SHEET_PATH subSheetPath = currentSheet;
        subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

        sheets.GetSheetsWithinPath( subSheets, subSheetPath );
    }


    std::unordered_set<SCH_SYMBOL*> selectedSymbols;

    if( aAnnotateScope == ANNOTATE_SELECTION )
        selectedSymbols = getInferredSymbols( selection );


    // Map of locked symbols
    SCH_MULTI_UNIT_REFERENCE_MAP lockedSymbols;

    // Map of previous annotation for building info messages
    std::map<wxString, wxString> previousAnnotation;

    // Test for and replace duplicate time stamps in symbols and sheets.  Duplicate time stamps
    // can happen with old schematics, schematic conversions, or manual editing of files.
    if( aRepairTimestamps )
    {
        int count = screens.ReplaceDuplicateTimeStamps();

        if( count )
        {
            wxString msg;
            msg.Printf( _( "%d duplicate time stamps were found and replaced." ), count );
            aReporter.ReportTail( msg, RPT_SEVERITY_WARNING );
        }
    }

    // Collect all the sets that must be annotated together.
    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
        sheets.GetMultiUnitSymbols( lockedSymbols );
        break;

    case ANNOTATE_CURRENT_SHEET:
        currentSheet.GetMultiUnitSymbols( lockedSymbols );

        if( aRecursive )
            subSheets.GetMultiUnitSymbols( lockedSymbols );

        break;

    case ANNOTATE_SELECTION:
        for( SCH_SYMBOL* symbol : selectedSymbols )
            currentSheet.AppendMultiUnitSymbol( lockedSymbols, symbol );

        if( aRecursive )
            selectedSheets.GetMultiUnitSymbols( lockedSymbols );

        break;
    }

    // Store previous annotations for building info messages
    mapExistingAnnotation( previousAnnotation );

    // Set sheet number and number of sheets.
    SetSheetNumberAndCount();

    // Build symbol list
    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
        sheets.GetSymbols( references );
        break;

    case ANNOTATE_CURRENT_SHEET:
        currentSheet.GetSymbols( references );

        if( aRecursive )
            subSheets.GetSymbolsWithinPath( references, currentSheet, false, true );

        break;

    case ANNOTATE_SELECTION:
        for( SCH_SYMBOL* symbol : selectedSymbols )
            currentSheet.AppendSymbol( references, symbol, false, true );

        if( aRecursive )
            selectedSheets.GetSymbolsWithinPath( references, currentSheet, false, true );

        break;
    }

    // Remove annotation only updates the "new" flag to indicate to the algorithm
    // that these references must be reannotated, but keeps the original reference
    // so that we can reannotate multi-unit symbols together.
    if( aResetAnnotation )
        references.RemoveAnnotation();

    // Build additional list of references to be used during reannotation
    // to avoid duplicate designators (no additional references when annotating
    // the full schematic)
    SCH_REFERENCE_LIST additionalRefs;

    if( aAnnotateScope != ANNOTATE_ALL )
    {
        SCH_REFERENCE_LIST allRefs;
        sheets.GetSymbols( allRefs );

        for( size_t i = 0; i < allRefs.GetCount(); i++ )
        {
            if( !references.Contains( allRefs[i] ) )
                additionalRefs.AddItem( allRefs[i] );
        }
    }

    references.SetRefDesTracker( Schematic().Settings().m_refDesTracker );

    // Break full symbol reference into name (prefix) and number:
    // example: IC1 become IC, and 1
    references.SplitReferences();

    // Annotate all of the references we've collected by our options
    references.AnnotateByOptions( aSortOption, aAlgoOption, aStartNumber, lockedSymbols,
                                  additionalRefs, false );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_REFERENCE&  ref = references[i];
        SCH_SYMBOL*     symbol = ref.GetSymbol();
        SCH_SHEET_PATH* sheet = &ref.GetSheetPath();

        aCommit->Modify( symbol, sheet->LastScreen() );
        ref.Annotate();

        KIID_PATH full_uuid = sheet->Path();
        full_uuid.push_back( symbol->m_Uuid );

        wxString  prevRef = previousAnnotation[ full_uuid.AsString() ];
        wxString  newRef  = symbol->GetRef( sheet );

        if( symbol->GetUnitCount() > 1 )
            newRef << symbol->SubReference( symbol->GetUnitSelection( sheet ) );

        wxString msg;

        if( prevRef.Length() )
        {
            if( newRef == prevRef )
                continue;

            if( symbol->GetUnitCount() > 1 )
            {
                msg.Printf( _( "Updated %s (unit %s) from %s to %s." ),
                            symbol->GetValue( true, sheet, false ),
                            symbol->SubReference( symbol->GetUnit(), false ),
                            prevRef,
                            newRef );
            }
            else
            {
                msg.Printf( _( "Updated %s from %s to %s." ),
                            symbol->GetValue( true, sheet, false ),
                            prevRef,
                            newRef );
            }
        }
        else
        {
            if( symbol->GetUnitCount() > 1 )
            {
                msg.Printf( _( "Annotated %s (unit %s) as %s." ),
                            symbol->GetValue( true, sheet, false ),
                            symbol->SubReference( symbol->GetUnit(), false ),
                            newRef );
            }
            else
            {
                msg.Printf( _( "Annotated %s as %s." ),
                            symbol->GetValue( true, sheet, false ),
                            newRef );
            }
        }

        aReporter.Report( msg, RPT_SEVERITY_ACTION );
    }

    // Final control (just in case ... ).
    if( !CheckAnnotate(
            [ &aReporter ]( ERCE_T , const wxString& aMsg, SCH_REFERENCE* , SCH_REFERENCE* )
            {
                aReporter.Report( aMsg, RPT_SEVERITY_ERROR );
            },
            aAnnotateScope, aRecursive ) )
    {
        aReporter.ReportTail( _( "Annotation complete." ), RPT_SEVERITY_ACTION );
    }

    // Update on screen references, that can be modified by previous calculations:
    GetCurrentSheet().UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    wxWindow* erc_dlg = wxWindow::FindWindowByName( DIALOG_ERC_WINDOW_NAME );

    if( erc_dlg )
        static_cast<DIALOG_ERC*>( erc_dlg )->UpdateAnnotationWarning();

    SyncView();
    GetCanvas()->Refresh();
    OnModify();

    // Must go after OnModify() so the connectivity graph has been updated
    UpdateNetHighlightStatus();
}


int SCH_EDIT_FRAME::CheckAnnotate( ANNOTATION_ERROR_HANDLER aErrorHandler,
                                   ANNOTATE_SCOPE_T         aAnnotateScope,
                                   bool                     aRecursive )
{
    SCH_REFERENCE_LIST  referenceList;
    constexpr bool      includePowerSymbols = false;
    SCH_SHEET_LIST      sheets = Schematic().Hierarchy();
    SCH_SHEET_PATH      currentSheet = GetCurrentSheet();

    // Build the list of symbols
    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
        sheets.GetSymbols( referenceList );
        break;

    case ANNOTATE_CURRENT_SHEET:
        GetCurrentSheet().GetSymbols( referenceList, includePowerSymbols );

        if( aRecursive )
        {
            SCH_SHEET_LIST subSheets;

            std::vector<SCH_ITEM*> tempSubSheets;
            currentSheet.LastScreen()->GetSheets( &tempSubSheets );

            for( SCH_ITEM* item : tempSubSheets )
            {
                SCH_SHEET_PATH subSheetPath = currentSheet;
                subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

                sheets.GetSheetsWithinPath( subSheets, subSheetPath );
            }

            for( SCH_SHEET_PATH sheet : subSheets )
                sheet.GetSymbols( referenceList, includePowerSymbols );
        }

        break;

    case ANNOTATE_SELECTION:
        SCH_SELECTION_TOOL* selTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();
        SCH_SELECTION&      selection = selTool->RequestSelection();

        for( SCH_SYMBOL* symbol : getInferredSymbols( selection ) )
            GetCurrentSheet().AppendSymbol( referenceList, symbol, false, true );

        if( aRecursive )
        {
            SCH_SHEET_LIST selectedSheets;

            for( EDA_ITEM* item : selection.Items() )
            {
                if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET_PATH subSheetPath = currentSheet;
                    subSheetPath.push_back( static_cast<SCH_SHEET*>( item ) );

                    sheets.GetSheetsWithinPath( selectedSheets, subSheetPath );
                }
            }

            for( SCH_SHEET_PATH sheet : selectedSheets )
                sheet.GetSymbols( referenceList, includePowerSymbols );
        }

        break;
    }

    // Empty schematic does not need annotation
    if( referenceList.GetCount() == 0 )
        return 0;

    return referenceList.CheckAnnotation( aErrorHandler );
}
