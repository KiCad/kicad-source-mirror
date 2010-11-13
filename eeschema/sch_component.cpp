/*******************************************************/
/* sch_component.cpp : handle the class SCH_COMPONENT  */
/*******************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "kicad_string.h"
#include "richio.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "macros.h"
#include "protos.h"
#include "class_library.h"
#include "lib_rectangle.h"
#include "lib_pin.h"
#include "lib_text.h"
#include "sch_component.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"
#include "template_fieldnames.h"

#include "dialogs/dialog_schematic_find.h"

#include <wx/tokenzr.h>


static LIB_COMPONENT* DummyCmp;


/* Descr component <DUMMY> used when a component is not found in library,
 *  to draw a dummy shape
 *  This component is a 400 mils square with the text ??
 *  DEF DUMMY U 0 40 Y Y 1 0 N
 *  F0 "U" 0 -350 60 H V
 *  F1 "DUMMY" 0 350 60 H V
 *  DRAW
 *  T 0 0 0 150 0 0 0 ??
 *  S -200 200 200 -200 0 1 0
 *  ENDDRAW
 *  ENDDEF
 */
void CreateDummyCmp()
{
    DummyCmp = new LIB_COMPONENT( wxEmptyString );

    LIB_RECTANGLE* Square = new LIB_RECTANGLE( DummyCmp );

    Square->Move( wxPoint( -200, 200 ) );
    Square->SetEndPosition( wxPoint( 200, -200 ) );

    LIB_TEXT* Text = new LIB_TEXT( DummyCmp );

    Text->m_Size.x = Text->m_Size.y = 150;
    Text->m_Text   = wxT( "??" );

    DummyCmp->AddDrawItem( Square );
    DummyCmp->AddDrawItem( Text );
}


SCH_COMPONENT::SCH_COMPONENT( const wxPoint& aPos, SCH_ITEM* aParent ) :
    SCH_ITEM( aParent, TYPE_SCH_COMPONENT )
{
    Init( aPos );
}


SCH_COMPONENT::SCH_COMPONENT( LIB_COMPONENT& libComponent,
                              SCH_SHEET_PATH* sheet, int unit, int convert,
                              const wxPoint& pos, bool setNewItemFlag ) :
    SCH_ITEM( NULL, TYPE_SCH_COMPONENT )
{
    Init( pos );

    m_Multi     = unit;
    m_Convert   = convert;
    m_ChipName  = libComponent.GetName();
    m_TimeStamp = GetTimeStamp();

    if( setNewItemFlag )
        m_Flags = IS_NEW | IS_MOVED;

    // Import user defined fields from the library component:
    LIB_FIELD_LIST libFields;

    libComponent.GetFields( libFields );

    for( LIB_FIELD_LIST::iterator it = libFields.begin();  it!=libFields.end();  ++it )
    {
        // Can no longer insert an empty name, since names are now keys.  The
        // field index is not used beyond the first MANDATORY_FIELDS
        if( it->m_Name.IsEmpty() )
            continue;

        // See if field by same name already exists.
        SCH_FIELD* schField = FindField( it->m_Name );

        if( !schField )
        {
            SCH_FIELD fld( wxPoint( 0, 0 ), GetFieldCount(), this, it->m_Name );
            schField = AddField( fld );
        }

        schField->m_Pos = m_Pos + it->m_Pos;

        schField->ImportValues( *it );

        schField->m_Text = it->m_Text;
    }

    wxString msg = libComponent.GetReferenceField().m_Text;

    if( msg.IsEmpty() )
        msg = wxT( "U" );

    m_PrefixString = msg;

    // update the reference -- just the prefix for now.
    msg += wxT( "?" );
    SetRef( sheet, msg );

    /* Use the schematic component name instead of the library value field
     * name.
     */
    GetField( VALUE )->m_Text = m_ChipName;
}


SCH_COMPONENT::SCH_COMPONENT( const SCH_COMPONENT& aTemplate ) :
    SCH_ITEM( NULL, TYPE_SCH_COMPONENT )
{
    /* assignment of all fields, including field vector elements, and linked
     * list pointers */
    *this = aTemplate;

    /* set linked list pointers to null, before this they were copies of
     * aTemplate's */
    Pback = NULL;
    Pnext = NULL;
    m_Son = NULL;

    // Re-parent the fields, which before this had aTemplate as parent
    for( int i = 0; i<GetFieldCount(); ++i )
    {
        GetField( i )->SetParent( this );
    }
}


void SCH_COMPONENT::Init( const wxPoint& pos )
{
    m_Pos     = pos;
    m_Multi   = 0;  // In multi unit chip - which unit to draw.
    m_Convert = 0;  // De Morgan Handling

    // The rotation/mirror transformation matrix. pos normal
    m_Transform = TRANSFORM();

    // construct only the mandatory fields, which are the first 4 only.
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        SCH_FIELD field( pos, i, this, TEMPLATE_FIELDNAME::GetDefaultFieldName( i ) );

        if( i==REFERENCE )
            field.SetLayer( LAYER_REFERENCEPART );
        else if( i==VALUE )
            field.SetLayer( LAYER_VALUEPART );

        // else keep LAYER_FIELDS from SCH_FIELD constructor

        // SCH_FIELD's implicitly created copy constructor is called in here
        AddField( field );
    }

    m_PrefixString = wxString( _( "U" ) );
}


