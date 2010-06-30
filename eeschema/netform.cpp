/*****************************/
/* Net list generation code. */
/*****************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "class_library.h"
#include "class_pin.h"

#include "build_version.h"

/**
 * @bug - Every place in this file where fprintf() is used and the return
 *        is not checked is a bug.  The fprintf() function can fail and
 *        returns a value less than 0 when it does.
 */

static void Write_GENERIC_NetList( WinEDA_SchematicFrame* frame,
                                   const wxString&        FullFileName );
static void WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f,
                                bool with_pcbnew );
static void WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f );
static void WriteListOfNetsCADSTAR( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );
static void WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f,
                                bool use_netnames );

static void WriteGENERICListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );
static void AddPinToComponentPinList( SCH_COMPONENT*  Component,
                                      SCH_SHEET_PATH* sheet,
                                      LIB_PIN*        PinEntry );
static void FindAllsInstancesOfComponent( SCH_COMPONENT*  Component,
                                          LIB_COMPONENT*  Entry,
                                          SCH_SHEET_PATH* Sheet_in );
static bool SortPinsByNum( NETLIST_OBJECT* Pin1, NETLIST_OBJECT* Pin2 );
static void EraseDuplicatePins( NETLIST_OBJECT_LIST& aPinList );

static void ClearUsedFlags( void );


static NETLIST_OBJECT_LIST s_SortedComponentPinList;

// list of references already found for multi part per packages components
// (used to avoid to used more than one time a component)
static wxArrayString s_ReferencesAlreadyFound;


/* Create the netlist file ( Format is given by frame->m_NetlistFormat )
 *  bool use_netnames is used only for Spice netlist
 */
