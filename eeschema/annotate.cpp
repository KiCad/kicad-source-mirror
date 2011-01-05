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

//#define USE_OLD_ALGO

/**
 * Function ComputeReferenceNumber
 * Compute the reference number for components without reference number
 * i.e. .m_NumRef member of each SCH_REFERENCE_LIST item not yet annotated
 * in aComponentsList.
 * if aUseSheetNum is false, this number starts from 1
 * if aUseSheetNum is false, this number starts from from SheetNumber * aSheetIntervalId
 * @param aComponentsList = the SCH_REFERENCE_LIST to fill
 * @param aUseSheetNum = false to start Ids from 0,
 *                       true to start each sheet annotation from SheetNumber * aSheetIntervalId
 * @param aSheetIntervalId = number of allowed Id by sheet and by reference prefix
 *       if There are more than aSheetIntervalId of  reference prefix in a given sheet
 *       number overlap next sheet inveral, but there is no annotation problem.
 *       Useful values are only 100 or 1000
 * For instance for a sheet number = 2, and aSheetIntervalId = 100, the first Id = 101
 * and the last Id is 199 when no overlap occurs with sheet number 2.
 * Rf there are 150 items in sheet number 2, items are referenced U201 to U351,
 * and items in sheet 3 start from U352
 */
static void ComputeReferenceNumber( SCH_REFERENCE_LIST& aComponentsList,
                                    bool aUseSheetNum, int aSheetIntervalId );

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
    ComputeReferenceNumber( references, useSheetNum, idStep );
    references.UpdateAnnotation();

    /* Final control (just in case ... )*/
    CheckAnnotate( NULL, !aAnnotateSchematic );
    OnModify();

    // Update on screen references, that can be modified by previous calculations:
    m_CurrentSheet->UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    DrawPanel->Refresh( true );
}


#ifndef USE_OLD_ALGO
/**
 * helper function CreateFirstFreeRefId
 * Search for a free ref Id inside a list of reference numbers in use.
 * Because this function just search for a hole in a list of incremented numbers,
 * this list must be:
 *      sorted by increasing values.
 *      and each value stored only once
 * @see BuildRefIdInUseList to prepare this list
 * @param aIdList = the buffer that contains Ids in use
 * @param aFirstValue = the first expected free value
 * @return a free (not yet used) Id
 * and this new id is added in list
 */
static int CreateFirstFreeRefId( std::vector<int>& aIdList, int aFirstValue )
{
    int expectedId = aFirstValue;

    // We search for expected Id a value >= aFirstValue.
    // Skip existing Id < aFirstValue
    unsigned ii = 0;

    for( ; ii < aIdList.size(); ii++ )
    {
        if( expectedId <= aIdList[ii]  )
            break;
    }

    // Ids are sorted by increasing value, from aFirstValue
    // So we search from aFirstValue the first not used value, i.e. the first hole in list.
    for(; ii < aIdList.size(); ii++ )
    {
        if( expectedId != aIdList[ii]  )    // This id is not yet used.
        {
            // Insert this free Id, in order to keep list sorted
            aIdList.insert(aIdList.begin() + ii, expectedId);
            return expectedId;
        }

        expectedId++;
    }

    // All existing Id are tested, and all values are found in use.
    // So Create a new one.
    aIdList.push_back( expectedId );
    return expectedId;
}
#endif

/*
 * Function ComputeReferenceNumber
 * Compute the reference number for components without reference number
 * i.e. .m_NumRef member of each SCH_REFERENCE_LIST item not yet annotated
 * in aComponentsList.
 * if aUseSheetNum is false, this number starts from 1
 * if aUseSheetNum is false, this number starts from from SheetNumber * aSheetIntervalId
 */
