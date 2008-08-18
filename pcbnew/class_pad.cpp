/************************************************/
/* class_pad.cpp : fonctions de la classe D_PAD */
/************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "wxstruct.h"
#include "common.h"
#include "pcbnew.h"
#include "trigo.h"
#include "id.h"             // ID_TRACK_BUTT

#ifdef PCBNEW
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"

#endif

#include "protos.h"

/*******************************/
/* classe D_PAD : constructeur */
/*******************************/

D_PAD::D_PAD( MODULE* parent ) :
    BOARD_ITEM( parent, TYPEPAD )
{
    m_NumPadName   = 0;
    m_Masque_Layer = CUIVRE_LAYER;
    SetNet( 0 );                    /* Numero de net pour comparaisons rapides */
    m_DrillShape = PAD_CIRCLE;          // Drill shape = circle

    m_Size.x = m_Size.y = 500;

    if( m_Parent && (m_Parent->Type()  == TYPEMODULE) )
    {
        m_Pos = ( (MODULE*) m_Parent )->GetPosition();
    }

    m_PadShape = PAD_CIRCLE;            // forme CERCLE, PAD_RECT PAD_OVAL PAD_TRAPEZOID ou libre
    m_Attribut = PAD_STANDARD;          // NORMAL, PAD_SMD, PAD_CONN, Bit 7 = STACK
    m_Orient   = 0;                 // en 1/10 degres

    m_logical_connexion  = 0;
    m_physical_connexion = 0;       // variables utilisee lors du calcul du chevelu
    ComputeRayon();
}


D_PAD::~D_PAD()
{
}


/****************************/
void D_PAD::ComputeRayon()
/****************************/

/* met a jour m_Rayon, rayon du cercle exinscrit
 */
{
    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        m_Rayon = m_Size.x / 2;
        break;

    case PAD_OVAL:
        m_Rayon = MAX( m_Size.x, m_Size.y ) / 2;
        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
        m_Rayon = (int) (sqrt( (double) m_Size.y * m_Size.y
                              + (double) m_Size.x * m_Size.x ) / 2);
        break;
    }
}


/**
 * Function GetBoundingBox
 * returns the bounding box of this pad
 * Mainly used to redraw the screen area occuped by the pad
 */
EDA_Rect D_PAD::GetBoundingBox()
{
    // Calculate area:
    ComputeRayon();		// calculate the radius of the area, considered as a circle
    EDA_Rect      area;
	area.SetOrigin(m_Pos);
	area.Inflate(m_Rayon, m_Rayon);

	return area;
}


/*********************************************/
const wxPoint D_PAD::ReturnShapePos()
/*********************************************/

// retourne la position de la forme (pastilles excentrees)
{
    if( m_Offset.x == 0 && m_Offset.y == 0 )
        return m_Pos;

    wxPoint shape_pos;
    int     dX, dY;

    dX = m_Offset.x;
    dY = m_Offset.y;

    RotatePoint( &dX, &dY, m_Orient );

    shape_pos.x = m_Pos.x + dX;
    shape_pos.y = m_Pos.y + dY;

    return shape_pos;
}


/****************************************/
wxString D_PAD::ReturnStringPadName()
/****************************************/

/* Return pad name as string in a wxString
 */
{
    wxString name;

    ReturnStringPadName( name );
    return name;
}


/********************************************/
void D_PAD::ReturnStringPadName( wxString& text )
/********************************************/

/* Return pad name as string in a buffer
 */
{
    int ii;

    text.Empty();
    for( ii = 0; ii < 4; ii++ )
    {
        if( m_Padname[ii] == 0 )
            break;
        text.Append( m_Padname[ii] );
    }
}


/********************************************/
void D_PAD::SetPadName( const wxString& name )
/********************************************/

// Change pad name
{
    int ii, len;

    len = name.Length();
    if( len > 4 )
        len = 4;
    for( ii = 0; ii < len; ii++ )
        m_Padname[ii] = name.GetChar( ii );

    for( ii = len; ii < 4; ii++ )
        m_Padname[ii] = 0;
}


