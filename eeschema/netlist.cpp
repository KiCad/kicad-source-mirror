/*****************/
/*  netlist.cpp  */
/*****************/

#include "fctsys.h"
#include "common.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "class_library.h"
#include "lib_pin.h"
#include "sch_junction.h"
#include "sch_component.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_text.h"
#include "sch_sheet.h"
#include "algorithm"

#include <boost/foreach.hpp>


// Buffer to build the list of items used in netlist and erc calculations
NETLIST_OBJECT_LIST g_NetObjectslist;

//#define NETLIST_DEBUG

static void PropageNetCode( int OldNetCode, int NewNetCode, int IsBus );
static void SheetLabelConnect( NETLIST_OBJECT* SheetLabel );
static void AddConnectedObjects( SCH_SHEET_PATH*      sheetlist,
                                 NETLIST_OBJECT_LIST& aNetItemBuffer );
static int  ConvertBusToMembers( NETLIST_OBJECT_LIST& aNetItemBuffer, NETLIST_OBJECT& ObjNet );
static void PointToPointConnect( NETLIST_OBJECT* Ref, int IsBus, int start );
static void SegmentToPointConnect( NETLIST_OBJECT* Jonction, int IsBus, int start );
static void LabelConnect( NETLIST_OBJECT* Label );
static void ConnectBusLabels( NETLIST_OBJECT_LIST& aNetItemBuffer );
static void SetUnconnectedFlag( NETLIST_OBJECT_LIST& aNetItemBuffer );

static void FindBestNetNameForEachNet( NETLIST_OBJECT_LIST& aNetItemBuffer );
static NETLIST_OBJECT* FindBestNetName( NETLIST_OBJECT_LIST& aLabelItemBuffer );

// Sort functions used here:
static bool SortItemsbyNetcode( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 );
static bool SortItemsBySheet( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 );

// Local variables
static int FirstNumWireBus, LastNumWireBus, RootBusNameLength;
static int LastNetCode, LastBusNetCode;


#if defined(DEBUG)
void dumpNetTable()
{
    for( unsigned idx = 0; idx < g_NetObjectslist.size(); ++idx )
    {
        g_NetObjectslist[idx]->Show( std::cout, idx );
    }
}


#endif


/*
 * Routine to free memory used to calculate the netlist TabNetItems = pointer
 * to the main table (list items)
 */
void FreeNetObjectsList( NETLIST_OBJECT_LIST& aNetObjectsBuffer )
{
    for( unsigned i = 0; i < aNetObjectsBuffer.size(); i++ )
        delete aNetObjectsBuffer[i];

    aNetObjectsBuffer.clear();
}


/*
 * Build net list connection table.
 *
 * Updates:
 *   g_NetObjectslist
 */
void SCH_EDIT_FRAME::BuildNetListBase()
{
    int             NetNumber;
    int             NetCode;
    SCH_SHEET_PATH* sheet;
    wxString        msg, activity;
    wxBusyCursor    Busy;

    NetNumber = 1;

    activity = _( "Building net list:" );
    SetStatusText( activity );

    FreeNetObjectsList( g_NetObjectslist );

    /* Build the sheet (not screen) list (flattened)*/
    SCH_SHEET_LIST SheetListList;

    /* Fill g_NetObjectslist with items used in connectivity calculation */

    sheet = SheetListList.GetFirst();
    for( ; sheet != NULL; sheet = SheetListList.GetNext() )
        AddConnectedObjects( sheet, g_NetObjectslist );

    if( g_NetObjectslist.size() == 0 )
        return;  // no objects

    activity << wxT( " " ) << _( "net count =" ) << wxT( " " ) << g_NetObjectslist.size();
    SetStatusText( activity );

    /* Sort objects by Sheet */

    sort( g_NetObjectslist.begin(), g_NetObjectslist.end(), SortItemsBySheet );

    activity << wxT( ",  " ) << _( "connections" ) << wxT( "..." );
    SetStatusText( activity );

    sheet = &(g_NetObjectslist[0]->m_SheetList);
    LastNetCode = LastBusNetCode = 1;

    for( unsigned ii = 0, istart = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        NETLIST_OBJECT* net_item = g_NetObjectslist[ii];
        if( net_item->m_SheetList != *sheet )   // Sheet change
        {
            sheet  = &(net_item->m_SheetList);
            istart = ii;
        }

        switch( net_item->m_Type )
        {
        case NET_ITEM_UNSPECIFIED:
            wxMessageBox( wxT( "BuildNetListBase() error" ) );
            break;

        case NET_PIN:
        case NET_PINLABEL:
        case NET_SHEETLABEL:
        case NET_NOCONNECT:
            if( net_item->GetNet() != 0 )
                break;

        case NET_SEGMENT:
            /* Control connections point to point type without bus.  */
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( LastNetCode );
                LastNetCode++;
            }
            PointToPointConnect( net_item, 0, istart );
            break;

        case NET_JONCTION:
            /* Control of the junction outside BUS. */
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( LastNetCode );
                LastNetCode++;
            }
            SegmentToPointConnect( net_item, 0, istart );

            /* Control of the junction, on BUS. */
            if( net_item->m_BusNetCode == 0 )
            {
                net_item->m_BusNetCode = LastBusNetCode;
                LastBusNetCode++;
            }
            SegmentToPointConnect( net_item, ISBUS, istart );
            break;

        case NET_LABEL:
        case NET_HIERLABEL:
        case NET_GLOBLABEL:
            /* Control connections type junction without bus. */
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( LastNetCode );
                LastNetCode++;
            }
            SegmentToPointConnect( net_item, 0, istart );
            break;

        case NET_SHEETBUSLABELMEMBER:
            if( net_item->m_BusNetCode != 0 )
                break;

        case NET_BUS:
            /* Control type connections point to point mode bus */
            if( net_item->m_BusNetCode == 0 )
            {
                net_item->m_BusNetCode = LastBusNetCode;
                LastBusNetCode++;
            }
            PointToPointConnect( net_item, ISBUS, istart );
            break;

        case NET_BUSLABELMEMBER:
        case NET_HIERBUSLABELMEMBER:
        case NET_GLOBBUSLABELMEMBER:
            /* Control connections similar has on BUS */
            if( net_item->GetNet() == 0 )
            {
                net_item->m_BusNetCode = LastBusNetCode;
                LastBusNetCode++;
            }
            SegmentToPointConnect( net_item, ISBUS, istart );
            break;
        }
    }

