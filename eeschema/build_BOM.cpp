// Name:        build_BOM.cpp

// Purpose:
// Author:      jean-pierre Charras
// Licence:   GPL license
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"


#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "dialog_build_BOM.h"


#include "protos.h"

// Filename extension for BOM list
#define EXT_LIST wxT( ".lst" )

// Exported functions
int         BuildComponentsListFromSchematic( ListComponent* aList );

/* fonctions locales */
static int  GenListeGLabels( ListLabel* aList );
static int  ListTriComposantByRef( ListComponent* obj1,
                                   ListComponent* obj2 );
static int  ListTriComposantByVal( ListComponent* obj1,
                                   ListComponent* obj2 );
static int  ListTriGLabelBySheet( ListLabel* obj1, ListLabel* obj2 );
static int  ListTriGLabelByVal( ListLabel* obj1, ListLabel* obj2 );
static void DeleteSubCmp( ListComponent* aList, int aItemCount );

static int  PrintListeGLabel( FILE* f, ListLabel* aList, int aItemCount );

int         RefDesStringCompare( char* obj1, char* obj2 );
int         SplitString( wxString  strToSplit,
                         wxString* strBeginning,
                         wxString* strDigits,
                         wxString* strEnd );

/* Local variables */

/* separator used in bom export to spreadsheet */
static char s_ExportSeparatorSymbol;


/**************************************************************************/
void WinEDA_Build_BOM_Frame::Create_BOM_Lists( bool aTypeFileIsExport,
                                               bool aIncludeSubComponents,
                                               char aExportSeparatorSymbol,
                                               bool aRunBrowser )
/**************************************************************************/
{
    wxString mask, filename;

    s_ExportSeparatorSymbol = aExportSeparatorSymbol;

    m_ListFileName = g_RootSheet->m_AssociatedScreen->m_FileName;
    ChangeFileNameExt( m_ListFileName, EXT_LIST );

    //need to get rid of the path.
    m_ListFileName = m_ListFileName.AfterLast( '/' );
    mask  = wxT( "*" );
    mask += EXT_LIST;

    filename = EDA_FileSelector( _( "Bill of materials:" ),
                                 wxEmptyString,     /* Chemin par defaut (ici dir courante) */
                                 m_ListFileName,    /* nom fichier par defaut, et resultat */
                                 EXT_LIST,          /* extension par defaut */
                                 mask,              /* Masque d'affichage */
                                 this,
                                 wxFD_SAVE,
                                 TRUE
                                 );
    if( filename.IsEmpty() )
        return;
    else
        m_ListFileName = filename;

    /* Close dialog, then show the list (if so requested) */

    if( aTypeFileIsExport )
        CreateExportList( m_ListFileName, aIncludeSubComponents );
    else
        GenereListeOfItems( m_ListFileName, aIncludeSubComponents );

    EndModal( 1 );

    if( aRunBrowser )
    {
        wxString editorname = GetEditorName();
        AddDelimiterString( filename );
        ExecuteFile( this, editorname, filename );
    }
}


/****************************************************************************/
void WinEDA_Build_BOM_Frame::CreateExportList( const wxString& aFullFileName,
                                               bool            aIncludeSubComponents )
/****************************************************************************/

/*
 * Print a list of components, in a form which can be imported by a spreadsheet
 * form is:
 * cmp name; cmp val; fields;
 */
{
    FILE*          f;
    ListComponent* aList;
    int            itemCount;
    wxString       msg;

    /* Creation de la liste des elements */
    if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << aFullFileName;
        DisplayError( this, msg );
        return;
    }

    itemCount = BuildComponentsListFromSchematic( NULL );
    if( itemCount )
    {
        aList = (ListComponent*) MyZMalloc( itemCount * sizeof(ListComponent) );
        if( aList == NULL )
        {
            fclose( f );
            return;
        }

        BuildComponentsListFromSchematic( aList );

        /*  sort component list */
        qsort( aList, itemCount, sizeof( ListComponent ),
               ( int( * ) ( const void*, const void* ) )ListTriComposantByRef );

        if( !aIncludeSubComponents )
            DeleteSubCmp( aList, itemCount );

        /* create the file */
        PrintComponentsListByRef( f, aList, itemCount, TRUE, aIncludeSubComponents );

        MyFree( aList );
    }

    fclose( f );
}