void WriteNetList( WinEDA_SchematicFrame* frame, const wxString& FileNameNL,
                   bool use_netnames )
{
    FILE* f = NULL;

    if( frame->m_NetlistFormat < NET_TYPE_CUSTOM1 )
    {
        if( ( f = wxFopen( FileNameNL, wxT( "wt" ) ) ) == NULL )
        {
            wxString msg = _( "Failed to create file " ) + FileNameNL;
            DisplayError( frame, msg );
            return;
        }
    }

    wxBusyCursor Busy;

    switch( frame->m_NetlistFormat )
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


/*  Find a "suitable" component from the DrawList
 *  build its pin list s_SortedComponentPinList.
 *  The list is sorted by pin num
 *  A suitable component is a "new" real component (power symbols are not
 *  considered)
 *  Must be deallocated by the user
 */
static SCH_COMPONENT* FindNextComponentAndCreatPinList( EDA_BaseStruct* DrawList,
                                                        SCH_SHEET_PATH* sheet )
{
    SCH_COMPONENT* Component = NULL;
    LIB_COMPONENT* Entry;
    LIB_PIN*       Pin;

    s_SortedComponentPinList.clear();
    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        if( DrawList->Type() != TYPE_SCH_COMPONENT )
            continue;
        Component = (SCH_COMPONENT*) DrawList;

        /* Power symbol and other component which have the reference starting
         * by "#" are not included in netlist (pseudo or virtual components) */
        wxString str = Component->GetRef( sheet );
        if( str[0] == '#' )  // ignore it
            continue;

        //if( Component->m_FlagControlMulti == 1 )
        //    continue;                                      /* yes */
        // removed because with multiple instances of one schematic
        // (several sheets pointing to 1 screen), this will be erroneously be
        // toggled.

        Entry = CMP_LIBRARY::FindLibraryComponent( Component->m_ChipName );

        if( Entry  == NULL )
            continue;

        // Multi parts per package: test if already visited:
        if( Entry->GetPartCount() > 1 )
        {
            bool found = false;
            for( unsigned jj = 0;
                 jj < s_ReferencesAlreadyFound.GetCount();
                 jj++ )
            {
                if( str == s_ReferencesAlreadyFound[jj] )  // Already visited
                {
                    found = true;
                    break;
                }
            }

            if( found )
                continue;
            else
            {
                s_ReferencesAlreadyFound.Add( str );  // Mark as visited
            }
        }

        if( Entry->GetPartCount() <= 1 )   // One part per package
        {
            LIB_PIN_LIST pins;

            Entry->GetPins( pins, Component->GetUnitSelection( sheet ), Component->m_Convert );

            for( size_t i = 0; i < pins.size(); i++ )
            {
                Pin = pins[i];

                wxASSERT( Pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                AddPinToComponentPinList( Component, sheet, Pin );
            }
        }
        else  // Multiple parts per package: Collect all parts ans pins for
              // this reference
            FindAllsInstancesOfComponent( Component, Entry, sheet );

        /* Sort Pins in s_SortedComponentPinList by pin number */
        sort( s_SortedComponentPinList.begin(),
              s_SortedComponentPinList.end(), SortPinsByNum );

        /* Remove duplicate Pins in s_SortedComponentPinList */
        EraseDuplicatePins( s_SortedComponentPinList );

        return Component;
    }

    return NULL;
}


/* Return the net name for the pin Pin.
 *  Net name is:
 *  "?" if pin not connected
 *  "netname" for global net (like gnd, vcc ..
 *  "netname_sheetnumber" for the usual nets
 */
static wxString ReturnPinNetName( NETLIST_OBJECT* Pin, const wxString& DefaultFormatNetname )
{
    int      netcode = Pin->GetNet();
    wxString NetName;

    if( ( netcode == 0 ) || ( Pin->m_FlagOfConnection != PAD_CONNECT ) )
        return NetName;

    NETLIST_OBJECT* netref = Pin->m_NetNameCandidate;
    if( netref )
        NetName = netref->m_Label;

    if( !NetName.IsEmpty() )
    {
        // prefix non global labels names by the sheet path, to avoid names collisions
        if( netref->m_Type != NET_PINLABEL )
        {
            wxString lnet = NetName;
            NetName = netref->m_SheetList.PathHumanReadable();

            // If sheet path is too long, use the time stamp name instead
            if( NetName.Length() > 32 )
                NetName = netref->m_SheetList.Path();
            NetName += lnet;
        }
    }
    else
    {
        NetName.Printf( DefaultFormatNetname.GetData(), netcode );
    }

    return NetName;
}


/* Create a generic netlist, and call an external netlister
 *  to change the netlist syntax and create the file
 */
void Write_GENERIC_NetList( WinEDA_SchematicFrame* frame, const wxString& FullFileName )
{
    wxString        Line, FootprintName;
    SCH_SHEET_PATH* sheet;
    EDA_BaseStruct* SchItem;
    SCH_COMPONENT*  Component;
    wxString        netname;
    FILE*           tmpfile;
    wxFileName      fn = FullFileName;

    fn.SetExt( wxT( "tmp" ) );

    if( ( tmpfile = wxFopen( fn.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        wxString msg = _( "Failed to create file " ) + fn.GetFullPath();
        DisplayError( frame, msg );
        return;
    }

    ClearUsedFlags();   /* Reset the flags FlagControlMulti in all schematic
                         * files*/
    fprintf( tmpfile, "$BeginNetlist\n" );

    /* Create netlist module section */
    fprintf( tmpfile, "$BeginComponentList\n" );
    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( SchItem = sheet->LastDrawList(); SchItem != NULL; SchItem = SchItem->Next() )
        {
            SchItem = Component = FindNextComponentAndCreatPinList( SchItem, sheet );

            if( Component == NULL )
                break;  // No component left

            FootprintName.Empty();
            if( !Component->GetField( FOOTPRINT )->IsVoid() )
            {
                FootprintName = Component->GetField( FOOTPRINT )->m_Text;
                FootprintName.Replace( wxT( " " ), wxT( "_" ) );
            }

            fprintf( tmpfile, "\n$BeginComponent\n" );
            fprintf( tmpfile, "TimeStamp=%8.8lX\n", Component->m_TimeStamp );
            fprintf( tmpfile, "Footprint=%s\n", CONV_TO_UTF8( FootprintName ) );
            Line = wxT( "Reference=" ) + Component->GetRef( sheet ) + wxT( "\n" );
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fputs( CONV_TO_UTF8( Line ), tmpfile );

            Line = Component->GetField( VALUE )->m_Text;
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( tmpfile, "Value=%s\n", CONV_TO_UTF8( Line ) );

            Line = Component->m_ChipName;
            Line.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( tmpfile, "Libref=%s\n", CONV_TO_UTF8( Line ) );

            // Write pin list:
            fprintf( tmpfile, "$BeginPinList\n" );
            for( unsigned ii = 0; ii < s_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* Pin = s_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                netname = ReturnPinNetName( Pin, wxT( "$-%.6d" ) );
                if( netname.IsEmpty() )
                    netname = wxT( "?" );
                fprintf( tmpfile, "%.4s=%s\n", (char*) &Pin->m_PinNum, CONV_TO_UTF8( netname ) );
            }

            fprintf( tmpfile, "$EndPinList\n" );
            fprintf( tmpfile, "$EndComponent\n" );
        }
    }

    fprintf( tmpfile, "$EndComponentList\n" );

    fprintf( tmpfile, "\n$BeginNets\n" );
    WriteGENERICListOfNets( tmpfile, g_NetObjectslist );
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

    CommandFile += wxT( " " ) + fn.GetFullPath();
    CommandFile += wxT( " " ) + FullFileName;

    ProcessExecute( CommandFile, wxEXEC_SYNC );
}


/* Clear flag list, used in netlist generation */
static void ClearUsedFlags( void )
{
    s_ReferencesAlreadyFound.Clear();
}


/* Routine generation of the netlist file (Format PSPICE)
 * = TRUE if use_netnames
 * Nodes are identified by the netname
 * If the nodes are identified by the netnumber
 *
 * All graphics text commentary by a [.-+] PSpice or [.-+] gnucap
 * Are considered in placing orders in the netlist
 * [.-] Or PSpice gnucap are beginning
 * + + Gnucap and PSpice are ultimately NetList
 */
static void WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f, bool use_netnames )
{
    char            Line[1024];
    SCH_SHEET_PATH* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT*  Component;
    int             nbitems;
    wxString        text;
    wxArrayString   SpiceCommandAtBeginFile, SpiceCommandAtEndFile;
    wxString        msg;

#define BUFYPOS_LEN 4
    wxChar          bufnum[BUFYPOS_LEN + 1];

    DateAndTime( Line );
    fprintf( f,
             "* %s (Spice format) creation date: %s\n\n",
             NETLIST_HEAD_STRING,
             Line );

    /* Create text list starting by [.-]pspice , or [.-]gnucap (simulator
     * commands) and create text list starting by [+]pspice , or [+]gnucap
     * (simulator commands) */
    bufnum[BUFYPOS_LEN] = 0;
    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst();
         sheet != NULL;
         sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList();
             DrawList != NULL;
             DrawList = DrawList->Next() )
        {
            wxChar ident;
            if( DrawList->Type() != TYPE_SCH_TEXT )
                continue;
            #define DRAWTEXT ( (SCH_TEXT*) DrawList )
            text = DRAWTEXT->m_Text; if( text.IsEmpty() )
                continue;
            ident = text.GetChar( 0 );
            if( ident != '.' && ident != '-' && ident != '+' )
                continue;
            text.Remove( 0, 1 );    // Remove the first char.
            text.Remove( 6 );       // text contains 6 char.
            text.MakeLower();
            if( ( text == wxT( "pspice" ) ) || ( text == wxT( "gnucap" ) ) )
            {
                /* Put the Y position as an ascii string, for sort by vertical
                 * position, using usual sort string by alphabetic value */
                int ypos = DRAWTEXT->m_Pos.y;
                for( int ii = 0; ii < BUFYPOS_LEN; ii++ )
                {
                    bufnum[BUFYPOS_LEN - 1 -
                           ii] = (ypos & 63) + ' '; ypos >>= 6;
                }

                text = DRAWTEXT->m_Text.AfterFirst( ' ' );
                // First BUFYPOS_LEN char are the Y position.
                msg.Printf( wxT( "%s %s" ), bufnum, text.GetData() );
                if( ident == '+' )
                    SpiceCommandAtEndFile.Add( msg );
                else
                    SpiceCommandAtBeginFile.Add( msg );
            }
        }
    }

    /* Print texts starting by [.-]pspice , ou [.-]gnucap (of course, without
     * the Y position string)*/
    nbitems = SpiceCommandAtBeginFile.GetCount();
    if( nbitems )
    {
        SpiceCommandAtBeginFile.Sort();
        for( int ii = 0; ii < nbitems; ii++ )
        {
            SpiceCommandAtBeginFile[ii].Remove( 0, BUFYPOS_LEN );
            SpiceCommandAtBeginFile[ii].Trim( TRUE );
            SpiceCommandAtBeginFile[ii].Trim( FALSE );
            fprintf( f, "%s\n", CONV_TO_UTF8( SpiceCommandAtBeginFile[ii] ) );
        }
    }
    fprintf( f, "\n" );


    /* Create component list */
    ClearUsedFlags();  /* Reset the flags FlagControlMulti in all schematic
                        * files*/
    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet );
            if( Component == NULL )
                break;

            fprintf( f, "%s ", CONV_TO_UTF8( Component->GetRef( sheet ) ) );

            // Write pin list:
            for( unsigned ii = 0; ii < s_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* Pin = s_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                wxString NetName = ReturnPinNetName( Pin, wxT( "N-%.6d" ) );
                if( NetName.IsEmpty() )
                    NetName = wxT( "?" );
                if( use_netnames )
                    fprintf( f, " %s", CONV_TO_UTF8( NetName ) );
                else    // Use number for net names (with net number = 0 for
                        // "GND"
                {
                    // NetName = "0" is "GND" net for Spice
                    if( NetName == wxT( "0" ) || NetName == wxT( "GND" ) )
                        fprintf( f, " 0" );
                    else
                        fprintf( f, " %d", Pin->GetNet() );
                }
            }

            fprintf( f, " %s\n",
                     CONV_TO_UTF8( Component->GetField( VALUE )->m_Text ) );
        }
    }

    s_SortedComponentPinList.clear();

    /* Print texts starting by [+]pspice , ou [+]gnucap */
    nbitems = SpiceCommandAtEndFile.GetCount();
    if( nbitems )
    {
        fprintf( f, "\n" );
        SpiceCommandAtEndFile.Sort();
        for( int ii = 0; ii < nbitems; ii++ )
        {
            SpiceCommandAtEndFile[ii].Remove( 0, +BUFYPOS_LEN );
            SpiceCommandAtEndFile[ii].Trim( TRUE );
            SpiceCommandAtEndFile[ii].Trim( FALSE );
            fprintf( f, "%s\n", CONV_TO_UTF8( SpiceCommandAtEndFile[ii] ) );
        }
    }

    fprintf( f, "\n.end\n" );
}


