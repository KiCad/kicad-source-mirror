/**********************************************************/
/*  lib_entry.cpp                                         */
/**********************************************************/

#include "fctsys.h"
#include "common.h"
#include "kicad_string.h"
#include "confirm.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "gr_basic.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "class_libentry.h"

#include <boost/foreach.hpp>


/*********************/
/* class CMP_LIB_ENTRY */
/*********************/

/* Basic class for library component description
 *  Not directly used
 *  Used to create the 2 derived classes :
 *      - LIB_ALIAS
 *      - LIB_COMPONENT
 */

/********************************************************************/
CMP_LIB_ENTRY::CMP_LIB_ENTRY( LibrEntryType type, const wxString& name,
                              CMP_LIBRARY* lib ) :
    EDA_BaseStruct( LIBCOMPONENT_STRUCT_TYPE )
{
    Type = type;
    m_Name.m_FieldId = VALUE;
    m_Name.SetParent( this );
    m_Name.m_Text = name;
    m_lib = lib;
}


CMP_LIB_ENTRY::CMP_LIB_ENTRY( CMP_LIB_ENTRY& entry, CMP_LIBRARY* lib ) :
    EDA_BaseStruct( entry )
{
    Type = entry.Type;
    m_Name = entry.m_Name;
    m_Doc = entry.m_Doc;
    m_KeyWord = entry.m_KeyWord;
    m_DocFile = entry.m_DocFile;
    m_Options = entry.m_Options;
    m_lib = lib;
    m_Name.SetParent( this );
}


CMP_LIB_ENTRY::~CMP_LIB_ENTRY()
{
}


wxString CMP_LIB_ENTRY::GetLibraryName()
{
    if( m_lib != NULL )
        return m_lib->GetName();

    return wxString( _( "none" ) );
}


/**
 * Function SaveDoc
 * writes the doc info out to a FILE in "*.dcm" format.
 * Only non empty fields are written.
 * If all fields are empty, does not write anything
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool CMP_LIB_ENTRY::SaveDoc( FILE* aFile )
{
    if( m_Doc.IsEmpty() && m_KeyWord.IsEmpty() && m_DocFile.IsEmpty() )
        return true;

    /* Generation des lignes utiles */
    if( fprintf( aFile, "#\n$CMP %s\n", CONV_TO_UTF8( m_Name.m_Text ) ) < 0 )
        return false;

    if( ! m_Doc.IsEmpty()
        && fprintf( aFile, "D %s\n", CONV_TO_UTF8( m_Doc ) ) < 0 )
        return false;

    if( ! m_KeyWord.IsEmpty()
        && fprintf( aFile, "K %s\n", CONV_TO_UTF8( m_KeyWord ) ) < 0 )
        return false;

    if( ! m_DocFile.IsEmpty()
        && fprintf( aFile, "F %s\n", CONV_TO_UTF8( m_DocFile ) ) < 0 )
        return false;

    if( fprintf( aFile, "$ENDCMP\n" ) < 0 )
        return false;

    return true;
}


bool CMP_LIB_ENTRY::operator==( const wxChar* name ) const
{
    return m_Name.m_Text.CmpNoCase( name ) == 0;
}


bool operator<( const CMP_LIB_ENTRY& item1, const CMP_LIB_ENTRY& item2 )
{
    return item1.m_Name.m_Text.CmpNoCase( item2.m_Name.m_Text ) < 0;
}


int LibraryEntryCompare( const CMP_LIB_ENTRY* LE1, const CMP_LIB_ENTRY* LE2 )
{
    return LE1->m_Name.m_Text.CmpNoCase( LE2->m_Name.m_Text );
}


/*******************************/
/* class LIB_ALIAS */
/*******************************/

/* Class to define an alias of a component
 *  An alias uses the component definition (graphic, pins...)
 *  but has its own name and documentation.
 *  Therefore, when the component is modified, alias of this component are
 *   modified.
 *  This is a simple method to create components with differs very few
 *  (like 74LS00, 74HC00 ... and many op amps )
 */

LIB_ALIAS::LIB_ALIAS( const wxString& name, LIB_COMPONENT* root,
                      CMP_LIBRARY* lib ) :
    CMP_LIB_ENTRY( ALIAS, name, lib )
{
    wxASSERT( root != NULL && root->Type == ROOT );

    m_root = root;
}


LIB_ALIAS::LIB_ALIAS( LIB_ALIAS& alias, CMP_LIBRARY* lib ) :
    CMP_LIB_ENTRY( alias )
{
    m_root = alias.m_root;
}


LIB_ALIAS::~LIB_ALIAS()
{
}


