/*******************************************************/
/* Module de generation de la Netliste , selon Formats */
/*******************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "protos.h"

/* Routines locales */
static void Write_GENERIC_NetList( WinEDA_SchematicFrame* frame, const wxString& FullFileName );
static void WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f,
                                bool with_pcbnew );
static void WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f );
static void WriteListOfNetsCADSTAR( FILE* f, ObjetNetListStruct* ObjNet );
static void WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f, bool use_netnames );

static void WriteGENERICListOfNets( FILE* f, ObjetNetListStruct* ObjNet );
static void AddPinToComponentPinList( SCH_COMPONENT* Component,
                                      DrawSheetPath* 		  sheet,
                                      LibDrawPin*             PinEntry );
static void FindOthersUnits( SCH_COMPONENT* Component, DrawSheetPath* Sheet_in);
static int  SortPinsByNum( ObjetNetListStruct** Pin1, ObjetNetListStruct** Pin2 );
static void EraseDuplicatePins( ObjetNetListStruct** TabPin, int NbrPin );

static void ClearUsedFlags( WinEDA_SchematicFrame* frame );


/* Variable locales */
static int s_SortedPinCount;
static ObjetNetListStruct** s_SortedComponentPinList;


/******************************************************************************/
void WriteNetList( WinEDA_SchematicFrame* frame, const wxString& FileNameNL,
                   bool use_netnames )
/*******************************************************************************/

/* Create the netlist file ( Format is given by g_NetFormat )
 *  bool use_netnames is used only for Spice netlist
 */
{
    FILE*        f = NULL;

    if( g_NetFormat < NET_TYPE_CUSTOM1 )
    {
        if( ( f = wxFopen( FileNameNL, wxT( "wt" ) ) ) == NULL )
        {
            wxString msg = _( "Failed to create file " ) + FileNameNL;
            DisplayError( frame, msg );
            return;
        }
    }

    wxBusyCursor Busy;

    switch( g_NetFormat )
    {
    case NET_TYPE_PCBNEW:
        WriteNetListPCBNEW( frame, f, TRUE );
        fclose( f );
        break;

    case NET_TYPE_ORCADPCB2:
        WriteNetListPCBNEW( frame, f, FALSE );
        fclose( f );
        break;

    case NET_TYPE_CADSTAR:
        WriteNetListCADSTAR( frame, f );
        fclose( f );
        break;

    case NET_TYPE_SPICE:
        WriteNetListPspice( frame, f, use_netnames );
        fclose( f );
        break;

     default:
       Write_GENERIC_NetList( frame, FileNameNL );
        break;
   }
}


/****************************************************************************/
static SCH_COMPONENT* FindNextComponentAndCreatPinList(
        EDA_BaseStruct* DrawList, DrawSheetPath* sheet)
/****************************************************************************/

/*	Find a "suitable" component from the DrawList
 *  build its pin list s_SortedComponentPinList.
 *  The list is sorted by pin num
 *  A suitable component is a "new" real component (power symbols are not considered)
 *
 *  alloc memory for s_SortedComponentPinList if s_SortedComponentPinList == NULL
 *  Must be deallocated by the user
 */
{
    SCH_COMPONENT* Component = NULL;
    EDA_LibComponentStruct* Entry;
    LibEDA_BaseStruct*      DEntry;

    s_SortedPinCount = 0;

    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;
        Component = (SCH_COMPONENT*) DrawList;

        /* already tested ? : */
        bool found = false;
        for(unsigned int i =0; i<Component->m_UsedOnSheets.GetCount(); i++){
            if( Component->m_UsedOnSheets.Item(i) == *sheet ){
                found = true;
                break;
            }
        }
        if( found ) continue;
        //if( Component->m_FlagControlMulti == 1 )
        //    continue;                                      /* yes */
        // removed because with multiple instances of one schematic
        // (several sheets pointing to 1 screen), this will be erroneously be toggled.

        Entry = FindLibPart( Component->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry  == NULL )
            continue;

        /* Power symbol and other component which have the reference starting by
         *  "#" are not included in netlist (pseudo components) */
        wxString str = Component->GetRef(sheet);
        if( str[0] == '#' )
            continue;

        /* Create the pin table for this component */
        int ii = sizeof(ObjetNetListStruct*) * MAXPIN;
        if( s_SortedComponentPinList == NULL )
            s_SortedComponentPinList = (ObjetNetListStruct**) MyMalloc( ii );
        memset( s_SortedComponentPinList, 0, ii );

        DEntry = Entry->m_Drawings;
        for( ; DEntry != NULL; DEntry = DEntry->Next() )
        {
            if( DEntry->Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( DEntry->m_Unit
               && (DEntry->m_Unit != Component->m_Multi) )
                continue;
            if( DEntry->m_Convert
               && (DEntry->m_Convert != Component->m_Convert) )
                continue;
            {
                AddPinToComponentPinList( Component, sheet, (LibDrawPin*) DEntry );
            }
        }

        //Component->m_FlagControlMulti = 1;
        Component->m_UsedOnSheets.Add(*sheet);

        if( Entry->m_UnitCount > 1 )
            FindOthersUnits( Component, sheet);

        /* Tri sur le numero de Pin de TabListePin */
        qsort( s_SortedComponentPinList, s_SortedPinCount, sizeof(ObjetNetListStruct*),
               ( int( * ) ( const void*, const void* ) )SortPinsByNum );

        /* Elimination des Pins redondantes du s_SortedComponentPinList */
        EraseDuplicatePins( s_SortedComponentPinList, s_SortedPinCount );

        return Component;
    }

    return NULL;
}


