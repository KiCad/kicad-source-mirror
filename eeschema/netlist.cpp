/***********************************/
/* Module de calcul de la Netliste */
/***********************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "netlist.h" /* Definitions generales liees au calcul de netliste */
#include "protos.h"

#include "algorithm"

// Buffer to build the list of items used in netlist and erc calculations
NETLIST_OBJECT_LIST g_NetObjectslist;

//#define NETLIST_DEBUG

/* Routines locales */
static void PropageNetCode( int OldNetCode, int NewNetCode, int IsBus );
static void SheetLabelConnect( NETLIST_OBJECT* SheetLabel );
static void ListeObjetConnection( DrawSheetPath*       sheetlist,
                                  NETLIST_OBJECT_LIST& aNetItemBuffer );
static int  ConvertBusToMembers( NETLIST_OBJECT_LIST& aNetItemBuffer,
                                 NETLIST_OBJECT&      ObjNet );
static void PointToPointConnect( NETLIST_OBJECT* Ref, int IsBus,
                                 int start );
static void SegmentToPointConnect( NETLIST_OBJECT* Jonction, int IsBus,
                                   int start );
static void LabelConnect( NETLIST_OBJECT* Label );
static void ConnectBusLabels( NETLIST_OBJECT_LIST& aNetItemBuffer );
static void SetUnconnectedFlag( NETLIST_OBJECT_LIST& aNetItemBuffer );

// Sort functions used here:
static bool SortItemsbyNetcode( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 );
static bool SortItemsBySheet( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 );

/* Variable locales */
static int FirstNumWireBus, LastNumWireBus, RootBusNameLength;
static int LastNetCode, LastBusNetCode;



#if defined(DEBUG)
void dumpNetTable()
{
    for( unsigned idx = 0;  idx < g_NetObjectslist.size();  ++idx )
    {
        g_NetObjectslist[idx]->Show( std::cout, idx );
    }
}
#endif

/*********************************************************************/
void FreeNetObjectsList( NETLIST_OBJECT_LIST& aNetObjectsBuffer )
/*********************************************************************/

/*
 *  Routine de liberation memoire des tableaux utilises pour le calcul
 *  de la netliste
 *  TabNetItems = pointeur sur le tableau principal (liste des items )
 */
{
    for( unsigned i = 0; i < aNetObjectsBuffer.size(); i++ )
        delete aNetObjectsBuffer[i];

    aNetObjectsBuffer.clear();
}


/************************************************************************/
void WinEDA_SchematicFrame::BuildNetListBase()
/************************************************************************/

/* Routine qui construit le tableau des elements connectes du projet
 *  met a jour:
 *      g_NetObjectslist
 *      g_NetObjectslist
 */
{
    int            NetNumber;
    int            NetCode;
    DrawSheetPath* sheet;
    wxString       msg, activity;
    wxBusyCursor   Busy;

    NetNumber = 1;

    activity = _( "List" );
    SetStatusText( activity );

    FreeNetObjectsList( g_NetObjectslist );

    /* Build the sheet (not screen) list (flattened)*/
    EDA_SheetList SheetListList;

    /* Fill g_NetObjectslist with items used in connectivity calculation */

    sheet = SheetListList.GetFirst();
    for( ; sheet != NULL; sheet = SheetListList.GetNext() )
        ListeObjetConnection( sheet, g_NetObjectslist );

    if( g_NetObjectslist.size() == 0 )
        return; // no objects

    activity.Empty();
    activity << wxT( " " ) << _( "NbItems" ) << wxT( " " ) << g_NetObjectslist.size();
    SetStatusText( activity );

    /* Sort objects by Sheet */

    sort( g_NetObjectslist.begin(), g_NetObjectslist.end(), SortItemsBySheet );

    activity << wxT( ";  " ) << _( "Conn" );
    SetStatusText( activity );

    sheet = &(g_NetObjectslist[0]->m_SheetList);
    LastNetCode = LastBusNetCode = 1;

    for( unsigned ii = 0, istart = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        NETLIST_OBJECT* net_item = g_NetObjectslist[ii];
        if( net_item->m_SheetList != *sheet )   // Sheet change
        {
            sheet = &(net_item->m_SheetList);
            istart = ii;
        }

        switch( net_item->m_Type )
        {
        case NET_ITEM_UNSPECIFIED:
            wxMessageBox(wxT("BuildNetListBase() error"));
            break;
        case NET_PIN:
        case NET_PINLABEL:
        case NET_SHEETLABEL:
        case NET_NOCONNECT:
            if( net_item->GetNet() != 0 )
                break; /* Deja connecte */

        case NET_SEGMENT:
            /* Controle des connexions type point a point ( Sans BUS ) */
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( LastNetCode );
                LastNetCode++;
            }
            PointToPointConnect( net_item, 0, istart );
            break;

        case NET_JONCTION:
            /* Controle des jonction , hors BUS */
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( LastNetCode );
                LastNetCode++;
            }
            SegmentToPointConnect( net_item, 0, istart );

            /* Controle des jonction , sur BUS */
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
            /* Controle des connexions type jonction ( Sans BUS ) */
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( LastNetCode );
                LastNetCode++;
            }
            SegmentToPointConnect( net_item, 0, istart );
            break;

        case NET_SHEETBUSLABELMEMBER:
            if( net_item->m_BusNetCode != 0 )
                break; /* Deja connecte */

        case NET_BUS:
            /* Controle des connexions type point a point mode BUS */
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
            /* Controle des connexions semblables a des sur BUS */
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


    activity << wxT( " " ) << _( "Done" );
    SetStatusText( activity );

    /* Mise a jour des NetCodes des Bus Labels connectes par les Bus */
    ConnectBusLabels( g_NetObjectslist );

    activity << wxT( ";  " ) << _( "Labels" );
    SetStatusText( activity );

    /* Connections des groupes d'objets par labels identiques */
    for( unsigned ii = 0;  ii < g_NetObjectslist.size(); ii++ )
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

    activity << wxT( " " ) << _( "Done" );
    SetStatusText( activity );

    /* Connexion des hierarchies */
    activity << wxT( ";  " ) << _( "Hierar." );
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
    std::cout << "after qsort()\n";
    dumpNetTable();