void LIB_ALIAS::SetComponent( LIB_COMPONENT* root )
{
    wxASSERT( root != NULL && root->Type == ROOT );

    m_root =  root;
}


/********************************/
/* class LIB_COMPONENT */
/********************************/

/* This is a standard component  (in library)
 */
LIB_COMPONENT::LIB_COMPONENT( const wxString& name, CMP_LIBRARY* lib ) :
    CMP_LIB_ENTRY( ROOT, name, lib )
{
    m_LastDate            = 0;
    m_UnitCount           = 1;
    m_TextInside          = 40;
    m_Options             = ENTRY_NORMAL;
    m_UnitSelectionLocked = FALSE;
    m_DrawPinNum          = 1;
    m_DrawPinName         = 1;
    m_Prefix.m_FieldId    = REFERENCE;
    m_Prefix.SetParent( this );
}


LIB_COMPONENT::LIB_COMPONENT( LIB_COMPONENT& component, CMP_LIBRARY* lib ) :
    CMP_LIB_ENTRY( component, lib )
{
    LIB_DRAW_ITEM* newItem;
    LibDrawField*  oldField;
    LibDrawField*  newField;

    m_Prefix              = component.m_Prefix;
    m_AliasList           = component.m_AliasList;
    m_FootprintList       = component.m_FootprintList;
    m_UnitCount           = component.m_UnitCount;
    m_UnitSelectionLocked = component.m_UnitSelectionLocked;
    m_TextInside          = component.m_TextInside;
    m_DrawPinNum          = component.m_DrawPinNum;
    m_DrawPinName         = component.m_DrawPinName;
    m_LastDate            = component.m_LastDate;

    m_Prefix.SetParent( this );

    BOOST_FOREACH( LIB_DRAW_ITEM& oldItem, component.GetDrawItemList() )
    {
        if( ( oldItem.m_Flags & IS_NEW ) != 0 )
            continue;

        newItem = oldItem.GenCopy();
        newItem->SetParent( this );
        m_Drawings.push_back( newItem );
    }

    for( oldField = component.m_Fields; oldField != NULL;
         oldField = oldField->Next() )
    {
        newField = (LibDrawField*) oldField->GenCopy();
        newField->SetParent( this );
        m_Fields.PushBack( newField );
    }
}


LIB_COMPONENT::~LIB_COMPONENT()
{
}


