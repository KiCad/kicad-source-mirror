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
#include "class_library.h"
#include "protos.h"
#include "netlist.h"
#include "class_pin.h"


static int  AddComponentsInSheetToList( std::vector <OBJ_CMP_TO_LIST>& aComponentsList,
                                        SCH_SHEET_PATH*                sheet );
static void BreakReference( std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static void ReAnnotateComponents( std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static void ComputeReferenceNumber( std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
int         GetLastReferenceNumber( int                            aObjet,
                                    std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static int  ExistUnit( int aObjet, int aUnit,
                       std::vector <OBJ_CMP_TO_LIST>& aComponentsList );
static int  ReplaceDuplicatedTimeStamps();


/* Set a sheet number, the sheet count for sheets in the whole schematic
 * and update the date in all screens
 */
void WinEDA_SchematicFrame::UpdateSheetNumberAndDate()
{
    wxString       date = GenDate();
    EDA_ScreenList s_list;

    // Set the date
    for( SCH_SCREEN* screen = s_list.GetFirst(); screen != NULL;
         screen = s_list.GetNext() )
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
    SCH_SHEET_LIST  SheetList;

    SCH_SHEET_PATH* sheet;
    int             CmpNumber = 1;

    for( sheet = SheetList.GetFirst(); sheet != NULL;
         sheet = SheetList.GetNext() )
    {
        EDA_BaseStruct* DrawList = sheet->LastDrawList();
        for( ; DrawList != NULL; DrawList = DrawList->Next() )
        {
            if( DrawList->Type() != TYPE_SCH_COMPONENT )
                continue;
            SCH_COMPONENT* DrawLibItem = (SCH_COMPONENT*) DrawList;
            LIB_COMPONENT* Entry =
                CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );

            if( ( Entry == NULL ) || !Entry->IsPower() )
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
static bool AnnotateBy_X_Position( const OBJ_CMP_TO_LIST& item1,
                                   const OBJ_CMP_TO_LIST& item2 )
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
static bool AnnotateBy_Y_Position( const OBJ_CMP_TO_LIST& item1,
                                   const OBJ_CMP_TO_LIST& item2 )
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
static bool AnnotateByValue( const OBJ_CMP_TO_LIST& item1,
                             const OBJ_CMP_TO_LIST& item2 )
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


/*****************************************************************************
 * qsort function to annotate items by value
 *  Components are sorted by time stamp
 *****************************************************************************/
static bool SortByTimeStamp( const OBJ_CMP_TO_LIST& item1,
                             const OBJ_CMP_TO_LIST& item2 )
{
    int ii = item1.m_SheetPath.Cmp( item2.m_SheetPath );

    if( ii == 0 )
        ii = item1.m_TimeStamp - item2.m_TimeStamp;

    return ii < 0;
}


/** Function DeleteAnnotation
 * Remove current component annotations
 * @param aCurrentSheetOnly : if false: remove all annotations, else remove
 *                             annotation relative to the current sheet only
 * @param aRedraw : true to refresh display
 */
void WinEDA_SchematicFrame::DeleteAnnotation( bool aCurrentSheetOnly,
                                              bool aRedraw )
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

        OnModify( );
        if( aCurrentSheetOnly )
            break;
        screen = ScreenList.GetNext();
    }


    //update the References
    m_CurrentSheet->UpdateAllScreenReferences();

    if( aRedraw )
        DrawPanel->Refresh( true );
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
void AnnotateComponents( WinEDA_SchematicFrame* parent,
                         bool                   annotateSchematic,
                         int                    sortOption,
                         bool                   resetAnnotation,
                         bool                   repairsTimestamps )
{
    std::vector <OBJ_CMP_TO_LIST> ComponentsList;

    wxBusyCursor dummy;

    // Test and replace duplicate time stamps
    // duplicate can happen with old schematics, or schematic conversions or
    // manual editions of files ...
    if( repairsTimestamps )
    {
        int ireplacecount = ReplaceDuplicatedTimeStamps();
        if( ireplacecount )
        {
            wxString msg;
            msg.Printf( _( "%d duplicate time stamps replaced." ),
                        ireplacecount );
            DisplayInfoMessage( NULL, msg, 2 );
        }
    }

    /* If it is an annotation for all the components, reset previous
     * annotation: */
    if( resetAnnotation )
        parent->DeleteAnnotation( !annotateSchematic, false );

    /* Build the sheet list */
    SCH_SHEET_LIST SheetList;

    /* Update the sheet number, sheet count and date */
    parent->UpdateSheetNumberAndDate();

    /* Build component list */
    if( annotateSchematic )
    {
        SCH_SHEET_PATH* sheet;
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
        sort( ComponentsList.begin(), ComponentsList.end(),
              AnnotateBy_X_Position );
        break;

    case 1:
        sort( ComponentsList.begin(), ComponentsList.end(),
              AnnotateBy_Y_Position );
        break;

    case 2:
        sort( ComponentsList.begin(), ComponentsList.end(), AnnotateByValue );
        break;
    }

    /* Recalculate reference numbers */
    ComputeReferenceNumber( ComponentsList );
    ReAnnotateComponents( ComponentsList );

    /* Final control (just in case ... )*/
    parent->CheckAnnotate( NULL, !annotateSchematic );
    parent->OnModify( );
    parent->DrawPanel->Refresh( true );
}


/** function AddComponentsInSheetToList()
 * Add a OBJ_CMP_TO_LIST object in aComponentsList for each component found
 * in sheet
 * @param aComponentsList = a std::vector list to fill
 * @param the SCH_SHEET_PATH sheet to analyze
 */
int AddComponentsInSheetToList(  std::vector <OBJ_CMP_TO_LIST>& aComponentsList,
                                 SCH_SHEET_PATH*                aSheet )
{
    int             NbrCmp   = 0;
    EDA_BaseStruct* DrawList = aSheet->LastDrawList();
    SCH_COMPONENT*  DrawLibItem;
    LIB_COMPONENT*  Entry;

    for( ; DrawList != NULL;   DrawList = DrawList->Next() )
    {
        if( DrawList->Type() == TYPE_SCH_COMPONENT )
        {
            DrawLibItem = (SCH_COMPONENT*) DrawList;
            Entry =
                CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );
            if( Entry == NULL )
                continue;

            OBJ_CMP_TO_LIST new_object;
            new_object.m_RootCmp   = DrawLibItem;
            new_object.m_Entry     = Entry;
            new_object.m_Unit      = DrawLibItem->GetUnitSelection( aSheet );
            new_object.m_SheetPath = *aSheet;
            new_object.m_IsNew     = false;
            new_object.m_Flag      = 0;
            new_object.m_TimeStamp = DrawLibItem->m_TimeStamp;

            if( DrawLibItem->GetRef( aSheet ).IsEmpty() )
                DrawLibItem->SetRef( aSheet, wxT( "DefRef?" ) );

            new_object.SetRef( DrawLibItem->GetRef( aSheet ) );

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


/*
 * Update the reference component for the schematic project (or the current
 * sheet)
 */
static void ReAnnotateComponents( std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    /* update the reference numbers */
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
#if 0
        char*          Text = aComponentsList[ii].m_Reference;
        SCH_COMPONENT* component = aComponentsList[ii].m_RootCmp;

        if( aComponentsList[ii].m_NumRef < 0 )
            strcat( Text, "?" );
        else
            sprintf( Text + strlen( Text ), "%d", aComponentsList[ii].m_NumRef );

        component->SetRef( &(aComponentsList[ii].m_SheetPath),
                           CONV_FROM_UTF8( Text ) );
#else

        wxString        ref = aComponentsList[ii].GetRef();
        SCH_COMPONENT*  component = aComponentsList[ii].m_RootCmp;

        if( aComponentsList[ii].m_NumRef < 0 )
            ref += wxChar( '?' );
        else
            ref << aComponentsList[ii].m_NumRef;

        aComponentsList[ii].SetRef( ref );

        component->SetRef( &aComponentsList[ii].m_SheetPath, ref );
#endif

        component->m_Multi = aComponentsList[ii].m_Unit;
        component->SetUnitSelection( &aComponentsList[ii].m_SheetPath,
                                     aComponentsList[ii].m_Unit );
    }
}


/**
 * Split component reference designators into a name (prefix) and number.
 * Example: IC1 becomes IC and 1 in the .m_NumRef member.
 * For multi part per package components not already annotated, set .m_Unit
 * to a max value (0x7FFFFFFF).
 *
 * @param aComponentsList = list of component
 * @param NbOfCmp   = item count in the list
 */
void BreakReference( std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    std::string refText;    // construct once outside loop

    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
    {
        aComponentsList[ii].m_NumRef = -1;

        refText = aComponentsList[ii].GetRefStr();

        int ll = refText.length() - 1;

        if( refText[ll] == '?' )
        {
            aComponentsList[ii].m_IsNew = true;

            if( !aComponentsList[ii].IsPartsLocked() )
                aComponentsList[ii].m_Unit = 0x7FFFFFFF;

            refText.erase(ll);  // delete last char

            aComponentsList[ii].SetRefStr( refText );
        }

        else if( isdigit( refText[ll] ) == 0 )
        {
            aComponentsList[ii].m_IsNew = true;
            if( !aComponentsList[ii].IsPartsLocked() )
                aComponentsList[ii].m_Unit = 0x7FFFFFFF;
        }

        else
        {
            while( ll >= 0 )
            {
                if( (refText[ll] <= ' ' ) || isdigit( refText[ll] ) )
                    ll--;
                else
                {
                    if( isdigit( refText[ll + 1] ) )
                    {
                        // nul terminated C string into cp
                        const char* cp = refText.c_str() + ll + 1;

                        aComponentsList[ii].m_NumRef = atoi( cp );
                    }

                    refText.erase( ll+1 );  // delete from ll+1 to end
                    break;
                }
            }

            aComponentsList[ii].SetRefStr( refText );
        }
    }
}


/*
 * Compute the reference number for components without reference number
 *  Compute .m_NumRef member
 */
static void ComputeReferenceNumber( std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    int LastReferenceNumber, NumberOfUnits, Unit;

    /* Components with an invisible reference (power...) always are
     * re-annotated.  So set their .m_IsNew member to true
     */
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
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
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
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
            for( unsigned jj = ii + 1; jj < aComponentsList.size(); jj++ )
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
int GetLastReferenceNumber( int aObjet,
                            std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    int LastNumber = 0;

    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
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
 * @param aObjet = index in aComponentsList for the given OBJ_CMP_TO_LIST
 *                 item to test
 * @param Unit = the given unit number to search
 * @param aComponentsList = list of items to examine
 * @return index in aComponentsList if found or -1 if not found
 */
static int ExistUnit( int aObjet, int Unit,
                      std::vector <OBJ_CMP_TO_LIST>& aComponentsList )
{
    int NumRef;

    NumRef = aComponentsList[aObjet].m_NumRef;
    for( unsigned ii = 0; ii < aComponentsList.size(); ii++ )
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
int WinEDA_SchematicFrame::CheckAnnotate( wxArrayString* aMessageList,
                                          bool           aOneSheetOnly )
{
    int            error;
    wxString       Buff;
    wxString       msg, cmpref;

    /* build the screen list */
    SCH_SHEET_LIST SheetList;

    std::vector <OBJ_CMP_TO_LIST> ComponentsList;

    /* Build the list of components */
    if( !aOneSheetOnly )
    {
        SCH_SHEET_PATH* sheet;
        for( sheet = SheetList.GetFirst(); sheet != NULL;
             sheet = SheetList.GetNext() )
            AddComponentsInSheetToList( ComponentsList, sheet );
    }
    else
        AddComponentsInSheetToList( ComponentsList, GetSheet() );

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

            cmpref = ComponentsList[ii].GetRef();
            msg.Printf( _( "item not annotated: %s%s" ),
                       GetChars( cmpref ), GetChars( Buff ) );

            if( ( ComponentsList[ii].m_Unit > 0 )
                && ( ComponentsList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                Buff.Printf( _( "( unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }

            if( aMessageList )
            {
                aMessageList->Add( msg + wxT( "\n" ) );
            }
            else
            {
                DisplayError( NULL, msg );
            }
            error++;
            break;
        }

        // Annotate error
        if( MAX( ComponentsList[ii].m_Entry->GetPartCount(), 1 )
            < ComponentsList[ii].m_Unit )
        {
            if( ComponentsList[ii].m_NumRef >= 0 )
                Buff << ComponentsList[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = ComponentsList[ii].GetRef();

            msg.Printf( _( "Error item %s%s" ), GetChars( cmpref ),
                       GetChars( Buff ) );

            Buff.Printf( _( " unit %d and no more than %d parts" ),
                         ComponentsList[ii].m_Unit,
                         ComponentsList[ii].m_Entry->GetPartCount() );
            msg << Buff;
            if( aMessageList )
            {
                aMessageList->Add( msg + wxT( "\n" ));
            }
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

            msg.Printf( _( "Multiple item %s%s" ),
                       GetChars( cmpref ), GetChars( Buff ) );

            if( ( ComponentsList[ii].m_Unit > 0 )
                && ( ComponentsList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                Buff.Printf( _( " (unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }
            if( aMessageList )
            {
                aMessageList->Add( msg + wxT( "\n" ));
            }
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
            msg.Printf( _( "Multiple item %s%s" ),
                       GetChars( cmpref ), GetChars( Buff ) );

            if( ( ComponentsList[ii].m_Unit > 0 )
                && ( ComponentsList[ii].m_Unit < 0x7FFFFFFF ) )
            {
                Buff.Printf( _( " (unit %d)" ), ComponentsList[ii].m_Unit );
                msg << Buff;
            }

            if( aMessageList )
            {
                aMessageList->Add( msg + wxT( "\n" ));
            }
            else
            {
                DisplayError( NULL, msg );
            }
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
            {
                aMessageList->Add( msg + wxT( "\n" ));
            }
            else
            {
                DisplayError( NULL, msg );
            }

            error++;
        }
    }

    // count the duplicated time stamps
    sort( ComponentsList.begin(), ComponentsList.end(), SortByTimeStamp );
    for( int ii = 0; ( ii < imax ) && ( error < 4 ); ii++ )
    {
        if( (ComponentsList[ii].m_TimeStamp
             != ComponentsList[ii + 1].m_TimeStamp)
           || ( ComponentsList[ii].m_SheetPath
                != ComponentsList[ii + 1].m_SheetPath ) )
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
        {
            aMessageList->Add( msg + wxT( "\n" ));
        }
        else
        {
            DisplayError( NULL, msg );
        }

        error++;
    }

    return error;
}


/***********************************************
 * function to sort sch_items by time stamp
 ************************************************/
static bool SortItemByTimeStamp( const SCH_ITEM* item1, const SCH_ITEM* item2 )
{
    int ii = item1->m_TimeStamp - item2->m_TimeStamp;

    /* if same time stamp, compare type, in order to have
     *  first : component
     *  after : sheet
     * because this is the first item that have its time stamp changed
     * and changing the time stamp of a sheet can loose annotation
     */

    if( ii == 0 && ( item1->Type() != item2->Type() ) )
        if( item1->Type() == DRAW_SHEET_STRUCT_TYPE )
            ii = -1;

    return ii < 0;
}


/** Function ReplaceDuplicatedTimeStamps
 * Search for duplicate time stamps in the whole hierarchy, and replace
 * duplicate by new time stamps
 */
int ReplaceDuplicatedTimeStamps()
{
    /* Build the whole screen list */
    EDA_ScreenList ScreenList;

    /* Build the list of items with time stamps (components and sheets)
     * note: if all items have a different time stamp, this ensure also
     * different paths in complex hierarchy
     * this is the reason we have different time stamps for components AND
     * sheets
     */
    std::vector <SCH_ITEM*> itemlist;
    SCH_SCREEN*             screen;
    SCH_ITEM* item;
    for( screen = ScreenList.GetFirst(); screen != NULL;
         screen = ScreenList.GetNext() )
    {
        item = screen->EEDrawList;
        while( item )
        {
            if( ( item->Type() == DRAW_SHEET_STRUCT_TYPE )
               || ( item->Type() == TYPE_SCH_COMPONENT ) )
                itemlist.push_back( item );

            item = item->Next();
        }
    }

    // Test and replace duplicated time stamps
    int imax     = itemlist.size() - 1;
    int errcount = 0;
    sort( itemlist.begin(), itemlist.end(), SortItemByTimeStamp );
    for( int ii = 0; ii < imax; ii++ )
    {
        item = itemlist[ii];
        SCH_ITEM* nextitem = itemlist[ii + 1];
        if( item->m_TimeStamp == nextitem->m_TimeStamp )
        {
            errcount++;

            // for a component, update its Time stamp and its paths
            // (m_PathsAndReferences field)
            if( item->Type() == TYPE_SCH_COMPONENT )
                ( (SCH_COMPONENT*) item )->SetTimeStamp( GetTimeStamp() );

            // for a sheet, update only its time stamp (annotation of its
            // components will be lost)
            // @todo: see how to change sheet paths for its cmp list (can
            //        be possible in most cases)
            else
                item->m_TimeStamp = GetTimeStamp();
        }
    }

    return errcount;
}