/****************************************************************************/
void WinEDA_Build_BOM_Frame::GenereListeOfItems( const wxString& aFullFileName,
                                                 bool            aIncludeSubComponents )
/****************************************************************************/

/*
 * Routine principale pour la creation des listings ( composants et/ou labels
 * globaux et "sheet labels" )
 */
{
    FILE*          f;
    ListComponent* list;
    ListLabel*     listOfLabels;
    int            itemCount;
    char           Line[1024];
    wxString       msg;

    /* Creation de la liste des elements */
    if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << aFullFileName;
        DisplayError( this, msg );
        return;
    }

    itemCount = BuildComponentsListFromSchematic( NULL );
    if( itemCount )
    {
        list = (ListComponent*) MyZMalloc( itemCount * sizeof(ListComponent) );
        if( list == NULL )  // Error memory alloc
        {
            fclose( f );
            return;
        }

        BuildComponentsListFromSchematic( list );

        /* generation du fichier listing */
        DateAndTime( Line );
        wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
        fprintf( f, "%s  >> Creation date: %s\n", CONV_TO_UTF8( Title ), Line );

        /* Tri et impression de la liste des composants */

        qsort( list, itemCount, sizeof( ListComponent ),
               ( int( * ) ( const void*, const void* ) )ListTriComposantByRef );

        if( !aIncludeSubComponents )
            DeleteSubCmp( list, itemCount );

//		if( s_ListByRef )
        if( m_ListCmpbyRefItems->GetValue() )
        {
            PrintComponentsListByRef( f, list, itemCount, false, aIncludeSubComponents );
        }

//		if( s_ListByValue )
        if( m_ListCmpbyValItems->GetValue() )
        {
            qsort( list, itemCount, sizeof( ListComponent ),
                   ( int( * ) ( const void*, const void* ) )ListTriComposantByVal );
            PrintComponentsListByVal( f, list, itemCount, aIncludeSubComponents );
        }
        MyFree( list );
    }

    /***************************************/
    /* Generation liste des Labels globaux */
    /***************************************/
    itemCount = GenListeGLabels( NULL );
    if( itemCount )
    {
        listOfLabels = (ListLabel*) MyZMalloc( itemCount * sizeof(ListLabel) );
        if( listOfLabels == NULL )
        {
            fclose( f );
            return;
        }

        GenListeGLabels( listOfLabels );

        /* Tri de la liste */

//		if( s_ListBySheet )
        if( m_GenListLabelsbySheet->GetValue() )
        {
            qsort( listOfLabels, itemCount, sizeof( ListLabel ),
                   ( int( * ) ( const void*, const void* ) )ListTriGLabelBySheet );

            msg.Printf( _(
                            "\n#Global, Hierarchical Labels and PinSheets ( order = Sheet Number ) count = %d\n" ),
                        itemCount );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels, itemCount );
        }

//		if( s_ListHierarchicalPinByName )
        if( m_GenListLabelsbyVal->GetValue() )
        {
            qsort( listOfLabels, itemCount, sizeof( ListLabel ),
                   ( int( * ) ( const void*, const void* ) )ListTriGLabelByVal );

            msg.Printf( _(
                            "\n#Global, Hierarchical Labels and PinSheets ( order = Alphab. ) count = %d\n\n" ),
                        itemCount );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, listOfLabels, itemCount );
        }
        MyFree( listOfLabels );
    }

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", CONV_TO_UTF8( msg ) );
    fclose( f );
}


/*********************************************************/
int BuildComponentsListFromSchematic( ListComponent* aList )
/*********************************************************/

/* Creates the list of components found in the whole schematic
 *
 * if List == null, just returns the count. if not, fills the list.
 * goes through the sheets, not the screens, so that we account for
 * multiple instances of a given screen.
 * Also Initialise m_Father as pointer pointeur of the SCH_SCREN parent
 */
{
    int             itemCount = 0;
    EDA_BaseStruct* SchItem;
    SCH_COMPONENT*  DrawLibItem;
    DrawSheetPath*  sheet;

    /* Build the sheet (not screen) list */
    EDA_SheetList   SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( SchItem = sheet->LastDrawList(); SchItem; SchItem = SchItem->Next() )
        {
            if( SchItem->Type() != TYPE_SCH_COMPONENT )
                continue;

            itemCount++;
            DrawLibItem = (SCH_COMPONENT*) SchItem;
            DrawLibItem->SetParent( sheet->LastScreen() );
            if( aList )
            {
                aList->m_Comp      = DrawLibItem;
                aList->m_SheetList = *sheet;
                aList->m_Unit = DrawLibItem->GetUnitSelection( sheet );

                strncpy( aList->m_Ref,
                        CONV_TO_UTF8( DrawLibItem->GetRef( sheet ) ),
                        sizeof( aList->m_Ref ) );

                // Ensure always nul terminate m_Ref.
                aList->m_Ref[sizeof( aList->m_Ref ) - 1 ] = 0;
                aList++;
            }
        }
    }

    return itemCount;
}