static void ComputeReferenceNumber( SCH_REFERENCE_LIST& aComponentsList,
                                    bool aUseSheetNum, int aSheetIntervalId  )
{
    if ( aComponentsList.GetCount() == 0 )
        return;

    int LastReferenceNumber = 0;
    int NumberOfUnits, Unit;

    /* Components with an invisible reference (power...) always are re-annotated. */
    aComponentsList.ResetHiddenReferences();

    /* calculate index of the first component with the same reference prefix
     * than the current component.  All components having the same reference
     * prefix will receive a reference number with consecutive values:
     * IC .. will be set to IC4, IC4, IC5 ...
     */
    unsigned first = 0;

    /* calculate the last used number for this reference prefix: */
#ifdef USE_OLD_ALGO
    int minRefId = 0;

    // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
    if( aUseSheetNum )
        minRefId = aComponentsList[first].m_SheetNum * aSheetIntervalId;

    LastReferenceNumber = aComponentsList.GetLastReference( first, minRefId );
#else
    int minRefId = 1;

    // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
    if( aUseSheetNum )
        minRefId = aComponentsList[first].m_SheetNum * aSheetIntervalId + 1;

    // This is the list of all Id already in use for a given reference prefix.
    // Will be refilled for each new reference prefix.
    std::vector<int>idList;
    aComponentsList.GetRefsInUse( first, idList, minRefId );
#endif
    for( unsigned ii = 0; ii < aComponentsList.GetCount(); ii++ )
    {
        if( aComponentsList[ii].m_Flag )
            continue;

        if( ( aComponentsList[first].CompareRef( aComponentsList[ii] ) != 0 )
            || ( aUseSheetNum
                 && ( aComponentsList[first].m_SheetNum != aComponentsList[ii].m_SheetNum ) ) )
        {
            /* New reference found: we need a new ref number for this reference */
            first = ii;
#ifdef USE_OLD_ALGO
            minRefId = 0;

            // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
            if( aUseSheetNum )
                minRefId = aComponentsList[ii].m_SheetNum * aSheetIntervalId;

            LastReferenceNumber = aComponentsList.GetLastReference( ii, minRefId );
#else
            minRefId = 1;

            // when using sheet number, ensure ref number >= sheet number* aSheetIntervalId
            if( aUseSheetNum )
                minRefId = aComponentsList[ii].m_SheetNum * aSheetIntervalId + 1;

            aComponentsList.GetRefsInUse( first, idList, minRefId );
#endif
        }

        // Annotation of one part per package components (trivial case).
        if( aComponentsList[ii].GetLibComponent()->GetPartCount() <= 1 )
        {
            if( aComponentsList[ii].m_IsNew )
            {
#ifdef USE_OLD_ALGO
                LastReferenceNumber++;
#else
                LastReferenceNumber = CreateFirstFreeRefId( idList, minRefId );
#endif
                aComponentsList[ii].m_NumRef = LastReferenceNumber;
            }

            aComponentsList[ii].m_Unit  = 1;
            aComponentsList[ii].m_Flag  = 1;
            aComponentsList[ii].m_IsNew = false;
            continue;
        }

        /* Annotation of multi-part components ( n parts per package ) (complex case) */
        NumberOfUnits = aComponentsList[ii].GetLibComponent()->GetPartCount();

        if( aComponentsList[ii].m_IsNew )
        {
#ifdef USE_OLD_ALGO
            LastReferenceNumber++;
#else
            LastReferenceNumber = CreateFirstFreeRefId( idList, minRefId );
#endif
            aComponentsList[ii].m_NumRef = LastReferenceNumber;

            if( !aComponentsList[ii].IsPartsLocked() )
                aComponentsList[ii].m_Unit = 1;

            aComponentsList[ii].m_Flag = 1;
        }

        /* search for others units of this component.
         * we search for others parts that have the same value and the same
         * reference prefix (ref without ref number)
         */
        for( Unit = 1; Unit <= NumberOfUnits; Unit++ )
        {
            if( aComponentsList[ii].m_Unit == Unit )
                continue;

            int found = aComponentsList.FindUnit( ii, Unit );

            if( found >= 0 )
                continue; /* this unit exists for this reference (unit already annotated) */

            /* Search a component to annotate ( same prefix, same value, not annotated) */
            for( unsigned jj = ii + 1; jj < aComponentsList.GetCount(); jj++ )
            {
                if( aComponentsList[jj].m_Flag )    // already tested
                    continue;

                if( aComponentsList[ii].CompareRef( aComponentsList[jj] ) != 0 )
                    continue;

                if( aComponentsList[jj].CompareValue( aComponentsList[ii] ) != 0 )
                    continue;

                if( !aComponentsList[jj].m_IsNew )
                    continue;

                /* Component without reference number found, annotate it if possible */
                if( !aComponentsList[jj].IsPartsLocked()
                    || ( aComponentsList[jj].m_Unit == Unit ) )
                {
                    aComponentsList[jj].m_NumRef = aComponentsList[ii].m_NumRef;
                    aComponentsList[jj].m_Unit   = Unit;
                    aComponentsList[jj].m_Flag   = 1;
                    aComponentsList[jj].m_IsNew  = false;
                    break;
                }
            }
        }
    }
}