/**************************************************************************************/
static wxString ReturnPinNetName( ObjetNetListStruct* Pin,
                                  const wxString&     DefaultFormatNetname )
/**************************************************************************************/

/* Return the net name for the pin Pin.
 *  Net name is:
 *  "?" if pin not connected
 *  "netname" for global net (like gnd, vcc ..
 *  "netname_sheetnumber" for the usual nets
 */
{
    int      netcode = Pin->GetNet();
    wxString NetName;

    if( (netcode == 0 ) || ( Pin->m_FlagOfConnection != PAD_CONNECT ) )
    {
        return NetName;
    }
    else
    {
        int jj;
        for( jj = 0; jj < g_NbrObjNet; jj++ )
        {
            if( g_TabObjNet[jj].GetNet() != netcode )
                continue;
            if( ( g_TabObjNet[jj].m_Type != NET_HIERLABEL)
               && ( g_TabObjNet[jj].m_Type != NET_LABEL)
                && ( g_TabObjNet[jj].m_Type != NET_PINLABEL) )
                continue;

            NetName = *g_TabObjNet[jj].m_Label;
            break;
        }

        if( !NetName.IsEmpty() )
        {
            if( g_TabObjNet[jj].m_Type != NET_PINLABEL ){
                NetName = g_TabObjNet[jj].m_SheetList.PathHumanReadable()
                            + NetName;
            }
                //NetName << wxT("_") << g_TabObjNet[jj].m_SheetList.PathHumanReadable();
        }
        else
        {
            NetName.Printf( DefaultFormatNetname.GetData(), netcode );
        }
    }
    return NetName;
}

/***********************************************************************/
void Write_GENERIC_NetList( WinEDA_SchematicFrame* frame,
                            const wxString&        FullFileName )
/***********************************************************************/

/* Create a generic netlist, and call an external netlister
 *  to change the netlist syntax and create the file
 * -- does this still work?
 */
{
    wxString                Line, FootprintName;
    DrawSheetPath* 		sheet;
    EDA_BaseStruct*         DrawList;
    SCH_COMPONENT* Component;
    wxString                netname;
    int ii;
    FILE* tmpfile;
    wxString                TmpFullFileName = FullFileName;

    ChangeFileNameExt( TmpFullFileName, wxT( ".tmp" ) );

    if( ( tmpfile = wxFopen( TmpFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        wxString msg = _( "Failed to create file " ) + TmpFullFileName;
        DisplayError( frame, msg );
        return;
    }

    ClearUsedFlags( frame );  /* Reset the flags FlagControlMulti in all schematic files*/
    fprintf( tmpfile, "$BeginNetlist\n" );

    /* Create netlist module section */
    fprintf( tmpfile, "$BeginComponentList\n" );
    EDA_SheetList SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Pnext )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet );

            if( Component == NULL )
                break;                      // No component left

            FootprintName.Empty();
            if( !Component->m_Field[FOOTPRINT].IsVoid() )
            {
                FootprintName = Component->m_Field[FOOTPRINT].m_Text;
                FootprintName.Replace( wxT( " " ), wxT( "_" ) );
            }

            fprintf( tmpfile, "\n$BeginComponent\n" );
            fprintf( tmpfile, "TimeStamp=%8.8lX\n", Component->m_TimeStamp );
            fprintf( tmpfile, "Footprint=%s\n", CONV_TO_UTF8( FootprintName ) );
            Line = wxT( "Reference=" ) + Component->GetRef(sheet) + wxT( "\n" );
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( tmpfile, CONV_TO_UTF8( Line ) );

            Line = Component->m_Field[VALUE].m_Text;
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( tmpfile, "Value=%s\n", CONV_TO_UTF8( Line ) );

            Line = Component->m_ChipName;
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( tmpfile, "Libref=%s\n", CONV_TO_UTF8( Line ) );

            // Write pin list:
            fprintf( tmpfile, "$BeginPinList\n" );
            for( ii = 0; ii < s_SortedPinCount; ii++ )
            {
                ObjetNetListStruct* Pin = s_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                netname = ReturnPinNetName( Pin, wxT( "$-%.6d" ) );
                if( netname.IsEmpty() )
                    netname = wxT( "?" );
                fprintf( tmpfile, "%.4s=%s\n", (char*) &Pin->m_PinNum,
                        CONV_TO_UTF8( netname ) );
            }

            fprintf( tmpfile, "$EndPinList\n" );
            fprintf( tmpfile, "$EndComponent\n" );
        }
    }

    MyFree( s_SortedComponentPinList );
    s_SortedComponentPinList = NULL;

    fprintf( tmpfile, "$EndComponentList\n" );

    fprintf( tmpfile, "\n$BeginNets\n" );
    WriteGENERICListOfNets( tmpfile, g_TabObjNet );
    fprintf( tmpfile, "$EndNets\n" );
    fprintf( tmpfile, "\n$EndNetlist\n" );
    fclose( tmpfile );

    // Call the external module (plug in )

    if( g_NetListerCommandLine.IsEmpty() )
        return;

    wxString CommandFile;
    if( wxIsAbsolutePath( g_NetListerCommandLine ) )
        CommandFile = g_NetListerCommandLine;
    else
        CommandFile = FindKicadFile( g_NetListerCommandLine );

    CommandFile += wxT( " " ) + TmpFullFileName;
    CommandFile += wxT( " " ) + FullFileName;

    ProcessExecute( CommandFile, wxEXEC_SYNC );
}


