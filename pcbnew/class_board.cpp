/*******************************************/
/* class_board.cpp - BOARD class functions */
/*******************************************/
#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"


/*****************/
/* Class BOARD: */
/*****************/

/* Constructor */
BOARD::BOARD( EDA_BaseStruct* parent, WinEDA_BasePcbFrame* frame ) :
    BOARD_ITEM( (BOARD_ITEM*) parent, TYPEPCB )
{
    m_PcbFrame   = frame;
    m_Status_Pcb = 0;                   // Mot d'etat: Bit 1 = Chevelu calcule
    m_NbNets = 0;                       // Nombre de nets (equipotentielles)
    m_BoardSettings = &g_DesignSettings;
    m_NbPads  = 0;                      // nombre total de pads
    m_NbNodes = 0;                      // nombre de pads connectes
    m_NbLinks = 0;                      // nombre de chevelus (donc aussi nombre
                                        // minimal de pistes a tracer
    m_NbSegmTrack = 0;                  // nombre d'elements de type segments de piste
    m_NbSegmZone  = 0;                  // nombre d'elements de type segments de zone
    m_NbNoconnect = 0;                  // nombre de chevelus actifs
    m_NbLoclinks  = 0;                  // nb ratsnest local

    m_Drawings = NULL;                  // pointeur sur liste drawings
    m_Modules  = NULL;                  // pointeur sur liste zone modules
    m_Equipots = NULL;                  // pointeur liste zone equipot
    m_Track    = NULL;                  // pointeur relatif zone piste
    m_Zone             = NULL;          // pointeur tableau zone zones de cuivre
    m_Pads             = NULL;          // pointeur liste d'acces aux pads
    m_Ratsnest         = NULL;          // pointeur liste rats
    m_LocalRatsnest    = NULL;          // pointeur liste rats local
    m_CurrentLimitZone = NULL;          // pointeur liste des EDEGE_ZONES
                                        // de determination des contours de zone
}


/***************/
/* Destructeur */
/***************/
BOARD::~BOARD()
{
    m_Drawings->DeleteStructList();
    m_Drawings = 0;

    m_Modules->DeleteStructList();
    m_Modules = 0;
    
    m_Equipots->DeleteStructList();
    m_Equipots = 0;
    
    m_Track->DeleteStructList();
    m_Track = 0;
    
    m_Zone->DeleteStructList();
    m_Zone = 0;
    
    m_CurrentLimitZone->DeleteStructList();
    m_CurrentLimitZone = 0;
    
    MyFree( m_Pads );
    m_Pads = 0;
    
    MyFree( m_Ratsnest );
    m_Ratsnest = 0;
    
    MyFree( m_LocalRatsnest );
    m_LocalRatsnest = 0;
}