#if defined(NETLIST_DEBUG) && defined(DEBUG)
    std::cout << "\n\nafter sheet local\n\n";
    dumpNetTable();
#endif


    activity << _( "done" );
    SetStatusText( activity );

    /* Updating the Bus Labels Netcode connected by Bus */
    ConnectBusLabels( g_NetObjectslist );

    activity << wxT( ",  " ) << _( "bus labels" ) << wxT( "..." );
    SetStatusText( activity );

    /* Group objects by label. */
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        switch( g_NetObjectslist[ii]->m_Type )
        {
        case NET_PIN:
        case NET_SHEETLABEL:
        case NET_SEGMENT:
        case NET_JONCTION:
        case NET_BUS:
        case NET_NOCONNECT:
            break;

        case NET_LABEL:
        case NET_GLOBLABEL:
        case NET_PINLABEL:
        case NET_BUSLABELMEMBER:
        case NET_GLOBBUSLABELMEMBER:
            LabelConnect( g_NetObjectslist[ii] );
            break;

        case NET_SHEETBUSLABELMEMBER:
        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
            break;

        case NET_ITEM_UNSPECIFIED:
            break;
        }
    }

#if defined(NETLIST_DEBUG) && defined(DEBUG)
    std::cout << "\n\nafter sheet global\n\n";
    dumpNetTable();
#endif

    activity << _( "done" );
    SetStatusText( activity );

    /* Connection hierarchy. */
    activity << wxT( ", " ) << _( "hierarchy..." );
    SetStatusText( activity );
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        if( g_NetObjectslist[ii]->m_Type == NET_SHEETLABEL
            || g_NetObjectslist[ii]->m_Type == NET_SHEETBUSLABELMEMBER )
            SheetLabelConnect( g_NetObjectslist[ii] );
    }

    /* Sort objects by NetCode */
    sort( g_NetObjectslist.begin(), g_NetObjectslist.end(), SortItemsbyNetcode );

#if defined(NETLIST_DEBUG) && defined(DEBUG)
    std::cout << "\n\nafter qsort()\n";
    dumpNetTable();
#endif

    activity << _( "done" );
    SetStatusText( activity );

    /* Compress numbers of Netcode having consecutive values. */
    LastNetCode = NetCode = 0;
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        if( g_NetObjectslist[ii]->GetNet() != LastNetCode )
        {
            NetCode++;
            LastNetCode = g_NetObjectslist[ii]->GetNet();
        }
        g_NetObjectslist[ii]->SetNet( NetCode );
    }

    /* Assignment of m_FlagOfConnection based connection or not. */
    SetUnconnectedFlag( g_NetObjectslist );

    /* find the best label object to give the best net name to each net */
    FindBestNetNameForEachNet( g_NetObjectslist );
}


/**
 * Function FindBestNetNameForEachNet
 * fill the .m_NetNameCandidate member of each item of aNetItemBuffer
 * with a reference to the "best" NETLIST_OBJECT usable to give a name to the net
 * If no suitable object found, .m_NetNameCandidate is filled with 0.
 * The "best" NETLIST_OBJECT is a NETLIST_OBJECT that have the type label
 * and by priority order:
 * the label is global or local
 * the label is in the first sheet in a hierarchy (the root sheet has the most priority)
 * alphabetic order.
 */