/*********************************************/
static int GenListeGLabels( ListLabel* list )
/*********************************************/

/* Count the Glabels, or fill the list Listwith Glabel pointers
 * If List == NULL: Item count only
 * Else fill list of Glabels
 */
{
    int             itemCount = 0;
    EDA_BaseStruct* DrawList;
    Hierarchical_PIN_Sheet_Struct* SheetLabel;
    DrawSheetPath*  sheet;

    /* Build the screen list */
    EDA_SheetList   SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        DrawList = sheet->LastDrawList();
        wxString path = sheet->PathHumanReadable();
        while( DrawList )
        {
            switch( DrawList->Type() )
            {
            case TYPE_SCH_HIERLABEL:
            case TYPE_SCH_GLOBALLABEL:
                itemCount++;
                if( list )
                {
                    list->m_LabelType = DrawList->Type();
                    snprintf( list->m_SheetPath, sizeof(list->m_SheetPath),
                             "%s", CONV_TO_UTF8( path ) );
                    list->m_Label = DrawList;
                    list++;
                }
                break;

            case DRAW_SHEET_STRUCT_TYPE:
                {
                    #define Sheet ( (DrawSheetStruct*) DrawList )
                    SheetLabel = Sheet->m_Label;
                    while( SheetLabel != NULL )
                    {
                        if( list )
                        {
                            list->m_LabelType = DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE;
                            snprintf( list->m_SheetPath, sizeof(list->m_SheetPath),
                                     "%s", CONV_TO_UTF8( path ) );
                            list->m_Label = SheetLabel;
                            list++;
                        }
                        itemCount++;
                        SheetLabel = SheetLabel->Next();
                    }
                }
                break;

            default:
                break;
            }
            DrawList = DrawList->Next();
        }
    }

    return itemCount;
}


/**********************************************************/
static int ListTriComposantByVal( ListComponent* obj1,
                                  ListComponent* obj2 )
/**********************************************************/

/* Routine de comparaison pour le tri du Tableau par qsort()
 * Les composants sont tries
 *     par valeur
 *     si meme valeur: par reference
 *         si meme valeur: par numero d'unite
 */
{
    int ii;
    const wxString* Text1, * Text2;

    if( ( obj1 == NULL ) && ( obj2 == NULL ) )
        return 0;
    if( obj1 == NULL )
        return -1;
    if( obj2 == NULL )
        return 1;
    if( ( obj1->m_Comp == NULL ) && ( obj2->m_Comp == NULL ) )
        return 0;
    if( obj1->m_Comp == NULL )
        return -1;
    if( obj2->m_Comp == NULL )
        return 1;

    Text1 = &(obj1->m_Comp->GetField(VALUE)->m_Text);
    Text2 = &(obj2->m_Comp->GetField(VALUE)->m_Text);
    ii    = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = RefDesStringCompare( obj1->m_Ref, obj2->m_Ref );
    }

    if( ii == 0 )
    {
        ii = obj1->m_Unit - obj2->m_Unit;
    }

    return ii;
}


/**********************************************************/
static int ListTriComposantByRef( ListComponent* obj1,
                                  ListComponent* obj2 )
/**********************************************************/

