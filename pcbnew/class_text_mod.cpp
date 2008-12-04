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
    BOARD_ITEM( parent, TYPE_TEXTE_MODULE )
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
    if( Module && (Module->Type() == TYPE_MODULE) )
    {
        m_Pos = Module->m_Pos;

        int moduleLayer = Module->GetLayer();

        if( moduleLayer == COPPER_LAYER_N )
            SetLayer( SILKSCREEN_N_CU );
        else if( moduleLayer == CMP_N )
            SetLayer( SILKSCREEN_N_CMP );
        else
            SetLayer( moduleLayer );

        if(  moduleLayer == SILKSCREEN_N_CU
             || moduleLayer == ADHESIVE_N_CU
             || moduleLayer == COPPER_LAYER_N )
        {
            m_Miroir = 0;
        }
    }
}


TEXTE_MODULE::~TEXTE_MODULE()
{
}


/*******************************************/
bool TEXTE_MODULE::Save( FILE* aFile ) const
/*******************************************/

/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
{
    MODULE* parent = (MODULE*) GetParent();
    int     orient = m_Orient;

    if( parent )
        orient += parent->m_Orient;

    int ret = fprintf( aFile, "T%d %d %d %d %d %d %d %c %c %d \"%s\"\n",
        m_Type,
        m_Pos0.x, m_Pos0.y,
        m_Size.y, m_Size.x,
        orient,
        m_Width,
        m_Miroir ? 'N' : 'M', m_NoShow ? 'I' : 'V',
        GetLayer(),
        CONV_TO_UTF8( m_Text ) );

    return ret > 20;
}


/*********************************************************************/
int TEXTE_MODULE::ReadDescr( char* aLine, FILE* aFile, int* aLineNum )
/*********************************************************************/
/**
 * Function ReadLineDescr
 * Read description from a given line in "*.brd" format.
 * @param aLine The current line which contains the first line of description.
 * @param aLine The FILE to read next lines (currently not used).
 * @param LineNum a point to the line count (currently not used).
 * @return int - > 0 if success reading else 0.
 */
{
    int success = true;
    int  type;
    int  layer;
    char BufCar1[128], BufCar2[128], BufLine[256];

    layer      = SILKSCREEN_N_CMP;
    BufCar1[0] = 0;
    BufCar2[0] = 0;
    if ( sscanf( aLine + 1, "%d %d %d %d %d %d %d %s %s %d",
        &type,
        &m_Pos0.x, &m_Pos0.y,
        &m_Size.y, &m_Size.x,
        &m_Orient, &m_Width,
        BufCar1, BufCar2, &layer ) < 10 )
        success = true;

    if( (type != TEXT_is_REFERENCE) && (type != TEXT_is_VALUE) )
        type = TEXT_is_DIVERS;
    m_Type = type;

    // .m_Orient member must be relative to the parent module
    m_Orient -= ((MODULE * )m_Parent)->m_Orient;
    if( BufCar1[0] == 'M' )
        m_Miroir = 0;
    else
        m_Miroir = 1;
    if( BufCar2[0]  == 'I' )
        m_NoShow = 1;
    else
        m_NoShow = 0;

    // Test for a reasonnable layer:
    if( layer < 0 )
        layer = 0;
    if( layer > LAST_NO_COPPER_LAYER )
        layer = LAST_NO_COPPER_LAYER;
    if( layer == COPPER_LAYER_N )
        layer = SILKSCREEN_N_CU;
    else if( layer == CMP_N )
        layer = SILKSCREEN_N_CMP;

    SetLayer( layer );

    /* calcul de la position vraie */
    SetDrawCoord();
    /* Lecture de la chaine "text" */
    ReadDelimitedText( BufLine, aLine, sizeof(BufLine) );
    m_Text = CONV_FROM_UTF8( BufLine );

    // Test for a reasonnable width:
    if( m_Width <= 1 )
        m_Width = 1;
    if( m_Width > TEXTS_MAX_WIDTH )
        m_Width = TEXTS_MAX_WIDTH;

    // Test for a reasonnable size:
    if( m_Size.x < TEXTS_MIN_SIZE )
        m_Size.x = TEXTS_MIN_SIZE;
    if( m_Size.y < TEXTS_MIN_SIZE )
        m_Size.y = TEXTS_MIN_SIZE;

    return success;
}


/**********************************************/
void TEXTE_MODULE::Copy( TEXTE_MODULE* source )
/**********************************************/

