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
int         BuildComponentsListFromSchematic( ListComponent* List );

/* fonctions locales */
static int  GenListeGLabels( ListLabel* List );
static int  ListTriComposantByRef( ListComponent* Objet1,
                                   ListComponent* Objet2 );
static int  ListTriComposantByVal( ListComponent* Objet1,
                                   ListComponent* Objet2 );
static int  ListTriGLabelBySheet( ListLabel* Objet1, ListLabel* Objet2 );
static int  ListTriGLabelByVal( ListLabel* Objet1, ListLabel* Objet2 );
static void DeleteSubCmp( ListComponent* List, int NbItems );

static int  PrintListeGLabel( FILE* f, ListLabel* List, int NbItems );

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
        wxEmptyString,                              /* Chemin par defaut (ici dir courante) */
        m_ListFileName,                             /* nom fichier par defaut, et resultat */
        EXT_LIST,                                   /* extension par defaut */
        mask,                                       /* Masque d'affichage */
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
void WinEDA_Build_BOM_Frame::CreateExportList( const wxString& FullFileName,
                                               bool            aIncludeSubComponents )
/****************************************************************************/

/*
 * Print a list of components, in a form which can be imported by a spreadsheet
 * form is:
 * cmp name; cmp val; fields;
 */
{
    FILE*          f;
    ListComponent* List;
    int            NbItems;
    wxString       msg;

    /* Creation de la liste des elements */
    if( ( f = wxFopen( FullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << FullFileName;
        DisplayError( this, msg );
        return;
    }

    NbItems = BuildComponentsListFromSchematic( NULL );
    if( NbItems )
    {
        List = (ListComponent*) MyZMalloc( NbItems * sizeof(ListComponent) );
        if( List == NULL )
        {
            fclose( f );
            return;
        }

        BuildComponentsListFromSchematic( List );

        /*  sort component list */
        qsort( List, NbItems, sizeof( ListComponent ),
            ( int( * ) ( const void*, const void* ) )ListTriComposantByRef );

        if( !aIncludeSubComponents )
            DeleteSubCmp( List, NbItems );

        /* create the file */
        PrintComponentsListByRef( f, List, NbItems, TRUE, aIncludeSubComponents );

        MyFree( List );
    }

    fclose( f );
}


/****************************************************************************/
void WinEDA_Build_BOM_Frame::GenereListeOfItems( const wxString& FullFileName,
                                                 bool            aIncludeSubComponents )
/****************************************************************************/

/*
 * Routine principale pour la creation des listings ( composants et/ou labels
 * globaux et "sheet labels" )
 */
{
    FILE*          f;
    ListComponent* List;
    ListLabel*     ListOfLabels;
    int            NbItems;
    char           Line[1024];
    wxString       msg;

    /* Creation de la liste des elements */
    if( ( f = wxFopen( FullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to open file " );
        msg << FullFileName;
        DisplayError( this, msg );
        return;
    }

    NbItems = BuildComponentsListFromSchematic( NULL );
    if( NbItems )
    {
        List = (ListComponent*) MyZMalloc( NbItems * sizeof(ListComponent) );
        if( List == NULL )  // Error memory alloc
        {
            fclose( f );
            return;
        }

        BuildComponentsListFromSchematic( List );

        /* generation du fichier listing */
        DateAndTime( Line );
        wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
        fprintf( f, "%s  >> Creation date: %s\n", CONV_TO_UTF8( Title ), Line );

        /* Tri et impression de la liste des composants */

        qsort( List, NbItems, sizeof( ListComponent ),
            ( int( * ) ( const void*, const void* ) )ListTriComposantByRef );

        if( !aIncludeSubComponents )
            DeleteSubCmp( List, NbItems );

//		if( s_ListByRef )
        if( m_ListCmpbyRefItems->GetValue() )
        {
            PrintComponentsListByRef( f, List, NbItems, false, aIncludeSubComponents );
        }

//		if( s_ListByValue )
        if( m_ListCmpbyValItems->GetValue() )
        {
            qsort( List, NbItems, sizeof( ListComponent ),
                ( int( * ) ( const void*, const void* ) )ListTriComposantByVal );
            PrintComponentsListByVal( f, List, NbItems, aIncludeSubComponents );
        }
        MyFree( List );
    }

    /***************************************/
    /* Generation liste des Labels globaux */
    /***************************************/
    NbItems = GenListeGLabels( NULL );
    if( NbItems )
    {
        ListOfLabels = (ListLabel*) MyZMalloc( NbItems * sizeof(ListLabel) );
        if( ListOfLabels == NULL )
        {
            fclose( f );
            return;
        }

        GenListeGLabels( ListOfLabels );

        /* Tri de la liste */

//		if( s_ListBySheet )
        if( m_GenListLabelsbySheet->GetValue() )
        {
            qsort( ListOfLabels, NbItems, sizeof( ListLabel ),
                ( int( * ) ( const void*, const void* ) )ListTriGLabelBySheet );

            msg.Printf( _(
                    "\n#Global, Hierarchical Labels and PinSheets ( order = Sheet Number ) count = %d\n" ),
                NbItems );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, ListOfLabels, NbItems );
        }

//		if( s_ListHierarchicalPinByName )
        if( m_GenListLabelsbyVal->GetValue() )
        {
            qsort( ListOfLabels, NbItems, sizeof( ListLabel ),
                ( int( * ) ( const void*, const void* ) )ListTriGLabelByVal );

            msg.Printf( _(
                    "\n#Global, Hierarchical Labels and PinSheets ( order = Alphab. ) count = %d\n\n" ),
                NbItems );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );
            PrintListeGLabel( f, ListOfLabels, NbItems );
        }
        MyFree( ListOfLabels );
    }

    msg = _( "\n#End List\n" );
    fprintf( f, "%s", CONV_TO_UTF8( msg ) );
    fclose( f );
}


/*********************************************************/
int BuildComponentsListFromSchematic( ListComponent* List )
/*********************************************************/

/* Creates the list of components found in the whole schematic
 *
 * if List == null, just returns the count. if not, fills the list.
 * goes through the sheets, not the screens, so that we account for
 * multiple instances of a given screen.
 * Also Initialise m_Father as pointer pointeur of the SCH_SCREN parent
 */
{
    int             ItemCount = 0;
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

            ItemCount++;
            DrawLibItem = (SCH_COMPONENT*) SchItem;
            DrawLibItem->m_Parent = sheet->LastScreen();
            if( List )
            {
                List->m_Comp      = DrawLibItem;
                List->m_SheetList = *sheet;
                List->m_Unit = DrawLibItem->GetUnitSelection( sheet );
                strncpy( List->m_Ref,
                    CONV_TO_UTF8( DrawLibItem->GetRef( sheet ) ),
                    sizeof( List->m_Ref ) );
                List++;
            }
        }
    }

    return ItemCount;
}