/**
 * Function CheckAnnotate
 *  Check errors relatives to annotation:
 *      components not annotated
 *      components having the same reference (duplicates)
 *      for multiple parts per package components :
 *          part number > number of parts
 *          different values between parts
 * @param aMessageList = a wxArrayString to store messages. If NULL, they
 *                       are displayed in a wxMessageBox
 * @param aOneSheetOnly : true = search is made only in the current sheet
 *                        false = search in whole hierarchy (usual search).
 * @return errors count
 */
int SCH_EDIT_FRAME::CheckAnnotate( wxArrayString* aMessageList, bool aOneSheetOnly )
{
    int            error = 0;
    wxString       Buff;
    wxString       msg, cmpref;

    /* build the screen list */
    SCH_SHEET_LIST SheetList;

    SCH_REFERENCE_LIST ComponentsList;

    /* Build the list of components */
    if( !aOneSheetOnly )
        SheetList.GetComponents( ComponentsList );
    else
        GetSheet()->GetComponents( ComponentsList );

    ComponentsList.SortByRefAndValue();

    /* Break full components reference in name (prefix) and number: example:
     * IC1 become IC, and 1 */
    ComponentsList.SplitReferences();

    /* count not yet annotated items or annotation error*/
    for( unsigned ii = 0; ii < ComponentsList.GetCount(); ii++ )
    {
        msg.Empty();
        Buff.Empty();

        if( ComponentsList[ii].m_IsNew )    // Not yet annotated
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = ComponentsList[ii].GetRef();
            msg.Printf( _( "item not annotated: %s%s" ), GetChars( cmpref ), GetChars( Buff ) );

            if( ( ComponentsList[ii].m_Unit > 0 ) && ( ComponentsList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                Buff.Printf( _( "( unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ) );
            else
                DisplayError( NULL, msg );

            error++;
            break;
        }

        // Annotate error if unit selected does not exist ( i.e. > number of parts )
        // Can happen if a component has changed in a lib, after a previous annotation
        if( MAX( ComponentsList[ii].GetLibComponent()->GetPartCount(), 1 ) < ComponentsList[ii].m_Unit )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = ComponentsList[ii].GetRef();

            msg.Printf( _( "Error item %s%s" ), GetChars( cmpref ), GetChars( Buff ) );

            Buff.Printf( _( " unit %d and no more than %d parts" ),
                         ComponentsList[ii].m_Unit,
                         ComponentsList[ii].GetLibComponent()->GetPartCount() );
            msg << Buff;

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ));
            else
                DisplayError( NULL, msg );

            error++;
            break;
        }
    }

    if( error )
        return error;

    // count the duplicated elements (if all are annotated)
    int imax = ComponentsList.GetCount() - 1;
    for( int ii = 0; (ii < imax) && (error < 4); ii++ )
    {
        msg.Empty();
        Buff.Empty();

        if( ( ComponentsList[ii].CompareRef( ComponentsList[ii + 1] ) != 0 )
           || ( ComponentsList[ii].m_NumRef != ComponentsList[ii + 1].m_NumRef ) )
            continue;

        /* Same reference found. If same unit, error !
         */
        if( ComponentsList[ii].m_Unit == ComponentsList[ii + 1].m_Unit )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = ComponentsList[ii].GetRef();

            msg.Printf( _( "Multiple item %s%s" ), GetChars( cmpref ), GetChars( Buff ) );

            if( ( ComponentsList[ii].m_Unit > 0 )&& ( ComponentsList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                Buff.Printf( _( " (unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ));
            else
                DisplayError( NULL, msg );

            error++;
            continue;
        }

        /* Test error if units are different but number of parts per package
         * too high (ex U3 ( 1 part) and we find U3B this is an error) */
        if( ComponentsList[ii].GetLibComponent()->GetPartCount()
            != ComponentsList[ii + 1].GetLibComponent()->GetPartCount() )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = ComponentsList[ii].GetRef();
            msg.Printf( _( "Multiple item %s%s" ), GetChars( cmpref ), GetChars( Buff ) );

            if( ( ComponentsList[ii].m_Unit > 0 ) && ( ComponentsList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                Buff.Printf( _( " (unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ));
            else
                DisplayError( NULL, msg );

            error++;
        }

        /* Error if values are different between units, for the same reference */
        int next = ii + 1;

        if( ComponentsList[ii].CompareValue( ComponentsList[next] ) != 0 )
        {
            wxString nextcmpref = ComponentsList[next].GetRef();

            cmpref = ComponentsList[ii].GetRef();

#if defined(KICAD_GOST)
            msg.Printf( _( "Diff values for %s%d.%c (%s) and %s%d.%c (%s)" ),
                        cmpref.GetData(),
                        ComponentsList[ii].m_NumRef,
                        ComponentsList[ii].m_Unit + '1' - 1,
                        GetChars( *ComponentsList[ii].m_Value ),
                        GetChars( nextcmpref ),
                        ComponentsList[next].m_NumRef,
                        ComponentsList[next].m_Unit + '1' - 1,
                        ComponentsList[next].m_Value->GetData() );
#else
            msg.Printf( _( "Diff values for %s%d%c (%s) and %s%d%c (%s)" ),
                        cmpref.GetData(),
                        ComponentsList[ii].m_NumRef,
                        ComponentsList[ii].m_Unit + 'A' - 1,
                        GetChars( *ComponentsList[ii].m_Value ),
                        GetChars( nextcmpref ),
                        ComponentsList[next].m_NumRef,
                        ComponentsList[next].m_Unit + 'A' - 1,
                        GetChars( *ComponentsList[next].m_Value ) );
#endif

            if( aMessageList )
                aMessageList->Add( msg + wxT( "\n" ));
            else
                DisplayError( NULL, msg );

            error++;
        }
    }

    // count the duplicated time stamps
    ComponentsList.SortComponentsByTimeStamp();

    for( int ii = 0; ( ii < imax ) && ( error < 4 ); ii++ )
    {
        if( ( ComponentsList[ii].m_TimeStamp != ComponentsList[ii + 1].m_TimeStamp )
            || ( ComponentsList[ii].GetSheetPath() != ComponentsList[ii + 1].GetSheetPath() ) )
            continue;

        /* Same time stamp found.  */
        wxString nextcmpref;
        wxString full_path;

        full_path.Printf( wxT( "%s%8.8X" ),
                          GetChars( ComponentsList[ii].GetSheetPath().Path() ),
                          ComponentsList[ii].m_TimeStamp );

        cmpref     = ComponentsList[ii].GetRef();
        nextcmpref = ComponentsList[ii + 1].GetRef();

        msg.Printf( _( "duplicate time stamp (%s) for %s%d and %s%d" ),
                    GetChars( full_path ),
                    GetChars( cmpref ), ComponentsList[ii].m_NumRef,
                    GetChars( nextcmpref ), ComponentsList[ii + 1].m_NumRef );

        if( aMessageList )
            aMessageList->Add( msg + wxT( "\n" ));
        else
            DisplayError( NULL, msg );

        error++;
    }

    return error;
}
