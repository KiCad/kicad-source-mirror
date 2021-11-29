/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <reporter.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <erc_settings.h>
#include <sch_reference_list.h>
#include <symbol_library.h>
#include <tools/ee_selection.h>
#include <tools/ee_selection_tool.h>
#include <tool/tool_manager.h>
#include <dialog_erc.h>

void SCH_EDIT_FRAME::mapExistingAnnotation( std::map<wxString, wxString>& aMap )
{
    SCH_REFERENCE_LIST references;

    Schematic().GetSheets().GetSymbols( references );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_SYMBOL*     symbol = references[ i ].GetSymbol();
        SCH_SHEET_PATH* curr_sheetpath = &references[ i ].GetSheetPath();
        KIID_PATH       curr_full_uuid = curr_sheetpath->Path();

        curr_full_uuid.push_back( symbol->m_Uuid );

        wxString ref = symbol->GetRef( curr_sheetpath );

        if( symbol->GetUnitCount() > 1 )
            ref << LIB_SYMBOL::SubReference( symbol->GetUnitSelection( curr_sheetpath ) );

        if( symbol->IsAnnotated( curr_sheetpath ) )
            aMap[ curr_full_uuid.AsString() ] = ref;
    }
}


void SCH_EDIT_FRAME::DeleteAnnotation( ANNOTATE_SCOPE_T aAnnotateScope, bool* aAppendUndo )
{
    auto clearSymbolAnnotation =
        [&]( EDA_ITEM* aItem, SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( aItem );

            SaveCopyInUndoList( aScreen, symbol, UNDO_REDO::CHANGED, *aAppendUndo );
            *aAppendUndo = true;
            symbol->ClearAnnotation( aSheet );
        };

    auto clearSheetAnnotation =
            [&]( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet )
            {
                for( SCH_ITEM* item : aScreen->Items().OfType( SCH_SYMBOL_T ) )
                    clearSymbolAnnotation( item, aScreen, aSheet );
            };

    SCH_SCREEN* screen = GetScreen();
    SCH_SHEET_PATH currentSheet = GetCurrentSheet();

    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
    {
        for( const SCH_SHEET_PATH& sheet : Schematic().GetSheets() )
            clearSheetAnnotation( sheet.LastScreen(), nullptr );

        break;
    }
    case ANNOTATE_CURRENT_SHEET:
    {
        clearSheetAnnotation( screen, &currentSheet );
        break;
    }

    case ANNOTATE_SELECTION:
    {
        EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selTool->RequestSelection();

        for( EDA_ITEM* item : selection.Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
                clearSymbolAnnotation( item, screen, &currentSheet );
        }
        break;
    }
    }

    // Update the references for the sheet that is currently being displayed.
    GetCurrentSheet().UpdateAllScreenReferences();

    wxWindow* erc_dlg = wxWindow::FindWindowByName( DIALOG_ERC_WINDOW_NAME );

    if( erc_dlg )
        static_cast<DIALOG_ERC*>( erc_dlg )->UpdateAnnotationWarning();

    SyncView();
    GetCanvas()->Refresh();
    OnModify();

    // Must go after OnModify() so the connectivity graph has been updated
    UpdateNetHighlightStatus();
}


