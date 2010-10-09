/****************************************************/
/* class_module.cpp : MODULE class implementation.  */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "plot_common.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "confirm.h"
#include "kicad_string.h"
#include "pcbcommon.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

#include "autorout.h"
#include "drag.h"

#include "3d_struct.h"
#include "protos.h"


/*********************************************/
/* Class MODULE : describes a pcb component. */
/*********************************************/
MODULE::MODULE( BOARD* parent ) :
    BOARD_ITEM( parent, TYPE_MODULE )
{
    m_Attributs    = MOD_DEFAULT;
    m_Layer        = LAYER_N_FRONT;
    m_Orient       = 0;
    m_ModuleStatus = 0;
    flag = 0;
    m_CntRot90 = m_CntRot180 = 0;
    m_Surface  = 0.0;
    m_Link     = 0;
    m_LastEdit_Time  = time( NULL );
    m_LocalClearance = 0;
    m_LocalSolderMaskMargin  = 0;
    m_LocalSolderPasteMargin = 0;
    m_LocalSolderPasteMarginRatio = 0.0;

    m_Reference = new TEXTE_MODULE( this, TEXT_is_REFERENCE );

    m_Value = new TEXTE_MODULE( this, TEXT_is_VALUE );

    // Reserve one void 3D entry, to avoid problems with void list
    m_3D_Drawings.PushBack( new S3D_MASTER( this ) );
}


MODULE::~MODULE()
{
    delete m_Reference;
    delete m_Value;
}


/* Draw the anchor cross (vertical)
 * Must be done after the pads, because drawing the hole will erase overwrite
 * every thing already drawn.
 */
void MODULE::DrawAncre( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                        int dim_ancre, int draw_mode )
{
#ifdef USE_WX_ZOOM
    int anchor_size = DC->DeviceToLogicalXRel( dim_ancre );
#else
    int anchor_size = panel->GetScreen()->Unscale( dim_ancre );
#endif

    GRSetDrawMode( DC, draw_mode );

    if( GetBoard()->IsElementVisible( ANCHOR_VISIBLE ) )
    {
        int color = g_ColorsSettings.GetItemColor( ANCHOR_VISIBLE );
        GRLine( &panel->m_ClipBox, DC,
                m_Pos.x - offset.x - anchor_size, m_Pos.y - offset.y,
                m_Pos.x - offset.x + anchor_size, m_Pos.y - offset.y,
                0, color );
        GRLine( &panel->m_ClipBox, DC,
                m_Pos.x - offset.x, m_Pos.y - offset.y - anchor_size,
                m_Pos.x - offset.x, m_Pos.y - offset.y + anchor_size,
                0, color );
    }
}


void MODULE::Copy( MODULE* aModule )
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
    m_Link          = aModule->m_Link;
    m_Path          = aModule->m_Path; //is this correct behavior?
    m_TimeStamp     = GetTimeStamp();
    m_LocalSolderMaskMargin  = aModule->m_LocalSolderMaskMargin;
    m_LocalSolderPasteMargin = aModule->m_LocalSolderPasteMargin;
    m_LocalSolderPasteMarginRatio = aModule->m_LocalSolderPasteMarginRatio;

    /* Copy reference and value. */
    m_Reference->Copy( aModule->m_Reference );
    m_Value->Copy( aModule->m_Value );

    /* Copy auxiliary data: Pads */
    m_Pads.DeleteAll();
    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
    {
        D_PAD* newpad = new D_PAD( this );
        newpad->Copy( pad );

        m_Pads.PushBack( newpad );
    }

    /* Copy auxiliary data: Drawings */
    m_Drawings.DeleteAll();
    for( BOARD_ITEM* item = aModule->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_TEXTE_MODULE:
            TEXTE_MODULE * textm;
            textm = new TEXTE_MODULE( this );
            textm->Copy( (TEXTE_MODULE*) item );
            m_Drawings.PushBack( textm );
            break;

        case TYPE_EDGE_MODULE:
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            edge->Copy( (EDGE_MODULE*) item );
            m_Drawings.PushBack( edge );
            break;

        default:
            wxMessageBox( wxT( "MODULE::Copy() Internal Err:  unknown type" ) );
            break;
        }
    }

    /* Copy auxiliary data: 3D_Drawings info */
    m_3D_Drawings.DeleteAll();

    // Ensure there is one (or more) item in m_3D_Drawings
    m_3D_Drawings.PushBack( new S3D_MASTER( this ) ); // push a void item
    for( S3D_MASTER* item = aModule->m_3D_Drawings;  item;  item = item->Next() )
    {
        if( item->m_Shape3DName.IsEmpty() )           // do not copy empty shapes.
            continue;
        S3D_MASTER* t3d = m_3D_Drawings;
        if( t3d && t3d->m_Shape3DName.IsEmpty() )       // The first entry can
                                                        // exist, but is empty :
                                                        // use it.
            t3d->Copy( item );
        else
        {
            t3d = new S3D_MASTER( this );
            t3d->Copy( item );
            m_3D_Drawings.PushBack( t3d );
        }
    }

    m_Doc     = aModule->m_Doc;
    m_KeyWord = aModule->m_KeyWord;
}


