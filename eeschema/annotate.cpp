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


static void BreakReference( SCH_REFERENCE_LIST& aComponentsList );
static void ReAnnotateComponents( SCH_REFERENCE_LIST& aComponentsList );
static void ComputeReferenceNumber( SCH_REFERENCE_LIST& aComponentsList, bool aUseSheetNum );
static int  GetLastReferenceNumber( int aObjet,SCH_REFERENCE_LIST& aComponentsList );
static int  ExistUnit( int aObjet, int aUnit, SCH_REFERENCE_LIST& aComponentList );


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
 * AnnotateComponents:
 *
 *  Compute the annotation of the components for the whole project, or the
 *  current sheet only.  All the components or the new ones only will be
 *  annotated.
 * @param parent = Schematic frame
 * @param annotateSchematic : true = entire schematic annotation,
 *                            false = current sheet only
 * @param sortOption : 0 = annotate by sorting X position,
 *                     1 = annotate by sorting Y position,
 *                     2 = annotate by sorting value
 * @param resetAnnotation : true = remove previous annotation
 *                          false = annotate new components only
 * @param repairsTimestamps : true = test for duplicate times stamps and
 *                                   replace duplicated
 *        Note: this option could change previous annotation, because time
 *              stamps are used to handle annotation mainly in complex
 *              hierarchies.
 */
void SCH_EDIT_FRAME::AnnotateComponents(
                         bool            annotateSchematic,
                         int             sortOption,
                         bool            resetAnnotation,
                         bool            repairsTimestamps )
{
    SCH_REFERENCE_LIST references;

    wxBusyCursor dummy;

    SCH_SCREENS screens;

    /* Build the sheet list */
    SCH_SHEET_LIST sheets;

    // Test for and replace duplicate time stamps in components and sheets.  Duplicate
    // time stamps can happen with old schematics, schematic conversions, or manual
    // editing of files.
    if( repairsTimestamps )
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
    if( resetAnnotation )
        DeleteAnnotation( !annotateSchematic );

    // Update the screen date.
    screens.SetDate( GenDate() );

    // Set sheet number and total sheet counts.
    SetSheetNumberAndCount();

    /* Build component list */
    if( annotateSchematic )
    {
        sheets.GetComponents( references );
    }
    else
    {
        GetSheet()->GetComponents( references );
    }

    /* Break full components reference in name (prefix) and number:
     * example: IC1 become IC, and 1 */
    BreakReference( references );

    bool useSheetNum = false;
    switch( sortOption )
    {
    case 0:
        references.SortCmpByXCoordinate();
        break;

    case 1:
        useSheetNum = true;
        references.SortCmpByXCoordinate();
        break;

    case 2:
        references.SortCmpByYCoordinate();
        break;

    case 3:
        useSheetNum = true;
        references.SortCmpByYCoordinate();
        break;

    case 4:
        references.SortComponentsByRefAndValue();
        break;
    }

    /* Recalculate reference numbers */
    ComputeReferenceNumber( references, useSheetNum );
    ReAnnotateComponents( references );

    /* Final control (just in case ... )*/
    CheckAnnotate( NULL, !annotateSchematic );
    OnModify();
    DrawPanel->Refresh( true );
}


/*
 * Update the reference component for the schematic project (or the current sheet)
 */
static void ReAnnotateComponents( SCH_REFERENCE_LIST& aComponentList )
{
    /* update the reference numbers */
    for( unsigned ii = 0; ii < aComponentList.GetCount(); ii++ )
    {
        aComponentList[ii].Annotate();
    }
}


void BreakReference( SCH_REFERENCE_LIST& aComponentsList )
{
    for( unsigned ii = 0; ii < aComponentsList.GetCount(); ii++ )
        aComponentsList[ii].Split();
}


/*
 * Compute the reference number for components without reference number
 *  Compute .m_NumRef member
 */
