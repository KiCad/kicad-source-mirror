/**********************************************************/
/*	lib_entry.cpp										  */
/**********************************************************/

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/*********************/
/* class LibCmpEntry */
/*********************/

/* Basic class for librarty oomponent description
 *  Not directly used
 *  Used to create the 2 derived classes :
 *      - EDA_LibCmpAliasStruct
 *      - EDA_LibComponentStruct
 */

/********************************************************************/
LibCmpEntry::LibCmpEntry( LibrEntryType CmpType, const wxChar* CmpName ) :
    EDA_BaseStruct( LIBCOMPONENT_STRUCT_TYPE )
/********************************************************************/
{
    Type = CmpType;
    m_Name.m_FieldId = VALUE;
    if( CmpName )
        m_Name.m_Text = CmpName;
}


/******************************/
LibCmpEntry::~LibCmpEntry()
/******************************/
{
}


/*******************************/
/* class EDA_LibCmpAliasStruct */
/*******************************/

/* Class to define an alias of a component
 *  An alias uses the component defintion (graphic, pins...)
 *  but has its own name and documentation.
 *  Therefore, when the component is modified, alias of this component are modified.
 *  This is a simple method to create components with differs very few
 *  (like 74LS00, 74HC00 ... and many op amps )
 */

EDA_LibCmpAliasStruct:: EDA_LibCmpAliasStruct( const wxChar* CmpName,
                                               const wxChar* CmpRootName ) :
    LibCmpEntry( ALIAS, CmpName )
{
    if( CmpRootName == NULL )
        m_RootName.Empty();
    else
        m_RootName = CmpRootName;
}


EDA_LibCmpAliasStruct::~EDA_LibCmpAliasStruct()
{
}


/********************************/
/* class EDA_LibComponentStruct */
/********************************/

/* This is a standard component  (in library)
 */
EDA_LibComponentStruct:: EDA_LibComponentStruct( const wxChar* CmpName ) :
    LibCmpEntry( ROOT, CmpName )
{
    m_Drawings   = NULL;
    m_LastDate   = 0;
    m_UnitCount  = 1;
    m_TextInside = 40;
    m_Options    = ENTRY_NORMAL;
    m_UnitSelectionLocked = FALSE;
    m_DrawPinNum = m_DrawPinName = 1;
    m_Prefix.m_FieldId = REFERENCE;
}


/******************************************************/
EDA_LibComponentStruct::~EDA_LibComponentStruct()
/******************************************************/
{
    LibEDA_BaseStruct* DrawItem, * NextDrawItem;

    /* suppression des elements dependants */
    DrawItem = m_Drawings; m_Drawings = NULL;
    while( DrawItem )
    {
        NextDrawItem = DrawItem->Next();
        SAFE_DELETE( DrawItem );
        DrawItem = NextDrawItem;
    }
}


/**********************************************************************/
EDA_Rect EDA_LibComponentStruct::GetBoundaryBox( int Unit, int Convert )
/**********************************************************************/

