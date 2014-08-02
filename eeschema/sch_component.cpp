/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file sch_component.cpp
 * @brief Implementation of the class SCH_COMPONENT.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <gr_basic.h>
#include <trigo.h>
#include <kicad_string.h>
#include <richio.h>
#include <wxEeschemaStruct.h>
#include <plot_common.h>
#include <msgpanel.h>

#include <general.h>
#include <class_library.h>
#include <lib_rectangle.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <class_netlist_object.h>

#include <dialogs/dialog_schematic_find.h>

#include <wx/tokenzr.h>

#define NULL_STRING "_NONAME_"

static LIB_COMPONENT* DummyCmp;


/**
 * Function toUTFTildaText
 * convert a wxString to UTF8 and replace any control characters with a ~,
 * where a control character is one of the first ASCII values up to ' ' 32d.
 */
static std::string toUTFTildaText( const wxString& txt )
{
    std::string ret = TO_UTF8( txt );

    for( std::string::iterator it = ret.begin();  it!=ret.end();  ++it )
    {
        if( (unsigned char) *it <= ' ' )
            *it = '~';
    }
    return ret;
}


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

    Text->SetSize( wxSize( 150, 150 ) );
    Text->SetText( wxString( wxT( "??" ) ) );

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
    SetTimeStamp( GetNewTimeStamp() );

    if( setNewItemFlag )
        m_Flags = IS_NEW | IS_MOVED;

    // Import user defined fields from the library component:
    LIB_FIELDS libFields;

    libComponent.GetFields( libFields );

    for( LIB_FIELDS::iterator it = libFields.begin();  it!=libFields.end();  ++it )
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

        schField->SetTextPosition( m_Pos + it->GetTextPosition() );

        schField->ImportValues( *it );

        schField->SetText( it->GetText() );
    }

    wxString msg = libComponent.GetReferenceField().GetText();

    if( msg.IsEmpty() )
        msg = wxT( "U" );

    m_prefix = msg;

    // update the reference -- just the prefix for now.
    msg += wxT( "?" );
    SetRef( sheet, msg );

    /* Use the schematic component name instead of the library value field
     * name.
     */
    GetField( VALUE )->SetText( m_ChipName );
}


SCH_COMPONENT::SCH_COMPONENT( const SCH_COMPONENT& aComponent ) :
    SCH_ITEM( aComponent )
{
    m_Parent = aComponent.m_Parent;
    m_Pos = aComponent.m_Pos;
    m_unit = aComponent.m_unit;
    m_convert = aComponent.m_convert;
    m_ChipName = aComponent.m_ChipName;
    SetTimeStamp( aComponent.m_TimeStamp );
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

        if( i == REFERENCE )
            field.SetLayer( LAYER_REFERENCEPART );
        else if( i == VALUE )
            field.SetLayer( LAYER_VALUEPART );

        // else keep LAYER_FIELDS from SCH_FIELD constructor

        // SCH_FIELD's implicitly created copy constructor is called in here
        AddField( field );
    }

    m_prefix = wxString( _( "U" ) );
}


EDA_ITEM* SCH_COMPONENT::Clone() const
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

void SCH_COMPONENT::UpdateUnit( int aUnit )
{
    m_unit = aUnit;
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


int SCH_COMPONENT::GetPartCount() const
{
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry == NULL )
        return 0;

    return Entry->GetPartCount();
}



void SCH_COMPONENT::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                          GR_DRAWMODE DrawMode, EDA_COLOR_T Color, bool DrawPinText )
{
    bool           dummy = false;

    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry == NULL )
    {
        /* Create a dummy component if the actual component can not be found. */
        dummy = true;

        if( DummyCmp == NULL )
            CreateDummyCmp();

        Entry = DummyCmp;
    }

    Entry->Draw( panel, DC, m_Pos + offset, dummy ? 0 : m_unit, dummy ? 0 : m_convert,
                 DrawMode, Color, m_transform, DrawPinText, false );

    SCH_FIELD* field = GetField( REFERENCE );

    if( field->IsVisible() && !field->IsMoving() )
    {
        field->Draw( panel, DC, offset, DrawMode );
    }

    for( int ii = VALUE; ii < GetFieldCount(); ii++ )
    {
        field = GetField( ii );

        if( field->IsMoving() )
            continue;

        field->Draw( panel, DC, offset, DrawMode );
    }