static void ComputeReferenceNumber( SCH_REFERENCE_LIST& aComponentsList, bool aUseSheetNum  )
{
    int LastReferenceNumber, NumberOfUnits, Unit;

    /* Components with an invisible reference (power...) always are
     * re-annotated.  So set their .m_IsNew member to true
     */
    for( unsigned ii = 0; ii < aComponentsList.GetCount(); ii++ )
    {
        if( aComponentsList[ii].GetRefStr()[0] == '#' )
        {
            aComponentsList[ii].m_IsNew  = true;
            aComponentsList[ii].m_NumRef = 0;
        }
    }

    /* calculate index of the first component with the same reference prefix
     * than the current component.  All components having the same reference
     * prefix will receive a reference number with consecutive values:
     * IC .. will be set to IC4, IC4, IC5 ...
     */
    unsigned first = 0;
    /* calculate the last used number for this reference prefix: */
    LastReferenceNumber = GetLastReferenceNumber( first, aComponentsList );
    for( unsigned ii = 0; ii < aComponentsList.GetCount(); ii++ )
    {
        if( aComponentsList[ii].m_Flag )
            continue;

        if( aComponentsList[first].CompareRef( aComponentsList[ii] ) != 0 )
        {
            /* New reference found: we need a new ref number for this
             * reference */
            first = ii;
            LastReferenceNumber = GetLastReferenceNumber( ii, aComponentsList );
        }
        // when using sheet number, ensure annot number >= sheet number* 100
        if( aUseSheetNum )
        {
            int min_num = aComponentsList[ii].m_SheetNum * 100;
            if( LastReferenceNumber < min_num )
                LastReferenceNumber = min_num;
        }
        /* Annotation of one part per package components (trivial case)*/
        if( aComponentsList[ii].m_Entry->GetPartCount() <= 1 )
        {
            if( aComponentsList[ii].m_IsNew )
            {
                LastReferenceNumber++;
                aComponentsList[ii].m_NumRef = LastReferenceNumber;
            }

            aComponentsList[ii].m_Unit  = 1;
            aComponentsList[ii].m_Flag  = 1;
            aComponentsList[ii].m_IsNew = false;
            continue;
        }

        /* Annotation of multi-part components ( n parts per package )
         * (complex case) */
        NumberOfUnits = aComponentsList[ii].m_Entry->GetPartCount();

        if( aComponentsList[ii].m_IsNew )
        {
            LastReferenceNumber++;
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

            int found = ExistUnit( ii, Unit, aComponentsList );

            if( found >= 0 )
                continue; /* this unit exists for this reference (unit
                           * already annotated) */

            /* Search a component to annotate ( same prefix, same value,
             * not annotated) */
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

                /* Component without reference number found, annotate it
                 * if possible */
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
 * Search the last used (greatest) reference number in the component list
 * for the prefix reference given by Objet
 * The component list must be sorted.
 *
 * @param aObjet = reference item ( aComponentsList[aObjet].m_TextRef is
 *                 the search pattern)
 * @param aComponentsList = list of items
 */
int GetLastReferenceNumber( int aObjet,SCH_REFERENCE_LIST& aComponentsList )
{
    int LastNumber = 0;

    for( unsigned ii = 0; ii < aComponentsList.GetCount(); ii++ )
    {
        /* New identifier. */
        if( aComponentsList[aObjet].CompareRef( aComponentsList[ii] ) != 0 )
            continue;

        if( LastNumber < aComponentsList[ii].m_NumRef )
            LastNumber = aComponentsList[ii].m_NumRef;
    }

    return LastNumber;
}


/**
 * Search in the sorted list of components, for a given component an other
 * component with the same reference and a given part unit.  Mainly used to
 * manage multiple parts per package components.
 * @param aObjet = index in aComponentsList for the given SCH_REFERENCE
 *                 item to test
 * @param Unit = the given unit number to search
 * @param aComponentsList = list of items to examine
 * @return index in aComponentsList if found or -1 if not found
 */
static int ExistUnit( int aObjet, int Unit, SCH_REFERENCE_LIST& aComponentsList )
{
    int NumRef;

    NumRef = aComponentsList[aObjet].m_NumRef;

    for( unsigned ii = 0; ii < aComponentsList.GetCount(); ii++ )
    {
        if( aObjet == (int) ii )
            // Do not compare with itself !
            continue;
        if( aComponentsList[ii].m_IsNew )
            // Not already with an updated reference
            continue;
        if( aComponentsList[ii].m_NumRef != NumRef )
            // Not the same reference number (like 35 in R35)
            continue;
        if( aComponentsList[aObjet].CompareRef( aComponentsList[ii] ) != 0 )
            // Not the same reference prefix
            continue;
        if( aComponentsList[ii].m_Unit == Unit )
        {
            // A part with the same reference and the given unit is found
            return ii;
        }
    }

    return -1;
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
    int            error;
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

    ComponentsList.SortComponentsByRefAndValue();

    /* Break full components reference in name (prefix) and number: example:
     * IC1 become IC, and 1 */
    BreakReference( ComponentsList );

    /* count not yet annotated items */
    error = 0;
    int imax = ComponentsList.GetCount() - 1;

    for( int ii = 0; ii < imax; ii++ )
    {
        msg.Empty();
        Buff.Empty();

        if( ComponentsList[ii].m_IsNew )
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

        // Annotate error
        if( MAX( ComponentsList[ii].m_Entry->GetPartCount(), 1 ) < ComponentsList[ii].m_Unit )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = ComponentsList[ii].GetRef();

            msg.Printf( _( "Error item %s%s" ), GetChars( cmpref ), GetChars( Buff ) );

            Buff.Printf( _( " unit %d and no more than %d parts" ),
                         ComponentsList[ii].m_Unit,
                         ComponentsList[ii].m_Entry->GetPartCount() );
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
        if( ComponentsList[ii].m_Entry->GetPartCount()
            != ComponentsList[ii + 1].m_Entry->GetPartCount() )
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

        /* Error if values are different between units, for the same
         * reference */
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
           || ( ComponentsList[ii].m_SheetPath != ComponentsList[ii + 1].m_SheetPath ) )
            continue;

        /* Same time stamp found.  */
        wxString nextcmpref;
        wxString full_path;

        full_path.Printf( wxT( "%s%8.8X" ),
                          GetChars( ComponentsList[ii].m_SheetPath.Path() ),
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
