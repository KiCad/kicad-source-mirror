/****************************************************/
/* class_module.cpp : fonctions de la classe MODULE */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "wxstruct.h"
#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "trigo.h"

#ifdef PCBNEW
#include "autorout.h"
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "3d_struct.h"
#include "protos.h"

/*********************************************************************************/
void MODULE::DrawAncre( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                        int dim_ancre, int draw_mode )
/*********************************************************************************/

/* trace de l'ancre (croix verticale)
 *  (doit etre fait apres les pads,
 *  car le trace du trou efface tout donc peut etre l'ancre */
{
    int zoom = panel->GetZoom();
    int anchor_size = dim_ancre * zoom;

    GRSetDrawMode( DC, draw_mode );

    if( (g_AnchorColor & ITEM_NOT_SHOW) == 0 )
    {
        GRLine( &panel->m_ClipBox, DC,
            m_Pos.x - offset.x - anchor_size, m_Pos.y - offset.y,
            m_Pos.x - offset.x + anchor_size, m_Pos.y - offset.y,
            0, g_AnchorColor );
        GRLine( &panel->m_ClipBox, DC,
            m_Pos.x - offset.x, m_Pos.y - offset.y - anchor_size,
            m_Pos.x - offset.x, m_Pos.y - offset.y + anchor_size,
            0, g_AnchorColor );
    }
}


/*************************************************/
/* Class MODULE : description d'un composant pcb */
/*************************************************/
MODULE::MODULE( BOARD* parent ) :
    BOARD_ITEM( parent, TYPE_MODULE )
{
    m_Attributs    = MOD_DEFAULT;
    m_Layer        = CMP_N;
    m_Orient       = 0;
    m_ModuleStatus = 0;
    flag = 0;
    m_CntRot90 = m_CntRot180 = 0;
    m_Surface  = 0;
    m_Link = 0;
    m_LastEdit_Time = time( NULL );

    m_Reference = new TEXTE_MODULE( this, TEXT_is_REFERENCE );
    m_Reference->SetBack( this );

    m_Value = new TEXTE_MODULE( this, TEXT_is_VALUE );
    m_Value->SetBack( this );

    m_3D_Drawings.PushBack( new S3D_MASTER( this ) );
}


MODULE::~MODULE()
{
    delete m_Reference;
    delete m_Value;
}


/*********************************/
void MODULE::Copy( MODULE* aModule )
/*********************************/
{
    m_Pos           = aModule->m_Pos;
    m_Layer         = aModule->m_Layer;
    m_LibRef        = aModule->m_LibRef;
    m_Attributs     = aModule->m_Attributs;
    m_Orient        = aModule->m_Orient;
    m_BoundaryBox   = aModule->m_BoundaryBox;
    m_PadNum        = aModule->m_PadNum;
    m_CntRot90      = aModule->m_CntRot90;
    m_CntRot180     = aModule->m_CntRot180;
    m_LastEdit_Time = aModule->m_LastEdit_Time;
    m_Path          = aModule->m_Path; //is this correct behavior?
    m_TimeStamp     = GetTimeStamp();

    /* Copy des structures auxiliaires: Reference et value */
    m_Reference->Copy( aModule->m_Reference );
    m_Value->Copy( aModule->m_Value );

    /* Copie des structures auxiliaires: Pads */
    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
    {
        D_PAD* newpad = new D_PAD( this );
        newpad->Copy( pad );

        m_Pads.PushBack( newpad );
    }

    /* Copy des structures auxiliaires: Drawings */
    for( BOARD_ITEM* item = aModule->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_TEXTE_MODULE:
            TEXTE_MODULE* textm;
            textm = new TEXTE_MODULE( this );
            textm->Copy( (TEXTE_MODULE*) item );
            m_Drawings.PushBack( textm );
            break;

        case TYPE_EDGE_MODULE:
            EDGE_MODULE* edge;
            edge = new EDGE_MODULE( this );
            edge->Copy( (EDGE_MODULE*) item );
            m_Drawings.PushBack( edge );
            break;

        default:
            DisplayError( NULL, wxT( "Internal Err: CopyModule: type indefini" ) );
            break;
        }
    }

    for( S3D_MASTER* item = aModule->m_3D_Drawings;  item;  item = item->Next() )
    {
        S3D_MASTER* t3d = new S3D_MASTER( this );
        t3d->Copy( item );
        m_3D_Drawings.PushBack( t3d );
    }

    /* Copie des elements complementaires */
    m_Doc     = aModule->m_Doc;
    m_KeyWord = aModule->m_KeyWord;
}