/********************************/
void D_PAD::Copy( D_PAD* source )
/********************************/
{
    if( source == NULL )
        return;

    m_Pos = source->m_Pos;
    m_Masque_Layer = source->m_Masque_Layer;

    memcpy( m_Padname, source->m_Padname, sizeof(m_Padname) );  /* nom de la pastille */
    SetNet( source->GetNet() );                                 /* Numero de net pour comparaisons rapides */
    m_Drill      = source->m_Drill;                             // Diametre de percage
    m_DrillShape = source->m_DrillShape;
    m_Offset     = source->m_Offset;                            // Offset de la forme
    m_Size      = source->m_Size;                               // Dimension ( pour orient 0 )
    m_DeltaSize = source->m_DeltaSize;                          // delta sur formes rectangle -> trapezes
    m_Pos0 = source->m_Pos0;                                    // Coord relatives a l'ancre du pad en
    //  orientation 0
    m_Rayon    = source->m_Rayon;                               // rayon du cercle exinscrit du pad
    m_PadShape = source->m_PadShape;                            // forme CERCLE, PAD_RECT PAD_OVAL PAD_TRAPEZOID ou libre
    m_Attribut = source->m_Attribut;                            // NORMAL, PAD_SMD, PAD_CONN, Bit 7 = STACK
    m_Orient   = source->m_Orient;                              // en 1/10 degres

    m_logical_connexion  = 0;                                   // variable utilisee lors du calcul du chevelu
    m_physical_connexion = 0;                                   // variable utilisee lors du calcul de la connexit�
    m_Netname = source->m_Netname;
}


/**************************/
void D_PAD::UnLink()
/**************************/

/* supprime du chainage la structure Struct
 *  les structures arrieres et avant sont chainees directement
 */
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
            ( (MODULE*) Pback )->m_Pads = (D_PAD*) Pnext;
        }
    }

    /* Modification du chainage avant */
    if( Pnext )
        Pnext->Pback = Pback;

    Pnext = Pback = NULL;
}


/*******************************************************************************************/
void D_PAD::Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode, const wxPoint& offset )
/*******************************************************************************************/

/** Draw a pad:
 *  @param  DC = device context
 *  @param offset = draw offset
 *  @param draw_mode = mode: GR_OR, GR_XOR, GR_AND...
 */
{
    int ii;
    int color = 0;
    int ux0, uy0,
        dx, dx0, dy, dy0,
        rotdx,
        delta_cx, delta_cy,
        xc, yc;
    int                  angle;
    wxPoint              coord[4];
    int                  zoom;
    int                  fillpad = 0;
    wxPoint              shape_pos;

	if ( m_Flags & DO_NOT_DRAW )
		return;

    wxASSERT( panel );

    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) panel->m_Parent;

    PCB_SCREEN* screen = frame->GetScreen();

    zoom = screen->GetZoom();

    if( frame->m_DisplayPadFill == FILLED )
        fillpad = 1;

#ifdef PCBNEW
    if( m_Flags & IS_MOVED )
        fillpad = 0;
