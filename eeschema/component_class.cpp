/***********************************************************************/
/* Methodes de base de gestion des classes des elements de schematique */
/***********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "macros.h"
#include "id.h"

#include "protos.h"

#include "macros.h"


/***************************/
/* class DrawPartStruct	*/
/* class EDA_SchComponentStruct */
/***************************/

/***********************************************************************************/
DrawPartStruct::DrawPartStruct( KICAD_T struct_type, const wxPoint& pos ) :
    EDA_BaseStruct( struct_type )
/***********************************************************************************/
{
    m_Layer = 0;
    m_Pos = pos;
    m_TimeStamp = 0;
}


/************************************/
DrawPartStruct::~DrawPartStruct()
/************************************/
{
}


/****************************************************************/
const wxString& ReturnDefaultFieldName( int aFieldNdx )
/****************************************************************/

/* Return the default field name from its index (REFERENCE, VALUE ..)
 *  FieldDefaultNameList is not static, because we want the text translation
 *  for I18n
 */
{
    // avoid unnecessarily copying wxStrings at runtime.
    static const wxString FieldDefaultNameList[] = {
        _( "Ref" ),             /* Reference of part, i.e. "IC21" */
        _( "Value" ),           /* Value of part, i.e. "3.3K" */
        _( "Footprint" ),       /* Footprint, used by cvpcb or pcbnew, i.e. "16DIP300" */
        _( "Sheet" ),           /* for components which are a schematic file, schematic file name, i.e. "cnt16.sch" */
        wxString(_( "Field" ))+wxT("1"),
        wxString(_( "Field" ))+wxT("2"),
        wxString(_( "Field" ))+wxT("3"),
        wxString(_( "Field" ))+wxT("4"),
        wxString(_( "Field" ))+wxT("5"),
        wxString(_( "Field" ))+wxT("6"),
        wxString(_( "Field" ))+wxT("7"),
        wxString(_( "Field" ))+wxT("8"),
        wxT( "badFieldNdx!" )               // error, and "sentinel" value
    };
    
    if( (unsigned) aFieldNdx > FIELD8 )     // catches < 0 also
        aFieldNdx = FIELD8+1;               // return the sentinel text
    
    return FieldDefaultNameList[aFieldNdx];
}


/****************************************************************/
const wxString& EDA_SchComponentStruct::ReturnFieldName( int aFieldNdx ) const  
/****************************************************************/

/* Return the Field name from its index (REFERENCE, VALUE ..)
 */
{
    // avoid unnecessarily copying wxStrings.
    
    if( aFieldNdx < FIELD1  ||  m_Field[aFieldNdx].m_Name.IsEmpty() )
        return ReturnDefaultFieldName( aFieldNdx );
    
    return m_Field[aFieldNdx].m_Name;
}


const wxString& EDA_SchComponentStruct::GetFieldValue( int aFieldNdx ) const
{
   // avoid unnecessarily copying wxStrings.
   static const wxString myEmpty = wxEmptyString;
    
   if( (unsigned) aFieldNdx > FIELD8  ||  m_Field[aFieldNdx].m_Text.IsEmpty() )
        return myEmpty;
    
    return m_Field[aFieldNdx].m_Text;
}


/*******************************************************************/
EDA_SchComponentStruct::EDA_SchComponentStruct( const wxPoint& pos ) :
    DrawPartStruct( DRAW_LIB_ITEM_STRUCT_TYPE, pos )
/*******************************************************************/
{
    int ii;

    m_Multi = 0;    /* In multi unit chip - which unit to draw. */
    m_RefIdNumber      = 0;
    m_FlagControlMulti = 0;
    m_Convert = 0;  /* Gestion des mutiples representations (conversion De Morgan) */
    
    /* The rotation/mirror transformation matrix. pos normal*/
    m_Transform[0][0] = 1;
    m_Transform[0][1] = 0;
    m_Transform[1][0] = 0;
    m_Transform[1][1] = -1;

    /* initialisation des Fields */
    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_Field[ii].m_Pos     = m_Pos;
        m_Field[ii].m_Layer   = LAYER_FIELDS;
        m_Field[ii].m_FieldId = REFERENCE + ii;
        m_Field[ii].m_Parent  = this;
    }

    m_Field[VALUE].m_Layer     = LAYER_VALUEPART;
    m_Field[REFERENCE].m_Layer = LAYER_REFERENCEPART;

    m_PinIsDangling = NULL;
}


