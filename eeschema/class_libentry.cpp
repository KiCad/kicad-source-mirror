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


/** class CMP_LIB_ENTRY
 * Base class to describe library components and aliases.
 * This class is not to be used directly.
 * There are 2 derived classes
 * class LIB_COMPONENT that describes a component in library
 * class LIB_ALIAS that describes an alias of an existing component
 * a LIB_COMPONENT object handle all info to draw a component
 *  (pins, graphic body items, fields, name, keywords and documentation)
 * a LIB_ALIAS object use info of its LIB_COMPONENT parent
 *   and has just a name, keywords and documentation
 */

CMP_LIB_ENTRY::CMP_LIB_ENTRY( LibrEntryType aType, const wxString& aName,
                              CMP_LIBRARY* aLibrary ) :
    EDA_BaseStruct( LIBCOMPONENT_STRUCT_TYPE )
{
    type = aType;
    name = aName;
    library = aLibrary;
}


CMP_LIB_ENTRY::CMP_LIB_ENTRY( CMP_LIB_ENTRY& aEntry, CMP_LIBRARY* aLibrary ) :
    EDA_BaseStruct( aEntry )
{
    type = aEntry.type;
    name = aEntry.name;
    description = aEntry.description;
    keyWords = aEntry.keyWords;
    docFileName = aEntry.docFileName;
    options = aEntry.options;
    library = aLibrary;
}


CMP_LIB_ENTRY::~CMP_LIB_ENTRY()
{
}


wxString CMP_LIB_ENTRY::GetLibraryName()
{
    if( library != NULL )
        return library->GetName();

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
    if( description.IsEmpty() && keyWords.IsEmpty() && docFileName.IsEmpty() )
        return true;

    if( fprintf( aFile, "#\n$CMP %s\n", CONV_TO_UTF8( name ) ) < 0 )
        return false;

    if( ! description.IsEmpty()
        && fprintf( aFile, "D %s\n", CONV_TO_UTF8( description ) ) < 0 )
        return false;

    if( ! keyWords.IsEmpty()
        && fprintf( aFile, "K %s\n", CONV_TO_UTF8( keyWords ) ) < 0 )
        return false;

    if( ! docFileName.IsEmpty()
        && fprintf( aFile, "F %s\n", CONV_TO_UTF8( docFileName ) ) < 0 )
        return false;

    if( fprintf( aFile, "$ENDCMP\n" ) < 0 )
        return false;

    return true;
}


bool CMP_LIB_ENTRY::operator==( const wxChar* aName ) const
{
    return name.CmpNoCase( aName ) == 0;
}


bool operator<( const CMP_LIB_ENTRY& aItem1, const CMP_LIB_ENTRY& aItem2 )
{
    return aItem1.GetName().CmpNoCase( aItem2.GetName() ) < 0;
}


