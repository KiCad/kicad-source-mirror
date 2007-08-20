/**********************************************************************/
/* fonctions membres des classes utilisees dans pcbnew (voir pcbstruct.h */
/*	  sauf routines relatives aux pistes (voir class_track.cpp)		  */
/**********************************************************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "protos.h"
#include "trigo.h"


/**************************************************************/
void EDA_BaseStruct::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/**************************************************************/

/* fonction virtuelle de placement: non utilisee en pcbnew (utilisee eeschema)
 *  ---- A mieux utiliser (TODO...)
 */
{
}


/**********************/
/* Classe EDGE_ZONE */
/**********************/

/* Classe EDGE_ZONE: constructeur */
EDGE_ZONE::EDGE_ZONE( EDA_BaseStruct* parent ) :
    DRAWSEGMENT( parent, TYPEEDGEZONE )
{
}


/* Effacement memoire de la structure */
EDGE_ZONE:: ~EDGE_ZONE( void )
{
}


/**********************/
/* Classe DRAWSEGMENT */
/**********************/

/* Classe DRAWSEGMENT: constructeur */
DRAWSEGMENT::DRAWSEGMENT( EDA_BaseStruct* StructFather, DrawStructureType idtype ) :
    EDA_BaseLineStruct( StructFather, idtype )
{
    m_Flags = m_Shape = m_Type = m_Angle = 0;
}


/* Effacement memoire de la structure */
DRAWSEGMENT:: ~DRAWSEGMENT( void )
{
}