// copy structure
{
    if( source == NULL )
        return;

    m_Pos = source->m_Pos;
    SetLayer( source->GetLayer() );

    m_Miroir = source->m_Miroir;        // Show normal / mirror
    m_NoShow = source->m_NoShow;        // 0: visible 1: invisible
    m_Type   = source->m_Type;          // 0: ref,1: val, others = 2..255
    m_Orient = source->m_Orient;        // orientation in 1/10 deg
    m_Pos0   = source->m_Pos0;          // text coordinates relatives to the footprint ancre, orient 0
                                        // Text coordinate ref point is the text centre

    m_Size  = source->m_Size;
    m_Width = source->m_Width;

    m_Text = source->m_Text;
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


// Update draw ccordinates
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


// Update "local" cooedinates (coordinates relatives to the footprint anchor point)
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

/** Function GetTextRect
 * @return an EDA_Rect which gives the position and size of the text area (for the O orient  footprint)
 */
EDA_Rect TEXTE_MODULE::GetTextRect( void )
{
    EDA_Rect area;

    int      dx, dy;

    dx  = ( m_Size.x * GetLength() ) / 2;
    dx  = (dx * 10) / 9; /* letter size = 10/9 */
    dx += m_Width / 2;
    dy  = ( m_Size.y + m_Width ) / 2;

    wxPoint Org = m_Pos;    // This is the position of the centre of the area
    Org.x -= dx;
    Org.y -= dy;
    area.SetOrigin( Org );
    area.SetHeight( 2 * dy );
    area.SetWidth( 2 * dx );
    area.Normalize();

    return area;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool TEXTE_MODULE::HitTest( const wxPoint& refPos )
{
    wxPoint  rel_pos;
    EDA_Rect area = GetTextRect();

    /* Rotate refPos to - angle
     * to test if refPos is within area (which is relative to an horizontal text)
     */
    rel_pos = refPos;
    RotatePoint( &rel_pos, m_Pos, -GetDrawRotation() );

    if( area.Inside( rel_pos ) )
        return true;

    return false;
}


/**
 * Function GetBoundingBox
 * returns the bounding box of this Text (according to text and footprint orientation)
 */
EDA_Rect TEXTE_MODULE::GetBoundingBox()
{
    // Calculate area without text fielsd:
    EDA_Rect text_area;
    int      angle = GetDrawRotation();
    wxPoint  textstart, textend;

    text_area = GetTextRect();
    textstart = text_area.GetOrigin();
    textend   = text_area.GetEnd();
    RotatePoint( &textstart, m_Pos, angle );
    RotatePoint( &textend, m_Pos, angle );

    text_area.SetOrigin( textstart );
    text_area.SetEnd( textend );
    text_area.Normalize();
    return text_area;
}


/******************************************************************************************/
void TEXTE_MODULE::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode, const wxPoint& offset )
/******************************************************************************************/

/** Function Draw
 * Draw the text accordint to the footprint pos and orient
 * @param panel = draw panel, Used to know the clip box
 * @param DC = Current Device Context
 * @param offset = draw offset (usually wxPoint(0,0)
 * @param draw_mode = GR_OR, GR_XOR..
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

    if( Module && Module->GetLayer() == COPPER_LAYER_N )
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

    MODULE*  module = (MODULE*) m_Parent;

    wxASSERT( module );

    if( !module )
        return;

    BOARD* board = (BOARD*) module->GetParent();
    wxASSERT( board );

    static const wxString text_type_msg[3] = {
        _( "Ref." ), _( "Value" ), _( "Text" )
    };

    frame->MsgPanel->EraseMsgBox();

    Line = module->m_Reference->m_Text;
    Affiche_1_Parametre( frame, 1, _( "Module" ), Line, DARKCYAN );

    Line = m_Text;
    Affiche_1_Parametre( frame, 10, _( "Text" ), Line, BROWN );

    ii = m_Type;
    if( ii > 2 )
        ii = 2;

    Affiche_1_Parametre( frame, 20, _( "Type" ), text_type_msg[ii], DARKGREEN );

    if( m_NoShow )
        msg = _( "No" );
    else
        msg = _( "Yes" );
    Affiche_1_Parametre( frame, 25, _( "Display" ), msg, DARKGREEN );

    ii = m_Layer;
    if( ii < NB_LAYERS )
        msg = board->GetLayerName( ii );
    else
        msg.Printf( wxT( "%d" ), ii );
    Affiche_1_Parametre( frame, 31, _( "Layer" ), msg, DARKGREEN );

    msg = wxT( " Yes" );
    if( m_Miroir & 1 )
        msg = wxT( " No" );

    Affiche_1_Parametre( frame, 37, _( "Mirror" ), msg, DARKGREEN );

    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    Affiche_1_Parametre( frame, 43, _( "Orient" ), msg, DARKGREEN );

    valeur_param( m_Width, msg );
    Affiche_1_Parametre( frame, 51, _( "Width" ), msg, DARKGREEN );

    valeur_param( m_Size.x, msg );
    Affiche_1_Parametre( frame, 60, _( "H Size" ), msg, RED );

    valeur_param( m_Size.y, msg );
    Affiche_1_Parametre( frame, 69, _( "V Size" ), msg, RED );
}


// see class_text_mod.h
bool TEXTE_MODULE::IsOnLayer( int aLayer ) const
{
    if( m_Layer == aLayer )
        return true;

    /* test the parent, which is a MODULE */
    if( aLayer == GetParent()->GetLayer() )
        return true;

    if( aLayer == COPPER_LAYER_N )
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
  * bool TEXTE_MODULE::IsOnOneOfTheseLayers( int aLayerMask ) const
  * {
 *
  * }
 */


#if defined (DEBUG)

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