#endif

    activity << wxT( " " ) << _( "Done" );
    SetStatusText( activity );

    /* Compression des numeros de NetCode a des valeurs consecutives */
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

    /* Affectation du m_FlagOfConnection en fonction de connection ou non */
    SetUnconnectedFlag( g_NetObjectslist );
}


/*************************************************************
 * Routine qui connecte les sous feuilles par les sheetLabels  *
 **************************************************************/
static void SheetLabelConnect( NETLIST_OBJECT* SheetLabel )
{
    if( SheetLabel->GetNet() == 0 )
        return;

    /* Calcul du numero de sous feuille correspondante au sheetlabel */

    /* Comparaison du SheetLabel avec les GLABELS de la sous feuille
     *      pour regroupement des NetCodes */
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        NETLIST_OBJECT* ObjetNet = g_NetObjectslist[ii];
        if( ObjetNet->m_SheetList != SheetLabel->m_SheetListInclude )
            continue; //use SheetInclude, not the sheet!!

        if( (ObjetNet->m_Type != NET_HIERLABEL )
           && (ObjetNet->m_Type != NET_HIERBUSLABELMEMBER ) )
            continue;

        if( ObjetNet->GetNet() == SheetLabel->GetNet() )
            continue; //already connected.

        wxASSERT(ObjetNet->m_Label);
        wxASSERT(SheetLabel->m_Label);
        if( ObjetNet->m_Label->CmpNoCase( *SheetLabel->m_Label ) != 0 )
            continue; //different names.

        /* Propagation du Netcode a tous les Objets de meme NetCode */
        if( ObjetNet->GetNet() )
            PropageNetCode( ObjetNet->GetNet(), SheetLabel->GetNet(), 0 );
        else
            ObjetNet->SetNet( SheetLabel->GetNet() );
    }
}


/**************************************************************************************/
static void ListeObjetConnection( DrawSheetPath*                sheetlist,
                                  std::vector<NETLIST_OBJECT*>& aNetItemBuffer )
/**************************************************************************************/