int LibraryEntryCompare( const CMP_LIB_ENTRY* aItem1, const CMP_LIB_ENTRY* aItem2 )
{
    return aItem1->GetName().CmpNoCase( aItem2->GetName() );
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

LIB_ALIAS::LIB_ALIAS( const wxString& aName, LIB_COMPONENT* aRootComponent,
                      CMP_LIBRARY* aLibrary ) :
    CMP_LIB_ENTRY( ALIAS, aName, aLibrary )
{
    wxASSERT( aRootComponent != NULL && aRootComponent->isComponent() );

    root = aRootComponent;
}


LIB_ALIAS::LIB_ALIAS( LIB_ALIAS& aAlias, CMP_LIBRARY* aLibrary ) :
    CMP_LIB_ENTRY( aAlias )
{
    root = aAlias.root;
}


LIB_ALIAS::~LIB_ALIAS()
{
}


void LIB_ALIAS::SetComponent( LIB_COMPONENT* aComponent )
{
    wxASSERT( aComponent != NULL && aComponent->isComponent() );

    root = aComponent;
}


/********************************/
/* class LIB_COMPONENT */
/********************************/

/**
 * Library component object definition.
 *
 * A library component object is typically saved and loaded
 * in a component library file (.lib).
 * Library components are different from schematic components.
 */
LIB_COMPONENT::LIB_COMPONENT( const wxString& aName, CMP_LIBRARY* aLibrary ) :
    CMP_LIB_ENTRY( ROOT, aName, aLibrary )
{
    m_LastDate            = 0;
    unitCount             = 1;
    m_TextInside          = 40;
    options             = ENTRY_NORMAL;
    m_UnitSelectionLocked = FALSE;
    m_DrawPinNum          = 1;
    m_DrawPinName         = 1;

    /* The minimum requirements for a component are a value and a reference
     * designator field.
     */
    LIB_FIELD* value = new LIB_FIELD( this, VALUE );
    value->m_Text = aName;
    drawings.push_back( value );
    drawings.push_back( new LIB_FIELD( this, REFERENCE ) );
}


LIB_COMPONENT::LIB_COMPONENT( LIB_COMPONENT& aComponent, CMP_LIBRARY* aLibrary ) :
    CMP_LIB_ENTRY( aComponent, aLibrary )
{
    LIB_DRAW_ITEM* newItem;

    m_AliasList           = aComponent.m_AliasList;
    m_FootprintList       = aComponent.m_FootprintList;
    unitCount             = aComponent.unitCount;
    m_UnitSelectionLocked = aComponent.m_UnitSelectionLocked;
    m_TextInside          = aComponent.m_TextInside;
    m_DrawPinNum          = aComponent.m_DrawPinNum;
    m_DrawPinName         = aComponent.m_DrawPinName;
    m_LastDate            = aComponent.m_LastDate;

    BOOST_FOREACH( LIB_DRAW_ITEM& oldItem, aComponent.GetDrawItemList() )
    {
        if( ( oldItem.m_Flags & IS_NEW ) != 0 )
            continue;

        newItem = oldItem.GenCopy();
        newItem->SetParent( this );
        drawings.push_back( newItem );
    }
}


LIB_COMPONENT::~LIB_COMPONENT()
{
}


void LIB_COMPONENT::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDc,
                          const wxPoint& aOffset, int aMulti,
                          int aConvert, int aDrawMode, int aColor,
                          const int aTransformMatrix[2][2],
                          bool aShowPinText, bool aDrawFields,
                          bool aOnlySelected )
{
    BASE_SCREEN*   screen = aPanel->GetScreen();

    GRSetDrawMode( aDc, aDrawMode );

    BOOST_FOREACH( LIB_DRAW_ITEM& drawItem, drawings )
    {
        if( aOnlySelected && drawItem.m_Selected == 0 )
            continue;

        // Do not draw an item while moving (the cursor handler does that)
        if( drawItem.m_Flags & IS_MOVED )
            continue;

        /* Do not draw items not attached to the current part */
        if( aMulti && drawItem.m_Unit && ( drawItem.m_Unit != aMulti ) )
            continue;

        if( aConvert && drawItem.m_Convert && ( drawItem.m_Convert != aConvert ) )
            continue;

        if( !aDrawFields && drawItem.Type() == COMPONENT_FIELD_DRAW_TYPE )
            continue;

        if( drawItem.Type() == COMPONENT_PIN_DRAW_TYPE )
        {
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode,
                           (void*) aShowPinText, aTransformMatrix );
        }
        else if( drawItem.Type() == COMPONENT_FIELD_DRAW_TYPE )
        {
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode,
                           (void*) NULL, aTransformMatrix );
        }
        else
        {
            bool forceNoFill = ( screen->m_IsPrinting
                                 && drawItem.m_Fill == FILLED_WITH_BG_BODYCOLOR
                                 && GetGRForceBlackPenState() );
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode,
                           (void*) forceNoFill, aTransformMatrix );
        }

    }

    /* Enable this to draw the anchor of the component. */
#if 0
#ifdef USE_WX_ZOOM
    int len = aDc->DeviceToLogicalXRel( 3 );
#else
    int len = aPanel->GetScreen()->Unscale( 3 );
#endif
    GRLine( &aPanel->m_ClipBox, aDc, aOffset.x, aOffset.y - len, aOffset.x,
            aOffset.y + len, 0, aColor );
    GRLine( &aPanel->m_ClipBox, aDc, aOffset.x - len, aOffset.y, aOffset.x + len,
            aOffset.y, 0, aColor );
#endif

    /* Enable this to draw the bounding box around the component to validate
     * the bounding box calculations. */