/* Generate net list file (Format 2 improves ORCAD PCB)
 * = TRUE if with_pcbnew
 * Format Pcbnew (OrcadPcb2 + reviews and lists of net)
 * = FALSE if with_pcbnew
 * Format ORCADPCB2 strict
 */
static void WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f, bool with_pcbnew )
{
    wxString Line, FootprintName;
    char Buf[256];
    SCH_SHEET_PATH* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT*  Component;
    OBJ_CMP_TO_LIST* CmpList = NULL;
    int CmpListCount = 0, CmpListSize = 1000;

    DateAndTime( Buf );
    if( with_pcbnew )
        fprintf( f, "# %s created  %s\n(\n", NETLIST_HEAD_STRING, Buf );
    else
        fprintf( f, "( { %s created  %s }\n", NETLIST_HEAD_STRING, Buf );


    /* Create netlist module section */
    ClearUsedFlags();   /* Reset the flags FlagControlMulti in all schematic
                         * files*/

    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet );
            if( Component == NULL )
                break;

            /* Get the Component FootprintFilter and put the component in
             * CmpList if filter is not void */
            LIB_COMPONENT* Entry =
                CMP_LIBRARY::FindLibraryComponent( Component->m_ChipName );

            if( Entry != NULL )
            {
                if( Entry->m_FootprintList.GetCount() != 0 ) /* Put in list */
                {
                    if( CmpList == NULL )
                    {
                        CmpList = (OBJ_CMP_TO_LIST*)
                                  MyZMalloc( sizeof(OBJ_CMP_TO_LIST) * CmpListSize );
                    }
                    if( CmpListCount >= CmpListSize )
                    {
                        CmpListSize += 1000;
                        CmpList =
                            (OBJ_CMP_TO_LIST*) realloc( CmpList,
                                                        sizeof(OBJ_CMP_TO_LIST)
                                                        * CmpListSize );
                    }
                    CmpList[CmpListCount].m_RootCmp = Component;
                    strcpy( CmpList[CmpListCount].m_Reference,
                            Component->GetRef( sheet ).mb_str() );
                    CmpListCount++;
                }
            }

            if( !Component->GetField( FOOTPRINT )->IsVoid() )
            {
                FootprintName = Component->GetField( FOOTPRINT )->m_Text;
                FootprintName.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                FootprintName = wxT( "$noname" );

            Line = Component->GetRef( sheet );
            fprintf( f, " ( %s %s",
                     CONV_TO_UTF8( Component->GetPath( sheet ) ),
                     CONV_TO_UTF8( FootprintName ) );
            fprintf( f, "  %s", CONV_TO_UTF8( Line ) );

            Line = Component->GetField( VALUE )->m_Text;
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
            for( unsigned ii = 0; ii < s_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* Pin = s_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                wxString netname = ReturnPinNetName( Pin, wxT( "N-%.6d" ) );
                if( netname.IsEmpty() )
                    netname = wxT( "?" );
                netname.Replace( wxT( " " ), wxT( "_" ) );

                fprintf( f, "  ( %4.4s %s )\n", (char*) &Pin->m_PinNum,
                         CONV_TO_UTF8( netname ) );
            }

            fprintf( f, " )\n" );
        }
    }

    fprintf( f, ")\n*\n" );

    s_SortedComponentPinList.clear();

    /* Write the allowed footprint list for each component */
    if( with_pcbnew && CmpList )
    {
        fprintf( f, "{ Allowed footprints by component:\n" );
        LIB_COMPONENT* Entry;
        for( int ii = 0; ii < CmpListCount; ii++ )
        {
            Component = CmpList[ii].m_RootCmp;
            Entry = CMP_LIBRARY::FindLibraryComponent( Component->m_ChipName );

            //Line.Printf(_("%s"), CmpList[ii].m_Ref);
            //Line.Replace( wxT( " " ), wxT( "_" ) );
            for( unsigned nn = 0;
                 nn<sizeof(CmpList[ii].m_Reference)
                 && CmpList[ii].m_Reference[nn];
                 nn++ )
            {
                if( CmpList[ii].m_Reference[nn] == ' ' )
                    CmpList[ii].m_Reference[nn] = '_';
            }

            fprintf( f, "$component %s\n", CmpList[ii].m_Reference );
            /* Write the footprint list */
            for( unsigned jj = 0; jj < Entry->m_FootprintList.GetCount(); jj++ )
            {
                fprintf( f, " %s\n",
                         CONV_TO_UTF8( Entry->m_FootprintList[jj] ) );
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
        WriteGENERICListOfNets( f, g_NetObjectslist );
        fprintf( f, "}\n" );
        fprintf( f, "#End\n" );
    }
}


/*
 * Add a new pin description in the pin list s_SortedComponentPinList
 * a pin description is a pointer to the corresponding structure
 * created by BuildNetList() in the table g_NetObjectslist
 */
static void AddPinToComponentPinList( SCH_COMPONENT* Component,
                                      SCH_SHEET_PATH* sheetlist, LIB_PIN* Pin )
{
    /* Search the PIN description for Pin in g_NetObjectslist*/
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        if( g_NetObjectslist[ii]->m_Type != NET_PIN )
            continue;
        if( g_NetObjectslist[ii]->m_Link != Component )
            continue;
        if( g_NetObjectslist[ii]->m_SheetList != *sheetlist )
            continue;
        if( g_NetObjectslist[ii]->m_PinNum != Pin->m_PinNum )
            continue;

        s_SortedComponentPinList.push_back( g_NetObjectslist[ii] );
        if( s_SortedComponentPinList.size() >= MAXPIN )
        {
            /* Log message for Internal error */
            DisplayError( NULL, wxT( "AddPinToComponentPinList err: MAXPIN reached" ) );
            return;
        }
    }
}