void FindBestNetNameForEachNet( NETLIST_OBJECT_LIST& aNetItemBuffer )
{
    if( aNetItemBuffer.size() == 0 )
        return; // Should not occur: if this function is called, obviously some items exist in list

    NETLIST_OBJECT_LIST candidates;
    int netcode = 0;            // current netcode for tested items
    unsigned idxstart = 0;       // index of the first item of this net
    for( unsigned ii = 0; ii <= aNetItemBuffer.size(); ii++ )
    {
        NETLIST_OBJECT* item;

        if( ii == aNetItemBuffer.size() ) // last item already found
            netcode = -2;
        else
            item = aNetItemBuffer[ii];
        if( netcode != item->GetNet() )      // End of net found
        {
            if( candidates.size() )         // One or more labels exists, find the best
            {
                NETLIST_OBJECT* bestlabel = FindBestNetName( candidates );
                for (unsigned jj = idxstart; jj < ii; jj++ )
                    aNetItemBuffer[jj]->m_NetNameCandidate = bestlabel;
            }
            if( netcode == -2 )
                break;
            netcode = item->GetNet();
            candidates.clear();
            idxstart = ii;
        }
        switch( item->m_Type )
        {
        case NET_HIERLABEL:
        case NET_LABEL:
        case NET_PINLABEL:
        case NET_GLOBLABEL:
            candidates.push_back( item );
            break;

        default:
            break;
        }
    }
}

/**
 * Function FindBestNetName
 * @return a reference to the "best" label that can be used to give a name
 *  to a net.
 * @param aLabelItemBuffer = list of NETLIST_OBJECT type labels candidates.
 *  labels are local labels, hierarchical labels or pin labels
 *   labels in included sheets have a lower priority than labels in the current sheet.
 *     so labels inside the root sheet have the higher priority.
 *   pin labels are global labels and have the higher priority
 *   local labels have the lower priority
 *   labels having the same priority are sorted by alphabetic order.
 *
 */
static NETLIST_OBJECT* FindBestNetName( NETLIST_OBJECT_LIST& aLabelItemBuffer )
{
    if( aLabelItemBuffer.size() == 0 )
        return NULL;

    // Define a priority (from low to high) to sort labels:
    // NET_PINLABEL and NET_GLOBLABEL are global labels
    // and priority >= NET_PRIO_MAX-1 is for global connections
    // ( i.e. for labels that are not prefixed by a sheetpath)
    #define NET_PRIO_MAX 4
    int priority_order[NET_PRIO_MAX+1] =
    { NET_ITEM_UNSPECIFIED, NET_LABEL, NET_HIERLABEL, NET_PINLABEL, NET_GLOBLABEL };

    NETLIST_OBJECT*item = aLabelItemBuffer[0];

    // Calculate item priority (initial priority)
    int item_priority = 0;
    for( unsigned ii = 0; ii <= NET_PRIO_MAX; ii++ )
    {
        if ( item->m_Type == priority_order[ii]  )
        {
            item_priority = ii;
            break;
        }
    }

    for( unsigned ii = 1; ii < aLabelItemBuffer.size(); ii++ )
    {
        NETLIST_OBJECT* candidate = aLabelItemBuffer[ii];
        // Calculate candidate priority
        int candidate_priority = 0;
        for( unsigned ii = 0; ii <= NET_PRIO_MAX; ii++ )
        {
            if ( candidate->m_Type == priority_order[ii]  )
            {
                candidate_priority = ii;
                break;
            }
        }
        if( candidate_priority > item_priority )
        {
            item = candidate;
            item_priority = candidate_priority;
        }
        else if( candidate_priority == item_priority )
        {
            // for global labels, we select the best candidate by alphabetic order
            // because they have no sheetpath as prefix name
            // for other labels, we select them before by sheet deep order
            // because the actual name is /sheetpath/label
            // and for a given path length, by alphabetic order

            if( item_priority >= NET_PRIO_MAX-1 )     // global label or pin label
            {   // selection by alphabetic order:
                if( candidate->m_Label.Cmp( item->m_Label ) < 0 )
                    item = candidate;
            }
            else    // not global: names are prefixed by their sheetpath
            {
                // use name defined in higher hierarchical sheet
                // (i.e. shorter path because paths are /<timestamp1>/<timestamp2>/...
                // and timestamp = 8 letters.
                if( candidate->m_SheetList.Path().Length() < item->m_SheetList.Path().Length() )
                {
                    item = candidate;
                }
                else if( candidate->m_SheetList.Path().Length() == item->m_SheetList.Path().Length() )
                {
                    // For labels on sheets having an equivalent deep in hierarchy, use
                    // alphabetic label name order:
                    if( candidate->m_Label.Cmp( item->m_Label ) < 0 )
                        item = candidate;
                }
            }
        }
    }

    return item;
}

/*
 * Connect sheets by sheetLabels
 */
static void SheetLabelConnect( NETLIST_OBJECT* SheetLabel )
{
    if( SheetLabel->GetNet() == 0 )
        return;

    /* Calculate the number of nodes in the corresponding sheetlabel */

    /* Comparison with SheetLabel GLABELS sub sheet to group Netcode */

    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        NETLIST_OBJECT* ObjetNet = g_NetObjectslist[ii];
        if( ObjetNet->m_SheetList != SheetLabel->m_SheetListInclude )
            continue;  //use SheetInclude, not the sheet!!

        if( (ObjetNet->m_Type != NET_HIERLABEL )
           && (ObjetNet->m_Type != NET_HIERBUSLABELMEMBER ) )
            continue;

        if( ObjetNet->GetNet() == SheetLabel->GetNet() )
            continue;  //already connected.

        if( ObjetNet->m_Label.CmpNoCase( SheetLabel->m_Label ) != 0 )
            continue;  //different names.

        /* Propagate Netcode having all the objects of the same Netcode. */
        if( ObjetNet->GetNet() )
            PropageNetCode( ObjetNet->GetNet(), SheetLabel->GetNet(), 0 );
        else
            ObjetNet->SetNet( SheetLabel->GetNet() );
    }
}