#if 0
    EDA_Rect bBox = GetBoundaryBox( aMulti, aConvert );
    GRRect( &aPanel->m_ClipBox, aDc, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LIB_COMPONENT::Plot( PLOTTER* aPlotter, int aUnit, int aConvert,
                          const wxPoint& aOffset, const int aTransform[2][2] )
{
    wxASSERT( aPlotter != NULL );

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
            continue;
        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        aPlotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
        bool fill = aPlotter->get_color_mode();

        item.Plot( aPlotter, aOffset, fill, aTransform );
    }
}


void LIB_COMPONENT::RemoveDrawItem( LIB_DRAW_ITEM* aItem,
                                    WinEDA_DrawPanel* aPanel,
                                    wxDC* aDc )
{
    wxASSERT( aItem != NULL );

    /* Value and reference fields cannot be removed. */
    if( aItem->Type() == COMPONENT_FIELD_DRAW_TYPE )
    {
        LIB_FIELD* field = (LIB_FIELD*)aItem;

        if( field->m_FieldId == VALUE || field->m_FieldId == REFERENCE )
        {
            wxString fieldType = ( field->m_FieldId == VALUE ) ?
                _( "value" ) : _( "reference" );

            wxLogWarning( _( "An attempt was made to remove the %s field \
from component %s in library %s." ),
                          GetChars( fieldType ), GetChars( GetName() ),
                          GetChars( GetLibraryName() ) );
            return;
        }
    }

    LIB_DRAW_ITEM_LIST::iterator i;

    if( aDc != NULL )
        aItem->Draw( aPanel, aDc, wxPoint( 0, 0 ), -1, g_XorMode, NULL,
                    DefaultTransformMatrix );

    for( i = drawings.begin(); i < drawings.end(); i++ )
    {
        if( *i == aItem )
        {
            drawings.erase( i );
            break;
        }
    }
}


void LIB_COMPONENT::AddDrawItem( LIB_DRAW_ITEM* aItem )
{
    wxASSERT( aItem != NULL );

    drawings.push_back( aItem );
    drawings.sort();
}


LIB_DRAW_ITEM* LIB_COMPONENT::GetNextDrawItem( LIB_DRAW_ITEM* aItem,
                                               KICAD_T aType )
{
    /* Return the next draw object pointer.
     * If item is NULL return the first item of type in the list.
     */
    if( drawings.empty() )
        return NULL;

    if( aItem == NULL && aType == TYPE_NOT_INIT )    // type is unspecified
        return &drawings[0];

    // Search for last item
    size_t idx = 0;
    if( aItem )
    {
        for( ; idx < drawings.size(); idx++ )
        {
            if( aItem == &drawings[idx] )
            {
                idx++;   // Prepare the next item search
                break;
            }
        }
    }

    // Search the next item
    for( ; idx < drawings.size(); idx++ )
    {
        if( aType == TYPE_NOT_INIT || drawings[ idx ].Type() == aType )
            return &drawings[ idx ];
    }

    return NULL;
}


void LIB_COMPONENT::GetPins( LIB_PIN_LIST& aList, int aUnit, int aConvert )
{
    /* Notes:
     * when aUnit == 0: no unit filtering
     * when aConvert == 0: no convert (shape selection) filtering
     * when .m_Unit == 0, the body item is common to units
     * when .m_Convert == 0, the body item is common to shapes
     */
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( item.Type() != COMPONENT_PIN_DRAW_TYPE )    // we search pins only
            continue;

        // Unit filtering:
        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
             continue;

        // Shape filtering:
        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        aList.push_back( (LIB_PIN*) &item );
    }
}


LIB_PIN* LIB_COMPONENT::GetPin( const wxString& aNumber, int aUnit, int aConvert )
{
    wxString     pNumber;
    LIB_PIN_LIST pinList;

    GetPins( pinList, aUnit, aConvert );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        wxASSERT( pinList[i]->Type() == COMPONENT_PIN_DRAW_TYPE );

        pinList[i]->ReturnPinStringNum( pNumber );

        if( aNumber == pNumber )
            return pinList[i];
    }

    return NULL;
}