/** Function Draw
 *  Draws the footprint to the current Device Context
 *  @param panel = The active Draw Panel (used to know the clip box)
 *  @param DC = current Device Context
 *  @param offset = draw offset (usually wxPoint(0,0)
 *  @param draw_mode =  GR_OR, GR_XOR, GR_AND
 */
void MODULE::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                   int draw_mode, const wxPoint& offset )
{
    if( (m_Flags & DO_NOT_DRAW) || (m_Flags & IS_MOVED) )
        return;

    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->m_Flags & IS_MOVED )
            continue;

        pad->Draw( panel, DC, draw_mode, offset );
    }

    BOARD* brd = GetBoard();

    // Draws footprint anchor
    DrawAncre( panel, DC, offset, DIM_ANCRE_MODULE, draw_mode );

    /* Draw graphic items */
    if( brd->IsElementVisible( MOD_REFERENCES_VISIBLE ) )
    {
        if( !(m_Reference->m_Flags & IS_MOVED) )
            m_Reference->Draw( panel, DC, draw_mode, offset );
    }

    if( brd->IsElementVisible( MOD_VALUES_VISIBLE ) )
    {
        if( !(m_Value->m_Flags & IS_MOVED) )
            m_Value->Draw( panel, DC, draw_mode, offset );
    }

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


/** Function DrawEdgesOnly
 *  Draws the footprint edges only to the current Device Context
 *  @param panel = The active Draw Panel (used to know the clip box)
 *  @param DC = current Device Context
 *  @param offset = draw offset (usually wxPoint(0,0)
 *  @param draw_mode =  GR_OR, GR_XOR, GR_AND
 */
void MODULE::DrawEdgesOnly( WinEDA_DrawPanel* panel, wxDC* DC,
                            const wxPoint& offset, int draw_mode )
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


bool MODULE::Save( FILE* aFile ) const
{
    char        statusTxt[8];
    BOARD_ITEM* item;

    if( GetState( DELETED ) )
        return true;

    bool rc = false;

    fprintf( aFile, "$MODULE %s\n", CONV_TO_UTF8( m_LibRef ) );

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
    if( m_LocalSolderMaskMargin != 0 )
        fprintf( aFile, ".SolderMask %d\n", m_LocalSolderMaskMargin );
    if( m_LocalSolderPasteMargin != 0 )
        fprintf( aFile, ".SolderPaste %d\n", m_LocalSolderPasteMargin );
    if( m_LocalSolderPasteMarginRatio != 0 )
        fprintf( aFile, ".SolderPasteRatio %g\n", m_LocalSolderPasteMarginRatio );
    if( m_LocalClearance != 0 )
        fprintf( aFile, ".LocalClearance %d\n", m_LocalClearance );

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
#if defined(DEBUG)
            printf( "MODULE::Save() ignoring type %d\n", item->Type() );
#endif
            break;
        }
    }

    // save the pads
    for( item = m_Pads;  item;  item = item->Next() )
        if( !item->Save( aFile ) )
            goto out;

    Write_3D_Descr( aFile );

    fprintf( aFile, "$EndMODULE  %s\n", CONV_TO_UTF8( m_LibRef ) );

    rc = true;
out:
    return rc;
}


/* Save the description of 3D MODULE
 */
int MODULE::Write_3D_Descr( FILE* File ) const
{
    char buf[512];

    for( S3D_MASTER* t3D = m_3D_Drawings;  t3D;  t3D = t3D->Next() )
    {
        if( !t3D->m_Shape3DName.IsEmpty() )
        {
            fprintf( File, "$SHAPE3D\n" );

            fprintf( File, "Na \"%s\"\n", CONV_TO_UTF8( t3D->m_Shape3DName ) );

            sprintf( buf, "Sc %lf %lf %lf\n",
                     t3D->m_MatScale.x,
                     t3D->m_MatScale.y,
                     t3D->m_MatScale.z );
            fprintf( File, "%s", to_point( buf ) );

            sprintf( buf, "Of %lf %lf %lf\n",
                     t3D->m_MatPosition.x,
                     t3D->m_MatPosition.y,
                     t3D->m_MatPosition.z );
            fprintf( File, "%s", to_point( buf ) );

            sprintf( buf, "Ro %lf %lf %lf\n",
                     t3D->m_MatRotation.x,
                     t3D->m_MatRotation.y,
                     t3D->m_MatRotation.z );
            fprintf( File, "%s", to_point( buf ) );

            fprintf( File, "$EndSHAPE3D\n" );
        }
    }

    return 0;
}


/* Read 3D module from file. (Ascii)
 * The 1st line of descr ($MODULE) is assumed to be already read
 * Returns 0 if OK
 */
int MODULE::Read_3D_Descr( FILE* File, int* LineNum )
{
    char        Line[1024];
    char*       text = Line + 3;

    S3D_MASTER* t3D = m_3D_Drawings;

    if( !t3D->m_Shape3DName.IsEmpty() )
    {
        S3D_MASTER* n3D = new S3D_MASTER( this );

        m_3D_Drawings.PushBack( n3D );

        t3D = n3D;
    }

    while( GetLine( File, Line, LineNum, sizeof(Line) - 1 ) != NULL )
    {
        switch( Line[0] )
        {
        case '$':
            if( Line[1] == 'E' )
                return 0;
            return 1;

        case 'N':       // Shape File Name
        {
            char buf[512];
            ReadDelimitedText( buf, text, 512 );
            t3D->m_Shape3DName = CONV_FROM_UTF8( buf );
            break;
        }

        case 'S':       // Scale
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatScale.x,
                    &t3D->m_MatScale.y,
                    &t3D->m_MatScale.z );
            break;

        case 'O':       // Offset
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatPosition.x,
                    &t3D->m_MatPosition.y,
                    &t3D->m_MatPosition.z );
            break;

        case 'R':       // Rotation
            sscanf( text, "%lf %lf %lf\n",
                    &t3D->m_MatRotation.x,
                    &t3D->m_MatRotation.y,
                    &t3D->m_MatRotation.z );
            break;

        default:
            break;
        }
    }

    return 1;
}


/* Read a MODULE description
 *  The first description line ($MODULE) is already read
 *  @return 0 if no error
 */