/**********************************************************/
void MODULE::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                   int draw_mode, const wxPoint& offset )
/**********************************************************/

/** Function Draw
 *  Draws the footprint to the current Device Context
 *  @param panel = The active Draw Panel (used to know the clip box)
 *  @param DC = current Device Context
 *  @param offset = draw offset (usually wxPoint(0,0)
 *  @param draw_mode =  GR_OR, GR_XOR, GR_AND
 */
{
    if( (m_Flags & DO_NOT_DRAW) )
        return;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->m_Flags & IS_MOVED )
            continue;

        pad->Draw( panel, DC, draw_mode, offset );
    }

    // Draws foootprint anchor
    DrawAncre( panel, DC, offset, DIM_ANCRE_MODULE, draw_mode );

    /* Draw graphic items */
    if( !(m_Reference->m_Flags & IS_MOVED) )
        m_Reference->Draw( panel, DC, draw_mode, offset );

    if( !(m_Value->m_Flags & IS_MOVED) )
        m_Value->Draw( panel, DC, draw_mode, offset );

    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->m_Flags & IS_MOVED )
            continue;

        switch( item->Type() )
        {
        case TYPE_TEXTE_MODULE:
        case TYPE_EDGE_MODULE:
            item->Draw( panel, DC, draw_mode, offset );
            break;

        default:
            break;
        }
    }
}


/**************************************************************/
void MODULE::DrawEdgesOnly( WinEDA_DrawPanel* panel, wxDC* DC,
                            const wxPoint& offset, int draw_mode )
/**************************************************************/

/** Function DrawEdgesOnly
 *  Draws the footprint edges only to the current Device Context
 *  @param panel = The active Draw Panel (used to know the clip box)
 *  @param DC = current Device Context
 *  @param offset = draw offset (usually wxPoint(0,0)
 *  @param draw_mode =  GR_OR, GR_XOR, GR_AND
 */
{
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_EDGE_MODULE:
            item->Draw( panel, DC, draw_mode, offset );
            break;

        default:
            break;
        }
    }
}


/**************************************/
bool MODULE::Save( FILE* aFile ) const
/**************************************/
{
    char        statusTxt[8];
    BOARD_ITEM* item;

    if( GetState( DELETED ) )
        return true;

    bool rc = false;

    fprintf( aFile, "$MODULE %s\n", CONV_TO_UTF8( m_LibRef ) );

    // Generation des coord et caracteristiques
    memset( statusTxt, 0, sizeof(statusTxt) );
    if( IsLocked() )
        statusTxt[0] = 'F';
    else
        statusTxt[0] = '~';

    if( m_ModuleStatus & MODULE_is_PLACED )
        statusTxt[1] = 'P';
    else
        statusTxt[1] = '~';

    fprintf( aFile, "Po %d %d %d %d %8.8lX %8.8lX %s\n",
        m_Pos.x, m_Pos.y,
        m_Orient, m_Layer, m_LastEdit_Time,
        m_TimeStamp, statusTxt );

    fprintf( aFile, "Li %s\n", CONV_TO_UTF8( m_LibRef ) );

    if( !m_Doc.IsEmpty() )
    {
        fprintf( aFile, "Cd %s\n", CONV_TO_UTF8( m_Doc ) );
    }

    if( !m_KeyWord.IsEmpty() )
    {
        fprintf( aFile, "Kw %s\n", CONV_TO_UTF8( m_KeyWord ) );
    }

    fprintf( aFile, "Sc %8.8lX\n", m_TimeStamp );
    fprintf( aFile, "AR %s\n", CONV_TO_UTF8( m_Path ) );
    fprintf( aFile, "Op %X %X 0\n", m_CntRot90, m_CntRot180 );

    // attributes
    if( m_Attributs != MOD_DEFAULT )
    {
        fprintf( aFile, "At " );
        if( m_Attributs & MOD_CMS )
            fprintf( aFile, "SMD " );
        if( m_Attributs & MOD_VIRTUAL )
            fprintf( aFile, "VIRTUAL " );
        fprintf( aFile, "\n" );
    }

    // save reference
    if( !m_Reference->Save( aFile ) )
        goto out;

    // save value
    if( !m_Value->Save( aFile ) )
        goto out;

    // save drawing elements
    for( item = m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_TEXTE_MODULE:
        case TYPE_EDGE_MODULE:
            if( !item->Save( aFile ) )
                goto out;
            break;

        default:
#if defined (DEBUG)
            printf( "MODULE::Save() ignoring type %d\n", item->Type() );
#endif
            break;
        }
    }

    // save the pads
    for( item = m_Pads;  item;  item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    // Generation des informations de trac�3D
    Write_3D_Descr( aFile );

    fprintf( aFile, "$EndMODULE  %s\n", CONV_TO_UTF8( m_LibRef ) );

    rc = true;
out:
    return rc;
}


