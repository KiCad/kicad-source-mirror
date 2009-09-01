/**************************************************************/
/* class_sch_component.cpp : handle the  class SCH_COMPONENT  */
/**************************************************************/

#include "fctsys.h"
#include "class_drawpanel.h"
#include "gr_basic.h"
#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "macros.h"

#include "protos.h"

#include <wx/tokenzr.h>


/* Local variables */
static EDA_LibComponentStruct* DummyCmp;

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
    DummyCmp = new EDA_LibComponentStruct( NULL );

    LibDrawSquare* Square = new LibDrawSquare(DummyCmp);

    Square->m_Pos   = wxPoint( -200, 200 );
    Square->m_End   = wxPoint( 200, -200 );

    LibDrawText* Text = new LibDrawText(DummyCmp);

    Text->m_Size.x = Text->m_Size.y = 150;
    Text->m_Text   = wxT( "??" );

    DummyCmp->m_Drawings = Square;
    Square->SetNext( Text );
}


/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as
* specified, and in the given drawing mode.
* if Color < 0: Draw in normal color
* else draw  in color = Color
*****************************************************************************/
/* DrawMode  = GrXOR, GrOR ..*/
void DrawLibPartAux( WinEDA_DrawPanel* panel, wxDC* DC,
                     SCH_COMPONENT* Component,
                     EDA_LibComponentStruct* Entry,
                     const wxPoint& Pos, const int TransMat[2][2],
                     int Multi, int convert, int DrawMode,
                     int Color, bool DrawPinText )
{
    wxPoint            pos1, pos2;
    bool               force_nofill;
    LibEDA_BaseStruct* DEntry;
    BASE_SCREEN*       screen = panel->GetScreen();

    if( Entry->m_Drawings == NULL )
        return;
    GRSetDrawMode( DC, DrawMode );

    for( DEntry = Entry->m_Drawings; DEntry != NULL; DEntry = DEntry->Next() )
    {
        /* Do not draw items not attached to the current part */
        if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) )
            continue;

        if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
            continue;

        // Do not draw an item while moving (the cursor handler does that)
        if( DEntry->m_Flags & IS_MOVED )
            continue;

        force_nofill = false;

        switch( DEntry->Type() )
        {
        case COMPONENT_PIN_DRAW_TYPE:
        {
            DrawPinPrms prms( Entry, DrawPinText );
            DEntry->Draw( panel, DC, Pos, Color, DrawMode, &prms, TransMat );
        }
        break;

        case COMPONENT_ARC_DRAW_TYPE:
        case COMPONENT_CIRCLE_DRAW_TYPE:
        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        case COMPONENT_RECT_DRAW_TYPE:
        case COMPONENT_POLYLINE_DRAW_TYPE:
        default:
            if( screen->m_IsPrinting
                && DEntry->m_Fill == FILLED_WITH_BG_BODYCOLOR
                && GetGRForceBlackPenState() )
                force_nofill = true;
            DEntry->Draw( panel, DC, Pos, Color, DrawMode, (void*) force_nofill,
                          TransMat );
            break;
        }
    }

    if( g_DebugLevel > 4 ) /* Draw the component boundary box */
    {
        EDA_Rect BoundaryBox;
        if( Component )
            BoundaryBox = Component->GetBoundaryBox();
        else
            BoundaryBox = Entry->GetBoundaryBox( Multi, convert );
        int x1 = BoundaryBox.GetX();
        int y1 = BoundaryBox.GetY();
        int x2 = BoundaryBox.GetRight();
        int y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
        BoundaryBox = Component->GetField( REFERENCE )->GetBoundaryBox();
        x1 = BoundaryBox.GetX();
        y1 = BoundaryBox.GetY();
        x2 = BoundaryBox.GetRight();
        y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
        BoundaryBox = Component->GetField( VALUE )->GetBoundaryBox();
        x1 = BoundaryBox.GetX();
        y1 = BoundaryBox.GetY();
        x2 = BoundaryBox.GetRight();
        y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
    }
}



/*******************************************************************/
SCH_COMPONENT::SCH_COMPONENT( const wxPoint& aPos, SCH_ITEM* aParent ) :
    SCH_ITEM( aParent, TYPE_SCH_COMPONENT )
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

    for( int i = 0; i < NUMBER_OF_FIELDS; ++i )
    {
        SCH_CMP_FIELD field( aPos, i, this, ReturnDefaultFieldName( i ) );

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
    for( int i=0; i<GetFieldCount(); ++i )
    {
        GetField( i )->SetParent( this );
    }
}