void SCH_EDIT_FRAME::AnnotateSymbols( ANNOTATE_SCOPE_T  aAnnotateScope,
                                      ANNOTATE_ORDER_T  aSortOption,
                                      ANNOTATE_ALGO_T   aAlgoOption,
                                      int               aStartNumber,
                                      bool              aResetAnnotation,
                                      bool              aRepairTimestamps,
                                      REPORTER&         aReporter )
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->RequestSelection();

    SCH_REFERENCE_LIST references;
    SCH_SCREENS        screens( Schematic().Root() );
    SCH_SHEET_LIST     sheets = Schematic().GetSheets();
    SCH_SHEET_PATH     currentSheet = GetCurrentSheet();
    bool               appendUndo = false;

    // Map of locked symbols
    SCH_MULTI_UNIT_REFERENCE_MAP lockedSymbols;

    // Map of previous annotation for building info messages
    std::map<wxString, wxString> previousAnnotation;

    // Test for and replace duplicate time stamps in symbols and sheets.  Duplicate
    // time stamps can happen with old schematics, schematic conversions, or manual
    // editing of files.
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
        break;

    case ANNOTATE_SELECTION:
        selection.GetMultiUnitSymbols( lockedSymbols, currentSheet );
        break;
    }

    // Store previous annotations for building info messages
    mapExistingAnnotation( previousAnnotation );

    // If it is an annotation for all the symbols, reset previous annotation.
    if( aResetAnnotation )
        DeleteAnnotation( aAnnotateScope, &appendUndo );

    // Set sheet number and number of sheets.
    SetSheetNumberAndCount();

    // Build symbol list
    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
        sheets.GetSymbols( references );
        break;

    case ANNOTATE_CURRENT_SHEET:
        GetCurrentSheet().GetSymbols( references );
        break;

    case ANNOTATE_SELECTION:
        selection.GetSymbols( references, currentSheet );
        break;
    }

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

    // Break full symbol reference into name (prefix) and number:
    // example: IC1 become IC, and 1
    references.SplitReferences();

    switch( aSortOption )
    {
    default:
    case SORT_BY_X_POSITION: references.SortByXCoordinate(); break;
    case SORT_BY_Y_POSITION: references.SortByYCoordinate(); break;
    }

    bool useSheetNum = false;
    int idStep = 100;

    switch( aAlgoOption )
    {
    default:
    case INCREMENTAL_BY_REF:
        break;

    case SHEET_NUMBER_X_100:
        useSheetNum = true;
        break;

    case SHEET_NUMBER_X_1000:
        useSheetNum = true;
        idStep = 1000;
        break;
    }

    // Recalculate and update reference numbers in schematic
    references.Annotate( useSheetNum, idStep, aStartNumber, lockedSymbols, additionalRefs );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_REFERENCE&  ref = references[i];
        SCH_SYMBOL*     symbol = ref.GetSymbol();
        SCH_SHEET_PATH* sheet = &ref.GetSheetPath();

        SaveCopyInUndoList( sheet->LastScreen(), symbol, UNDO_REDO::CHANGED, appendUndo );
        appendUndo = true;
        ref.Annotate();

        KIID_PATH full_uuid = sheet->Path();
        full_uuid.push_back( symbol->m_Uuid );

        wxString  prevRef = previousAnnotation[ full_uuid.AsString() ];
        wxString  newRef  = symbol->GetRef( sheet );

        if( symbol->GetUnitCount() > 1 )
            newRef << LIB_SYMBOL::SubReference( symbol->GetUnitSelection( sheet ) );

        wxString msg;

        if( prevRef.Length() )
        {
            if( newRef == prevRef )
                continue;

            if( symbol->GetUnitCount() > 1 )
            {
                msg.Printf( _( "Updated %s (unit %s) from %s to %s." ),
                            symbol->GetValue( sheet, true ),
                            LIB_SYMBOL::SubReference( symbol->GetUnit(), false ),
                            prevRef,
                            newRef );
            }
            else
            {
                msg.Printf( _( "Updated %s from %s to %s." ),
                            symbol->GetValue( sheet, true ),
                            prevRef,
                            newRef );
            }
        }
        else
        {
            if( symbol->GetUnitCount() > 1 )
            {
                msg.Printf( _( "Annotated %s (unit %s) as %s." ),
                            symbol->GetValue( sheet, true ),
                            LIB_SYMBOL::SubReference( symbol->GetUnit(), false ),
                            newRef );
            }
            else
            {
                msg.Printf( _( "Annotated %s as %s." ),
                            symbol->GetValue( sheet, true ),
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
            aAnnotateScope ) )
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
                                   ANNOTATE_SCOPE_T         aAnnotateScope )
{
    SCH_REFERENCE_LIST  referenceList;
    constexpr bool      includePowerSymbols = false;

    // Build the list of symbols
    switch( aAnnotateScope )
    {
    case ANNOTATE_ALL:
        Schematic().GetSheets().GetSymbols( referenceList );
        break;

    case ANNOTATE_CURRENT_SHEET:
        GetCurrentSheet().GetSymbols( referenceList, includePowerSymbols );
        break;

    case ANNOTATE_SELECTION:
        EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selTool->RequestSelection();
        selection.GetSymbols( referenceList, GetCurrentSheet(), includePowerSymbols );
        break;
    }

    // Empty schematic does not need annotation
    if( referenceList.GetCount() == 0 )
        return 0;

    return referenceList.CheckAnnotation( aErrorHandler );
}