/********************************************************/
static void ClearUsedFlags( WinEDA_SchematicFrame* frame )
/********************************************************/
/* Clear flag list, used in netlist generation */
{
    SCH_SCREEN*     screen;
    EDA_BaseStruct* DrawList;

    EDA_ScreenList ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        DrawList = screen->EEDrawList;
        while( DrawList )
        {
            if( DrawList->Type() == TYPE_SCH_COMPONENT )
            {
                SCH_COMPONENT* Component = (SCH_COMPONENT*) DrawList;
                //Component->m_FlagControlMulti = 0;
                Component->m_UsedOnSheets.Clear();
            }
            DrawList = DrawList->Pnext;
        }
    }
}


/*************************************************************/
static void WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f,
                                bool use_netnames )
/*************************************************************/

/* Routine de generation du fichier netliste ( Format PSPICE )
 *  si use_netnames = TRUE
 *      les nodes sont identifies par le netname
 *  sinon	les nodes sont identifies par le netnumber
 *
 *  tous les textes graphiques commen�ant par [.-+]pspice ou  [.-+]gnucap
 *  sont consid�r�s comme des commandes a placer dans la netliste
 *      [.-]pspice ou gnucap sont en debut
 +pspice et +gnucap sont en fin de netliste
 */
{
    char                    Line[1024];
    DrawSheetPath* 		sheet;
    EDA_BaseStruct*         DrawList;
    SCH_COMPONENT* Component;
    int ii, nbitems;
    wxString                text;
    wxArrayString           SpiceCommandAtBeginFile, SpiceCommandAtEndFile;
    wxString                msg;

#define BUFYPOS_LEN 4
    wxChar                  bufnum[BUFYPOS_LEN + 1];

    DateAndTime( Line );
    fprintf( f, "* %s (Spice format) creation date: %s\n\n", NETLIST_HEAD_STRING, Line );

    /* Create text list starting by [.-]pspice , or [.-]gnucap (simulator commands) */
    /* and create text list starting by [+]pspice , or [+]gnucap (simulator commands) */
    bufnum[BUFYPOS_LEN] = 0;
    EDA_SheetList SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Pnext )
        {
            wxChar ident;
            if( DrawList->Type() != TYPE_SCH_TEXT )
                continue;
            #define DRAWTEXT ( (SCH_TEXT*) DrawList )
            text  = DRAWTEXT->m_Text; if( text.IsEmpty() )
                continue;
            ident = text.GetChar( 0 );
            if( ident != '.' && ident != '-' && ident != '+' )
                continue;
            text.Remove( 0, 1 );    //Remove the first char.
            text.Remove( 6 );       //text contains 6 char.
            if( ( text == wxT( "pspice" ) ) || ( text == wxT( "gnucap" ) ) )
            {
                /* Put the Y position as an ascii string, for sort by vertical position,
                 *  using usual sort string by alphabetic value */
                int ypos = DRAWTEXT->m_Pos.y;
                for( ii = 0; ii < BUFYPOS_LEN; ii++ )
                {
                    bufnum[BUFYPOS_LEN - 1 - ii] = (ypos & 63) + ' '; ypos >>= 6;
                }

                text = DRAWTEXT->m_Text.AfterFirst( ' ' );
                msg.Printf( wxT( "%s %s" ), bufnum, text.GetData() );    // First BUFYPOS_LEN char are the Y position
                if( ident == '+' )
                    SpiceCommandAtEndFile.Add( msg );
                else
                    SpiceCommandAtBeginFile.Add( msg );
            }
        }
    }

    /* Print texts starting by [.-]pspice , ou [.-]gnucap (of course, without the Y position string)*/
    nbitems = SpiceCommandAtBeginFile.GetCount();
    if( nbitems )
    {
        SpiceCommandAtBeginFile.Sort();
        for( ii = 0; ii < nbitems; ii++ )
        {
            SpiceCommandAtBeginFile[ii].Remove( 0, BUFYPOS_LEN );
            SpiceCommandAtBeginFile[ii].Trim( TRUE );
            SpiceCommandAtBeginFile[ii].Trim( FALSE );
            fprintf( f, "%s\n", CONV_TO_UTF8( SpiceCommandAtBeginFile[ii] ) );
        }
    }
    fprintf( f, "\n" );


    /* Create component list */
    ClearUsedFlags( frame );  /* Reset the flags FlagControlMulti in all schematic files*/
    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Pnext )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet );
            if( Component == NULL )
                break;

            fprintf( f, "%s ", CONV_TO_UTF8( Component->GetRef( sheet ) ) );

            // Write pin list:
            for( ii = 0; ii < s_SortedPinCount; ii++ )
            {
                ObjetNetListStruct* Pin = s_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                wxString            NetName = ReturnPinNetName( Pin, wxT( "N-%.6d" ) );
                if( NetName.IsEmpty() )
                    NetName = wxT( "?" );
                if( use_netnames )
                    fprintf( f, " %s", CONV_TO_UTF8( NetName ) );
                else    // Use number for net names (with net number = 0 for "GND"
                {
                    // NetName = "0" is "GND" net for Spice
                    if( NetName == wxT( "0" ) || NetName == wxT( "GND" ) )
                        fprintf( f, " 0" );
                    else
                        fprintf( f, " %d", Pin->GetNet() );
                }
            }

            fprintf( f, " %s\n", CONV_TO_UTF8( Component->m_Field[VALUE].m_Text ) );
        }
    }

    MyFree( s_SortedComponentPinList );
    s_SortedComponentPinList = NULL;

    /* Print texts starting by [+]pspice , ou [+]gnucap */
    nbitems = SpiceCommandAtEndFile.GetCount();
    if( nbitems )
    {
        fprintf( f, "\n" );
        SpiceCommandAtEndFile.Sort();
        for( ii = 0; ii < nbitems; ii++ )
        {
            SpiceCommandAtEndFile[ii].Remove( 0, +BUFYPOS_LEN );
            SpiceCommandAtEndFile[ii].Trim( TRUE );
            SpiceCommandAtEndFile[ii].Trim( FALSE );
            fprintf( f, "%s\n", CONV_TO_UTF8( SpiceCommandAtEndFile[ii] ) );
        }
    }

    fprintf( f, "\n.end\n" );
}