/*********************************************/
static int GenListeGLabels( ListLabel* List )
/*********************************************/

/* Count the Glabels, or fill the list Listwith Glabel pointers
 * If List == NULL: Item count only
 * Else fill list of Glabels
 */
{
    int             ItemCount = 0;
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
                ItemCount++;
                if( List )
                {
                    List->m_LabelType = DrawList->Type();
                    snprintf( List->m_SheetPath, sizeof(List->m_SheetPath),
                        "%s", CONV_TO_UTF8( path ) );
                    List->m_Label = DrawList;
                    List++;
                }
                break;

            case DRAW_SHEET_STRUCT_TYPE:
            {
                #define Sheet ( (DrawSheetStruct*) DrawList )
                SheetLabel = Sheet->m_Label;
                while( SheetLabel != NULL )
                {
                    if( List )
                    {
                        List->m_LabelType = DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE;
                        snprintf( List->m_SheetPath, sizeof(List->m_SheetPath),
                            "%s", CONV_TO_UTF8( path ) );
                        List->m_Label = SheetLabel;
                        List++;
                    }
                    ItemCount++;
                    SheetLabel = (Hierarchical_PIN_Sheet_Struct*) (SheetLabel->Pnext);
                }
            }
                break;

            default:
                break;
            }
            DrawList = DrawList->Pnext;
        }
    }

    return ItemCount;
}


/**********************************************************/
static int ListTriComposantByVal( ListComponent* Objet1,
                                  ListComponent* Objet2 )
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

    if( ( Objet1 == NULL ) && ( Objet2 == NULL ) )
        return 0;
    if( Objet1 == NULL )
        return -1;
    if( Objet2 == NULL )
        return 1;
    if( ( Objet1->m_Comp == NULL ) && ( Objet2->m_Comp == NULL ) )
        return 0;
    if( Objet1->m_Comp == NULL )
        return -1;
    if( Objet2->m_Comp == NULL )
        return 1;

    Text1 = &(Objet1->m_Comp->m_Field[VALUE].m_Text);
    Text2 = &(Objet2->m_Comp->m_Field[VALUE].m_Text);
    ii    = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = strcmp( Objet1->m_Ref, Objet2->m_Ref );
    }

    if( ii == 0 )
    {
        ii = Objet1->m_Unit - Objet2->m_Unit;
    }

    return ii;
}