/* Routine de comparaison pour le tri du Tableau par qsort()
 * Les composants sont tries
 *     par reference
 *     si meme referenece: par valeur
 *         si meme valeur: par numero d'unite
 */
{
    int ii;
    const wxString* Text1, * Text2;

    if( ( obj1 == NULL ) && ( obj2 == NULL ) )
        return 0;
    if( obj1 == NULL )
        return -1;
    if( obj2 == NULL )
        return 1;

    if( ( obj1->m_Comp == NULL ) && ( obj2->m_Comp == NULL ) )
        return 0;
    if( obj1->m_Comp == NULL )
        return -1;
    if( obj2->m_Comp == NULL )
        return 1;

    ii = RefDesStringCompare( obj1->m_Ref, obj2->m_Ref );

    if( ii == 0 )
    {
        Text1 = &( obj1->m_Comp->GetField(VALUE)->m_Text );
        Text2 = &( obj2->m_Comp->GetField(VALUE)->m_Text );
        ii    = Text1->CmpNoCase( *Text2 );
    }

    if( ii == 0 )
    {
        ii = obj1->m_Unit - obj2->m_Unit;
    }

    return ii;
}


/******************************************************************/
static int ListTriGLabelByVal( ListLabel* obj1, ListLabel* obj2 )
/*******************************************************************/

/* Routine de comparaison pour le tri du Tableau par qsort()
 * Les labels sont tries
 *     par comparaison ascii
 *     si meme valeur: par numero de sheet
 */
{
    int ii;
    const wxString* Text1, * Text2;

    if( obj1->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
        Text1 = &( (Hierarchical_PIN_Sheet_Struct*) obj1->m_Label )->m_Text;
    else
        Text1 = &( (SCH_TEXT*) obj1->m_Label )->m_Text;

    if( obj2->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
        Text2 = &( (Hierarchical_PIN_Sheet_Struct*) obj2->m_Label )->m_Text;
    else
        Text2 = &( (SCH_TEXT*) obj2->m_Label )->m_Text;

    ii = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = strcmp( obj1->m_SheetPath, obj2->m_SheetPath );
    }

    return ii;
}


/*******************************************************************/
static int ListTriGLabelBySheet( ListLabel* obj1, ListLabel* obj2 )
/*******************************************************************/

/* Routine de comparaison pour le tri du Tableau par qsort()
 * Les labels sont tries
 *     par sheet number
 *     si meme valeur, par ordre alphabetique
 */
{
    int ii;
    const wxString* Text1, * Text2;


    ii = strcmp( obj1->m_SheetPath, obj2->m_SheetPath );

    if( ii == 0 )
    {
        if( obj1->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
            Text1 = &( (Hierarchical_PIN_Sheet_Struct*) obj1->m_Label )->m_Text;
        else
            Text1 = &( (SCH_TEXT*) obj1->m_Label )->m_Text;

        if( obj2->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
            Text2 = &( (Hierarchical_PIN_Sheet_Struct*) obj2->m_Label )->m_Text;
        else
            Text2 = &( (SCH_TEXT*) obj2->m_Label )->m_Text;

        ii = Text1->CmpNoCase( *Text2 );
    }

    return ii;
}


/**************************************************************/
static void DeleteSubCmp( ListComponent* aList, int aItemCount )
/**************************************************************/

/* Remove sub components from the list, when multiples parts per package are found in this list
 * The component list **MUST** be sorted by reference and by unit number
 */
{
    SCH_COMPONENT* libItem;
    wxString oldName;
    wxString currName;


    for( int ii = 0; ii < aItemCount; ii++ )
    {
        libItem = aList[ii].m_Comp;
        if( libItem == NULL )
            continue;

        currName = CONV_FROM_UTF8( aList[ii].m_Ref );

        if( !oldName.IsEmpty() )
        {
            if( oldName == currName )   // currName is a subpart of oldName: remove it
            {
                aList[ii].m_Comp = NULL;
                aList[ii].m_SheetList.Clear();
                aList[ii].m_Ref[0] = 0;
            }
        }
        oldName = currName;
    }
}


/*******************************************************************************************/
void WinEDA_Build_BOM_Frame::PrintFieldData( FILE* f, SCH_COMPONENT* DrawLibItem,
                                             bool CompactForm )
/*******************************************************************************************/
{
    // @todo make this variable length
    static const wxCheckBox* FieldListCtrl[] = {
        m_AddField1,
        m_AddField2,
        m_AddField3,
        m_AddField4,
        m_AddField5,
        m_AddField6,
        m_AddField7,
        m_AddField8
    };

    int ii;
    const wxCheckBox* FieldCtrl = FieldListCtrl[0];

    if( m_AddFootprintField->IsChecked() )
    {
        if( CompactForm )
        {
            fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField(FOOTPRINT)->m_Text ) );
        }
        else
            fprintf( f, "; %-12s", CONV_TO_UTF8( DrawLibItem->GetField(FOOTPRINT)->m_Text ) );
    }

    for( ii = FIELD1; ii < DrawLibItem->GetFieldCount(); ii++ )
    {
        FieldCtrl = FieldListCtrl[ii - FIELD1];
        if( FieldCtrl == NULL )
            continue;

        if( !FieldCtrl->IsChecked() )
            continue;

        if( CompactForm )
            fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField(ii)->m_Text ) );
        else
            fprintf( f, "; %-12s", CONV_TO_UTF8( DrawLibItem->GetField(ii)->m_Text ) );
    }
}