/** Function EraseDuplicatePins
 *  Function to remove duplicate Pins in the TabPin pin list
 *  (This is a list of pins found in the whole schematic, for a given
 * component)
 *  These duplicate pins were put in list because some pins (powers... )
 *  are found more than one time when we have a multiple parts per package
 * component
 *  for instance, a 74ls00 has 4 parts, and therefore the VCC pin and GND pin
 * appears 4 times
 *  in the list.
 * @param aPinList = a NETLIST_OBJECT_LIST that contains the list of pins for a
 * given component.
 * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
 */
static void EraseDuplicatePins( NETLIST_OBJECT_LIST& aPinList )
{
    if( aPinList.size() == 0 )  // Trivial case: component with no pin
        return;

    for( unsigned ii = 0; ii < aPinList.size(); ii++ )
    {
        if( aPinList[ii] == NULL ) /* already deleted */
            continue;

        /* Search for duplicated pins
         * If found, remove duplicates. The priority is to keep connected pins
         * and remove unconnected
         * - So this allows (for instance when using multi op amps per package
         * - to connect only one op amp to power
         * Because the pin list is sorted by m_PinNum value, duplicated pins
         * are necessary successive in list
         */
        int idxref = ii;
        for( unsigned jj = ii + 1; jj < aPinList.size(); jj++ )
        {
            if(  aPinList[jj] == NULL )   // Already removed
                continue;
            // other pin num end of duplicate list.
            if( aPinList[idxref]->m_PinNum != aPinList[jj]->m_PinNum )
                break;
            if( aPinList[idxref]->m_FlagOfConnection == PAD_CONNECT )
                aPinList[jj] = NULL;
            else /* the reference pin is not connected: remove this pin if the
                  * other pin is connected */
            {
                if( aPinList[jj]->m_FlagOfConnection == PAD_CONNECT )
                {
                    aPinList[idxref] = NULL;
                    idxref = jj;
                }
                else    // the 2 pins are not connected: remove the tested pin,
                        // and continue ...
                    aPinList[jj] = NULL;
            }
        }
    }
}


