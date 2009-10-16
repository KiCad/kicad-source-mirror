/**************************************************************/
/* class_sch_cmp_field.cpp : handle the  class SCH_CMP_FIELD  */
/**************************************************************/

/* Fields are texts attached to a component, having a specuial meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "base_struct.h"
#include "gr_basic.h"
#include "drawtxt.h"
#include "macros.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"


SCH_CMP_FIELD::SCH_CMP_FIELD( const wxPoint& aPos, int aFieldId,
                              SCH_COMPONENT* aParent, wxString aName ) :
    SCH_ITEM( aParent, DRAW_PART_TEXT_STRUCT_TYPE ),
    EDA_TextStruct()
{
    m_Pos     = aPos;
    m_FieldId = aFieldId;
    m_AddExtraText = false;
    m_Attributs    = TEXT_NO_VISIBLE;
    m_Name = aName;

    SetLayer( LAYER_FIELDS );
}


SCH_CMP_FIELD::~SCH_CMP_FIELD()
{
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_CMP_FIELD::GetPenSize()
{
    int pensize = m_Width;

    if( pensize == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            pensize = GetPenSizeForBold( m_Size.x );
        else
            pensize = g_DrawDefaultLineThickness;
    }

    // Clip pen size for small texts:
    pensize = Clamp_Text_PenSize( pensize, m_Size, m_Bold );
    return pensize;
}


/**
 * Routine de trace des textes type Field du composant.
 *  entree:
 *      DrawMode: mode de trace
 */
void SCH_CMP_FIELD::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& offset, int DrawMode, int Color )
{
    int            orient;
    EDA_Colors     color;
    wxPoint        textpos;
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;
    int            LineWidth = m_Width;

    if( LineWidth == 0 )   // Use default values for pen size
    {
        if( m_Bold  )
            LineWidth = GetPenSizeForBold( m_Size.x );
        else
            LineWidth = g_DrawDefaultLineThickness;
    }


    // Clip pen size for small texts:
    LineWidth = Clamp_Text_PenSize( LineWidth, m_Size, m_Bold );

    if( ( m_Attributs & TEXT_NO_VISIBLE ) || IsVoid() )
        return;

    GRSetDrawMode( DC, DrawMode );

    /* Calculate the text orientation, according to the component orientation/mirror */
    orient = m_Orient;
    if( parentComponent->m_Transform[0][1] )  // Rotation du composant de 90deg
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
    }

    /* Calculate the text justification, according to the component orientation/mirror
     * this is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H vustifications
     *      according to the text orienation.
     * - When a component is mirrored, the text is not mirrored and justifications
     *      are complicated to calculate
     * so the more easily way is to use no justifications ( Centered text )
     * and use GetBoundaryBox to know the text coordinate considered as centered
    */
    EDA_Rect BoundaryBox = GetBoundaryBox();
    GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
    GRTextVertJustifyType vjustify = GR_TEXT_VJUSTIFY_CENTER;
    textpos = BoundaryBox.Centre();

    if( m_FieldId == REFERENCE )
        color = ReturnLayerColor( LAYER_REFERENCEPART );
    else if( m_FieldId == VALUE )
        color = ReturnLayerColor( LAYER_VALUEPART );
    else
        color = ReturnLayerColor( LAYER_FIELDS );

    if( !m_AddExtraText || (m_FieldId != REFERENCE) )
    {
        DrawGraphicText( panel, DC, textpos, color, m_Text,
                         orient,
                         m_Size, hjustify, vjustify,
                         LineWidth, m_Italic, m_Bold );
    }
    else
    {
        /* For more than one part per package, we must add the part selection
         * A, B, ... or 1, 2, .. to the reference. */
        wxString fulltext = m_Text;
        char     part_id;
#if defined(KICAD_GOST)
        fulltext.Append( '.' );
        part_id = '1' - 1 + parentComponent->m_Multi;
#else
        part_id = 'A' - 1 + parentComponent->m_Multi;
#endif
        fulltext.Append( part_id );

        DrawGraphicText( panel, DC, textpos, color, fulltext,
                         orient,
                         m_Size, hjustify, vjustify,
                         LineWidth, m_Italic,
                         m_Bold, false );
    }
    /* Enable this to draw the bounding box around the text field to validate
     * the bounding box calculations.
    */
