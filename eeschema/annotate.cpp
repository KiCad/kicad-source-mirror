/**
 * @file annotate.cpp
 * @brief Component annotation.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <reporter.h>
#include <sch_edit_frame.h>

#include <sch_reference_list.h>
#include <class_library.h>


void mapExistingAnnotation( std::map<timestamp_t, wxString>& aMap )
{
    SCH_SHEET_LIST     sheets( g_RootSheet );
    SCH_REFERENCE_LIST references;

    sheets.GetComponents( references );

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_COMPONENT* comp = references[ i ].GetComp();
        wxString       ref = comp->GetField( REFERENCE )->GetFullyQualifiedText();

        if( !ref.Contains( wxT( "?" ) ) )
            aMap[ comp->GetTimeStamp() ] = ref;
    }
}


void SCH_EDIT_FRAME::DeleteAnnotation( bool aCurrentSheetOnly )
{
    if( aCurrentSheetOnly )
    {
        SCH_SCREEN* screen = GetScreen();
        wxCHECK_RET( screen != NULL, wxT( "Attempt to clear annotation of a NULL screen." ) );
        screen->ClearAnnotation( g_CurrentSheet );
    }
    else
    {
        SCH_SCREENS ScreenList;
        ScreenList.ClearAnnotation();
    }

    // Update the references for the sheet that is currently being displayed.
    g_CurrentSheet->UpdateAllScreenReferences();

    SyncView();
    GetCanvas()->Refresh();
    OnModify();
}


void SCH_EDIT_FRAME::AnnotateComponents( bool              aAnnotateSchematic,
                                         ANNOTATE_ORDER_T  aSortOption,
                                         ANNOTATE_OPTION_T aAlgoOption,
                                         int               aStartNumber,
                                         bool              aResetAnnotation,
                                         bool              aRepairTimestamps,
                                         bool              aLockUnits,
                                         REPORTER&         aReporter )
{
    SCH_REFERENCE_LIST references;

    SCH_SCREENS screens;

    // Build the sheet list.
    SCH_SHEET_LIST sheets( g_RootSheet );

    // Map of locked components
    SCH_MULTI_UNIT_REFERENCE_MAP lockedComponents;

    // Map of previous annotation for building info messages
    std::map<timestamp_t, wxString> previousAnnotation;

    // Test for and replace duplicate time stamps in components and sheets.  Duplicate
    // time stamps can happen with old schematics, schematic conversions, or manual
    // editing of files.
    if( aRepairTimestamps )
    {
        int count = screens.ReplaceDuplicateTimeStamps();

        if( count )
        {
            wxString msg;
            msg.Printf( _( "%d duplicate time stamps were found and replaced." ), count );
            aReporter.ReportTail( msg, REPORTER::RPT_WARNING );
        }
    }

    // If units must be locked, collect all the sets that must be annotated together.
    if( aLockUnits )
    {
        if( aAnnotateSchematic )
        {
            sheets.GetMultiUnitComponents( lockedComponents );
        }
        else
        {
            g_CurrentSheet->GetMultiUnitComponents( lockedComponents );
        }
    }

    // Store previous annotations for building info messages
    mapExistingAnnotation( previousAnnotation );

    // If it is an annotation for all the components, reset previous annotation.
    if( aResetAnnotation )
        DeleteAnnotation( !aAnnotateSchematic );

    // Set sheet number and number of sheets.
    SetSheetNumberAndCount();

    // Build component list
    if( aAnnotateSchematic )
    {
        sheets.GetComponents( references );
    }
    else
    {
        g_CurrentSheet->GetComponents( references );
    }

    // Break full components reference in name (prefix) and number:
    // example: IC1 become IC, and 1
    references.SplitReferences();

    switch( aSortOption )
    {
    default:
    case SORT_BY_X_POSITION:
        references.SortByXCoordinate();
        break;

    case SORT_BY_Y_POSITION:
        references.SortByYCoordinate();
        break;
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
    references.Annotate( useSheetNum, idStep, aStartNumber, lockedComponents );
    references.UpdateAnnotation();

    for( size_t i = 0; i < references.GetCount(); i++ )
    {
        SCH_COMPONENT* comp = references[ i ].GetComp();
        wxString       prevRef = previousAnnotation[ comp->GetTimeStamp() ];
        wxString       newRef  = comp->GetField( REFERENCE )->GetFullyQualifiedText();
        wxString       msg;

        if( prevRef.Length() )
        {
            if( newRef == prevRef )
                continue;

            if( comp->GetUnitCount() > 1 )
                msg.Printf( _( "Updated %s (unit %s) from %s to %s" ),
                            GetChars( comp->GetField( VALUE )->GetShownText() ),
                            LIB_PART::SubReference( comp->GetUnit(), false ),
                            GetChars( prevRef ),
                            GetChars( newRef ) );
            else
                msg.Printf( _( "Updated %s from %s to %s" ),
                            GetChars( comp->GetField( VALUE )->GetShownText() ),
                            GetChars( prevRef ),
                            GetChars( newRef ) );
        }
        else
        {
            if( comp->GetUnitCount() > 1 )
                msg.Printf( _( "Annotated %s (unit %s) as %s" ),
                            GetChars( comp->GetField( VALUE )->GetShownText() ),
                            LIB_PART::SubReference( comp->GetUnit(), false ),
                            GetChars( newRef ) );
            else
                msg.Printf( _( "Annotated %s as %s" ),
                            GetChars( comp->GetField( VALUE )->GetShownText() ),
                            GetChars( newRef ) );
        }

        aReporter.Report( msg, REPORTER::RPT_ACTION );
    }

    // Final control (just in case ... ).
    if( !CheckAnnotate( aReporter, !aAnnotateSchematic ) )
        aReporter.ReportTail( _( "Annotation complete." ), REPORTER::RPT_ACTION );

    // Update on screen references, that can be modified by previous calculations:
    g_CurrentSheet->UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    SyncView();
    GetCanvas()->Refresh();
    OnModify();
}


int SCH_EDIT_FRAME::CheckAnnotate( REPORTER& aReporter, bool aOneSheetOnly )
{
    // build the screen list
    SCH_SHEET_LIST      sheetList( g_RootSheet );
    SCH_REFERENCE_LIST  componentsList;

    // Build the list of components
    if( !aOneSheetOnly )
        sheetList.GetComponents( componentsList );
    else
        g_CurrentSheet->GetComponents( componentsList );

    return componentsList.CheckAnnotation( aReporter );
}
