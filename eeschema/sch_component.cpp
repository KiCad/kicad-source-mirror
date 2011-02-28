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
    SCH_ITEM( aParent, SCH_COMPONENT_T )
{
    Init( aPos );
}


SCH_COMPONENT::SCH_COMPONENT( LIB_COMPONENT& libComponent, SCH_SHEET_PATH* sheet, int unit,
                              int convert, const wxPoint& pos, bool setNewItemFlag ) :
    SCH_ITEM( NULL, SCH_COMPONENT_T )
{
    Init( pos );

    m_unit      = unit;
    m_convert   = convert;
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
        if( it->GetName().IsEmpty() )
            continue;

        // See if field by same name already exists.
        SCH_FIELD* schField = FindField( it->GetName() );

        if( !schField )
        {
            SCH_FIELD fld( wxPoint( 0, 0 ), GetFieldCount(), this, it->GetName() );
            schField = AddField( fld );
        }

        schField->m_Pos = m_Pos + it->m_Pos;

        schField->ImportValues( *it );

        schField->m_Text = it->m_Text;
    }

    wxString msg = libComponent.GetReferenceField().m_Text;

    if( msg.IsEmpty() )
        msg = wxT( "U" );

    m_prefix = msg;

    // update the reference -- just the prefix for now.
    msg += wxT( "?" );
    SetRef( sheet, msg );

    /* Use the schematic component name instead of the library value field
     * name.
     */
    GetField( VALUE )->m_Text = m_ChipName;
}


SCH_COMPONENT::SCH_COMPONENT( const SCH_COMPONENT& aComponent ) :
    SCH_ITEM( aComponent )
{
    m_Parent = aComponent.m_Parent;
    m_Pos = aComponent.m_Pos;
    m_unit = aComponent.m_unit;
    m_convert = aComponent.m_convert;
    m_ChipName = aComponent.m_ChipName;
    m_TimeStamp = aComponent.m_TimeStamp;
    m_transform = aComponent.m_transform;
    m_prefix = aComponent.m_prefix;
    m_PathsAndReferences = aComponent.m_PathsAndReferences;
    m_Fields = aComponent.m_Fields;

    // Re-parent the fields, which before this had aComponent as parent
    for( int i = 0; i<GetFieldCount(); ++i )
    {
        GetField( i )->SetParent( this );
    }
}


void SCH_COMPONENT::Init( const wxPoint& pos )
{
    m_Pos     = pos;
    m_unit    = 0;  // In multi unit chip - which unit to draw.
    m_convert = 0;  // De Morgan Handling

    // The rotation/mirror transformation matrix. pos normal
    m_transform = TRANSFORM();

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

    m_prefix = wxString( _( "U" ) );
}


EDA_ITEM* SCH_COMPONENT::doClone() const
{
    return new SCH_COMPONENT( *this );
}


void SCH_COMPONENT::SetLibName( const wxString& aName )
{
    if( m_ChipName != aName )
    {
        m_ChipName = aName;
        SetModified();
    }
}


void SCH_COMPONENT::SetUnit( int aUnit )
{
    if( m_unit != aUnit )
    {
        m_unit = aUnit;
        SetModified();
    }
}


void SCH_COMPONENT::SetConvert( int aConvert )
{
    if( m_convert != aConvert )
    {
        m_convert = aConvert;
        SetModified();
    }
}


void SCH_COMPONENT::SetTransform( const TRANSFORM& aTransform )
{
    if( m_transform != aTransform )
    {
        m_transform = aTransform;
        SetModified();
    }
}


void SCH_COMPONENT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                          int DrawMode, int Color, bool DrawPinText )
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

    Entry->Draw( panel, DC, m_Pos + offset, dummy ? 0 : m_unit, dummy ? 0 : m_convert,
                 DrawMode, Color, m_transform, DrawPinText, false );

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
        BoundaryBox = GetBoundingBox();
        GRRect( &panel->m_ClipBox, DC, BoundaryBox, 0, BROWN );