#endif

    if( m_Masque_Layer & CMP_LAYER )
        color = g_PadCMPColor;

    if( m_Masque_Layer & CUIVRE_LAYER )
        color |= g_PadCUColor;

    if( color == 0 ) /* Not on copper layer */
    {
        switch( m_Masque_Layer & ~ALL_CU_LAYERS )
        {
        case ADHESIVE_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[ADHESIVE_N_CU];
            break;

        case ADHESIVE_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[ADHESIVE_N_CMP];
            break;

        case SOLDERPASTE_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[SOLDERPASTE_N_CU];
            break;

        case SOLDERPASTE_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[SOLDERPASTE_N_CMP];
            break;

        case SILKSCREEN_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[SILKSCREEN_N_CU];
            break;

        case SILKSCREEN_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[SILKSCREEN_N_CMP];
            break;

        case SOLDERMASK_LAYER_CU:
            color = g_DesignSettings.m_LayerColor[SOLDERMASK_N_CU];
            break;

        case SOLDERMASK_LAYER_CMP:
            color = g_DesignSettings.m_LayerColor[SOLDERMASK_N_CMP];
            break;

        case DRAW_LAYER:
            color = g_DesignSettings.m_LayerColor[DRAW_N];
            break;

        case COMMENT_LAYER:
            color = g_DesignSettings.m_LayerColor[COMMENT_N];
            break;

        case ECO1_LAYER:
            color = g_DesignSettings.m_LayerColor[ECO1_N];
            break;

        case ECO2_LAYER:
            color = g_DesignSettings.m_LayerColor[ECO2_N];
            break;

        case EDGE_LAYER:
            color = g_DesignSettings.m_LayerColor[EDGE_N];
            break;

        default:
            color = DARKGRAY;
            break;
        }
    }


    // if PAD_SMD pad and high contrast mode
    if( (m_Attribut==PAD_SMD || m_Attribut==PAD_CONN) && DisplayOpt.ContrastModeDisplay )
    {
        // when routing tracks
        if( frame && frame->m_ID_current_state == ID_TRACK_BUTT )
        {
            int routeTop = screen->m_Route_Layer_TOP;
            int routeBot = screen->m_Route_Layer_BOTTOM;

            // if routing between copper and component layers,
            // or the current layer is one of said 2 external copper layers,
            // then highlight only the current layer.
            if( ((1<<routeTop) | (1<<routeBot)) == (CUIVRE_LAYER | CMP_LAYER)
             || ((1<<screen->m_Active_Layer) & (CUIVRE_LAYER | CMP_LAYER)) )
            {
                if( !IsOnLayer( screen->m_Active_Layer ) )
                {
                    color &= ~MASKCOLOR;
                    color |= DARKDARKGRAY;
                }
            }

            // else routing between an internal signal layer and some other layer.
            // grey out all PAD_SMD pads not on current or the single selected
            // external layer.
            else if( !IsOnLayer( screen->m_Active_Layer )
                  && !IsOnLayer( routeTop )
                  && !IsOnLayer( routeBot ) )
            {
                color &= ~MASKCOLOR;
                color |= DARKDARKGRAY;
            }
        }

        // when not edting tracks, show PAD_SMD components not on active layer as greyed out
        else
        {
            if( !IsOnLayer( screen->m_Active_Layer ) )
            {
                color &= ~MASKCOLOR;
                color |= DARKDARKGRAY;
            }
        }
    }

    if( draw_mode & GR_SURBRILL )
    {
        if( draw_mode & GR_AND )
            color &= ~HIGHT_LIGHT_FLAG;
        else
            color |= HIGHT_LIGHT_FLAG;
    }

    if( color & HIGHT_LIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    GRSetDrawMode( DC, draw_mode );  /* mode de trace */

    /* calcul du centre des pads en coordonnees Ecran : */
    shape_pos = ReturnShapePos();
    ux0 = shape_pos.x - offset.x;
    uy0 = shape_pos.y - offset.y;
    xc  = ux0;
    yc  = uy0;

    /* le trace depend de la rotation de l'empreinte */

    dx = dx0 = m_Size.x >> 1;
    dy = dy0 = m_Size.y >> 1; /* demi dim  dx et dy */

    angle = m_Orient;

    bool DisplayIsol = DisplayOpt.DisplayPadIsol;
    if( ( m_Masque_Layer & ALL_CU_LAYERS ) == 0 )
        DisplayIsol = FALSE;

    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        if( fillpad )
            GRFilledCircle( &panel->m_ClipBox, DC, xc, yc, dx, 0, color, color );
        else
            GRCircle( &panel->m_ClipBox, DC, xc, yc, dx, 0, color );

        if( DisplayIsol )
        {
            GRCircle( &panel->m_ClipBox,
                      DC,
                      xc,
                      yc,
                      dx + g_DesignSettings.m_TrackClearence,
                      0,
                      color );
        }
        break;

    case PAD_OVAL:
        /* calcul de l'entraxe de l'ellipse */
        if( dx > dy )       /* ellipse horizontale */
        {
            delta_cx = dx - dy;
            delta_cy = 0;
            rotdx    = m_Size.y;
        }
        else                /* ellipse verticale */
        {
            delta_cx = 0;
            delta_cy = dy - dx;
            rotdx    = m_Size.x;
        }
        RotatePoint( &delta_cx, &delta_cy, angle );

        if( fillpad )
        {
            GRFillCSegm( &panel->m_ClipBox, DC, ux0 + delta_cx, uy0 + delta_cy,
                         ux0 - delta_cx, uy0 - delta_cy,
                         rotdx, color );
        }
        else
        {
            GRCSegm( &panel->m_ClipBox, DC, ux0 + delta_cx, uy0 + delta_cy,
                     ux0 - delta_cx, uy0 - delta_cy,
                     rotdx, color );
        }

        /* Trace de la marge d'isolement */
        if( DisplayIsol )
        {
            rotdx = rotdx + g_DesignSettings.m_TrackClearence + g_DesignSettings.m_TrackClearence;

            GRCSegm( &panel->m_ClipBox, DC, ux0 + delta_cx, uy0 + delta_cy,
                     ux0 - delta_cx, uy0 - delta_cy,
                     rotdx, color );
        }
        break;

    case PAD_RECT:
    case PAD_TRAPEZOID:
    {
        int ddx, ddy;
        ddx = m_DeltaSize.x >> 1;
        ddy = m_DeltaSize.y >> 1;      /* demi dim  dx et dy */

        coord[0].x = -dx - ddy;
        coord[0].y = +dy + ddx;

        coord[1].x = -dx + ddy;
        coord[1].y = -dy - ddx;

        coord[2].x = +dx - ddy;
        coord[2].y = -dy + ddx;

        coord[3].x = +dx + ddy;
        coord[3].y = +dy - ddx;

        for( ii = 0; ii < 4; ii++ )
        {
            RotatePoint( &coord[ii].x, &coord[ii].y, angle );
            coord[ii].x = coord[ii].x + ux0;
            coord[ii].y = coord[ii].y + uy0;
        }

        GRClosedPoly( &panel->m_ClipBox, DC, 4, (int*) coord, fillpad, color, color );

        if( DisplayIsol )
        {
            dx += g_DesignSettings.m_TrackClearence;
            dy += g_DesignSettings.m_TrackClearence;

            coord[0].x = -dx - ddy;
            coord[0].y = dy + ddx;

            coord[1].x = -dx + ddy;
            coord[1].y = -dy - ddx;

            coord[2].x = dx - ddy;
            coord[2].y = -dy + ddx;

            coord[3].x = dx + ddy;
            coord[3].y = dy - ddx;

            for( ii = 0; ii < 4; ii++ )
            {
                RotatePoint( &coord[ii].x, &coord[ii].y, angle );
                coord[ii].x = coord[ii].x + ux0;
                coord[ii].y = coord[ii].y + uy0;
            }

            GRClosedPoly( &panel->m_ClipBox, DC, 4, (int*) coord, 0, color, color );
        }
    }
        break;


    default:
        break;
    }

    /* Draw the pad hole */
    int cx0  = m_Pos.x - offset.x;
    int cy0  = m_Pos.y - offset.y;
    int hole = m_Drill.x >> 1;

    if( fillpad && hole )
    {
        bool blackpenstate = false;
        if ( g_IsPrinting )
        {
            blackpenstate = GetGRForceBlackPenState( );
            GRForceBlackPen( false );
            color = WHITE;
        }
        else
            color = BLACK; // or DARKGRAY;

        if( draw_mode != GR_XOR )
            GRSetDrawMode( DC, GR_COPY );
        else
            GRSetDrawMode( DC, GR_XOR );

        switch( m_DrillShape )
        {
        case PAD_CIRCLE:
            if( (hole / zoom) > 1 ) /* draw hole if its size is enought */
                GRFilledCircle( &panel->m_ClipBox, DC, cx0, cy0, hole, 0, color, color );
            break;

        case PAD_OVAL:
            dx = m_Drill.x >> 1;
            dy = m_Drill.y >> 1;            /* demi dim  dx et dy */

            /* calcul de l'entraxe de l'ellipse */
            if( m_Drill.x > m_Drill.y )     /* ellipse horizontale */
            {
                delta_cx = dx - dy; delta_cy = 0;
                rotdx    = m_Drill.y;
            }
            else                /* ellipse verticale */
            {
                delta_cx = 0; delta_cy = dy - dx;
                rotdx    = m_Drill.x;
            }
            RotatePoint( &delta_cx, &delta_cy, angle );

            GRFillCSegm( &panel->m_ClipBox, DC, ux0 + delta_cx, uy0 + delta_cy,
                         ux0 - delta_cx, uy0 - delta_cy,
                         rotdx, color );
            break;

        default:
            break;
        }
        if ( g_IsPrinting )
            GRForceBlackPen( blackpenstate );
    }

    GRSetDrawMode( DC, draw_mode );

    /* Trace du symbole "No connect" ( / ou \ ou croix en X) si necessaire : */
    if( m_Netname.IsEmpty() && DisplayOpt.DisplayPadNoConn )
    {
        dx0 = MIN( dx0, dy0 );
        int nc_color = BLUE;

        if( m_Masque_Layer & CMP_LAYER ) /* Trace forme \ */
            GRLine( &panel->m_ClipBox, DC, cx0 - dx0, cy0 - dx0,
                    cx0 + dx0, cy0 + dx0, 0, nc_color );

        if( m_Masque_Layer & CUIVRE_LAYER ) /* Trace forme / */
            GRLine( &panel->m_ClipBox, DC, cx0 + dx0, cy0 - dx0,
                    cx0 - dx0, cy0 + dx0, 0, nc_color );
    }

    /* Draw the pad number */
    if( frame && !frame->m_DisplayPadNum )
        return;

    dx = MIN( m_Size.x, m_Size.y );     /* dx = text size */
    if( (dx / zoom) > 12 )              /* size must be enought to draw 2 chars */
    {
        wxString buffer;

        ReturnStringPadName( buffer );
        dy = buffer.Len();

        /* Draw text with an angle between -90 deg and + 90 deg */
        NORMALIZE_ANGLE_90( angle );
        if( dy < 2 )
            dy = 2;                     /* text min size is 2 char */

        dx = (dx * 9 ) / (dy * 13 );    /* Text size ajusted to pad size */

        DrawGraphicText( panel, DC, wxPoint( ux0, uy0 ),
                         WHITE, buffer, angle, wxSize( dx, dx ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER );
    }
}