void LIB_COMPONENT::Draw( WinEDA_DrawPanel* panel, wxDC* dc,
                          const wxPoint& offset, int multi,
                          int convert, int drawMode, int color,
                          const int transformMatrix[2][2],
                          bool showPinText, bool drawFields,
                          bool onlySelected )
{
    wxString       fieldText;
    LibDrawField*  Field;
    BASE_SCREEN*   screen = panel->GetScreen();

    GRSetDrawMode( dc, drawMode );

    BOOST_FOREACH( LIB_DRAW_ITEM& drawItem, m_Drawings )
    {
        if( onlySelected && drawItem.m_Selected == 0 )
            continue;

        // Do not draw an item while moving (the cursor handler does that)
        if( drawItem.m_Flags & IS_MOVED )
            continue;

        /* Do not draw items not attached to the current part */
        if( multi && drawItem.m_Unit && ( drawItem.m_Unit != multi ) )
            continue;

        if( convert && drawItem.m_Convert && ( drawItem.m_Convert != convert ) )
            continue;


        if( drawItem.Type() == COMPONENT_PIN_DRAW_TYPE )
        {
            drawItem.Draw( panel, dc, offset, color, drawMode, &showPinText,
                           transformMatrix );
        }
        else
        {
            bool force_nofill =
                ( screen->m_IsPrinting
                  && drawItem.m_Fill == FILLED_WITH_BG_BODYCOLOR
                  && GetGRForceBlackPenState() );

            drawItem.Draw( panel, dc, offset, color, drawMode,
                           (void*) force_nofill, transformMatrix );
        }
    }

    if( drawFields )
    {
        /* The reference designator field is a special case for naming
         * convention.
         *
         * FIXME: This should be handled by the LibDrawField class.
         */
        if( m_UnitCount > 1 )
        {
#if defined(KICAD_GOST)
            fieldText.Printf( wxT( "%s?.%c" ), (const wxChar*) m_Prefix.m_Text,
                              multi + '1' - 1 );
#else
            fieldText.Printf( wxT( "%s?%c" ), (const wxChar*) m_Prefix.m_Text,
                              multi + 'A' - 1 );
#endif
        }
        else
        {
            fieldText = m_Prefix.m_Text + wxT( "?" );
        }

        if( !( onlySelected && m_Prefix.m_Selected == 0 ) )
            m_Prefix.Draw( panel, dc, offset, color, drawMode, &fieldText,
                           transformMatrix );

        if( !( onlySelected && m_Name.m_Selected == 0 ) )
            m_Name.Draw( panel, dc, offset, color, drawMode, NULL,
                         transformMatrix );

        for( Field = m_Fields; Field != NULL; Field = Field->Next() )
        {
            if( onlySelected && Field->m_Selected == 0 )
                continue;

            Field->Draw( panel, dc, offset, color, drawMode, NULL,
                         transformMatrix );
        }
    }

    /* Enable this to draw the anchor of the component. */
#if 0
    int len = panel->GetScreen()->Unscale( 3 );
    GRLine( &panel->m_ClipBox, dc, offset.x, offset.y - len, offset.x,
            offset.y + len, 0, color );
    GRLine( &panel->m_ClipBox, dc, offset.x - len, offset.y, offset.x + len,
            offset.y, 0, color );
#endif

    /* Enable this to draw the bounding box around the component to validate
     * the bounding box calculations. */
#if 0
    EDA_Rect bBox = GetBoundaryBox( multi, convert );
    GRRect( &panel->m_ClipBox, dc, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LIB_COMPONENT::Plot( PLOTTER* plotter, int unit, int convert,
                          const wxPoint& offset, const int transform[2][2] )
{
    wxASSERT( plotter != NULL );

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( unit && item.m_Unit && ( item.m_Unit != unit ) )
            continue;
        if( convert && item.m_Convert && ( item.m_Convert != convert ) )
            continue;

        plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
        bool fill = plotter->get_color_mode();

        item.Plot( plotter, offset, fill, transform );
    }
}


void LIB_COMPONENT::RemoveDrawItem( LIB_DRAW_ITEM* item,
                                    WinEDA_DrawPanel* panel,
                                    wxDC* dc )
{
    wxASSERT( item != NULL );

    LIB_DRAW_ITEM_LIST::iterator i;

    if( dc != NULL )
        item->Draw( panel, dc, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                    DefaultTransformMatrix );

    if( item->Type() != COMPONENT_FIELD_DRAW_TYPE )
    {
        for( i = m_Drawings.begin(); i < m_Drawings.end(); i++ )
        {
            if( *i == item )
            {
                m_Drawings.erase( i );
                break;
            }
        }
    }
    else
    {
        LibDrawField* field;

        for( field = m_Fields; field != NULL; field = field->Next() )
        {
            if( field == item )
            {
                m_Fields.Remove( field );
                delete field;
                break;
            }
        }
    }
}


void LIB_COMPONENT::AddDrawItem( LIB_DRAW_ITEM* item )
{
    wxASSERT( item != NULL );

    m_Drawings.push_back( item );
    m_Drawings.sort();
}


LIB_DRAW_ITEM* LIB_COMPONENT::GetNextDrawItem( LIB_DRAW_ITEM* item,
                                               KICAD_T type )
{
    if( m_Drawings.empty() )
        return NULL;

    if( item == NULL && type == TYPE_NOT_INIT )
        return &m_Drawings[0];

    for( size_t i = 0; i < m_Drawings.size() - 1; i++ )
    {
        if( item != &m_Drawings[i] )
            continue;

        if( type == TYPE_NOT_INIT || m_Drawings[ i + 1 ].Type() == type )
            return &m_Drawings[ i + 1 ];
    }

    return NULL;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool LIB_COMPONENT::Save( FILE* aFile )
{
    LibDrawField*  Field;

    /* First line: it s a comment (component name for readers) */
    if( fprintf( aFile, "#\n# %s\n#\n", CONV_TO_UTF8( m_Name.m_Text ) ) < 0 )
        return false;

    /* Save data */
    if( fprintf( aFile, "DEF" ) < 0 )
        return false;

    if( (m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0 )
    {
        if( fprintf( aFile, " %s", CONV_TO_UTF8( m_Name.m_Text ) ) < 0 )
            return false;
    }
    else
    {
        if( fprintf( aFile, " ~%s", CONV_TO_UTF8( m_Name.m_Text ) ) < 0 )
            return false;
    }

    if( !m_Prefix.m_Text.IsEmpty() )
    {
        if( fprintf( aFile, " %s", CONV_TO_UTF8( m_Prefix.m_Text ) ) < 0 )
            return false;
    }
    else
    {
        if( fprintf( aFile, " ~" ) < 0 )
            return false;
    }

    if( fprintf( aFile, " %d %d %c %c %d %c %c\n",
                 0, m_TextInside,
                 m_DrawPinNum ? 'Y' : 'N',
                 m_DrawPinName ? 'Y' : 'N',
                 m_UnitCount, m_UnitSelectionLocked ? 'L' : 'F',
                 m_Options == ENTRY_POWER ? 'P' : 'N' ) < 0 )
        return false;

    if( !SaveDateAndTime( aFile ) || !m_Prefix.Save( aFile )
        || !m_Name.Save( aFile ) )
        return false;

    for( Field = m_Fields; Field != NULL; Field = Field->Next() )
    {
        if( Field->m_Text.IsEmpty() && Field->m_Name.IsEmpty() )
            continue;
        if( !Field->Save( aFile ) )
            return false;
    }

    /* Save the alias list: a line starting by "ALIAS" */
    if( m_AliasList.GetCount() != 0 )
    {
        if( fprintf( aFile, "ALIAS" ) < 0 )
            return false;

        for( size_t ii = 0; ii < m_AliasList.GetCount(); ii++ )
        {
            if( fprintf( aFile, " %s", CONV_TO_UTF8( m_AliasList[ii] ) ) < 0 )
                return false;
        }

        if( fprintf( aFile, "\n" ) < 0 )
            return false;
    }

    /* Write the footprint filter list */
    if( m_FootprintList.GetCount() != 0 )
    {
        if( fprintf( aFile, "$FPLIST\n" ) < 0 )
            return false;

        for( size_t ii = 0; ii < m_FootprintList.GetCount(); ii++ )
        {
            if( fprintf( aFile, " %s\n",
                         CONV_TO_UTF8( m_FootprintList[ii] ) ) < 0 )
                return false;
        }

        if( fprintf( aFile, "$ENDFPLIST\n" ) < 0 )
            return false;
    }

    /* Save graphics items (including pins) */
    if( !m_Drawings.empty() )
    {
        /* we sort the draw items, in order to have an edition more easy,
         *  when a file editing "by hand" is made */
        m_Drawings.sort();

        if( fprintf( aFile, "DRAW\n" ) < 0 )
            return false;

        BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
        {
            if( !item.Save( aFile ) )
                return false;
        }

        if( fprintf( aFile, "ENDDRAW\n" ) < 0 )
            return false;
    }

    if( fprintf( aFile, "ENDDEF\n" ) < 0 )
        return false;

    return true;
}

bool LIB_COMPONENT::Load( FILE* file, char* line, int* lineNum,
                          wxString& errorMsg )
{
    int      unused;
    char*    p;
    char*    name;
    char*    prefix = NULL;

    bool     Res;
    wxString Msg;

    p = strtok( line, " \t\r\n" );

    if( strcmp( p, "DEF" ) != 0 )
    {
        errorMsg.Printf( wxT( "DEF command expected in line %d, aborted." ),
                         *lineNum );
        return false;
    }

    /* Read DEF line: */
    char drawnum = 0;
    char drawname = 0;

    if( ( name = strtok( NULL, " \t\n" ) ) == NULL       /* Part name: */
        || ( prefix = strtok( NULL, " \t\n" ) ) == NULL  /* Prefix name: */
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* NumOfPins: */
        || sscanf( p, "%d", &unused ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* TextInside: */
        || sscanf( p, "%d", &m_TextInside ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* DrawNums: */
        || sscanf( p, "%c", &drawnum ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* DrawNums: */
        || sscanf( p, "%c", &drawname ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* m_UnitCount: */
        || sscanf( p, "%d", &m_UnitCount ) != 1 )
    {
        errorMsg.Printf( wxT( "Wrong DEF format in line %d, skipped." ),
                         *lineNum );
        while( GetLine( file, line, lineNum, 1024 ) )
        {
            p = strtok( line, " \t\n" );
            if( stricmp( p, "ENDDEF" ) == 0 )
                break;
        }

        return false;
    }

    m_DrawPinNum  = (drawnum == 'N') ? FALSE : TRUE;
    m_DrawPinName = (drawname == 'N') ? FALSE : TRUE;

    /* Copy part name and prefix. */
    strupper( name );
    if( name[0] != '~' )
        m_Name.m_Text = CONV_FROM_UTF8( name );
    else
    {
        m_Name.m_Text = CONV_FROM_UTF8( &name[1] );
        m_Name.m_Attributs |= TEXT_NO_VISIBLE;
    }

    if( strcmp( prefix, "~" ) == 0 )
    {
        m_Prefix.m_Text.Empty();
        m_Prefix.m_Attributs |= TEXT_NO_VISIBLE;
    }
    else
        m_Prefix.m_Text = CONV_FROM_UTF8( prefix );

    // Copy optional infos
    if( ( p = strtok( NULL, " \t\n" ) ) != NULL && *p == 'L' )
        m_UnitSelectionLocked = TRUE;
    if( ( p = strtok( NULL, " \t\n" ) ) != NULL  && *p == 'P' )
        m_Options = ENTRY_POWER;

    /* Read next lines */
    while( GetLine( file, line, lineNum, 1024 ) )
    {
        p = strtok( line, " \t\n" );

        /* This is the error flag ( if an error occurs, Res = FALSE) */
        Res = true;

        if( (line[0] == 'T') && (line[1] == 'i') )
            Res = LoadDateAndTime( line );
        else if( line[0] == 'F' )
            Res = LoadField( line, Msg );
        else if( strcmp( p, "ENDDEF" ) == 0 )
            break;
        else if( strcmp( p, "DRAW" ) == 0 )
            Res = LoadDrawEntries( file, line, lineNum, Msg );
        else if( strncmp( p, "ALIAS", 5 ) == 0 )
        {
            p = strtok( NULL, "\r\n" );
            Res = LoadAliases( p, errorMsg );
        }
        else if( strncmp( p, "$FPLIST", 5 ) == 0 )
            Res = LoadFootprints( file, line, lineNum, Msg );

        /* End line or block analysis: test for an error */
        if( !Res )
        {
            if( Msg.IsEmpty() )
                errorMsg.Printf( wxT( "error occurred at line %d " ), *lineNum );
            else
                errorMsg.Printf( wxT( "error <%s> occurred at line %d " ),
                                 ( const wxChar* ) Msg, *lineNum );
            return false;
        }
    }

    /* If we are here, this part is O.k. - put it in: */
    m_Drawings.sort();

    return true;
}


bool LIB_COMPONENT::LoadDrawEntries( FILE* f, char* line,
                                     int* lineNum, wxString& errorMsg )
{
    LIB_DRAW_ITEM* newEntry = NULL;

    while( true )
    {
        if( GetLine( f, line, lineNum, 1024 ) == NULL )
        {
            errorMsg = _( "file ended prematurely loading component draw element" );
            return false;
        }

        if( strncmp( line, "ENDDRAW", 7 ) == 0 )
            break;

        newEntry = NULL;

        switch( line[0] )
        {
        case 'A':    /* Arc */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawArc(this);
            break;

        case 'C':    /* Circle */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawCircle(this);
            break;

        case 'T':    /* Text */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawText(this);
            break;

        case 'S':    /* Square */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawSquare(this);
            break;

        case 'X':    /* Pin Description */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawPin(this);
            break;

        case 'P':    /* Polyline */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawPolyline(this);
            break;

        case 'B':    /* Bezier Curves */
            newEntry = ( LIB_DRAW_ITEM* ) new LibDrawBezier(this);
            break;

        default:
            errorMsg.Printf( wxT( "undefined DRAW command %c" ), line[0] );
            return false;
        }

        if( !newEntry->Load( line, errorMsg ) )
        {
            errorMsg.Printf( wxT( "error <%s> in DRAW command %c" ),
                             ( const wxChar* ) errorMsg, line[0] );
            SAFE_DELETE( newEntry );

            /* Flush till end of draw section */
            do
            {
                if( GetLine( f, line, lineNum, 1024 ) == NULL )
                {
                    errorMsg = _( "file ended prematurely while attempting \
to flush to end of drawing section." );
                    return false;
                }
            } while( strncmp( line, "ENDDRAW", 7 ) != 0 );

            return false;
        }
        else
        {
            m_Drawings.push_back( newEntry );
        }
    }

    return true;
}


bool LIB_COMPONENT::LoadAliases( char* line, wxString& errorMsg )
{
    char* text = strtok( line, " \t\r\n" );

    while( text )
    {
        m_AliasList.Add( CONV_FROM_UTF8( text ) );
        text = strtok( NULL, " \t\r\n" );
    }

    return true;
}


bool LIB_COMPONENT::LoadField( char* line, wxString& errorMsg )
{
    LibDrawField* field = new LibDrawField( this );

    if ( !field->Load( line, errorMsg ) )
    {
        SAFE_DELETE( field );
        return false;
    }

    if( field->m_FieldId == REFERENCE )
    {
        m_Prefix = *field;
        SAFE_DELETE( field );
    }
    else if ( field->m_FieldId == VALUE )
    {
        m_Name = *field;
        SAFE_DELETE( field );
    }
    else
    {
        m_Fields.PushBack( field );
    }

    return true;
}


bool LIB_COMPONENT::LoadFootprints( FILE* file, char* line,
                                    int* lineNum, wxString& errorMsg )
{
    while( true )
    {
        if( GetLine( file, line, lineNum, 1024 ) == NULL )
        {
            errorMsg = wxT( "file ended prematurely while loading footprints" );
            return false;
        }

        if( stricmp( line, "$ENDFPLIST" ) == 0 )
            break;

        m_FootprintList.Add( CONV_FROM_UTF8( line + 1 ) );
    }

    return true;
}


/**********************************************************************/
/* Return the component boundary box ( in user coordinates )
 *  The unit Unit, and the shape Convert are considered.
 *  If Unit == 0, Unit is not used
 *  if Convert == 0 Convert is non used
 **/
/**********************************************************************/
EDA_Rect LIB_COMPONENT::GetBoundaryBox( int Unit, int Convert )
{
    EDA_Rect bBox( wxPoint( 0, 0 ), wxSize( 0, 0 ) );

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( ( item.m_Unit > 0 ) && ( ( m_UnitCount > 1 ) && ( Unit > 0 )
                                     && ( Unit != item.m_Unit ) ) )
            continue;
        if( item.m_Convert > 0
            && ( ( Convert > 0 ) && ( Convert != item.m_Convert ) ) )
            continue;

        bBox.Merge( item.GetBoundingBox() );
    }

    bBox.Merge( m_Name.GetBoundingBox() );
    bBox.Merge( m_Prefix.GetBoundingBox() );

    return bBox;
}


/** Function SetFields
 * initialize fields from a vector of fields
 * @param aFields a std::vector <LibDrawField> to import.
 */
void LIB_COMPONENT::SetFields( const std::vector <LibDrawField> aFields )
{
    // Init basic fields (Value = name in lib, and reference):
    aFields[VALUE].Copy( &m_Name );
    aFields[REFERENCE].Copy( &m_Prefix );

    // Remove others fields:
    m_Fields.DeleteAll();

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
            LibDrawField*Field = new LibDrawField( this, ii );
            aFields[ii].Copy( Field );
            m_Fields.PushBack( Field );
        }
    }

    /* for a user field (FieldId >= FIELD1), if a field value is void,
     * fill it with "~" because for a library component a void field is not
     * a very good idea  (we do not see anything...) and in schematic this
     * text is like a void text and for non editable names, remove the name
     * (set to the default name)
     */
    for( LibDrawField* Field = m_Fields; Field; Field = Field->Next() )
    {
        Field->SetParent( this );
        if( Field->m_FieldId >= FIELD1 )
        {
            if( Field->m_Text.IsEmpty() )
                Field->m_Text = wxT( "~" );
        }
        else
            Field->m_Name.Empty();
    }
}


/*
 * lit date et time de modif composant sous le format:
 *  "Ti yy/mm/jj hh:mm:ss"
 */
bool LIB_COMPONENT::SaveDateAndTime( FILE* file )
{
    int year, mon, day, hour, min, sec;

    if( m_LastDate == 0 )
        return true;

    sec  = m_LastDate & 63;
    min  = ( m_LastDate >> 6 ) & 63;
    hour = ( m_LastDate >> 12 ) & 31;
    day  = ( m_LastDate >> 17 ) & 31;
    mon  = ( m_LastDate >> 22 ) & 15;
    year = ( m_LastDate >> 26 ) + 1990;

    if ( fprintf( file, "Ti %d/%d/%d %d:%d:%d\n",
                  year, mon, day, hour, min, sec ) < 0 )
        return false;

    return true;
}

/* lit date et time de modif composant sous le format:
 *  "Ti yy/mm/jj hh:mm:ss"
 */
bool LIB_COMPONENT::LoadDateAndTime( char* Line )
{
    int   year, mon, day, hour, min, sec;
    char* text;

    year = mon = day = hour = min = sec = 0;
    text = strtok( Line, " \r\t\n" );
    text = strtok( NULL, " \r\t\n" );  // text pointe donnees utiles

    if (sscanf( Line, "%d/%d/%d %d:%d:%d",
                &year, &mon, &day, &hour, &min, &sec ) != 6 )
        return false;

    m_LastDate = ( sec & 63 ) + ( ( min & 63 ) << 6 ) +
        ( ( hour & 31 ) << 12 ) + ( ( day & 31 ) << 17 ) +
        ( ( mon & 15 ) << 22 ) + ( ( year - 1990 ) << 26 );

    return true;
}


void LIB_COMPONENT::SetOffset( const wxPoint& offset )
{
    m_Name.SetOffset( offset );
    m_Prefix.SetOffset( offset );

    for( LibDrawField* field = m_Fields; field != NULL; field = field->Next() )
    {
        field->SetOffset( offset );
    }

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        item.SetOffset( offset );
    }
}


void LIB_COMPONENT::RemoveDuplicateDrawItems()
{
    m_Drawings.unique();
}


bool LIB_COMPONENT::HasConversion() const
{
    BOOST_FOREACH( const LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( item.m_Convert > 1 )
            return true;
    }

    return false;
}


void LIB_COMPONENT::ClearStatus( void )
{
    LibDrawField* field;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
        item.m_Flags = 0;

    m_Name.m_Flags = 0;
    m_Prefix.m_Flags = 0;

    for( field = m_Fields.GetFirst(); field != NULL; field = field->Next() )
        field->m_Flags = 0;
}


int LIB_COMPONENT::SelectItems( EDA_Rect& rect, int unit, int convert,
                                bool editPinByPin )
{
    int            ItemsCount = 0;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        item.m_Selected = 0;

        if( ( item.m_Unit && item.m_Unit != unit )
            || ( item.m_Convert && item.m_Convert != convert ) )
        {
            if( item.Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;

             // Specific rules for pins.
            if( editPinByPin || m_UnitSelectionLocked
                || ( item.m_Convert && item.m_Convert != convert ) )
                continue;
        }

        if( item.Inside( rect ) )
        {
            item.m_Selected = IS_SELECTED;
            ItemsCount++;
        }
    }

    if( m_Name.Inside( rect ) )
    {
        m_Name.m_Selected = IS_SELECTED;
        ItemsCount++;
    }

    if( m_Prefix.Inside( rect ) )
    {
        m_Prefix.m_Selected = IS_SELECTED;
        ItemsCount++;
    }

    for( LibDrawField* field = m_Fields.GetFirst(); field != NULL;
         field = field->Next() )
    {
        if( field->Inside( rect ) )
        {
            field->m_Selected = IS_SELECTED;
            ItemsCount++;
        }
    }

    return ItemsCount;
}


void LIB_COMPONENT::MoveSelectedItems( const wxPoint& offset )
{
    LibDrawField*  field;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( item.m_Selected == 0 )
            continue;

        item.SetOffset( offset );
        item.m_Flags = item.m_Selected = 0;
    }

    if( m_Name.m_Selected )
    {
        m_Name.SetOffset( offset );
        m_Name.m_Flags = m_Name.m_Selected = 0;
    }

    if( m_Prefix.m_Selected )
    {
        m_Prefix.SetOffset( offset );
        m_Prefix.m_Flags = m_Prefix.m_Selected = 0;
    }

    for( field = m_Fields.GetFirst(); field != NULL; field = field->Next() )
    {
        if( field->m_Selected )
        {
            field->SetOffset( offset );
            field->m_Flags = field->m_Selected = 0;
        }
    }

    m_Drawings.sort();
}


void LIB_COMPONENT::ClearSelectedItems( void )
{
    LibDrawField* field;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
        item.m_Flags = item.m_Selected = 0;

    m_Name.m_Flags = m_Name.m_Selected = 0;
    m_Prefix.m_Flags = m_Prefix.m_Selected = 0;

    for( field = m_Fields.GetFirst(); field != NULL; field = field->Next() )
        field->m_Flags = field->m_Selected = 0;
}


void LIB_COMPONENT::DeleteSelectedItems( void )
{
    LIB_DRAW_ITEM_LIST::iterator i = m_Drawings.begin();

    while( i != m_Drawings.end() )
    {
        if( i->m_Selected == 0 )
            i++;
        else
            i = m_Drawings.erase( i );
    }
}


void LIB_COMPONENT::CopySelectedItems( const wxPoint& offset )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( item.m_Selected == 0 )
            continue;

        item.m_Selected = 0;
        LIB_DRAW_ITEM* newItem = item.GenCopy();
        newItem->m_Selected = IS_SELECTED;
        m_Drawings.push_back( newItem );
    }

    MoveSelectedItems( offset );
    m_Drawings.sort();
}

void LIB_COMPONENT::MirrorSelectedItemsH( const wxPoint& center )
{
    LibDrawField*  field;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( item.m_Selected == 0 )
            continue;

        item.SetOffset( center );
        item.m_Flags = item.m_Selected = 0;
    }

    if( m_Name.m_Selected )
    {
        m_Name.SetOffset( center );
        m_Name.m_Flags = m_Name.m_Selected = 0;
    }

    if( m_Prefix.m_Selected )
    {
        m_Prefix.SetOffset( center );
        m_Prefix.m_Flags = m_Prefix.m_Selected = 0;
    }

    for( field = m_Fields.GetFirst(); field != NULL; field = field->Next() )
    {
        if( field->m_Selected )
        {
            field->SetOffset( center );
            field->m_Flags = field->m_Selected = 0;
        }
    }

    m_Drawings.sort();
}



/**
 * Locate a draw object.
 *
 * @param unit - Unit number of draw item.
 * @param convert - Body style of draw item.
 * @param type - Draw object type, set to 0 to search for any type.
 * @param pt - Coordinate for hit testing.
 *
 * @return LIB_DRAW_ITEM - Pointer the the draw object if found.
 *                         Otherwise NULL.
 */
LIB_DRAW_ITEM* LIB_COMPONENT::LocateDrawItem( int unit, int convert,
                                              KICAD_T type, const wxPoint& pt )
{
    LibDrawField*  field;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
    {
        if( ( unit && item.m_Unit && ( unit != item.m_Unit) )
            || ( convert && item.m_Convert && ( convert != item.m_Convert ) )
            || ( ( item.Type() != type ) && ( type != TYPE_NOT_INIT ) ) )
            continue;

        if( item.HitTest( pt ) )
            return &item;
    }

    if( type == COMPONENT_FIELD_DRAW_TYPE || type == TYPE_NOT_INIT )
    {
        if( m_Name.HitTest( pt ) )
            return &m_Name;
        if( m_Prefix.HitTest( pt ) )
            return &m_Prefix;

        for( field = m_Fields.GetFirst(); field != NULL; field = field->Next() )
        {
            if( field->HitTest( pt ) )
                return field;
        }
    }

    return NULL;
}

