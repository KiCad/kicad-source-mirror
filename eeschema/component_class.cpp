/***********************************************************************/
/* component_class.cpp : handle the  class SCH_COMPONENT  */
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

#include <wx/arrimpl.cpp>
#include <wx/tokenzr.h>

WX_DEFINE_OBJARRAY( ArrayOfSheetLists );

/***************************/
/* class SCH_COMPONENT */
/***************************/

/** Function AddHierarchicalReference
 * Add a full hierachical reference (path + local reference)
 * @param path = hierarchical path (/<sheet timestamp>/component timestamp> like /05678E50/A23EF560)
 * @param ref = local reference like C45, R56
 */
void SCH_COMPONENT::AddHierarchicalReference( const wxString& path, const wxString& ref )
{

    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii<m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( path ) == 0 )
        {
            m_PathsAndReferences.RemoveAt(ii);
            ii --;
        }
    }

    h_ref = path + wxT( " " ) + ref;

    h_ref << wxT( " " ) << m_Multi;
    m_PathsAndReferences.Add( h_ref );
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
        _( "Ref" ),                             /* Reference of part, i.e. "IC21" */
        _( "Value" ),                           /* Value of part, i.e. "3.3K" */
        _( "Footprint" ),                       /* Footprint, used by cvpcb or pcbnew, i.e. "16DIP300" */
        _( "Sheet" ),                           /* for components which are a schematic file, schematic file name, i.e. "cnt16.sch" */
        wxString( _( "Field" ) ) + wxT( "1" ),
        wxString( _( "Field" ) ) + wxT( "2" ),
        wxString( _( "Field" ) ) + wxT( "3" ),
        wxString( _( "Field" ) ) + wxT( "4" ),
        wxString( _( "Field" ) ) + wxT( "5" ),
        wxString( _( "Field" ) ) + wxT( "6" ),
        wxString( _( "Field" ) ) + wxT( "7" ),
        wxString( _( "Field" ) ) + wxT( "8" ),
        wxT( "badFieldNdx!" )               // error, and "sentinel" value
    };

    if( (unsigned) aFieldNdx > FIELD8 )     // catches < 0 also
        aFieldNdx = FIELD8 + 1;             // return the sentinel text

    return FieldDefaultNameList[aFieldNdx];
}


/****************************************************************/
const wxString& SCH_COMPONENT::ReturnFieldName( int aFieldNdx ) const
/****************************************************************/

/* Return the Field name from its index (REFERENCE, VALUE ..)
 */
{
    // avoid unnecessarily copying wxStrings.

    if( aFieldNdx < FIELD1  ||  m_Field[aFieldNdx].m_Name.IsEmpty() )
        return ReturnDefaultFieldName( aFieldNdx );

    return m_Field[aFieldNdx].m_Name;
}


/****************************************************************/
wxString SCH_COMPONENT::GetPath( DrawSheetPath* sheet )
/****************************************************************/
{
    wxString str;

    str.Printf( wxT( "%8.8lX" ), m_TimeStamp );
    return sheet->Path() + str;
}


/********************************************************************/
const wxString SCH_COMPONENT::GetRef( DrawSheetPath* sheet )
/********************************************************************/
{
    wxString          path = GetPath( sheet );
    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    for( unsigned ii = 0; ii<m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( path ) == 0 )
        {
            h_ref = tokenizer.GetNextToken();

            //printf("GetRef hpath: %s\n",CONV_TO_UTF8(m_PathsAndReferences[ii]));
            return h_ref;
        }
    }

    //if it was not found in m_Paths array, then see if it is in
    // m_Field[REFERENCE] -- if so, use this as a default for this path.
    // this will happen if we load a version 1 schematic file.
    // it will also mean that multiple instances of the same sheet by default
    // all have the same component references, but perhaps this is best.
    if( !m_Field[REFERENCE].m_Text.IsEmpty() )
    {
        SetRef( sheet, m_Field[REFERENCE].m_Text );
        return m_Field[REFERENCE].m_Text;
    }
    return m_PrefixString;
}