/*********************************************************************************************/
int WinEDA_Build_BOM_Frame::PrintComponentsListByRef( FILE* f,
                                                      ListComponent* aList,
                                                      int aItemCount,
                                                      bool CompactForm,
                                                      bool aIncludeSubComponents )
/*********************************************************************************************/

/* Print the B.O.M sorted by reference
 */
{
    int ii, Multi, Unit;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* DrawLibItem;
    EDA_LibComponentStruct* Entry;
    char CmpName[80];
    wxString msg;

    if( CompactForm )
    {
        // @todo make this variable length
        static const wxCheckBox* FieldListCtrl[FIELD8 - FIELD1 + 1] = {
            m_AddField1,
            m_AddField2,
            m_AddField3,
            m_AddField4,
            m_AddField5,
            m_AddField6,
            m_AddField7,
            m_AddField8
        };

        // Print comment line:
        fprintf( f, "ref%cvalue", s_ExportSeparatorSymbol );

        if( aIncludeSubComponents )
        {
            fprintf( f, "%csheet path", s_ExportSeparatorSymbol );
            fprintf( f, "%clocation", s_ExportSeparatorSymbol );
        }

        if( m_AddFootprintField->IsChecked() )
            fprintf( f, "%cfootprint", s_ExportSeparatorSymbol );

        for( ii = FIELD1; ii <= FIELD8; ii++ )
        {
            const wxCheckBox* FieldCtrl = FieldListCtrl[ii - FIELD1];
            if( FieldCtrl == NULL )
                continue;

            if( !FieldCtrl->IsChecked() )
                continue;

            msg = _( "Field" );
            fprintf( f, "%c%s%d", s_ExportSeparatorSymbol, CONV_TO_UTF8( msg ), ii - FIELD1 + 1 );
        }

        fprintf( f, "\n" );
    }
    else
    {
        msg = _( "\n#Cmp ( order = Reference )" );

        if( aIncludeSubComponents )
            msg << _( " (with SubCmp)" );
        fprintf( f, "%s\n", CONV_TO_UTF8( msg ) );
    }

    // Print list of items
    for( ii = 0; ii < aItemCount; ii++ )
    {
        DrawList = aList[ii].m_Comp;

        if( DrawList == NULL )
            continue;
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) DrawList;
        if( aList[ii].m_Ref[0] == '#' )
            continue;

        Multi = 0;
        Unit  = ' ';
        Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry )
            Multi = Entry->m_UnitCount;

        if( ( Multi > 1 ) && aIncludeSubComponents )
#if defined(KICAD_GOST)
            Unit = aList[ii].m_Unit + '1' - 1;
#else
            Unit = aList[ii].m_Unit + 'A' - 1;
#endif

        sprintf( CmpName, "%s", aList[ii].m_Ref );
        if( !CompactForm || Unit != ' ' )
            sprintf( CmpName + strlen( CmpName ), "%c", Unit );

        if( CompactForm )
            fprintf( f, "%s%c%s", CmpName, s_ExportSeparatorSymbol,
                    CONV_TO_UTF8( DrawLibItem->GetField(VALUE)->m_Text ) );
        else
            fprintf( f, "| %-10s %-12s", CmpName,
                    CONV_TO_UTF8( DrawLibItem->GetField(VALUE)->m_Text ) );

        if( aIncludeSubComponents )
        {
            msg = aList[ii].m_SheetList.PathHumanReadable();
            if( CompactForm )
            {
                fprintf( f, "%c%s", s_ExportSeparatorSymbol, CONV_TO_UTF8( msg ) );
                msg = m_Parent->GetXYSheetReferences( (BASE_SCREEN*)DrawLibItem->GetParent(), DrawLibItem->m_Pos );
                fprintf( f, "%c%s)", s_ExportSeparatorSymbol, CONV_TO_UTF8( msg ) );
            }
            else
            {
                fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
                msg = m_Parent->GetXYSheetReferences( (BASE_SCREEN*)DrawLibItem->GetParent(), DrawLibItem->m_Pos );
                fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
            }
        }

        PrintFieldData( f, DrawLibItem, CompactForm );

        fprintf( f, "\n" );
    }

    if( !CompactForm )
    {
        msg = _( "#End Cmp\n" );
        fprintf( f, CONV_TO_UTF8( msg ) );
    }
    return 0;
}