/* Return the componenty boundary box ( in user coordinates )
 *  The unit Unit, and the shape Convert are considered.
 *  If Unit == 0, Unit is not used
 *  if Convert == 0 Convert is non used
 **/
{
    int                xmin, xmax, ymin, ymax, x1, y1;
    int*               pt, ii;
    LibEDA_BaseStruct* DrawEntry;
    EDA_Rect           BoundaryBox;

    DrawEntry = m_Drawings;
    if( DrawEntry )
    {
        xmin = ymin = 0x7FFFFFFF;
        xmax = ymax = 0x80000000;
    }
    else
    {
        xmin = ymin = -50;
        xmax = ymax = 50;    // Min size in 1/1000 inch
    }

    for( ; DrawEntry != NULL; DrawEntry = DrawEntry->Next() )
    {
        if( DrawEntry->m_Unit > 0 )  // The item is non common to units
            if( (m_UnitCount > 1 ) && (Unit > 0) && (Unit != DrawEntry->m_Unit) )
                continue;
        if( DrawEntry->m_Convert > 0 )  //The item is not common to alls convert
            if( (Convert > 0) && (Convert != DrawEntry->m_Convert) )
                continue;

        switch( DrawEntry->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
        {
            // Arc is reduced to a line from m_Start to m_End.
            // TODO better.
            LibDrawArc* Arc = (LibDrawArc*) DrawEntry;
            x1   = Arc->m_ArcStart.x;
            y1   = Arc->m_ArcStart.y;
            xmin = MIN( xmin, x1 );
            ymin = MIN( ymin, y1 );
            xmax = MAX( xmax, x1 );
            ymax = MAX( ymax, y1 );
            x1   = Arc->m_ArcEnd.x;
            y1   = Arc->m_ArcEnd.y;
            xmin = MIN( xmin, x1 );
            ymin = MIN( ymin, y1 );
            xmax = MAX( xmax, x1 );
            ymax = MAX( ymax, y1 );
        }
        break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
        {
            LibDrawCircle* Circle = (LibDrawCircle*) DrawEntry;
            x1   = Circle->m_Pos.x - Circle->m_Rayon;
            y1   = Circle->m_Pos.y - Circle->m_Rayon;
            xmin = MIN( xmin, x1 );
            ymin = MIN( ymin, y1 );
            x1   = Circle->m_Pos.x + Circle->m_Rayon;
            y1   = Circle->m_Pos.y + Circle->m_Rayon;
            xmax = MAX( xmax, x1 );
            ymax = MAX( ymax, y1 );
        }
        break;

        case COMPONENT_RECT_DRAW_TYPE:
        {
            LibDrawSquare* Square = (LibDrawSquare*) DrawEntry;
            xmin = MIN( xmin, Square->m_Pos.x );
            xmin = MIN( xmin, Square->m_End.x );
            xmax = MAX( xmax, Square->m_Pos.x );
            xmax = MAX( xmax, Square->m_End.x );
            ymin = MIN( ymin, Square->m_Pos.y );
            ymin = MIN( ymin, Square->m_End.y );
            ymax = MAX( ymax, Square->m_Pos.y );
            ymax = MAX( ymax, Square->m_End.y );
        }
        break;

        case COMPONENT_PIN_DRAW_TYPE:
        {
            LibDrawPin* Pin = (LibDrawPin*) DrawEntry;
            x1   = Pin->m_Pos.x;
            y1   = Pin->m_Pos.y;
            xmin = MIN( xmin, x1 );
            xmax = MAX( xmax, x1 );
            ymin = MIN( ymin, y1 );
            ymax = MAX( ymax, y1 );
#if 0
            // 0 pour englober le point origine de la pin, 1 pour englober toute la pin
            switch( Pin->Orient )
            {
            case PIN_UP:
                y1 += Pin->Len; break;

            case PIN_DOWN:
                y1 -= Pin->Len; break;

            case PIN_LEFT:
                x1 -= Pin->Len; break;

            case PIN_RIGHT:
                x1 += Pin->Len; break;
            }

            xmin = MIN( xmin, x1 );
            xmax = MAX( xmax, x1 );
            ymin = MIN( ymin, y1 );
            ymax = MAX( ymax, y1 );
#endif
        }
        break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
        {
            LibDrawPolyline* polyline = (LibDrawPolyline*) DrawEntry;
            pt = polyline->m_PolyList;
            for( ii = 0; ii < polyline->m_CornersCount; ii++ )
            {
                if( xmin > *pt )
                    xmin = *pt;
                if( xmax < *pt )
                    xmax = *pt;
                pt++;
                if( ymin > *pt )
                    ymin = *pt;
                if( ymax < *pt )
                    ymax = *pt;
                pt++;
            }
        }
        break;

        default:
            ;
        }
    }

    // Update the BoundaryBox. Remember the fact the screen Y axis is the reverse */
    NEGATE(ymax); NEGATE(ymin);    // Y is not is screen axis sense
    // Ensure w and H > 0 (wxRect assume it)
    if( xmax < xmin )
        EXCHG( xmax, xmin );
    if( ymax < ymin )
        EXCHG( ymax, ymin );
    BoundaryBox.SetX( xmin ); BoundaryBox.SetWidth( xmax - xmin );
    BoundaryBox.SetY( ymin ); BoundaryBox.SetHeight( ymax - ymin );

    return BoundaryBox;
}


/** Function SetFields
 * initialize fields from a vector of fields
 * @param aFields a std::vector <LibDrawField> to import.
 */
void EDA_LibComponentStruct::SetFields( const std::vector <LibDrawField> aFields )
{
    // Init basic fields (Value = name in lib, and reference):
    aFields[VALUE].Copy( &m_Name );
    aFields[REFERENCE].Copy( &m_Prefix );

    // Remove others fields:
    CurrentLibEntry->m_Fields.DeleteAll();

    for( unsigned ii = FOOTPRINT; ii < aFields.size(); ii++ )
    {
            bool create = FALSE;
            if( !aFields[ii].m_Text.IsEmpty() )
                create = TRUE;
            if( !aFields[ii].m_Name.IsEmpty()
               && ( aFields[ii].m_Name != ReturnDefaultFieldName( ii ) ) )
                create = TRUE;
            if( create )
            {
                LibDrawField*Field = new LibDrawField( ii );
                aFields[ii].Copy( Field );
                CurrentLibEntry->m_Fields.PushBack( Field );
            }
    }

    /* for a user field (FieldId >= FIELD1), if a field value is void,
     *  fill it with "~" because for a library component a void field is not a very good idea
     *  (we do not see anything...) and in schematic this text is like a void text
     * and for non editable names, remove the name (set to the default name)
     */
    for( LibDrawField* Field = CurrentLibEntry->m_Fields; Field; Field = Field->Next() )
    {
        if( Field->m_FieldId >= FIELD1 )
        {
            if( Field->m_Text.IsEmpty() )
                Field->m_Text = wxT( "~" );
        }
        else
            Field->m_Name.Empty();
    }
}
