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
    EDA_BaseStruct( parent, TYPEPCB )
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
BOARD::~BOARD( void )
{
}


void BOARD::UnLink( void )
{
    /* Modification du chainage arriere */
    if( Pback )
    {
        if( Pback->m_StructType == TYPEPCB )
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
int BOARD::GetNumSegmTrack( void )
{
    TRACK* CurTrack = m_Track;
    int    ii = 0;

    for( ; CurTrack != NULL; CurTrack = (TRACK*) CurTrack->Pnext )
        ii++;

    m_NbSegmTrack = ii;
    return ii;
}


int BOARD::GetNumSegmZone( void )
{
    TRACK* CurTrack = m_Zone;
    int    ii = 0;

    for( ; CurTrack != NULL; CurTrack = (TRACK*) CurTrack->Pnext )
        ii++;

    m_NbSegmZone = ii;
    return ii;
}


// retourne le nombre de connexions manquantes
int BOARD::GetNumNoconnect( void )
{
    return m_NbNoconnect;
}


// retourne le nombre de chevelus
int BOARD::GetNumRatsnests( void )
{
    return m_NbLinks;
}


// retourne le nombre de pads a netcode > 0
int BOARD::GetNumNodes( void )
{
    return m_NbNodes;
}


/***********************************/
bool BOARD::ComputeBoundaryBox( void )
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
        if( PtStruct->m_StructType != TYPEDRAWSEGMENT )
            continue;

        ptr = (DRAWSEGMENT*) PtStruct;
        d   = (ptr->m_Width / 2) + 1;
        
        if( ptr->m_Shape == S_CIRCLE )
        {
            cx        = ptr->m_Start.x; cy = ptr->m_Start.y;
            rayon     = (int) hypot( (double) (ptr->m_End.x - cx), (double) (ptr->m_End.y - cy) );
            rayon    += d;
            xmin      = min( xmin, cx - rayon );
            ymin      = min( ymin, cy - rayon );
            xmax      = max( xmax, cx + rayon );
            ymax      = max( ymax, cy + rayon );
            Has_Items = TRUE;
        }
        else
        {
            cx        = min( ptr->m_Start.x, ptr->m_End.x );
            cy        = min( ptr->m_Start.y, ptr->m_End.y );
            xmin      = min( xmin, cx - d );
            ymin      = min( ymin, cy - d );
            cx        = max( ptr->m_Start.x, ptr->m_End.x );
            cy        = max( ptr->m_Start.y, ptr->m_End.y );
            xmax      = max( xmax, cx + d );
            ymax      = max( ymax, cy + d );
            Has_Items = TRUE;
        }
    }

    /* Analyse des Modules  */
    MODULE* module = m_Modules;
    for( ; module != NULL; module = (MODULE*) module->Pnext )
    {
        Has_Items = TRUE;
        xmin = min( xmin, ( module->m_Pos.x + module->m_BoundaryBox.GetX() ) );
        ymin = min( ymin, ( module->m_Pos.y + module->m_BoundaryBox.GetY() ) );
        xmax = max( xmax, module->m_Pos.x + module->m_BoundaryBox.GetRight() );
        ymax = max( ymax, module->m_Pos.y + module->m_BoundaryBox.GetBottom() );

        D_PAD* pt_pad = module->m_Pads;
        for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
        {
            d    = pt_pad->m_Rayon;
            xmin = min( xmin, pt_pad->m_Pos.x - d );
            ymin = min( ymin, pt_pad->m_Pos.y - d );
            xmax = max( xmax, pt_pad->m_Pos.x + d );
            ymax = max( ymax, pt_pad->m_Pos.y + d );
        }
    }

    /* Analyse des segments de piste et zone*/
    for( Track = m_Track; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        d         = (Track->m_Width / 2) + 1;
        cx        = min( Track->m_Start.x, Track->m_End.x );
        cy        = min( Track->m_Start.y, Track->m_End.y );
        xmin      = min( xmin, cx - d );
        ymin      = min( ymin, cy - d );
        cx        = max( Track->m_Start.x, Track->m_End.x );
        cy        = max( Track->m_Start.y, Track->m_End.y );
        xmax      = max( xmax, cx + d );
        ymax      = max( ymax, cy + d );
        Has_Items = TRUE;
    }

    for( Track = m_Zone; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        d         = (Track->m_Width / 2) + 1;
        cx        = min( Track->m_Start.x, Track->m_End.x );
        cy        = min( Track->m_Start.y, Track->m_End.y );
        xmin      = min( xmin, cx - d );
        ymin      = min( ymin, cy - d );
        cx        = max( Track->m_Start.x, Track->m_End.x );
        cy        = max( Track->m_Start.y, Track->m_End.y );
        xmax      = max( xmax, cx + d );
        ymax      = max( ymax, cy + d );
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
SEARCH_RESULT BOARD::Visit( INSPECTOR* inspector, const void* testData, 
    const KICAD_T scanTypes[] )
{
    KICAD_T         stype;
    SEARCH_RESULT   result = SEARCH_CONTINUE;
    const KICAD_T*  p = scanTypes;
    
    while( (stype = *p++) != EOT )
    {
        switch( stype )
        {
        case TYPEPCB:
            result = inspector->Inspect( this, testData );  // inspect me
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
            result = IterateForward( m_Modules, inspector, testData, scanTypes );
            break;

        case TYPEDRAWSEGMENT:
        case TYPETEXTE:
        case TYPEMARQUEUR:
        case TYPECOTATION:
        case TYPEMIRE:
            result = IterateForward( m_Drawings, inspector, testData, scanTypes );
            break;
            
        case TYPEVIA:
        case TYPETRACK:
            result = IterateForward( m_Track, inspector, testData, scanTypes );
            break;            
            
        case PCB_EQUIPOT_STRUCT_TYPE:
            result = IterateForward( m_Equipots, inspector, testData, scanTypes );
            break;            

        case TYPEZONE:
            result = IterateForward( m_Zone, inspector, testData, scanTypes );
            break;
            
        case TYPEEDGEZONE:
            result = IterateForward( m_CurrentLimitZone, inspector, testData, scanTypes );
            break;            
            
        default:
            break;
        }
        
        if( result == SEARCH_QUIT )
            break;
    }

    return result;    
}


// see pcbstruct.h     
EDA_BaseStruct* BOARD::FindPadOrModule( const wxPoint& refPos, int layer )
{
    class PadOrModule : public INSPECTOR
    {
    public:
        EDA_BaseStruct*     found;
        int                 layer;
    
        PadOrModule( int alayer ) :
            found(0), layer(alayer)
        {}
    
        SEARCH_RESULT Inspect( EDA_BaseStruct* testItem, const void* testData )
        {
            const wxPoint& refPos =  *(const wxPoint*) testData;
    
            if( testItem->m_StructType == TYPEPAD )
            {
                if( testItem->HitTest( refPos ) )
                {
                    found = testItem;
                    return SEARCH_QUIT;
                }
            }
            
            else if( testItem->m_StructType == TYPEMODULE )
            {
                int mlayer = ((MODULE*)testItem)->m_Layer; 

                // consider only visible modules
                if( IsModuleLayerVisible( mlayer ) )
                {
                    if( testItem->HitTest( refPos ) )
                    {
                        if( layer == mlayer )
                        {
                            found = testItem;
                            return SEARCH_QUIT;
                        }
                        
                        // layer mismatch, save in case we don't find a
                        // future layer match hit.
                        if( !found )
                            found = testItem;
                    }
                }
            }
            return SEARCH_CONTINUE;
        }
    };
    
    PadOrModule inspector( layer );

    // search only for PADs first, then MODULES, and preferably a layer match
    static const KICAD_T scanTypes[] = { TYPEPAD, TYPEMODULE, EOT };

    IterateForward( m_Modules, &inspector, &refPos, scanTypes );
    
    return inspector.found;
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

    NestedSpace( nestLevel+1, os ) << "<edgezones>\n";
    p = m_CurrentLimitZone;
    for( ; p; p = p->Pnext )
        p->Show( nestLevel+2, os );
    NestedSpace( nestLevel+1, os ) << "</edgezones>\n";
    
    p = m_Son;
    for( ; p;  p = p->Pnext )
    {
        p->Show( nestLevel+1, os );
    }
    
    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