/*****************************************************************************************/
static void WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f, bool with_pcbnew )
/*****************************************************************************************/

/* Routine de generation du fichier netliste ( Format ORCAD PCB 2 ameliore )
 *  si with_pcbnew = FALSE
 *      format PCBNEW (OrcadPcb2 + commentaires et liste des nets)
 *  si with_pcbnew = FALSE
 *      Format ORCADPCB2 strict
 */
{
    wxString Line, FootprintName;
    char Buf[256];
    DrawSheetPath* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* Component;
    int ii;
    ListComponent* CmpList = NULL;
    int CmpListCount = 0, CmpListSize = 1000;

    DateAndTime( Buf );
    if( with_pcbnew )
        fprintf( f, "# %s created  %s\n(\n", NETLIST_HEAD_STRING, Buf );
    else
        fprintf( f, "( { %s created  %s }\n", NETLIST_HEAD_STRING, Buf );


        /* Create netlist module section */
    ClearUsedFlags( frame );  /* Reset the flags FlagControlMulti in all schematic files*/

    EDA_SheetList SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Pnext )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet);
            if( Component == NULL )
                break;

            /* Get the Component FootprintFilter and put the component in CmpList if filter is not void */
            EDA_LibComponentStruct* Entry;
            if( ( Entry = FindLibPart( Component->m_ChipName.GetData(), wxEmptyString,
                                       FIND_ROOT ) ) != NULL )
            {
                if( Entry->m_FootprintList.GetCount() != 0 )   /* Put in list */
                {
                    if( CmpList == NULL ){
                        CmpList = (ListComponent*)
                                MyZMalloc( sizeof(ListComponent) * CmpListSize );
                    }
                    if( CmpListCount >= CmpListSize )
                    {
                        CmpListSize += 1000;
                        CmpList = (ListComponent*) realloc(
                            CmpList,
                            sizeof(ListComponent) * CmpListSize );
                    }
                    CmpList[CmpListCount].m_Comp = Component;
                    strcpy(CmpList[CmpListCount].m_Ref, Component->GetRef( sheet ).mb_str());
                    CmpListCount++;
                }
            }

            if( !Component->m_Field[FOOTPRINT].IsVoid() )
            {
                FootprintName = Component->m_Field[FOOTPRINT].m_Text;
                FootprintName.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                FootprintName = wxT( "$noname" );

            Line = Component->GetRef(sheet);
            fprintf( f, " ( %s %s",
                    CONV_TO_UTF8(Component->GetPath(sheet)),
                    CONV_TO_UTF8( FootprintName ) );
            fprintf( f, "  %s", CONV_TO_UTF8( Line ) );

            Line = Component->m_Field[VALUE].m_Text;
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( f, " %s", CONV_TO_UTF8( Line ) );

            if( with_pcbnew )  // Add the lib name for this component
            {
                Line = Component->m_ChipName;
                Line.Replace( wxT( " " ), wxT( "_" ) );
                fprintf( f, " {Lib=%s}", CONV_TO_UTF8( Line ) );
            }
            fprintf( f, "\n" );

            // Write pin list:
            for( ii = 0; ii < s_SortedPinCount; ii++ )
            {
                ObjetNetListStruct* Pin = s_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                wxString netname = ReturnPinNetName( Pin, wxT( "N-%.6d" ) );
                if( netname.IsEmpty() )
                    netname = wxT( " ?" );
                fprintf( f, "  ( %4.4s %s )\n", (char*) &Pin->m_PinNum,
                        CONV_TO_UTF8( netname ) );
            }

            fprintf( f, " )\n" );
        }
    }

    fprintf( f, ")\n*\n" );

    MyFree( s_SortedComponentPinList );
    s_SortedComponentPinList = NULL;

    /* Write the allowed footprint list for each component */
    if( with_pcbnew && CmpList )
    {
        fprintf( f, "{ Allowed footprints by component:\n" );
        EDA_LibComponentStruct* Entry;
        for( ii = 0; ii < CmpListCount; ii++ )
        {
            Component = CmpList[ii].m_Comp;
            Entry = FindLibPart( Component->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
            //Line.Printf(_("%s"), CmpList[ii].m_Ref);
            //Line.Replace( wxT( " " ), wxT( "_" ) );
            unsigned int i;
            for(i=0; i<sizeof(CmpList[ii].m_Ref) && CmpList[ii].m_Ref[i]; i++){
                if(CmpList[ii].m_Ref[i] == ' ')
                    CmpList[ii].m_Ref[i] = '_';
            }
            fprintf( f, "$component %s\n", CmpList[ii].m_Ref );
            /* Write the footprint list */
            for( unsigned int jj = 0; jj < Entry->m_FootprintList.GetCount(); jj++ )
            {
                fprintf( f, " %s\n", CONV_TO_UTF8( Entry->m_FootprintList[jj] ) );
            }

            fprintf( f, "$endlist\n" );
        }

        fprintf( f, "$endfootprintlist\n}\n" );
    }
    if( CmpList )
        free( CmpList );

    if( with_pcbnew )
    {
        fprintf( f, "{ Pin List by Nets\n" );
        WriteGENERICListOfNets( f, g_TabObjNet );
        fprintf( f, "}\n" );
        fprintf( f, "#End\n" );
    }
}