/** Function HitTest (overlaid)
 * @return true if the point aPosRef is near this object
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half
 *                     thickness of a line)
 * @param aTransMat = the transform matrix
 *
 * @return LIB_DRAW_ITEM - Pointer the the draw object if found.
 *                         Otherwise NULL.
 */
LIB_DRAW_ITEM* LIB_COMPONENT::LocateDrawItem( int unit, int convert,
                                              KICAD_T type, const wxPoint& pt, const int aTransMat[2][2] )
{
    /* we use LocateDrawItem( int unit, int convert, KICAD_T type, const wxPoint& pt )
     * to search items.
     * because this function uses DefaultTransformMatrix as orient/mirror matrix
     * we temporary copy aTransMat in DefaultTransformMatrix
    */
    LIB_DRAW_ITEM * item;
    int matrix[2][2];
    for ( int ii =0; ii<2;ii++ )
    {
        for ( int jj =0; jj<2;jj++ )
        {
            matrix[ii][jj] = aTransMat[ii][jj];
            EXCHG(matrix[ii][jj], DefaultTransformMatrix[ii][jj]);
        }
    }
    item = LocateDrawItem( unit, convert, type, pt );
    //Restore matrix
    for ( int ii =0; ii<2;ii++ )
    {
        for ( int jj =0; jj<2;jj++ )
        {
            EXCHG(matrix[ii][jj], DefaultTransformMatrix[ii][jj]);
        }
    }
    
    return item;
}