bool LIB_COMPONENT::Save( FILE* aFile )
{
    size_t     i;
    LIB_FIELD& value = GetValueField();

    /* First line: it s a comment (component name for readers) */
    if( fprintf( aFile, "#\n# %s\n#\n", CONV_TO_UTF8( value.m_Text ) ) < 0 )
        return false;

    /* Save data */
    if( fprintf( aFile, "DEF" ) < 0 )
        return false;

    if( value.IsVisible() )
    {
        if( fprintf( aFile, " %s", CONV_TO_UTF8( value.m_Text ) ) < 0 )
            return false;
    }
    else
    {
        if( fprintf( aFile, " ~%s", CONV_TO_UTF8( value.m_Text ) ) < 0 )
            return false;
    }

    LIB_FIELD& reference = GetReferenceField();

    if( !reference.m_Text.IsEmpty() )
    {
        if( fprintf( aFile, " %s", CONV_TO_UTF8( reference.m_Text ) ) < 0 )
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
                 unitCount, m_UnitSelectionLocked ? 'L' : 'F',
                 options == ENTRY_POWER ? 'P' : 'N' ) < 0 )
        return false;

    if( !SaveDateAndTime( aFile ) )
        return false;

    LIB_FIELD_LIST fields;
    GetFields( fields );

    for( i = 0; i < fields.size(); i++ )
    {
        if( fields[i].m_Text.IsEmpty() && fields[i].m_Name.IsEmpty() )
            continue;

        if( !fields[i].Save( aFile ) )
            return false;
    }

    /* Save the alias list: a line starting by "ALIAS" */
    if( m_AliasList.GetCount() != 0 )
    {
        if( fprintf( aFile, "ALIAS" ) < 0 )
            return false;

        for( i = 0; i < m_AliasList.GetCount(); i++ )
        {
            if( fprintf( aFile, " %s", CONV_TO_UTF8( m_AliasList[i] ) ) < 0 )
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

        for( i = 0; i < m_FootprintList.GetCount(); i++ )
        {
            if( fprintf( aFile, " %s\n",
                         CONV_TO_UTF8( m_FootprintList[i] ) ) < 0 )
                return false;
        }

        if( fprintf( aFile, "$ENDFPLIST\n" ) < 0 )
            return false;
    }

    /* Save graphics items (including pins) */
    if( !drawings.empty() )
    {
        /* we sort the draw items, in order to have an edition more easy,
         *  when a file editing "by hand" is made */
        drawings.sort();

        if( fprintf( aFile, "DRAW\n" ) < 0 )
            return false;

        BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
        {
            if( item.Type() == COMPONENT_FIELD_DRAW_TYPE )
                continue;
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

bool LIB_COMPONENT::Load( FILE* aFile, char* aLine, int* aLineNum,
                          wxString& aErrorMsg )
{
    int      unused;
    char*    p;
    char*    componentName;
    char*    prefix = NULL;

    bool     Res;
    wxString Msg;

    p = strtok( aLine, " \t\r\n" );

    if( strcmp( p, "DEF" ) != 0 )
    {
        aErrorMsg.Printf( wxT( "DEF command expected in line %d, aborted." ),
                          *aLineNum );
        return false;
    }

    /* Read DEF line: */
    char drawnum = 0;
    char drawname = 0;

    if( ( componentName = strtok( NULL, " \t\n" ) ) == NULL  /* Part name: */
        || ( prefix = strtok( NULL, " \t\n" ) ) == NULL      /* Prefix name: */
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           /* NumOfPins: */
        || sscanf( p, "%d", &unused ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           /* TextInside: */
        || sscanf( p, "%d", &m_TextInside ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           /* DrawNums: */
        || sscanf( p, "%c", &drawnum ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           /* DrawNums: */
        || sscanf( p, "%c", &drawname ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           /* unitCount: */
        || sscanf( p, "%d", &unitCount ) != 1 )
    {
        aErrorMsg.Printf( wxT( "Wrong DEF format in line %d, skipped." ),
                          *aLineNum );
        while( GetLine( aFile, aLine, aLineNum, 1024 ) )
        {
            p = strtok( aLine, " \t\n" );
            if( stricmp( p, "ENDDEF" ) == 0 )
                break;
        }

        return false;
    }

    // Ensure unitCount is >= 1 (could be read as 0 in old libraries)
    if( unitCount < 1 )
        unitCount = 1;

    m_DrawPinNum  = ( drawnum == 'N' ) ? FALSE : true;
    m_DrawPinName = ( drawname == 'N' ) ? FALSE : true;

    /* Copy part name and prefix. */
    LIB_FIELD& value = GetValueField();

    strupper( componentName );
    if( componentName[0] != '~' )
    {
        name = value.m_Text = CONV_FROM_UTF8( componentName );
    }
    else
    {
        name = value.m_Text = CONV_FROM_UTF8( &componentName[1] );
        value.m_Attributs |= TEXT_NO_VISIBLE;
    }

    LIB_FIELD& reference = GetReferenceField();

    if( strcmp( prefix, "~" ) == 0 )
    {
        reference.m_Text.Empty();
        reference.m_Attributs |= TEXT_NO_VISIBLE;
    }
    else
    {
        reference.m_Text = CONV_FROM_UTF8( prefix );
    }

    // Copy optional infos
    if( ( p = strtok( NULL, " \t\n" ) ) != NULL && *p == 'L' )
        m_UnitSelectionLocked = true;
    if( ( p = strtok( NULL, " \t\n" ) ) != NULL  && *p == 'P' )
        options = ENTRY_POWER;

    /* Read next lines */
    while( GetLine( aFile, aLine, aLineNum, 1024 ) )
    {
        p = strtok( aLine, " \t\n" );

        /* This is the error flag ( if an error occurs, Res = FALSE) */
        Res = true;

        if( (aLine[0] == 'T') && (aLine[1] == 'i') )
            Res = LoadDateAndTime( aLine );
        else if( aLine[0] == 'F' )
            Res = LoadField( aLine, Msg );
        else if( strcmp( p, "ENDDEF" ) == 0 )
            break;
        else if( strcmp( p, "DRAW" ) == 0 )
            Res = LoadDrawEntries( aFile, aLine, aLineNum, Msg );
        else if( strncmp( p, "ALIAS", 5 ) == 0 )
        {
            p = strtok( NULL, "\r\n" );
            Res = LoadAliases( p, aErrorMsg );
        }
        else if( strncmp( p, "$FPLIST", 5 ) == 0 )
            Res = LoadFootprints( aFile, aLine, aLineNum, Msg );

        /* End line or block analysis: test for an error */
        if( !Res )
        {
            if( Msg.IsEmpty() )
                aErrorMsg.Printf( wxT( "error occurred at line %d " ), *aLineNum );
            else
                aErrorMsg.Printf( wxT( "error <%s> occurred at line %d " ),
                                  GetChars( Msg ), *aLineNum );
            return false;
        }
    }

    /* If we are here, this part is O.k. - put it in: */
    drawings.sort();

    return true;
}


bool LIB_COMPONENT::LoadDrawEntries( FILE* aFile, char* aLine,
                                     int* aLineNum, wxString& aErrorMsg )
{
    LIB_DRAW_ITEM* newEntry = NULL;

    while( true )
    {
        if( GetLine( aFile, aLine, aLineNum, 1024 ) == NULL )
        {
            aErrorMsg = wxT( "file ended prematurely loading component draw element" );
            return false;
        }

        if( strncmp( aLine, "ENDDRAW", 7 ) == 0 )
            break;

        newEntry = NULL;

        switch( aLine[0] )
        {
        case 'A':    /* Arc */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_ARC(this);
            break;

        case 'C':    /* Circle */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_CIRCLE(this);
            break;

        case 'T':    /* Text */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_TEXT(this);
            break;

        case 'S':    /* Square */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_RECTANGLE(this);
            break;

        case 'X':    /* Pin Description */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_PIN(this);
            break;

        case 'P':    /* Polyline */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_POLYLINE(this);
            break;

        case 'B':    /* Bezier Curves */
            newEntry = ( LIB_DRAW_ITEM* ) new LIB_BEZIER(this);
            break;

        default:
            aErrorMsg.Printf( wxT( "undefined DRAW command %c" ), aLine[0] );
            return false;
        }

        if( !newEntry->Load( aLine, aErrorMsg ) )
        {
            aErrorMsg.Printf( wxT( "error <%s> in DRAW command %c" ),
                              GetChars( aErrorMsg ), aLine[0] );
            SAFE_DELETE( newEntry );

            /* Flush till end of draw section */
            do
            {
                if( GetLine( aFile, aLine, aLineNum, 1024 ) == NULL )
                {
                    aErrorMsg = wxT( "file ended prematurely while attempting \
to flush to end of drawing section." );
                    return false;
                }
            } while( strncmp( aLine, "ENDDRAW", 7 ) != 0 );

            return false;
        }
        else
        {
            drawings.push_back( newEntry );
        }
    }

    return true;
}


bool LIB_COMPONENT::LoadAliases( char* aLine, wxString& aErrorMsg )
{
    char* text = strtok( aLine, " \t\r\n" );

    while( text )
    {
        m_AliasList.Add( CONV_FROM_UTF8( text ) );
        text = strtok( NULL, " \t\r\n" );
    }

    return true;
}


bool LIB_COMPONENT::LoadField( char* aLine, wxString& aErrorMsg )
{
    LIB_FIELD* field = new LIB_FIELD( this );

    if ( !field->Load( aLine, aErrorMsg ) )
    {
        SAFE_DELETE( field );
        return false;
    }

    if( field->m_FieldId == REFERENCE )
    {
        GetReferenceField() = *field;
        SAFE_DELETE( field );
    }
    else if ( field->m_FieldId == VALUE )
    {
        GetValueField() = *field;
        name = field->m_Text;
        SAFE_DELETE( field );
    }
    else
    {
        drawings.push_back( field );
    }

    return true;
}


bool LIB_COMPONENT::LoadFootprints( FILE* aFile, char* aLine,
                                    int* aLineNum, wxString& aErrorMsg )
{
    while( true )
    {
        if( GetLine( aFile, aLine, aLineNum, 1024 ) == NULL )
        {
            aErrorMsg = wxT( "file ended prematurely while loading footprints" );
            return false;
        }

        if( stricmp( aLine, "$ENDFPLIST" ) == 0 )
            break;

        m_FootprintList.Add( CONV_FROM_UTF8( aLine + 1 ) );
    }

    return true;
}


/**********************************************************************/
/* Return the component boundary box ( in user coordinates )
 *  The unit aUnit, and the shape aConvert are considered.
 *  If aUnit == 0, unit is not used
 *  if aConvert == 0 Convert is non used
 *  Invisible fields are not take in account
 **/
/**********************************************************************/
EDA_Rect LIB_COMPONENT::GetBoundaryBox( int aUnit, int aConvert )
{
    EDA_Rect bBox( wxPoint( 0, 0 ), wxSize( 0, 0 ) );

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( ( item.m_Unit > 0 ) && ( ( unitCount > 1 ) && ( aUnit > 0 )
                                     && ( aUnit != item.m_Unit ) ) )
            continue;
        if( item.m_Convert > 0
            && ( ( aConvert > 0 ) && ( aConvert != item.m_Convert ) ) )
            continue;

        if ( ( item.Type() == COMPONENT_FIELD_DRAW_TYPE )
            && !( ( LIB_FIELD& ) item ).IsVisible() )
            continue;

        bBox.Merge( item.GetBoundingBox() );
    }

    return bBox;
}


/** Function SetFields
 * initialize fields from a vector of fields
 * @param aFields a std::vector <LIB_FIELD> to import.
 */
void LIB_COMPONENT::SetFields( const std::vector <LIB_FIELD> aFields )
{
    LIB_FIELD* field;

    for( size_t i = 0; i < aFields.size(); i++ )
    {
        field = GetField( aFields[i].m_FieldId );

        if( field )
        {
            *field = aFields[i];

            if( (int) i == VALUE )
                name = field->m_Text;

            continue;
        }

        /* If the field isn't set, don't add it to the component. */
        if( aFields[i].m_Text.IsEmpty() )
            continue;

        field = new LIB_FIELD( aFields[i] );
        drawings.push_back( field );
    }

    drawings.sort();
}


void LIB_COMPONENT::GetFields( LIB_FIELD_LIST& aList )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( item.Type() != COMPONENT_FIELD_DRAW_TYPE )
            continue;

        LIB_FIELD* field = ( LIB_FIELD* ) &item;
        aList.push_back( *field );
    }
}


LIB_FIELD* LIB_COMPONENT::GetField( int aId )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( item.Type() != COMPONENT_FIELD_DRAW_TYPE )
            continue;

        LIB_FIELD* field = ( LIB_FIELD* ) &item;

        if( field->m_FieldId == aId )
            return field;
    }

    return NULL;
}


LIB_FIELD& LIB_COMPONENT::GetValueField()
{
    LIB_FIELD* field = GetField( VALUE );
    wxASSERT( field != NULL );
    return *field;
}


LIB_FIELD& LIB_COMPONENT::GetReferenceField()
{
    LIB_FIELD* field = GetField( REFERENCE );
    wxASSERT( field != NULL );
    return *field;
}


/*
 * Read date and time of component in the format:
 *  "Ti yy/mm/jj hh:mm:ss"
 */
bool LIB_COMPONENT::SaveDateAndTime( FILE* aFile )
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

    if ( fprintf( aFile, "Ti %d/%d/%d %d:%d:%d\n",
                  year, mon, day, hour, min, sec ) < 0 )
        return false;

    return true;
}

/* lit date et time de modif composant sous le format:
 *  "Ti yy/mm/jj hh:mm:ss"
 */
bool LIB_COMPONENT::LoadDateAndTime( char* aLine )
{
    int   year, mon, day, hour, min, sec;
    char* text;

    year = mon = day = hour = min = sec = 0;
    text = strtok( aLine, " \r\t\n" );
    text = strtok( NULL, " \r\t\n" );

    if (sscanf( aLine, "%d/%d/%d %d:%d:%d",
                &year, &mon, &day, &hour, &min, &sec ) != 6 )
        return false;

    m_LastDate = ( sec & 63 ) + ( ( min & 63 ) << 6 ) +
        ( ( hour & 31 ) << 12 ) + ( ( day & 31 ) << 17 ) +
        ( ( mon & 15 ) << 22 ) + ( ( year - 1990 ) << 26 );

    return true;
}


void LIB_COMPONENT::SetOffset( const wxPoint& aOffset )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        item.SetOffset( aOffset );
    }
}


void LIB_COMPONENT::RemoveDuplicateDrawItems()
{
    drawings.unique();
}


bool LIB_COMPONENT::HasConversion() const
{
    BOOST_FOREACH( const LIB_DRAW_ITEM& item, drawings )
    {
        if( item.m_Convert > 1 )
            return true;
    }

    return false;
}


void LIB_COMPONENT::ClearStatus()
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
        item.m_Flags = 0;
}


int LIB_COMPONENT::SelectItems( EDA_Rect& aRect, int aUnit, int aConvert,
                                bool aEditPinByPin )
{
    int itemCount = 0;

    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        item.m_Selected = 0;

        if( ( item.m_Unit && item.m_Unit != aUnit )
            || ( item.m_Convert && item.m_Convert != aConvert ) )
        {
            if( item.Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;

             // Specific rules for pins.
            if( aEditPinByPin || m_UnitSelectionLocked
                || ( item.m_Convert && item.m_Convert != aConvert ) )
                continue;
        }

        if( item.Inside( aRect ) )
        {
            item.m_Selected = IS_SELECTED;
            itemCount++;
        }
    }

    return itemCount;
}


void LIB_COMPONENT::MoveSelectedItems( const wxPoint& aOffset )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( item.m_Selected == 0 )
            continue;

        item.SetOffset( aOffset );
        item.m_Flags = item.m_Selected = 0;
    }

    drawings.sort();
}