/*************************************************************************************/
static void AddPinToComponentPinList( SCH_COMPONENT* Component,
                                      DrawSheetPath* sheetlist, LibDrawPin* Pin )
/*************************************************************************************/

/* Add a new pin description in the pin list s_SortedComponentPinList
 *  a pin description is a pointer to the corresponding structure
 *  created by BuildNetList() in the table g_TabObjNet
 */
{
    int ii;

    /* Search the PIN description for Pin in g_TabObjNet*/
    for( ii = 0; ii < g_NbrObjNet; ii++ )
    {
        if( g_TabObjNet[ii].m_Type != NET_PIN )
            continue;
        if( g_TabObjNet[ii].m_Link != Component )
            continue;
        if( g_TabObjNet[ii].m_SheetList != *sheetlist )
            continue;
        if( g_TabObjNet[ii].m_PinNum != Pin->m_PinNum )
            continue;
        {
            s_SortedComponentPinList[s_SortedPinCount] = &g_TabObjNet[ii];
            s_SortedPinCount++;
            if( s_SortedPinCount >= MAXPIN )
            {
                /* Log message for Internal error */
                DisplayError( NULL, wxT( "AddPinToComponentPinList err: MAXPIN reached" ) );
                return;
            }
        }
    }
}


/**********************************************************************/
static void EraseDuplicatePins( ObjetNetListStruct** TabPin, int NbrPin )
/**********************************************************************/