/*************************************************/
int D_PAD::ReadDescr( FILE* File, int* LineNum )
/*************************************************/

/* Routine de lecture de descr de pads
 *  la 1ere ligne de descr ($PAD) est supposee etre deja lue
 *  syntaxe:
 *  $PAD
 *  Sh "N1" C 550 550 0 0 1800
 *  Dr 310 0 0
 *  At STD N 00C0FFFF
 *  Ne 3 "netname"
 *  Po 6000 -6000
 *  $EndPAD
 */
{
    char  Line[1024], BufLine[1024], BufCar[256];
    char* PtLine;
    int   nn, ll, dx, dy;

    while( GetLine( File, Line, LineNum ) != NULL )
    {
        if( Line[0] == '$' )
            return 0;

        PtLine = Line + 3;

        /* Pointe 1er code utile de la ligne */
        switch( Line[0] )
        {
        case 'S':           /* Ligne de description de forme et dims*/
            /* Lecture du nom pad */
            nn = 0;
            while( (*PtLine != '"') && *PtLine )
                PtLine++;

            if( *PtLine )
                PtLine++;

            memset( m_Padname, 0, sizeof(m_Padname) );
            while( (*PtLine != '"') && *PtLine )
            {
                if( nn < (int) sizeof(m_Padname) )
                {
                    if( *PtLine > ' ' )
                    {
                        m_Padname[nn] = *PtLine; nn++;
                    }
                }
                PtLine++;
            }

            if( *PtLine == '"' )
                PtLine++;

            nn = sscanf( PtLine, " %s %d %d %d %d %d",
                         BufCar, &m_Size.x, &m_Size.y,
                         &m_DeltaSize.x, &m_DeltaSize.y,
                         &m_Orient );

            ll = 0xFF & BufCar[0];

            /* Mise a jour de la forme */
            m_PadShape = PAD_CIRCLE;

            switch( ll )
            {
            case 'C':
                m_PadShape = PAD_CIRCLE; break;

            case 'R':
                m_PadShape = PAD_RECT; break;

            case 'O':
                m_PadShape = PAD_OVAL; break;

            case 'T':
                m_PadShape = PAD_TRAPEZOID; break;
            }

            ComputeRayon();
            break;

        case 'D':
            BufCar[0] = 0;
            nn = sscanf( PtLine, "%d %d %d %s %d %d", &m_Drill.x,
                         &m_Offset.x, &m_Offset.y, BufCar, &dx, &dy );
            m_Drill.y    = m_Drill.x;
            m_DrillShape = PAD_CIRCLE;

            if( nn >= 6 )       // Drill shape = OVAL ?
            {
                if( BufCar[0] == 'O' )
                {
                    m_Drill.x    = dx; m_Drill.y = dy;
                    m_DrillShape = PAD_OVAL;
                }
            }
            break;

        case 'A':
            nn = sscanf( PtLine, "%s %s %X", BufLine, BufCar,
                         &m_Masque_Layer );

            /* Contenu de BufCar non encore utilise ( reserve pour evolutions
             *  ulterieures */
            /* Mise a jour de l'attribut */
            m_Attribut = PAD_STANDARD;
            if( strncmp( BufLine, "SMD", 3 ) == 0 )
                m_Attribut = PAD_SMD;
            if( strncmp( BufLine, "CONN", 4 ) == 0 )
                m_Attribut = PAD_CONN;
            if( strncmp( BufLine, "HOLE", 4 ) == 0 )
                m_Attribut = PAD_HOLE_NOT_PLATED;
            break;

        case 'N':       /* Lecture du netname */
            int netcode;
            nn = sscanf( PtLine, "%d", &netcode );
            SetNet( netcode );

            /* Lecture du netname */
            ReadDelimitedText( BufLine, PtLine, sizeof(BufLine) );
            m_Netname = CONV_FROM_UTF8( StrPurge( BufLine ) );
            break;

        case 'P':
            nn    = sscanf( PtLine, "%d %d", &m_Pos0.x, &m_Pos0.y );
            m_Pos = m_Pos0;
            break;

        default:
            DisplayError( NULL, wxT( "Err Pad: Id inconnu" ) );
            return 1;
        }
    }

    return 2;   /* error : EOF */
}