#if 0
    /* Draw the component boundary box */
    {
        EDA_RECT BoundaryBox;
        BoundaryBox = GetBoundingBox();
        GRRect( panel->GetClipBox(), DC, BoundaryBox, 0, BROWN );
#if 1
        if( GetField( REFERENCE )->IsVisible() )
        {
            BoundaryBox = GetField( REFERENCE )->GetBoundingBox();
            GRRect( panel->GetClipBox(), DC, BoundaryBox, 0, BROWN );
        }

        if( GetField( VALUE )->IsVisible() )
        {
            BoundaryBox = GetField( VALUE )->GetBoundingBox();
            GRRect( panel->GetClipBox(), DC, BoundaryBox, 0, BROWN );
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


wxString SCH_COMPONENT::GetPath( const SCH_SHEET_PATH* sheet ) const
{
    wxCHECK_MSG( sheet != NULL, wxEmptyString,
                 wxT( "Cannot get component path with invalid sheet object." ) );

    wxString str;

    str.Printf( wxT( "%8.8lX" ), m_TimeStamp );
    return sheet->Path() + str;
}


const wxString SCH_COMPONENT::GetRef( const SCH_SHEET_PATH* sheet )
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
    if( !GetField( REFERENCE )->GetText().IsEmpty() )
    {
        SetRef( sheet, GetField( REFERENCE )->GetText() );
        return GetField( REFERENCE )->GetText();
    }

    return m_prefix;
}


/* Function IsReferenceStringValid (static function)
 * Tests for an acceptable reference string
 * An acceptable reference string must support unannotation
 * i.e starts by letter
 * returns true if OK
 */
bool SCH_COMPONENT::IsReferenceStringValid( const wxString& aReferenceString )
{
    wxString text = aReferenceString;
    bool ok = true;

    // Try to unannotate this reference
    while( !text.IsEmpty() && ( text.Last() == '?' || isdigit( text.Last() ) ) )
        text.RemoveLast();

    if( text.IsEmpty() )
        ok = false;

    // Add here other constraints
    // Currently:no other constraint

    return ok;
}


void SCH_COMPONENT::SetRef( const SCH_SHEET_PATH* sheet, const wxString& ref )
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

    if( rf->GetText().IsEmpty()
      || ( abs( rf->GetTextPosition().x - m_Pos.x ) +
           abs( rf->GetTextPosition().y - m_Pos.y ) > 10000 ) )
    {
        // move it to a reasonable position
        rf->SetTextPosition( m_Pos + wxPoint( 50, 50 ) );
    }

    rf->SetText( ref );  // for drawing.

    // Reinit the m_prefix member if needed
    wxString prefix = ref;

    if( IsReferenceStringValid( prefix ) )
    {
        while( prefix.Last() == '?' || isdigit( prefix.Last() ) )
            prefix.RemoveLast();
    }
    else
    {
        prefix = wxT( "U" );        // Set to default ref prefix
    }

    if( m_prefix != prefix )
        m_prefix = prefix;
}


void SCH_COMPONENT::SetTimeStamp( time_t aNewTimeStamp )
{
    wxString string_timestamp, string_oldtimestamp;

    string_timestamp.Printf( wxT( "%08lX" ), aNewTimeStamp );
    string_oldtimestamp.Printf( wxT( "%08lX" ), m_TimeStamp );
    EDA_ITEM::SetTimeStamp( aNewTimeStamp );

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
        if( aFieldName == m_Fields[i].GetName( false ) )
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


void SCH_COMPONENT::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( (aItem != NULL) && (aItem->Type() == SCH_COMPONENT_T),
                 wxT( "Cannot swap data with invalid component." ) );

    SCH_COMPONENT* component = (SCH_COMPONENT*) aItem;

    EXCHG( m_ChipName, component->m_ChipName );
    EXCHG( m_Pos, component->m_Pos );
    EXCHG( m_unit, component->m_unit );
    EXCHG( m_convert, component->m_convert );

    TRANSFORM tmp = m_transform;
    m_transform = component->m_transform;
    component->m_transform = tmp;

    m_Fields.swap( component->m_Fields );    // std::vector's swap()

    // Reparent items after copying data
    // (after swap(), m_Parent member does not point to the right parent):
    for( int ii = 0; ii < component->GetFieldCount();  ++ii )
    {
        component->GetField( ii )->SetParent( component );
    }

    for( int ii = 0; ii < GetFieldCount();  ++ii )
    {
        GetField( ii )->SetParent( this );
    }

    EXCHG( m_PathsAndReferences, component->m_PathsAndReferences );
}


void SCH_COMPONENT::ClearAnnotation( SCH_SHEET_PATH* aSheetPath )
{
    bool           keepMulti = false;
    LIB_COMPONENT* Entry;
    static const wxString separators( wxT( " " ) );
    wxArrayString  reference_fields;

    Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( Entry && Entry->UnitsLocked() )
        keepMulti = true;

    // Build a reference with no annotation,
    // i.e. a reference ended by only one '?'
    wxString defRef = m_prefix;

    if( IsReferenceStringValid( defRef ) )
    {
        while( defRef.Last() == '?' )
            defRef.RemoveLast();
    }
    else
    {   // This is a malformed reference: reinit this reference
        m_prefix = defRef = wxT("U");        // Set to default ref prefix
    }

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
    m_Fields[REFERENCE].SetText( defRef ); //for drawing.

    SetModified();
}


void SCH_COMPONENT::SetOrientation( int aOrientation )
{
    TRANSFORM temp = TRANSFORM();
    bool transform = false;

    switch( aOrientation )
    {
    case CMP_ORIENT_0:
    case CMP_NORMAL:                    // default transform matrix
        m_transform.x1 = 1;
        m_transform.y2 = -1;
        m_transform.x2 = m_transform.y1 = 0;
        break;

    case CMP_ROTATE_COUNTERCLOCKWISE:  // Rotate + (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = 1;
        temp.x2   = -1;
        transform = true;
        break;

    case CMP_ROTATE_CLOCKWISE:          // Rotate - (incremental rotation)
        temp.x1   = temp.y2 = 0;
        temp.y1   = -1;
        temp.x2   = 1;
        transform = true;
        break;

    case CMP_MIRROR_Y:                  // Mirror Y (incremental rotation)
        temp.x1   = -1;
        temp.y2   = 1;
        temp.y1   = temp.x2 = 0;
        transform = true;
        break;

    case CMP_MIRROR_X:                  // Mirror X (incremental rotation)
        temp.x1   = 1;
        temp.y2   = -1;
        temp.y1   = temp.x2 = 0;
        transform = true;
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
        transform = false;
        wxMessageBox( wxT( "SetRotateMiroir() error: ill value" ) );
        break;
    }

    if( transform )
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

void SCH_COMPONENT::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " ref=\"" << TO_UTF8( GetField( 0 )->GetName() )
                                 << '"' << " chipName=\""
                                 << TO_UTF8( m_ChipName ) << '"' << m_Pos
                                 << " layer=\"" << m_Layer
                                 << '"' << ">\n";

    // skip the reference, it's been output already.
    for( int i = 1; i < GetFieldCount();  ++i )
    {
        wxString value = GetField( i )->GetText();

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" << " name=\""
                                             << TO_UTF8( GetField( i )->GetName() )
                                             << '"' << " value=\""
                                             << TO_UTF8( value ) << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << TO_UTF8( GetClass().Lower() ) << ">\n";
}

#endif


bool SCH_COMPONENT::Save( FILE* f ) const
{
    std::string     name1;
    std::string     name2;
    wxArrayString   reference_fields;

    static wxString delimiters( wxT( " " ) );

    // this is redundant with the AR entries below, but it makes the
    // files backwards-compatible.
    if( m_PathsAndReferences.GetCount() > 0 )
    {
        reference_fields = wxStringTokenize( m_PathsAndReferences[0], delimiters );

        name1 = toUTFTildaText( reference_fields[1] );
    }
    else
    {
        if( GetField( REFERENCE )->GetText().IsEmpty() )
            name1 = toUTFTildaText( m_prefix );
        else
            name1 = toUTFTildaText( GetField( REFERENCE )->GetText() );
    }

    if( !m_ChipName.IsEmpty() )
    {
        name2 = toUTFTildaText( m_ChipName );
    }
    else
    {
        name2 = NULL_STRING;
    }

    if( fprintf( f, "$Comp\n" ) == EOF )
        return false;

    if( fprintf( f, "L %s %s\n", name2.c_str(), name1.c_str() ) == EOF )
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
     * this is useful for old Eeschema version compatibility
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
        fld->SetId( i );  // we don't need field Ids, please be gone.
    }

    // Fixed fields:
    // Save mandatory fields even if they are blank,
    // because the visibility, size and orientation are set from libary editor.
    for( unsigned i = 0;  i<MANDATORY_FIELDS;  ++i )
    {
        SCH_FIELD* fld = GetField( i );
        if( !fld->Save( f ) )
            return false;
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
    char        name1[256], name2[256],
                char1[256], char2[256], char3[256];
    int         newfmt = 0;
    char*       ptcar;
    wxString    fieldName;
    char*       line = aLine.Line();

    m_convert = 1;

    if( line[0] == '$' )
    {
        newfmt = 1;

        if( !(line = aLine.ReadLine()) )
            return true;
    }

    if( sscanf( &line[1], "%s %s", name1, name2 ) != 2 )
    {
        aErrorMsg.Printf( wxT( "Eeschema component description error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( line );
        return false;
    }

    if( strcmp( name1, NULL_STRING ) != 0 )
    {
        for( ii = 0; ii < (int) strlen( name1 ); ii++ )
        {
            if( name1[ii] == '~' )
                name1[ii] = ' ';
        }

        m_ChipName = FROM_UTF8( name1 );

        if( !newfmt )
            GetField( VALUE )->SetText( FROM_UTF8( name1 ) );
    }
    else
    {
        m_ChipName.Empty();
        GetField( VALUE )->Empty();
        GetField( VALUE )->SetOrientation( TEXT_ORIENT_HORIZ );
        GetField( VALUE )->SetVisible( false );
    }

    if( strcmp( name2, NULL_STRING ) != 0 )
    {
        bool isDigit = false;

        for( ii = 0; ii < (int) strlen( name2 ); ii++ )
        {
            if( name2[ii] == '~' )
                name2[ii] = ' ';

            // get RefBase from this, too. store in name1.
            if( name2[ii] >= '0' && name2[ii] <= '9' )
            {
                isDigit   = true;
                name1[ii] = 0;  //null-terminate.
            }

            if( !isDigit )
            {
                name1[ii] = name2[ii];
            }
        }

        name1[ii] = 0; //just in case
        int  jj;

        for( jj = 0; jj<ii && name1[jj] == ' '; jj++ )
            ;

        if( jj == ii )
        {
            // blank string.
            m_prefix = wxT( "U" );
        }
        else
        {
            m_prefix = FROM_UTF8( &name1[jj] );

            //printf("prefix: %s\n", TO_UTF8(component->m_prefix));
        }

        if( !newfmt )
            GetField( REFERENCE )->SetText( FROM_UTF8( name2 ) );
    }
    else
    {
        GetField( REFERENCE )->SetVisible( false );
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
        if( !(line = aLine.ReadLine()) )
            return false;

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
            for( int i = 0; i<GetFieldCount();  i++ )
            {
                if( GetField( i )->GetText().IsEmpty() )
                    GetField( i )->SetTextPosition( m_Pos );
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
            ii     = ReadDelimitedText( name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString path = FROM_UTF8( name1 );

            // copy the reference
            ii     = ReadDelimitedText( name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString ref = FROM_UTF8( name1 );

            // copy the multi, if exists
            ii = ReadDelimitedText( name1, ptcar, 255 );

            if( name1[0] == 0 )  // Nothing read, put a default value
                sprintf( name1, "%d", m_unit );

            int multi = atoi( name1 );

            if( multi < 0 || multi > 26 )
                multi = 1;

            AddHierarchicalReference( path, ref, multi );
            GetField( REFERENCE )->SetText( ref );
        }
        else if( line[0] == 'F' )
        {
            int  fieldNdx;

            wxString fieldText;
            EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_CENTER;
            EDA_TEXT_VJUSTIFY_T vjustify = GR_TEXT_VJUSTIFY_CENTER;

            ptcar = (char*) aLine;

            while( *ptcar && (*ptcar != '"') )
                ptcar++;

            if( *ptcar != '"' )
            {
                aErrorMsg.Printf( wxT( "Eeschema file library field F at line %d, aborted" ),
                                  aLine.LineNumber() );
                return false;
            }

            ptcar += ReadDelimitedText( &fieldText, ptcar );

            if( *ptcar == 0 )
            {
                aErrorMsg.Printf( wxT( "Component field F at line %d, aborted" ),
                                  aLine.LineNumber() );
                return false;
            }

            fieldNdx = atoi( line + 2 );

            ReadDelimitedText( &fieldName, ptcar );

            if( fieldName.IsEmpty() )
                fieldName = TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldNdx );

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
                GetField( fieldNdx )->SetName( fieldName );
            }

            GetField( fieldNdx )->SetText( fieldText );
            memset( char3, 0, sizeof(char3) );
            int x, y, w, attr;

            if( ( ii = sscanf( ptcar, "%s %d %d %d %X %s %s", char1, &x, &y, &w, &attr,
                               char2, char3 ) ) < 4 )
            {
                aErrorMsg.Printf( wxT( "Component Field error line %d, aborted" ),
                                  aLine.LineNumber() );
                continue;
            }

            GetField( fieldNdx )->SetTextPosition( wxPoint( x, y ) );
            GetField( fieldNdx )->SetAttributes( attr );

            if( (w == 0 ) || (ii == 4) )
                w = GetDefaultTextSize();

            GetField( fieldNdx )->SetSize( wxSize( w, w ) );
            GetField( fieldNdx )->SetOrientation( TEXT_ORIENT_HORIZ );

            if( char1[0] == 'V' )
                GetField( fieldNdx )->SetOrientation( TEXT_ORIENT_VERT );

            if( ii >= 7 )
            {
                if( *char2 == 'L' )
                    hjustify = GR_TEXT_HJUSTIFY_LEFT;
                else if( *char2 == 'R' )
                    hjustify = GR_TEXT_HJUSTIFY_RIGHT;

                if( char3[0] == 'B' )
                    vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                else if( char3[0] == 'T' )
                    vjustify = GR_TEXT_VJUSTIFY_TOP;

                GetField( fieldNdx )->SetItalic( char3[1] == 'I' );
                GetField( fieldNdx )->SetBold( char3[2] == 'B' );
                GetField( fieldNdx )->SetHorizJustify( hjustify );
                GetField( fieldNdx )->SetVertJustify( vjustify );
            }

            if( fieldNdx == REFERENCE )
                if( GetField( fieldNdx )->GetText()[0] == '#' )
                    GetField( fieldNdx )->SetVisible( false );
        }
        else
        {
            break;
        }
    }

    if( sscanf( line, "%d %d %d", &m_unit, &m_Pos.x, &m_Pos.y ) != 3 )
    {
        aErrorMsg.Printf( wxT( "Component unit & pos error at line %d, aborted" ),
                          aLine.LineNumber() );
        return false;
    }

    if( !(line = aLine.ReadLine()) ||
        sscanf( line, "%d %d %d %d",
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
        if( !(line = aLine.ReadLine()) )
            return false;

        if( strnicmp( "$End", line, 4 ) != 0 )
        {
            aErrorMsg.Printf( wxT( "Component End expected at line %d, aborted" ),
                              aLine.LineNumber() );
            return false;
        }
    }

    return true;
}


EDA_RECT SCH_COMPONENT::GetBodyBoundingBox() const
{
    LIB_COMPONENT* Entry = CMP_LIBRARY::FindLibraryComponent( m_ChipName );
    EDA_RECT       bBox;
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


const EDA_RECT SCH_COMPONENT::GetBoundingBox() const
{
    EDA_RECT bbox = GetBodyBoundingBox();
    for( size_t i = 0; i < m_Fields.size(); i++ )
    {
        bbox.Merge( m_Fields[i].GetBoundingBox() );
    }

    return bbox;
}


void SCH_COMPONENT::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    // search for the component in lib
    // Entry and root_component can differ if Entry is an alias
    LIB_ALIAS* alias = CMP_LIBRARY::FindLibraryEntry( m_ChipName );
    LIB_COMPONENT* root_component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( (alias == NULL) || (root_component == NULL) )
        return;

    wxString msg;

    if( m_currentSheetPath )
        aList.push_back( MSG_PANEL_ITEM( _( "Reference" ),
                                         GetRef( m_currentSheetPath ),
                                         DARKCYAN ) );

    if( root_component->IsPower() )
        msg = _( "Power symbol" );
    else
        msg = _( "Value" );

    aList.push_back( MSG_PANEL_ITEM( msg, GetField( VALUE )->GetText(), DARKCYAN ) );

    // Display component reference in library and library
    aList.push_back( MSG_PANEL_ITEM( _( "Component" ), m_ChipName, BROWN ) );

    if( alias->GetName() != root_component->GetName() )
        aList.push_back( MSG_PANEL_ITEM( _( "Alias of" ), root_component->GetName(), BROWN ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Library" ), alias->GetLibraryName(), BROWN ) );

    // Display the current associated footprin, if exists.
    if( ! GetField( FOOTPRINT )->IsVoid() )
        msg = GetField( FOOTPRINT )->GetText();
    else
        msg = _( "<Unknown>" );

    aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), msg, DARKRED ) );

    // Display description of the component, and keywords found in lib
    aList.push_back( MSG_PANEL_ITEM( _( "Description" ), alias->GetDescription(), DARKCYAN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Key words" ), alias->GetKeyWords(), DARKCYAN ) );
}


void SCH_COMPONENT::MirrorY( int aYaxis_position )
{
    int dx = m_Pos.x;

    SetOrientation( CMP_MIRROR_Y );
    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
    dx -= m_Pos.x;     // dx,0 is the move vector for this transform

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        // Move the fields to the new position because the component itself has moved.
        wxPoint pos = GetField( ii )->GetTextPosition();
        pos.x -= dx;
        GetField( ii )->SetTextPosition( pos );
    }
}


void SCH_COMPONENT::MirrorX( int aXaxis_position )
{
    int dy = m_Pos.y;

    SetOrientation( CMP_MIRROR_X );
    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
    dy -= m_Pos.y;     // dy,0 is the move vector for this transform

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        // Move the fields to the new position because the component itself has moved.
        wxPoint pos = GetField( ii )->GetTextPosition();
        pos.y -= dy;
        GetField( ii )->SetTextPosition( pos );
    }
}


void SCH_COMPONENT::Rotate( wxPoint aPosition )
{
    wxPoint prev = m_Pos;

    RotatePoint( &m_Pos, aPosition, 900 );

    SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        // Move the fields to the new position because the component itself has moved.
        wxPoint pos = GetField( ii )->GetTextPosition();
        pos.x -= prev.x - m_Pos.x;
        pos.y -= prev.y - m_Pos.y;
        GetField( ii )->SetTextPosition( pos );
    }
}


bool SCH_COMPONENT::Matches( wxFindReplaceData& aSearchData, void* aAuxData,
                             wxPoint* aFindLocation )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText() );

    // Components are searchable via the child field and pin item text.
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

        DANGLING_END_ITEM item( PIN_END, Pin, GetPinPhysicalPosition( Pin ) );
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

    EDA_RECT boundingBox = GetBoundingBox();

    if( aRect.Intersects( boundingBox ) )
        SetFlags( SELECTED );
    else
        ClearFlags( SELECTED );

    return previousState != IsSelected();
}