/**********************************************************************/
EDA_Rect EDA_SchComponentStruct::GetBoundaryBox()
/**********************************************************************/
{
    EDA_LibComponentStruct* Entry = FindLibPart( m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    EDA_Rect BoundaryBox;
    int      x0, xm, y0, ym;

    /* Get the basic Boundary box */
    if( Entry )
    {
        BoundaryBox = Entry->GetBoundaryBox( m_Multi, m_Convert );
        x0 = BoundaryBox.GetX(); xm = BoundaryBox.GetRight();

        // We must reverse Y values, because matrix orientation
        // suppose Y axis normal for the library items coordinates,
        // m_Transform reverse Y values, but BoundaryBox ais already reversed!
        y0 = -BoundaryBox.GetY();
        ym = -BoundaryBox.GetBottom();
    }
    else    /* if lib Entry not found, give a reasonable size */
    {
        x0 = y0 = -50;
        xm = ym = 50;
    }

    /* Compute the real Boundary box (rotated, mirrored ...)*/
    int x1 = m_Transform[0][0] * x0 + m_Transform[0][1] * y0;

    int y1 = m_Transform[1][0] * x0 + m_Transform[1][1] * y0;

    int x2 = m_Transform[0][0] * xm + m_Transform[0][1] * ym;

    int y2 = m_Transform[1][0] * xm + m_Transform[1][1] * ym;

    // H and W must be > 0 for wxRect:
    if( x2 < x1 )
        EXCHG( x2, x1 );
    if( y2 < y1 )
        EXCHG( y2, y1 );
    
    BoundaryBox.SetX( x1 ); BoundaryBox.SetY( y1 );
    BoundaryBox.SetWidth( x2 - x1 );
    BoundaryBox.SetHeight( y2 - y1 );

    BoundaryBox.Offset( m_Pos );
    return BoundaryBox;
}


/**************************************************************************/
void PartTextStruct::SwapData( PartTextStruct* copyitem )
/**************************************************************************/

/* Used if undo / redo command:
 *  swap data between this and copyitem
 */
{
    EXCHG( m_Text, copyitem->m_Text );
    EXCHG( m_Layer, copyitem->m_Layer );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_Width, copyitem->m_Width );
    EXCHG( m_Orient, copyitem->m_Orient );
    EXCHG( m_Miroir, copyitem->m_Miroir );
    EXCHG( m_Attributs, copyitem->m_Attributs );
    EXCHG( m_CharType, copyitem->m_CharType );
    EXCHG( m_HJustify, copyitem->m_HJustify );
    EXCHG( m_VJustify, copyitem->m_VJustify );
    EXCHG( m_ZoomLevelDrawable, copyitem->m_ZoomLevelDrawable );
    EXCHG( m_TextDrawings, copyitem->m_TextDrawings );
    EXCHG( m_TextDrawingsSize, copyitem->m_TextDrawingsSize );
}


/**************************************************************************/
void EDA_SchComponentStruct::SwapData( EDA_SchComponentStruct* copyitem )
/**************************************************************************/

/* Used if undo / redo command:
 *  swap data between this and copyitem
 */
{
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Multi, copyitem->m_Multi );
    EXCHG( m_Convert, copyitem->m_Convert );
    EXCHG( m_Transform[0][0], copyitem->m_Transform[0][0] );
    EXCHG( m_Transform[0][1], copyitem->m_Transform[0][1] );
    EXCHG( m_Transform[1][0], copyitem->m_Transform[1][0] );
    EXCHG( m_Transform[1][1], copyitem->m_Transform[1][1] );
    for( int ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_Field[ii].SwapData( &copyitem->m_Field[ii] );
    }
}


/***********************************************************************/
void EDA_SchComponentStruct::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/***********************************************************************/
{
    /* save old text in undo list */
    if( g_ItemToUndoCopy
       && ( g_ItemToUndoCopy->Type() == Type() )
       && ( (m_Flags & IS_NEW) == 0 ) )
    {
        /* restore old values and save new ones */
        SwapData( (EDA_SchComponentStruct*) g_ItemToUndoCopy );
        
        /* save in undo list */
        ( (WinEDA_SchematicFrame*) frame )->SaveCopyInUndoList( this, IS_CHANGED );
        
        /* restore new values */
        SwapData( (EDA_SchComponentStruct*) g_ItemToUndoCopy );
        
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = NULL;
    }

    EDA_BaseStruct::Place( frame, DC );
}


/***************************************************/
void EDA_SchComponentStruct::ClearAnnotation()
/***************************************************/

/* Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
 */
{
    m_RefIdNumber = 0;

    while( isdigit( m_Field[REFERENCE].m_Text.Last() ) )
        m_Field[REFERENCE].m_Text.RemoveLast();

    if( m_Field[REFERENCE].m_Text.Last() != '?' )
        m_Field[REFERENCE].m_Text.Append( '?' );

    EDA_LibComponentStruct* Entry;
    Entry = FindLibPart( m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( !Entry || !Entry->m_UnitSelectionLocked )
        m_Multi = 1;
}


/**************************************************************/
EDA_SchComponentStruct* EDA_SchComponentStruct::GenCopy()
/**************************************************************/
{
    EDA_SchComponentStruct* new_item = new EDA_SchComponentStruct( m_Pos );

    int ii;

    new_item->m_Multi    = m_Multi;
    new_item->m_ChipName = m_ChipName;
    new_item->m_FlagControlMulti = m_FlagControlMulti;
    new_item->m_Convert = m_Convert;
    new_item->m_Transform[0][0] = m_Transform[0][0];
    new_item->m_Transform[0][1] = m_Transform[0][1];
    new_item->m_Transform[1][0] = m_Transform[1][0];
    new_item->m_Transform[1][1] = m_Transform[1][1];
    new_item->m_TimeStamp = m_TimeStamp;


    /* initialisation des Fields */
    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_Field[ii].PartTextCopy( &new_item->m_Field[ii] );
    }

    return new_item;
}


/*****************************************************************/
void EDA_SchComponentStruct::SetRotationMiroir( int type_rotate )
/******************************************************************/

/* Compute the new matrix transform for a schematic component
 *  in order to have the requested transform (type_rotate = rot, mirror..)
 *  which is applied to the initial transform.
 */
{
    int  TempMat[2][2];
    bool Transform = FALSE;

    switch( type_rotate )
    {
    case CMP_ORIENT_0:
    case CMP_NORMAL:            /* Position Initiale */
        m_Transform[0][0] = 1;
        m_Transform[1][1] = -1;
        m_Transform[1][0] = m_Transform[0][1] = 0;
        break;

    case CMP_ROTATE_CLOCKWISE:            /* Rotate + */
        TempMat[0][0] = TempMat[1][1] = 0;
        TempMat[0][1] = 1;
        TempMat[1][0] = -1;
        Transform = TRUE;
        break;

    case CMP_ROTATE_COUNTERCLOCKWISE:             /* Rotate - */
        TempMat[0][0] = TempMat[1][1] = 0;
        TempMat[0][1] = -1;
        TempMat[1][0] = 1;
        Transform = TRUE;
        break;

    case CMP_MIROIR_Y:          /* MirrorY */
        TempMat[0][0] = -1;
        TempMat[1][1] = 1;
        TempMat[0][1] = TempMat[1][0] = 0;
        Transform = TRUE;
        break;

    case CMP_MIROIR_X:            /* MirrorX */
        TempMat[0][0] = 1;
        TempMat[1][1] = -1;
        TempMat[0][1] = TempMat[1][0] = 0;
        Transform = TRUE;
        break;

    case CMP_ORIENT_90:
        SetRotationMiroir( CMP_ORIENT_0 );
        SetRotationMiroir( CMP_ROTATE_COUNTERCLOCKWISE );
        break;

    case CMP_ORIENT_180:
        SetRotationMiroir( CMP_ORIENT_0 );
        SetRotationMiroir( CMP_ROTATE_COUNTERCLOCKWISE );
        SetRotationMiroir( CMP_ROTATE_COUNTERCLOCKWISE );
        break;

    case CMP_ORIENT_270:
        SetRotationMiroir( CMP_ORIENT_0 );
        SetRotationMiroir( CMP_ROTATE_CLOCKWISE );
        break;

    case (CMP_ORIENT_0 + CMP_MIROIR_X):
        SetRotationMiroir( CMP_ORIENT_0 );
        SetRotationMiroir( CMP_MIROIR_X );
        break;

    case (CMP_ORIENT_0 + CMP_MIROIR_Y):
        SetRotationMiroir( CMP_ORIENT_0 );
        SetRotationMiroir( CMP_MIROIR_Y );
        break;

    case (CMP_ORIENT_90 + CMP_MIROIR_X):
        SetRotationMiroir( CMP_ORIENT_90 );
        SetRotationMiroir( CMP_MIROIR_X );
        break;

    case (CMP_ORIENT_90 + CMP_MIROIR_Y):
        SetRotationMiroir( CMP_ORIENT_90 );
        SetRotationMiroir( CMP_MIROIR_Y );
        break;

    case (CMP_ORIENT_180 + CMP_MIROIR_X):
        SetRotationMiroir( CMP_ORIENT_180 );
        SetRotationMiroir( CMP_MIROIR_X );
        break;

    case (CMP_ORIENT_180 + CMP_MIROIR_Y):
        SetRotationMiroir( CMP_ORIENT_180 );
        SetRotationMiroir( CMP_MIROIR_Y );
        break;

    case (CMP_ORIENT_270 + CMP_MIROIR_X):
        SetRotationMiroir( CMP_ORIENT_270 );
        SetRotationMiroir( CMP_MIROIR_X );
        break;

    case (CMP_ORIENT_270 + CMP_MIROIR_Y):
        SetRotationMiroir( CMP_ORIENT_270 );
        SetRotationMiroir( CMP_MIROIR_Y );
        break;

    default:
        Transform = FALSE;
        DisplayError( NULL, wxT( "SetRotateMiroir() error: ill value" ) );
        break;
    }

    if( Transform )
    {/* The new matrix transform is the old matrix transform modified by the
      *  requested transformation, which is the TempMat transform (rot, mirror ..)
      *  in order to have (in term of matrix transform):
      *     transform coord = new_m_Transform * coord
      *  where transform coord is the coord modified by new_m_Transform from the initial
      *  value coord.
      *  new_m_Transform is computed (from old_m_Transform and TempMat) to have:
      *     transform coord = old_m_Transform * coord * TempMat
      */
        int NewMatrix[2][2];

        NewMatrix[0][0] = m_Transform[0][0] * TempMat[0][0] +
                          m_Transform[1][0] * TempMat[0][1];

        NewMatrix[0][1] = m_Transform[0][1] * TempMat[0][0] +
                          m_Transform[1][1] * TempMat[0][1];

        NewMatrix[1][0] = m_Transform[0][0] * TempMat[1][0] +
                          m_Transform[1][0] * TempMat[1][1];

        NewMatrix[1][1] = m_Transform[0][1] * TempMat[1][0] +
                          m_Transform[1][1] * TempMat[1][1];

        m_Transform[0][0] = NewMatrix[0][0];
        m_Transform[0][1] = NewMatrix[0][1];
        m_Transform[1][0] = NewMatrix[1][0];
        m_Transform[1][1] = NewMatrix[1][1];
    }
}


