/**************************************/
/* annotate.cpp: component annotation */
/**************************************/

#include "fctsys.h"
#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "netlist.h"
#include "protos.h"

/* Local Functions*/
static int  ListeComposants( CmpListStruct* BaseListeCmp, DrawSheetList* sheet );
static void BreakReference( CmpListStruct* BaseListeCmp, int NbOfCmp );
static void ReAnnotateComponents( CmpListStruct* BaseListeCmp, int NbOfCmp );
static void ComputeReferenceNumber( CmpListStruct* BaseListeCmp, int NbOfCmp );
int GetLastReferenceNumber( CmpListStruct* Objet,
                            CmpListStruct* BaseListeCmp,
                            int NbOfCmp );
static int  ExistUnit( CmpListStruct* Objet, int Unit,
                       CmpListStruct* BaseListeCmp, int NbOfCmp );


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
    /* Build the screen list (screen, not sheet) */
    EDA_SheetList SheetList( NULL );

    /* Update the screen number, sheet count and date */

    //ScreenList.UpdateScreenNumberAndDate();

    DrawSheetList* sheet;
    int            CmpNumber = 1;

    for( sheet = SheetList.GetFirst();
         sheet != NULL;
         sheet = SheetList.GetNext() )
    {
        EDA_BaseStruct* DrawList = sheet->LastDrawList();
        for( ; DrawList != NULL; DrawList = DrawList->Pnext )
        {
            if( DrawList->Type() != DRAW_LIB_ITEM_STRUCT_TYPE )
                continue;
            EDA_SchComponentStruct* DrawLibItem =
                (EDA_SchComponentStruct*) DrawList;
            EDA_LibComponentStruct* Entry =
                FindLibPart(
                    DrawLibItem->m_ChipName.GetData(), wxEmptyString,
                    FIND_ROOT );
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


CmpListStruct* AllocateCmpListStrct( int numcomponents )
{
    int            ii   = numcomponents * sizeof(CmpListStruct);
    CmpListStruct* list = (CmpListStruct*) MyZMalloc( ii );

    //fill this memory with zeros.
    char*          cptr = (char*) list;

    for( int i = 0; i<ii; i++ )
        *cptr++ = 0;

    return list;
}


/* qsort function to annotate items by their position. */
int AnnotateByPosition( const void* o1, const void* o2 )
{
    CmpListStruct* item1 = (CmpListStruct*) o1;
    CmpListStruct* item2 = (CmpListStruct*) o2;

    int ii = strnicmp( item1->m_TextRef, item2->m_TextRef, 32 );

    if( ii == 0 )
        ii = item1->m_SheetList.Cmp( item2->m_SheetList );
    if( ii == 0 )
        ii = item1->m_Pos.x - item2->m_Pos.x;
    if( ii == 0 )
        ii = item1->m_Pos.y - item2->m_Pos.y;
    if( ii == 0 )
        ii = item1->m_TimeStamp - item2->m_TimeStamp;

    return ii;
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
int AnnotateByValue( const void* o1, const void* o2 )
{
    CmpListStruct* item1 = (CmpListStruct*) o1;
    CmpListStruct* item2 = (CmpListStruct*) o2;

    int ii = strnicmp( item1->m_TextRef, item2->m_TextRef, 32 );

    if( ii == 0 )
        ii = strnicmp( item1->m_TextValue, item2->m_TextValue, 32 );
    if( ii == 0 )
        ii = item1->m_Unit - item2->m_Unit;
    if( ii == 0 )
        ii = item1->m_SheetList.Cmp( item2->m_SheetList );
    if( ii == 0 )
        ii = item1->m_Pos.x - item2->m_Pos.x;
    if( ii == 0 )
        ii = item1->m_Pos.y - item2->m_Pos.y;
    if( ii == 0 )
        ii = item1->m_TimeStamp - item2->m_TimeStamp;

    return ii;
}


/*****************************************************************************
 * DeleteAnnotation:
 *
 * Clear the current annotation.
 ****************************************************************************/
void DeleteAnnotation( WinEDA_SchematicFrame* parent, bool annotateSchematic )
{
    DrawSheetStruct* sheet;

    if( annotateSchematic )
        sheet = g_RootSheet;
    else
        sheet = parent->GetSheet()->Last();

    sheet->DeleteAnnotation( annotateSchematic );

    g_RootSheet->m_s->SetModify();
    parent->DrawPanel->Refresh( true );
}


/*****************************************************************************
 * AnnotateComponents:
 *
 *  Compute the annotation of the components for the whole project, or the
 *  current sheet only.  All the components or the new ones only will be
 *  annotated.
 *****************************************************************************/
void AnnotateComponents( WinEDA_SchematicFrame* parent,
                         bool annotateSchematic,
                         bool sortByPosition,
                         bool resetAnnotation )
{
    int            ii, NbOfCmp;
    DrawSheetList* sheet;
    CmpListStruct* BaseListeCmp;

    wxBusyCursor   dummy;

    /* If it is an annotation for all the components, reset previous
       annotation: */
    if( resetAnnotation )
        DeleteAnnotation( parent, annotateSchematic );

    /* Build the sheet list */
    EDA_SheetList SheetList( g_RootSheet );

    /* Update the sheet number */
    ii = 0;

    /* First pass: Component counting */
    sheet = parent->GetSheet();
    if( annotateSchematic )
    {
        NbOfCmp = 0;
        for( sheet = SheetList.GetFirst();
             sheet != NULL;
             sheet = SheetList.GetNext() )
            NbOfCmp += ListeComposants( NULL, sheet );
    }
    else
        NbOfCmp = ListeComposants( NULL, sheet );

    if( NbOfCmp == 0 )
        return;

    BaseListeCmp = AllocateCmpListStrct( NbOfCmp );

    /* Second pass : Int data tables */
    if( annotateSchematic )
    {
        ii = 0;
        for( sheet = SheetList.GetFirst();
             sheet != NULL;
             sheet = SheetList.GetNext() )
            ii += ListeComposants( BaseListeCmp + ii, sheet );
    }
    else
        ii = ListeComposants( BaseListeCmp, sheet );

    if( ii != NbOfCmp )
        DisplayError( parent, wxT( "Internal error in AnnotateComponents()" ) );

    /* Break full components reference in name (prefix) and number:
       example: IC1 become IC, and 1 */
    BreakReference( BaseListeCmp, NbOfCmp );

    if ( sortByPosition )
        qsort( BaseListeCmp, NbOfCmp, sizeof(CmpListStruct),
               ( int( * ) ( const void*, const void* ) )AnnotateByValue );
    else
        qsort( BaseListeCmp, NbOfCmp, sizeof(CmpListStruct),
               ( int( * ) ( const void*, const void* ) )AnnotateByPosition );

    /* Recalculate reference numbers */
    ComputeReferenceNumber( BaseListeCmp, NbOfCmp );
    ReAnnotateComponents( BaseListeCmp, NbOfCmp );

    MyFree( BaseListeCmp );
    BaseListeCmp = NULL;

    /* Final control */
    CheckAnnotate( parent, !annotateSchematic );
    parent->DrawPanel->Refresh( true );
}


/*****************************************************************************
 * if BaseListeCmp == NULL : count components
 *  else update data table BaseListeCmp
 *****************************************************************************/
int ListeComposants( CmpListStruct* BaseListeCmp, DrawSheetList* sheet )
{
    int                     NbrCmp   = 0;
    EDA_BaseStruct*         DrawList = sheet->LastDrawList();
    EDA_SchComponentStruct* DrawLibItem;
    EDA_LibComponentStruct* Entry;

    for(  ; DrawList;   DrawList = DrawList->Pnext )
    {
        if( DrawList->Type() == DRAW_LIB_ITEM_STRUCT_TYPE )
        {
            DrawLibItem = (EDA_SchComponentStruct*) DrawList;
            Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(),
                                 wxEmptyString,
                                 FIND_ROOT );
            if( Entry == NULL )
                continue;

            if( BaseListeCmp == NULL )      /* Items counting only */
            {
                NbrCmp++;
                continue;
            }

            BaseListeCmp[NbrCmp].m_Cmp         = DrawLibItem;
            BaseListeCmp[NbrCmp].m_NbParts     = Entry->m_UnitCount;
            BaseListeCmp[NbrCmp].m_Unit        = DrawLibItem->m_Multi;
            BaseListeCmp[NbrCmp].m_PartsLocked = Entry->m_UnitSelectionLocked;
            BaseListeCmp[NbrCmp].m_SheetList   = *sheet;
            BaseListeCmp[NbrCmp].m_IsNew       = FALSE;
            BaseListeCmp[NbrCmp].m_Pos = DrawLibItem->m_Pos;
            BaseListeCmp[NbrCmp].m_TimeStamp = DrawLibItem->m_TimeStamp;

            if( DrawLibItem->GetRef( sheet ).IsEmpty() )
                DrawLibItem->SetRef( sheet, wxT( "DefRef?" ) );

            strncpy( BaseListeCmp[NbrCmp].m_TextRef,
                     CONV_TO_UTF8( DrawLibItem->GetRef( sheet ) ), 32 );

            BaseListeCmp[NbrCmp].m_NumRef = -1;

            if( DrawLibItem->m_Field[VALUE].m_Text.IsEmpty() )
                DrawLibItem->m_Field[VALUE].m_Text = wxT( "~" );

            strncpy( BaseListeCmp[NbrCmp].m_TextValue,
                     CONV_TO_UTF8( DrawLibItem->m_Field[VALUE].m_Text ), 32 );
            NbrCmp++;
        }
    }

    return NbrCmp;
}


/*****************************************************************************
 * Update the reference component for the schematic project (or the current
 * sheet)
 *****************************************************************************/
static void ReAnnotateComponents( CmpListStruct* BaseListeCmp, int NbOfCmp )
{
    int   ii;
    char* Text;
    EDA_SchComponentStruct* DrawLibItem;

    /* Reattribution des numeros */
    for( ii = 0; ii < NbOfCmp; ii++ )
    {
        Text = BaseListeCmp[ii].m_TextRef;
        DrawLibItem = BaseListeCmp[ii].m_Cmp;

        if( BaseListeCmp[ii].m_NumRef < 0 )
            strcat( Text, "?" );
        else
            sprintf( Text + strlen( Text ), "%d", BaseListeCmp[ii].m_NumRef );

        DrawLibItem->SetRef( &(BaseListeCmp[ii].m_SheetList),
                             CONV_FROM_UTF8( Text ) );
        DrawLibItem->m_Multi = BaseListeCmp[ii].m_Unit;
    }
}


/*****************************************************************************
 * Split component reference designators into a name (prefix) and number.
 * Example: IC1 becomes IC and 1 in the .m_NumRef member.
 * For multi part per package components not already annotated, set .m_Unit
 * to a max value (0x7FFFFFFF).
 *
 * @param BaseListeCmp = list of component
 * @param NbOfCmp   = item count in the list
 *****************************************************************************/
void BreakReference( CmpListStruct* BaseListeCmp, int NbOfCmp )
{
    int   ii, ll;
    char* Text;

    for( ii = 0; ii < NbOfCmp; ii++ )
    {
        BaseListeCmp[ii].m_NumRef = -1;
        Text = BaseListeCmp[ii].m_TextRef;
        ll   = strlen( Text ) - 1;
        if( Text[ll] == '?' )
        {
            BaseListeCmp[ii].m_IsNew = true;
            if( !BaseListeCmp[ii].m_PartsLocked )
                BaseListeCmp[ii].m_Unit = 0x7FFFFFFF;
            Text[ll] = 0;
            continue;
        }

        if( isdigit( Text[ll] ) == 0 )
        {
            BaseListeCmp[ii].m_IsNew = true;
            if( !BaseListeCmp[ii].m_PartsLocked )
                BaseListeCmp[ii].m_Unit = 0x7FFFFFFF;
            continue;
        }

        while( ll >= 0 )
        {
            if( (Text[ll] <= ' ' ) || isdigit( Text[ll] ) )
                ll--;
            else
            {
                if( isdigit( Text[ll + 1] ) )
                    BaseListeCmp[ii].m_NumRef = atoi( &Text[ll + 1] );
                Text[ll + 1] = 0;
                break;
            }
        }

        wxLogDebug( wxT("BreakReference(): %s number found: %d\n" ),
                    BaseListeCmp[ii].m_TextRef,
                    BaseListeCmp[ii].m_NumRef );
    }
}


/*****************************************************************************
 * Compute the reference number for components without reference number
 *  Compute .m_NumRef member
 *****************************************************************************/
static void ComputeReferenceNumber( CmpListStruct* BaseListeCmp, int NbOfCmp )
{
    int            ii, jj, LastReferenceNumber, NumberOfUnits, Unit;
    const char*    Text, * RefText, * ValText;
    CmpListStruct* ObjRef, * ObjToTest;

    /* Components with an invisible reference (power...) always are
     * re-annotated */
    for( ii = 0; ii < NbOfCmp; ii++ )
    {
        Text = BaseListeCmp[ii].m_TextRef;
        if( *Text == '#' )
        {
            BaseListeCmp[ii].m_IsNew  = true;
            BaseListeCmp[ii].m_NumRef = 0;
        }
    }

    ValText = RefText = ""; LastReferenceNumber = 1;
    for( ii = 0; ii < NbOfCmp; ii++ )
    {
        ObjRef = &BaseListeCmp[ii];
        if( BaseListeCmp[ii].m_Flag )
            continue;

        Text = BaseListeCmp[ii].m_TextRef;
        if( strnicmp( RefText, Text, 32 ) != 0 ) /* Nouveau Identificateur */
        {
            RefText = BaseListeCmp[ii].m_TextRef;
            LastReferenceNumber = GetLastReferenceNumber( BaseListeCmp + ii,
                                                          BaseListeCmp,
                                                          NbOfCmp );
        }

        /* Annotation of one part per package components (trivial case)*/
        if( BaseListeCmp[ii].m_NbParts <= 1 )
        {
            if( BaseListeCmp[ii].m_IsNew )
            {
                LastReferenceNumber++;
                BaseListeCmp[ii].m_NumRef = LastReferenceNumber;
            }
            BaseListeCmp[ii].m_Unit  = 1;
            BaseListeCmp[ii].m_Flag  = 1;
            BaseListeCmp[ii].m_IsNew = FALSE;
            continue;
        }

        /* Annotation of multi-part components ( n parts per package )
           (complex case) */
        ValText = BaseListeCmp[ii].m_TextValue;
        NumberOfUnits = BaseListeCmp[ii].m_NbParts;

        if( BaseListeCmp[ii].m_IsNew )
        {
            LastReferenceNumber++;
            BaseListeCmp[ii].m_NumRef = LastReferenceNumber;

            if( !BaseListeCmp[ii].m_PartsLocked )
                BaseListeCmp[ii].m_Unit = 1;
            BaseListeCmp[ii].m_Flag = 1;
        }

        for( Unit = 1; Unit <= NumberOfUnits; Unit++ )
        {
            if( BaseListeCmp[ii].m_Unit == Unit )
                continue;
            jj = ExistUnit( BaseListeCmp + ii, Unit, BaseListeCmp, NbOfCmp );
            if( jj >= 0 )
                continue; /* Unit exists for this reference */

            /* Search a component to annotate ( same prefix, same value) */
            for( jj = ii + 1; jj < NbOfCmp; jj++ )
            {
                ObjToTest = &BaseListeCmp[jj];
                if( BaseListeCmp[jj].m_Flag )
                    continue;
                Text = BaseListeCmp[jj].m_TextRef;
                if( strnicmp( RefText, Text, 32 ) != 0 )
                    continue; // references are different
                Text = BaseListeCmp[jj].m_TextValue;
                if( strnicmp( ValText, Text, 32 ) != 0 )
                    continue; // values are different
                if( !BaseListeCmp[jj].m_IsNew )
                {
                    continue;
                }
                /* Component without reference number found, annotate it if
                   possible */
                if( !BaseListeCmp[jj].m_PartsLocked ||
                    (BaseListeCmp[jj].m_Unit == Unit) )
                {
                    BaseListeCmp[jj].m_NumRef = BaseListeCmp[ii].m_NumRef;
                    BaseListeCmp[jj].m_Unit   = Unit;
                    BaseListeCmp[jj].m_Flag   = 1;
                    BaseListeCmp[jj].m_IsNew  = FALSE;
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
 * @param Objet = reference item ( Objet->m_TextRef is the search pattern)
 * @param BaseListeCmp = list of items
 * @param NbOfCmp = items count in list of items
 *****************************************************************************/
int GetLastReferenceNumber( CmpListStruct* Objet,
                            CmpListStruct* BaseListeCmp,
                            int NbOfCmp )
{
    CmpListStruct* LastObjet  = BaseListeCmp + NbOfCmp;
    int            LastNumber = 0;
    const char*    RefText;

    RefText = Objet->m_TextRef;
    for( ; Objet < LastObjet; Objet++ )
    {
        /* Nouveau Identificateur */
        if( strnicmp( RefText, Objet->m_TextRef, 32 ) != 0 )
            break;
        if( LastNumber < Objet->m_NumRef )
            LastNumber = Objet->m_NumRef;
    }

    return LastNumber;
}


/*****************************************************************************
 * TODO: Translate this to english/
 * Recherche dans la liste triee des composants, pour les composants
 *  multiples s'il existe pour le composant de reference Objet,
 *  une unite de numero Unit
 *      Retourne index dans BaseListeCmp si oui
 *      retourne -1 si non
 *****************************************************************************/
static int ExistUnit( CmpListStruct* Objet, int Unit,
                      CmpListStruct* BaseListeCmp, int NbOfCmp )
{
    CmpListStruct* EndList = BaseListeCmp + NbOfCmp;
    char*          RefText, * ValText;
    int            NumRef, ii;
    CmpListStruct* ItemToTest;

    RefText = Objet->m_TextRef;
    ValText = Objet->m_TextValue;
    NumRef  = Objet->m_NumRef;
    for( ItemToTest = BaseListeCmp, ii = 0;
         ItemToTest < EndList;
         ItemToTest++, ii++ )
    {
        if( Objet == ItemToTest )
            continue;
        if( ItemToTest->m_IsNew )
            continue; /* non affecte */
        if( ItemToTest->m_NumRef != NumRef )
            continue;
        /* Nouveau Identificateur */
        if( strnicmp( RefText, ItemToTest->m_TextRef, 32 ) != 0 )
            continue;
        if( ItemToTest->m_Unit == Unit )
        {
            return ii;
        }
    }

    return -1;
}


/*****************************************************************************
 *
 * Function CheckAnnotate
 * @return component count ( which are not annotated or have the same
 *  reference (duplicates))
 * @param oneSheetOnly : true = search is made only in the current sheet
 *                       false = search in whole hierarchy (usual search).
 *
 *****************************************************************************/
int CheckAnnotate( WinEDA_SchematicFrame* frame, bool oneSheetOnly )
{
    int            ii, error, NbOfCmp;
    DrawSheetList* sheet;
    CmpListStruct* ListeCmp = NULL;
    wxString       Buff;
    wxString       msg, cmpref;

    /* build the screen list */
    EDA_SheetList  SheetList( NULL );

    g_RootSheet->m_s->SetModify();
    ii = 0;

    /* first pass : count composents */
    if( !oneSheetOnly )
    {
        NbOfCmp = 0;
        for( sheet = SheetList.GetFirst();
             sheet != NULL;
             sheet = SheetList.GetNext() )
            NbOfCmp += ListeComposants( NULL, sheet );
    }
    else
        NbOfCmp = ListeComposants( NULL, frame->GetSheet() );

    if( NbOfCmp == 0 )
    {
        wxBell();
        return 0;
    }


    /* Second pass : create the list of components */
    ListeCmp = AllocateCmpListStrct( NbOfCmp );

    printf( "CheckAnnotate() listing all components:\n" );
    if( !oneSheetOnly )
    {
        ii = 0;
        for( sheet = SheetList.GetFirst();
             sheet != NULL;
             sheet = SheetList.GetNext() )
            ii += ListeComposants( ListeCmp + ii, sheet );
    }
    else
        ListeComposants( ListeCmp, frame->GetSheet() );

    printf( "CheckAnnotate() done:\n" );

    qsort( ListeCmp, NbOfCmp, sizeof(CmpListStruct), AnnotateByValue );

    /* Break full components reference in name (prefix) and number: example:
       IC1 become IC, and 1 */
    BreakReference( ListeCmp, NbOfCmp );

    /* count not yet annotated items */
    error = 0;
    for( ii = 0; ii < NbOfCmp - 1; ii++ )
    {
        msg.Empty();
        Buff.Empty();

        if( ListeCmp[ii].m_IsNew )
        {
            if( ListeCmp[ii].m_NumRef >= 0 )
                Buff << ListeCmp[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = CONV_FROM_UTF8( ListeCmp[ii].m_TextRef );
            msg.Printf( _( "item not annotated: %s%s" ),
                        cmpref.GetData(), Buff.GetData() );

            if( (ListeCmp[ii].m_Unit > 0) && (ListeCmp[ii].m_Unit < 0x7FFFFFFF) )
            {
                Buff.Printf( _( "( unit %d)" ), ListeCmp[ii].m_Unit );
                msg << Buff;
            }
            DisplayError( NULL, msg );
            error++;
            break;
        }

        // Annotate error
        if( MAX( ListeCmp[ii].m_NbParts, 1 ) < ListeCmp[ii].m_Unit  )
        {
            if( ListeCmp[ii].m_NumRef >= 0 )
                Buff << ListeCmp[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = CONV_FROM_UTF8( ListeCmp[ii].m_TextRef );
            msg.Printf( _( "Error item %s%s" ), cmpref.GetData(),
                        Buff.GetData() );

            Buff.Printf( _( " unit %d and no more than %d parts" ),
                         ListeCmp[ii].m_Unit, ListeCmp[ii].m_NbParts );
            msg << Buff;
            DisplayError( frame, msg );
            error++;
            break;
        }
    }

    if( error )
        return error;

    // count the duplicated elements (if all are annotated)
    for( ii = 0; (ii < NbOfCmp - 1) && (error < 4); ii++ )
    {
        msg.Empty();
        Buff.Empty();

        if( (stricmp( ListeCmp[ii].m_TextRef,
                      ListeCmp[ii + 1].m_TextRef ) != 0)||
            ( ListeCmp[ii].m_NumRef != ListeCmp[ii + 1].m_NumRef ) )
            continue;

        /* Same reference found */

        /* If same unit, error ! */
        if( ListeCmp[ii].m_Unit == ListeCmp[ii + 1].m_Unit )
        {
            if( ListeCmp[ii].m_NumRef >= 0 )
                Buff << ListeCmp[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = CONV_FROM_UTF8( ListeCmp[ii].m_TextRef );
            msg.Printf( _( "Multiple item %s%s" ),
                        cmpref.GetData(), Buff.GetData() );

            if( (ListeCmp[ii].m_Unit > 0) && (ListeCmp[ii].m_Unit < 0x7FFFFFFF) )
            {
                Buff.Printf( _( " (unit %d)" ), ListeCmp[ii].m_Unit );
                msg << Buff;
            }
            DisplayError( frame, msg );
            error++;
            continue;
        }

        /* Test error if units are different but number of parts per package
         * too hight (ex U3 ( 1 part) and we find U3B the is an error) */
        if( ListeCmp[ii].m_NbParts != ListeCmp[ii + 1].m_NbParts )
        {
            if( ListeCmp[ii].m_NumRef >= 0 )
                Buff << ListeCmp[ii].m_NumRef;
            else
                Buff = wxT( "?" );

            cmpref = CONV_FROM_UTF8( ListeCmp[ii].m_TextRef );
            msg.Printf( _( "Multiple item %s%s" ),
                        cmpref.GetData(), Buff.GetData() );

            if( (ListeCmp[ii].m_Unit > 0) && (ListeCmp[ii].m_Unit < 0x7FFFFFFF) )
            {
                Buff.Printf( _( " (unit %d)" ), ListeCmp[ii].m_Unit );
                msg << Buff;
            }

            DisplayError( frame, msg );
            error++;
        }

        /* Error if values are different between units, for the same reference */
        if( stricmp( ListeCmp[ii].m_TextValue,
                     ListeCmp[ii + 1].m_TextValue ) != 0 )
        {
            wxString nextcmpref, cmpvalue, nextcmpvalue;
            cmpref       = CONV_FROM_UTF8( ListeCmp[ii].m_TextRef );
            nextcmpref   = CONV_FROM_UTF8( ListeCmp[ii + 1].m_TextRef );
            cmpvalue     = CONV_FROM_UTF8( ListeCmp[ii].m_TextValue );
            nextcmpvalue = CONV_FROM_UTF8( ListeCmp[ii + 1].m_TextValue );
            msg.Printf( _( "Diff values for %s%d%c (%s) and %s%d%c (%s)" ),
                        cmpref.GetData(),
                        ListeCmp[ii].m_NumRef,
                        ListeCmp[ii].m_Unit + 'A' - 1,
                        cmpvalue.GetData(), nextcmpref.GetData(),
                        ListeCmp[ii + 1].m_NumRef,
                        ListeCmp[ii + 1].m_Unit + 'A' - 1,
                        nextcmpvalue.GetData() );

            DisplayError( frame, msg );
            error++;
        }
    }

    MyFree( ListeCmp );
    return error;
}
