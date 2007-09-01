/****************************************************/
/* class_module.cpp : fonctions de la classe MODULE */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "wxstruct.h"
#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#ifdef PCBNEW
#include "autorout.h"
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"

#endif

#include "protos.h"

/************************************************************************/
/* Class TEXTE_MODULE classe de base des elements type Texte sur module */
/************************************************************************/

/* Constructeur de TEXTE_MODULE */
TEXTE_MODULE::TEXTE_MODULE( MODULE* parent, int text_type ) :
    BOARD_ITEM( parent, TYPETEXTEMODULE )
{
    MODULE* Module = (MODULE*) m_Parent;

    m_NoShow = 0;               /* visible */
    m_Type   = text_type;       /* Reference */
    if( (m_Type != TEXT_is_REFERENCE) && (m_Type != TEXT_is_VALUE) )
        m_Type = TEXT_is_DIVERS;

    m_Size.x = m_Size.y = 400; m_Width = 120;   /* dimensions raisonnables par defaut */
    m_Orient = 0;                               /* en 1/10 degre */
    m_Miroir = 1;                               // Mode normal (pas de miroir)
    m_Unused = 0;
    
    SetLayer( SILKSCREEN_N_CMP );
    if( Module && (Module->Type() == TYPEMODULE) )
    {
        m_Pos   = Module->m_Pos;

        int moduleLayer = Module->GetLayer();
        
        if( moduleLayer == CUIVRE_N )
            SetLayer( SILKSCREEN_N_CU );
        else if( moduleLayer == CMP_N )
            SetLayer( SILKSCREEN_N_CMP );
        else 
            SetLayer( moduleLayer );
        
        if(  moduleLayer == SILKSCREEN_N_CU
          || moduleLayer == ADHESIVE_N_CU
          || moduleLayer == CUIVRE_N )
        {
            m_Miroir = 0;
        }
    }
}


TEXTE_MODULE::~TEXTE_MODULE()
{
}


void TEXTE_MODULE::Copy( TEXTE_MODULE* source )      // copy structure
{
    if( source == NULL )
        return;

    m_Pos   = source->m_Pos;
    SetLayer( source->GetLayer() );

    m_Miroir = source->m_Miroir;        // vue normale / miroir
    m_NoShow = source->m_NoShow;        // 0: visible 1: invisible
    m_Type   = source->m_Type;          // 0: ref,1: val, autre = 2..255
    m_Orient = source->m_Orient;        // orientation en 1/10 degre
    m_Pos0   = source->m_Pos0;          // coord du debut du texte /ancre, orient 0

    m_Size  = source->m_Size;
    m_Width = source->m_Width;

    m_Text = source->m_Text;
}