#if 0
    // Draw boundary box:
    int x1 = BoundaryBox.GetX();
    int y1 = BoundaryBox.GetY();
    int x2 = BoundaryBox.GetRight();
    int y2 = BoundaryBox.GetBottom();
    GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
    // Draw the text anchor point
    /* Calculate the text position, according to the component orientation/mirror */
    textpos  = m_Pos - parentComponent->m_Pos;
    textpos  = parentComponent->GetScreenCoord( textpos );
    textpos += parentComponent->m_Pos;
    x1 = textpos.x;
    y1 = textpos.y;
    int len = 10;
    GRLine( &panel->m_ClipBox, DC, x1 - len, y1, x1 + len, y1, 0, BLUE );
    GRLine( &panel->m_ClipBox, DC, x1, y1 - len, x1, y1 +len, 0, BLUE );

#endif
}


/**
 * Function ImportValues
 * copy parameters from a source.
 * Pointers and specific values (position, texts) are not copied
 * used to init a field from the model read from a lib entry
 * @param aSource = the LIB_FIELD to read
 */
void SCH_CMP_FIELD::ImportValues( const LIB_FIELD& aSource )
{
    m_Orient    = aSource.m_Orient;
    m_Size      = aSource.m_Size;
    m_HJustify  = aSource.m_HJustify;
    m_VJustify  = aSource.m_VJustify;
    m_Italic    = aSource.m_Italic;
    m_Bold      = aSource.m_Bold;
    m_Width     = aSource.m_Width;
    m_Attributs = aSource.m_Attributs;
    m_Mirror    = aSource.m_Mirror;
}


/**
 * Used if undo / redo command:
 *  swap data between this and copyitem
 */
void SCH_CMP_FIELD::SwapData( SCH_CMP_FIELD* copyitem )
{
    EXCHG( m_Text, copyitem->m_Text );
    EXCHG( m_Layer, copyitem->m_Layer );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_Width, copyitem->m_Width );
    EXCHG( m_Orient, copyitem->m_Orient );
    EXCHG( m_Mirror, copyitem->m_Mirror );
    EXCHG( m_Attributs, copyitem->m_Attributs );
    EXCHG( m_Italic, copyitem->m_Italic );
    EXCHG( m_Bold, copyitem->m_Bold );
    EXCHG( m_HJustify, copyitem->m_HJustify );
    EXCHG( m_VJustify, copyitem->m_VJustify );
}


/**
 * return True if the field is void, i.e.:
 *  contains  "~" or ""
 */
bool SCH_CMP_FIELD::IsVoid()
{
    if( m_Text.IsEmpty() || m_Text == wxT( "~" ) )
        return true;
    return false;
}


/**
 * Function GetBoundaryBox()
 * @return an EDA_Rect contains the real (user coordinates) boundary box for
 *         a text field,
 *  according to the component position, rotation, mirror ...
 */