void DRAWSEGMENT::UnLink( void )
{
    /* Modification du chainage arriere */
    if( Pback )
    {
        if( Pback->m_StructType != TYPEPCB )
        {
            Pback->Pnext = Pnext;
        }
        else /* Le chainage arriere pointe sur la structure "Pere" */
        {
            ( (BOARD*) Pback )->m_Drawings = Pnext;
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/*******************************************/
void DRAWSEGMENT::Copy( DRAWSEGMENT* source )
/*******************************************/
{
    m_Type      = source->m_Type;
    m_Layer     = source->m_Layer;
    m_Width     = source->m_Width;
    m_Start     = source->m_Start;
    m_End       = source->m_End;
    m_Shape     = source->m_Shape;
    m_Angle     = source->m_Angle;
    m_TimeStamp = source->m_TimeStamp;
}


/********************************************************/
bool DRAWSEGMENT::WriteDrawSegmentDescr( FILE* File )
/********************************************************/
{
    if( GetState( DELETED ) )
        return FALSE;

    fprintf( File, "$DRAWSEGMENT\n" );
    fprintf( File, "Po %d %d %d %d %d %d\n",
             m_Shape,
             m_Start.x, m_Start.y,
             m_End.x, m_End.y, m_Width );
    fprintf( File, "De %d %d %d %lX %X\n",
            m_Layer, m_Type, m_Angle,
            m_TimeStamp, ReturnStatus() );
    fprintf( File, "$EndDRAWSEGMENT\n" );
    return TRUE;
}


/******************************************************************/
bool DRAWSEGMENT::ReadDrawSegmentDescr( FILE* File, int* LineNum )
/******************************************************************/

/* Lecture de la description de 1 segment type Drawing PCB
 */
{
    char Line[2048];


    while( GetLine( File, Line, LineNum ) != NULL )
    {
        if( strnicmp( Line, "$End", 4 ) == 0 )
            return TRUE;                                /* fin de liste */
        if( Line[0] == 'P' )
        {
            sscanf( Line + 2, " %d %d %d %d %d %d",
                    &m_Shape, &m_Start.x, &m_Start.y,
                    &m_End.x, &m_End.y, &m_Width );
            if( m_Width < 0 )
                m_Width = 0;
        }

        if( Line[0] == 'D' )
        {
            int status;
            sscanf( Line + 2, " %d %d %d %lX %X",
                    &m_Layer, &m_Type, &m_Angle,
                    &m_TimeStamp, &status );

            if( m_Layer < FIRST_NO_COPPER_LAYER )
                m_Layer = FIRST_NO_COPPER_LAYER;
            if( m_Layer > LAST_NO_COPPER_LAYER )
                m_Layer = LAST_NO_COPPER_LAYER;

            SetState( status, ON );
        }
    }

    return FALSE;
}


// see pcbstruct.h
void DRAWSEGMENT::Display_Infos( WinEDA_DrawFrame* frame )
{
    int      itype;
    wxString msg;

    frame->MsgPanel->EraseMsgBox();

    itype = m_Type & 0x0F;

    msg = wxT( "DRAWING" );

    Affiche_1_Parametre( frame, 1, _( "Type" ), msg, DARKCYAN );

    Affiche_1_Parametre( frame, 16, _( "Shape" ), wxEmptyString, RED );
    
    if( m_Shape == S_CIRCLE )
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Circle" ), RED );
    
    else if( m_Shape == S_ARC )
    {
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "  Arc  " ), RED );
        msg.Printf( wxT( "%d" ), m_Angle );
        Affiche_1_Parametre( frame, 32, wxT( " l.arc " ), msg, RED );
    }
    else
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Segment" ), RED );

    Affiche_1_Parametre( frame, 48, _( "Layer" ), 
             ReturnPcbLayerName( m_Layer ), BROWN );

    /* Affiche Epaisseur */
    valeur_param( (unsigned) m_Width, msg );
    Affiche_1_Parametre( frame, 60, _( "Width" ), msg, DARKCYAN );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param ref_pos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool DRAWSEGMENT::HitTest( const wxPoint& ref_pos )
{
    int ux0 = m_Start.x; 
    int uy0 = m_Start.y;
    
    /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
    int dx = m_End.x - ux0; 
    int dy = m_End.y - uy0;
    
    int spot_cX = ref_pos.x - ux0; 
    int spot_cY = ref_pos.y - uy0;

    if( m_Shape==S_CIRCLE || m_Shape==S_ARC )
    {
        int rayon, dist, stAngle, endAngle, mouseAngle;

        rayon = (int) hypot( (double) (dx), (double) (dy) );
        dist  = (int) hypot( (double) (spot_cX), (double) (spot_cY) );
        
        if( abs( rayon - dist ) <= (m_Width / 2) )
        {
            if( m_Shape == S_CIRCLE )
                return true;
            
            /* pour un arc, controle complementaire */
            mouseAngle = (int) ArcTangente( spot_cY, spot_cX );
            stAngle    = (int) ArcTangente( dy, dx );
            endAngle   = stAngle + m_Angle;

            if( endAngle > 3600 )
            {
                stAngle  -= 3600; 
                endAngle -= 3600;
            }
            
            if( mouseAngle >= stAngle  &&  mouseAngle <= endAngle )
                return true;
        }
    }
    else
    {
        if( DistanceTest( m_Width / 2, dx, dy, spot_cX, spot_cY ) )
            return true;
    }
    return false;
}



/*******************/
/* Classe MARQUEUR */
/*******************/

MARQUEUR::MARQUEUR( EDA_BaseStruct* StructFather ) :
    EDA_BaseStruct( StructFather, TYPEMARQUEUR )
{
    m_Bitmap = NULL;
    m_Type   = 0;
    m_Color  = RED;
}


/* Effacement memoire de la structure */
MARQUEUR:: ~MARQUEUR( void )
{
}


/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
void MARQUEUR::UnLink( void )
{
    /* Modification du chainage arriere */
    if( Pback )
    {
        if( Pback->m_StructType != TYPEPCB )
        {
            Pback->Pnext = Pnext;
        }
        else /* Le chainage arriere pointe sur la structure "Pere" */
        {
            ( (BOARD*) Pback )->m_Drawings = Pnext;
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/**************************************************/
/* Class SCREEN: classe de gestion d'un affichage */
/***************************************************/
/* Constructeur de SCREEN */
PCB_SCREEN::PCB_SCREEN( int idscreen ) : BASE_SCREEN( TYPESCREEN )
{
    int zoom_list[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 0 };

    m_Type = idscreen;
    SetGridList( g_GridList );
    SetZoomList( zoom_list );
    Init();
}


/***************************/
PCB_SCREEN::~PCB_SCREEN( void )
/***************************/
{
}


/*************************/
void PCB_SCREEN::Init( void )
/*************************/
{
    InitDatas();
    m_Active_Layer       = CUIVRE_N;    /* ref couche active 0.. 31 */
    m_Route_Layer_TOP    = CMP_N;       /* ref couches par defaut pour vias (Cu.. Cmp) */
    m_Route_Layer_BOTTOM = CUIVRE_N;
    m_Zoom = 128;                       /* valeur */
    m_Grid = wxSize( 500, 500 );        /* pas de la grille en 1/10000 "*/
}


/*************************/
/* class DISPLAY_OPTIONS */
/*************************/

/*
 *  Options diverses d'affichage �l'�ran:
 */

DISPLAY_OPTIONS::DISPLAY_OPTIONS( void )
{
    DisplayPadFill   = TRUE;
    DisplayPadNum    = TRUE;
    DisplayPadNoConn = TRUE;
    DisplayPadIsol   = TRUE;

    DisplayModEdge      = TRUE;
    DisplayModText      = TRUE;
    DisplayPcbTrackFill = TRUE; /* FALSE = sketch , TRUE = rempli */
    DisplayTrackIsol    = FALSE;
    m_DisplayViaMode    = VIA_HOLE_NOT_SHOW;

    DisplayPolarCood = TRUE;
    DisplayZones     = TRUE;
    Show_Modules_Cmp = TRUE;
    Show_Modules_Cu  = TRUE;

    DisplayDrawItems    = TRUE;
    ContrastModeDisplay = FALSE;
}


/*****************************************************/
EDA_BoardDesignSettings::EDA_BoardDesignSettings( void )
/*****************************************************/

// Default values for designing boards
{
    int ii;
    int default_layer_color[32] = {
        GREEN,     LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, RED,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY,
        MAGENTA,   CYAN,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY
    };

    m_CopperLayerCount = 2;             // Default design is a double sided board
    m_ViaDrill = 250;                   // via drill (for the entire board)
    m_CurrentViaSize    = 450;          // Current via size
    m_CurrentViaType    = VIA_NORMALE;  /* via type (BLIND, TROUGHT ...), bits 1 and 2 (not 0 and 1)*/
    m_CurrentTrackWidth = 170;          // current track width
    for( ii = 0; ii < HIST0RY_NUMBER; ii++ )
    {
        m_TrackWidhtHistory[ii] = 0;    // Last HIST0RY_NUMBER used track widths
        m_ViaSizeHistory[ii]    = 0;    // Last HIST0RY_NUMBER used via sizes
    }

    m_DrawSegmentWidth = 100;               // current graphic line width (not EDGE layer)
    m_EdgeSegmentWidth = 100;               // current graphic line width (EDGE layer only)
    m_PcbTextWidth   = 100;                 // current Pcb (not module) Text width
    m_PcbTextSize    = wxSize( 500, 500 );  // current Pcb (not module) Text size
    m_TrackClearence = 100;                 // track to track and track to pads clearance
    m_ZoneClearence  = 150;                 // zone to track and zone to pads clearance
    m_MaskMargin = 150;                     // Solder mask margin
    /* Color options for screen display of the Printed Board: */
    m_PcbGridColor = DARKGRAY;              // Grid color
    for( ii = 0; ii < 32; ii++ )
        m_LayerColor[ii] = default_layer_color[ii];

    // Layer colors (tracks and graphic items)
    m_ViaColor[VIA_BORGNE]   = CYAN;
    m_ViaColor[VIA_ENTERREE] = BROWN;
    m_ViaColor[VIA_NORMALE]  = WHITE;
    m_ModuleTextCMPColor = LIGHTGRAY;   // Text module color for modules on the COMPONENT layer
    m_ModuleTextCUColor  = MAGENTA;     // Text module color for modules on the COPPER layer
    m_ModuleTextNOVColor = DARKGRAY;    // Text module color for "invisible" texts (must be BLACK if really not displayed)
    m_AnchorColor   = BLUE;             // Anchor color for modules and texts
    m_PadCUColor    = GREEN;            // Pad color for the COMPONENT side of the pad
    m_PadCMPColor   = RED;              // Pad color for the COPPER side of the pad
    m_RatsnestColor = WHITE;            // Ratsnest color
}