/**
 * Used for multiple parts per package components.
 *
 * Search all instances of Component_in,
 * Calls AddPinToComponentPinList() to and pins founds to the current
 * component pin list
 */
static void FindAllsInstancesOfComponent( SCH_COMPONENT*  Component_in,
                                          LIB_COMPONENT*  Entry,
                                          SCH_SHEET_PATH* Sheet_in )
{
    EDA_BaseStruct* SchItem;
    SCH_COMPONENT* Component2;
    LIB_PIN* pin;
    SCH_SHEET_PATH* sheet;
    wxString str, Reference = Component_in->GetRef( Sheet_in );

    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( SchItem = sheet->LastDrawList(); SchItem; SchItem = SchItem->Next() )
        {
            if( SchItem->Type() != TYPE_SCH_COMPONENT )
                continue;

            Component2 = (SCH_COMPONENT*) SchItem;

            str = Component2->GetRef( sheet );
            if( str.CmpNoCase( Reference ) != 0 )
                continue;

            if( Entry == NULL )
                continue;

            for( pin = Entry->GetNextPin(); pin != NULL; pin = Entry->GetNextPin( pin ) )
            {
                wxASSERT( pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                if( pin->m_Unit
                   && ( pin->m_Unit != Component2->GetUnitSelection( sheet ) ) )
                    continue;

                if( pin->m_Convert
                   && ( pin->m_Convert != Component2->m_Convert ) )
                    continue;

                // A suitable pin in found: add it to the current list
                AddPinToComponentPinList( Component2, sheet, pin );
            }
        }
    }
}