/***************************************/
int MODULE::Write_3D_Descr( FILE* File ) const
/***************************************/

/* Sauvegarde de la description 3D du MODULE
 */
{
    char             buf[512];
    S3D_MASTER* Struct3D = m_3D_Drawings;

    for( ; Struct3D != NULL; Struct3D = Struct3D->Next() )
    {
        if( !Struct3D->m_Shape3DName.IsEmpty() )
        {
            fprintf( File, "$SHAPE3D\n" );

            fprintf( File, "Na \"%s\"\n", CONV_TO_UTF8( Struct3D->m_Shape3DName ) );

            sprintf( buf, "Sc %lf %lf %lf\n",
                Struct3D->m_MatScale.x,
                Struct3D->m_MatScale.y,
                Struct3D->m_MatScale.z );
            fprintf( File, to_point( buf ) );

            sprintf( buf, "Of %lf %lf %lf\n",
                Struct3D->m_MatPosition.x,
                Struct3D->m_MatPosition.y,
                Struct3D->m_MatPosition.z );
            fprintf( File, to_point( buf ) );

            sprintf( buf, "Ro %lf %lf %lf\n",
                Struct3D->m_MatRotation.x,
                Struct3D->m_MatRotation.y,
                Struct3D->m_MatRotation.z );
            fprintf( File, to_point( buf ) );

            fprintf( File, "$EndSHAPE3D\n" );
        }
    }

    return 0;
}


/****************************************************/
int MODULE::Read_3D_Descr( FILE* File, int* LineNum )
/****************************************************/

/* Lecture de la description d'un MODULE (format Ascii)
 *  la 1ere ligne de descr ($MODULE) est supposee etre deja lue
 *  retourne 0 si OK
 */
{
    char             Line[1024];
    char*            text = Line + 3;

    S3D_MASTER* Struct3D = m_3D_Drawings;

    if( !Struct3D->m_Shape3DName.IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( this );

        m_3D_Drawings.PushBack( n3D );

        Struct3D = n3D;
    }

    while( GetLine( File, Line, LineNum, sizeof(Line) - 1 ) != NULL )
    {
        switch( Line[0] )
        {
        case '$':       // Fin de description
            if( Line[1] == 'E' )
                return 0;
            return 1;

        case 'N':       // Shape File Name
        {
            char buf[512];
            ReadDelimitedText( buf, text, 512 );
            Struct3D->m_Shape3DName = CONV_FROM_UTF8( buf );
            break;
        }

        case 'S':       // Scale
            sscanf( text, "%lf %lf %lf\n",
                &Struct3D->m_MatScale.x,
                &Struct3D->m_MatScale.y,
                &Struct3D->m_MatScale.z );
            break;

        case 'O':       // Offset
            sscanf( text, "%lf %lf %lf\n",
                &Struct3D->m_MatPosition.x,
                &Struct3D->m_MatPosition.y,
                &Struct3D->m_MatPosition.z );
            break;

        case 'R':       // Rotation
            sscanf( text, "%lf %lf %lf\n",
                &Struct3D->m_MatRotation.x,
                &Struct3D->m_MatRotation.y,
                &Struct3D->m_MatRotation.z );
            break;

        default:
            break;
        }
    }

    return 1;
}


/**************************************************/
int MODULE::ReadDescr( FILE* File, int* LineNum )
/**************************************************/