/***********************************************************************/
void SCH_COMPONENT::SetRef( DrawSheetPath* sheet, const wxString& ref )
/***********************************************************************/
{
    //check to see if it is already there before inserting it
    wxString          path = GetPath( sheet );

    // printf( "SetRef path: %s ref: %s\n", CONV_TO_UTF8( path ), CONV_TO_UTF8( ref ) ); // Debug
    bool              notInArray = true;

    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    for( unsigned ii = 0; ii<m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( path ) == 0 )
        {
            //just update the reference text, not the timestamp.
            h_ref  = h_path + wxT( " " ) + ref;
            h_ref += wxT( " " );
            tokenizer.GetNextToken();               // Skip old reference
            h_ref += tokenizer.GetNextToken();      // Add part selection
            // Ann the part selection
            m_PathsAndReferences[ii] = h_ref;
            notInArray = false;
        }
    }

    if( notInArray )
        AddHierarchicalReference( path, ref );

    if( m_Field[REFERENCE].m_Text.IsEmpty()
       || ( abs( m_Field[REFERENCE].m_Pos.x - m_Pos.x ) +
            abs( m_Field[REFERENCE].m_Pos.y - m_Pos.y ) > 10000) )
    {
        //move it to a reasonable position..
        m_Field[REFERENCE].m_Pos    = m_Pos;
        m_Field[REFERENCE].m_Pos.x += 50; //a slight offset..
        m_Field[REFERENCE].m_Pos.y += 50;
    }
    m_Field[REFERENCE].m_Text = ref; //for drawing.
}


/******************************************************************/
const wxString& SCH_COMPONENT::GetFieldValue( int aFieldNdx ) const
/******************************************************************/
{
    // avoid unnecessarily copying wxStrings.
    static const wxString myEmpty = wxEmptyString;

    if( (unsigned) aFieldNdx > FIELD8  ||  m_Field[aFieldNdx].m_Text.IsEmpty() )
        return myEmpty;

    return m_Field[aFieldNdx].m_Text;
}


/*******************************************************************/
SCH_COMPONENT::SCH_COMPONENT( const wxPoint& aPos ) :
    SCH_ITEM( NULL, TYPE_SCH_COMPONENT )
/*******************************************************************/
{
    int ii;

    m_Multi = 0;    /* In multi unit chip - which unit to draw. */

    m_Pos = aPos;

    //m_FlagControlMulti = 0;
    m_UsedOnSheets.Clear();
    m_Convert = 0;  /* Gestion des mutiples representations (conversion De Morgan) */

    /* The rotation/mirror transformation matrix. pos normal*/
    m_Transform[0][0] = 1;
    m_Transform[0][1] = 0;
    m_Transform[1][0] = 0;
    m_Transform[1][1] = -1;

    /* initialisation des Fields */
    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_Field[ii].m_Pos = m_Pos;
        m_Field[ii].SetLayer( LAYER_FIELDS );
        m_Field[ii].m_FieldId = REFERENCE + ii;
        m_Field[ii].m_Parent  = this;
    }

    m_Field[VALUE].SetLayer( LAYER_VALUEPART );
    m_Field[REFERENCE].SetLayer( LAYER_REFERENCEPART );

    m_PrefixString = wxString( _( "U" ) );
}


/************************************************/
EDA_Rect SCH_COMPONENT::GetBoundaryBox() const
/************************************************/
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
        // m_Transform reverse Y values, but BoundaryBox is already reversed!
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
void SCH_COMPONENT::SwapData( SCH_COMPONENT* copyitem )
/**************************************************************************/

/* Used if undo / redo command:
 *  swap data between this and copyitem
 */
{
    EXCHG( m_ChipName, copyitem->m_ChipName );
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
void SCH_COMPONENT::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/***********************************************************************/
{
    /* save old text in undo list */
    if( g_ItemToUndoCopy
       && ( g_ItemToUndoCopy->Type() == Type() )
       && ( (m_Flags & IS_NEW) == 0 ) )
    {
        /* restore old values and save new ones */
        SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );

        /* save in undo list */
        ( (WinEDA_SchematicFrame*) frame )->SaveCopyInUndoList( this, IS_CHANGED );

        /* restore new values */
        SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );

        SAFE_DELETE( g_ItemToUndoCopy );
    }

    SCH_ITEM::Place( frame, DC );
}


/***************************************************/
void SCH_COMPONENT::ClearAnnotation()
/***************************************************/

/* Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
 */
{
    wxString defRef    = m_PrefixString;
    bool     KeepMulti = false;
    EDA_LibComponentStruct* Entry;

    Entry = FindLibPart( m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( Entry && Entry->m_UnitSelectionLocked )
        KeepMulti = true;

    while( defRef.Last() == '?' )
        defRef.RemoveLast();

    defRef.Append( wxT( "?" ) );

    wxString multi = wxT( "1" );
    wxString NewHref;
    for( unsigned int ii = 0; ii< m_PathsAndReferences.GetCount(); ii++ )
    {
        if( KeepMulti )  // Get and keep part selection
            multi = m_PathsAndReferences[ii].AfterLast( wxChar( ' ' ) );
        NewHref = m_PathsAndReferences[ii].BeforeFirst( wxChar( ' ' ) );
        NewHref << wxT( " " ) << defRef << wxT( " " ) << multi;
        m_PathsAndReferences[ii] = NewHref;
    }

    m_Field[REFERENCE].m_Text = defRef; //for drawing.

    if( !KeepMulti )
        m_Multi = 1;
}


/**************************************************************/
SCH_COMPONENT* SCH_COMPONENT::GenCopy()
/**************************************************************/
{
    SCH_COMPONENT* new_item = new SCH_COMPONENT( m_Pos );

    int            ii;

    new_item->m_Multi        = m_Multi;
    new_item->m_ChipName     = m_ChipName;
    new_item->m_PrefixString = m_PrefixString;

    //new_item->m_FlagControlMulti = m_FlagControlMulti;
    new_item->m_UsedOnSheets = m_UsedOnSheets;
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
void SCH_COMPONENT::SetRotationMiroir( int type_rotate )
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
    {
        /* The new matrix transform is the old matrix transform modified by the
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
int SCH_COMPONENT::GetRotationMiroir()
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
wxPoint SCH_COMPONENT::GetScreenCoord( const wxPoint& coord )
/***********************************************************************/

/* Renvoie la coordonn�e du point coord, en fonction de l'orientation
 *  du composant (rotation, miroir).
 *  Les coord sont toujours relatives a l'ancre (coord 0,0) du composant
 */
{
    wxPoint screenpos;

    screenpos.x = m_Transform[0][0] * coord.x + m_Transform[0][1] * coord.y;
    screenpos.y = m_Transform[1][0] * coord.x + m_Transform[1][1] * coord.y;
    return screenpos;
}


#if defined (DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void SCH_COMPONENT::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " ref=\"" << ReturnFieldName( 0 ) << '"' <<
    " chipName=\"" << m_ChipName.mb_str() << '"' <<
    m_Pos <<
    " layer=\"" << m_Layer << '"' <<
    "/>\n";

    // skip the reference, it's been output already.
    for( int i = 1;  i<NUMBER_OF_FIELDS;  ++i )
    {
        wxString value = GetFieldValue( i );

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" <<
            " name=\"" << ReturnFieldName( i ).mb_str() << '"' <<
            " value=\"" << value.mb_str() << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}


#endif


/***************************************************************************/
PartTextStruct::PartTextStruct( const wxPoint& pos, const wxString& text ) :
    SCH_ITEM( NULL, DRAW_PART_TEXT_STRUCT_TYPE ),
    EDA_TextStruct( text )
/***************************************************************************/
{
    m_Pos          = pos;
    m_FieldId      = 0;
    m_AddExtraText = false;
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
EDA_Rect PartTextStruct::GetBoundaryBox() const
/********************************************/

/* return
 *  EDA_Rect contains the real (user coordinates) boundary box for a text field,
 *  according to the component position, rotation, mirror ...
 *
 */
{
    EDA_Rect       BoundaryBox;
    int            hjustify, vjustify;
    int            textlen;
    int            orient;
    int            dx, dy, x1, y1, x2, y2;

    SCH_COMPONENT* DrawLibItem = (SCH_COMPONENT*) m_Parent;

    orient = m_Orient;
    wxPoint        pos = DrawLibItem->m_Pos;
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


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool PartTextStruct::Save( FILE* aFile ) const
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
    if( fprintf( aFile, "F %d \"%s\" %c %-3d %-3d %-3d %4.4X %c %c", m_FieldId,
            CONV_TO_UTF8( m_Text ),
            m_Orient == TEXT_ORIENT_HORIZ ? 'H' : 'V',
            m_Pos.x, m_Pos.y,
            m_Size.x,
            m_Attributs,
            hjustify, vjustify ) == EOF )
    {
        return false;
    }


    // Save field name, if necessary
    if( m_FieldId >= FIELD1 && !m_Name.IsEmpty() )
    {
        wxString fieldname = ReturnDefaultFieldName( m_FieldId );
        if( fieldname != m_Name )
        {
            if( fprintf( aFile, " \"%s\"", CONV_TO_UTF8( m_Name ) ) == EOF )
            {
                return false;
            }
        }
    }

    if( fprintf( aFile, "\n" ) == EOF )
    {
        return false;
    }

    return true;
}


/****************************************/
bool SCH_COMPONENT::Save( FILE* f ) const
/****************************************/

/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
{
    int             ii, Success = true;
    char            Name1[256], Name2[256];
    wxArrayString   reference_fields;
    static wxString delimiters( wxT( " " ) );

    //this is redundant with the AR entries below, but it makes the
    //files backwards-compatible.
    if( m_PathsAndReferences.GetCount() > 0 )
    {
        reference_fields = wxStringTokenize( m_PathsAndReferences[0], delimiters );
        strncpy( Name1, CONV_TO_UTF8( reference_fields[1] ), sizeof(Name1) );
    }
    else
    {
        if( m_Field[REFERENCE].m_Text.IsEmpty() )
            strncpy( Name1, CONV_TO_UTF8( m_PrefixString ), sizeof(Name1) );
        else
            strncpy( Name1, CONV_TO_UTF8( m_Field[REFERENCE].m_Text ), sizeof(Name1) );
    }
    for( ii = 0; ii < (int) strlen( Name1 ); ii++ )
    {
        if( Name1[ii] <= ' ' )
            Name1[ii] = '~';
    }

    if( !m_ChipName.IsEmpty() )
    {
        strncpy( Name2, CONV_TO_UTF8( m_ChipName ), sizeof(Name2) );
        for( ii = 0; ii < (int) strlen( Name2 ); ii++ )
            if( Name2[ii] <= ' ' )
                Name2[ii] = '~';
    }
    else
        strncpy( Name2, NULL_STRING, sizeof(Name2) );

    fprintf( f, "$Comp\n" );

    if( fprintf( f, "L %s %s\n", Name2, Name1 ) == EOF )
    {
        Success = false;
        return Success;
    }

    /* Generation de numero d'unit, convert et Time Stamp*/
    if( fprintf( f, "U %d %d %8.8lX\n", m_Multi, m_Convert, m_TimeStamp ) == EOF )
    {
        Success = false;
        return Success;
    }

    /* Save the position */
    if( fprintf( f, "P %d %d\n", m_Pos.x, m_Pos.y ) == EOF )
    {
        Success = false;
        return Success;
    }

    /* If this is a complex hierarchy; save hierarchical references.
      * but for simple hierarchies it is not necessary.
      * the reference inf is already saved
      * this is usefull for old eeschema version compatibility
     */
    if( m_PathsAndReferences.GetCount() > 1 )
    {
        for( unsigned int ii = 0; ii< m_PathsAndReferences.GetCount(); ii++ )
        {
            /*format:
             * AR Path="/140/2" Ref="C99"   Part="1"
             * where 140 is the uid of the containing sheet
             * and 2 is the timestamp of this component.
             * (timestamps are actually 8 hex chars)
             * Ref is the conventional component reference for this 'path'
             * Part is the conventional component part selection for this 'path'
             */
            reference_fields = wxStringTokenize( m_PathsAndReferences[ii], delimiters );
            if( fprintf( f, "AR Path=\"%s\" Ref=\"%s\"  Part=\"%s\" \n",
                    CONV_TO_UTF8( reference_fields[0] ),
                    CONV_TO_UTF8( reference_fields[1] ),
                    CONV_TO_UTF8( reference_fields[2] )
                    ) == EOF )
            {
                Success = false;
                return Success;
            }
        }
    }

    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        const PartTextStruct* field = &m_Field[ii];
        if( field->m_Text.IsEmpty() )
            continue;
        if( !field->Save( f ) )
        {
            Success = false; break;
        }
    }

    if( !Success )
        return Success;

    /* Generation du num unit, position, box ( ancienne norme )*/
    if( fprintf( f, "\t%-4d %-4d %-4d\n", m_Multi, m_Pos.x, m_Pos.y ) == EOF )
    {
        Success = false;
        return Success;
    }

    if( fprintf( f, "\t%-4d %-4d %-4d %-4d\n",
            m_Transform[0][0],
            m_Transform[0][1],
            m_Transform[1][0],
            m_Transform[1][1] ) == EOF )
    {
        Success = false;
        return Success;
    }

    fprintf( f, "$EndComp\n" );
    return Success;
}


EDA_Rect SCH_COMPONENT::GetBoundingBox()
{
    const int PADDING = 40;

    // This gives a reasonable approximation (but some things are missing so...
    EDA_Rect  ret = GetBoundaryBox();

    // Include BoundingBoxes of fields
    for( int i = REFERENCE; i < NUMBER_OF_FIELDS; i++ )
    {
        ret.Merge( m_Field[i].GetBoundaryBox() );
    }

    // ... add padding
    ret.Inflate( PADDING, PADDING );

    return ret;
}