/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
void TEXTE_MODULE::UnLink()
{
    /* Modification du chainage arriere */
    if( Pback )
    {
        if( Pback->Type() != TYPEMODULE )
        {
            Pback->Pnext = Pnext;
        }
        else /* Le chainage arriere pointe sur la structure "Pere" */
        {
            ( (MODULE*) Pback )->m_Drawings = (BOARD_ITEM*) Pnext;
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/******************************************/
int TEXTE_MODULE:: GetLength()
/******************************************/
{
    return m_Text.Len();
}


/******************************************/
void TEXTE_MODULE:: SetWidth( int new_width )
/******************************************/
{
    m_Width = new_width;
}


// mise a jour des coordonn�s absolues pour affichage
void TEXTE_MODULE:: SetDrawCoord()
{
    MODULE* Module = (MODULE*) m_Parent;

    m_Pos = m_Pos0;

    if( Module == NULL )
        return;

    int angle = Module->m_Orient;
    NORMALIZE_ANGLE_POS( angle );

    RotatePoint( &m_Pos.x, &m_Pos.y, angle );
    m_Pos.x += Module->m_Pos.x;
    m_Pos.y += Module->m_Pos.y;
}


// mise a jour des coordonn�s relatives au module
void TEXTE_MODULE:: SetLocalCoord()
{
    MODULE* Module = (MODULE*) m_Parent;

    if( Module == NULL )
        return;

    m_Pos0.x = m_Pos.x - Module->m_Pos.x;
    m_Pos0.y = m_Pos.y - Module->m_Pos.y;

    int angle = Module->m_Orient;
    NORMALIZE_ANGLE_POS( angle );

    RotatePoint( &m_Pos0.x, &m_Pos0.y, -angle );
}


/* locate functions */
bool TEXTE_MODULE::HitTest( const wxPoint& posref )
{
    int     mX, mY, dx, dy;
    MODULE* Module = (MODULE*) m_Parent;
    int     angle  = m_Orient;

    if( Module )
        angle += Module->m_Orient;

    dx = ( m_Size.x * GetLength() ) / 2;
    dy = m_Size.y / 2;
    dx = ( (dx * 10) / 9 ) + m_Width; /* Facteur de forme des lettres : 10/9 */

    /* le point de reference est tourn�de - angle
     *  pour se ramener a un rectangle de reference horizontal */
    mX = posref.x - m_Pos.x; 
    mY = posref.y - m_Pos.y;
    
    RotatePoint( &mX, &mY, -angle );
    
    /* le point de reference est-il dans ce rectangle */
    if( ( abs( mX ) <= abs( dx ) ) && ( abs( mY ) <= abs( dy ) ) )
    {
        return true;
    }
    return false;
}


/******************************************************************************************/
void TEXTE_MODULE::Draw( WinEDA_DrawPanel* panel, wxDC* DC, wxPoint offset, int draw_mode )
/******************************************************************************************/

/* trace 1 texte de module
 *  Utilise la police definie dans grfonte.h
 *  (Se reporter a ce fichier pour les explications complementaires)
 *  offset = offset de trace ( reference au centre du texte)
 *  draw_mode = GR_OR, GR_XOR..
 */
{
    int                  zoom;
    int                  width, color, orient, miroir;
    wxSize               size;
    wxPoint              pos; // Centre du texte
    PCB_SCREEN*          screen;
    WinEDA_BasePcbFrame* frame;
    MODULE*              Module = (MODULE*) m_Parent;


    if( panel == NULL )
        return;

    screen = (PCB_SCREEN*) panel->GetScreen();
    frame  = (WinEDA_BasePcbFrame*) panel->m_Parent;
    zoom   = screen->GetZoom();

    pos.x = m_Pos.x - offset.x;
    pos.y = m_Pos.y - offset.y;

    size   = m_Size;
    orient = GetDrawRotation();
    miroir = m_Miroir & 1; // = 0 si vu en miroir
    width  = m_Width;
    
    if( (frame->m_DisplayModText == FILAIRE) || ( (width / zoom) < L_MIN_DESSIN ) )
        width = 0;
    else if( frame->m_DisplayModText == SKETCH )
        width = -width;

    GRSetDrawMode( DC, draw_mode );

    /* trace du centre du texte */
    if( (g_AnchorColor & ITEM_NOT_SHOW) == 0 )
    {
        int anchor_size = 2 * zoom;
        GRLine( &panel->m_ClipBox, DC,
                pos.x - anchor_size, pos.y,
                pos.x + anchor_size, pos.y, 0, g_AnchorColor );
        GRLine( &panel->m_ClipBox, DC,
                pos.x, pos.y - anchor_size,
                pos.x, pos.y + anchor_size, 0, g_AnchorColor );
    }

    color = g_DesignSettings.m_LayerColor[Module->GetLayer()];

    if( Module && Module->GetLayer() == CUIVRE_N )
        color = g_ModuleTextCUColor;
    
    else if( Module && Module->GetLayer() == CMP_N )
        color = g_ModuleTextCMPColor;

    if( (color & ITEM_NOT_SHOW) != 0 )
        return;

    if( m_NoShow )
        color = g_ModuleTextNOVColor;
    
    if( (color & ITEM_NOT_SHOW) != 0 )
        return;

    /* Si le texte doit etre mis en miroir: modif des parametres */
    if( miroir == 0 )
        size.x = -size.x;

    /* Trace du texte */
    DrawGraphicText( panel, DC, pos, color, m_Text,
                     orient, size, GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, width );
}


/******************************************/
int TEXTE_MODULE::GetDrawRotation()
/******************************************/

/* Return text rotation for drawings and plotting
 */
{
    int     rotation;
    MODULE* Module = (MODULE*) m_Parent;

    rotation = m_Orient;
    if( Module )
        rotation += Module->m_Orient;

    NORMALIZE_ANGLE_POS( rotation );

//	if( (rotation > 900 ) && (rotation < 2700 ) ) rotation -= 1800;	// For angle = 0 .. 180 deg
    while( rotation > 900 )
        rotation -= 1800;

    // For angle = -90 .. 90 deg

    return rotation;
}


// see class_text_mod.h 
void TEXTE_MODULE::Display_Infos( WinEDA_DrawFrame* frame )  
{
    wxString msg, Line;
    int      ii;
    
    MODULE* module = (MODULE*) m_Parent;
    
    if( !module )
        return;

    static const wxString text_type_msg[3] = { 
        _( "Ref." ), _( "Value" ), _( "Text" ) };
    
    frame->MsgPanel->EraseMsgBox();

    Line = module->m_Reference->m_Text;
    Affiche_1_Parametre( frame, 1, _( "Module" ), Line, DARKCYAN );

    Line = m_Text;
    Affiche_1_Parametre( frame, 10, _( "Text" ), Line, YELLOW );

    ii = m_Type; 
    if( ii > 2 )
        ii = 2;
    
    Affiche_1_Parametre( frame, 20, _( "Type" ), text_type_msg[ii], DARKGREEN );

    Affiche_1_Parametre( frame, 25, _( "Display" ), wxEmptyString, DARKGREEN );
    if( m_NoShow )
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "No" ), DARKGREEN );
    else
        Affiche_1_Parametre( frame, -1, wxEmptyString, _( "Yes" ), DARKGREEN );

    ii = m_Layer;
    if( ii <= 28 )
        Affiche_1_Parametre( frame, 28, _( "Layer" ), ReturnPcbLayerName( ii ), DARKGREEN );
    else
    {
        msg.Printf( wxT( "%d" ), ii );
        Affiche_1_Parametre( frame, 28, _( "Layer" ), msg, DARKGREEN );
    }

    msg = wxT( " Yes" );
    if( m_Miroir & 1 )
        msg = wxT( " No" );
    
    Affiche_1_Parametre( frame, 36, _( "Mirror" ), msg, DARKGREEN );

    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    Affiche_1_Parametre( frame, 42, _( "Orient" ), msg, DARKGREEN );

    valeur_param( m_Width, msg );
    Affiche_1_Parametre( frame, 48, _( "Width" ), msg, DARKGREEN );

    valeur_param( m_Size.x, msg );
    Affiche_1_Parametre( frame, 56, _( "H Size" ), msg, RED );

    valeur_param( m_Size.y, msg );
    Affiche_1_Parametre( frame, 64, _( "V Size" ), msg, RED );
}


// see class_text_mod.h
bool TEXTE_MODULE::IsOnLayer( int aLayer ) const
{
    if( m_Layer == aLayer )
        return true;

    /* test the parent, which is a MODULE */
    if( aLayer == GetParent()->GetLayer() )
        return true;
    
    if( aLayer == CUIVRE_N )
    {
        if( m_Layer==ADHESIVE_N_CU || m_Layer==SILKSCREEN_N_CU )
            return true;
    }
    
    else if( aLayer == CMP_N )
    {
        if( m_Layer==ADHESIVE_N_CMP || m_Layer==SILKSCREEN_N_CMP )
            return true;
    }
    
    return false;
}

    
/* see class_text_mod.h
bool TEXTE_MODULE::IsOnOneOfTheseLayers( int aLayerMask ) const
{
    
}
*/


#if defined(DEBUG)
/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level 
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void TEXTE_MODULE::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
        " string=\"" << m_Text.mb_str() << "\"/>\n"; 
    
//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif
