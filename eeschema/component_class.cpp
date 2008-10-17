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

#include "protos.h"

#include <wx/arrimpl.cpp>
#include <wx/tokenzr.h>

WX_DEFINE_OBJARRAY( ArrayOfSheetLists );

/***************************/
/* class SCH_COMPONENT */
/***************************/


/*******************************************************************/
SCH_COMPONENT::SCH_COMPONENT( const wxPoint& aPos, SCH_ITEM* aParent ) :
    SCH_ITEM( aParent, TYPE_SCH_COMPONENT )
/*******************************************************************/
{
    m_Multi = 0;    /* In multi unit chip - which unit to draw. */

    m_Pos = aPos;

    m_Convert = 0;  /* De Morgan Handling  */

    /* The rotation/mirror transformation matrix. pos normal */
    m_Transform[0][0] = 1;
    m_Transform[0][1] = 0;
    m_Transform[1][0] = 0;
    m_Transform[1][1] = -1;

    m_Fields.reserve( NUMBER_OF_FIELDS );

    for( int i=0;  i<NUMBER_OF_FIELDS;  ++i )
    {
        SCH_CMP_FIELD field( aPos, i, this, ReturnDefaultFieldName(i) );

        if( i==REFERENCE )
            field.SetLayer( LAYER_REFERENCEPART );
        else if( i==VALUE )
            field.SetLayer( LAYER_VALUEPART );
        // else keep LAYER_FIELDS from SCH_CMP_FIELD constructor

        // SCH_CMP_FIELD's implicitly created copy constructor is called in here
        AddField( field );
    }

    m_PrefixString = wxString( _( "U" ) );
}


/**
 * Function AddHierarchicalReference
 * adds a full hierachical reference (path + local reference)
 * @param aPath = hierarchical path (/<sheet timestamp>/component timestamp> like /05678E50/A23EF560)
 * @param aRef = local reference like C45, R56
 * @param aMulti = part selection, used in multi part per package (0 or 1 for non multi)
 */
void SCH_COMPONENT::AddHierarchicalReference( const wxString& aPath,
                                              const wxString& aRef,
                                              int             aMulti )
{
    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii<m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( aPath ) == 0 )
        {
            m_PathsAndReferences.RemoveAt( ii );
            ii--;
        }
    }

    h_ref = aPath + wxT( " " ) + aRef;

    h_ref << wxT( " " ) << aMulti;
    m_PathsAndReferences.Add( h_ref );
}


/****************************************************************/
wxString ReturnDefaultFieldName( int aFieldNdx )
/****************************************************************/

/* Return the default field name from its index (REFERENCE, VALUE ..)
 *  FieldDefaultNameList is not static, because we want the text translation
 *  for I18n
 */
{
    // avoid unnecessarily copying wxStrings at runtime.
    static const wxString defaults[] = {
        _( "Ref" ),             // Reference of part, i.e. "IC21"
        _( "Value" ),           // Value of part, i.e. "3.3K"
        _( "Footprint" ),       // Footprint, used by cvpcb or pcbnew, i.e. "16DIP300"
        _( "Datasheet" ),
    };

    if( (unsigned) aFieldNdx <= DATASHEET )
        return defaults[ aFieldNdx ];

    else
    {
        wxString ret = _("Field");
        ret << ( aFieldNdx - FIELD1 + 1);
        return ret;
    }
}


/****************************************************************/
wxString SCH_COMPONENT::ReturnFieldName( int aFieldNdx ) const
/****************************************************************/
{
    SCH_CMP_FIELD* field = GetField( aFieldNdx );

    if( field )
    {
        if( !field->m_Name.IsEmpty() )
            return field->m_Name;
        else
            return ReturnDefaultFieldName( aFieldNdx );
    }

    return wxEmptyString;
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

    // if it was not found in m_Paths array, then see if it is in
    // m_Field[REFERENCE] -- if so, use this as a default for this path.
    // this will happen if we load a version 1 schematic file.
    // it will also mean that multiple instances of the same sheet by default
    // all have the same component references, but perhaps this is best.
    if( !GetField(REFERENCE)->m_Text.IsEmpty() )
    {
        SetRef( sheet, GetField(REFERENCE)->m_Text );
        return GetField(REFERENCE)->m_Text;
    }
    return m_PrefixString;
}