/*************************************/
bool D_PAD::Save( FILE* aFile ) const
/*************************************/
{
    int         cshape;
    const char* texttype;

    if( GetState( DELETED ) )
        return true;

    bool        rc = false;

    // check the return values for first and last fprints() in this function
    if( fprintf( aFile, "$PAD\n" ) != sizeof("$PAD\n")-1 )
        goto out;

    switch( m_PadShape )
    {
    case PAD_CIRCLE:
        cshape = 'C'; break;

    case PAD_RECT:
        cshape = 'R'; break;

    case PAD_OVAL:
        cshape = 'O'; break;

    case PAD_TRAPEZOID:
        cshape = 'T'; break;

    default:
        cshape = 'C';
        DisplayError( NULL, _( "Unknown Pad shape" ) );
        break;
    }

    fprintf( aFile, "Sh \"%.4s\" %c %d %d %d %d %d\n",
             m_Padname, cshape, m_Size.x, m_Size.y,
             m_DeltaSize.x, m_DeltaSize.y, m_Orient );

    fprintf( aFile, "Dr %d %d %d", m_Drill.x, m_Offset.x, m_Offset.y );
    if( m_DrillShape == PAD_OVAL )
    {
        fprintf( aFile, " %c %d %d", 'O', m_Drill.x, m_Drill.y );
    }
    fprintf( aFile, "\n" );

    switch( m_Attribut )
    {
    case PAD_STANDARD:
        texttype = "STD"; break;

    case PAD_SMD:
        texttype = "SMD"; break;

    case PAD_CONN:
        texttype = "CONN"; break;

    case PAD_HOLE_NOT_PLATED:
        texttype = "HOLE"; break;

    default:
        texttype = "STD";
        DisplayError( NULL, wxT( "Invalid Pad attribute" ) );
        break;
    }

    fprintf( aFile, "At %s N %8.8X\n", texttype, m_Masque_Layer );

    fprintf( aFile, "Ne %d \"%s\"\n", GetNet(), CONV_TO_UTF8( m_Netname ) );

    fprintf( aFile, "Po %d %d\n", m_Pos0.x, m_Pos0.y );

    if( fprintf( aFile, "$EndPAD\n" ) != sizeof("$EndPAD\n")-1 )
        goto out;

    rc = true;

out:
    return rc;
}