/**
 * Function AddConnectedObjects
 * Creates the list of objects related to connections (pins of components,
 * wires, labels, junctions ...)
 *
 * @param sheetlist: pointer to a sheetlist.
 * @param aNetItemBuffer: a std::vector to store pointer on NETLIST_OBJECT
 *                        created
 */
static void AddConnectedObjects( SCH_SHEET_PATH*               sheetlist,
                                 std::vector<NETLIST_OBJECT*>& aNetItemBuffer )
{
    int             ii;
    SCH_ITEM*       DrawList;
    NETLIST_OBJECT* new_item;
    SCH_COMPONENT*  DrawLibItem;
    LIB_COMPONENT*  Entry;
    SCH_SHEET_PATH  list;

    DrawList = sheetlist->LastScreen()->GetDrawItems();
    for( ; DrawList; DrawList = DrawList->Next() )
    {
        switch( DrawList->Type() )
        {
        case SCH_LINE_T:
            #undef STRUCT
            #define STRUCT ( (SCH_LINE*) DrawList )
            if( (STRUCT->GetLayer() != LAYER_BUS)
               && (STRUCT->GetLayer() != LAYER_WIRE) )
                break;

            new_item = new NETLIST_OBJECT();
            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp  = STRUCT;
            new_item->m_Start = STRUCT->m_Start;
            new_item->m_End   = STRUCT->m_End;

            if( STRUCT->GetLayer() == LAYER_BUS )
            {
                new_item->m_Type = NET_BUS;
            }
            else            /* WIRE */
            {
                new_item->m_Type = NET_SEGMENT;
            }
            aNetItemBuffer.push_back( new_item );
            break;

        case SCH_JUNCTION_T:
            #undef STRUCT
            #define STRUCT ( (SCH_JUNCTION*) DrawList )
            new_item = new NETLIST_OBJECT();

            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp  = STRUCT;
            new_item->m_Type  = NET_JONCTION;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;

            aNetItemBuffer.push_back( new_item );
            break;

        case SCH_NO_CONNECT_T:
            #undef STRUCT
            #define STRUCT ( (SCH_NO_CONNECT*) DrawList )
            new_item = new NETLIST_OBJECT();

            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp  = STRUCT;
            new_item->m_Type  = NET_NOCONNECT;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;

            aNetItemBuffer.push_back( new_item );
            break;

        case SCH_LABEL_T:
            #undef STRUCT
            #define STRUCT ( (SCH_LABEL*) DrawList )
            ii = IsBusLabel( STRUCT->m_Text );

            new_item = new NETLIST_OBJECT();
            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp = STRUCT;
            new_item->m_Type = NET_LABEL;

            if( STRUCT->GetLayer() ==  LAYER_GLOBLABEL )
                new_item->m_Type = NET_GLOBLABEL;
            if( STRUCT->GetLayer() ==  LAYER_HIERLABEL )
                new_item->m_Type = NET_HIERLABEL;

            new_item->m_Label = STRUCT->m_Text;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;

            aNetItemBuffer.push_back( new_item );
            /* If a bus connects to label */
            if( ii )
                ConvertBusToMembers( aNetItemBuffer, *new_item );


            break;

        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            #undef STRUCT
            #define STRUCT ( (SCH_LABEL*) DrawList )
            ii = IsBusLabel( STRUCT->m_Text );
            new_item = new NETLIST_OBJECT();
            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp = STRUCT;
            new_item->m_Type = NET_LABEL;

            // this is not the simplest way of doing it
            // (look at the case statement above).
            if( STRUCT->GetLayer() ==  LAYER_GLOBLABEL )
                new_item->m_Type = NET_GLOBLABEL;
            if( STRUCT->GetLayer() ==  LAYER_HIERLABEL )
                new_item->m_Type = NET_HIERLABEL;

            new_item->m_Label = STRUCT->m_Text;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;
            aNetItemBuffer.push_back( new_item );

            /* If a bus connects to label */
            if( ii )
                ConvertBusToMembers( aNetItemBuffer, *new_item );


            break;

        case SCH_COMPONENT_T:
            DrawLibItem = (SCH_COMPONENT*) DrawList;

            Entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->GetLibName() );
            if( Entry == NULL )
                break;

            for( LIB_PIN* pin = Entry->GetNextPin();  pin;  pin = Entry->GetNextPin( pin ) )
            {
                wxASSERT( pin->Type() == LIB_PIN_T );

                if( pin->GetUnit() &&
                    ( pin->GetUnit() != DrawLibItem->GetUnitSelection( sheetlist ) ) )
                    continue;

                if( pin->GetConvert() && ( pin->GetConvert() != DrawLibItem->GetConvert() ) )
                    continue;

                wxPoint pos2;

                pos2 = DrawLibItem->GetTransform().TransformCoordinate( pin->GetPosition() ) +
                    DrawLibItem->m_Pos;

                new_item = new NETLIST_OBJECT();
                new_item->m_SheetListInclude = *sheetlist;
                new_item->m_Comp = pin;
                new_item->m_SheetList = *sheetlist;
                new_item->m_Type = NET_PIN;
                new_item->m_Link = DrawLibItem;
                new_item->m_ElectricalType = pin->GetType();
                new_item->m_PinNum = pin->GetNumber();
                new_item->m_Label  = pin->GetName();
                new_item->m_Start  = new_item->m_End = pos2;

                aNetItemBuffer.push_back( new_item );

                if( ( (int) pin->GetType() == (int) PIN_POWER_IN ) && !pin->IsVisible() )
                {
                    /* There is an associated PIN_LABEL. */
                    new_item = new NETLIST_OBJECT();
                    new_item->m_SheetListInclude = *sheetlist;
                    new_item->m_Comp = NULL;
                    new_item->m_SheetList = *sheetlist;
                    new_item->m_Type  = NET_PINLABEL;
                    new_item->m_Label = pin->GetName();
                    new_item->m_Start = pos2;
                    new_item->m_End   = new_item->m_Start;

                    aNetItemBuffer.push_back( new_item );
                }
            }
            break;

        case SCH_POLYLINE_T:
        case SCH_BUS_ENTRY_T:
        case SCH_MARKER_T:
        case SCH_TEXT_T:
            break;

        case SCH_SHEET_T:
        {
            #undef STRUCT
            #define STRUCT ( (SCH_SHEET*) DrawList )
            list = *sheetlist;
            list.Push( STRUCT );
            SCH_SHEET* sheet = (SCH_SHEET*) DrawList;

            BOOST_FOREACH( SCH_SHEET_PIN label, sheet->GetSheetPins() )
            {
                ii = IsBusLabel( label.m_Text );
                new_item = new NETLIST_OBJECT();
                new_item->m_SheetListInclude = *sheetlist;
                new_item->m_Comp = &label;
                new_item->m_SheetList = *sheetlist;
                new_item->m_Link = DrawList;
                new_item->m_Type = NET_SHEETLABEL;
                new_item->m_ElectricalType = label.m_Shape;
                new_item->m_Label = label.m_Text;
                new_item->m_SheetListInclude = list;
                new_item->m_Start = new_item->m_End = label.m_Pos;
                aNetItemBuffer.push_back( new_item );

                if( ii )
                    ConvertBusToMembers( aNetItemBuffer, *new_item );
            }

            break;
        }

        case SCH_SHEET_LABEL_T:
        default:
        {
            wxString msg;
            msg.Printf( wxT( "Netlist: unexpected struct type %d" ),
                       DrawList->Type() );
            wxMessageBox( msg );
            break;
        }
        }
    }
}