void SCH_COMPONENT::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
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


LIB_ITEM* SCH_COMPONENT::GetDrawItem( const wxPoint& aPosition, KICAD_T aType )
{
    LIB_COMPONENT* component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

    if( component == NULL )
        return NULL;

    // Calculate the position relative to the component.
    wxPoint libPosition = aPosition - m_Pos;

    return component->LocateDrawItem( m_unit, m_convert, aType, libPosition, m_transform );
}


wxString SCH_COMPONENT::GetSelectMenuText() const
{
    wxString tmp;
    tmp.Printf( _( "Component %s, %s" ),
                GetChars( m_ChipName ),
                GetChars( GetField( REFERENCE )->GetText() ) );
    return tmp;
}


SEARCH_RESULT SCH_COMPONENT::Visit( INSPECTOR* aInspector, const void* aTestData,
                                    const KICAD_T aFilterTypes[] )
{
    KICAD_T stype;
    LIB_COMPONENT* component;

    for( const KICAD_T* p = aFilterTypes; (stype = *p) != EOT; ++p )
    {
        // If caller wants to inspect component type or and component children types.
        if( stype == Type() )
        {
            if( SEARCH_QUIT == aInspector->Inspect( this, aTestData ) )
                return SEARCH_QUIT;
        }
        switch( stype )
        {
            case SCH_FIELD_T:
                // Test the bounding boxes of fields if they are visible and not empty.
                for( int ii = 0; ii < GetFieldCount(); ii++ )
                {
                    if( SEARCH_QUIT == aInspector->Inspect( GetField( ii ), (void*) this ) )
                        return SEARCH_QUIT;
                }
                break;

            case SCH_FIELD_LOCATE_REFERENCE_T:
                if( SEARCH_QUIT == aInspector->Inspect( GetField( REFERENCE ), (void*) this ) )
                    return SEARCH_QUIT;
                break;

            case SCH_FIELD_LOCATE_VALUE_T:
                if( SEARCH_QUIT == aInspector->Inspect( GetField( VALUE ), (void*) this ) )
                    return SEARCH_QUIT;
                break;

            case SCH_FIELD_LOCATE_FOOTPRINT_T:
                if( SEARCH_QUIT == aInspector->Inspect( GetField( FOOTPRINT ), (void*) this ) )
                    return SEARCH_QUIT;
                break;


            case LIB_PIN_T:
                component = CMP_LIBRARY::FindLibraryComponent( m_ChipName );

                if( component != NULL )
                {
                    LIB_PINS pins;
                    component->GetPins( pins, m_unit, m_convert );
                    for( size_t i = 0;  i < pins.size();  i++ )
                    {
                        if( SEARCH_QUIT == aInspector->Inspect( pins[ i ], (void*) this ) )
                            return SEARCH_QUIT;
                    }
                }
                break;

            default:
                break;
        }
    }

    return SEARCH_CONTINUE;
}


void SCH_COMPONENT::GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                                    SCH_SHEET_PATH*      aSheetPath )
{
    LIB_COMPONENT* component = CMP_LIBRARY::FindLibraryComponent( GetLibName() );

    if( component == NULL )
        return;

    for( LIB_PIN* pin = component->GetNextPin();  pin;  pin = component->GetNextPin( pin ) )
    {
        wxASSERT( pin->Type() == LIB_PIN_T );

        if( pin->GetUnit() && ( pin->GetUnit() != GetUnitSelection( aSheetPath ) ) )
            continue;

        if( pin->GetConvert() && ( pin->GetConvert() != GetConvert() ) )
            continue;

        wxPoint pos = GetTransform().TransformCoordinate( pin->GetPosition() ) + m_Pos;

        NETLIST_OBJECT* item = new NETLIST_OBJECT();
        item->m_SheetPathInclude = *aSheetPath;
        item->m_Comp = (SCH_ITEM*) pin;
        item->m_SheetPath = *aSheetPath;
        item->m_Type = NET_PIN;
        item->m_Link = (SCH_ITEM*) this;
        item->m_ElectricalType = pin->GetType();
        item->m_PinNum = pin->GetNumber();
        item->m_Label = pin->GetName();
        item->m_Start = item->m_End = pos;

        aNetListItems.push_back( item );

        if( ( (int) pin->GetType() == (int) PIN_POWER_IN ) && !pin->IsVisible() )
        {
            /* There is an associated PIN_LABEL. */
            item = new NETLIST_OBJECT();
            item->m_SheetPathInclude = *aSheetPath;
            item->m_Comp = NULL;
            item->m_SheetPath = *aSheetPath;
            item->m_Type  = NET_PINLABEL;
            item->m_Label = pin->GetName();
            item->m_Start = pos;
            item->m_End = item->m_Start;

            aNetListItems.push_back( item );
        }
    }
}