/******************************************************/
void D_PAD::Display_Infos( WinEDA_DrawFrame* frame )
/******************************************************/
/* Affiche en bas d'ecran les caract de la pastille demandee */
{
    int      ii;
    MODULE*  module;
    wxString Line;
    int      pos = 1;

    /* Pad messages */
    static const wxString Msg_Pad_Shape[6] =
    { wxT( "??? " ), wxT( "Circ" ), wxT( "Rect" ), wxT( "Oval" ), wxT( "trap" ), wxT( "spec" ) };

    static const wxString Msg_Pad_Layer[9] =
    {
        wxT( "??? " ),     wxT( "cmp   " ),  wxT( "cu    " ),  wxT( "cmp+cu " ), wxT( "int    " ),
        wxT( "cmp+int " ), wxT( "cu+int " ), wxT( "all    " ), wxT( "No copp" )
    };

    static const wxString Msg_Pad_Attribut[5] =
    { wxT( "norm" ), wxT( "smd " ), wxT( "conn" ), wxT( "hole" ), wxT( "????" ) };


    frame->MsgPanel->EraseMsgBox();

    /* Recherche du module correspondant */
    module = (MODULE*) m_Parent;
    if( module )
    {
        wxString msg = module->GetReference();
        Affiche_1_Parametre( frame, pos, _( "Module" ), msg, DARKCYAN );
        ReturnStringPadName( Line );
        pos += 8;
        Affiche_1_Parametre( frame, pos, _( "RefP" ), Line, BROWN );
    }
    pos += 4;
    Affiche_1_Parametre( frame, pos, _( "Net" ), m_Netname, DARKCYAN );

    /* For test and debug only: display m_physical_connexion and m_logical_connexion */
    pos += 10;
#if 0
    Line.Printf( wxT( "%d.%d " ), m_logical_connexion, m_physical_connexion );
    Affiche_1_Parametre( frame, pos, "L.P", Line, WHITE );
#endif

    wxString LayerInfo;

    ii = 0;
    if( m_Masque_Layer & CUIVRE_LAYER )
        ii = 2;
    if( m_Masque_Layer & CMP_LAYER )
        ii += 1;
    if( (m_Masque_Layer & ALL_CU_LAYERS) == ALL_CU_LAYERS )
        ii = 7;

    LayerInfo = Msg_Pad_Layer[ii];
    if( (m_Masque_Layer & ALL_CU_LAYERS) == 0 )
    {
        if( m_Masque_Layer )
            LayerInfo = Msg_Pad_Layer[8];

        switch( m_Masque_Layer & ~ALL_CU_LAYERS )
        {
        case ADHESIVE_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( ADHESIVE_N_CU );
            break;

        case ADHESIVE_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( ADHESIVE_N_CMP );
            break;

        case SOLDERPASTE_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( SOLDERPASTE_N_CU );
            break;

        case SOLDERPASTE_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( SOLDERPASTE_N_CMP );
            break;

        case SILKSCREEN_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( SILKSCREEN_N_CU );
            break;

        case SILKSCREEN_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( SILKSCREEN_N_CMP );
            break;

        case SOLDERMASK_LAYER_CU:
            LayerInfo = ReturnPcbLayerName( SOLDERMASK_N_CU );
            break;

        case SOLDERMASK_LAYER_CMP:
            LayerInfo = ReturnPcbLayerName( SOLDERMASK_N_CMP );
            break;

        case DRAW_LAYER:
            LayerInfo = ReturnPcbLayerName( DRAW_N );
            break;

        case COMMENT_LAYER:
            LayerInfo = ReturnPcbLayerName( COMMENT_N );
            break;

        case ECO1_LAYER:
            LayerInfo = ReturnPcbLayerName( ECO1_N );
            break;

        case ECO2_LAYER:
            LayerInfo = ReturnPcbLayerName( ECO2_N );
            break;

        case EDGE_LAYER:
            LayerInfo = ReturnPcbLayerName( EDGE_N );
            break;

        default:
            break;
        }
    }
    pos += 3;
    Affiche_1_Parametre( frame, pos, _( "Layer" ), LayerInfo, DARKGREEN );

    pos += 6;
    Affiche_1_Parametre( frame, pos, Msg_Pad_Shape[m_PadShape], wxEmptyString, DARKGREEN );

    Affiche_1_Parametre( frame,
                         -1,
                         wxEmptyString,
                         Msg_Pad_Attribut[m_Attribut & 15],
                         DARKGREEN );

    valeur_param( m_Size.x, Line );
    pos += 6;
    Affiche_1_Parametre( frame, pos, _( "H Size" ), Line, RED );

    valeur_param( m_Size.y, Line );
    pos += 7;
    Affiche_1_Parametre( frame, pos, _( "V Size" ), Line, RED );

    pos += 7;
    valeur_param( (unsigned) m_Drill.x, Line );
    if( m_DrillShape == PAD_CIRCLE )
    {
        Affiche_1_Parametre( frame, pos, _( "Drill" ), Line, RED );
    }
    else
    {
        valeur_param( (unsigned) m_Drill.x, Line );
        wxString msg;
        valeur_param( (unsigned) m_Drill.y, msg );
        Line += wxT( " / " ) + msg;
        Affiche_1_Parametre( frame, pos, _( "Drill X / Y" ), Line, RED );
    }


    int module_orient = module ? module->m_Orient : 0;
    if( module_orient )
        Line.Printf( wxT( "%3.1f(+%3.1f)" ),
                     (float) (m_Orient - module_orient) / 10, (float) module_orient / 10 );
    else
        Line.Printf( wxT( "%3.1f" ), (float) m_Orient / 10 );
    pos += 8;
    Affiche_1_Parametre( frame, pos, _( "Orient" ), Line, BLUE );

    valeur_param( m_Pos.x, Line );
    pos += 8;
    Affiche_1_Parametre( frame, pos, _( "X Pos" ), Line, BLUE );

    valeur_param( m_Pos.y, Line );
    pos += 6;
    Affiche_1_Parametre( frame, pos, _( "Y pos" ), Line, BLUE );
}