/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as	 *
* specified, and in the given drawing mode. Only this one is visible...		 *
*****************************************************************************/
void SCH_COMPONENT::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& offset, int DrawMode, int Color )
{
    EDA_LibComponentStruct* Entry;
    int  ii;
    bool dummy = FALSE;

    Entry = ( EDA_LibComponentStruct* ) FindLibPart( m_ChipName );

    if( Entry == NULL )
    {
        /* composant non trouve, on affiche un composant "dummy" */
        dummy = TRUE;
        if( DummyCmp == NULL )
            CreateDummyCmp();
        Entry = DummyCmp;
    }

    DrawLibPartAux( panel, DC, this, Entry, m_Pos + offset,  m_Transform,
                    dummy ? 0 : m_Multi, dummy ? 0 : m_Convert, DrawMode );

    /* Trace des champs, avec placement et orientation selon orient. du
     *  composant
     */

    SCH_CMP_FIELD* field = GetField( REFERENCE );

    if( ( ( field->m_Attributs & TEXT_NO_VISIBLE ) == 0 )
        && !( field->m_Flags & IS_MOVED ) )
    {
        if( Entry->m_UnitCount > 1 )
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

    for( ii = VALUE; ii < GetFieldCount(); ii++ )
    {
        field = GetField( ii );

        if( field->m_Flags & IS_MOVED )
            continue;

        field->Draw( panel, DC, offset, DrawMode );
    }
}


/**
 * Function AddHierarchicalReference
 * adds a full hierachical reference (path + local reference)
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

    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
    {
        tokenizer.SetString( m_PathsAndReferences[ii], separators );
        h_path = tokenizer.GetNextToken();

        if( h_path.Cmp( path ) == 0 )
        {
            h_ref = tokenizer.GetNextToken();

            /* printf( "GetRef hpath: %s\n",
                    CONV_TO_UTF8( m_PathsAndReferences[ii] ) ); */
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
    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
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
             abs( rf->m_Pos.y - m_Pos.y ) > 10000 ) )
    {
        // move it to a reasonable position
        rf->m_Pos    = m_Pos;
        rf->m_Pos.x += 50;         // a slight offset
        rf->m_Pos.y += 50;
    }

    rf->m_Text = ref;  // for drawing.
}

/** function SetTimeStamp
 * Change the old time stamp to the new time stamp.
 * the time stamp is also modified in paths
 * @param aNewTimeStamp = new time stamp
 */
void SCH_COMPONENT::SetTimeStamp( long aNewTimeStamp)
{
    wxString string_timestamp, string_oldtimestamp;
    string_timestamp.Printf(wxT("%8.8X"), aNewTimeStamp);
    string_oldtimestamp.Printf(wxT("%8.8X"), m_TimeStamp);
    m_TimeStamp = aNewTimeStamp;
    for( unsigned ii = 0; ii < m_PathsAndReferences.GetCount(); ii++ )
    {
        m_PathsAndReferences[ii].Replace(string_oldtimestamp.GetData(), string_timestamp.GetData());
   }
}


/***********************************************************/
//returns the unit selection, for the given sheet path.
/***********************************************************/
int SCH_COMPONENT::GetUnitSelection( DrawSheetPath* aSheet )
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
void SCH_COMPONENT::SetUnitSelection( DrawSheetPath* aSheet, int aUnitSelection )
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


SCH_CMP_FIELD* SCH_COMPONENT::GetField( int aFieldNdx ) const
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


void SCH_COMPONENT::AddField( const SCH_CMP_FIELD& aField )
{
    m_Fields.push_back( aField );
}


EDA_Rect SCH_COMPONENT::GetBoundaryBox() const
{
    EDA_LibComponentStruct* Entry =
        ( EDA_LibComponentStruct* ) FindLibPart( m_ChipName );
    EDA_Rect BoundaryBox;
    int      x0, xm, y0, ym;

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
    int x1 = m_Transform[0][0] * x0 + m_Transform[0][1] * y0;
    int y1 = m_Transform[1][0] * x0 + m_Transform[1][1] * y0;
    int x2 = m_Transform[0][0] * xm + m_Transform[0][1] * ym;
    int y2 = m_Transform[1][0] * xm + m_Transform[1][1] * ym;

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


/**************************************************************************/
/* Used if undo / redo command:
 *  swap data between this and copyitem
 */
/**************************************************************************/
void SCH_COMPONENT::SwapData( SCH_COMPONENT* copyitem )
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

    // Reparent items after copying data
    // (after swap(), m_Parent member does not point to the right parent):
    for( int ii = 0; ii < copyitem->GetFieldCount();  ++ii )
    {
       copyitem->GetField(ii)->SetParent( copyitem );
    }
    for( int ii = 0; ii < GetFieldCount();  ++ii )
    {
       GetField(ii)->SetParent( this );
    }

}