/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as    *
* specified, and in the given drawing mode. Only this one is visible...      *
*****************************************************************************/
void SCH_COMPONENT::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& offset, int DrawMode, int Color,
                          bool DrawPinText )
{
    bool           dummy = FALSE;

    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry == NULL )
    {
        /* Create a dummy component if the actual component can not be found. */
        dummy = TRUE;
        if( DummyCmp == NULL )
            CreateDummyCmp();
        Entry = DummyCmp;
    }

    Entry->Draw( panel, DC, m_Pos + offset, dummy ? 0 : m_Multi,
                 dummy ? 0 : m_Convert, DrawMode, Color, m_Transform,
                 DrawPinText, false );

    SCH_FIELD* field = GetField( REFERENCE );

    if( field->IsVisible() && !( field->m_Flags & IS_MOVED ) )
    {
        if( Entry->GetPartCount() > 1 )
        {
            field->m_AddExtraText = true;
            field->Draw( panel, DC, offset, DrawMode );
        }
        else
        {
            field->m_AddExtraText = false;
            field->Draw( panel, DC, offset, DrawMode );
        }
    }

    for( int ii = VALUE; ii < GetFieldCount(); ii++ )
    {
        field = GetField( ii );

        if( field->m_Flags & IS_MOVED )
            continue;

        field->Draw( panel, DC, offset, DrawMode );
    }


#if 0
    /* Draw the component boundary box */
    {
        EDA_Rect BoundaryBox;
        BoundaryBox = GetBoundaryBox();
        GRRect( &panel->m_ClipBox, DC, BoundaryBox, BROWN );
#if 1
        if( GetField( REFERENCE )->IsVisible() )
        {
            BoundaryBox = GetField( REFERENCE )->GetBoundaryBox();
            GRRect( &panel->m_ClipBox, DC, BoundaryBox, BROWN );
        }

        if( GetField( VALUE )->IsVisible() )
        {
            BoundaryBox = GetField( VALUE )->GetBoundaryBox();
            GRRect( &panel->m_ClipBox, DC, BoundaryBox, BROWN );
        }
#endif
    }
#endif
}


/**
 * Function AddHierarchicalReference
 * adds a full hierarchical reference (path + local reference)
 * @param aPath = hierarchical path (/<sheet timestamp>/component timestamp>
 * like /05678E50/A23EF560)
 * @param aRef = local reference like C45, R56
 * @param aMulti = part selection, used in multi part per package (0 or 1 for
 * non multi)
 */
void SCH_COMPONENT::AddHierarchicalReference( const wxString& aPath,
                                              const wxString& aRef,
                                              int             aMulti )
{
    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    // Search for an existing path and remove it if found (should not occur)
    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
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


wxString SCH_COMPONENT::ReturnFieldName( int aFieldNdx ) const
{
    SCH_FIELD* field = GetField( aFieldNdx );

    if( field )
    {
        if( !field->m_Name.IsEmpty() )
            return field->m_Name;
        else
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( aFieldNdx );
    }

    return wxEmptyString;
}


wxString SCH_COMPONENT::GetPath( SCH_SHEET_PATH* sheet )
{
    wxString str;

    str.Printf( wxT( "%8.8lX" ), m_TimeStamp );
    return sheet->Path() + str;
}


const wxString SCH_COMPONENT::GetRef( SCH_SHEET_PATH* sheet )
{
    wxString          path = GetPath( sheet );
    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();

        if( h_path.Cmp( path ) == 0 )
        {
            h_ref = tokenizer.GetNextToken();

            /* printf( "GetRef hpath: %s\n",
             *       CONV_TO_UTF8( m_PathsAndReferences[ii] ) ); */
            return h_ref;
        }
    }

    // if it was not found in m_Paths array, then see if it is in
    // m_Field[REFERENCE] -- if so, use this as a default for this path.
    // this will happen if we load a version 1 schematic file.
    // it will also mean that multiple instances of the same sheet by default
    // all have the same component references, but perhaps this is best.
    if( !GetField( REFERENCE )->m_Text.IsEmpty() )
    {
        SetRef( sheet, GetField( REFERENCE )->m_Text );
        return GetField( REFERENCE )->m_Text;
    }
    return m_PrefixString;
}


void SCH_COMPONENT::SetRef( SCH_SHEET_PATH* sheet, const wxString& ref )
{
    wxString          path = GetPath( sheet );

    bool              notInArray = true;

    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    // check to see if it is already there before inserting it
    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();
        if( h_path.Cmp( path ) == 0 )
        {
            // just update the reference text, not the timestamp.
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

    SCH_FIELD* rf = GetField( REFERENCE );

    if( rf->m_Text.IsEmpty()
       || ( abs( rf->m_Pos.x - m_Pos.x ) +
            abs( rf->m_Pos.y - m_Pos.y ) > 10000 ) )
    {
        // move it to a reasonable position
        rf->m_Pos    = m_Pos;
        rf->m_Pos.x += 50;         // a slight offset
        rf->m_Pos.y += 50;
    }

    rf->m_Text = ref;  // for drawing.

    // Reinit the m_PrefixString member if needed
    wxString prefix = ref;
    while( prefix.Last() == '?' or isdigit( prefix.Last() ) )
        prefix.RemoveLast();

    if( m_PrefixString != prefix )
        m_PrefixString = prefix;
}


/**
 * Function SetTimeStamp
 * Change the old time stamp to the new time stamp.
 * the time stamp is also modified in paths
 * @param aNewTimeStamp = new time stamp
 */
void SCH_COMPONENT::SetTimeStamp( long aNewTimeStamp )
{
    wxString string_timestamp, string_oldtimestamp;

    string_timestamp.Printf( wxT( "%8.8X" ), aNewTimeStamp );
    string_oldtimestamp.Printf( wxT( "%8.8X" ), m_TimeStamp );
    m_TimeStamp = aNewTimeStamp;
    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
    {
        m_PathsAndReferences[ii].Replace( string_oldtimestamp.GetData(),
                                         string_timestamp.GetData() );
    }
}


/***********************************************************/

//returns the unit selection, for the given sheet path.
/***********************************************************/
int SCH_COMPONENT::GetUnitSelection( SCH_SHEET_PATH* aSheet )
{
    wxString          path = GetPath( aSheet );
    wxString          h_path, h_multi;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
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

    // if it was not found in m_Paths array, then use m_Multi.
    // this will happen if we load a version 1 schematic file.
    return m_Multi;
}


/****************************************************************************/

//Set the unit selection, for the given sheet path.
/****************************************************************************/
void SCH_COMPONENT::SetUnitSelection( SCH_SHEET_PATH* aSheet,
                                      int             aUnitSelection )
{
    wxString          path = GetPath( aSheet );

    bool              notInArray = true;

    wxString          h_path, h_ref;
    wxStringTokenizer tokenizer;
    wxString          separators( wxT( " " ) );

    //check to see if it is already there before inserting it
    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
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


SCH_FIELD* SCH_COMPONENT::GetField( int aFieldNdx ) const
{
    const SCH_FIELD* field;

    if( (unsigned) aFieldNdx < m_Fields.size() )
        field = &m_Fields[aFieldNdx];
    else
        field = NULL;

    wxASSERT( field );

    // use cast to remove const-ness
    return (SCH_FIELD*) field;
}


SCH_FIELD* SCH_COMPONENT::AddField( const SCH_FIELD& aField )
{
    int newNdx = m_Fields.size();

    m_Fields.push_back( aField );
    return &m_Fields[newNdx];
}


SCH_FIELD* SCH_COMPONENT::FindField( const wxString& aFieldName )
{
    for( unsigned i = 0;  i<m_Fields.size();  ++i )
    {
        if( aFieldName == m_Fields[i].m_Name )
            return &m_Fields[i];
    }

    return NULL;
}


LIB_PIN* SCH_COMPONENT::GetPin( const wxString& number )
{
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry == NULL )
        return NULL;

    return Entry->GetPin( number, m_Multi, m_Convert );
}


/**
 * Function GetBoundaryBox
 * returns the orthogonal, bounding box of this object for display purposes.
 * This box should be an enclosing perimeter for graphic items and pins.
 * this include only fields defined in library
 * use GetBoundingBox() to include fields in schematic
 */
EDA_Rect SCH_COMPONENT::GetBoundaryBox() const
{
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );
    EDA_Rect       BoundaryBox;
    int            x0, xm, y0, ym;

    /* Get the basic Boundary box */
    if( Entry )
    {
        BoundaryBox = Entry->GetBoundaryBox( m_Multi, m_Convert );
        x0 = BoundaryBox.GetX();
        xm = BoundaryBox.GetRight();

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
    int x1 = m_Transform.x1 * x0 + m_Transform.y1 * y0;
    int y1 = m_Transform.x2 * x0 + m_Transform.y2 * y0;
    int x2 = m_Transform.x1 * xm + m_Transform.y1 * ym;
    int y2 = m_Transform.x2 * xm + m_Transform.y2 * ym;

    // H and W must be > 0:
    if( x2 < x1 )
        EXCHG( x2, x1 );
    if( y2 < y1 )
        EXCHG( y2, y1 );

    BoundaryBox.SetX( x1 );
    BoundaryBox.SetY( y1 );
    BoundaryBox.SetWidth( x2 - x1 );
    BoundaryBox.SetHeight( y2 - y1 );

    BoundaryBox.Offset( m_Pos );
    return BoundaryBox;
}


/* Used in undo / redo command:
 *  swap data between this and copyitem
 */
void SCH_COMPONENT::SwapData( SCH_COMPONENT* copyitem )
{
    EXCHG( m_ChipName, copyitem->m_ChipName );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Multi, copyitem->m_Multi );
    EXCHG( m_Convert, copyitem->m_Convert );

    TRANSFORM tmp = m_Transform;
    m_Transform = copyitem->m_Transform;
    copyitem->m_Transform = tmp;

    m_Fields.swap( copyitem->m_Fields );    // std::vector's swap()

    // Reparent items after copying data
    // (after swap(), m_Parent member does not point to the right parent):
    for( int ii = 0; ii < copyitem->GetFieldCount();  ++ii )
    {
        copyitem->GetField( ii )->SetParent( copyitem );
    }

    for( int ii = 0; ii < GetFieldCount();  ++ii )
    {
        GetField( ii )->SetParent( this );
    }

    EXCHG( m_PathsAndReferences, copyitem->m_PathsAndReferences );
}


void SCH_COMPONENT::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
{
    /* save old text in undo list */
    if( g_ItemToUndoCopy
       && ( g_ItemToUndoCopy->Type() == Type() )
       && ( ( m_Flags & IS_NEW ) == 0 ) )
    {
        /* restore old values and save new ones */
        SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );

        /* save in undo list */
        frame->SaveCopyInUndoList( this, UR_CHANGED );

        /* restore new values */
        SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );

        SAFE_DELETE( g_ItemToUndoCopy );
    }

    SCH_ITEM::Place( frame, DC );
}


/**
 * Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
 * @param aSheet: SCH_SHEET_PATH value: if NULL remove all annotations,
 *                else remove annotation relative to this sheetpath
 */
void SCH_COMPONENT::ClearAnnotation( SCH_SHEET_PATH* aSheet )
{
    wxString       defRef    = m_PrefixString;
    bool           KeepMulti = false;
    LIB_COMPONENT* Entry;
    wxString       separators( wxT( " " ) );
    wxArrayString  reference_fields;

    Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry && Entry->UnitsLocked() )
        KeepMulti = true;

    while( defRef.Last() == '?' )
        defRef.RemoveLast();

    defRef.Append( wxT( "?" ) );

    wxString multi = wxT( "1" );

    // We cannot remove all annotations: part selection must be kept
    if( KeepMulti )
    {
        wxString NewHref;
        wxString path;
        if( aSheet )
            path = GetPath( aSheet );
        for( unsigned int ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
        {
            // Break hierarchical reference in path, ref and multi selection:
            reference_fields = wxStringTokenize( m_PathsAndReferences[ii],
                                                 separators );
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
        // Empty strings, but does not free memory because a new annotation
        // will reuse it
        m_PathsAndReferences.Empty();
        m_Multi = 1;
    }


    // These 2 changes do not work in complex hierarchy.
    // When a clear annotation is made, the calling function must call a
    // UpdateAllScreenReferences for the active sheet.
    // But this call cannot made here.
    m_Fields[REFERENCE].m_Text = defRef; //for drawing.
}


/******************************************************************/

/* Compute the new matrix transform for a schematic component
 *  in order to have the requested transform (type_rotate = rot, mirror..)
 *  which is applied to the initial transform.
 */
/*****************************************************************/
void SCH_COMPONENT::SetOrientation( int aOrientation )
{
    TRANSFORM temp = TRANSFORM();
    bool Transform = false;

    switch( aOrientation )
    {
    case CMP_ORIENT_0:
    case CMP_NORMAL:            /* Position Initiale */
        m_Transform.x1 = 1;
        m_Transform.y2 = -1;
        m_Transform.x2 = m_Transform.y1 = 0;
        break;

    case CMP_ROTATE_CLOCKWISE:            /* Rotate + */
        temp.x1   = temp.y2 = 0;
        temp.y1   = 1;
        temp.x2   = -1;
        Transform = true;
        break;

    case CMP_ROTATE_COUNTERCLOCKWISE:             /* Rotate - */
        temp.x1   = temp.y2 = 0;
        temp.y1   = -1;
        temp.x2   = 1;
        Transform = true;
        break;

    case CMP_MIRROR_Y:          /* MirrorY */
        temp.x1   = -1;
        temp.y2   = 1;
        temp.y1   = temp.x2 = 0;
        Transform = true;
        break;

    case CMP_MIRROR_X:            /* MirrorX */
        temp.x1   = 1;
        temp.y2   = -1;
        temp.y1   = temp.x2 = 0;
        Transform = TRUE;
        break;

    case CMP_ORIENT_90:
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
        break;

    case CMP_ORIENT_180:
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
        SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
        break;

    case CMP_ORIENT_270:
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_ROTATE_CLOCKWISE );
        break;

    case ( CMP_ORIENT_0 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_0 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_0 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    case ( CMP_ORIENT_90 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_90 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_90 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_90 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    case ( CMP_ORIENT_180 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_180 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_180 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_180 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    case ( CMP_ORIENT_270 + CMP_MIRROR_X ):
        SetOrientation( CMP_ORIENT_270 );
        SetOrientation( CMP_MIRROR_X );
        break;

    case ( CMP_ORIENT_270 + CMP_MIRROR_Y ):
        SetOrientation( CMP_ORIENT_270 );
        SetOrientation( CMP_MIRROR_Y );
        break;

    default:
        Transform = FALSE;
        wxMessageBox( wxT( "SetRotateMiroir() error: ill value" ) );
        break;
    }

    if( Transform )
    {
        /* The new matrix transform is the old matrix transform modified by the
         *  requested transformation, which is the TempMat transform (rot,
         *  mirror ..) in order to have (in term of matrix transform):
         *     transform coord = new_m_Transform * coord
         *  where transform coord is the coord modified by new_m_Transform from
         *  the initial value coord.
         *  new_m_Transform is computed (from old_m_Transform and TempMat) to
         *  have:
         *     transform coord = old_m_Transform * coord * TempMat
         */
        TRANSFORM newTransform;

        newTransform.x1 = m_Transform.x1 * temp.x1 + m_Transform.x2 * temp.y1;
        newTransform.y1 = m_Transform.y1 * temp.x1 + m_Transform.y2 * temp.y1;
        newTransform.x2 = m_Transform.x1 * temp.x2 + m_Transform.x2 * temp.y2;
        newTransform.y2 = m_Transform.y1 * temp.x2 + m_Transform.y2 * temp.y2;
        m_Transform = newTransform;
    }
}


/**
 * Function GetOrientation
 * Used to display component orientation (in dialog editor or info)
 * @return the orientation and mirror
 * Note: Because there are different ways to have a given orientation/mirror,
 * the orientation/mirror is not necessary what the used does
 * (example : a mirrorX then a mirrorY give no mirror but rotate the component).
 * So this function find a rotation and a mirror value
 * ( CMP_MIRROR_X because this is the first mirror option tested)
 *  but can differs from the orientation made by an user
 * ( a CMP_MIRROR_Y is find as a CMP_MIRROR_X + orientation 180, because they
 * are equivalent)
 *
 */
int SCH_COMPONENT::GetOrientation()
{
    int type_rotate = CMP_ORIENT_0;
    TRANSFORM transform;
    int ii;

    #define ROTATE_VALUES_COUNT 12

    // list of all possibilities, but only the first 8 are actually used
    int rotate_value[ROTATE_VALUES_COUNT] =
    {
        CMP_ORIENT_0,                  CMP_ORIENT_90,                  CMP_ORIENT_180,
        CMP_ORIENT_270,
        CMP_MIRROR_X + CMP_ORIENT_0,   CMP_MIRROR_X + CMP_ORIENT_90,
        CMP_MIRROR_X + CMP_ORIENT_180, CMP_MIRROR_X + CMP_ORIENT_270,
        CMP_MIRROR_Y + CMP_ORIENT_0,   CMP_MIRROR_Y + CMP_ORIENT_90,
        CMP_MIRROR_Y + CMP_ORIENT_180, CMP_MIRROR_Y + CMP_ORIENT_270
    };

    // Try to find the current transform option:
    transform = m_Transform;

    for( ii = 0; ii < ROTATE_VALUES_COUNT; ii++ )
    {
        type_rotate = rotate_value[ii];
        SetOrientation( type_rotate );

        if( transform == m_Transform )
            return type_rotate;
    }

    // Error: orientation not found in list (should not happen)
    wxMessageBox( wxT( "Component orientation matrix internal error" ) );
    m_Transform = transform;

    return CMP_NORMAL;
}


/**
 * Returns the coordinated point, depending on the orientation of the
 * component (rotation, mirror).
 * The coordinates are always relative to the anchor position of the component.
 */
wxPoint SCH_COMPONENT::GetScreenCoord( const wxPoint& coord )
{
    return m_Transform.TransformCoordinate( coord );
}


#if defined(DEBUG)

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
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " ref=\"" << CONV_TO_UTF8( ReturnFieldName( 0 ) )
                                 << '"' << " chipName=\""
                                 << CONV_TO_UTF8( m_ChipName ) << '"' << m_Pos
                                 << " layer=\"" << m_Layer
                                 << '"' << ">\n";

    // skip the reference, it's been output already.
    for( int i = 1; i < GetFieldCount();  ++i )
    {
        wxString value = GetField( i )->m_Text;

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" << " name=\""
                                             << CONV_TO_UTF8( ReturnFieldName( i ) )
                                             << '"' << " value=\""
                                             << CONV_TO_UTF8( value ) << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << CONV_TO_UTF8( GetClass().Lower() )
                                 << ">\n";
}


#endif


bool SCH_COMPONENT::Save( FILE* f ) const
{
    int             ii;
    char            Name1[256], Name2[256];
    wxArrayString   reference_fields;

    static wxString delimiters( wxT( " " ) );

    //this is redundant with the AR entries below, but it makes the
    //files backwards-compatible.
    if( m_PathsAndReferences.GetCount() > 0 )
    {
        reference_fields = wxStringTokenize( m_PathsAndReferences[0],
                                             delimiters );
        strncpy( Name1, CONV_TO_UTF8( reference_fields[1] ), sizeof( Name1 ) );
    }
    else
    {
        if( GetField( REFERENCE )->m_Text.IsEmpty() )
            strncpy( Name1, CONV_TO_UTF8( m_PrefixString ), sizeof( Name1 ) );
        else
            strncpy( Name1, CONV_TO_UTF8( GetField( REFERENCE )->m_Text ), sizeof( Name1 ) );
    }

    for( ii = 0; ii < (int) strlen( Name1 ); ii++ )
    {
#if defined(KICAD_GOST)
        if( Name1[ii] == ' ' )
#else
        if( Name1[ii] <= ' ' )
#endif

            Name1[ii] = '~';
    }

    if( !m_ChipName.IsEmpty() )
    {
        strncpy( Name2, CONV_TO_UTF8( m_ChipName ), sizeof( Name2 ) );
        for( ii = 0; ii < (int) strlen( Name2 ); ii++ )
#if defined(KICAD_GOST)

            if( Name2[ii] == ' ' )
#else

            if( Name2[ii] <= ' ' )
#endif

                Name2[ii] = '~';
    }
    else
        strncpy( Name2, NULL_STRING, sizeof( Name2 ) );

    if( fprintf( f, "$Comp\n" ) == EOF )
        return false;

    if( fprintf( f, "L %s %s\n", Name2, Name1 ) == EOF )
        return false;

    /* Generate unit number, convert and time stamp*/
    if( fprintf( f, "U %d %d %8.8lX\n", m_Multi, m_Convert,
                 m_TimeStamp ) == EOF )
        return false;

    /* Save the position */
    if( fprintf( f, "P %d %d\n", m_Pos.x, m_Pos.y ) == EOF )
        return false;

    /* If this is a complex hierarchy; save hierarchical references.
     * but for simple hierarchies it is not necessary.
     * the reference inf is already saved
     * this is useful for old eeschema version compatibility
     */
    if( m_PathsAndReferences.GetCount() > 1 )
    {
        for( unsigned int ii = 0; ii <  m_PathsAndReferences.GetCount(); ii++ )
        {
            /*format:
             * AR Path="/140/2" Ref="C99"   Part="1"
             * where 140 is the uid of the containing sheet
             * and 2 is the timestamp of this component.
             * (timestamps are actually 8 hex chars)
             * Ref is the conventional component reference for this 'path'
             * Part is the conventional component part selection for this 'path'
             */
            reference_fields = wxStringTokenize( m_PathsAndReferences[ii],
                                                 delimiters );
            if( fprintf( f, "AR Path=\"%s\" Ref=\"%s\"  Part=\"%s\" \n",
                        CONV_TO_UTF8( reference_fields[0] ),
                        CONV_TO_UTF8( reference_fields[1] ),
                        CONV_TO_UTF8( reference_fields[2] ) ) == EOF )
                return false;
        }
    }

    // update the ugly field index, which I would like to see go away someday soon.
    for( unsigned i = 0;  i<m_Fields.size();  ++i )
    {
        SCH_FIELD* fld = GetField( i );
        fld->m_FieldId = i;  // we don't need field Ids, please be gone.
    }

    // Fixed fields:
    // Save fixed fields which are non blank.
    for( unsigned i = 0;  i<MANDATORY_FIELDS;  ++i )
    {
        SCH_FIELD* fld = GetField( i );
        if( !fld->m_Text.IsEmpty() )
        {
            if( !fld->Save( f ) )
                return false;
        }
    }

    // User defined fields:
    // The *policy* about which user defined fields are part of a symbol is now
    // only in the dialog editors.  No policy should be enforced here, simply
    // save all the user defined fields, they are present because a dialog editor
    // thought they should be.  If you disagree, go fix the dialog editors.
    for( unsigned i = MANDATORY_FIELDS;  i<m_Fields.size();  ++i )
    {
        SCH_FIELD* fld = GetField( i );
        if( !fld->Save( f ) )
            return false;
    }

    /* Unit number, position, box ( old standard ) */
    if( fprintf( f, "\t%-4d %-4d %-4d\n", m_Multi, m_Pos.x, m_Pos.y ) == EOF )
        return false;

    if( fprintf( f, "\t%-4d %-4d %-4d %-4d\n",
                 m_Transform.x1, m_Transform.y1, m_Transform.x2, m_Transform.y2 ) == EOF )
        return false;

    if( fprintf( f, "$EndComp\n" ) == EOF )
        return false;

    return true;
}


bool SCH_COMPONENT::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int            ii;
    char           Name1[256], Name2[256],
                   Char1[256], Char2[256], Char3[256];
    int            newfmt = 0;
    char*          ptcar;
    wxString       fieldName;

    m_Convert = 1;

    if( ((char*)aLine)[0] == '$' )
    {
        newfmt = 1;

        if( !aLine.ReadLine() )
            return TRUE;
    }

    if( sscanf( &((char*)aLine)[1], "%s %s", Name1, Name2 ) != 2 )
    {
        aErrorMsg.Printf( wxT( "EESchema Component descr error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << CONV_FROM_UTF8( ((char*)aLine) );
        return false;
    }

    if( strcmp( Name1, NULL_STRING ) != 0 )
    {
        for( ii = 0; ii < (int) strlen( Name1 ); ii++ )
            if( Name1[ii] == '~' )
                Name1[ii] = ' ';

        m_ChipName = CONV_FROM_UTF8( Name1 );
        if( !newfmt )
            GetField( VALUE )->m_Text = CONV_FROM_UTF8( Name1 );
    }
    else
    {
        m_ChipName.Empty();
        GetField( VALUE )->m_Text.Empty();
        GetField( VALUE )->m_Orient    = TEXT_ORIENT_HORIZ;
        GetField( VALUE )->m_Attributs = TEXT_NO_VISIBLE;
    }

    if( strcmp( Name2, NULL_STRING ) != 0 )
    {
        bool isDigit = false;
        for( ii = 0; ii < (int) strlen( Name2 ); ii++ )
        {
            if( Name2[ii] == '~' )
                Name2[ii] = ' ';

            // get RefBase from this, too. store in Name1.
            if( Name2[ii] >= '0' && Name2[ii] <= '9' )
            {
                isDigit   = true;
                Name1[ii] = 0;  //null-terminate.
            }
            if( !isDigit )
            {
                Name1[ii] = Name2[ii];
            }
        }

        Name1[ii] = 0; //just in case
        int  jj;
        for( jj = 0; jj<ii && Name1[jj] == ' '; jj++ )
            ;

        if( jj == ii )
        {
            // blank string.
            m_PrefixString = wxT( "U" );
        }
        else
        {
            m_PrefixString = CONV_FROM_UTF8( &Name1[jj] );

            //printf("prefix: %s\n", CONV_TO_UTF8(component->m_PrefixString));
        }

        if( !newfmt )
            GetField( REFERENCE )->m_Text = CONV_FROM_UTF8( Name2 );
    }
    else
    {
        GetField( REFERENCE )->m_Attributs = TEXT_NO_VISIBLE;
    }

    /* Parse component description
     * These lines begin with:
     * "P" = Position
     * U = Num Unit and Conversion
     * "Fn" = Fields (0 .. n = = number of field)
     * "Ar" = Alternate reference in the case of multiple sheets referring to
     *        one schematic file.
     */
    for( ; ; )
    {
        if( !aLine.ReadLine() )
            return false;

        if( ((char*)aLine)[0] == 'U' )
        {
            sscanf( ((char*)aLine) + 1, "%d %d %lX", &m_Multi, &m_Convert, &m_TimeStamp );
        }
        else if( ((char*)aLine)[0] == 'P' )
        {
            sscanf( ((char*)aLine) + 1, "%d %d", &m_Pos.x, &m_Pos.y );

            // Set fields position to a default position (that is the
            // component position.  For existing fields, the real position
            // will be set later
            for( int i = 0; i<GetFieldCount();  ++i )
            {
                if( GetField( i )->m_Text.IsEmpty() )
                    GetField( i )->m_Pos = m_Pos;
            }
        }
        else if( ((char*)aLine)[0] == 'A' && ((char*)aLine)[1] == 'R' )
        {
            /* format:
             * AR Path="/9086AF6E/67452AA0" Ref="C99" Part="1"
             * where 9086AF6E is the unique timestamp of the containing sheet
             * and 67452AA0 is the timestamp of this component.
             * C99 is the reference given this path.
             */
            int ii;
            ptcar = ((char*)aLine) + 2;

            //copy the path.
            ii     = ReadDelimitedText( Name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString path = CONV_FROM_UTF8( Name1 );

            // copy the reference
            ii     = ReadDelimitedText( Name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString ref = CONV_FROM_UTF8( Name1 );

            // copy the multi, if exists
            ii = ReadDelimitedText( Name1, ptcar, 255 );
            if( Name1[0] == 0 )  // Nothing read, put a default value
                sprintf( Name1, "%d", m_Multi );
            int multi = atoi( Name1 );
            if( multi < 0 || multi > 25 )
                multi = 1;
            AddHierarchicalReference( path, ref, multi );
            GetField( REFERENCE )->m_Text = ref;
        }
        else if( ((char*)aLine)[0] == 'F' )
        {
            int  fieldNdx;

            char FieldUserName[1024];
            GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
            GRTextVertJustifyType  vjustify = GR_TEXT_VJUSTIFY_CENTER;

            FieldUserName[0] = 0;

            ptcar = (char*) aLine;

            while( *ptcar && (*ptcar != '"') )
                ptcar++;

            if( *ptcar != '"' )
            {
                aErrorMsg.Printf( wxT( "EESchema file lib field F at line %d, aborted" ),
                                  aLine.LineNumber() );
                return false;
            }

            for( ptcar++, ii = 0; ; ii++, ptcar++ )
            {
                Name1[ii] = *ptcar;
                if( *ptcar == 0 )
                {
                    aErrorMsg.Printf( wxT( "Component field F at line %d, aborted" ),
                                      aLine.LineNumber() );
                    return false;
                }

                if( *ptcar == '"' )
                {
                    Name1[ii] = 0;
                    ptcar++;
                    break;
                }
            }

            fieldNdx = atoi( ((char*)aLine) + 2 );

            ReadDelimitedText( FieldUserName, ptcar, sizeof(FieldUserName) );

            if( !FieldUserName[0] )
                fieldName = TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldNdx );
            else
                fieldName = CONV_FROM_UTF8( FieldUserName );

            if( fieldNdx >= GetFieldCount() )
            {
                // The first MANDATOR_FIELDS _must_ be constructed within
                // the SCH_COMPONENT constructor.  This assert is simply here
                // to guard against a change in that constructor.
                wxASSERT( GetFieldCount() >= MANDATORY_FIELDS );

                // Ignore the _supplied_ fieldNdx.  It is not important anymore
                // if within the user defined fields region (i.e. >= MANDATORY_FIELDS).
                // We freely renumber the index to fit the next available field slot.

                fieldNdx = GetFieldCount();  // new has this index after insertion

                SCH_FIELD field( wxPoint( 0, 0 ),
                                 -1,     // field id is not relavant for user defined fields
                                 this, fieldName );

                AddField( field );
            }
            else
            {
                GetField( fieldNdx )->m_Name = fieldName;
            }

            GetField( fieldNdx )->m_Text = CONV_FROM_UTF8( Name1 );
            memset( Char3, 0, sizeof(Char3) );
            if( ( ii = sscanf( ptcar, "%s %d %d %d %X %s %s", Char1,
                               &GetField( fieldNdx )->m_Pos.x,
                               &GetField( fieldNdx )->m_Pos.y,
                               &GetField( fieldNdx )->m_Size.x,
                               &GetField( fieldNdx )->m_Attributs,
                               Char2, Char3 ) ) < 4 )
            {
                aErrorMsg.Printf( wxT( "Component Field error line %d, aborted" ),
                                  aLine.LineNumber() );
                continue;
            }

            if( (GetField( fieldNdx )->m_Size.x == 0 ) || (ii == 4) )
                GetField( fieldNdx )->m_Size.x = DEFAULT_SIZE_TEXT;

            GetField( fieldNdx )->m_Orient = TEXT_ORIENT_HORIZ;
            GetField( fieldNdx )->m_Size.y = GetField( fieldNdx )->m_Size.x;

            if( Char1[0] == 'V' )
                GetField( fieldNdx )->m_Orient = TEXT_ORIENT_VERT;

            if( ii >= 7 )
            {
                if( *Char2 == 'L' )
                    hjustify = GR_TEXT_HJUSTIFY_LEFT;
                else if( *Char2 == 'R' )
                    hjustify = GR_TEXT_HJUSTIFY_RIGHT;
                if( Char3[0] == 'B' )
                    vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                else if( Char3[0] == 'T' )
                    vjustify = GR_TEXT_VJUSTIFY_TOP;
                if( Char3[1] == 'I' )
                    GetField( fieldNdx )->m_Italic = true;
                else
                    GetField( fieldNdx )->m_Italic = false;
                if( Char3[2] == 'B' )
                    GetField( fieldNdx )->m_Bold = true;
                else
                    GetField( fieldNdx )->m_Bold = false;

                GetField( fieldNdx )->m_HJustify = hjustify;
                GetField( fieldNdx )->m_VJustify = vjustify;
            }

            if( fieldNdx == REFERENCE )
                if( GetField( fieldNdx )->m_Text[0] == '#' )
                    GetField( fieldNdx )->m_Attributs |= TEXT_NO_VISIBLE;
        }
        else
            break;
    }

    if( sscanf( ((char*)aLine), "%d %d %d", &m_Multi, &m_Pos.x, &m_Pos.y ) != 3 )
    {
        aErrorMsg.Printf( wxT( "Component unit & pos error at line %d, aborted" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !aLine.ReadLine() ||
        sscanf( ((char*)aLine), "%d %d %d %d",
                &m_Transform.x1,
                &m_Transform.y1,
                &m_Transform.x2,
                &m_Transform.y2 ) != 4 )
    {
        aErrorMsg.Printf( wxT( "Component orient error at line %d, aborted" ),
                          aLine.LineNumber() );
        return false;
    }

    if( newfmt )
    {
        if( !aLine.ReadLine() )
            return false;

        if( strnicmp( "$End", ((char*)aLine), 4 ) != 0 )
        {
            aErrorMsg.Printf( wxT( "Component End expected at line %d, aborted" ),
                              aLine.LineNumber() );
            return false;
        }
    }

    return true;
}


/**
 * Function GetBoundingBox
 * returns the orthogonal, bounding box of this object for display purposes.
 * This box should be an enclosing perimeter for visible components of this
 * object, and the units should be in the pcb or schematic coordinate system.
 * It is OK to overestimate the size by a few counts.
 */
EDA_Rect SCH_COMPONENT::GetBoundingBox()
{
    const int PADDING = 40;

    // This gives a reasonable approximation (but some things are missing so...)
    EDA_Rect  bbox = GetBoundaryBox();

    // Include BoundingBoxes of fields
    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        if( !GetField( ii )->IsVisible() )
            continue;
        bbox.Merge( GetField( ii )->GetBoundaryBox() );
    }

    // ... add padding
    bbox.Inflate( PADDING );

    return bbox;
}


void SCH_COMPONENT::DisplayInfo( WinEDA_DrawFrame* frame )
{
    // search for the component in lib
    // Entry and root_component can differ if Entry is an alias
    LIB_ALIAS* alias = CMP_LIBRARY::FindLibraryEntry( m_ChipName );
    LIB_COMPONENT* root_component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( (alias == NULL) || (root_component == NULL) )
        return;

    wxString msg;

    frame->ClearMsgPanel();

    frame->AppendMsgPanel( _( "Reference" ),
                           GetRef( ( (WinEDA_SchematicFrame*) frame )->GetSheet() ),
                           DARKCYAN );

    if( root_component->IsPower() )
        msg = _( "Power symbol" );
    else
        msg = _( "Name" );
    frame->AppendMsgPanel( msg, GetField( VALUE )->m_Text, DARKCYAN );

    // Display component reference in library and library
    frame->AppendMsgPanel( _( "Component" ), m_ChipName, BROWN );
    if( alias->GetName() != root_component->GetName() )
        frame->AppendMsgPanel( _( "Alias of" ), root_component->GetName(), BROWN );
    frame->AppendMsgPanel( _( "Library" ), alias->GetLibraryName(), BROWN );

    // Display description of the component, and keywords found in lib
    frame->AppendMsgPanel( _( "Description" ), alias->GetDescription(), DARKCYAN );
    frame->AppendMsgPanel( _( "Key words" ), alias->GetKeyWords(), DARKCYAN );
}


/**
 * Function Mirror_Y (virtual)
 * mirror item relative to an Y axis
 * @param aYaxis_position = the y axis position
 */
void SCH_COMPONENT::Mirror_Y( int aYaxis_position )
{
    int dx = m_Pos.x;

    SetOrientation( CMP_MIRROR_Y );
    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
    dx -= m_Pos.x;     // dx,0 is the move vector for this transform

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        /* move the fields to the new position because the component itself
         * has moved */
        GetField( ii )->m_Pos.x -= dx;
    }
}


/**
 * Function Mirror_X (virtual)
 * mirror item relative to an X axis
 * @param aXaxis_position = the x axis position
 */
void SCH_COMPONENT::Mirror_X( int aXaxis_position )
{
    int dy = m_Pos.y;

    SetOrientation( CMP_MIRROR_X );
    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
    dy -= m_Pos.y;     // dy,0 is the move vector for this transform

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        /* move the fields to the new position because the component itself
         * has moved */
        GetField( ii )->m_Pos.y -= dy;
    }
}


void SCH_COMPONENT::Rotate( wxPoint rotationPoint )
{
    wxPoint prev = m_Pos;

    RotatePoint( &m_Pos, rotationPoint, 900 );

    //SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );
    SetOrientation( CMP_ROTATE_CLOCKWISE );

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        /* move the fields to the new position because the component itself
         * has moved */
        GetField( ii )->m_Pos.x -= prev.x - m_Pos.x;
        GetField( ii )->m_Pos.y -= prev.y - m_Pos.y;
    }
}


bool SCH_COMPONENT::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint * aFindLocation )
{
    // Search reference.
    // reference is a special field because a part identifier is added
    // in multi parts per package
    // the .m_AddExtraText of the field must be set to add this identifier:
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry && Entry->GetPartCount() > 1 )
        GetField( REFERENCE )->m_AddExtraText = true;
    else
        GetField( REFERENCE )->m_AddExtraText = false;

    if( GetField( REFERENCE )->Matches( aSearchData, aAuxData, aFindLocation ) )
        return true;

    if( GetField( VALUE )->Matches( aSearchData, aAuxData, aFindLocation ) )
        return true;

    if( !( aSearchData.GetFlags() & FR_SEARCH_ALL_FIELDS ) )
        return false;

    for( size_t i = VALUE + 1; i < m_Fields.size(); i++ )
    {
        if( GetField( i )->Matches( aSearchData, aAuxData, aFindLocation ) )
            return true;
    }

    // Search for a match in pin name or pin number.
    // @TODO: see if the Matches method must be made in LIB_PIN.
    // when Matches method will be used in Libedit, this is the best
    // Currently, Pins are tested here.
    if( !( aSearchData.GetFlags() & FR_SEARCH_ALL_PINS ) )
        return false;

    if( Entry )
    {
        LIB_PIN_LIST pinList;
        Entry->GetPins( pinList, m_Multi, m_Convert );
        // Search for a match in pinList
        for( unsigned ii = 0; ii < pinList.size(); ii ++ )
        {
            LIB_PIN* pin = pinList[ii];
            wxString pinNum;
            pin->ReturnPinStringNum( pinNum );
            if( SCH_ITEM::Matches(pin->m_PinName, aSearchData ) ||
                SCH_ITEM::Matches(pinNum, aSearchData ) )
            {
                if( aFindLocation )
                {
                    wxPoint pinpos = pin->m_Pos;
                    pinpos = m_Transform.TransformCoordinate( pinpos );
                    *aFindLocation = pinpos + m_Pos;
                }
                return true;
            }

        }
    }

    return false;
}


void SCH_COMPONENT::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry == NULL )
        return;

    for( LIB_PIN* Pin = Entry->GetNextPin(); Pin != NULL; Pin = Entry->GetNextPin( Pin ) )
    {
        wxASSERT( Pin->Type() == COMPONENT_PIN_DRAW_TYPE );

        if( Pin->GetUnit() && m_Multi && ( m_Multi != Pin->GetUnit() ) )
            continue;

        if( Pin->GetConvert() && m_Convert && ( m_Convert != Pin->GetConvert() ) )
            continue;

        DANGLING_END_ITEM item( PIN_END, Pin );
        item.m_Pos = GetPinPhysicalPosition( Pin );
        aItemList.push_back( item );
    }
}


wxPoint SCH_COMPONENT::GetPinPhysicalPosition( LIB_PIN* Pin )
{
    wxCHECK_MSG( Pin != NULL && Pin->Type() == COMPONENT_PIN_DRAW_TYPE, wxPoint( 0, 0 ),
                 wxT( "Cannot get physical position of pin." ) );

    return m_Transform.TransformCoordinate( Pin->m_Pos ) + m_Pos;
}


bool SCH_COMPONENT::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    EDA_Rect boundingBox = GetBoundingBox();

    if( aRect.Intersects( boundingBox ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_COMPONENT::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    LIB_PIN* pin;
    LIB_COMPONENT* component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    wxCHECK_RET( component != NULL,
                 wxT( "Cannot add connection points to list.  Cannot find component <" ) +
                 m_ChipName + wxT( "> in any of the loaded libraries." ) );

    for( pin = component->GetNextPin(); pin != NULL; pin = component->GetNextPin( pin ) )
    {
        wxCHECK_RET( pin->Type() == COMPONENT_PIN_DRAW_TYPE,
                     wxT( "GetNextPin() did not return a pin object.  Bad programmer!" ) );

        // Skip items not used for this part.
        if( m_Multi && pin->GetUnit() && ( pin->GetUnit() != m_Multi ) )
            continue;
        if( m_Convert && pin->GetConvert() && ( pin->GetConvert() != m_Convert ) )
            continue;

        // Calculate the pin position relative to the component position and orientation.
        aPoints.push_back( m_Transform.TransformCoordinate( pin->m_Pos ) + m_Pos );
    }
}