// see class_pad.h
bool D_PAD::IsOnLayer( int aLayer ) const
{
    return (1<<aLayer) & m_Masque_Layer;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param ref_pos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool D_PAD::HitTest( const wxPoint& ref_pos )
{
    int     deltaX, deltaY;
    int     dx, dy;
    double  dist;

    wxPoint shape_pos = ReturnShapePos();

    deltaX = ref_pos.x - shape_pos.x;
    deltaY = ref_pos.y - shape_pos.y;

    /* Test rapide: le point a tester doit etre a l'interieur du cercle exinscrit ... */
    if( (abs( deltaX ) > m_Rayon )
       || (abs( deltaY ) > m_Rayon) )
        return false;

    /* calcul des demi dim  dx et dy */
    dx = m_Size.x >> 1; // dx also is the radius for rounded pads
    dy = m_Size.y >> 1;

    /* localisation ? */
    switch( m_PadShape & 0x7F )
    {
    case PAD_CIRCLE:
        dist = hypot( deltaX, deltaY );
        if( (int) ( round( dist ) ) <= dx )
            return true;
        break;

    default:
        /* calcul des coord du point test  dans le repere du Pad */
        RotatePoint( &deltaX, &deltaY, -m_Orient );
        if( (abs( deltaX ) <= dx ) && (abs( deltaY ) <= dy) )
            return true;
        break;
    }

    return false;
}


/************************************************************/
int D_PAD::Compare( const D_PAD* padref, const D_PAD* padcmp )
/************************************************************/
{
    int          diff;

    if( (diff = padref->m_PadShape - padcmp->m_PadShape) )
        return diff;
    if( (diff = padref->m_Size.x - padcmp->m_Size.x) )
        return diff;
    if( (diff = padref->m_Size.y - padcmp->m_Size.y) )
        return diff;
    if( (diff = padref->m_Offset.x - padcmp->m_Offset.x) )
        return diff;
    if( (diff = padref->m_Offset.y - padcmp->m_Offset.y) )
        return diff;
    if( (diff = padref->m_DeltaSize.x - padcmp->m_DeltaSize.x) )
        return diff;
    if( (diff = padref->m_DeltaSize.y - padcmp->m_DeltaSize.y) )
        return diff;

    // @todo check if export_gencad still works:
    // specctra_export needs this, but maybe export_gencad does not.  added on Jan 24 2008 by Dick.
    if( (diff = padref->m_Masque_Layer - padcmp->m_Masque_Layer) )
        return diff;

    return 0;
}


#if defined (DEBUG)

// @todo: could this be useable elsewhere also?
static const char* ShowPadType( int aPadType )
{
    switch( aPadType )
    {
    case PAD_CIRCLE:        return "circle";
    case PAD_OVAL:          return "oval";
    case PAD_RECT:          return "rect";
    case PAD_TRAPEZOID:     return "trap";
    default:                return "??unknown??";
    }
}


static const char* ShowPadAttr( int aPadAttr )
{
    switch( aPadAttr )
    {
    case PAD_STANDARD:      return "STD";
    case PAD_SMD:           return "SMD";
    case PAD_CONN:          return "CONN";
    case PAD_HOLE_NOT_PLATED:        return "HOLE";
    default:                return "??unkown??";
    }
}


/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void D_PAD::Show( int nestLevel, std::ostream& os )
{
    char padname[5] = { m_Padname[0], m_Padname[1], m_Padname[2], m_Padname[3], 0 };

    char layerMask[16];

    sprintf( layerMask, "0x%08X", m_Masque_Layer );

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " shape=\""     << ShowPadType( m_PadShape ) << '"' <<
    " attr=\""      << ShowPadAttr( m_Attribut ) << '"' <<
    " num=\""       << padname << '"' <<
    " net=\""       << m_Netname.mb_str() << '"' <<
    " netcode=\""   << GetNet() << '"' <<
    " layerMask=\"" << layerMask << '"' << m_Pos << "/>\n";

//    NestedSpace( nestLevel+1, os ) << m_Text.mb_str() << '\n';

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