int MODULE::ReadDescr( FILE* File, int* LineNum )
{
    char Line[256], BufLine[256], BufCar1[128], * PtLine;
    int  itmp1, itmp2;

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
                RotatePoint( &pad->m_Pos, m_Orient );
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

        /* Decode the first code of the current line and read the
         * corresponding data
         */
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

        case 'L':       /* Li = read the library name of the footprint */
            *BufLine = 0;
            sscanf( PtLine, " %s", BufLine );
            m_LibRef = CONV_FROM_UTF8( BufLine );
            break;

        case 'S':
            sscanf( PtLine, " %lX", &m_TimeStamp );
            break;


        case 'O':       /* (Op)tions for auto placement */
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
                /* At = (At)tributes of module */
                if( strstr( PtLine, "SMD" ) )
                    m_Attributs |= MOD_CMS;
                if( strstr( PtLine, "VIRTUAL" ) )
                    m_Attributs |= MOD_VIRTUAL;
            }
            if( Line[1] == 'R' )
            {
                // alternate reference, e.g. /478C2408/478AD1B6
                sscanf( PtLine, " %s", BufLine );
                m_Path = CONV_FROM_UTF8( BufLine );
            }
            break;

        case 'T':    /* Read a footprint text description (ref, value, or
                      * drawing */
            TEXTE_MODULE * textm;
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

        case 'D':    /* read a drawing item */
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( this );
            m_Drawings.PushBack( edge );
            edge->ReadDescr( Line, File, LineNum );
            edge->SetDrawCoord();
            break;

        case 'C':    /* read documentation data */
            m_Doc = CONV_FROM_UTF8( StrPurge( PtLine ) );
            break;

        case 'K':    /* Read key words */
            m_KeyWord = CONV_FROM_UTF8( StrPurge( PtLine ) );
            break;

        case '.':    /* Read specific data */
            if( strnicmp( Line, ".SolderMask ", 12 ) == 0 )
                m_LocalSolderMaskMargin = atoi( Line + 12 );
            else if( strnicmp( Line, ".SolderPaste ", 13 )  == 0 )
                m_LocalSolderPasteMargin = atoi( Line + 13 );
            else if( strnicmp( Line, ".SolderPasteRatio ", 18 ) == 0 )
                m_LocalSolderPasteMarginRatio = atof( Line + 18 );
            else if( strnicmp( Line, ".LocalClearance ", 16 ) == 0 )
                m_LocalClearance = atoi( Line + 16 );
            break;

        default:
            break;
        }
    }

    /* Recalculate the bounding box */
    Set_Rectangle_Encadrement();
    return 0;
}


/* Update the bounding rectangle of the module
 *
 * The bounding box includes outlines and pads, but not the fields.
 * The rectangle is:
 *      for orientation 0
 *      coordinates relative to the module anchor.
 */
void MODULE::Set_Rectangle_Encadrement()
{
    int width;
    int cx, cy, uxf, uyf, rayon;
    int xmax, ymax;
    int xmin, ymin;

    /* Initial coordinates of the module has a nonzero limit value. */
    xmin = ymin = -250;
    xmax = ymax = 250;

    for( EDGE_MODULE* edge = (EDGE_MODULE*) m_Drawings.GetFirst();
        edge; edge = edge->Next() )
    {
        if( edge->Type() != TYPE_EDGE_MODULE )
            continue;

        width = edge->m_Width / 2;

        switch( edge->m_Shape )
        {
        case S_ARC:
        case S_CIRCLE:
        {
            cx     = edge->m_Start0.x;
            cy     = edge->m_Start0.y; // center
            uxf    = edge->m_End0.x;
            uyf    = edge->m_End0.y;
            rayon  = (int) hypot( (double) ( cx - uxf ), (double) ( cy - uyf ) );
            rayon += width;
            xmin   = MIN( xmin, cx - rayon );
            ymin   = MIN( ymin, cy - rayon );
            xmax   = MAX( xmax, cx + rayon );
            ymax   = MAX( ymax, cy + rayon );
            break;
        }

        case S_SEGMENT:
            xmin = MIN( xmin, edge->m_Start0.x - width );
            xmin = MIN( xmin, edge->m_End0.x - width );
            ymin = MIN( ymin, edge->m_Start0.y - width );
            ymin = MIN( ymin, edge->m_End0.y - width );
            xmax = MAX( xmax, edge->m_Start0.x + width );
            xmax = MAX( xmax, edge->m_End0.x + width );
            ymax = MAX( ymax, edge->m_Start0.y + width );
            ymax = MAX( ymax, edge->m_End0.y + width );
            break;

        case S_POLYGON:
            for( unsigned ii = 0; ii < edge->m_PolyPoints.size(); ii++ )
            {
                wxPoint pt = edge->m_PolyPoints[ii];
                xmin = MIN( xmin, (pt.x - width) );
                ymin = MIN( ymin, (pt.y - width) );
                xmax = MAX( xmax, (pt.x + width) );
                ymax = MAX( ymax, (pt.y + width) );
            }

            break;
        }
    }

    /* Pads: find the min and max coordinates and update the bounding box.
     */
    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        rayon = pad->m_ShapeMaxRadius;
        cx    = pad->m_Pos0.x;
        cy    = pad->m_Pos0.y;
        xmin  = MIN( xmin, cx - rayon );
        ymin  = MIN( ymin, cy - rayon );
        xmax  = MAX( xmax, cx + rayon );
        ymax  = MAX( ymax, cy + rayon );
    }

    m_BoundaryBox.m_Pos.x = xmin;
    m_BoundaryBox.m_Pos.y = ymin;
    m_BoundaryBox.SetWidth( xmax - xmin );
    m_BoundaryBox.SetHeight( ymax - ymin );
}