/***********************************************************************/
void SCH_COMPONENT::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
/***********************************************************************/
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
 * @param aSheet: DrawSheetPath value: if NULL remove all annotations,
 *                else remove annotation relative to this sheetpath
 */
void SCH_COMPONENT::ClearAnnotation( DrawSheetPath* aSheet )
{
    wxString                defRef    = m_PrefixString;
    bool                    KeepMulti = false;
    EDA_LibComponentStruct* Entry;
    wxString                separators( wxT( " " ) );
    wxArrayString           reference_fields;

    Entry = ( EDA_LibComponentStruct* ) FindLibPart( m_ChipName );

    if( Entry && Entry->m_UnitSelectionLocked )
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
            // Break hierachical reference in path, ref and multi selection:
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
void SCH_COMPONENT::SetRotationMiroir( int type_rotate )
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


int SCH_COMPONENT::GetRotationMiroir()
/** function GetRotationMiroir()
 * Used to display component orientation (in dialog editor or info)
 * @return the orientation and mirror
 * Note: Because there are different ways to have a given orientation/mirror,
 * the orientation/mirror is not necessary wht the used does
 * (example : a mirrorX then a mirrorY give no mirror but rotate the component).
 * So this function find a rotation and a mirror value
 * ( CMP_MIROIR_X because this is the first mirror option tested)
 *  but can differs from the orientation made by an user
 * ( a CMP_MIROIR_Y is find as a CMP_MIROIR_X + orientation 180, because they are equivalent)
 *
*/
{
    int  type_rotate = CMP_ORIENT_0;
    int  ComponentMatOrient[2][2];
    int  ii;

    #define ROTATE_VALUES_COUNT 12
    int rotate_value[ROTATE_VALUES_COUNT] =    // list of all possibilities, but only the first 8 are actually used
    {
        CMP_ORIENT_0, CMP_ORIENT_90, CMP_ORIENT_180, CMP_ORIENT_270,
        CMP_MIROIR_X + CMP_ORIENT_0, CMP_MIROIR_X + CMP_ORIENT_90, CMP_MIROIR_X + CMP_ORIENT_180, CMP_MIROIR_X + CMP_ORIENT_270,
        CMP_MIROIR_Y + CMP_ORIENT_0, CMP_MIROIR_Y + CMP_ORIENT_90, CMP_MIROIR_Y + CMP_ORIENT_180, CMP_MIROIR_Y + CMP_ORIENT_270
    };

    // Try to find the current transform option:
    memcpy( ComponentMatOrient, m_Transform, sizeof( ComponentMatOrient ) );

    for( ii = 0; ii < ROTATE_VALUES_COUNT; ii++ )
    {
        type_rotate = rotate_value[ii];
        SetRotationMiroir( type_rotate );
        if( memcmp( ComponentMatOrient, m_Transform, sizeof(ComponentMatOrient) ) == 0 )
            return type_rotate;
    }

    // Error: orientation not found in list (should not happen)
    wxMessageBox(wxT("Component orientation matrix internal error") );
    memcpy( m_Transform, ComponentMatOrient, sizeof( ComponentMatOrient ) );
    return CMP_NORMAL;
}


/**
 * Renvoie la coordonn�e du point coord, en fonction de l'orientation
 *  du composant (rotation, miroir).
 *  Les coord sont toujours relatives a l'ancre (coord 0,0) du composant
 */
wxPoint SCH_COMPONENT::GetScreenCoord( const wxPoint& coord )
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
        " ref=\"" << ReturnFieldName( 0 ) << '"' << " chipName=\"" <<
        m_ChipName.mb_str() << '"' <<  m_Pos << " layer=\"" << m_Layer <<
        '"' << "/>\n";

    // skip the reference, it's been output already.
    for( int i = 1; i < GetFieldCount();  ++i )
    {
        wxString value = GetField( i )->m_Text;

        if( !value.IsEmpty() )
        {
            NestedSpace( nestLevel + 1, os ) << "<field" << " name=\"" <<
                ReturnFieldName( i ).mb_str() << '"' <<  " value=\"" <<
                value.mb_str() << "\"/>\n";
        }
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
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
            strncpy( Name1, CONV_TO_UTF8( GetField( REFERENCE )->m_Text ),
                     sizeof( Name1 ) );
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

    if ( fprintf( f, "$Comp\n" ) == EOF )
        return false;

    if( fprintf( f, "L %s %s\n", Name2, Name1 ) == EOF )
        return false;

    /* Generation de numero d'unit, convert et Time Stamp*/
    if( fprintf( f, "U %d %d %8.8lX\n", m_Multi, m_Convert,
                 m_TimeStamp ) == EOF )
        return false;

    /* Save the position */
    if( fprintf( f, "P %d %d\n", m_Pos.x, m_Pos.y ) == EOF )
        return false;

    /* If this is a complex hierarchy; save hierarchical references.
     * but for simple hierarchies it is not necessary.
     * the reference inf is already saved
     * this is usefull for old eeschema version compatibility
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

    for( int fieldNdx = 0; fieldNdx < GetFieldCount(); ++fieldNdx )
    {
        SCH_CMP_FIELD* field = GetField( fieldNdx );
        wxString       defaultName = ReturnDefaultFieldName( fieldNdx );

        // only save the field if there is a value in the field or if field name
        // is different than the default field name
        if( field->m_Text.IsEmpty() && defaultName == field->m_Name )
            continue;

        if( !field->Save( f ) )
            return false;
    }

    /* Generation du num unit, position, box ( ancienne norme )*/
    if( fprintf( f, "\t%-4d %-4d %-4d\n", m_Multi, m_Pos.x, m_Pos.y ) == EOF )
        return false;

    if( fprintf( f, "\t%-4d %-4d %-4d %-4d\n",
                 m_Transform[0][0], m_Transform[0][1],
                 m_Transform[1][0], m_Transform[1][1] ) == EOF )
        return false;

    if( fprintf( f, "$EndComp\n" ) == EOF )
        return false;

    return true;
}


EDA_Rect SCH_COMPONENT::GetBoundingBox()
{
    const int PADDING = 40;

    // This gives a reasonable approximation (but some things are missing so...)
    EDA_Rect  bbox = GetBoundaryBox();

    // Include BoundingBoxes of fields
    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        bbox.Merge( GetField( ii )->GetBoundaryBox() );
    }

    // ... add padding
    bbox.Inflate( PADDING, PADDING );

    return bbox;
}