bool SCH_COMPONENT::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    SCH_COMPONENT* component = (SCH_COMPONENT*) &aItem;

    EDA_RECT rect = GetBodyBoundingBox();

    if( rect.GetArea() != component->GetBodyBoundingBox().GetArea() )
        return rect.GetArea() < component->GetBodyBoundingBox().GetArea();

    if( m_Pos.x != component->m_Pos.x )
        return m_Pos.x < component->m_Pos.x;

    if( m_Pos.y != component->m_Pos.y )
        return m_Pos.y < component->m_Pos.y;

    return false;
}


bool SCH_COMPONENT::operator==( const SCH_COMPONENT& aComponent ) const
{
    if( GetFieldCount() !=  aComponent.GetFieldCount() )
        return false;

    for( int i = VALUE; i < GetFieldCount(); i++ )
    {
        if( GetField( i )->GetText().Cmp( aComponent.GetField( i )->GetText() ) != 0 )
            return false;
    }

    return true;
}


bool SCH_COMPONENT::operator!=( const SCH_COMPONENT& aComponent ) const
{
    return !( *this == aComponent );
}


SCH_ITEM& SCH_COMPONENT::operator=( const SCH_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " ) +
                 GetClass() );

    if( &aItem != this )
    {
        SCH_ITEM::operator=( aItem );

        SCH_COMPONENT* component = (SCH_COMPONENT*) &aItem;
        m_ChipName = component->m_ChipName;
        m_Pos = component->m_Pos;
        m_unit = component->m_unit;
        m_convert = component->m_convert;
        m_transform = component->m_transform;
        m_PathsAndReferences = component->m_PathsAndReferences;

        m_Fields = component->m_Fields;    // std::vector's assignment operator.

        // Reparent fields after assignment to new component.
        for( int ii = 0; ii < GetFieldCount();  ++ii )
        {
            GetField( ii )->SetParent( this );
        }
    }

    return *this;
}