/*
 * Comparison routine for sorting by pin numbers.
 */
static bool SortPinsByNum( NETLIST_OBJECT* Pin1, NETLIST_OBJECT* Pin2 )
{
    int Num1, Num2;
    char Line[5];

    Num1    = Pin1->m_PinNum;
    Num2    = Pin2->m_PinNum;
    Line[4] = 0;
    memcpy( Line, &Num1, 4 ); Num1 = atoi( Line );
    memcpy( Line, &Num2, 4 ); Num2 = atoi( Line );
    return Num1 < Num2;
}


/* Written in the file / net list (ranked by Netcode), and elements that are
 * connected
 */
static void WriteGENERICListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
{
    int NetCode, LastNetCode = -1;
    int SameNetcodeCount = 0;
    SCH_COMPONENT* Cmp;
    wxString NetName, CmpRef;
    wxString NetcodeName;
    char FirstItemInNet[1024];

    for( unsigned ii = 0; ii < aObjectsList.size(); ii++ )
    {
        // New net found, write net id;
        if( ( NetCode = aObjectsList[ii]->GetNet() ) != LastNetCode )
        {
            SameNetcodeCount = 0;              // Items count for this net
            NetName.Empty();

            // Find a label (if exists) for this net.
            NETLIST_OBJECT* netref;
            netref = aObjectsList[ii]->m_NetNameCandidate;
            if( netref )
                NetName = netref->m_Label;

            NetcodeName.Printf( wxT( "Net %d " ), NetCode );
            NetcodeName += wxT( "\"" );
            if( !NetName.IsEmpty() )
            {
                if( netref->m_Type != NET_PINLABEL )
                {
                    // usual net name, prefix it by the sheet path
                    NetcodeName += netref->m_SheetList.PathHumanReadable();
                }
                NetcodeName += NetName;
            }
            NetcodeName += wxT( "\"" );

            // Add the netname without prefix, in cases we need only the
            // "short" netname
            NetcodeName += wxT( " \"" ) + NetName + wxT( "\"" );
            LastNetCode  = NetCode;
        }

        if( aObjectsList[ii]->m_Type != NET_PIN )
            continue;

        Cmp = (SCH_COMPONENT*) aObjectsList[ii]->m_Link;

        // Get the reference for the net name and the main parent component
        CmpRef = Cmp->GetRef( &aObjectsList[ii]->m_SheetList );
        if( CmpRef.StartsWith( wxT( "#" ) ) )
            continue;                 // Pseudo component (Like Power symbol)

        // Print the pin list for this net, if  2 or more items are connected:
        SameNetcodeCount++;
        if( SameNetcodeCount == 1 )     /* first item for this net found,
                                         * Print this connection, when a
                                         * second item will be found */
        {
            sprintf( FirstItemInNet, " %s %.4s\n", CONV_TO_UTF8( CmpRef ),
                     (const char*) &aObjectsList[ii]->m_PinNum );
        }

        // Second item for this net found, Print the Net name, and the
        // first item
        if( SameNetcodeCount == 2 )
        {
            fprintf( f, "%s\n", CONV_TO_UTF8( NetcodeName ) );
            fputs( FirstItemInNet, f );
        }

        if( SameNetcodeCount >= 2 )
            fprintf( f, " %s %.4s\n", CONV_TO_UTF8( CmpRef ),
                     (const char*) &aObjectsList[ii]->m_PinNum );
    }
}