/*
 *  Routine qui elimine les Pins de meme Numero de la liste des objets
 *  (Liste des Pins d'un boitier) passee en entree
 *  Ces pins redondantes proviennent des pins (alims... ) communes a plusieurs
 *  elements d'un boitier a multiple parts.
 */
{
    int ii, jj;

    for( ii = 0; ii < NbrPin - 1; ii++ )
    {
        if( TabPin[ii] == NULL )
            continue;                       /* Deja supprime */
        if( TabPin[ii]->m_PinNum != TabPin[ii + 1]->m_PinNum )
            continue;
        /* 2 Pins doublees */
        for( jj = ii + 1; jj < NbrPin; jj++ )
        {
            if( TabPin[ii]->m_PinNum != TabPin[jj]->m_PinNum )
                break;
            TabPin[jj] = NULL;
        }
    }
}


/**********************************************************************/
static void FindOthersUnits( SCH_COMPONENT* Component_in, DrawSheetPath* Sheet_in)
/**********************************************************************/

/* Recherche les autres parts du boitier auquel appartient la part Component,
 *  pour les boitiers a parts multiples.
 *  Appelle AddPinToComponentPinList() pour classer les pins des parts trouvees
 */
{
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* Component2;
    EDA_LibComponentStruct* Entry;
    LibEDA_BaseStruct* DEntry;
    DrawSheetPath* sheet;
    wxString str;

    EDA_SheetList SheetList( NULL );
    unsigned int i;
    bool found;
    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        DrawList = sheet->LastDrawList();
        while( DrawList )
        {
            switch( DrawList->Type() )
            {
            case TYPE_SCH_COMPONENT:
                Component2 = (SCH_COMPONENT*) DrawList;

                found = false;
                for( i =0; i<Component2->m_UsedOnSheets.GetCount(); i++){
                    if( Component2->m_UsedOnSheets.Item(i) == *Sheet_in ){
                        found = true;
                    }
                }
                if( found ) break;
                //if( Component2->m_FlagControlMulti == 1 ) //has it been used? (on this sheet?)
                //    break;
                str = Component2->GetRef(sheet);
                if( str.CmpNoCase( Component_in->GetRef(Sheet_in) ) != 0 )
                    break;

                Entry = FindLibPart( Component2->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
                if( Entry  == NULL )
                    break;

                if( str[0] == '#' )
                    break;

                if( Entry->m_Drawings != NULL )
                {
                    DEntry = Entry->m_Drawings;
                    for( ; DEntry != NULL; DEntry = DEntry->Next() )
                    {
                        if( DEntry->Type() != COMPONENT_PIN_DRAW_TYPE )
                            continue;
                        if( DEntry->m_Unit
                           && (DEntry->m_Unit != Component2->m_Multi) )
                            continue;
                        if( DEntry->m_Convert
                           && (DEntry->m_Convert != Component2->m_Convert) )
                            continue;
                        {
                            AddPinToComponentPinList( Component2, sheet, (LibDrawPin*) DEntry );
                        }
                    }
                }
                Component2->m_UsedOnSheets.Add(*Sheet_in);
                //Component2->m_FlagControlMulti = 1; //mark it as used..
                break;

            default:
                break;
            }

            DrawList = DrawList->Pnext;
        }
    }
}


/**************************************************************************/
static int SortPinsByNum( ObjetNetListStruct** Pin1, ObjetNetListStruct** Pin2 )
/**************************************************************************/

/* Routine de comparaison pour le tri des pins par numero croissant
 *  du tableau des pins s_SortedComponentPinList par qsort()
 */
{
    ObjetNetListStruct* Obj1, * Obj2;
    int Num1, Num2;
    char Line[5];

    Obj1    = *Pin1; Obj2 = *Pin2;
    Num1    = Obj1->m_PinNum; Num2 = Obj2->m_PinNum;
    Line[4] = 0; memcpy( Line, &Num1, 4 ); Num1 = atoi( Line );
    memcpy( Line, &Num2, 4 ); Num2 = atoi( Line );
    return Num1 - Num2;
}


/*************************************************************************/
static void WriteGENERICListOfNets( FILE* f, ObjetNetListStruct* ObjNet )
/*************************************************************************/