/*
 * Routine that analyzes the type labels xxBUSLABELMEMBER
 * Propagate Netcode between the corresponding labels (ie when
 * Their member number is the same) when they are connected
 * Generally by their BusNetCode
 * Uses and updates the variable LastNetCode
 */
static void ConnectBusLabels( NETLIST_OBJECT_LIST& aNetItemBuffer )
{
    for( unsigned ii = 0; ii < aNetItemBuffer.size(); ii++ )
    {
        NETLIST_OBJECT* Label = aNetItemBuffer[ii];
        if( (Label->m_Type == NET_SHEETBUSLABELMEMBER)
           || (Label->m_Type == NET_BUSLABELMEMBER)
           || (Label->m_Type == NET_HIERBUSLABELMEMBER) )
        {
            if( Label->GetNet() == 0 )
            {
                Label->SetNet( LastNetCode );
                LastNetCode++;
            }

            for( unsigned jj = ii + 1; jj < aNetItemBuffer.size(); jj++ )
            {
                NETLIST_OBJECT* LabelInTst = aNetItemBuffer[jj];
                if( (LabelInTst->m_Type == NET_SHEETBUSLABELMEMBER)
                   || (LabelInTst->m_Type == NET_BUSLABELMEMBER)
                   || (LabelInTst->m_Type == NET_HIERBUSLABELMEMBER) )
                {
                    if( LabelInTst->m_BusNetCode != Label->m_BusNetCode )
                        continue;

                    if( LabelInTst->m_Member != Label->m_Member )
                        continue;

                    if( LabelInTst->GetNet() == 0 )
                        LabelInTst->SetNet( Label->GetNet() );
                    else
                        PropageNetCode( LabelInTst->GetNet(),
                                        Label->GetNet(), 0 );
                }
            }
        }
    }
}


/* Check if the Label has a bus notation.
 * Returns 0 if not
 * Number of members if yes
 * Updates FirstNumWireBus, LastNumWireBus and RootBusNameLength
 */