/****************************************************/
int EDA_SchComponentStruct::GetRotationMiroir()
/****************************************************/
{
    int  type_rotate = CMP_ORIENT_0;
    int  TempMat[2][2], MatNormal[2][2];
    int  ii;
    bool found = FALSE;

    memcpy( TempMat, m_Transform, sizeof(TempMat) );
    SetRotationMiroir( CMP_ORIENT_0 );
    memcpy( MatNormal, m_Transform, sizeof(MatNormal) );

    for( ii = 0; ii < 4; ii++ )
    {
        if( memcmp( TempMat, m_Transform, sizeof(MatNormal) ) == 0 )
        {
            found = TRUE; break;
        }
        SetRotationMiroir( CMP_ROTATE_COUNTERCLOCKWISE );
    }

    if( !found )
    {
        type_rotate = CMP_MIROIR_X + CMP_ORIENT_0;
        SetRotationMiroir( CMP_NORMAL );
        SetRotationMiroir( CMP_MIROIR_X );
        for( ii = 0; ii < 4; ii++ )
        {
            if( memcmp( TempMat, m_Transform, sizeof(MatNormal) ) == 0 )
            {
                found = TRUE; break;
            }
            SetRotationMiroir( CMP_ROTATE_COUNTERCLOCKWISE );
        }
    }

    if( !found )
    {
        type_rotate = CMP_MIROIR_Y + CMP_ORIENT_0;
        SetRotationMiroir( CMP_NORMAL );
        SetRotationMiroir( CMP_MIROIR_Y );
        for( ii = 0; ii < 4; ii++ )
        {
            if( memcmp( TempMat, m_Transform, sizeof(MatNormal) ) == 0 )
            {
                found = TRUE; break;
            }
            SetRotationMiroir( CMP_ROTATE_COUNTERCLOCKWISE );
        }
    }

    memcpy( m_Transform, TempMat, sizeof(m_Transform) );

    if( found )
    {
        return type_rotate + ii;
    }
    else
    {
        wxBell(); return CMP_NORMAL;
    }
}


/***********************************************************************/
wxPoint EDA_SchComponentStruct::GetScreenCoord( const wxPoint& coord )
/***********************************************************************/

/* Renvoie la coordonnée du point coord, en fonction de l'orientation
 *  du composant (rotation, miroir).
 *  Les coord sont toujours relatives à l'ancre (coord 0,0) du composant
 */
{
    wxPoint screenpos;

    screenpos.x = m_Transform[0][0] * coord.x + m_Transform[0][1] * coord.y;
    screenpos.y = m_Transform[1][0] * coord.x + m_Transform[1][1] * coord.y;
    return screenpos;
}