#if 1
        if( GetField( REFERENCE )->IsVisible() )
        {
            BoundaryBox = GetField( REFERENCE )->GetBoundingBox();
            GRRect( &panel->m_ClipBox, DC, BoundaryBox, 0, BROWN );
        }

        if( GetField( VALUE )->IsVisible() )
        {
            BoundaryBox = GetField( VALUE )->GetBoundingBox();
            GRRect( &panel->m_ClipBox, DC, BoundaryBox, 0, BROWN );
        }
#endif
    }
#endif
}


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
             *       TO_UTF8( m_PathsAndReferences[ii] ) ); */
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
    return m_prefix;
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
        AddHierarchicalReference( path, ref, m_unit );

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

    // Reinit the m_prefix member if needed
    wxString prefix = ref;

    while( prefix.Last() == '?' or isdigit( prefix.Last() ) )
        prefix.RemoveLast();

    if( m_prefix != prefix )
        m_prefix = prefix;
}


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

    // if it was not found in m_Paths array, then use m_unit.
    // this will happen if we load a version 1 schematic file.
    return m_unit;
}


void SCH_COMPONENT::SetUnitSelection( SCH_SHEET_PATH* aSheet, int aUnitSelection )
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
        AddHierarchicalReference( path, m_prefix, aUnitSelection );
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

    return Entry->GetPin( number, m_unit, m_convert );
}


void SCH_COMPONENT::SwapData( SCH_COMPONENT* copyitem )
{
    EXCHG( m_ChipName, copyitem->m_ChipName );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_unit, copyitem->m_unit );
    EXCHG( m_convert, copyitem->m_convert );

    TRANSFORM tmp = m_transform;
    m_transform = copyitem->m_transform;
    copyitem->m_transform = tmp;

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


void SCH_COMPONENT::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    /* save old text in undo list */
    if( g_ItemToUndoCopy && ( g_ItemToUndoCopy->Type() == Type() ) && !IsNew() )
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


void SCH_COMPONENT::ClearAnnotation( SCH_SHEET_PATH* aSheetPath )
{
    wxString       defRef    = m_prefix;
    bool           keepMulti = false;
    LIB_COMPONENT* Entry;
    static const wxString separators( wxT( " " ) );
    wxArrayString  reference_fields;

    Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry && Entry->UnitsLocked() )
        keepMulti = true;

    while( defRef.Last() == '?' )
        defRef.RemoveLast();

    defRef.Append( wxT( "?" ) );

    wxString multi = wxT( "1" );

    // For components with units locked,
    // we cannot remove all annotations: part selection must be kept
    // For all components: if aSheetPath is not NULL,
    // remove annotation only for the given path
    if( keepMulti || aSheetPath )
    {
        wxString NewHref;
        wxString path;

        if( aSheetPath )
            path = GetPath( aSheetPath );

        for( unsigned int ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
        {
            // Break hierarchical reference in path, ref and multi selection:
            reference_fields = wxStringTokenize( m_PathsAndReferences[ii], separators );

            if( aSheetPath == NULL || reference_fields[0].Cmp( path ) == 0 )
            {
                if( keepMulti )  // Get and keep part selection
                    multi = reference_fields[2];

                NewHref = reference_fields[0];
                NewHref << wxT( " " ) << defRef << wxT( " " ) << multi;
                m_PathsAndReferences[ii] = NewHref;
            }
        }
    }
    else
    {
        // Clear reference strings, but does not free memory because a new annotation
        // will reuse it
        m_PathsAndReferences.Empty();
        m_unit = 1;
    }

    // These 2 changes do not work in complex hierarchy.
    // When a clear annotation is made, the calling function must call a
    // UpdateAllScreenReferences for the active sheet.
    // But this call cannot made here.
    m_Fields[REFERENCE].m_Text = defRef; //for drawing.

    SetModified();
}


void SCH_COMPONENT::SetOrientation( int aOrientation )
{

    TRANSFORM temp = TRANSFORM();
    bool Transform = false;

    switch( aOrientation )
    {
    case CMP_ORIENT_0:
    case CMP_NORMAL:                    // default transform matrix
        m_transform.x1 = 1;
        m_transform.y2 = -1;
        m_transform.x2 = m_transform.y1 = 0;
        break;

    case CMP_ROTATE_CLOCKWISE:            // Rotate + (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = 1;
        temp.x2   = -1;
        Transform = true;
        break;

    case CMP_ROTATE_COUNTERCLOCKWISE:    // Rotate - (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = -1;
        temp.x2   = 1;
        Transform = true;
        break;

    case CMP_MIRROR_Y:                  // Mirror Y (incremental rotation)
        temp.x1   = -1;
        temp.y2   = 1;
        temp.y1   = temp.x2 = 0;
        Transform = true;
        break;

    case CMP_MIRROR_X:                  // Mirror X (incremental rotation)
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
         *  requested transformation, which is the temp transform (rot,
         *  mirror ..) in order to have (in term of matrix transform):
         *     transform coord = new_m_transform * coord
         *  where transform coord is the coord modified by new_m_transform from
         *  the initial value coord.
         *  new_m_transform is computed (from old_m_transform and temp) to
         *  have:
         *     transform coord = old_m_transform * temp
         */
        TRANSFORM newTransform;

        newTransform.x1 = m_transform.x1 * temp.x1 + m_transform.x2 * temp.y1;
        newTransform.y1 = m_transform.y1 * temp.x1 + m_transform.y2 * temp.y1;
        newTransform.x2 = m_transform.x1 * temp.x2 + m_transform.x2 * temp.y2;
        newTransform.y2 = m_transform.y1 * temp.x2 + m_transform.y2 * temp.y2;
        m_transform = newTransform;
    }
}


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
    transform = m_transform;

    for( ii = 0; ii < ROTATE_VALUES_COUNT; ii++ )
    {
        type_rotate = rotate_value[ii];
        SetOrientation( type_rotate );

        if( transform == m_transform )
            return type_rotate;
    }

    // Error: orientation not found in list (should not happen)
    wxMessageBox( wxT( "Component orientation matrix internal error" ) );
    m_transform = transform;

    return CMP_NORMAL;
}


wxPoint SCH_COMPONENT::GetScreenCoord( const wxPoint& aPoint )
{
    return m_transform.TransformCoordinate( aPoint );
}


#if defined(DEBUG)

void SCH_COMPONENT::Show( int nestLevel, std::ostream& os )
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " ref=\"" << TO_UTF8( ReturnFieldName( 0 ) )
                                 << '"' << " chipName=\""
                                 << TO_UTF8( m_ChipName ) << '"' << m_Pos
                                 << " layer=\"" << m_Layer
                                 << '"' << ">\n";

    // skip the reference, it's been output already.
    for( int i = 1; i < GetFieldCount();  ++i )
    {
        wxString value = GetField( i )->m_Text;

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" << " name=\""
                                             << TO_UTF8( ReturnFieldName( i ) )
                                             << '"' << " value=\""
                                             << TO_UTF8( value ) << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << TO_UTF8( GetClass().Lower() ) << ">\n";
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
        strncpy( Name1, TO_UTF8( reference_fields[1] ), sizeof( Name1 ) );
    }
    else
    {
        if( GetField( REFERENCE )->m_Text.IsEmpty() )
            strncpy( Name1, TO_UTF8( m_prefix ), sizeof( Name1 ) );
        else
            strncpy( Name1, TO_UTF8( GetField( REFERENCE )->m_Text ), sizeof( Name1 ) );
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
        strncpy( Name2, TO_UTF8( m_ChipName ), sizeof( Name2 ) );
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
    if( fprintf( f, "U %d %d %8.8lX\n", m_unit, m_convert, m_TimeStamp ) == EOF )
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
            reference_fields = wxStringTokenize( m_PathsAndReferences[ii], delimiters );

            if( fprintf( f, "AR Path=\"%s\" Ref=\"%s\"  Part=\"%s\" \n",
                         TO_UTF8( reference_fields[0] ),
                         TO_UTF8( reference_fields[1] ),
                         TO_UTF8( reference_fields[2] ) ) == EOF )
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
    if( fprintf( f, "\t%-4d %-4d %-4d\n", m_unit, m_Pos.x, m_Pos.y ) == EOF )
        return false;

    if( fprintf( f, "\t%-4d %-4d %-4d %-4d\n",
                 m_transform.x1, m_transform.y1, m_transform.x2, m_transform.y2 ) == EOF )
        return false;

    if( fprintf( f, "$EndComp\n" ) == EOF )
        return false;

    return true;
}


bool SCH_COMPONENT::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int         ii;
    char        Name1[256], Name2[256],
                Char1[256], Char2[256], Char3[256];
    int         newfmt = 0;
    char*       ptcar;
    wxString    fieldName;
    char*       line = aLine.Line();

    m_convert = 1;

    if( line[0] == '$' )
    {
        newfmt = 1;

        if( !aLine.ReadLine() )
            return TRUE;

        line = aLine.Line();
    }

    if( sscanf( &line[1], "%s %s", Name1, Name2 ) != 2 )
    {
        aErrorMsg.Printf( wxT( "EESchema Component descr error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( line );
        return false;
    }

    if( strcmp( Name1, NULL_STRING ) != 0 )
    {
        for( ii = 0; ii < (int) strlen( Name1 ); ii++ )
            if( Name1[ii] == '~' )
                Name1[ii] = ' ';

        m_ChipName = FROM_UTF8( Name1 );

        if( !newfmt )
            GetField( VALUE )->m_Text = FROM_UTF8( Name1 );
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
            m_prefix = wxT( "U" );
        }
        else
        {
            m_prefix = FROM_UTF8( &Name1[jj] );

            //printf("prefix: %s\n", TO_UTF8(component->m_prefix));
        }

        if( !newfmt )
            GetField( REFERENCE )->m_Text = FROM_UTF8( Name2 );
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

        line = aLine.Line();

        if( line[0] == 'U' )
        {
            sscanf( line + 1, "%d %d %lX", &m_unit, &m_convert, &m_TimeStamp );
        }
        else if( line[0] == 'P' )
        {
            sscanf( line + 1, "%d %d", &m_Pos.x, &m_Pos.y );

            // Set fields position to a default position (that is the
            // component position.  For existing fields, the real position
            // will be set later
            for( int i = 0; i<GetFieldCount();  ++i )
            {
                if( GetField( i )->m_Text.IsEmpty() )
                    GetField( i )->m_Pos = m_Pos;
            }
        }
        else if( line[0] == 'A' && line[1] == 'R' )
        {
            /* format:
             * AR Path="/9086AF6E/67452AA0" Ref="C99" Part="1"
             * where 9086AF6E is the unique timestamp of the containing sheet
             * and 67452AA0 is the timestamp of this component.
             * C99 is the reference given this path.
             */
            int ii;
            ptcar = line + 2;

            //copy the path.
            ii     = ReadDelimitedText( Name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString path = FROM_UTF8( Name1 );

            // copy the reference
            ii     = ReadDelimitedText( Name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString ref = FROM_UTF8( Name1 );

            // copy the multi, if exists
            ii = ReadDelimitedText( Name1, ptcar, 255 );
            if( Name1[0] == 0 )  // Nothing read, put a default value
                sprintf( Name1, "%d", m_unit );
            int multi = atoi( Name1 );
            if( multi < 0 || multi > 25 )
                multi = 1;
            AddHierarchicalReference( path, ref, multi );
            GetField( REFERENCE )->m_Text = ref;
        }
        else if( line[0] == 'F' )
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

            fieldNdx = atoi( line + 2 );

            ReadDelimitedText( FieldUserName, ptcar, sizeof(FieldUserName) );

            if( !FieldUserName[0] )
                fieldName = TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldNdx );
            else
                fieldName = FROM_UTF8( FieldUserName );

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

            GetField( fieldNdx )->m_Text = FROM_UTF8( Name1 );
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

    if( sscanf( line, "%d %d %d", &m_unit, &m_Pos.x, &m_Pos.y ) != 3 )
    {
        aErrorMsg.Printf( wxT( "Component unit & pos error at line %d, aborted" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !aLine.ReadLine() ||
        sscanf( ((char*)aLine), "%d %d %d %d",
                &m_transform.x1,
                &m_transform.y1,
                &m_transform.x2,
                &m_transform.y2 ) != 4 )
    {
        aErrorMsg.Printf( wxT( "Component orient error at line %d, aborted" ),
                          aLine.LineNumber() );
        return false;
    }

    if( newfmt )
    {
        if( !aLine.ReadLine() )
            return false;

        line = aLine.Line();

        if( strnicmp( "$End", line, 4 ) != 0 )
        {
            aErrorMsg.Printf( wxT( "Component End expected at line %d, aborted" ),
                              aLine.LineNumber() );
            return false;
        }
    }

    return true;
}


EDA_Rect SCH_COMPONENT::GetBodyBoundingBox() const
{
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );
    EDA_Rect       bBox;
    int            x0, xm, y0, ym;

    if( Entry == NULL )
    {
        if( DummyCmp == NULL )
            CreateDummyCmp();
        Entry = DummyCmp;
    }

    /* Get the basic Boundary box */
    bBox = Entry->GetBodyBoundingBox( m_unit, m_convert );
    x0 = bBox.GetX();
    xm = bBox.GetRight();

    // We must reverse Y values, because matrix orientation
    // suppose Y axis normal for the library items coordinates,
    // m_transform reverse Y values, but bBox is already reversed!
    y0 = -bBox.GetY();
    ym = -bBox.GetBottom();

    /* Compute the real Boundary box (rotated, mirrored ...)*/
    int x1 = m_transform.x1 * x0 + m_transform.y1 * y0;
    int y1 = m_transform.x2 * x0 + m_transform.y2 * y0;
    int x2 = m_transform.x1 * xm + m_transform.y1 * ym;
    int y2 = m_transform.x2 * xm + m_transform.y2 * ym;

    // H and W must be > 0:
    if( x2 < x1 )
        EXCHG( x2, x1 );

    if( y2 < y1 )
        EXCHG( y2, y1 );

    bBox.SetX( x1 );
    bBox.SetY( y1 );
    bBox.SetWidth( x2 - x1 );
    bBox.SetHeight( y2 - y1 );

    bBox.Offset( m_Pos );
    return bBox;
}


EDA_Rect SCH_COMPONENT::GetBoundingBox() const
{
    EDA_Rect bBox = GetBodyBoundingBox();

    // Include BoundingBoxes of fields if they are visible and not empty.
    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        if( !GetField( ii )->IsVisible() || GetField( ii )->IsVoid() )
            continue;

        bBox.Merge( GetField( ii )->GetBoundingBox() );
    }

    return bBox;
}


void SCH_COMPONENT::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    // search for the component in lib
    // Entry and root_component can differ if Entry is an alias
    LIB_ALIAS* alias = CMP_LIBRARY::FindLibraryEntry( m_ChipName );
    LIB_COMPONENT* root_component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( (alias == NULL) || (root_component == NULL) )
        return;

    wxString msg;

    frame->ClearMsgPanel();

    frame->AppendMsgPanel( _( "Reference" ), GetRef( ( (SCH_EDIT_FRAME*) frame )->GetSheet() ),
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


bool SCH_COMPONENT::Matches( wxFindReplaceData& aSearchData, void* aAuxData,
                             wxPoint* aFindLocation )
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
        Entry->GetPins( pinList, m_unit, m_convert );

        // Search for a match in pinList
        for( unsigned ii = 0; ii < pinList.size(); ii ++ )
        {
            LIB_PIN* pin = pinList[ii];
            wxString pinNum;
            pin->ReturnPinStringNum( pinNum );

            if( SCH_ITEM::Matches( pin->GetName(), aSearchData ) ||
                SCH_ITEM::Matches( pinNum, aSearchData ) )
            {
                if( aFindLocation )
                {
                    wxPoint pinpos = pin->GetPosition();
                    pinpos = m_transform.TransformCoordinate( pinpos );
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
        wxASSERT( Pin->Type() == LIB_PIN_T );

        if( Pin->GetUnit() && m_unit && ( m_unit != Pin->GetUnit() ) )
            continue;

        if( Pin->GetConvert() && m_convert && ( m_convert != Pin->GetConvert() ) )
            continue;

        DANGLING_END_ITEM item( PIN_END, Pin );
        item.m_Pos = GetPinPhysicalPosition( Pin );
        aItemList.push_back( item );
    }
}


wxPoint SCH_COMPONENT::GetPinPhysicalPosition( LIB_PIN* Pin )
{
    wxCHECK_MSG( Pin != NULL && Pin->Type() == LIB_PIN_T, wxPoint( 0, 0 ),
                 wxT( "Cannot get physical position of pin." ) );

    return m_transform.TransformCoordinate( Pin->GetPosition() ) + m_Pos;
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
        wxCHECK_RET( pin->Type() == LIB_PIN_T,
                     wxT( "GetNextPin() did not return a pin object.  Bad programmer!" ) );

        // Skip items not used for this part.
        if( m_unit && pin->GetUnit() && ( pin->GetUnit() != m_unit ) )
            continue;

        if( m_convert && pin->GetConvert() && ( pin->GetConvert() != m_convert ) )
            continue;

        // Calculate the pin position relative to the component position and orientation.
        aPoints.push_back( m_transform.TransformCoordinate( pin->GetPosition() ) + m_Pos );
    }
}


LIB_DRAW_ITEM* SCH_COMPONENT::GetDrawItem( const wxPoint& aPosition, KICAD_T aType )
{
    LIB_COMPONENT* component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( component == NULL )
        return NULL;

    // Calculate the position relative to the component.
    wxPoint libPosition = aPosition - m_Pos;

    return component->LocateDrawItem( m_unit, m_convert, aType, libPosition, m_transform );
}


bool SCH_COMPONENT::doHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    EDA_Rect bBox;

    if( aFilter & FIELD_T )
    {
        // Test the bounding boxes of fields if they are visible and not empty.
        for( int ii = 0; ii < GetFieldCount(); ii++ )
        {
            if( !GetField( ii )->IsVisible() || GetField( ii )->IsVoid() )
                continue;

            bBox = GetField( ii )->GetBoundingBox();
            bBox.Inflate( aAccuracy );

            if( bBox.Contains( aPoint ) )
                return true;
        }
    }

    if( aFilter & COMPONENT_T )
    {
        bBox = GetBodyBoundingBox();
        bBox.Inflate( aAccuracy );

        if( bBox.Contains( aPoint ) )
            return true;
    }

    return false;
}


bool SCH_COMPONENT::doHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_COMPONENT::doIsConnected( const wxPoint& aPosition ) const
{
    vector< wxPoint > pts;

    GetConnectionPoints( pts );

    for( size_t i = 0;  i < pts.size();  i++ )
    {
        if( pts[i] == aPosition )
            return true;
    }

    return false;
}