/* Ecrit dans le fichier f la liste des nets ( classee par NetCodes ), et des
 *  elements qui y sont connectes
 */
{
    int ii, jj;
    int NetCode, LastNetCode = -1;
    int SameNetcodeCount = 0;
    SCH_COMPONENT* Cmp;
    wxString NetName, CmpRef;
    wxString NetcodeName;
    char FirstItemInNet[1024];

    for( ii = 0; ii < g_NbrObjNet; ii++ )
    {
        if( (NetCode = ObjNet[ii].GetNet()) != LastNetCode )   // New net found, write net id;
        {
            SameNetcodeCount = 0;                               // Items count for this net
            NetName.Empty();
            for( jj = 0; jj < g_NbrObjNet; jj++ )               // Find a label (if exists) for this net
            {
                if( ObjNet[jj].GetNet() != NetCode )
                    continue;
                if( ( ObjNet[jj].m_Type != NET_HIERLABEL)
                   && ( ObjNet[jj].m_Type != NET_LABEL)
                   && ( ObjNet[jj].m_Type != NET_PINLABEL) )
                    continue;

                NetName = *g_TabObjNet[jj].m_Label; break;
            }

            NetcodeName.Printf( wxT( "Net %d " ), NetCode );
            NetcodeName += wxT( "\"" );
            if( !NetName.IsEmpty() )
            {
                NetcodeName += NetName;
                if( g_TabObjNet[jj].m_Type != NET_PINLABEL ){
                    // usual net name, add in the sheet path
                    NetcodeName = g_TabObjNet[jj].m_SheetList.PathHumanReadable()
                            + NetcodeName;
                    //NetcodeName << wxT("_") <<
                    //		g_TabObjNet[jj].m_SheetList.PathHumanReadable();
                }
            }
            NetcodeName += wxT( "\"" );
            LastNetCode  = NetCode;
        }

        if( ObjNet[ii].m_Type != NET_PIN )
            continue;

        Cmp    = (SCH_COMPONENT*) ObjNet[ii].m_Link;
        CmpRef = Cmp->GetRef(&ObjNet[ii].m_SheetList); //is this correct?
        if( CmpRef.StartsWith( wxT( "#" ) ) )
            continue;                                   // Pseudo component (Like Power symbol)

        // Print the pin list for this net, if  2 or more items are connected:
        SameNetcodeCount++;
        if( SameNetcodeCount == 1 )     /* first item for this net found,
                                         *  Print this connection, when a second item will be found */
        {
            sprintf( FirstItemInNet, " %s %.4s\n", CONV_TO_UTF8( CmpRef ),
                     (const char*) &ObjNet[ii].m_PinNum );
        }
        if( SameNetcodeCount == 2 )    // Second item for this net found, Print the Net name, and the first item
        {
            fprintf( f, "%s\n", CONV_TO_UTF8( NetcodeName ) );
            fputs( FirstItemInNet, f );
        }

        if( SameNetcodeCount >= 2 )
            fprintf( f, " %s %.4s\n", CONV_TO_UTF8( CmpRef ),
                     (const char*) &ObjNet[ii].m_PinNum );
    }
}


/* Generation des netlistes au format CadStar */
wxString StartLine( wxT( "." ) );

/*********************************************************/
static void WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f )
/*********************************************************/

/* Routine de generation du fichier netliste ( Format CADSTAR )
 *  Entete:
 *  ..HEA
 *  ..TIM 2004 07 29 16 22 17
 *  ..APP "Cadstar RINF Output - Version 6.0.2.3"
 *  ..UNI INCH 1000.0 in
 *  ..TYP FULL
 *
 *  liste des composants:
 *  ..ADD_COM    X1         "CNT D41612 (48PTS MC CONTOUR)"
 *  ..ADD_COM    U2         "74HCT245D" "74HCT245D"
 *
 *  Connexions:
 *  ..ADD_TER    RR2        6          "$42"
 *  ..TER        U1         100
 *          CA         6
 *
 *  ..ADD_TER    U2         6          "$59"
 *  ..TER        U7         39
 *          U6         17
 *          U1         122
 *
 *  ..ADD_TER    P2         1          "$9"
 *  ..TER        T3         1
 *          U1         14
 */
{
    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    wxString FootprintName;
    char Line[1024];
    DrawSheetPath* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* Component;
    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();

    fprintf( f, "%sHEA\n", CONV_TO_UTF8( StartLine ) );
    DateAndTime( Line );
    fprintf( f, "%sTIM %s\n", CONV_TO_UTF8( StartLine ), Line );
    fprintf( f, "%sAPP ", CONV_TO_UTF8( StartLine ) );
    fprintf( f, "\"%s\"\n", CONV_TO_UTF8( Title ) );
    fprintf( f, "\n" );

    /* Create netlist module section */
    ClearUsedFlags( frame );  /* Reset the flags FlagControlMulti in all schematic files*/
    EDA_SheetList SheetList( NULL );

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Pnext )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet);
            if( Component == NULL )
                break;

            if( !Component->m_Field[FOOTPRINT].IsVoid() )
            {
                FootprintName = Component->m_Field[FOOTPRINT].m_Text;
                FootprintName.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                FootprintName = wxT( "$noname" );

            msg = Component->GetRef(sheet);
            fprintf( f, "%s     ", CONV_TO_UTF8( StartCmpDesc ) );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );

            msg = Component->m_Field[VALUE].m_Text;
            msg.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( f, "     \"%s\"", CONV_TO_UTF8( msg ) );
            fprintf( f, "\n" );
        }
    }

    fprintf( f, "\n" );

    MyFree( s_SortedComponentPinList );
    s_SortedComponentPinList = NULL;

    WriteListOfNetsCADSTAR( f, g_TabObjNet );

    fprintf( f, "\n%sEND\n", CONV_TO_UTF8( StartLine ) );
}


