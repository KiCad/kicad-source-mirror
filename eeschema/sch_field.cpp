/***********************************************/
/* sch_field.cpp : handle the class SCH_FIELD  */
/***********************************************/

/* Fields are texts attached to a component, having a special meaning
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
#include "trigo.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_component.h"
#include "sch_field.h"
#include "template_fieldnames.h"


SCH_FIELD::SCH_FIELD( const wxPoint& aPos, int aFieldId,
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


SCH_FIELD::~SCH_FIELD()
{
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_FIELD::GetPenSize()
{
    int pensize = m_Thickness;

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
 * Draw schematic component fields.
 */
void SCH_FIELD::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                      const wxPoint& offset, int DrawMode, int Color )
{
    int            orient;
    EDA_Colors     color;
    wxPoint        textpos;
    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;
    int            LineWidth = m_Thickness;

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

    /* Calculate the text orientation, according to the component
     * orientation/mirror */
    orient = m_Orient;
    if( parentComponent->m_Transform.y1 )  // Rotate component 90 degrees.
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
    }

    /* Calculate the text justification, according to the component
     * orientation/mirror this is a bit complicated due to cumulative
     * calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications
     *      according to the text orientation.
     * - When a component is mirrored, the text is not mirrored and
     *   justifications are complicated to calculate
     * so the more easily way is to use no justifications ( Centered text )
     * and use GetBoundaryBox to know the text coordinate considered as centered
     */
    EDA_Rect BoundaryBox = GetBoundaryBox();
    GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
    GRTextVertJustifyType  vjustify = GR_TEXT_VJUSTIFY_CENTER;
    textpos = BoundaryBox.Centre();

    if( m_FieldId == REFERENCE )
        color = ReturnLayerColor( LAYER_REFERENCEPART );
    else if( m_FieldId == VALUE )
        color = ReturnLayerColor( LAYER_VALUEPART );
    else
        color = ReturnLayerColor( LAYER_FIELDS );

    if( !m_AddExtraText || ( m_FieldId != REFERENCE ) )
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
        fulltext << LIB_COMPONENT::ReturnSubReference( parentComponent->m_Multi );

        DrawGraphicText( panel, DC, textpos, color, fulltext,
                         orient,
                         m_Size, hjustify, vjustify,
                         LineWidth, m_Italic, m_Bold );
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

    /* Calculate the text position, according to the component
     * orientation/mirror */
    textpos  = m_Pos - parentComponent->m_Pos;
    textpos  = parentComponent->GetScreenCoord( textpos );
    textpos += parentComponent->m_Pos;
    x1 = textpos.x;
    y1 = textpos.y;
    int len = 10;
    GRLine( &panel->m_ClipBox, DC, x1 - len, y1, x1 + len, y1, 0, BLUE );
    GRLine( &panel->m_ClipBox, DC, x1, y1 - len, x1, y1 + len, 0, BLUE );
#endif
}


/**
 * Function ImportValues
 * copy parameters from a source.
 * Pointers and specific values (position, texts) are not copied
 * used to init a field from the model read from a lib entry
 * @param aSource = the LIB_FIELD to read
 */
void SCH_FIELD::ImportValues( const LIB_FIELD& aSource )
{
    m_Orient    = aSource.m_Orient;
    m_Size      = aSource.m_Size;
    m_HJustify  = aSource.m_HJustify;
    m_VJustify  = aSource.m_VJustify;
    m_Italic    = aSource.m_Italic;
    m_Bold      = aSource.m_Bold;
    m_Thickness     = aSource.m_Thickness;
    m_Attributs = aSource.m_Attributs;
    m_Mirror    = aSource.m_Mirror;
}


/**
 * Used if undo / redo command:
 *  swap data between this and copyitem
 */
void SCH_FIELD::SwapData( SCH_FIELD* copyitem )
{
    EXCHG( m_Text, copyitem->m_Text );
    EXCHG( m_Layer, copyitem->m_Layer );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_Thickness, copyitem->m_Thickness );
    EXCHG( m_Orient, copyitem->m_Orient );
    EXCHG( m_Mirror, copyitem->m_Mirror );
    EXCHG( m_Attributs, copyitem->m_Attributs );
    EXCHG( m_Italic, copyitem->m_Italic );
    EXCHG( m_Bold, copyitem->m_Bold );
    EXCHG( m_HJustify, copyitem->m_HJustify );
    EXCHG( m_VJustify, copyitem->m_VJustify );
}