/* Generate CADSTAR net list. */
wxString StartLine( wxT( "." ) );


/* Routine generation of the netlist file (CADSTAR Format)
 * Header:
 * HEA ..
 * TIM .. 2004 07 29 16 22 17
 * APA .. "Cadstar RINF Output - Version 6.0.2.3"
 * INCH UNI .. 1000.0 in
 * FULL TYP ..
 *
 * List of components:
 * .. ADD_COM X1 "CNT D41612 (48pts CONTOUR TM)"
 * .. ADD_COM U2 "74HCT245D" "74HCT245D"
 *
 * Connections:
 *  .. ADD_TER RR2 * 6 "$ 42"
 * .. B U1 100
 * 6 CA
 *
 * ADD_TER .. U2 * 6 "$ 59"
 * .. B * U7 39
 * U6 17
 * U1 * 122
 *
 * .. ADD_TER P2 * 1 "$ 9"
 * .. B * T3 1
 *U1 * 14
 */
static void WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f )
{
    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    wxString FootprintName;
    char Line[1024];
    SCH_SHEET_PATH* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* Component;
    wxString Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

    fprintf( f, "%sHEA\n", CONV_TO_UTF8( StartLine ) );
    DateAndTime( Line );
    fprintf( f, "%sTIM %s\n", CONV_TO_UTF8( StartLine ), Line );
    fprintf( f, "%sAPP ", CONV_TO_UTF8( StartLine ) );
    fprintf( f, "\"%s\"\n", CONV_TO_UTF8( Title ) );
    fprintf( f, "\n" );

    /* Create netlist module section */
    ClearUsedFlags();   /* Reset the flags FlagControlMulti in all schematic
                         *files*/
    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = Component = FindNextComponentAndCreatPinList( DrawList, sheet );
            if( Component == NULL )
                break;

            if( !Component->GetField( FOOTPRINT )->IsVoid() )
            {
                FootprintName = Component->GetField( FOOTPRINT )->m_Text;
                FootprintName.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                FootprintName = wxT( "$noname" );

            msg = Component->GetRef( sheet );
            fprintf( f, "%s     ", CONV_TO_UTF8( StartCmpDesc ) );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );

            msg = Component->GetField( VALUE )->m_Text;
            msg.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( f, "     \"%s\"", CONV_TO_UTF8( msg ) );
            fprintf( f, "\n" );
        }
    }

    fprintf( f, "\n" );

    s_SortedComponentPinList.clear();

    WriteListOfNetsCADSTAR( f, g_NetObjectslist );

    fprintf( f, "\n%sEND\n", CONV_TO_UTF8( StartLine ) );
}