void LIB_COMPONENT::ClearSelectedItems()
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
        item.m_Flags = item.m_Selected = 0;
}


void LIB_COMPONENT::DeleteSelectedItems()
{
    LIB_DRAW_ITEM_LIST::iterator i = drawings.begin();

    while( i != drawings.end() )
    {
        if( i->m_Selected == 0 )
            i++;
        else
            i = drawings.erase( i );
    }
}


void LIB_COMPONENT::CopySelectedItems( const wxPoint& aOffset )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( item.m_Selected == 0 )
            continue;

        item.m_Selected = 0;
        LIB_DRAW_ITEM* newItem = item.GenCopy();
        newItem->m_Selected = IS_SELECTED;
        drawings.push_back( newItem );
    }

    MoveSelectedItems( aOffset );
    drawings.sort();
}


void LIB_COMPONENT::MirrorSelectedItemsH( const wxPoint& aCenter )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( item.m_Selected == 0 )
            continue;

        item.SetOffset( aCenter );
        item.m_Flags = item.m_Selected = 0;
    }

    drawings.sort();
}



/**
 * Locate a draw object.
 *
 * @param aUnit - Unit number of draw item.
 * @param aConvert - Body style of draw item.
 * @param aType - Draw object type, set to 0 to search for any type.
 * @param aPoint - Coordinate for hit testing.
 *
 * @return LIB_DRAW_ITEM - Pointer the the draw object if found.
 *                         Otherwise NULL.
 */