/** Function ListeObjetConnection
 * Creates the list of objects related to connections (pins of components, wires, labels, junctions ...)
 * @param sheetlist: pointer to a sheetlist.
 * @param aNetItemBuffer: a std::vector to store pointer on NETLIST_OBJECT created
 */
{
    int                            ii;
    SCH_ITEM*                      DrawList;
    NETLIST_OBJECT*                new_item;
    SCH_COMPONENT*                 DrawLibItem;
    EDA_LibComponentStruct*        Entry;
    LibEDA_BaseStruct*             DEntry;
    Hierarchical_PIN_Sheet_Struct* SheetLabel;
    DrawSheetPath                  list;

    DrawList = sheetlist->LastScreen()->EEDrawList;
    for( ; DrawList;  DrawList = DrawList->Next() )
    {
        switch( DrawList->Type() )
        {
        case DRAW_SEGMENT_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (EDA_DrawLineStruct*) DrawList )
            if( (STRUCT->GetLayer() != LAYER_BUS) && (STRUCT->GetLayer() != LAYER_WIRE) )
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
            else            /* Cas des WIRE */
            {
                new_item->m_Type = NET_SEGMENT;
            }
            aNetItemBuffer.push_back( new_item );
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawJunctionStruct*) DrawList )
            new_item = new NETLIST_OBJECT();

            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp  = STRUCT;
            new_item->m_Type  = NET_JONCTION;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;

            aNetItemBuffer.push_back( new_item );
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawNoConnectStruct*) DrawList )
            new_item = new NETLIST_OBJECT();

            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp  = STRUCT;
            new_item->m_Type  = NET_NOCONNECT;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;

            aNetItemBuffer.push_back( new_item );
            break;

        case TYPE_SCH_LABEL:
            #undef STRUCT
            #define STRUCT ( (SCH_LABEL*) DrawList )
            ii = IsBusLabel( STRUCT->m_Text );

            new_item = new NETLIST_OBJECT();
            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp = STRUCT;
            new_item->m_Type = NET_LABEL;

            if( STRUCT->m_Layer ==  LAYER_GLOBLABEL )
                new_item->m_Type = NET_GLOBLABEL;
            if( STRUCT->m_Layer ==  LAYER_HIERLABEL )
                new_item->m_Type = NET_HIERLABEL;

            new_item->m_Label = &STRUCT->m_Text;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;

            aNetItemBuffer.push_back( new_item );
            /* Si c'est un Bus, eclatement en Label */
            if( ii )
                ConvertBusToMembers( aNetItemBuffer, *new_item );


            break;

        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            #undef STRUCT
            #define STRUCT ( (SCH_LABEL*) DrawList )
            ii = IsBusLabel( STRUCT->m_Text );
            new_item = new NETLIST_OBJECT();
            new_item->m_SheetList = *sheetlist;
            new_item->m_SheetListInclude = *sheetlist;
            new_item->m_Comp = STRUCT;
            new_item->m_Type = NET_LABEL;

            if( STRUCT->m_Layer ==  LAYER_GLOBLABEL )       //this is not the simplest way of doing it
                new_item->m_Type = NET_GLOBLABEL;           // (look at the case statement above).
            if( STRUCT->m_Layer ==  LAYER_HIERLABEL )
                new_item->m_Type = NET_HIERLABEL;

            new_item->m_Label = &STRUCT->m_Text;
            new_item->m_Start = new_item->m_End = STRUCT->m_Pos;
            aNetItemBuffer.push_back( new_item );

            /* Si c'est un Bus, eclatement en Label */
            if( ii )
                ConvertBusToMembers( aNetItemBuffer, *new_item );


            break;

        case TYPE_SCH_COMPONENT:
            DrawLibItem = (SCH_COMPONENT*) DrawList;

            Entry = FindLibPart( DrawLibItem->m_ChipName, wxEmptyString, FIND_ROOT );

            if( Entry == NULL )
                break;

            if( Entry->m_Drawings == NULL )
                break;

            DEntry = Entry->m_Drawings;

            for( ;  DEntry;   DEntry = DEntry->Next() )
            {
                LibDrawPin* Pin = (LibDrawPin*) DEntry;
                if( DEntry->Type() != COMPONENT_PIN_DRAW_TYPE )
                    continue;

                if( DEntry->m_Unit && ( DEntry->m_Unit != DrawLibItem->GetUnitSelection( sheetlist ) ) )
                    continue;

                if( DEntry->m_Convert
                   && (DEntry->m_Convert != DrawLibItem->m_Convert) )
                    continue;

                wxPoint pos2 =
                    TransformCoordinate( DrawLibItem->m_Transform,
                                         Pin->m_Pos ) + DrawLibItem->m_Pos;

                new_item = new NETLIST_OBJECT();
                new_item->m_SheetListInclude = *sheetlist;
                new_item->m_Comp = DEntry;
                new_item->m_SheetList = *sheetlist;
                new_item->m_Type = NET_PIN;
                new_item->m_Link = DrawLibItem;
                new_item->m_ElectricalType = Pin->m_PinType;
                new_item->m_PinNum = Pin->m_PinNum;
                new_item->m_Label  = &Pin->m_PinName;
                new_item->m_Start  = new_item->m_End = pos2;

                aNetItemBuffer.push_back( new_item );

                if( ( (int) Pin->m_PinType == (int) PIN_POWER_IN )
                   && ( Pin->m_Attributs & PINNOTDRAW ) )
                {
                    /* Il y a un PIN_LABEL Associe */
                    new_item = new NETLIST_OBJECT();
                    new_item->m_SheetListInclude = *sheetlist;
                    new_item->m_Comp = NULL;
                    new_item->m_SheetList = *sheetlist;
                    new_item->m_Type  = NET_PINLABEL;
                    new_item->m_Label = &Pin->m_PinName;
                    new_item->m_Start = pos2;
                    new_item->m_End   = new_item->m_Start;

                    aNetItemBuffer.push_back( new_item );
                }
            }

            break;

        case DRAW_PICK_ITEM_STRUCT_TYPE:
        case DRAW_POLYLINE_STRUCT_TYPE:
        case DRAW_BUSENTRY_STRUCT_TYPE:
        case DRAW_MARKER_STRUCT_TYPE:
        case TYPE_SCH_TEXT:
            break;

        case DRAW_SHEET_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (DrawSheetStruct*) DrawList )
            list = *sheetlist;
            list.Push( STRUCT );
            SheetLabel = STRUCT->m_Label;
            for( ; SheetLabel != NULL;
                SheetLabel = SheetLabel->Next() )
            {
                ii = IsBusLabel( SheetLabel->m_Text );
                new_item = new NETLIST_OBJECT();
                new_item->m_SheetListInclude = *sheetlist;
                new_item->m_Comp = SheetLabel;
                new_item->m_SheetList = *sheetlist;
                new_item->m_Link = DrawList;
                new_item->m_Type = NET_SHEETLABEL;
                new_item->m_ElectricalType = SheetLabel->m_Shape;
                new_item->m_Label = &SheetLabel->m_Text;
                new_item->m_SheetListInclude = list;
                new_item->m_Start = new_item->m_End = SheetLabel->m_Pos;
                aNetItemBuffer.push_back( new_item );

                /* Si c'est un Bus, eclatement en Label */
                if( ii )
                    ConvertBusToMembers( aNetItemBuffer, *new_item );
            }

            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            DisplayError( NULL, wxT( "Netlist: Type DRAW_SHEETLABEL inattendu" ) );
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT( "Netlist: unexpected type struct %d" ),
                       DrawList->Type() );
            DisplayError( NULL, msg );
            break;
        }
        }
    }
}