#if defined(DEBUG)
/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level 
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void EDA_SchComponentStruct::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
        " ref=\""           << GetReference().mb_str() << '"' <<
        " chipName=\""      << m_ChipName.mb_str() << '"' <<
        m_Pos <<
        " layer=\""         << m_Layer      << '"' <<
        "/>\n";

    // skip the reference, it's been output already.        
    for( int i=1;  i<NUMBER_OF_FIELDS;  ++i )
    {
        wxString value = GetFieldValue( i );
        
        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel+1, os ) << "<field" <<
                " name=\""  << ReturnFieldName(i).mb_str() << '"' <<
                " value=\"" << value.mb_str() << "\"/>\n";
        }
    }        
        
    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif




/***************************************************************************/
PartTextStruct::PartTextStruct( const wxPoint& pos, const wxString& text ) :
    EDA_BaseStruct( DRAW_PART_TEXT_STRUCT_TYPE ), EDA_TextStruct( text )
/***************************************************************************/
{
    m_Pos     = pos;
    m_FieldId = 0;
}


/************************************/
PartTextStruct::~PartTextStruct()
/************************************/
{
}


/***********************************************************/
void PartTextStruct::PartTextCopy( PartTextStruct* target )
/***********************************************************/
{
    target->m_Text = m_Text;
    if( m_FieldId >= FIELD1 )
        target->m_Name = m_Name;
    target->m_Layer     = m_Layer;
    target->m_Pos       = m_Pos;
    target->m_Size      = m_Size;
    target->m_Attributs = m_Attributs;
    target->m_FieldId   = m_FieldId;
    target->m_Orient    = m_Orient;
    target->m_HJustify  = m_HJustify;
    target->m_VJustify  = m_VJustify;
    target->m_Flags     = m_Flags;
}


/*********************************/
bool PartTextStruct::IsVoid()
/*********************************/

/* return True if The field is void, i.e.:
 *  contains wxEmptyString or "~"
 */
{
    if( m_Text.IsEmpty() || m_Text == wxT( "~" ) )
        return TRUE;
    return FALSE;
}


/********************************************/
EDA_Rect PartTextStruct::GetBoundaryBox()
/********************************************/

/* return
 *  EDA_Rect contains the real (user coordinates) boundary box for a text field,
 *  according to the component position, rotation, mirror ...
 * 
 */
{
    EDA_Rect BoundaryBox;
    int      hjustify, vjustify;
    int      textlen;
    int      orient;
    int      dx, dy, x1, y1, x2, y2;

    EDA_SchComponentStruct* DrawLibItem = (EDA_SchComponentStruct*) m_Parent;

    orient = m_Orient;
    wxPoint pos = DrawLibItem->m_Pos;
    x1 = m_Pos.x - pos.x;
    y1 = m_Pos.y - pos.y;

    textlen = GetLength();
    if( m_FieldId == REFERENCE )   // Real Text can be U1 or U1A
    {
        EDA_LibComponentStruct* Entry =
            FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry && (Entry->m_UnitCount > 1) )
            textlen++; // because U1 is show as U1A or U1B ...
    }
    dx = m_Size.x * textlen;

    // Real X Size is 10/9 char size because space between 2 chars is 1/10 X Size
    dx = (dx * 10) / 9;

    dy = m_Size.y;
    hjustify = m_HJustify;
    vjustify = m_VJustify;

    x2 = pos.x + (DrawLibItem->m_Transform[0][0] * x1)
         + (DrawLibItem->m_Transform[0][1] * y1);
    y2 = pos.y + (DrawLibItem->m_Transform[1][0] * x1)
         + (DrawLibItem->m_Transform[1][1] * y1);

    /* If the component orientation is +/- 90 deg, the text orienation must be changed */
    if( DrawLibItem->m_Transform[0][1] )
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
        /* is it mirrored (for text justify)*/
        EXCHG( hjustify, vjustify );
        if( DrawLibItem->m_Transform[1][0] < 0 )
            vjustify = -vjustify;
        if( DrawLibItem->m_Transform[0][1] > 0 )
            hjustify = -hjustify;
    }
    else    /* component horizontal: is it mirrored (for text justify)*/
    {
        if( DrawLibItem->m_Transform[0][0] < 0 )
            hjustify = -hjustify;
        if( DrawLibItem->m_Transform[1][1] > 0 )
            vjustify = -vjustify;
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

    return BoundaryBox;
}