EDA_Rect SCH_CMP_FIELD::GetBoundaryBox() const
{
    EDA_Rect       BoundaryBox;
    int            hjustify, vjustify;
    int            orient;
    int            dx, dy, x1, y1, x2, y2;

    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;

    orient = m_Orient;
    wxPoint        pos = parentComponent->m_Pos;
    x1 = m_Pos.x - pos.x;
    y1 = m_Pos.y - pos.y;

    dx = LenSize( m_Text );
    dy = m_Size.y;
    hjustify = m_HJustify;
    vjustify = m_VJustify;

    x2 = pos.x + (parentComponent->m_Transform[0][0] *x1)
         + (parentComponent->m_Transform[0][1] *y1);
    y2 = pos.y + (parentComponent->m_Transform[1][0] *x1)
         + (parentComponent->m_Transform[1][1] *y1);

    /* Calculate the text orientation, according to the component orientation/mirror */
    if( parentComponent->m_Transform[0][1] )
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
    }

    /* Calculate the text justification, according to the component orientation/mirror */
    if( parentComponent->m_Transform[0][1] )
    {
        /* is it mirrored (for text justify)*/
        EXCHG( hjustify, vjustify );
        if( parentComponent->m_Transform[1][0] < 0 )
            NEGATE( vjustify );
        if( parentComponent->m_Transform[0][1] > 0 )
            NEGATE( hjustify );
    }
    else    /* component horizontal: is it mirrored (for text justify)*/
    {
        if( parentComponent->m_Transform[0][0] < 0 )
            NEGATE( hjustify );
        if( parentComponent->m_Transform[1][1] > 0 )
            NEGATE( vjustify );
    }

    if( orient == TEXT_ORIENT_VERT )
        EXCHG( dx, dy );

    switch( hjustify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        x1 = x2 - (dx / 2);
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        x1 = x2 - dx;
        break;

    default:
        x1 = x2;
        break;
    }

    switch( vjustify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        y1 = y2 - (dy / 2);
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        y1 = y2 - dy;
        break;

    default:
        y1 = y2;
        break;
    }

    BoundaryBox.SetX( x1 );
    BoundaryBox.SetY( y1 );
    BoundaryBox.SetWidth( dx );
    BoundaryBox.SetHeight( dy );

    // Take thickness in account:
    int linewidth = (m_Width == 0) ? g_DrawDefaultLineThickness : m_Width;
    BoundaryBox.Inflate( linewidth, linewidth );

    return BoundaryBox;
}


bool SCH_CMP_FIELD::Save( FILE* aFile ) const
{
    char hjustify = 'C';

    if( m_HJustify == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';

    char vjustify = 'C';
    if( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';

    if( fprintf( aFile, "F %d \"%s\" %c %-3d %-3d %-3d %4.4X %c %c%c%c",
                 m_FieldId,
                 CONV_TO_UTF8( m_Text ),
                 m_Orient == TEXT_ORIENT_HORIZ ? 'H' : 'V',
                 m_Pos.x, m_Pos.y,
                 m_Size.x,
                 m_Attributs,
                 hjustify, vjustify,
                 m_Italic ? 'I' : 'N',
                 m_Bold ? 'B' : 'N' ) == EOF )
    {
        return false;
    }

    // Save field name, if the name is user definable
    if( m_FieldId >= FIELD1 )
    {
        if( fprintf( aFile, " \"%s\"", CONV_TO_UTF8( m_Name ) ) == EOF )
        {
            return false;
        }
    }

    if( fprintf( aFile, "\n" ) == EOF )
    {
        return false;
    }

    return true;
}


void SCH_CMP_FIELD::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
{
    int            fieldNdx;
    LIB_COMPONENT* Entry;

    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;

    SCH_COMPONENT* component = (SCH_COMPONENT*) GetParent();

    // save old cmp in undo list
    if( g_ItemToUndoCopy && ( g_ItemToUndoCopy->Type() == component->Type() ) )
    {
        component->SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );
        frame->SaveCopyInUndoList( component, UR_CHANGED );
        component->SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );
    }

    fieldNdx = m_FieldId;
    m_AddExtraText = 0;
    if( fieldNdx == REFERENCE )
    {
        Entry = CMP_LIBRARY::FindLibraryComponent( component->m_ChipName );
        if( Entry != NULL )
        {
            if( Entry->GetPartCount() > 1 )
                m_AddExtraText = 1;
        }
    }

    Draw( frame->DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    m_Flags = 0;
    frame->GetScreen()->SetCurItem( NULL );
    frame->GetScreen()->SetModify();
    frame->SetCurrentField( NULL );
}
