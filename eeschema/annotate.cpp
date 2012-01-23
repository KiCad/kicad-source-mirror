/**
 * @file annotate.cpp
 * @brief Component annotation.
 */

#include <algorithm> // to use sort vector
#include <vector>

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxEeschemaStruct.h>

#include <netlist.h>
#include <class_library.h>
#include <protos.h>
#include <sch_component.h>
#include <lib_pin.h>


void SCH_EDIT_FRAME::DeleteAnnotation( bool aCurrentSheetOnly )
{
    if( aCurrentSheetOnly )
    {
        SCH_SCREEN* screen = GetScreen();
        wxCHECK_RET( screen != NULL, wxT( "Attempt to clear annotation of a NULL screen." ) );
        screen->ClearAnnotation( m_CurrentSheet );
    }
    else
    {
        SCH_SCREENS ScreenList;
        ScreenList.ClearAnnotation();
    }

    // Update the references for the sheet that is currently being displayed.
    m_CurrentSheet->UpdateAllScreenReferences();
}


void SCH_EDIT_FRAME::AnnotateComponents( bool              aAnnotateSchematic,
                                         ANNOTATE_ORDER_T  aSortOption,
                                         ANNOTATE_OPTION_T aAlgoOption,
                                         bool              aResetAnnotation,
                                         bool              aRepairTimestamps )
{
    SCH_REFERENCE_LIST references;

    wxBusyCursor dummy;

    SCH_SCREENS screens;

    // Build the sheet list.
    SCH_SHEET_LIST sheets;

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
            DisplayInfoMessage( NULL, msg, 2 );
        }
    }

    // If it is an annotation for all the components, reset previous annotation.
    if( aResetAnnotation )
        DeleteAnnotation( !aAnnotateSchematic );

    // Update the screen date.
    screens.SetDate( GenDate() );

    // Set sheet number and number of sheets.
    SetSheetNumberAndCount();

    /* Build component list */
    if( aAnnotateSchematic )
    {
        sheets.GetComponents( references );
    }
    else
    {
        m_CurrentSheet->GetComponents( references );
    }

    /* Break full components reference in name (prefix) and number:
     * example: IC1 become IC, and 1 */
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
    references.Annotate( useSheetNum, idStep );
    references.UpdateAnnotation();

    wxArrayString errors;

    // Final control (just in case ... ).
    if( CheckAnnotate( &errors, !aAnnotateSchematic ) )
    {
        wxString msg;

        for( size_t i = 0; i < errors.GetCount(); i++ )
            msg += errors[i];

        // wxLogWarning is a cheap and dirty way to dump a potentially long list of
        // strings to a dialog that can be saved to a file.  This should be replaced
        // by a more elegant solution.
        wxLogWarning( msg );
    }

    OnModify();

    // Update on screen references, that can be modified by previous calculations:
    m_CurrentSheet->UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    m_canvas->Refresh( true );
}


int SCH_EDIT_FRAME::CheckAnnotate( wxArrayString* aMessageList, bool aOneSheetOnly )
{
    /* build the screen list */
    SCH_SHEET_LIST SheetList;
    SCH_REFERENCE_LIST ComponentsList;

    /* Build the list of components */
    if( !aOneSheetOnly )
        SheetList.GetComponents( ComponentsList );
    else
        m_CurrentSheet->GetComponents( ComponentsList );

    return ComponentsList.CheckAnnotation( aMessageList );
}