/************************************************************************/
static void ConnectBusLabels( NETLIST_OBJECT_LIST& aNetItemBuffer )
/************************************************************************/

/* Routine qui analyse les labels type xxBUSLABELMEMBER
 *  Propage les Netcodes entre labels correspondants ( c'est a dire lorsque
 *  leur numero de membre est identique) lorsqu'ils sont connectes
 *  globalement par leur BusNetCode
 *  Utilise et met a jour la variable LastNetCode
 */
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

            for( unsigned jj = ii + 1;  jj < aNetItemBuffer.size(); jj++ )
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
                        PropageNetCode( LabelInTst->GetNet(), Label->GetNet(), 0 );
                }
            }
        }
    }
}


/**************************************************/
int IsBusLabel( const wxString& LabelDrawList )
/**************************************************/

/* Routine qui verifie si le Label a une notation de type Bus
 *  Retourne 0 si non
 *  nombre de membres si oui
 *  met a jour FirstNumWireBus, LastNumWireBus et RootBusNameLength
 */

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
        error = TRUE;

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
        error = TRUE;;
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


/***************************************************************/
static int ConvertBusToMembers( NETLIST_OBJECT_LIST& aNetItemBuffer,
                                NETLIST_OBJECT&      BusLabel )
/***************************************************************/