void BOARD::UnLink()
{
    /* Modification du chainage arriere */
    if( Pback )
    {
        if( Pback->Type() == TYPEPCB )
        {
            Pback->Pnext = Pnext;
        }
        else /* Le chainage arriere pointe sur la structure "Pere" */
        {
//			Pback-> = Pnext;
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/* Routines de calcul des nombres de segments pistes et zones */
int BOARD::GetNumSegmTrack()
{
    TRACK* CurTrack = m_Track;
    int    ii = 0;

    for( ; CurTrack != NULL; CurTrack = (TRACK*) CurTrack->Pnext )
        ii++;

    m_NbSegmTrack = ii;
    return ii;
}


int BOARD::GetNumSegmZone()
{
    TRACK* CurTrack = m_Zone;
    int    ii = 0;

    for( ; CurTrack != NULL; CurTrack = (TRACK*) CurTrack->Pnext )
        ii++;

    m_NbSegmZone = ii;
    return ii;
}


// retourne le nombre de connexions manquantes
int BOARD::GetNumNoconnect()
{
    return m_NbNoconnect;
}


// retourne le nombre de chevelus
int BOARD::GetNumRatsnests()
{
    return m_NbLinks;
}


// retourne le nombre de pads a netcode > 0
int BOARD::GetNumNodes()
{
    return m_NbNodes;
}


/***********************************/
bool BOARD::ComputeBoundaryBox()
/***********************************/

/* Determine le rectangle d'encadrement du pcb
 *  Ce rectangle englobe les contours pcb, pads , vias et piste
 *  Sortie:
 *  m_PcbBox
 * 
 *  retourne:
 *      0 si aucun element utile
 *      1 sinon
 */
{
    int             rayon, cx, cy, d, xmin, ymin, xmax, ymax;
    bool            Has_Items = FALSE;
    EDA_BaseStruct* PtStruct;
    DRAWSEGMENT*    ptr;
    TRACK*          Track;

    xmin = ymin = 0x7FFFFFFFl;
    xmax = ymax = -0x7FFFFFFFl;

    /* Analyse des Contours PCB */
    PtStruct = m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->Type() != TYPEDRAWSEGMENT )
            continue;

        ptr = (DRAWSEGMENT*) PtStruct;
        d   = (ptr->m_Width / 2) + 1;
        
        if( ptr->m_Shape == S_CIRCLE )
        {
            cx        = ptr->m_Start.x; cy = ptr->m_Start.y;
            rayon     = (int) hypot( (double) (ptr->m_End.x - cx), (double) (ptr->m_End.y - cy) );
            rayon    += d;
            xmin      = MIN( xmin, cx - rayon );
            ymin      = MIN( ymin, cy - rayon );
            xmax      = MAX( xmax, cx + rayon );
            ymax      = MAX( ymax, cy + rayon );
            Has_Items = TRUE;
        }
        else
        {
            cx        = MIN( ptr->m_Start.x, ptr->m_End.x );
            cy        = MIN( ptr->m_Start.y, ptr->m_End.y );
            xmin      = MIN( xmin, cx - d );
            ymin      = MIN( ymin, cy - d );
            cx        = MAX( ptr->m_Start.x, ptr->m_End.x );
            cy        = MAX( ptr->m_Start.y, ptr->m_End.y );
            xmax      = MAX( xmax, cx + d );
            ymax      = MAX( ymax, cy + d );
            Has_Items = TRUE;
        }
    }

    /* Analyse des Modules  */
    MODULE* module = m_Modules;
    for( ; module != NULL; module = (MODULE*) module->Pnext )
    {
        Has_Items = TRUE;
        xmin = MIN( xmin, ( module->m_Pos.x + module->m_BoundaryBox.GetX() ) );
        ymin = MIN( ymin, ( module->m_Pos.y + module->m_BoundaryBox.GetY() ) );
        xmax = MAX( xmax, module->m_Pos.x + module->m_BoundaryBox.GetRight() );
        ymax = MAX( ymax, module->m_Pos.y + module->m_BoundaryBox.GetBottom() );

        D_PAD* pt_pad = module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
        {
            d    = pt_pad->m_Rayon;
            xmin = MIN( xmin, pt_pad->m_Pos.x - d );
            ymin = MIN( ymin, pt_pad->m_Pos.y - d );
            xmax = MAX( xmax, pt_pad->m_Pos.x + d );
            ymax = MAX( ymax, pt_pad->m_Pos.y + d );
        }
    }

    /* Analyse des segments de piste et zone*/
    for( Track = m_Track; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        d         = (Track->m_Width / 2) + 1;
        cx        = MIN( Track->m_Start.x, Track->m_End.x );
        cy        = MIN( Track->m_Start.y, Track->m_End.y );
        xmin      = MIN( xmin, cx - d );
        ymin      = MIN( ymin, cy - d );
        cx        = MAX( Track->m_Start.x, Track->m_End.x );
        cy        = MAX( Track->m_Start.y, Track->m_End.y );
        xmax      = MAX( xmax, cx + d );
        ymax      = MAX( ymax, cy + d );
        Has_Items = TRUE;
    }

    for( Track = m_Zone; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        d         = (Track->m_Width / 2) + 1;
        cx        = MIN( Track->m_Start.x, Track->m_End.x );
        cy        = MIN( Track->m_Start.y, Track->m_End.y );
        xmin      = MIN( xmin, cx - d );
        ymin      = MIN( ymin, cy - d );
        cx        = MAX( Track->m_Start.x, Track->m_End.x );
        cy        = MAX( Track->m_Start.y, Track->m_End.y );
        xmax      = MAX( xmax, cx + d );
        ymax      = MAX( ymax, cy + d );
        Has_Items = TRUE;
    }

    if( !Has_Items && m_PcbFrame )
    {
        if( m_PcbFrame->m_Draw_Sheet_Ref )
        {
            xmin = ymin = 0;
            xmax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().x;
            ymax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().y;
        }
        else
        {
            xmin = -m_PcbFrame->m_CurrentScreen->ReturnPageSize().x / 2;
            ymin = -m_PcbFrame->m_CurrentScreen->ReturnPageSize().y / 2;
            xmax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().x / 2;
            ymax = m_PcbFrame->m_CurrentScreen->ReturnPageSize().y / 2;
        }
    }

    m_BoundaryBox.SetX( xmin );
    m_BoundaryBox.SetY( ymin );
    m_BoundaryBox.SetWidth( xmax - xmin );
    m_BoundaryBox.SetHeight( ymax - ymin );

    return Has_Items;
}


// virtual, see pcbstruct.h
void BOARD::Display_Infos( WinEDA_DrawFrame* frame )
{
/* Affiche l'etat du PCB : nb de pads, nets , connexions.. */
#define POS_AFF_NBPADS      1
#define POS_AFF_NBVIAS      8
#define POS_AFF_NBNODES     16
#define POS_AFF_NBLINKS     24
#define POS_AFF_NBNETS      32
#define POS_AFF_NBCONNECT   40
#define POS_AFF_NBNOCONNECT 48

    int             nb_vias = 0, ii;
    EDA_BaseStruct* Struct;
    wxString        txt;

    frame->MsgPanel->EraseMsgBox();

    txt.Printf( wxT( "%d" ), m_NbPads );
    Affiche_1_Parametre( frame, POS_AFF_NBPADS, _( "Pads" ), txt, DARKGREEN );

    for( ii = 0, Struct = m_Track; Struct != NULL; Struct = Struct->Pnext )
    {
        ii++;
        if( Struct->Type() == TYPEVIA )
            nb_vias++;
    }

    txt.Printf( wxT( "%d" ), nb_vias );
    Affiche_1_Parametre( frame, POS_AFF_NBVIAS, _( "Vias" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), GetNumNodes() );
    Affiche_1_Parametre( frame, POS_AFF_NBNODES, _( "Nodes" ), txt, DARKCYAN );

    txt.Printf( wxT( "%d" ), m_NbLinks );
    Affiche_1_Parametre( frame, POS_AFF_NBLINKS, _( "Links" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), m_NbNets );
    Affiche_1_Parametre( frame, POS_AFF_NBNETS, _( "Nets" ), txt, RED );

    txt.Printf( wxT( "%d" ), m_NbLinks - GetNumNoconnect() );
    Affiche_1_Parametre( frame, POS_AFF_NBCONNECT, _( "Connect" ), txt, DARKGREEN );

    txt.Printf( wxT( "%d" ), GetNumNoconnect() );
    Affiche_1_Parametre( frame, POS_AFF_NBNOCONNECT, _( "NoConn" ), txt, BLUE );
}


// virtual, see pcbstruct.h     
SEARCH_RESULT BOARD::Visit( INSPECTOR* inspector, const void* testData, 
    const KICAD_T scanTypes[] )
{
    KICAD_T         stype;
    SEARCH_RESULT   result = SEARCH_CONTINUE;
    const KICAD_T*  p = scanTypes;
    bool            done=false;

#if 0 && defined(DEBUG)
    std::cout <<  GetClass().mb_str() << ' ';
#endif    
    
    while( !done )
    {
        stype = *p;
        switch( stype )
        {
        case TYPEPCB:
            result = inspector->Inspect( this, testData );  // inspect me
            // skip over any types handled in the above call.
            ++p;
            break;

        /*  Instances of the requested KICAD_T live in a list, either one
            that I manage, or that my modules manage.  If it's a type managed
            by class MODULE, then simply pass it on to each module's 
            MODULE::Visit() function by way of the 
            IterateForward( m_Modules, ... ) call.
        */ 
            
        case TYPEMODULE:
        case TYPEPAD:
        case TYPETEXTEMODULE:
        case TYPEEDGEMODULE:
            // this calls MODULE::Visit() on each module.
            result = IterateForward( m_Modules, inspector, testData, p );
            // skip over any types handled in the above call.
            for(;;)
            {
                switch( stype = *++p )
                {
                case TYPEMODULE:
                case TYPEPAD:
                case TYPETEXTEMODULE:
                case TYPEEDGEMODULE:
                    continue;
                default:;
                }
                break;
            }
            break;

        case TYPEDRAWSEGMENT:
        case TYPETEXTE:
        case TYPEMARQUEUR:
        case TYPECOTATION:
        case TYPEMIRE:
            result = IterateForward( m_Drawings, inspector, testData, p );
            // skip over any types handled in the above call.
            for(;;)
            {
                switch( stype = *++p )
                {
                case TYPEDRAWSEGMENT:
                case TYPETEXTE:
                case TYPEMARQUEUR:
                case TYPECOTATION:
                case TYPEMIRE:
                    continue;
                default:;
                }
                break;
            }
                ;
            break;

#if 0   // both these are on same list, so we must scan it twice in order to get VIA priority, 
        // using new #else code below.
        // But we are not using separte lists for TRACKs and SEGVIAs, because items are ordered (sortered) in the linked
		// list by netcode AND by physical distance:
		// when created, if a track or via is connected to an existing track or via, it is put in linked list
		// after this existing track or via
		// So usually, connected tracks or vias are grouped in this list
		// So the algorithm (used in rastnest computations) which computes the track connectivity is faster (more than 100 time regarding to
		// a non ordered list) because when it searchs for a connexion, first it tests the near (near in term of linked list) 50 items
		// from the current item (track or via) in test.
		// Usually, because of this sort, a connected item (if exists) is found.
		// If not found (and only in this case) an exhaustive (and time consumming) search is made,
		// but this case is statistically rare.
        case TYPEVIA:
        case TYPETRACK:
            result = IterateForward( m_Track, inspector, testData, p );
            // skip over any types handled in the above call.
            for(;;)
            {
                switch( stype = *++p )
                {
                case TYPEVIA:
                case TYPETRACK:
                    continue;
                default:;
                }
                break;
            }
            break;
#else
        case TYPEVIA:
            result = IterateForward( m_Track, inspector, testData, p );
            ++p;
            break;
            
        case TYPETRACK:
            result = IterateForward( m_Track, inspector, testData, p );
            ++p;
            break;
#endif
            
        case PCB_EQUIPOT_STRUCT_TYPE:
            result = IterateForward( m_Equipots, inspector, testData, p );
            ++p;
            break;            

        case TYPEZONE:
            result = IterateForward( m_Zone, inspector, testData, p );
            ++p;
            break;
            
        case TYPEEDGEZONE:
            result = IterateForward( m_CurrentLimitZone, inspector, testData, p );
            ++p;
            break;            
            
        default:        // catch EOT or ANY OTHER type here and return.
            done = true;
            break;
        }
        
        if( result == SEARCH_QUIT )
            break;
    }

    return result;    
}


/*  now using PcbGeneralLocateAndDisplay()
// see pcbstruct.h     
BOARD_ITEM* BOARD::FindPadOrModule( const wxPoint& refPos, int layer )
{
    class PadOrModule : public INSPECTOR
    {
    public:
        BOARD_ITEM*         found;
        int                 layer;
        int                 layer_mask;
    
        PadOrModule( int alayer ) :
            found(0), layer(alayer), layer_mask( g_TabOneLayerMask[alayer] )
        {}
    
        SEARCH_RESULT Inspect( EDA_BaseStruct* testItem, const void* testData )
        {
            BOARD_ITEM*     item   = (BOARD_ITEM*) testItem;
            const wxPoint&  refPos = *(const wxPoint*) testData;
    
            if( item->Type() == TYPEPAD )
            {
                D_PAD*  pad = (D_PAD*) item;
                if( pad->HitTest( refPos ) )
                {
                    if( layer_mask & pad->m_Masque_Layer ) 
                    {
                        found = item;
                        return SEARCH_QUIT;
                    }
                    else if( !found )
                    {
                        MODULE* parent = (MODULE*) pad->m_Parent;
                        if( IsModuleLayerVisible( parent->GetLayer() ) )
                            found = item;
                    }
                }
            }
            
            else if( item->Type() == TYPEMODULE )
            {
                MODULE* module = (MODULE*) item;

                // consider only visible modules
                if( IsModuleLayerVisible( module->GetLayer() ) )
                {
                    if( module->HitTest( refPos ) )
                    {
                        if( layer == module->GetLayer() )
                        {
                            found = item;
                            return SEARCH_QUIT;
                        }
                        
                        // layer mismatch, save in case we don't find a
                        // future layer match hit.
                        if( !found )
                            found = item;
                    }
                }
            }
            return SEARCH_CONTINUE;
        }
    };
    
    PadOrModule inspector( layer );

    // search only for PADs first, then MODULES, and preferably a layer match
    static const KICAD_T scanTypes[] = { TYPEPAD, TYPEMODULE, EOT };

    // visit this BOARD with the above inspector
    Visit( &inspector, &refPos, scanTypes );
    
    return inspector.found;
}
*/


/**
 * Function FindNet
 * searches for a net with the given netcode.
 * @param anetcode The netcode to search for.
 * @return EQUIPOT* - the net or NULL if not found.
 */
EQUIPOT* BOARD::FindNet( int anetcode ) const
{
    // the first valid netcode is 1.
    // zero is reserved for "no connection" and is not used.
    if( anetcode > 0 )  
    {
        for( EQUIPOT* net = m_Equipots;  net;  net=net->Next() )
        {
            if( net->GetNet() == anetcode )
                return net;
        }
    }
    return NULL;
}



bool BOARD::Save( FILE* aFile ) const
{
    bool        rc = false;
    BOARD_ITEM* item;

    // save the nets
    for( item = m_Equipots;  item;  item=item->Next() )
        if( !item->Save( aFile ) )
            goto out;
    
    // save the modules
    for( item = m_Modules;  item;  item=item->Next() )
        if( !item->Save( aFile ) )
            goto out;
    
    for( item = m_Drawings;  item;  item=item->Next() )
    {
        switch( item->Type() )
        {
        case TYPETEXTE:
        case TYPEDRAWSEGMENT:
        case TYPEMIRE:
        case TYPECOTATION:
            if( !item->Save( aFile ) )
                goto out;
            break;

        case TYPEMARQUEUR:      // do not save MARKERs, they can be regenerated easily 
            break;

        default:
            // future: throw exception here
#if defined(DEBUG)            
            printf( "BOARD::Save() ignoring m_Drawings type %d\n", item->Type() );
#endif            
            break;
        }
    }
    
    // save the tracks & vias
    fprintf( aFile, "$TRACK\n" );
    for( item = m_Track;  item;  item=item->Next() )
        if( !item->Save( aFile ) )
            goto out;
    fprintf( aFile, "$EndTRACK\n" );

    // save the zones
    fprintf( aFile, "$ZONE\n" );
    for( item = m_Zone;  item;  item=item->Next() )
        if( !item->Save( aFile ) )
            goto out;
    fprintf( aFile, "$EndZONE\n" );
    
    // save the zone edges
/*    
    if( m_CurrentLimitZone )
    {
        fprintf( aFile, "$ZONE_EDGE\n" );
        for( item = m_CurrentLimitZone;  item;  item=item->Next() )
            if( !item->Save( aFile ) )
                goto out;
        fprintf( aFile, "$EndZONE_EDGE\n" );
    }
*/    
    
    if( fprintf( aFile, "$EndBOARD\n" ) != sizeof("$EndBOARD\n")-1 )
        goto out;

    rc = true;  // wrote all OK 
   
out:
    return rc;    
}



#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level 
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void BOARD::Show( int nestLevel, std::ostream& os )
{
    EDA_BaseStruct* p;
    
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";

    // specialization of the output:
    NestedSpace( nestLevel+1, os ) << "<modules>\n";
    p = m_Modules;
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</modules>\n";

    NestedSpace( nestLevel+1, os ) << "<pdrawings>\n";
    p = m_Drawings;
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</pdrawings>\n";
    
    NestedSpace( nestLevel+1, os ) << "<nets>\n";
    p = m_Equipots;
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</nets>\n";

    NestedSpace( nestLevel+1, os ) << "<tracks>\n";
    p = m_Track;    
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</tracks>\n";

    NestedSpace( nestLevel+1, os ) << "<zones>\n";
    p = m_Zone;    
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</zones>\n";

    NestedSpace( nestLevel+1, os ) << "<zoneedges>\n";
    p = m_CurrentLimitZone;
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</zoneedges>\n";
    
    p = m_Son;
    for( ; p;  p = p->Pnext )
    {
        p->Show( nestLevel+1, os );
    }
    
    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}


/* wrote this before discovering ReturnPcbLayerName()
const char* BOARD::ShowLayer( int aLayer )
{
    const char* rs;
    
    switch( aLayer )
    {
    case LAYER_COPPER_LAYER_N:        rs = "cu";              break;
    case LAYER_N_2:             rs = "layer2";          break;             
    case LAYER_N_3:             rs = "layer3";          break;
    case LAYER_N_4:             rs = "layer4";          break;
    case LAYER_N_5:             rs = "layer5";          break;
    case LAYER_N_6:             rs = "layer6";          break;
    case LAYER_N_7:             rs = "layer7";          break;
    case LAYER_N_8:             rs = "layer8";          break;
    case LAYER_N_9:             rs = "layer9";          break;
    case LAYER_N_10:            rs = "layer10";         break;
    case LAYER_N_11:            rs = "layer11";         break;
    case LAYER_N_12:            rs = "layer12";         break;
    case LAYER_N_13:            rs = "layer13";         break;
    case LAYER_N_14:            rs = "layer14";         break;
    case LAYER_N_15:            rs = "layer15";         break;
    case LAYER_CMP_N:           rs = "cmp";             break;
    case ADHESIVE_N_CU:         rs = "cu/adhesive";     break;
    case ADHESIVE_N_CMP:        rs = "cmp/adhesive";    break; 
    case SOLDERPASTE_N_CU:      rs = "cu/sldrpaste";    break;     
    case SOLDERPASTE_N_CMP:     rs = "cmp/sldrpaste";   break;
    case SILKSCREEN_N_CU:       rs = "cu/silkscreen";   break;
    case SILKSCREEN_N_CMP:      rs = "cmp/silkscreen";  break;
    case SOLDERMASK_N_CU:       rs = "cu/sldrmask";     break;
    case SOLDERMASK_N_CMP:      rs = "cmp/sldrmask";    break;
    case DRAW_N:                rs = "drawing";         break;
    case COMMENT_N:             rs = "comment";         break;
    case ECO1_N:                rs = "eco_1";           break;
    case ECO2_N:                rs = "eco_2";           break;
    case EDGE_N:                rs = "edge";            break;
    default:                    rs = "???";             break;        
    }
    
    return rs;
}
*/

#endif