/*********************************************************************************************/
int WinEDA_Build_BOM_Frame::PrintComponentsListByVal( FILE* f,
                                                      ListComponent* aList,
                                                      int aItemCount,
                                                      bool aIncludeSubComponents )
/**********************************************************************************************/
{
    int Multi;
    wxChar Unit;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* DrawLibItem;
    EDA_LibComponentStruct* Entry;
    char CmpName[80];
    wxString msg;

    msg = _( "\n#Cmp ( order = Value )" );

    if( aIncludeSubComponents )
        msg << _( " (with SubCmp)" );
    msg << wxT( "\n" );
    fprintf( f, CONV_TO_UTF8( msg ) );

    for( int ii = 0; ii < aItemCount; ii++ )
    {
        DrawList = aList[ii].m_Comp;

        if( DrawList == NULL )
            continue;

        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) DrawList;
        if( aList[ii].m_Ref[0] == '#' )
            continue;

        Multi = 0;
        Unit  = ' ';
        Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry )
            Multi = Entry->m_UnitCount;

        if( ( Multi > 1 ) && aIncludeSubComponents )
        {
#if defined(KICAD_GOST)
            Unit = aList[ii].m_Unit + '1' - 1;
        }

        sprintf( CmpName, "%s.%c", aList[ii].m_Ref, Unit );
#else
            Unit = aList[ii].m_Unit + 'A' - 1;
        }

        sprintf( CmpName, "%s%c", aList[ii].m_Ref, Unit );
#endif
        fprintf( f, "| %-12s %-10s", CONV_TO_UTF8( DrawLibItem->GetField(VALUE)->m_Text ), CmpName );

        // print the sheet path
        if( aIncludeSubComponents )
        {
            msg = aList[ii].m_SheetList.PathHumanReadable();
            fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
            msg = m_Parent->GetXYSheetReferences( (BASE_SCREEN*)DrawLibItem->GetParent(), DrawLibItem->m_Pos );
            fprintf( f, "   (loc %s)", CONV_TO_UTF8( msg ) );
        }

        PrintFieldData( f, DrawLibItem );

        fprintf( f, "\n" );
    }

    msg = _( "#End Cmp\n" );
    fprintf( f, CONV_TO_UTF8( msg ) );
    return 0;
}


/******************************************************************/
static int PrintListeGLabel( FILE* f, ListLabel* aList, int aItemCount )
/******************************************************************/
{
    int ii, jj;
    SCH_LABEL* DrawTextItem;
    Hierarchical_PIN_Sheet_Struct* DrawSheetLabel;
    ListLabel* LabelItem;
    wxString msg, sheetpath;
    wxString labeltype;

    for( ii = 0; ii < aItemCount; ii++ )
    {
        LabelItem = &aList[ii];

        switch( LabelItem->m_LabelType )
        {
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_GLOBALLABEL:
            DrawTextItem = (SCH_LABEL*) (LabelItem->m_Label);

            if( LabelItem->m_LabelType == TYPE_SCH_HIERLABEL )
                labeltype = wxT( "Hierarchical" );
            else
                labeltype = wxT( "Global      " );

            sheetpath = CONV_FROM_UTF8( LabelItem->m_SheetPath );
            msg.Printf(
                _( "> %-28.28s %s        (Sheet %s) pos: %3.3f, %3.3f\n" ),
                DrawTextItem->m_Text.GetData(),
                labeltype.GetData(),
                sheetpath.GetData(),
                (float) DrawTextItem->m_Pos.x / 1000,
                (float) DrawTextItem->m_Pos.y / 1000 );

            fprintf( f, CONV_TO_UTF8( msg ) );
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        {
            DrawSheetLabel = (Hierarchical_PIN_Sheet_Struct*) LabelItem->m_Label;
            jj = DrawSheetLabel->m_Shape;
            if( jj < 0 )
                jj = NET_TMAX;
            if( jj > NET_TMAX )
                jj = 4;
            wxString labtype = CONV_FROM_UTF8( SheetLabelType[jj] );
            msg.Printf(
                _( "> %-28.28s PinSheet %-7.7s (Sheet %s) pos: %3.3f, %3.3f\n" ),
                DrawSheetLabel->m_Text.GetData(),
                labtype.GetData(),
                LabelItem->m_SheetPath,
                (float) DrawSheetLabel->m_Pos.x / 1000,
                (float) DrawSheetLabel->m_Pos.y / 1000 );
            fprintf( f, CONV_TO_UTF8( msg ) );
        }
            break;

        default:
            break;
        }
    }

    msg = _( "#End labels\n" );
    fprintf( f, CONV_TO_UTF8( msg ) );
    return 0;
}