/*
 * Written in the file / net list (ranked by Netcode), and
 * Pins connected to it
 * Format:
 *. ADD_TER RR2 6 "$ 42"
 *. B U1 100
 * 6 CA
 */
static void WriteListOfNetsCADSTAR( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
{
    wxString InitNetDesc  = StartLine + wxT( "ADD_TER" );
    wxString StartNetDesc = StartLine + wxT( "TER" );
    wxString NetcodeName, InitNetDescLine;
    unsigned ii;
    int print_ter = 0;
    int NetCode, LastNetCode = -1;
    SCH_COMPONENT* Cmp;
    wxString NetName;

    for( ii = 0; ii < aObjectsList.size(); ii++ )
        aObjectsList[ii]->m_Flag = 0;

    for( ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        // Get the NetName of the current net :
        if( ( NetCode = aObjectsList[ii]->GetNet() ) != LastNetCode )
        {
            NetName.Empty();

            NETLIST_OBJECT* netref;
            netref = aObjectsList[ii]->m_NetNameCandidate;
            if( netref )
                NetName = netref->m_Label;

            NetcodeName = wxT( "\"" );
            if( !NetName.IsEmpty() )
            {
                if( netref->m_Type != NET_PINLABEL )
                {
                    // usual net name, prefix it by the sheet path
                    NetcodeName +=
                        netref->m_SheetList.PathHumanReadable();
                }
                NetcodeName += NetName;
            }
            else  // this net has no name: create a default name $<net number>
                NetcodeName << wxT( "$" ) << NetCode;
            NetcodeName += wxT( "\"" );
            LastNetCode  = NetCode;
            print_ter    = 0;
        }


        if( aObjectsList[ii]->m_Type != NET_PIN )
            continue;

        if( aObjectsList[ii]->m_Flag != 0 )
            continue;

        Cmp = (SCH_COMPONENT*) aObjectsList[ii]->m_Link;
        wxString refstr = Cmp->GetRef( &(aObjectsList[ii]->m_SheetList) );
        if( refstr[0] == '#' )
            continue;  // Power supply symbols.

        switch( print_ter )
        {
        case 0:
        {
            char buf[5];
            wxString str_pinnum;
            strncpy( buf, (char*) &aObjectsList[ii]->m_PinNum, 4 );
            buf[4]     = 0;
            str_pinnum = CONV_FROM_UTF8( buf );
            InitNetDescLine.Printf( wxT( "\n%s   %s   %.4s     %s" ),
                                    GetChars(InitNetDesc),
                                    GetChars(refstr),
                                    GetChars(str_pinnum),
                                    GetChars(NetcodeName) );
        }
            print_ter++;
            break;

        case 1:
            fprintf( f, "%s\n", CONV_TO_UTF8( InitNetDescLine ) );
            fprintf( f, "%s       %s   %.4s\n",
                     CONV_TO_UTF8( StartNetDesc ),
                     CONV_TO_UTF8( refstr ),
                     (char*) &aObjectsList[ii]->m_PinNum );
            print_ter++;
            break;

        default:
            fprintf( f, "            %s   %.4s\n",
                     CONV_TO_UTF8( refstr ),
                     (char*) &aObjectsList[ii]->m_PinNum );
            break;
        }

        aObjectsList[ii]->m_Flag = 1;

        // Search for redundant pins to avoid generation of the same connection
        // more than once.
        for( unsigned jj = ii + 1; jj < aObjectsList.size(); jj++ )
        {
            if( aObjectsList[jj]->GetNet() != NetCode )
                break;
            if( aObjectsList[jj]->m_Type != NET_PIN )
                continue;
            SCH_COMPONENT* tstcmp =
                (SCH_COMPONENT*) aObjectsList[jj]->m_Link;
            wxString p    = Cmp->GetPath( &( aObjectsList[ii]->m_SheetList ) );
            wxString tstp = tstcmp->GetPath( &( aObjectsList[jj]->m_SheetList ) );
            if( p.Cmp( tstp ) != 0 )
                continue;

            if( aObjectsList[jj]->m_PinNum == aObjectsList[ii]->m_PinNum )
                aObjectsList[jj]->m_Flag = 1;
        }
    }
}