/**
 * Function GetBoundaryBox
 * @return an EDA_Rect contains the real (user coordinates) boundary box for
 *         a text field,
 *  according to the component position, rotation, mirror ...
 */
EDA_Rect SCH_FIELD::GetBoundaryBox() const
{
    EDA_Rect       BoundaryBox;
    int            hjustify, vjustify;
    int            orient;
    wxSize         size;
    wxPoint        pos1, pos2;

    SCH_COMPONENT* parentComponent = (SCH_COMPONENT*) m_Parent;

    orient = m_Orient;
    wxPoint        pos = parentComponent->m_Pos;
    pos1 = m_Pos - pos;

    size.x   = LenSize( m_Text );
    size.y   = m_Size.y;
    hjustify = m_HJustify;
    vjustify = m_VJustify;

    pos2 = pos + parentComponent->m_Transform.TransformCoordinate( pos1 );

    /* Calculate the text orientation, according to the component
     * orientation/mirror */
    if( parentComponent->m_Transform.y1 )
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
    }

    /* Calculate the text justification, according to the component
     * orientation/mirror */
    if( parentComponent->m_Transform.y1 )
    {
        /* is it mirrored (for text justify)*/
        EXCHG( hjustify, vjustify );
        if( parentComponent->m_Transform.x2 < 0 )
            NEGATE( vjustify );
        if( parentComponent->m_Transform.y1 > 0 )
            NEGATE( hjustify );
    }
    else    /* component horizontal: is it mirrored (for text justify)*/
    {
        if( parentComponent->m_Transform.x1 < 0 )
            NEGATE( hjustify );
        if( parentComponent->m_Transform.y2 > 0 )
            NEGATE( vjustify );
    }

    if( orient == TEXT_ORIENT_VERT )
        EXCHG( size.x, size.y );

    switch( hjustify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        pos1.x = pos2.x - (size.x / 2);
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        pos1.x = pos2.x - size.x;
        break;

    default:
        pos1.x = pos2.x;
        break;
    }

    switch( vjustify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        pos1.y = pos2.y - (size.y / 2);
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        pos1.y = pos2.y - size.y;
        break;

    default:
        pos1.y = pos2.y;
        break;
    }

    BoundaryBox.SetOrigin( pos1 );
    BoundaryBox.SetSize( size );

    // Take thickness in account:
    int linewidth = ( m_Thickness == 0 ) ? g_DrawDefaultLineThickness : m_Thickness;
    BoundaryBox.Inflate( linewidth, linewidth );

    return BoundaryBox;
}


bool SCH_FIELD::Save( FILE* aFile ) const
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


void SCH_FIELD::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
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
    frame->OnModify();
    frame->SetCurrentField( NULL );
}


bool SCH_FIELD::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint * aFindLocation )
{
    bool match;
    if( aAuxData && m_FieldId == REFERENCE )
    {
        // reference is a special field because:
        // >> a part identifier is added in multi parts per package
        //      (the .m_AddExtraText of the field is set in this case )
        // >> In complex hierarchies, the actual reference depend on the sheet path.
        SCH_COMPONENT*  pSch     = (SCH_COMPONENT*) m_Parent;
        SCH_SHEET_PATH* sheet    = (SCH_SHEET_PATH*) aAuxData;
        wxString        fulltext = pSch->GetRef( sheet );
        if( m_AddExtraText )
        {
            /* For more than one part per package, we must add the part selection
             * A, B, ... or 1, 2, .. to the reference. */
            int part_id = pSch->GetUnitSelection( sheet );
            fulltext << LIB_COMPONENT::ReturnSubReference( part_id );
        }
        match = SCH_ITEM::Matches( fulltext, aSearchData );
    }

    else
        match = SCH_ITEM::Matches( m_Text, aSearchData );
    if( match )
    {
        EDA_Rect BoundaryBox = GetBoundaryBox();
        if( aFindLocation )
            *aFindLocation = GetBoundaryBox().Centre();
        return true;
    }
    return false;
}


void SCH_FIELD::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
}
