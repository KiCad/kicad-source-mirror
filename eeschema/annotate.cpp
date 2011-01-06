/**************************************/
/* annotate.cpp: component annotation */
/**************************************/

#include <algorithm> // to use sort vector
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxstruct.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "netlist.h"
#include "class_library.h"
#include "protos.h"
#include "sch_component.h"
#include "lib_pin.h"


/**
 * Function DeleteAnnotation
 * Remove current component annotations
 * @param aCurrentSheetOnly : if false: remove all annotations, else remove
 *                             annotation relative to the current sheet only
 */
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


/**
 * Function AnnotateComponents:
 *
 *  Compute the annotation of the components for the whole project, or the
 *  current sheet only.  All the components or the new ones only will be
 *  annotated.
 * @param aAnnotateSchematic : true = entire schematic annotation,
 *                            false = current sheet only
 * @param aSortOption : 0 = annotate by sorting X position,
 *                      1 = annotate by sorting Y position,
 *                      2 = annotate by sorting value
 * @param aAlgoOption : 0 = annotate schematic using first free Id number
 *                      1 = annotate using first free Id number, starting to sheet number * 100
 *                      2 = annotate  using first free Id number, starting to sheet number * 1000
 * @param aResetAnnotation : true = remove previous annotation
 *                          false = annotate new components only
 * @param aRepairsTimestamps : true = test for duplicate times stamps and
 *                                   replace duplicated
 *        Note: this option could change previous annotation, because time
 *              stamps are used to handle annotation mainly in complex
 *              hierarchies.
 * When the sheet number is used in annotation,
 *      for each sheet annotation starts from sheet number * 100
 *      ( the first sheet uses 100 to 199, the second 200 to 299 ... )
 */
void SCH_EDIT_FRAME::AnnotateComponents( bool aAnnotateSchematic,
                                         int  aSortOption,
                                         int  aAlgoOption,
                                         bool aResetAnnotation,
                                         bool aRepairsTimestamps )
{
    SCH_REFERENCE_LIST references;

    wxBusyCursor dummy;

    SCH_SCREENS screens;

    /* Build the sheet list */
    SCH_SHEET_LIST sheets;

    // Test for and replace duplicate time stamps in components and sheets.  Duplicate
    // time stamps can happen with old schematics, schematic conversions, or manual
    // editing of files.
    if( aRepairsTimestamps )
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
        GetSheet()->GetComponents( references );
    }

    /* Break full components reference in name (prefix) and number:
     * example: IC1 become IC, and 1 */
    references.SplitReferences();

    switch( aSortOption )
    {
    default:
    case 0:
        references.SortByXCoordinate();
        break;

    case 1:
        references.SortByYCoordinate();
        break;
    }

    bool useSheetNum = false;
    int idStep = 100;

    switch( aAlgoOption )
    {
    default:
    case 0:
        break;

    case 1:
        useSheetNum = true;
        break;

    case 2:
        useSheetNum = true;
        idStep = 1000;
        break;
    }

    // Recalculate and update reference numbers in schematic
    references.Annotate( useSheetNum, idStep );
    references.UpdateAnnotation();

    wxArrayString errors;

    /* Final control (just in case ... )*/
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

    DrawPanel->Refresh( true );
}


/**
 * Function CheckAnnotate
 *  Check errors relatives to annotation:
 *      components not annotated
 *      components having the same reference (duplicates)
 *      for multiple parts per package components :
 *          part number > number of parts
 *          different values between parts
 * @param aMessageList = a wxArrayString to store messages.
 * @param aOneSheetOnly : true = search is made only in the current sheet
 *                        false = search in whole hierarchy (usual search).
 * @return errors count
 */
int SCH_EDIT_FRAME::CheckAnnotate( wxArrayString* aMessageList, bool aOneSheetOnly )
{
    /* build the screen list */
    SCH_SHEET_LIST SheetList;
    SCH_REFERENCE_LIST ComponentsList;

    /* Build the list of components */
    if( !aOneSheetOnly )
        SheetList.GetComponents( ComponentsList );
    else
        GetSheet()->GetComponents( ComponentsList );

    return ComponentsList.CheckAnnotation( aMessageList );
}