/* Routine qui eclate un label type Bus en autant de Label qu'il contient de membres,
 *  et qui cree les structures avec le type NET_GLOBBUSLABELMEMBER, NET_BUSLABELMEMBER
 *  ou NET_SHEETBUSLABELMEMBER
 *  entree = pointeur sur l'NETLIST_OBJECT initialise corresp au buslabel
 *  suppose que FirstNumWireBus, LastNumWireBus et RootBusNameLength sont a jour
 *  modifie l'NETLIST_OBJECT de base et remplit les suivants
 *  m_Label is a pointer to a new wxString
 *  m_Label must be deallocated by the user (only for a NET_GLOBBUSLABELMEMBER,
 *  NET_BUSLABELMEMBER or a NET_SHEETBUSLABELMEMBER object type)
 */
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

    /* Convertion du BusLabel en la racine du Label + le numero du fil */
    BufLine = BusLabel.m_Label->Left( RootBusNameLength );

    BusMember = FirstNumWireBus;
    BufLine << BusMember;
    BusLabel.m_Label = new wxString( BufLine );

    BusLabel.m_Member = BusMember;
    NumItem = 1;

    for( BusMember++; BusMember <= LastNumWireBus; BusMember++ )
    {
        NETLIST_OBJECT* new_label = new NETLIST_OBJECT( BusLabel );
        NumItem++;
        /* Convertion du BusLabel en la racine du Label + le numero du fil */
        BufLine = BusLabel.m_Label->Left( RootBusNameLength );
        BufLine << BusMember;
        new_label->m_Label = new wxString( BufLine );

        new_label->m_Member = BusMember;
        aNetItemBuffer.push_back( new_label );
    }

    return NumItem;
}


/**********************************************************************/
static void PropageNetCode( int OldNetCode, int NewNetCode, int IsBus )
/**********************************************************************/

/* PropageNetCode propage le netcode NewNetCode sur tous les elements
 *  appartenant a l'ancien netcode OldNetCode
 *  Si IsBus == 0; c'est le membre NetCode qui est propage
 *  Si IsBus != 0; c'est le membre BusNetCode qui est propage
 */