LIB_DRAW_ITEM* LIB_COMPONENT::LocateDrawItem( int aUnit, int aConvert,
                                              KICAD_T aType, const wxPoint& aPoint )
{
    BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
    {
        if( ( aUnit && item.m_Unit && ( aUnit != item.m_Unit) )
            || ( aConvert && item.m_Convert && ( aConvert != item.m_Convert ) )
            || ( ( item.Type() != aType ) && ( aType != TYPE_NOT_INIT ) ) )
            continue;

        if( item.HitTest( aPoint ) )
            return &item;
    }

    return NULL;
}

/** Function HitTest (overlaid)
 * @return true if the point aPosRef is near this object
 * @param aPosRef = a wxPoint to test
 * @param aThreshold = max distance to this object (usually the half
 *                     thickness of a line)
 * @param aTransform = the transform matrix
 *
 * @return LIB_DRAW_ITEM - Pointer the the draw object if found.
 *                         Otherwise NULL.
 */
LIB_DRAW_ITEM* LIB_COMPONENT::LocateDrawItem( int aUnit, int aConvert, KICAD_T aType,
                                              const wxPoint& aPoint, const int aTransform[2][2] )
{
    /* we use LocateDrawItem( int aUnit, int convert, KICAD_T type, const
     * wxPoint& pt ) to search items.
     * because this function uses DefaultTransformMatrix as orient/mirror matrix
     * we temporary copy aTransMat in DefaultTransformMatrix
     */
    LIB_DRAW_ITEM * item;
    int matrix[2][2];
    for ( int ii = 0; ii < 2; ii++ )
    {
        for ( int jj = 0; jj < 2; jj++ )
        {
            matrix[ii][jj] = aTransform[ii][jj];
            EXCHG( matrix[ii][jj], DefaultTransformMatrix[ii][jj] );
        }
    }
    item = LocateDrawItem( aUnit, aConvert, aType, aPoint );
    //Restore matrix
    for ( int ii = 0; ii < 2; ii++ )
    {
        for ( int jj = 0; jj < 2; jj++ )
        {
            EXCHG( matrix[ii][jj], DefaultTransformMatrix[ii][jj] );
        }
    }

    return item;
}


