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
#include "program.h"
#include "libcmp.h"
#include "protos.h"
#include "netlist.h"


/* Local Functions*/
static int  AddComponentsInSheetToList( std::vector <OBJ_CMP_TO_LIST>& aComponentsList,
                                DrawSheetPath*                 sheet );
static void BreakReference( std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static void ReAnnotateComponents( std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static void ComputeReferenceNumber( std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
int         GetLastReferenceNumber( int                            aObjet,
                                    std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static int  ExistUnit( int aObjet, int aUnit,
                       std::vector <OBJ_CMP_TO_LIST>& aComponentsList );


/******************************************************/
void WinEDA_SchematicFrame::UpdateSheetNumberAndDate()
/******************************************************/

/* Set a sheet number, the sheet count for sheets in the whole schematic
 * and update the date in all screens
 */
{
    wxString       date = GenDate();
    EDA_ScreenList s_list;

    // Set the date
    for( SCH_SCREEN* screen = s_list.GetFirst(); screen != NULL; screen = s_list.GetNext() )
        screen->m_Date = date;

    // Set sheet counts
    SetSheetNumberAndCount();
}


/*****************************************************************************
 * Used to annotate the power symbols, before testing erc or computing
 * netlist when a component reannotation is not necessary
 *
 * In order to avoid conflicts the reference number starts with a 0. A
 * PWR with id 12 is named PWR12 in global annotation and PWR012 by the
 * Power annotation.
 ****************************************************************************/
void ReAnnotatePowerSymbolsOnly( void )
{
    /* Build the whole sheet list in hierarchy (sheet, not screen) */
    EDA_SheetList  SheetList;

    DrawSheetPath* sheet;
    int            CmpNumber = 1;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        EDA_BaseStruct* DrawList = sheet->LastDrawList();
        for( ; DrawList != NULL; DrawList = DrawList->Next() )
        {
            if( DrawList->Type() != TYPE_SCH_COMPONENT )
                continue;
            SCH_COMPONENT*          DrawLibItem =
                (SCH_COMPONENT*) DrawList;
            EDA_LibComponentStruct* Entry = FindLibPart(
                DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
            if( (Entry == NULL) || (Entry->m_Options != ENTRY_POWER) )
                continue;

            //DrawLibItem->ClearAnnotation(sheet); this clears all annotation :(
            wxString refstr = DrawLibItem->m_PrefixString;

            //str will be "C?" or so after the ClearAnnotation call.
            while( refstr.Last() == '?' )
                refstr.RemoveLast();

            if( !refstr.StartsWith( wxT( "#" ) ) )
                refstr = wxT( "#" ) + refstr;
            refstr << wxT( "0" ) << CmpNumber;
            DrawLibItem->SetRef( sheet, refstr );
            CmpNumber++;
        }
    }
}


/* sort function to annotate items by their position.
 *  Components are sorted
 *      by reference
 *      if same reference: by sheet
 *          if same sheet, by X pos
 *                if same X pos, by Y pos
 *                  if same Y pos, by time stamp
 */
static bool AnnotateBy_X_Position( const OBJ_CMP_TO_LIST& item1, const OBJ_CMP_TO_LIST& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.m_SheetPath.Cmp( item2.m_SheetPath );
    if( ii == 0 )
        ii = item1.m_RootCmp->m_Pos.x - item2.m_RootCmp->m_Pos.x;
    if( ii == 0 )
        ii = item1.m_RootCmp->m_Pos.y - item2.m_RootCmp->m_Pos.y;
    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


/* sort function to annotate items by their position.
 *  Components are sorted
 *      by reference
 *      if same reference: by sheet
 *          if same sheet, by Y pos
 *                if same Y pos, by X pos
 *                  if same X pos, by time stamp
 */
static bool AnnotateBy_Y_Position( const OBJ_CMP_TO_LIST& item1, const OBJ_CMP_TO_LIST& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.m_SheetPath.Cmp( item2.m_SheetPath );
    if( ii == 0 )
        ii = item1.m_RootCmp->m_Pos.y - item2.m_RootCmp->m_Pos.y;
    if( ii == 0 )
        ii = item1.m_RootCmp->m_Pos.x - item2.m_RootCmp->m_Pos.x;
    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


/*****************************************************************************
* qsort function to annotate items by value
*  Components are sorted
*      by reference
*      if same reference: by value
*          if same value: by unit number
*              if same unit number, by sheet
*                  if same sheet, by time stamp
*****************************************************************************/
static bool AnnotateByValue( const OBJ_CMP_TO_LIST& item1, const OBJ_CMP_TO_LIST& item2 )
{
    int ii = item1.CompareRef( item2 );

    if( ii == 0 )
        ii = item1.CompareValue( item2 );
    if( ii == 0 )
        ii = item1.m_Unit - item2.m_Unit;
    if( ii == 0 )
        ii = item1.m_SheetPath.Cmp( item2.m_SheetPath );
    if( ii == 0 )
        ii = item1.m_RootCmp->m_Pos.x - item2.m_RootCmp->m_Pos.x;
    if( ii == 0 )
        ii = item1.m_RootCmp->m_Pos.y - item2.m_RootCmp->m_Pos.y;
    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


/**************************************************************************************/
void WinEDA_SchematicFrame::DeleteAnnotation( bool aCurrentSheetOnly, bool aRedraw )
/**************************************************************************************/

/** Function DeleteAnnotation
 * Remove current component annotations
 * @param aCurrentSheetOnly : if false: remove all annotations, else remove annotation relative to the current sheet only
 * @param aRedraw : true to refresh display
 */
{
    SCH_ITEM*      strct;
    SCH_SCREEN*    screen;
    EDA_ScreenList ScreenList;

    screen = ScreenList.GetFirst();

    if( aCurrentSheetOnly )
        screen = GetScreen();

    if( screen == NULL )
        return;
    while( screen )
    {
        strct = screen->EEDrawList;
        for( ; strct; strct = strct->Next() )
        {
            if( strct->Type() == TYPE_SCH_COMPONENT )
            {
                if( aCurrentSheetOnly )
                    ( (SCH_COMPONENT*) strct )->ClearAnnotation( m_CurrentSheet );
                else
                    ( (SCH_COMPONENT*) strct )->ClearAnnotation( NULL );
            }
        }

        screen->SetModify();
        if( aCurrentSheetOnly )
            break;
        screen = ScreenList.GetNext();
    }


    //update the References
    m_CurrentSheet->UpdateAllScreenReferences();

    if( aRedraw )
        DrawPanel->Refresh( true );
}


/*****************************************************************************
* AnnotateComponents:
*
*  Compute the annotation of the components for the whole project, or the
*  current sheet only.  All the components or the new ones only will be
*  annotated.
* @param parent = Schematic frame
* @param annotateSchematic : true = entire schematic annotation, false = current scheet only
* @param sortOption : 0 = annotate by sorting X position,
*                     1 = annotate by sorting Y position,
*                     2 = annotate by sorting value
* @param resetAnnotation : true = remove previous annotation false = anotate new components only
*****************************************************************************/
void AnnotateComponents( WinEDA_SchematicFrame* parent,
                         bool                   annotateSchematic,
                         int                    sortOption,
                         bool                   resetAnnotation )
{
    std::vector <OBJ_CMP_TO_LIST> ComponentsList;

    wxBusyCursor dummy;

    /* If it is an annotation for all the components, reset previous
     * annotation: */
    if( resetAnnotation )
        parent->DeleteAnnotation( !annotateSchematic, false );

    /* Build the sheet list */
    EDA_SheetList SheetList;

    /* Update the sheet number, sheet count and date */
    parent->UpdateSheetNumberAndDate();

    /* Build component list */
    if( annotateSchematic )
    {
        DrawSheetPath* sheet;
        for( sheet = SheetList.GetFirst();
            sheet != NULL;
            sheet = SheetList.GetNext() )
            AddComponentsInSheetToList( ComponentsList, sheet );
    }
    else
        AddComponentsInSheetToList( ComponentsList, parent->GetSheet() );


    /* Break full components reference in name (prefix) and number:
     * example: IC1 become IC, and 1 */
    BreakReference( ComponentsList );

    switch( sortOption )
    {
    case 0:
        sort( ComponentsList.begin(), ComponentsList.end(), AnnotateBy_X_Position );
        break;

    case 1:
        sort( ComponentsList.begin(), ComponentsList.end(), AnnotateBy_Y_Position );
        break;

    case 2:
        sort( ComponentsList.begin(), ComponentsList.end(), AnnotateByValue );
        break;
    }

    /* Recalculate reference numbers */
    ComputeReferenceNumber( ComponentsList );
    ReAnnotateComponents( ComponentsList );

    /* Final control (just in case ... )*/
    CheckAnnotate( parent, !annotateSchematic );
    parent->DrawPanel->Refresh( true );
}


/*******************************************************************************************************/
int AddComponentsInSheetToList(  std::vector <OBJ_CMP_TO_LIST>& aComponentsList, DrawSheetPath* aSheet )
/********************************************************************************************************/
/** function AddComponentsInSheetToList()
 * Add a OBJ_CMP_TO_LIST object in aComponentsList for each component found in sheet
 * @param aComponentsList = a std::vector list to fill
 * @param the DrawSheetPath sheet to analyse
*/
{
    int                     NbrCmp   = 0;
    EDA_BaseStruct*         DrawList = aSheet->LastDrawList();
    SCH_COMPONENT*          DrawLibItem;
    EDA_LibComponentStruct* Entry;

    for( ; DrawList != NULL;   DrawList = DrawList->Next() )
    {
        if( DrawList->Type() == TYPE_SCH_COMPONENT )
        {
            DrawLibItem = (SCH_COMPONENT*) DrawList;
            Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(),
                wxEmptyString,
                FIND_ROOT );
            if( Entry == NULL )
                continue;

            OBJ_CMP_TO_LIST new_object;
            new_object.m_RootCmp     = DrawLibItem;
            new_object.m_Entry       = Entry;
            new_object.m_Unit        = DrawLibItem->GetUnitSelection( aSheet );
            new_object.m_SheetPath   = *aSheet;
            new_object.m_IsNew       = false;
            new_object.m_Flag        = 0;
            new_object.m_TimeStamp = DrawLibItem->m_TimeStamp;

            if( DrawLibItem->GetRef( aSheet ).IsEmpty() )
                DrawLibItem->SetRef( aSheet, wxT( "DefRef?" ) );

            strncpy( new_object.m_Reference, CONV_TO_UTF8( DrawLibItem->GetRef( aSheet ) ), 32 );

            new_object.m_NumRef = -1;

            if( DrawLibItem->GetField( VALUE )->m_Text.IsEmpty() )
                DrawLibItem->GetField( VALUE )->m_Text = wxT( "~" );

            new_object.m_Value = &DrawLibItem->GetField( VALUE )->m_Text;

            aComponentsList.push_back( new_object );
            NbrCmp++;
        }
    }

    return NbrCmp;
}


/*****************************************************************************
* Update the reference component for the schematic project (or the current
* sheet)
*****************************************************************************/
static void ReAnnotateComponents( std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    /* update the reference numbers */
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        char*          Text = aComponentsList[ii].m_Reference;
        SCH_COMPONENT* component = aComponentsList[ii].m_RootCmp;

        if( aComponentsList[ii].m_NumRef < 0 )
            strcat( Text, "?" );
        else
            sprintf( Text + strlen( Text ), "%d", aComponentsList[ii].m_NumRef );

        component->SetRef( &(aComponentsList[ii].m_SheetPath), CONV_FROM_UTF8( Text ) );
        component->m_Multi = aComponentsList[ii].m_Unit;
        component->SetUnitSelection( &(aComponentsList[ii].m_SheetPath),
            aComponentsList[ii].m_Unit );
    }
}


/*****************************************************************************
* Split component reference designators into a name (prefix) and number.
* Example: IC1 becomes IC and 1 in the .m_NumRef member.
* For multi part per package components not already annotated, set .m_Unit
* to a max value (0x7FFFFFFF).
*
* @param aComponentsList = list of component
* @param NbOfCmp   = item count in the list
*****************************************************************************/
void BreakReference( std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    char* Text;

    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        aComponentsList[ii].m_NumRef = -1;
        Text = aComponentsList[ii].m_Reference;
        int ll = strlen( Text ) - 1;
        if( Text[ll] == '?' )
        {
            aComponentsList[ii].m_IsNew = true;
            if( !aComponentsList[ii].IsPartsLocked() )
                aComponentsList[ii].m_Unit = 0x7FFFFFFF;
            Text[ll] = 0;
            continue;
        }

        if( isdigit( Text[ll] ) == 0 )
        {
            aComponentsList[ii].m_IsNew = true;
            if( !aComponentsList[ii].IsPartsLocked() )
                aComponentsList[ii].m_Unit = 0x7FFFFFFF;
            continue;
        }

        while( ll >= 0 )
        {
            if( (Text[ll] <= ' ' ) || isdigit( Text[ll] ) )
                ll--;
            else
            {
                if( isdigit( Text[ll + 1] ) )
                    aComponentsList[ii].m_NumRef = atoi( &Text[ll + 1] );
                Text[ll + 1] = 0;
                break;
            }
        }
    }
}


/*****************************************************************************
* Compute the reference number for components without reference number
*  Compute .m_NumRef member
*****************************************************************************/
static void ComputeReferenceNumber( std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    int LastReferenceNumber, NumberOfUnits, Unit;

    /* Components with an invisible reference (power...) always are re-annotated.
     * So set theirs .m_IsNew member to true
     */
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        if( aComponentsList[ii].m_Reference[0] == '#' )
        {
            aComponentsList[ii].m_IsNew  = true;
            aComponentsList[ii].m_NumRef = 0;
        }
    }

    /* calculate index of the first component with the same reference prefix than the current component.
     *  all of component having the same reference prefix will receive a reference number with consecutives values:
     *  IC .. will be set to IC4, IC4, IC5 ...
     */
    unsigned first = 0;
    /* calculate the last used number for this reference prefix: */
    LastReferenceNumber = GetLastReferenceNumber( first, aComponentsList );
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        if( aComponentsList[ii].m_Flag )
            continue;

        if( aComponentsList[first].CompareRef( aComponentsList[ii] ) != 0 ) /* New reference found: we need a new ref number for this reference */
        {
            first = ii;
            LastReferenceNumber = GetLastReferenceNumber( ii, aComponentsList );
        }

        /* Annotation of one part per package components (trivial case)*/
        if( aComponentsList[ii].m_Entry->m_UnitCount <= 1 )
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
        NumberOfUnits = aComponentsList[ii].m_Entry->m_UnitCount;

        if( aComponentsList[ii].m_IsNew )
        {
            LastReferenceNumber++;
            aComponentsList[ii].m_NumRef = LastReferenceNumber;

            if( !aComponentsList[ii].IsPartsLocked() )
                aComponentsList[ii].m_Unit = 1;
            aComponentsList[ii].m_Flag = 1;
        }

        /* search for others units of this component.
         * we search for others parts that have the same value and the same reference prefix (ref without ref number)
         */
        for( Unit = 1; Unit <= NumberOfUnits; Unit++ )
        {
            if( aComponentsList[ii].m_Unit == Unit )
                continue;
            int found = ExistUnit( ii, Unit, aComponentsList );
            if( found >= 0 )
                continue; /* this unit exists for this reference (unit already annotated) */

            /* Search a component to annotate ( same prefix, same value, not annotated) */
            for( unsigned jj = ii + 1; jj < aComponentsList.size(); jj++ )
            {
                if( aComponentsList[jj].m_Flag )    // already tested
                    continue;

                if( aComponentsList[ii].CompareRef( aComponentsList[jj] ) != 0 ) // references are different
                    continue;
                if( aComponentsList[jj].CompareValue( aComponentsList[ii] ) != 0 )
                    continue; // values are different
                if( !aComponentsList[jj].m_IsNew )
                    continue;

                /* Component without reference number found, annotate it if possible */
                if( !aComponentsList[jj].IsPartsLocked() || (aComponentsList[jj].m_Unit == Unit) )
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


/*****************************************************************************
* Search the last used (greatest) reference number in the component list
* for the prefix reference given by Objet
* The component list must be sorted.
*
* @param aObjet = reference item ( aComponentsList[aObjet].m_TextRef is the search pattern)
* @param aComponentsList = list of items
*****************************************************************************/
int GetLastReferenceNumber( int aObjet, std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    int LastNumber = 0;

    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        /* Nouveau Identificateur */
        if( aComponentsList[aObjet].CompareRef( aComponentsList[ii] ) != 0 )
            continue;
        if( LastNumber < aComponentsList[ii].m_NumRef )
            LastNumber = aComponentsList[ii].m_NumRef;
    }

    return LastNumber;
}


/*****************************************************************************
* Search in the sorted list of components, for a given componen,t an other component
* with the same reference and a given part unit.
* Mainly used to manage multiple parts per package components
* @param aObjet = index in aComponentsList for the given OBJ_CMP_TO_LIST item to test
* @param Unit = the given unit number to search
* @param aComponentsList = list of items to examine
* @return index in aComponentsList if found or -1 if not found
*****************************************************************************/
static int ExistUnit( int aObjet, int Unit,
                      std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    int NumRef;

    NumRef = aComponentsList[aObjet].m_NumRef;
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        if( aObjet == (int) ii )                                                // Do not compare with itself !
            continue;
        if( aComponentsList[ii].m_IsNew )                                       // Not already with an updated reference
            continue;
        if( aComponentsList[ii].m_NumRef != NumRef )                            // Not the same reference number (like 35 in R35)
            continue;
        if( aComponentsList[aObjet].CompareRef( aComponentsList[ii] ) != 0 )    // Not the same reference prefix
            continue;
        if( aComponentsList[ii].m_Unit == Unit )                                // A part with the same reference and the given unit is found
        {
            return ii;
        }
    }

    return -1;
}


/*******************************************************************/
int CheckAnnotate( WinEDA_SchematicFrame* frame, bool oneSheetOnly )
/*******************************************************************/

/**
 * Function CheckAnnotate
 *  Check errors relatives to annotation:
 *      components not annotated
 *      components having the same reference (duplicates)
 *      for multiple parts per package components :
 *          part number > number of parts
 *          different values between parts
 * @return errors count
 * @param oneSheetOnly : true = search is made only in the current sheet
 *                       false = search in whole hierarchy (usual search).
 */
{
    int           error;
    wxString      Buff;
    wxString      msg, cmpref;

    /* build the screen list */
    EDA_SheetList SheetList;

    std::vector <OBJ_CMP_TO_LIST> ComponentsList;

    g_RootSheet->m_AssociatedScreen->SetModify();

    /* Build the list of components */
    if( !oneSheetOnly )
    {
        DrawSheetPath* sheet;
        for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
            AddComponentsInSheetToList( ComponentsList, sheet );
    }
    else
        AddComponentsInSheetToList( ComponentsList, frame->GetSheet() );

    sort( ComponentsList.begin(), ComponentsList.end(), AnnotateByValue );

    /* Break full components reference in name (prefix) and number: example:
     * IC1 become IC, and 1 */
    BreakReference( ComponentsList );

    /* count not yet annotated items */
    error = 0;
    int imax = ComponentsList.size() - 1;
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

            cmpref = CONV_FROM_UTF8( ComponentsList[ii].m_Reference );
            msg.Printf( _( "item not annotated: %s%s" ),
                cmpref.GetData(), Buff.GetData() );

            if( (ComponentsList[ii].m_Unit > 0) && (ComponentsList[ii].m_Unit < 0x7FFFFFFF) )
            {
                Buff.Printf( _( "( unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }
            DisplayError( frame, msg );
            error++;
            break;
        }

        // Annotate error
        if( MAX( ComponentsList[ii].m_Entry->m_UnitCount, 1 ) < ComponentsList[ii].m_Unit  )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = CONV_FROM_UTF8( ComponentsList[ii].m_Reference );
            msg.Printf( _( "Error item %s%s" ), cmpref.GetData(),
                Buff.GetData() );

            Buff.Printf( _( " unit %d and no more than %d parts" ),
                ComponentsList[ii].m_Unit, ComponentsList[ii].m_Entry->m_UnitCount );
            msg << Buff;
            DisplayError( frame, msg );
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

        if( (ComponentsList[ii].CompareRef(ComponentsList[ii + 1]) != 0)
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

            cmpref = CONV_FROM_UTF8( ComponentsList[ii].m_Reference );
            msg.Printf( _( "Multiple item %s%s" ),
                cmpref.GetData(), Buff.GetData() );

            if( (ComponentsList[ii].m_Unit > 0) && (ComponentsList[ii].m_Unit < 0x7FFFFFFF) )
            {
                Buff.Printf( _( " (unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }
            DisplayError( frame, msg );
            error++;
            continue;
        }

        /* Test error if units are different but number of parts per package
         * too hight (ex U3 ( 1 part) and we find U3B this is an error) */
        if( ComponentsList[ii].m_Entry->m_UnitCount != ComponentsList[ii + 1].m_Entry->m_UnitCount )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = CONV_FROM_UTF8( ComponentsList[ii].m_Reference );
            msg.Printf( _( "Multiple item %s%s" ),
                cmpref.GetData(), Buff.GetData() );

            if( (ComponentsList[ii].m_Unit > 0) && (ComponentsList[ii].m_Unit < 0x7FFFFFFF) )
            {
                Buff.Printf( _( " (unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }

            DisplayError( frame, msg );
            error++;
        }

        /* Error if values are different between units, for the same reference */
        int next = ii+1;
        if( ComponentsList[ii].CompareValue( ComponentsList[next] ) != 0 )
        {
            wxString nextcmpref;
            cmpref     = CONV_FROM_UTF8( ComponentsList[ii].m_Reference );
            nextcmpref = CONV_FROM_UTF8( ComponentsList[next].m_Reference );
#if defined (KICAD_GOST)
            msg.Printf( _( "Diff values for %s%d.%c (%s) and %s%d.%c (%s)" ),
                cmpref.GetData(),
                ComponentsList[ii].m_NumRef,
                ComponentsList[ii].m_Unit + '1' - 1,
                ComponentsList[ii].m_Value->GetData(), nextcmpref.GetData(),
                ComponentsList[next].m_NumRef,
                ComponentsList[next].m_Unit + '1' - 1,
                ComponentsList[next].m_Value->GetData() );
#else
            msg.Printf( _( "Diff values for %s%d%c (%s) and %s%d%c (%s)" ),
                cmpref.GetData(),
                ComponentsList[ii].m_NumRef,
                ComponentsList[ii].m_Unit + 'A' - 1,
                ComponentsList[ii].m_Value->GetData(), nextcmpref.GetData(),
                ComponentsList[next].m_NumRef,
                ComponentsList[next].m_Unit + 'A' - 1,
                ComponentsList[next].m_Value->GetData() );
#endif

            DisplayError( frame, msg );
            error++;
        }
    }

    return error;
}