/* Lecture de la description d'un MODULE (format Ascii)
 *  la 1ere ligne de descr ($MODULE) est supposee etre deja lue
 *  retourne 0 si OK
 */
{
    char            Line[256], BufLine[256], BufCar1[128], * PtLine;
    int             itmp1, itmp2;

    while( GetLine( File, Line, LineNum, sizeof(Line) - 1 ) != NULL )
    {
        if( Line[0] == '$' )
        {
            if( Line[1] == 'E' )
                break;
            if( Line[1] == 'P' )
            {
                D_PAD* pad = new D_PAD( this );
                pad->ReadDescr( File, LineNum );
                RotatePoint( &pad->m_Pos.x, &pad->m_Pos.y, m_Orient );
                pad->m_Pos.x += m_Pos.x;
                pad->m_Pos.y += m_Pos.y;

                m_Pads.PushBack( pad );
                continue;
            }
            if( Line[1] == 'S' )
                Read_3D_Descr( File, LineNum );
        }

        if( strlen( Line ) < 4 )
            continue;

        PtLine = Line + 3;

        /* Pointe 1er code utile de la ligne */
        switch( Line[0] )
        {
        case 'P':
            memset( BufCar1, 0, sizeof(BufCar1) );
            sscanf( PtLine, "%d %d %d %d %lX %lX %s",
                &m_Pos.x, &m_Pos.y,
                &m_Orient, &m_Layer,
                &m_LastEdit_Time, &m_TimeStamp, BufCar1 );

            m_ModuleStatus = 0;
            if( BufCar1[0] == 'F' )
                SetLocked( true );
            if( BufCar1[1] == 'P' )
                m_ModuleStatus |= MODULE_is_PLACED;
            break;

        case 'L':       /* Li = Lecture du nom librairie du module */
            *BufLine = 0;
            sscanf( PtLine, " %s", BufLine );
            m_LibRef = CONV_FROM_UTF8( BufLine );
            break;

        case 'S':
            sscanf( PtLine, " %lX", &m_TimeStamp );
            break;


        case 'O':       /* (Op)tions de placement auto */
            itmp1 = itmp2 = 0;
            sscanf( PtLine, " %X %X", &itmp1, &itmp2 );

            m_CntRot180 = itmp2 & 0x0F;
            if( m_CntRot180 > 10 )
                m_CntRot180 = 10;

            m_CntRot90 = itmp1 & 0x0F;
            if( m_CntRot90 > 10 )
                m_CntRot90 = 0;
            itmp1 = (itmp1 >> 4) & 0x0F;
            if( itmp1 > 10 )
                itmp1 = 0;
            m_CntRot90 |= itmp1 << 4;
            break;

        case 'A':
            if( Line[1] == 't' )
            {
                /* At = (At)tributs du module */
                if( strstr( PtLine, "SMD" ) )
                    m_Attributs |= MOD_CMS;
                if( strstr( PtLine, "VIRTUAL" ) )
                    m_Attributs |= MOD_VIRTUAL;
            }
            if( Line[1] == 'R' )
            {
                //alternate reference, e.g. /478C2408/478AD1B6
                sscanf( PtLine, " %s", BufLine );
                m_Path = CONV_FROM_UTF8( BufLine );
            }
            break;

        case 'T':    /* Read a footprint text description (ref, value, or drawing */
            TEXTE_MODULE* textm;
            sscanf( Line + 1, "%d", &itmp1 );
            if( itmp1 == TEXT_is_REFERENCE )
                textm = m_Reference;
            else if( itmp1 == TEXT_is_VALUE )
                textm = m_Value;
            else        /* text is a drawing */
            {
                textm = new TEXTE_MODULE( this );
                m_Drawings.PushBack( textm );
            }
            textm->ReadDescr( Line, File, LineNum );
            break;

        case 'D':    /* lecture du contour */
            EDGE_MODULE* edge;
            edge = new EDGE_MODULE( this );
            m_Drawings.PushBack( edge );
            edge->ReadDescr( Line, File, LineNum );
            edge->SetDrawCoord();
            break;

        case 'C':    /* Lecture de la doc */
            m_Doc = CONV_FROM_UTF8( StrPurge( PtLine ) );
            break;

        case 'K':    /* Lecture de la liste des mots cle */
            m_KeyWord = CONV_FROM_UTF8( StrPurge( PtLine ) );
            break;

        default:
            break;
        }
    }

    /* Recalculate the bounding box */
    Set_Rectangle_Encadrement();
    return 0;
}


/*************************************************/
void MODULE::SetPosition( const wxPoint& newpos )
/*************************************************/