int IsBusLabel( const wxString& LabelDrawList )
{
    unsigned Num;
    int ii;
    wxString BufLine;
    long tmp;
    bool error = FALSE;

    /* Search for  '[' because a bus label is like "busname[nn..mm]" */
    ii = LabelDrawList.Find( '[' );
    if( ii < 0 )
        return 0;

    Num = (unsigned) ii;

    FirstNumWireBus   = LastNumWireBus = 9;
    RootBusNameLength = Num;
    Num++;
    while( LabelDrawList[Num] != '.' && Num < LabelDrawList.Len() )
    {
        BufLine.Append( LabelDrawList[Num] );
        Num++;
    }

    if( !BufLine.ToLong( &tmp ) )
        error = true;

    FirstNumWireBus = tmp;
    while( LabelDrawList[Num] == '.' && Num < LabelDrawList.Len() )
        Num++;

    BufLine.Empty();
    while( LabelDrawList[Num] != ']' && Num < LabelDrawList.Len() )
    {
        BufLine.Append( LabelDrawList[Num] );
        Num++;
    }

    if( !BufLine.ToLong( &tmp ) )
        error = true;
    LastNumWireBus = tmp;

    if( FirstNumWireBus < 0 )
        FirstNumWireBus = 0;
    if( LastNumWireBus < 0 )
        LastNumWireBus = 0;
    if( FirstNumWireBus > LastNumWireBus )
    {
        EXCHG( FirstNumWireBus, LastNumWireBus );
    }

    return LastNumWireBus - FirstNumWireBus + 1;
}


/*
 * Routine which breaks a seal Bus type Label in as many members it contains,
 * And creates structures with type NET_GLOBBUSLABELMEMBER, NET_BUSLABELMEMBER
 * Or NET_SHEETBUSLABELMEMBER
 * Entry = pointer to NETLIST_OBJECT initializes the corresp buslabel
 * Assumes that FirstNumWireBus, LastNumWireBus and RootBusNameLength are up
 * to date
 * Amends NETLIST_OBJECT base and meets the following
 * M_Label is a pointer to a new wxString
 * M_Label must be deallocated by the user (only for a NET_GLOBBUSLABELMEMBER,
 * NET_BUSLABELMEMBER gold NET_SHEETBUSLABELMEMBER object type)
 */
static int ConvertBusToMembers( NETLIST_OBJECT_LIST& aNetItemBuffer,
                                NETLIST_OBJECT&      BusLabel )
{
    int NumItem, BusMember;
    wxString BufLine;

    if( BusLabel.m_Type == NET_HIERLABEL )
        BusLabel.m_Type = NET_HIERBUSLABELMEMBER;
    else if( BusLabel.m_Type == NET_GLOBLABEL )
        BusLabel.m_Type = NET_GLOBBUSLABELMEMBER;
    else if( BusLabel.m_Type == NET_SHEETLABEL )
        BusLabel.m_Type = NET_SHEETBUSLABELMEMBER;
    else
        BusLabel.m_Type = NET_BUSLABELMEMBER;

    /* Conversion of BusLabel to the root Label name + the member id like mybus0, mybus1 ... */
    BufLine = BusLabel.m_Label.Left( RootBusNameLength );

    BusMember = FirstNumWireBus;
    BufLine << BusMember;
    BusLabel.m_Label = BufLine;

    BusLabel.m_Member = BusMember;
    NumItem = 1;

    for( BusMember++; BusMember <= LastNumWireBus; BusMember++ )
    {
        NETLIST_OBJECT* new_label = new NETLIST_OBJECT( BusLabel );
        NumItem++;

        /* Conversion of BusLabel to the root name + the current member id.*/
        BufLine = BusLabel.m_Label.Left( RootBusNameLength );
        BufLine << BusMember;
        new_label->m_Label = BufLine;

        new_label->m_Member = BusMember;
        aNetItemBuffer.push_back( new_label );
    }

    return NumItem;
}


/*
 * PropageNetCode propagates Netcode NewNetCode on all elements
 * belonging to the former Netcode OldNetCode
 * If IsBus == 0; Netcode is the member who is spreading
 * If IsBus != 0; is the member who is spreading BusNetCode
 */
static void PropageNetCode( int OldNetCode, int NewNetCode, int IsBus )
{
    if( OldNetCode == NewNetCode )
        return;

    if( IsBus == 0 )    /* Propagate NetCode */
    {
        for( unsigned jj = 0; jj < g_NetObjectslist.size(); jj++ )
        {
            NETLIST_OBJECT* Objet = g_NetObjectslist[jj];
            if( Objet->GetNet() == OldNetCode )
            {
                Objet->SetNet( NewNetCode );
            }
        }
    }
    else               /* Propagate BusNetCode */
    {
        for( unsigned jj = 0; jj < g_NetObjectslist.size(); jj++ )
        {
            NETLIST_OBJECT* Objet = g_NetObjectslist[jj];
            if( Objet->m_BusNetCode == OldNetCode )
            {
                Objet->m_BusNetCode = NewNetCode;
            }
        }
    }
}


/*
 * Check if Ref element is connected to other elements of the list of objects
 * in the schematic, by mode point
 * A point (end superimposed)
 *
 * If IsBus:
 * The connection involves elements such as bus
 * (Or BUS or BUSLABEL JUNCTION)
 * Otherwise
 * The connection involves elements such as non-bus
 * (Other than BUS or BUSLABEL)
 *
 * The Ref object must have a valid Netcode.
 *
 * The list of objects is SUPPOSED class by SheetPath Croissants,
 * And research is done from the start element, 1st element
 * Leaf schema
 * (There can be no physical connection between elements of different sheets)
 */