/***********************************************************************/
void SCH_COMPONENT::SetRef( DrawSheetPath* sheet, const wxString& ref )
/***********************************************************************/
{
    wxString          path = GetPath( sheet );

    bool              notInArray = true;

    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    //check to see if it is already there before inserting it
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
        AddHierarchicalReference( path, ref, m_Multi );

    SCH_CMP_FIELD* rf = GetField( REFERENCE );

    if( rf->m_Text.IsEmpty()
       || ( abs( rf->m_Pos.x - m_Pos.x ) +
            abs( rf->m_Pos.y - m_Pos.y ) > 10000) )
    {
        // move it to a reasonable position
        rf->m_Pos    = m_Pos;
        rf->m_Pos.x += 50;         // a slight offset
        rf->m_Pos.y += 50;
    }

    rf->m_Text = ref;  // for drawing.
}


/***********************************************************/
int SCH_COMPONENT::GetUnitSelection( DrawSheetPath* aSheet )
/***********************************************************/

//returns the unit selection, for the given sheet path.
{
    wxString          path = GetPath( aSheet );
    wxString          h_path, h_multi;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    for( unsigned ii = 0; ii<m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( path ) == 0 )
        {
            tokenizer.GetNextToken();   // Skip reference
            h_multi = tokenizer.GetNextToken();
            long imulti = 1;
            h_multi.ToLong( &imulti );
            return imulti;
        }
    }

    //if it was not found in m_Paths array, then use m_Multi.
    // this will happen if we load a version 1 schematic file.
    return m_Multi;
}


/********************************************************************************/
void SCH_COMPONENT::SetUnitSelection( DrawSheetPath* aSheet, int aUnitSelection )
/********************************************************************************/

//Set the unit selection, for the given sheet path.
{
    wxString          path = GetPath( aSheet );

    bool              notInArray = true;

    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    //check to see if it is already there before inserting it
    for( unsigned ii = 0; ii<m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( path ) == 0 )
        {
            //just update the unit selection.
            h_ref  = h_path + wxT( " " );
            h_ref += tokenizer.GetNextToken();      // Add reference
            h_ref += wxT( " " );
            h_ref << aUnitSelection;                // Add part selection
            // Ann the part selection
            m_PathsAndReferences[ii] = h_ref;
            notInArray = false;
        }
    }

    if( notInArray )
        AddHierarchicalReference( path, m_PrefixString, aUnitSelection );
}


/******************************************************************/
SCH_CMP_FIELD* SCH_COMPONENT::GetField( int aFieldNdx ) const
/******************************************************************/
{
    const SCH_CMP_FIELD* field;

    if( (unsigned) aFieldNdx < m_Fields.size() )
        field = &m_Fields[aFieldNdx];
    else
        field = NULL;

    wxASSERT( field );

    // use case to remove const-ness
    return (SCH_CMP_FIELD*) field;
}


/******************************************************************/
void SCH_COMPONENT::AddField( const SCH_CMP_FIELD& aField )
/******************************************************************/
{
    m_Fields.push_back( aField );
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

    m_Fields.swap( copyitem->m_Fields );    // std::vector's swap()
}


/***********************************************************************/
void SCH_COMPONENT::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
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
        frame->SaveCopyInUndoList( this, IS_CHANGED );

        /* restore new values */
        SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );

        SAFE_DELETE( g_ItemToUndoCopy );
    }

    SCH_ITEM::Place( frame, DC );
}


/**********************************************************/
void SCH_COMPONENT::ClearAnnotation( DrawSheetPath* aSheet )
/**********************************************************/