/* Equivalent to Module::Set_Rectangle_Encadrement() but in board coordinates:
 * Updates the module bounding box on the board
 * The rectangle is the rectangle with outlines and pads, but not the fields
 * Also updates the surface (.M_Surface) module.
 */
void MODULE::SetRectangleExinscrit()
{
    m_RealBoundaryBox.m_Pos = m_Pos;
    m_RealBoundaryBox.SetEnd( m_Pos );
    m_RealBoundaryBox.Inflate( 500 );       // Give a min size

    for( EDGE_MODULE* edge = (EDGE_MODULE*) m_Drawings.GetFirst();
        edge; edge = edge->Next() )
    {
        if( edge->Type() != TYPE_EDGE_MODULE )  // Shoud not occur
            continue;

        EDA_Rect rect = edge->GetBoundingBox();
        m_RealBoundaryBox.Merge( rect );
    }


    for( D_PAD* pad = m_Pads;  pad;  pad = pad->Next() )
    {
        EDA_Rect rect = pad->GetBoundingBox();
        m_RealBoundaryBox.Merge( rect );
    }

    m_Surface = ABS( (double) m_RealBoundaryBox.GetWidth()
                    * m_RealBoundaryBox.GetHeight() );
}


/**
 * Function GetBoundingBox
 * returns the full bounding box of this Footprint, including fields
 * Mainly used to redraw the screen area occupied by the footprint
 */
EDA_Rect MODULE::GetBoundingBox()
{
    // Calculate area without text fields:
    SetRectangleExinscrit();
    EDA_Rect area = m_RealBoundaryBox;

    // Calculate extended area including text field:
    EDA_Rect text_area;
    text_area = m_Reference->GetBoundingBox();
    area.Merge( text_area );

    text_area = m_Value->GetBoundingBox();
    area.Merge( text_area );

    for( EDGE_MODULE* edge = (EDGE_MODULE*) m_Drawings.GetFirst(); edge;
        edge = edge->Next() )
    {
        if( edge->Type() != TYPE_TEXTE_MODULE )
            continue;
        text_area = ( (TEXTE_MODULE*) edge )->GetBoundingBox();
        area.Merge( text_area );
    }

    // Add the Clearance shape size: (shape around the pads when the
    // clearance is shown.  Not optimized, but the draw cost is small
    // (perhaps smaller than optimization).
    int biggest_clearance = GetBoard()->GetBiggestClearanceValue();
    area.Inflate( biggest_clearance );

    return area;
}


/* Virtual function, from EDA_BaseStruct.
 * display module info on MsgPanel
 */