/********************************************/
int RefDesStringCompare( char* obj1, char* obj2 )
/********************************************/

/* This function will act just like the strcmp function but correctly sort
 * the numerical order in the string
 * return -1 if first string is less than the second
 * return 0 if the strings are equal
 * return 1 if the first string is greater than the second
 */
{
    /* The strings we are going to compare */
    wxString strFWord;
    wxString strSWord;

    /* The different sections of the first string */
    wxString strFWordBeg, strFWordMid, strFWordEnd;

    /* The different sections of the second string */
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    int isEqual = 0;            /* The numerical results of a string compare */
    int iReturn = 0;            /* The variable that is being returned */

    long lFirstDigit  = 0;      /* The converted middle section of the first string */
    long lSecondDigit = 0;      /* The converted middle section of the second string */

    /* Since m_Ref is a char * it is ASCII */
    strFWord = wxString::FromAscii( obj1 );
    strSWord = wxString::FromAscii( obj2 );

    /* Split the two string into seperate parts */
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    /* Compare the Beginning section of the strings */
    isEqual = strFWordBeg.CmpNoCase( strSWordBeg );
    if( isEqual > 0 )
        iReturn = 1;
    else if( isEqual < 0 )
        iReturn = -1;
    else
    {
        /* If the first sections are equal compare there digits */
        strFWordMid.ToLong( &lFirstDigit );
        strSWordMid.ToLong( &lSecondDigit );
        if( lFirstDigit > lSecondDigit )
            iReturn = 1;
        else if( lFirstDigit < lSecondDigit )
            iReturn = -1;
        else
        {
            /* If the first two sections are equal compare the endings */
            isEqual = strFWordEnd.CmpNoCase( strSWordEnd );
            if( isEqual > 0 )
                iReturn = 1;
            else if( isEqual < 0 )
                iReturn = -1;
            else
                iReturn = 0;
        }
    }

    return iReturn;
}


/**************************************************************************************************/
int SplitString( wxString  strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd )
/**************************************************************************************************/

/* This is the function that breaks a string into three parts.
 * The alphabetic preamble
 * The numeric part
 * Any alphabetic ending
 * For example C10A is split to C 10 A
 */
{
    /* Clear all the return strings */
    strBeginning->Clear();
    strDigits->Clear();
    strEnd->Clear();

    /* There no need to do anything if the string is empty */
    if( strToSplit.length() == 0 )
        return 0;

    /* Starting at the end of the string look for the first digit */
    int ii;
    for( ii = (strToSplit.length() - 1); ii >= 0; ii-- )
    {
        if( isdigit( strToSplit[ii] ) )
            break;
    }

    /* If there were no digits then just set the single string */
    if( ii < 0 )
        *strBeginning = strToSplit;
    else
    {
        /* Since there is at least one digit this is the trailing string */
        *strEnd = strToSplit.substr( ii + 1 );

        /* Go to the end of the digits */
        int position = ii + 1;
        for( ; ii >= 0; ii-- )
        {
            if( !isdigit( strToSplit[ii] ) )
                break;
        }

        /* If all that was left was digits, then just set the digits string */
        if( ii < 0 )
            *strDigits = strToSplit.substr( 0, position );

        /* We were only looking for the last set of digits everything else is part of the preamble */
        else
        {
            *strDigits    = strToSplit.substr( ii + 1, position - ii - 1 );
            *strBeginning = strToSplit.substr( 0, ii + 1 );
        }
    }

    return 0;
}