void SCH_COMPONENT::DisplayInfo( WinEDA_DrawFrame* frame )
{
    EDA_LibComponentStruct* Entry =
        ( EDA_LibComponentStruct* ) FindLibPart( m_ChipName );

    wxString msg;

    frame->MsgPanel->EraseMsgBox();

    Affiche_1_Parametre( frame, 1, _( "Ref" ),
                         GetRef(((WinEDA_SchematicFrame*)frame)->GetSheet()),
                         DARKCYAN );

    if( Entry && Entry->m_Options == ENTRY_POWER )
        msg = _( "Pwr Symb" );
    else
        msg = _( "Val" );

    Affiche_1_Parametre( frame, 10, msg, GetField( VALUE )->m_Text, DARKCYAN );

    Affiche_1_Parametre( frame, 28, _( "RefLib" ), m_ChipName.GetData(), BROWN );

    msg = FindLibName;
    Affiche_1_Parametre( frame, 40, _( "Lib" ), msg, DARKRED );

    if( Entry )
    {
        Affiche_1_Parametre( frame, 52, Entry->m_Doc, Entry->m_KeyWord,
                             DARKCYAN );
    }
}

/** virtual function Mirror_Y
 * mirror item relative to an Y axis
 * @param aYaxis_position = the y axis position
 */
void SCH_COMPONENT::Mirror_Y(int aYaxis_position)
{
    int dx = m_Pos.x;
    SetRotationMiroir( CMP_MIROIR_Y );
    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
    dx -= m_Pos.x;     // dx,0 is the move vector for this transform

    for( int ii = 0; ii < GetFieldCount(); ii++ )
    {
        /* move the fields to the new position because the component itself has moved */
        GetField( ii )->m_Pos.x -= dx;
    }
}