void MODULE::DisplayInfo( WinEDA_DrawFrame* frame )
{
    int      nbpad;
    char     bufcar[512], Line[512];
    bool     flag = FALSE;
    wxString msg;
    BOARD*   board = GetBoard();

    frame->EraseMsgBox();
    if( frame->m_Ident != PCB_FRAME )
        flag = TRUE;

    frame->AppendMsgPanel( m_Reference->m_Text, m_Value->m_Text,
                           DARKCYAN );

    if( flag ) // Display last date the component was edited( useful in Module Editor)
    {
        time_t edit_time = m_LastEdit_Time;
        strcpy( Line, ctime( &edit_time ) );
        strtok( Line, " \n\r" );
        strcpy( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, " " );
        strcat( bufcar, strtok( NULL, " \n\r" ) ); strcat( bufcar, ", " );
        strtok( NULL, " \n\r" );
        strcat( bufcar, strtok( NULL, " \n\r" ) );
        msg = CONV_FROM_UTF8( bufcar );
        frame->AppendMsgPanel( _( "Last Change" ), msg, BROWN );
    }
    else    // display time stamp in schematic
    {
        msg.Printf( wxT( "%8.8lX" ), m_TimeStamp );
        frame->AppendMsgPanel( _( "Netlist path" ), m_Path, BROWN );
    }

    frame->AppendMsgPanel( _( "Layer" ), board->GetLayerName( m_Layer ), RED );

    EDA_BaseStruct* PtStruct = m_Pads;
    nbpad = 0;
    while( PtStruct )
    {
        nbpad++;
        PtStruct = PtStruct->Next();
    }

    msg.Printf( wxT( "%d" ), nbpad );
    frame->AppendMsgPanel( _( "Pads" ), msg, BLUE );

    msg = wxT( ".." );
    if( IsLocked() )
        msg[0] = 'L';
    if( m_ModuleStatus & MODULE_is_PLACED )
        msg[1] = 'P';
    frame->AppendMsgPanel( _( "Stat" ), msg, MAGENTA );

    msg.Printf( wxT( "%.1f" ), (float) m_Orient / 10 );
    frame->AppendMsgPanel( _( "Orient" ), msg, BROWN );

    frame->AppendMsgPanel( _( "Module" ), m_LibRef, BLUE );

    if(  m_3D_Drawings != NULL )
        msg = m_3D_Drawings->m_Shape3DName;
    else
        msg = _( "No 3D shape" );
    frame->AppendMsgPanel( _( "3D-Shape" ), msg, RED );

    wxString doc     = _( "Doc:  " ) + m_Doc;
    wxString keyword = _( "KeyW: " ) + m_KeyWord;
    frame->AppendMsgPanel( doc, keyword, BLACK );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool MODULE::HitTest( const wxPoint& refPos )
{
    /* Calculation of the cursor coordinate relative to  module */
    int spot_cX = refPos.x - m_Pos.x;
    int spot_cY = refPos.y - m_Pos.y;

    RotatePoint( &spot_cX, &spot_cY, -m_Orient );

    /* Check if cursor is in the rectangle. */
    if( m_BoundaryBox.Inside( spot_cX, spot_cY ) )
        return true;

    return false;
}


/**
 * Function HitTest (overlaid)
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

#if 0 && defined(DEBUG)
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


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void MODULE::Show( int nestLevel, std::ostream& os )
{
    BOARD* board = GetBoard();

    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " ref=\"" << m_Reference->m_Text.mb_str() << '"' <<
    " value=\"" << m_Value->m_Text.mb_str() << '"' <<
    " layer=\"" << board->GetLayerName( m_Layer ).mb_str() << '"' <<
    ">\n";

    NestedSpace( nestLevel + 1, os ) <<
    "<boundingBox" << m_BoundaryBox.m_Pos << m_BoundaryBox.m_Size << "/>\n";

    NestedSpace( nestLevel + 1, os ) << "<orientation tenths=\"" << m_Orient
                                     << "\"/>\n";

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

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str()
                                 << ">\n";
}


#endif