static void PointToPointConnect( NETLIST_OBJECT* Ref, int IsBus, int start )
{
    int netCode;

    if( IsBus == 0 )    /* Objects other than BUS and BUSLABELS. */
    {
        netCode = Ref->GetNet();
        for( unsigned i = start; i < g_NetObjectslist.size(); i++ )
        {
            NETLIST_OBJECT* item = g_NetObjectslist[i];
            if( item->m_SheetList != Ref->m_SheetList )  //used to be > (why?)
                continue;

            switch( item->m_Type )
            {
            case NET_SEGMENT:
            case NET_PIN:
            case NET_LABEL:
            case NET_HIERLABEL:
            case NET_GLOBLABEL:
            case NET_SHEETLABEL:
            case NET_PINLABEL:
            case NET_JONCTION:
            case NET_NOCONNECT:
                if( Ref->m_Start == item->m_Start
                    || Ref->m_Start == item->m_End
                    || Ref->m_End   == item->m_Start
                    || Ref->m_End   == item->m_End )
                {
                    if( item->GetNet() == 0 )
                        item->SetNet( netCode );
                    else
                        PropageNetCode( item->GetNet(), netCode, 0 );
                }
                break;

            case NET_BUS:
            case NET_BUSLABELMEMBER:
            case NET_SHEETBUSLABELMEMBER:
            case NET_HIERBUSLABELMEMBER:
            case NET_GLOBBUSLABELMEMBER:
            case NET_ITEM_UNSPECIFIED:
                break;
            }
        }
    }
    else    /* Object type BUS, BUSLABELS, and junctions. */
    {
        netCode = Ref->m_BusNetCode;
        for( unsigned i = start; i<g_NetObjectslist.size(); i++ )
        {
            NETLIST_OBJECT* item = g_NetObjectslist[i];
            if( item->m_SheetList != Ref->m_SheetList )
                continue;

            switch( item->m_Type )
            {
            case NET_ITEM_UNSPECIFIED:
            case NET_SEGMENT:
            case NET_PIN:
            case NET_LABEL:
            case NET_HIERLABEL:
            case NET_GLOBLABEL:
            case NET_SHEETLABEL:
            case NET_PINLABEL:
            case NET_NOCONNECT:
                break;

            case NET_BUS:
            case NET_BUSLABELMEMBER:
            case NET_SHEETBUSLABELMEMBER:
            case NET_HIERBUSLABELMEMBER:
            case NET_GLOBBUSLABELMEMBER:
            case NET_JONCTION:
                if( Ref->m_Start == item->m_Start
                    || Ref->m_Start == item->m_End
                    || Ref->m_End   == item->m_Start
                    || Ref->m_End   == item->m_End )
                {
                    if( item->m_BusNetCode == 0 )
                        item->m_BusNetCode = netCode;
                    else
                        PropageNetCode( item->m_BusNetCode, netCode, 1 );
                }
                break;
            }
        }
    }
}


/*
 * Search if a junction is connected to segments and propagate the junction Netcode
 * to objects connected by the junction.
 * The junction must have a valid Netcode
 * The list of objects is expected sorted by sheets.
 * Search is done from index aIdxStart to the last element of g_NetObjectslist
 */
static void SegmentToPointConnect( NETLIST_OBJECT* aJonction,
                                   int aIsBus, int aIdxStart )
{
    for( unsigned i = aIdxStart; i < g_NetObjectslist.size(); i++ )
    {
        NETLIST_OBJECT* Segment = g_NetObjectslist[i];

        // if different sheets, no physical connection between elements is possible.
        if( Segment->m_SheetList != aJonction->m_SheetList )
            continue;

        if( aIsBus == 0 )
        {
            if( Segment->m_Type != NET_SEGMENT )
                continue;
        }
        else
        {
            if( Segment->m_Type != NET_BUS )
                continue;
        }

        if( SegmentIntersect( Segment->m_Start, Segment->m_End, aJonction->m_Start ) )
        {
            /* Propagation Netcode has all the objects of the same Netcode. */
            if( aIsBus == 0 )
            {
                if( Segment->GetNet() )
                    PropageNetCode( Segment->GetNet(),
                                    aJonction->GetNet(), aIsBus );
                else
                    Segment->SetNet( aJonction->GetNet() );
            }
            else
            {
                if( Segment->m_BusNetCode )
                    PropageNetCode( Segment->m_BusNetCode,
                                    aJonction->m_BusNetCode, aIsBus );
                else
                    Segment->m_BusNetCode = aJonction->m_BusNetCode;
            }
        }
    }
}


/*****************************************************************
 * Function which connects the groups of object which have the same label
 *******************************************************************/