// replace le module en position newpos
{
    int deltaX = newpos.x - m_Pos.x;
    int deltaY = newpos.y - m_Pos.y;

    /* deplacement de l'ancre */
    m_Pos.x += deltaX;
    m_Pos.y += deltaY;

    /* deplacement de la reference */
    m_Reference->m_Pos.x += deltaX;
    m_Reference->m_Pos.y += deltaY;

    /* deplacement de la Valeur */
    m_Value->m_Pos.x += deltaX;
    m_Value->m_Pos.y += deltaY;

    /* deplacement des pastilles */
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->m_Pos.x += deltaX;
        pad->m_Pos.y += deltaY;
    }

    /* deplacement des dessins de l'empreinte : */
    EDA_BaseStruct* PtStruct = m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_EDGE_MODULE:
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) PtStruct;
            pt_edgmod->SetDrawCoord();
            break;
        }

        case TYPE_TEXTE_MODULE:
        {
            TEXTE_MODULE* pt_texte = (TEXTE_MODULE*) PtStruct;
            pt_texte->m_Pos.x += deltaX;
            pt_texte->m_Pos.y += deltaY;
            break;
        }

        default:
            DisplayError( NULL, wxT( "Type Draw Indefini" ) ); break;
        }
    }

    Set_Rectangle_Encadrement();
}


/*********************************************/
void MODULE::SetOrientation( int newangle )
/*********************************************/

/* Tourne de newangle (en 0.1 degres) le module
 */
{
    int px, py;

    newangle -= m_Orient;       // = delta de rotation

    m_Orient += newangle;
    NORMALIZE_ANGLE_POS( m_Orient );

    /* deplacement et rotation des pastilles */
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        px = pad->m_Pos0.x;
        py = pad->m_Pos0.y;

        pad->m_Orient += newangle; /* change m_Orientation */
        NORMALIZE_ANGLE_POS( pad->m_Orient );

        RotatePoint( &px, &py, (int) m_Orient );
        pad->m_Pos.x = m_Pos.x + px;
        pad->m_Pos.y = m_Pos.y + py;
    }

    /* mise a jour de la reference et de la valeur*/
    m_Reference->SetDrawCoord();
    m_Value->SetDrawCoord();

    /* deplacement des contours et textes de l'empreinte : */
    EDA_BaseStruct* PtStruct = m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() == TYPE_EDGE_MODULE )
        {
            EDGE_MODULE* pt_edgmod = (EDGE_MODULE*) PtStruct;
            pt_edgmod->SetDrawCoord();
        }
        if( PtStruct->Type() == TYPE_TEXTE_MODULE )
        {
            /* deplacement des inscriptions : */
            TEXTE_MODULE* pt_texte = (TEXTE_MODULE*) PtStruct;
            pt_texte->SetDrawCoord();
        }
    }

    /* Recalcul du rectangle d'encadrement */
    Set_Rectangle_Encadrement();
}


/************************************************/
void MODULE::Set_Rectangle_Encadrement()
/************************************************/

/* Mise a jour du rectangle d'encadrement du module
 *  Entree : pointeur sur module
 *  Le rectangle d'encadrement est le rectangle comprenant les contours et les
 *  pads.
 *  Le rectangle est calcule:
 *      pour orient 0
 *      en coord relatives / position ancre
 */
{
    int          width;
    int          cx, cy, uxf, uyf, rayon;
    int          xmax, ymax;


    /* Init des pointeurs */
    /* Init des coord du cadre a une valeur limite non nulle */
    m_BoundaryBox.m_Pos.x = -500; xmax = 500;
    m_BoundaryBox.m_Pos.y = -500; ymax = 500;

    /* Contours: Recherche des coord min et max et mise a jour du cadre */
    for( EDGE_MODULE* pt_edge_mod = (EDGE_MODULE*) m_Drawings.GetFirst();
        pt_edge_mod; pt_edge_mod = pt_edge_mod->Next() )
    {
        if( pt_edge_mod->Type() != TYPE_EDGE_MODULE )
            continue;

        width = pt_edge_mod->m_Width / 2;

        switch( pt_edge_mod->m_Shape )
        {
        case S_ARC:
        case S_CIRCLE:
        {
            cx     = pt_edge_mod->m_Start0.x; cy = pt_edge_mod->m_Start0.y;  // centre
            uxf    = pt_edge_mod->m_End0.x; uyf = pt_edge_mod->m_End0.y;
            rayon  = (int) hypot( (double) (cx - uxf), (double) (cy - uyf) );
            rayon += width;
            m_BoundaryBox.m_Pos.x = MIN( m_BoundaryBox.m_Pos.x, cx - rayon );
            m_BoundaryBox.m_Pos.y = MIN( m_BoundaryBox.m_Pos.y, cy - rayon );
            xmax = MAX( xmax, cx + rayon );
            ymax = MAX( ymax, cy + rayon );
            break;
        }

        default:
            m_BoundaryBox.m_Pos.x = MIN( m_BoundaryBox.m_Pos.x, pt_edge_mod->m_Start0.x - width );
            m_BoundaryBox.m_Pos.x = MIN( m_BoundaryBox.m_Pos.x, pt_edge_mod->m_End0.x - width );
            m_BoundaryBox.m_Pos.y = MIN( m_BoundaryBox.m_Pos.y, pt_edge_mod->m_Start0.y - width );
            m_BoundaryBox.m_Pos.y = MIN( m_BoundaryBox.m_Pos.y, pt_edge_mod->m_End0.y - width );
            xmax = MAX( xmax, pt_edge_mod->m_Start0.x + width );
            xmax = MAX( xmax, pt_edge_mod->m_End0.x + width );
            ymax = MAX( ymax, pt_edge_mod->m_Start0.y + width );
            ymax = MAX( ymax, pt_edge_mod->m_End0.y + width );
            break;
        }
    }

    /* Pads:  Recherche des coord min et max et mise a jour du cadre */
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        rayon = pad->m_Rayon;
        cx    = pad->m_Pos0.x; cy = pad->m_Pos0.y;
        m_BoundaryBox.m_Pos.x = MIN( m_BoundaryBox.m_Pos.x, cx - rayon );
        m_BoundaryBox.m_Pos.y = MIN( m_BoundaryBox.m_Pos.y, cy - rayon );
        xmax = MAX( xmax, cx + rayon );
        ymax = MAX( ymax, cy + rayon );
    }

    m_BoundaryBox.SetWidth( xmax - m_BoundaryBox.m_Pos.x );
    m_BoundaryBox.SetHeight( ymax - m_BoundaryBox.m_Pos.y );
}