bool SCH_COMPONENT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bBox = GetBodyBoundingBox();
    bBox.Inflate( aAccuracy );

    if( bBox.Contains( aPosition ) )
        return true;

    return false;
}


bool SCH_COMPONENT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_Flags & STRUCT_DELETED || m_Flags & SKIP_STRUCT )
        return false;

    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBodyBoundingBox() );

    return rect.Intersects( GetBodyBoundingBox() );
}


bool SCH_COMPONENT::doIsConnected( const wxPoint& aPosition ) const
{
    std::vector< wxPoint > pts;

    GetConnectionPoints( pts );

    for( size_t i = 0;  i < pts.size();  i++ )
    {
        if( pts[i] == aPosition )
            return true;
    }

    return false;
}

/* return true if the component is in netlist
 * which means this is not a power component, or something
 * like a component reference starting by #
 */
bool SCH_COMPONENT::IsInNetlist() const
{
    SCH_FIELD* rf = GetField( REFERENCE );
    return ! rf->GetText().StartsWith( wxT( "#" ) );
}


void SCH_COMPONENT::Plot( PLOTTER* aPlotter )
{
    LIB_COMPONENT* Entry;
    TRANSFORM temp = TRANSFORM();

    Entry = CMP_LIBRARY::FindLibraryComponent( GetLibName() );

    if( Entry == NULL )
        return;

    temp = GetTransform();

    Entry->Plot( aPlotter, GetUnit(), GetConvert(), m_Pos, temp );

    for( size_t i = 0; i < m_Fields.size(); i++ )
    {
        m_Fields[i].Plot( aPlotter );
    }
}