void LIB_COMPONENT::SetPartCount( int aCount )
{
    if( unitCount == aCount )
        return;

    if( aCount < unitCount )
    {
        LIB_DRAW_ITEM_LIST::iterator i;
        i = drawings.begin();

        while( i != drawings.end() )
        {
            if( i->m_Unit > aCount )
                i = drawings.erase( i );
            else
                i++;
        }
    }
    else
    {
        int prevCount = unitCount;

        // We cannot use an iterator here, because when adding items in vector
        // the buffer can be reallocated, that change the previous value of
        // .begin() and .end() iterators and invalidate others iterators
        unsigned imax = drawings.size();
        for( unsigned ii = 0; ii < imax; ii++ )
        {
            if( drawings[ii].m_Unit != 1 )
                continue;

            for( int j = prevCount + 1; j <= aCount; j++ )
            {
                LIB_DRAW_ITEM* newItem = drawings[ii].GenCopy();
                newItem->m_Unit = j;
                drawings.push_back( newItem );
            }
        }

        drawings.sort();
    }

    unitCount = aCount;
}


void LIB_COMPONENT::SetConversion( bool aSetConvert )
{
    if( aSetConvert == HasConversion() )
        return;

    // Duplicate items to create the converted shape
    if( aSetConvert )
    {

        BOOST_FOREACH( LIB_DRAW_ITEM& item, drawings )
        {
            /* Only pins are duplicated. */
            if( item.Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( item.m_Convert == 1 )
            {
                LIB_DRAW_ITEM* newItem = item.GenCopy();
                newItem->m_Convert = 2;
                drawings.push_back( newItem );
            }
        }
    }
    else
    {
        // Delete converted shape items because the converted shape does
        // not exist
        LIB_DRAW_ITEM_LIST::iterator i = drawings.begin();

        while( i != drawings.end() )
        {
            if( i->m_Convert > 1 )
                i = drawings.erase( i );
            else
                i++;
        }
    }
}