/**
 * Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
 * @param aSheet: DrawSheetPath value: if NULL remove all annotations,
 *             else remove annotation relative to this sheetpath
 */
{
    wxString                defRef    = m_PrefixString;
    bool                    KeepMulti = false;
    EDA_LibComponentStruct* Entry;
    wxString                separators( wxT( " " ) );
    wxArrayString           reference_fields;

    Entry = FindLibPart( m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( Entry && Entry->m_UnitSelectionLocked )
        KeepMulti = true;

    while( defRef.Last() == '?' )
        defRef.RemoveLast();

    defRef.Append( wxT( "?" ) );

    wxString multi = wxT( "1" );

    if ( KeepMulti )        // We cannot remove all annotations: part selection must be kept
    {
        wxString NewHref;
        wxString path;
        if( aSheet )
            path = GetPath( aSheet );;
        for( unsigned int ii = 0; ii< m_PathsAndReferences.GetCount(); ii++ )
        {
            // Break hierachical reference in path, ref and multi selection:
            reference_fields = wxStringTokenize( m_PathsAndReferences[ii], separators );
            if( aSheet == NULL || reference_fields[0].Cmp( path ) == 0 )
            {
                if( KeepMulti )  // Get and keep part selection
                    multi = reference_fields[2];
                NewHref = reference_fields[0];
                NewHref << wxT( " " ) << defRef << wxT( " " ) << multi;
                m_PathsAndReferences[ii] = NewHref;
            }
        }
    }
    else
    {
        m_PathsAndReferences.Empty();   // Empty strings, but does not free memory because a new annotation will reuse it
        m_Multi = 1;
    }


    // These 2 changes do not work in complex hierarchy.
    // When a clear annotation is made, the calling function must call a
    // UpdateAllScreenReferences for the active sheet.
    // But this call cannot made here.
    m_Fields[REFERENCE].m_Text = defRef; //for drawing.

}


/**************************************************************/
SCH_COMPONENT* SCH_COMPONENT::GenCopy()
/**************************************************************/
{

#if 0
    SCH_COMPONENT* new_item = new SCH_COMPONENT( m_Pos );

    new_item->m_Multi        = m_Multi;
    new_item->m_ChipName     = m_ChipName;
    new_item->m_PrefixString = m_PrefixString;

    new_item->m_Convert = m_Convert;
    new_item->m_Transform[0][0] = m_Transform[0][0];
    new_item->m_Transform[0][1] = m_Transform[0][1];
    new_item->m_Transform[1][0] = m_Transform[1][0];
    new_item->m_Transform[1][1] = m_Transform[1][1];
    new_item->m_TimeStamp = m_TimeStamp;

    new_item->m_Fields = m_Fields;
#else
    SCH_COMPONENT* new_item = new SCH_COMPONENT( *this );

#endif

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
    for( int i = 1;  i<GetFieldCount();  ++i )
    {
        wxString value = GetField( i )->m_Text;

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



/****************************************/
bool SCH_COMPONENT::Save( FILE* f ) const
/****************************************/
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
        if( GetField(REFERENCE)->m_Text.IsEmpty() )
            strncpy( Name1, CONV_TO_UTF8( m_PrefixString ), sizeof(Name1) );
        else
            strncpy( Name1, CONV_TO_UTF8( GetField(REFERENCE)->m_Text ), sizeof(Name1) );
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

    for( int fieldNdx=0; fieldNdx<GetFieldCount();  ++fieldNdx )
    {
        SCH_CMP_FIELD* field = GetField( fieldNdx );

        wxString defaultName = ReturnDefaultFieldName( fieldNdx );

        // only save the field if there is a value in the field or if field name
        // is different than the default field name
        if( field->m_Text.IsEmpty() && defaultName == field->m_Name )
            continue;

        if( !field->Save( f ) )
        {
            Success = false;
            break;
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

    // This gives a reasonable approximation (but some things are missing so...)
    EDA_Rect  bbox = GetBoundaryBox();

    // Include BoundingBoxes of fields
    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        bbox.Merge( GetField(ii)->GetBoundaryBox() );
    }

    // ... add padding
    bbox.Inflate( PADDING, PADDING );

    return bbox;
}