/**********************************************************/
static int ListTriComposantByRef( ListComponent* Objet1,
                                  ListComponent* Objet2 )
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

    if( ( Objet1 == NULL ) && ( Objet2 == NULL ) )
        return 0;
    if( Objet1 == NULL )
        return -1;
    if( Objet2 == NULL )
        return 1;

    if( ( Objet1->m_Comp == NULL ) && ( Objet2->m_Comp == NULL ) )
        return 0;
    if( Objet1->m_Comp == NULL )
        return -1;
    if( Objet2->m_Comp == NULL )
        return 1;

    ii = strcmp( Objet1->m_Ref, Objet2->m_Ref );

    if( ii == 0 )
    {
        Text1 = &( Objet1->m_Comp->m_Field[VALUE].m_Text );
        Text2 = &( Objet2->m_Comp->m_Field[VALUE].m_Text );
        ii    = Text1->CmpNoCase( *Text2 );
    }

    if( ii == 0 )
    {
        ii = Objet1->m_Unit - Objet2->m_Unit;
    }

    return ii;
}


/******************************************************************/
static int ListTriGLabelByVal( ListLabel* Objet1, ListLabel* Objet2 )
/*******************************************************************/

/* Routine de comparaison pour le tri du Tableau par qsort()
 * Les labels sont tries
 *     par comparaison ascii
 *     si meme valeur: par numero de sheet
 */
{
    int ii;
    const wxString* Text1, * Text2;

    if( Objet1->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
        Text1 = &( (Hierarchical_PIN_Sheet_Struct*) Objet1->m_Label )->m_Text;
    else
        Text1 = &( (SCH_TEXT*) Objet1->m_Label )->m_Text;

    if( Objet2->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
        Text2 = &( (Hierarchical_PIN_Sheet_Struct*) Objet2->m_Label )->m_Text;
    else
        Text2 = &( (SCH_TEXT*) Objet2->m_Label )->m_Text;

    ii = Text1->CmpNoCase( *Text2 );

    if( ii == 0 )
    {
        ii = strcmp( Objet1->m_SheetPath, Objet2->m_SheetPath );
    }

    return ii;
}


/*******************************************************************/
static int ListTriGLabelBySheet( ListLabel* Objet1, ListLabel* Objet2 )
/*******************************************************************/

/* Routine de comparaison pour le tri du Tableau par qsort()
 * Les labels sont tries
 *     par sheet number
 *     si meme valeur, par ordre alphabetique
 */
{
    int ii;
    const wxString* Text1, * Text2;


    ii = strcmp( Objet1->m_SheetPath, Objet2->m_SheetPath );

    if( ii == 0 )
    {
        if( Objet1->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
            Text1 = &( (Hierarchical_PIN_Sheet_Struct*) Objet1->m_Label )->m_Text;
        else
            Text1 = &( (SCH_TEXT*) Objet1->m_Label )->m_Text;

        if( Objet2->m_LabelType == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
            Text2 = &( (Hierarchical_PIN_Sheet_Struct*) Objet2->m_Label )->m_Text;
        else
            Text2 = &( (SCH_TEXT*) Objet2->m_Label )->m_Text;

        ii = Text1->CmpNoCase( *Text2 );
    }

    return ii;
}


/**************************************************************/
static void DeleteSubCmp( ListComponent* List, int NbItems )
/**************************************************************/

/* Remove sub components from the list, when multiples parts per package are found in this list
 * The component list **MUST** be sorted by reference and by unit number
 */
{
    int ii;
    SCH_COMPONENT* LibItem;
    wxString OldName, CurrName;

    for( ii = 0; ii < NbItems; ii++ )
    {
        LibItem = List[ii].m_Comp;
        if( LibItem == NULL )
            continue;
        CurrName = CONV_FROM_UTF8( List[ii].m_Ref );
        if( !OldName.IsEmpty() )
        {
            if( OldName == CurrName )   // CurrName is a subpart of OldName: remove it
            {
                List[ii].m_Comp = NULL;
                List[ii].m_SheetList.Clear();
                List[ii].m_Ref[0] = 0;
            }
        }
        OldName = CurrName;
    }
}


/*******************************************************************************************/
void WinEDA_Build_BOM_Frame::PrintFieldData( FILE* f, SCH_COMPONENT* DrawLibItem,
                                             bool CompactForm )
/*******************************************************************************************/
{
    wxCheckBox* FieldListCtrl[] = {
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
    wxCheckBox* FieldCtrl = FieldListCtrl[0];

    if( CompactForm )
    {
        fprintf( f, "%c%s", s_ExportSeparatorSymbol,
            CONV_TO_UTF8( DrawLibItem->m_Field[FOOTPRINT].m_Text ) );
    }
    else if( m_AddFootprintField->IsChecked() )
        fprintf( f, "; %-12s", CONV_TO_UTF8( DrawLibItem->m_Field[FOOTPRINT].m_Text ) );

    for( ii = FIELD1; ii <= FIELD8; ii++ )
    {
        FieldCtrl = FieldListCtrl[ii - FIELD1];
        if( FieldCtrl == NULL )
            continue;
        if( !FieldCtrl->IsChecked() )
            continue;
        if( CompactForm )
            fprintf( f, "%c%s", s_ExportSeparatorSymbol,
                CONV_TO_UTF8( DrawLibItem->m_Field[ii].m_Text ) );
        else
            fprintf( f, "; %-12s", CONV_TO_UTF8( DrawLibItem->m_Field[ii].m_Text ) );
    }
}


/*********************************************************************************************/
int WinEDA_Build_BOM_Frame::PrintComponentsListByRef( FILE* f, ListComponent* List, int NbItems,
                                                      bool CompactForm, bool aIncludeSubComponents )
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
        wxCheckBox* FieldListCtrl[FIELD8 - FIELD1 + 1] = {
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
            fprintf( f, "%csheet path", s_ExportSeparatorSymbol );

        fprintf( f, "%cfootprint", s_ExportSeparatorSymbol );

        for( ii = FIELD1; ii <= FIELD8; ii++ )
        {
            wxCheckBox* FieldCtrl = FieldListCtrl[ii - FIELD1];
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
    for( ii = 0; ii < NbItems; ii++ )
    {
        DrawList = List[ii].m_Comp;

        if( DrawList == NULL )
            continue;
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) DrawList;
        if( List[ii].m_Ref[0] == '#' )
            continue;

        Multi = 0;
        Unit  = ' ';
        Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry )
            Multi = Entry->m_UnitCount;

        if( ( Multi > 1 ) && aIncludeSubComponents )
            Unit = List[ii].m_Unit + 'A' - 1;

        sprintf( CmpName, "%s", List[ii].m_Ref );
        if( !CompactForm || Unit != ' ' )
            sprintf( CmpName + strlen( CmpName ), "%c", Unit );

        if( CompactForm )
            fprintf( f, "%s%c%s", CmpName, s_ExportSeparatorSymbol,
                CONV_TO_UTF8( DrawLibItem->m_Field[VALUE].m_Text ) );
        else
            fprintf( f, "| %-10s %-12s", CmpName,
                CONV_TO_UTF8( DrawLibItem->m_Field[VALUE].m_Text ) );

        if( aIncludeSubComponents )
        {
            msg = List[ii].m_SheetList.PathHumanReadable();
            if( CompactForm )
                fprintf( f, "%c%s", s_ExportSeparatorSymbol, CONV_TO_UTF8( msg ) );
            else
                fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
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
int WinEDA_Build_BOM_Frame::PrintComponentsListByVal( FILE* f, ListComponent* List, int NbItems,
                                                      bool aIncludeSubComponents )
/**********************************************************************************************/
{
    int ii, Multi;
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

    for( ii = 0; ii < NbItems; ii++ )
    {
        DrawList = List[ii].m_Comp;

        if( DrawList == NULL )
            continue;
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;

        DrawLibItem = (SCH_COMPONENT*) DrawList;
        if( List[ii].m_Ref[0] == '#' )
            continue;

        Multi = 0;
        Unit  = ' ';
        Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry )
            Multi = Entry->m_UnitCount;

        if( ( Multi > 1 ) && aIncludeSubComponents )
        {
            Unit = List[ii].m_Unit + 'A' - 1;
        }

        sprintf( CmpName, "%s%c", List[ii].m_Ref, Unit );
        fprintf( f, "| %-12s %-10s", CONV_TO_UTF8( DrawLibItem->m_Field[VALUE].m_Text ), CmpName );

        // print the sheet path
        if( aIncludeSubComponents )
        {
            msg = List[ii].m_SheetList.PathHumanReadable();
            fprintf( f, "   (Sheet %s)", CONV_TO_UTF8( msg ) );
        }

        PrintFieldData( f, DrawLibItem );

        fprintf( f, "\n" );
    }

    msg = _( "#End Cmp\n" );
    fprintf( f, CONV_TO_UTF8( msg ) );
    return 0;
}


/******************************************************************/
static int PrintListeGLabel( FILE* f, ListLabel* List, int NbItems )
/******************************************************************/
{
    int ii, jj;
    SCH_LABEL* DrawTextItem;
    Hierarchical_PIN_Sheet_Struct* DrawSheetLabel;
    ListLabel* LabelItem;
    wxString msg, sheetpath;
    wxString labeltype;

    for( ii = 0; ii < NbItems; ii++ )
    {
        LabelItem = &List[ii];

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