/****************************************/
void MODULE::SetRectangleExinscrit()
/****************************************/

/*	Analogue a MODULE::Set_Rectangle_Encadrement() mais en coord reelles:
 *  Mise a jour du rectangle d'encadrement reel du module c.a.d en coord PCB
 *  Entree : pointeur sur module
 *  Le rectangle d'encadrement est le rectangle comprenant les contours et les
 *  pads.
 *  Met egalement a jour la surface (.m_Surface) du module.
 */
{
    int          width;
    int          cx, cy, uxf, uyf, rayon;
    int          xmax, ymax;

    m_RealBoundaryBox.m_Pos.x = xmax = m_Pos.x;
    m_RealBoundaryBox.m_Pos.y = ymax = m_Pos.y;

    /* Contours: Recherche des coord min et max et mise a jour du cadre */
    for( EDGE_MODULE* edge = (EDGE_MODULE*) m_Drawings.GetFirst();  edge; edge = edge->Next() )
    {
        if( edge->Type() != TYPE_EDGE_MODULE )
            continue;

        width = edge->m_Width / 2;

        switch( edge->m_Shape )
        {
        case S_ARC:
        case S_CIRCLE:
        {
            cx     = edge->m_Start.x; cy = edge->m_Start.y;  // centre
            uxf    = edge->m_End.x; uyf = edge->m_End.y;
            rayon  = (int) hypot( (double) (cx - uxf), (double) (cy - uyf) );
            rayon += width;
            m_RealBoundaryBox.m_Pos.x = MIN( m_RealBoundaryBox.m_Pos.x, cx - rayon );
            m_RealBoundaryBox.m_Pos.y = MIN( m_RealBoundaryBox.m_Pos.y, cy - rayon );
            xmax = MAX( xmax, cx + rayon );
            ymax = MAX( ymax, cy + rayon );
            break;
        }

        default:
            m_RealBoundaryBox.m_Pos.x = MIN( m_RealBoundaryBox.m_Pos.x, edge->m_Start.x - width );
            m_RealBoundaryBox.m_Pos.x = MIN( m_RealBoundaryBox.m_Pos.x, edge->m_End.x - width );
            m_RealBoundaryBox.m_Pos.y = MIN( m_RealBoundaryBox.m_Pos.y, edge->m_Start.y - width );
            m_RealBoundaryBox.m_Pos.y = MIN( m_RealBoundaryBox.m_Pos.y, edge->m_End.y - width );
            xmax = MAX( xmax, edge->m_Start.x + width );
            xmax = MAX( xmax, edge->m_End.x + width );
            ymax = MAX( ymax, edge->m_Start.y + width );
            ymax = MAX( ymax, edge->m_End.y + width );
            break;
        }
    }

    /* Pads:  Recherche des coord min et max et mise a jour du cadre */
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        rayon = pad->m_Rayon;

        cx = pad->m_Pos.x;
        cy = pad->m_Pos.y;

        m_RealBoundaryBox.m_Pos.x = MIN( m_RealBoundaryBox.m_Pos.x, cx - rayon );
        m_RealBoundaryBox.m_Pos.y = MIN( m_RealBoundaryBox.m_Pos.y, cy - rayon );

        xmax = MAX( xmax, cx + rayon );
        ymax = MAX( ymax, cy + rayon );
    }

    m_RealBoundaryBox.SetWidth( xmax - m_RealBoundaryBox.m_Pos.x );
    m_RealBoundaryBox.SetHeight( ymax - m_RealBoundaryBox.m_Pos.y );
    m_Surface = ABS( (float) m_RealBoundaryBox.GetWidth() * m_RealBoundaryBox.GetHeight() );
}