void LabelConnect( NETLIST_OBJECT* LabelRef )
{
    if( LabelRef->GetNet() == 0 )
        return;

    for( unsigned i = 0; i < g_NetObjectslist.size(); i++ )
    {
        if( g_NetObjectslist[i]->GetNet() == LabelRef->GetNet() )
            continue;
        if( g_NetObjectslist[i]->m_SheetList != LabelRef->m_SheetList )
        {
            if( (g_NetObjectslist[i]->m_Type != NET_PINLABEL
                 && g_NetObjectslist[i]->m_Type != NET_GLOBLABEL
                 && g_NetObjectslist[i]->m_Type != NET_GLOBBUSLABELMEMBER) )
                continue;
            if( (g_NetObjectslist[i]->m_Type == NET_GLOBLABEL
                 || g_NetObjectslist[i]->m_Type == NET_GLOBBUSLABELMEMBER)
               && g_NetObjectslist[i]->m_Type != LabelRef->m_Type )
                //global labels only connect other global labels.
                continue;
        }

        // regular labels are sheet-local;
        // NET_HIERLABEL are used to connect sheets.
        // NET_LABEL is sheet-local (***)
        // NET_GLOBLABEL is global.
        // NET_PINLABEL is a kind of global label (generated by a power pin invisible)
        NetObjetType ntype = g_NetObjectslist[i]->m_Type;
        if(  ntype == NET_LABEL
             || ntype == NET_GLOBLABEL
             || ntype == NET_HIERLABEL
             || ntype == NET_BUSLABELMEMBER
             || ntype == NET_GLOBBUSLABELMEMBER
             || ntype == NET_HIERBUSLABELMEMBER
             || ntype == NET_PINLABEL )
        {
            if( g_NetObjectslist[i]->m_Label.CmpNoCase( LabelRef->m_Label ) != 0 )
                continue;

            if( g_NetObjectslist[i]->GetNet() )
                PropageNetCode( g_NetObjectslist[i]->GetNet(), LabelRef->GetNet(), 0 );
            else
                g_NetObjectslist[i]->SetNet( LabelRef->GetNet() );
        }
    }
}


/* Comparison routine for sorting by increasing Netcode
 * table of elements connected (TabPinSort) by qsort ()
 */
bool SortItemsbyNetcode( const NETLIST_OBJECT* Objet1,
                         const NETLIST_OBJECT* Objet2 )
{
    return Objet1->GetNet() < Objet2->GetNet();
}


/* Comparison routine for sorting items by Sheet Number ( used by qsort )
 */

bool SortItemsBySheet( const NETLIST_OBJECT* Objet1,
                       const NETLIST_OBJECT* Objet2 )
{
    return Objet1->m_SheetList.Cmp( Objet2->m_SheetList ) < 0;
}


/* Routine positioning member. FlagNoConnect ELEMENTS
 * List of objects NetList, sorted by order of Netcode
 */
static void SetUnconnectedFlag( NETLIST_OBJECT_LIST& aNetItemBuffer )
{
    NETLIST_OBJECT* NetItemRef;
    unsigned NetStart, NetEnd;
    ConnectType StateFlag;

    NetStart  = NetEnd = 0;
    StateFlag = UNCONNECTED;
    for( unsigned ii = 0; ii < aNetItemBuffer.size(); ii++ )
    {
        NetItemRef = aNetItemBuffer[ii];
        if( NetItemRef->m_Type == NET_NOCONNECT && StateFlag != PAD_CONNECT )
            StateFlag = NOCONNECT_SYMBOL_PRESENT;

        /* Analysis of current net. */
        unsigned idxtoTest = ii + 1;

        if( ( idxtoTest >= aNetItemBuffer.size() )
           || ( NetItemRef->GetNet() != aNetItemBuffer[idxtoTest]->GetNet() ) )
        {
            /* Net analysis to update m_FlagOfConnection */
            NetEnd = idxtoTest;

            /* set m_FlagOfConnection member to StateFlag for all items of
             * this net: */
            for( unsigned kk = NetStart; kk < NetEnd; kk++ )
                aNetItemBuffer[kk]->m_FlagOfConnection = StateFlag;

            if( idxtoTest >= aNetItemBuffer.size() )
                return;

            /* Start Analysis next Net */
            StateFlag = UNCONNECTED;
            NetStart  = idxtoTest;
            continue;
        }

        /* test the current item: if this is a pin and if the reference item
         * is also a pin, then 2 pins are connected, so set StateFlag to
         * PAD_CONNECT (can be already done)  Of course, if the current
         * item is a no connect symbol, set StateFlag to
         * NOCONNECT_SYMBOL_PRESENT to inhibit error diags. However if
         * StateFlag is already set to PAD_CONNECT this state is kept (the
         * no connect symbol was surely an error and an ERC will report this)
         */
        for( ; ; idxtoTest++ )
        {
            if( ( idxtoTest >= aNetItemBuffer.size() )
               || ( NetItemRef->GetNet() != aNetItemBuffer[idxtoTest]->GetNet() ) )
                break;

            switch( aNetItemBuffer[idxtoTest]->m_Type )
            {
            case NET_ITEM_UNSPECIFIED:
                wxMessageBox( wxT( "BuildNetListBase() error" ) );
                break;

            case NET_SEGMENT:
            case NET_LABEL:
            case NET_HIERLABEL:
            case NET_GLOBLABEL:
            case NET_SHEETLABEL:
            case NET_PINLABEL:
            case NET_BUS:
            case NET_BUSLABELMEMBER:
            case NET_SHEETBUSLABELMEMBER:
            case NET_HIERBUSLABELMEMBER:
            case NET_GLOBBUSLABELMEMBER:
            case NET_JONCTION:
                break;

            case NET_PIN:
                if( NetItemRef->m_Type == NET_PIN )
                    StateFlag = PAD_CONNECT;
                break;

            case NET_NOCONNECT:
                if( StateFlag != PAD_CONNECT )
                    StateFlag = NOCONNECT_SYMBOL_PRESENT;
                break;
            }
        }
    }
}