{
    if( OldNetCode == NewNetCode )
        return;

    if( IsBus == 0 )    /* Propagation du NetCode */
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
    else            /* Propagation du BusNetCode */
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


/***************************************************************************/
static void PointToPointConnect( NETLIST_OBJECT* Ref, int IsBus, int start )
/***************************************************************************/

/* Routine qui verifie si l'element *Ref est connecte a
 *  d'autres elements de la liste des objets du schema, selon le mode Point
 *  a point ( Extremites superposees )
 *
 *  si IsBus:
 *      la connexion ne met en jeu que des elements type bus
 *          ( BUS ou BUSLABEL ou JONCTION )
 *  sinon
 *      la connexion ne met en jeu que des elements type non bus
 *          ( autres que BUS ou BUSLABEL )
 *
 *  L'objet Ref doit avoir un NetCode valide.
 *
 *  La liste des objets est supposee classe par SheetPath Croissants,
 *  et la recherche se fait a partir de l'element start, 1er element
 *  de la feuille de schema
 *  ( il ne peut y avoir connexion physique entre elements de differentes sheets)
 */
{
    int netCode;

    if( IsBus == 0 )    /* Objets autres que BUS et BUSLABELS */
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
    else    /* Objets type BUS et BUSLABELS ( et JONCTIONS )*/
    {
        netCode = Ref->m_BusNetCode;
        for( unsigned i = start;   i<g_NetObjectslist.size();   i++ )
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


/**************************************************************/
static void SegmentToPointConnect( NETLIST_OBJECT* Jonction,
                                   int IsBus, int start )
/***************************************************************/

/*
 *  Routine qui recherche si un point (jonction) est connecte a des segments,
 *  et regroupe les NetCodes des objets connectes a la jonction.
 *  Le point de jonction doit avoir un netcode valide
 *  La liste des objets est supposee classe par NumSheet Croissants,
 *  et la recherche se fait a partir de l'element start, 1er element
 *  de la feuille de schema
 *  ( il ne peut y avoir connexion physique entre elements de differentes sheets)
 */
{
    for( unsigned i = start; i < g_NetObjectslist.size(); i++ )
    {
        NETLIST_OBJECT* Segment = g_NetObjectslist[i];

        if( Segment->m_SheetList != Jonction->m_SheetList )
            continue;

        if( IsBus == 0 )
        {
            if( Segment->m_Type != NET_SEGMENT )
                continue;
        }
        else
        {
            if( Segment->m_Type != NET_BUS )
                continue;
        }

        if( SegmentIntersect( Segment->m_Start.x, Segment->m_Start.y,
                              Segment->m_End.x, Segment->m_End.y,
                              Jonction->m_Start.x, Jonction->m_Start.y ) )
        {
            /* Propagation du Netcode a tous les Objets de meme NetCode */
            if( IsBus == 0 )
            {
                if( Segment->GetNet() )
                    PropageNetCode( Segment->GetNet(),
                                    Jonction->GetNet(), IsBus );
                else
                    Segment->SetNet( Jonction->GetNet() );
            }
            else
            {
                if( Segment->m_BusNetCode )
                    PropageNetCode( Segment->m_BusNetCode,
                                    Jonction->m_BusNetCode, IsBus );
                else
                    Segment->m_BusNetCode = Jonction->m_BusNetCode;
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

    for( unsigned i = 0; i < g_NetObjectslist.size();  i++ )
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

        //regular labels are sheet-local;
        //NET_HIERLABEL are used to connect sheets.
        //NET_LABEL is sheet-local (***)
        //NET_GLOBLABEL is global.
        NetObjetType ntype = g_NetObjectslist[i]->m_Type;
        if(  ntype == NET_LABEL
             || ntype == NET_GLOBLABEL
             || ntype == NET_HIERLABEL
             || ntype == NET_BUSLABELMEMBER
             || ntype == NET_GLOBBUSLABELMEMBER
             || ntype == NET_HIERBUSLABELMEMBER
             || ntype == NET_PINLABEL )
        {
            if( g_NetObjectslist[i]->m_Label->CmpNoCase( *LabelRef->m_Label ) != 0 )
                continue;

            // Propagation du Netcode a tous les Objets de meme NetCode
            if( g_NetObjectslist[i]->GetNet() )
                PropageNetCode( g_NetObjectslist[i]->GetNet(), LabelRef->GetNet(), 0 );
            else
                g_NetObjectslist[i]->SetNet( LabelRef->GetNet() );
        }
    }
}


/****************************************************************************/
bool SortItemsbyNetcode( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 )
/****************************************************************************/

/* Routine de comparaison pour le tri par NetCode croissant
 *  du tableau des elements connectes ( TabPinSort ) par qsort()
 */
{
    return Objet1->GetNet() < Objet2->GetNet();
}


/*****************************************************************************************/
bool SortItemsBySheet( const NETLIST_OBJECT* Objet1, const NETLIST_OBJECT* Objet2 )
/*****************************************************************************************/

/* Routine de comparaison pour le tri par NumSheet
 *  du tableau des elements connectes ( TabPinSort ) par qsort() */

{
    return Objet1->m_SheetList.Cmp( Objet2->m_SheetList ) < 0;
}


/**********************************************************************/
static void SetUnconnectedFlag( NETLIST_OBJECT_LIST& aNetItemBuffer )
/**********************************************************************/

/* Routine positionnant le membre .FlagNoConnect des elements de
 *  la liste des objets netliste, tries par ordre de NetCode
 */
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

        /* Analyse du net en cours */
        unsigned idxtoTest = ii + 1;

        if( ( idxtoTest >= aNetItemBuffer.size() )
           || ( NetItemRef->GetNet() != aNetItemBuffer[idxtoTest]->GetNet() ) )
        {
            /* Net analyse: mise a jour de m_FlagOfConnection */
            NetEnd = idxtoTest;

            /* set m_FlagOfConnection member to StateFlag for all items of this net: */
            for( unsigned kk = NetStart; kk < NetEnd; kk++ )
                aNetItemBuffer[kk]->m_FlagOfConnection = StateFlag;

            if( idxtoTest >= aNetItemBuffer.size() )
                return;

            /* Start Analysis next Net */
            StateFlag = UNCONNECTED;
            NetStart  = idxtoTest;
            continue;
        }

        /* test the current item: if this is a pin and if the reference item is also a pin,
         * then 2 pins are connected, so set StateFlag to PAD_CONNECT (can be already done)
         * Of course, if the current item is a no connect symbol, set StateFlag to NOCONNECT_SYMBOL_PRESENT
         * to inhibit error diags. However if StateFlag is already set to PAD_CONNECT
         * this state is kept (the no connect symbol was surely an error and an ERC will report this)
         */
        for( ; ; idxtoTest++ )
        {
            if( ( idxtoTest >= aNetItemBuffer.size() )
               || ( NetItemRef->GetNet() != aNetItemBuffer[idxtoTest]->GetNet() ) )
                break;

            switch( aNetItemBuffer[idxtoTest]->m_Type )
            {
            case NET_ITEM_UNSPECIFIED:
                wxMessageBox(wxT("BuildNetListBase() error"));
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