void LIB_COMPONENT::SetPartCount( int count )
{
    LIB_DRAW_ITEM_LIST::iterator i;

    if( m_UnitCount == count )
        return;

    if( count < m_UnitCount )
    {
        i = m_Drawings.begin();

        while( i != m_Drawings.end() )
        {
            if( i->m_Unit > count )
                i = m_Drawings.erase( i );
            else
                i++;
        }
    }
    else
    {
        int prevCount = m_UnitCount;

        for( i = m_Drawings.begin(); i != m_Drawings.end(); i++ )
        {
            if( i->m_Unit != 1 )
                continue;

            for( int j = prevCount + 1; j <= count; j++ )
            {
                LIB_DRAW_ITEM* newItem = i->GenCopy();
                newItem->m_Unit = j;
                m_Drawings.push_back( newItem );
            }
        }

        m_Drawings.sort();
    }

    m_UnitCount = count;
}


void LIB_COMPONENT::SetConversion( bool asConvert )
{
    if( asConvert == HasConversion() )
        return;

    if( asConvert )
    {

        BOOST_FOREACH( LIB_DRAW_ITEM& item, m_Drawings )
        {
            /* Only pins are duplicated. */
            if( item.Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( item.m_Convert == 1 )
            {
                LIB_DRAW_ITEM* newItem = item.GenCopy();
                newItem->m_Convert = 2;
                m_Drawings.push_back( newItem );
            }
        }
    }
    else
    {
        LIB_DRAW_ITEM_LIST::iterator i = m_Drawings.begin();

        while( i != m_Drawings.end() )
        {
            if( i->m_Convert > 1 )
                i = m_Drawings.erase( i );
            else
                i++;
        }
    }
}