/*************************************************************************/
static void WriteListOfNetsCADSTAR( FILE* f, ObjetNetListStruct* ObjNet )
/*************************************************************************/

/* Ecrit dans le fichier f la liste des nets ( classee par NetCodes ), et des
 *  pins qui y sont connectes
 *  format:
 *  .ADD_TER    RR2        6          "$42"
 *  .TER        U1         100
 *          CA         6
 */
{
    wxString InitNetDesc  = StartLine + wxT( "ADD_TER" );
    wxString StartNetDesc = StartLine + wxT( "TER" );
    wxString NetcodeName, InitNetDescLine;
    int ii, jj, print_ter = 0;
    int NetCode, LastNetCode = -1;
    SCH_COMPONENT* Cmp;
    wxString NetName;

    for( ii = 0; ii < g_NbrObjNet; ii++ )
        ObjNet[ii].m_Flag = 0;

    for( ii = 0; ii < g_NbrObjNet; ii++ )
    {
        // Get the NetName of the current net :
        if( (NetCode = ObjNet[ii].GetNet()) != LastNetCode )
        {
            NetName.Empty();
            for( jj = 0; jj < g_NbrObjNet; jj++ )
            {
                if( ObjNet[jj].GetNet() != NetCode )
                    continue;
                if( ( ObjNet[jj].m_Type != NET_HIERLABEL)
                   && ( ObjNet[jj].m_Type != NET_LABEL)
                   && ( ObjNet[jj].m_Type != NET_PINLABEL) )
                    continue;

                NetName = *ObjNet[jj].m_Label; break;
            }

            NetcodeName = wxT( "\"" );
            if( !NetName.IsEmpty() )
            {
                NetcodeName += NetName;
                if( g_TabObjNet[jj].m_Type != NET_PINLABEL ){
                    NetcodeName = g_TabObjNet[jj].m_SheetList.PathHumanReadable()
                            + NetcodeName;
                    //NetcodeName << wxT("_") <<
                    //		g_TabObjNet[jj].m_SheetList.PathHumanReadable();
                }
            }
            else  // this net has no name: create a default name $<net number>
                NetcodeName << wxT( "$" ) << NetCode;
            NetcodeName += wxT( "\"" );
            LastNetCode  = NetCode;
            print_ter    = 0;
        }


        if( ObjNet[ii].m_Type != NET_PIN )
            continue;

        if( ObjNet[ii].m_Flag != 0 )
            continue;

        Cmp = (SCH_COMPONENT*) ObjNet[ii].m_Link;
        wxString refstr = Cmp->GetRef(&(ObjNet[ii].m_SheetList));
        if( refstr[0] == '#' )
            continue; // Pseudo composant (symboles d'alims)

        switch( print_ter )
        {
        case 0:
        {
            char buf[5];
            wxString str_pinnum;
            strncpy( buf, (char*) &ObjNet[ii].m_PinNum, 4 ); buf[4] = 0;
            str_pinnum = CONV_FROM_UTF8( buf );
            InitNetDescLine.Printf( wxT( "\n%s   %s   %.4s     %s" ),
                                   InitNetDesc.GetData(),
                                   refstr.GetData(),
                                   str_pinnum.GetData(), NetcodeName.GetData() );
        }
            print_ter++;
            break;

        case 1:
            fprintf( f, "%s\n", CONV_TO_UTF8( InitNetDescLine ) );
            fprintf( f, "%s       %s   %.4s\n",
                     CONV_TO_UTF8( StartNetDesc ),
                     CONV_TO_UTF8( refstr ),
                     (char*) &ObjNet[ii].m_PinNum );
            print_ter++;
            break;

        default:
            fprintf( f, "            %s   %.4s\n",
                     CONV_TO_UTF8( refstr ),
                     (char*) &ObjNet[ii].m_PinNum );
            break;
        }

        ObjNet[ii].m_Flag = 1;

        // Recherche des pins redondantes et mise a 1 de m_Flag,
        //	pour ne pas generer plusieurs fois la connexion
        for( jj = ii + 1; jj < g_NbrObjNet; jj++ )
        {
            if( ObjNet[jj].GetNet() != NetCode )
                break;
            if( ObjNet[jj].m_Type != NET_PIN )
                continue;
            SCH_COMPONENT* tstcmp =
                (SCH_COMPONENT*) ObjNet[jj].m_Link;
            wxString p = Cmp->GetPath( &( ObjNet[ii].m_SheetList ) );
            wxString tstp = tstcmp->GetPath( &( ObjNet[jj].m_SheetList ) );
            if( p.Cmp(tstp) != 0 )
                continue;

            if( ObjNet[jj].m_PinNum == ObjNet[ii].m_PinNum )
                ObjNet[jj].m_Flag = 1;
        }
    }
}