/**
 * Function GetBoundingBox
 * returns the full bounding box of this Footprint, including texts
 * Mainly used to redraw the screen area occuped by the footprint
 */
EDA_Rect MODULE::GetBoundingBox()
{
    // Calculate area without text fields:
    SetRectangleExinscrit();
    EDA_Rect      area = m_RealBoundaryBox;

    // Calculate extended area including text field:
    EDA_Rect      text_area;
    text_area = m_Reference->GetBoundingBox();
    area.Merge( text_area );

    text_area = m_Value->GetBoundingBox();
    area.Merge( text_area );

    for( EDGE_MODULE* edge = (EDGE_MODULE*) m_Drawings.GetFirst();  edge; edge = edge->Next() )
    {
        if( edge->Type() != TYPE_TEXTE_MODULE )
            continue;
        text_area = ((TEXTE_MODULE*)edge)->GetBoundingBox();
        area.Merge( text_area );
    }

    // Add the Clearence shape size: (shape around the pads when the clearence is shown
    // Not optimized, but the draw cost is small (perhaps smaller than optimization)
    area.Inflate(g_DesignSettings.m_TrackClearence, g_DesignSettings.m_TrackClearence);

    return area;
}


/*******************************************************/
void MODULE::Display_Infos( WinEDA_DrawFrame* frame )
/*******************************************************/
{
    int      nbpad;
    char     bufcar[512], Line[512];
    int      pos;
    bool     flag = FALSE;
    wxString msg;
    BOARD*   board = (BOARD*) m_Parent;

    frame->MsgPanel->EraseMsgBox();    /* Effacement de la zone message */
    if( frame->m_Ident != PCB_FRAME )
        flag = TRUE;
    pos = 1;
    Affiche_1_Parametre( frame, pos, m_Reference->m_Text, m_Value->m_Text, DARKCYAN );

    /* Affiche signature temporelle ou date de modif (en edition de modules) */
    pos += 6;
    if( flag ) // Affichage date de modification (utile en Module Editor)
    {
        time_t edit_time = m_LastEdit_Time;
        strcpy( Line, ctime( &edit_time ) );
        strtok( Line, " \n\r" );
        strcpy( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, " " );
        strcat( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, ", " );
        strtok( NULL, " \n\r" );
        strcat( bufcar, strtok( NULL, " \n\r" ) );
        msg = CONV_FROM_UTF8( bufcar );
        Affiche_1_Parametre( frame, pos, _( "Last Change" ), msg, BROWN );
        pos += 4;
    }
    else
    {
        msg.Printf( wxT( "%8.8lX" ), m_TimeStamp );
        Affiche_1_Parametre( frame, pos, _( "Netlist path" ), /*msg*/ m_Path, BROWN );
    }

    pos += 12;
    Affiche_1_Parametre( frame, pos, _( "Layer" ), board->GetLayerName( m_Layer ), RED );

    pos += 6;
    EDA_BaseStruct* PtStruct = m_Pads;
    nbpad = 0;
    while( PtStruct )
    {
        nbpad++;
        PtStruct = PtStruct->Next();
    }

    msg.Printf( wxT( "%d" ), nbpad );
    Affiche_1_Parametre( frame, pos, _( "Pads" ), msg, BLUE );

    pos += 4;
    msg  = wxT( ".." );
    if( IsLocked() )
        msg[0] = 'L';
    if( m_ModuleStatus & MODULE_is_PLACED )
        msg[1] = 'P';
    Affiche_1_Parametre( frame, pos, _( "Stat" ), msg, MAGENTA );

    pos += 4;
    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    Affiche_1_Parametre( frame, pos, _( "Orient" ), msg, BROWN );

    pos += 5;
    Affiche_1_Parametre( frame, pos, _( "Module" ), m_LibRef, BLUE );

    pos += 9;
    Affiche_1_Parametre( frame, pos, _( "3D-Shape" ),
        m_3D_Drawings->m_Shape3DName, RED );

    pos += 14;
    wxString doc     = _( "Doc:  " ) + m_Doc;
    wxString keyword = _( "KeyW: " ) + m_KeyWord;
    Affiche_1_Parametre( frame, pos, doc, keyword, BLACK );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool MODULE::HitTest( const wxPoint& refPos )
{
    /* Calcul des coord souris dans le repere module */
    int spot_cX = refPos.x - m_Pos.x;
    int spot_cY = refPos.y - m_Pos.y;

    RotatePoint( &spot_cX, &spot_cY, -m_Orient );

    /* la souris est-elle dans ce rectangle : */
    if( m_BoundaryBox.Inside( spot_cX, spot_cY ) )
        return true;

    return false;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect the bounds of this object.
 * @param refArea : the given EDA_Rect
 * @return bool - true if a hit, else false
 */
bool MODULE::HitTest( EDA_Rect& refArea )
{
    bool is_out_of_box = false;

    SetRectangleExinscrit();

    if( m_RealBoundaryBox.m_Pos.x < refArea.GetX() )
        is_out_of_box = true;
    if( m_RealBoundaryBox.m_Pos.y < refArea.GetY() )
        is_out_of_box = true;
    if( m_RealBoundaryBox.GetRight() > refArea.GetRight() )
        is_out_of_box = true;
    if( m_RealBoundaryBox.GetBottom() > refArea.GetBottom() )
        is_out_of_box = true;

    return is_out_of_box ? false : true;
}


D_PAD* MODULE::FindPadByName( const wxString& aPadName ) const
{
    wxString buf;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        pad->ReturnStringPadName( buf );
#if 1
        if( buf.CmpNoCase( aPadName ) == 0 )    // why case insensitive?
#else
        if( buf == aPadName )
#endif

            return pad;
    }

    return NULL;
}


// see class_module.h
SEARCH_RESULT MODULE::Visit( INSPECTOR* inspector, const void* testData,
                             const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_CONTINUE;
    const KICAD_T* p    = scanTypes;
    bool           done = false;

#if 0 && defined (DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    while( !done )
    {
        stype = *p;

        switch( stype )
        {
        case TYPE_MODULE:
            result = inspector->Inspect( this, testData );  // inspect me
            ++p;
            break;

        case TYPE_PAD:
            result = IterateForward( m_Pads, inspector, testData, p );
            ++p;
            break;

        case TYPE_TEXTE_MODULE:
            result = inspector->Inspect( m_Reference, testData );
            if( result == SEARCH_QUIT )
                break;

            result = inspector->Inspect( m_Value, testData );
            if( result == SEARCH_QUIT )
                break;

            // m_Drawings can hold TYPETEXTMODULE also, so fall thru

        case TYPE_EDGE_MODULE:
            result = IterateForward( m_Drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case TYPE_TEXTE_MODULE:
                case TYPE_EDGE_MODULE:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        default:
            done = true;
            break;
        }

        if( result == SEARCH_QUIT )
            break;
    }

    return result;
}


#if defined (DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void MODULE::Show( int nestLevel, std::ostream& os )
{
    BOARD* board = (BOARD*) m_Parent;

    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " ref=\"" << m_Reference->m_Text.mb_str() << '"' <<
    " value=\"" << m_Value->m_Text.mb_str() << '"' <<
    " layer=\"" << board->GetLayerName( m_Layer ).mb_str() << '"' <<
    ">\n";

    NestedSpace( nestLevel + 1, os ) <<
    "<boundingBox" << m_BoundaryBox.m_Pos << m_BoundaryBox.m_Size << "/>\n";

    NestedSpace( nestLevel + 1, os ) << "<orientation tenths=\"" << m_Orient << "\"/>\n";

    EDA_BaseStruct* p;

    NestedSpace( nestLevel + 1, os ) << "<mpads>\n";
    p = m_Pads;
    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );

    NestedSpace( nestLevel + 1, os ) << "</mpads>\n";

    NestedSpace( nestLevel + 1, os ) << "<mdrawings>\n";
    p = m_Drawings;
    for( ; p; p = p->Next() )
        p->Show( nestLevel + 2, os );

    NestedSpace( nestLevel + 1, os ) << "</mdrawings>\n";

    p = m_Son;
    for( ; p;  p = p->Next() )
    {
        p->Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}


#endif
