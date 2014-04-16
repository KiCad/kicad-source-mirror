/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_libentry.cpp
 */

#include <fctsys.h>
#include <macros.h>
#include <kicad_string.h>
#include <class_drawpanel.h>
#include <plot_common.h>
#include <gr_basic.h>
#include <class_sch_screen.h>
#include <richio.h>

#include <general.h>
#include <template_fieldnames.h>
#include <transform.h>
#include <class_library.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <lib_arc.h>
#include <lib_bezier.h>
#include <lib_circle.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>

#include <boost/foreach.hpp>

// Set this to 1 to print debugging output in alias and component destructors to verify
// objects get cleaned up properly.
#if defined( TRACE_DESTRUCTOR )
#undef TRACE_DESTRUCTOR
#endif

#define TRACE_DESTRUCTOR 0

// the separator char between the subpart id and the reference
// 0 (no separator) or '.' or some other character
int LIB_COMPONENT::m_subpartIdSeparator = 0;
// the ascii char value to calculate the subpart symbol id from the part number:
// 'A' or '1' usually. (to print U1.A or U1.1)
// if this a a digit, a number is used as id symbol
int LIB_COMPONENT::m_subpartFirstId = 'A';


LIB_ALIAS::LIB_ALIAS( const wxString& aName, LIB_COMPONENT* aRootComponent ):
    EDA_ITEM( LIB_ALIAS_T )
{
    root = aRootComponent;
    name = aName;
}


LIB_ALIAS::LIB_ALIAS( const LIB_ALIAS& aAlias, LIB_COMPONENT* aRootComponent ) :
    EDA_ITEM( aAlias )
{
    name = aAlias.name;
    root = aRootComponent;
    description = aAlias.description;
    keyWords = aAlias.keyWords;
    docFileName = aAlias.docFileName;
}


LIB_ALIAS::~LIB_ALIAS()
{
#if TRACE_DESTRUCTOR
    wxLogDebug( wxT( "Destroying alias \"%s\" of component \"%s\" with alias list count %d." ),
                GetChars( name ), GetChars( root->GetName() ), root->m_aliases.size() );
#endif
}


wxString LIB_ALIAS::GetLibraryName()
{
    if( GetComponent() )
        return GetComponent()->GetLibraryName();

    return wxString( _( "none" ) );
}


bool LIB_ALIAS::IsRoot() const
{
    return name.CmpNoCase( root->GetName() ) == 0;
}

CMP_LIBRARY* LIB_ALIAS::GetLibrary()
{
    return root->GetLibrary();
}


bool LIB_ALIAS::SaveDoc( OUTPUTFORMATTER& aFormatter )
{
    if( description.IsEmpty() && keyWords.IsEmpty() && docFileName.IsEmpty() )
        return true;

    try
    {
        aFormatter.Print( 0, "#\n$CMP %s\n", TO_UTF8( name ) );

        if( !description.IsEmpty() )
            aFormatter.Print( 0, "D %s\n", TO_UTF8( description ) );

        if( !keyWords.IsEmpty() )
            aFormatter.Print( 0, "K %s\n", TO_UTF8( keyWords ) );

        if( !docFileName.IsEmpty() )
            aFormatter.Print( 0, "F %s\n", TO_UTF8( docFileName ) );

        aFormatter.Print( 0, "$ENDCMP\n" );
    }
    catch( const IO_ERROR& ioe )
    {
        return false;
    }

    return true;
}


bool LIB_ALIAS::operator==( const wxChar* aName ) const
{
    return name.CmpNoCase( aName ) == 0;
}


bool operator<( const LIB_ALIAS& aItem1, const LIB_ALIAS& aItem2 )
{
    return aItem1.GetName().CmpNoCase( aItem2.GetName() ) < 0;
}


int LibraryEntryCompare( const LIB_ALIAS* aItem1, const LIB_ALIAS* aItem2 )
{
    return aItem1->GetName().CmpNoCase( aItem2->GetName() );
}


LIB_COMPONENT::LIB_COMPONENT( const wxString& aName, CMP_LIBRARY* aLibrary ) :
    EDA_ITEM( LIB_COMPONENT_T )
{
    m_name                = aName;
    m_library             = aLibrary;
    m_dateModified        = 0;
    m_unitCount           = 1;
    m_pinNameOffset       = 40;
    m_options             = ENTRY_NORMAL;
    m_unitsLocked         = false;
    m_showPinNumbers      = true;
    m_showPinNames        = true;

    // Create the default alias if the name parameter is not empty.
    if( !aName.IsEmpty() )
        m_aliases.push_back( new LIB_ALIAS( aName, this ) );

    // Add the MANDATORY_FIELDS in RAM only.  These are assumed to be present
    // when the field editors are invoked.
    LIB_FIELD* value = new LIB_FIELD( this, VALUE );
    value->SetText( aName );
    drawings.push_back( value );

    drawings.push_back( new LIB_FIELD( this, REFERENCE ) );
    drawings.push_back( new LIB_FIELD( this, FOOTPRINT ) );
    drawings.push_back( new LIB_FIELD( this, DATASHEET ) );
}


LIB_COMPONENT::LIB_COMPONENT( LIB_COMPONENT& aComponent, CMP_LIBRARY* aLibrary ) :
    EDA_ITEM( aComponent )
{
    LIB_ITEM* newItem;

    m_library             = aLibrary;
    m_name                = aComponent.m_name;
    m_FootprintList       = aComponent.m_FootprintList;
    m_unitCount           = aComponent.m_unitCount;
    m_unitsLocked         = aComponent.m_unitsLocked;
    m_pinNameOffset       = aComponent.m_pinNameOffset;
    m_showPinNumbers      = aComponent.m_showPinNumbers;
    m_showPinNames        = aComponent.m_showPinNames;
    m_dateModified        = aComponent.m_dateModified;
    m_options             = aComponent.m_options;

    BOOST_FOREACH( LIB_ITEM& oldItem, aComponent.GetDrawItemList() )
    {
        if( oldItem.IsNew() )
            continue;

        newItem = (LIB_ITEM*) oldItem.Clone();
        newItem->SetParent( this );
        drawings.push_back( newItem );
    }

    for( size_t i = 0; i < aComponent.m_aliases.size(); i++ )
    {
        LIB_ALIAS* alias = new LIB_ALIAS( *aComponent.m_aliases[i], this );
        m_aliases.push_back( alias );
    }
}


LIB_COMPONENT::~LIB_COMPONENT()
{
#if TRACE_DESTRUCTOR
    wxLogDebug( wxT( "Destroying component <%s> with alias list count of %d" ),
                GetChars( GetName() ), m_aliases.size() );
#endif

    // If the component is being delete directly rather than trough the library, free all
    // of the memory allocated by the aliases.
    if( !m_aliases.empty() )
    {
        LIB_ALIAS* alias;

        while( !m_aliases.empty() )
        {
            alias = m_aliases.back();
            m_aliases.pop_back();
            delete alias;
        }
    }
}


wxString LIB_COMPONENT::GetLibraryName()
{
    if( m_library != NULL )
        return m_library->GetName();

    return wxString( _( "none" ) );
}


wxString LIB_COMPONENT::SubReference( int aUnit, bool aAddSeparator )
{
    wxString subRef;

    if( m_subpartIdSeparator != 0 && aAddSeparator )
        subRef << wxChar( m_subpartIdSeparator );

    if( m_subpartFirstId >= '0' && m_subpartFirstId <= '9' )
        subRef << aUnit;
    else
        subRef << wxChar( m_subpartFirstId + aUnit - 1);

    return subRef;
}


void LIB_COMPONENT::SetName( const wxString& aName )
{
    m_name = aName;
    GetValueField().SetText( aName );
    m_aliases[0]->SetName( aName );
}


void LIB_COMPONENT::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDc, const wxPoint& aOffset, int aMulti,
                          int aConvert, GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor, const TRANSFORM& aTransform,
                          bool aShowPinText, bool aDrawFields, bool aOnlySelected )
{
    BASE_SCREEN*   screen = aPanel ? aPanel->GetScreen() : NULL;

    GRSetDrawMode( aDc, aDrawMode );

    /* draw background for filled items using background option
     * Solid lines will be drawn after the background
     * Note also, background is not drawn when:
     *   printing in black and white
     *   If the color is not the default color (aColor != -1 )
     */
    if( ! (screen && screen->m_IsPrinting && GetGRForceBlackPenState())
            && (aColor == UNSPECIFIED_COLOR) )
    {
        BOOST_FOREACH( LIB_ITEM& drawItem, drawings )
        {
            if( drawItem.m_Fill != FILLED_WITH_BG_BODYCOLOR )
                continue;

            if( aOnlySelected && !drawItem.IsSelected() )
                continue;

            // Do not draw an item while moving (the cursor handler does that)
            if( drawItem.m_Flags & IS_MOVED )
                continue;

            // Do not draw items not attached to the current part
            if( aMulti && drawItem.m_Unit && ( drawItem.m_Unit != aMulti ) )
                continue;

            if( aConvert && drawItem.m_Convert && ( drawItem.m_Convert != aConvert ) )
                continue;

            if( drawItem.Type() == LIB_FIELD_T )
                continue;

            if( drawItem.Type() == LIB_FIELD_T )
            {
                drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode, (void*) NULL, aTransform );
            }

            // Now, draw only the background for items with
            // m_Fill == FILLED_WITH_BG_BODYCOLOR:
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode, (void*) false, aTransform );
        }
    }

    BOOST_FOREACH( LIB_ITEM& drawItem, drawings )
    {
        if( aOnlySelected && !drawItem.IsSelected() )
            continue;

        // Do not draw an item while moving (the cursor handler does that)
        if( drawItem.m_Flags & IS_MOVED )
            continue;

        // Do not draw items not attached to the current part
        if( aMulti && drawItem.m_Unit && ( drawItem.m_Unit != aMulti ) )
            continue;

        if( aConvert && drawItem.m_Convert && ( drawItem.m_Convert != aConvert ) )
            continue;

        if( !aDrawFields && drawItem.Type() == LIB_FIELD_T )
            continue;

        if( drawItem.Type() == LIB_PIN_T )
        {
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode, (void*) aShowPinText,
                           aTransform );
        }
        else if( drawItem.Type() == LIB_FIELD_T )
        {
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode, (void*) NULL, aTransform );
        }
        else
        {
            bool forceNoFill = drawItem.m_Fill == FILLED_WITH_BG_BODYCOLOR;
            drawItem.Draw( aPanel, aDc, aOffset, aColor, aDrawMode, (void*) forceNoFill,
                           aTransform );
        }

    }

    // Enable this to draw the anchor of the component.
#if 0
    int len = aDc->DeviceToLogicalXRel( 3 );
    EDA_RECT* const clipbox  = aPanel ? aPanel->GetClipBox() : NULL;

    GRLine( clipbox, aDc, aOffset.x, aOffset.y - len, aOffset.x,
            aOffset.y + len, 0, aColor );
    GRLine( clipbox, aDc, aOffset.x - len, aOffset.y, aOffset.x + len,
            aOffset.y, 0, aColor );
#endif

    /* Enable this to draw the bounding box around the component to validate
     * the bounding box calculations. */
#if 0
    EDA_RECT bBox = GetBoundingBox( aMulti, aConvert );
    GRRect( aPanel ? aPanel->GetClipBox() : NULL, aDc, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void LIB_COMPONENT::Plot( PLOTTER* aPlotter, int aUnit, int aConvert,
                          const wxPoint& aOffset, const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
    bool fill = aPlotter->GetColorMode();

    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        // Lib Fields not are plotted here, because this plot function
        // is used to plot schematic items, which have they own fields
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
            continue;

        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        item.Plot( aPlotter, aOffset, fill, aTransform );
    }
}

void LIB_COMPONENT::PlotLibFields( PLOTTER* aPlotter, int aUnit, int aConvert,
                                  const wxPoint& aOffset, const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    aPlotter->SetColor( GetLayerColor( LAYER_FIELDS ) );
    bool fill = aPlotter->GetColorMode();

    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
            continue;

        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        // The reference is a special case: we shoud change the basic text
        // to add '?' and the part id
        LIB_FIELD& field = (LIB_FIELD&) item;
        wxString tmp = field.GetText();
        if( field.GetId() == REFERENCE )
        {
            wxString text = field.GetFullText( aUnit );
            field.SetText( text );
        }
        item.Plot( aPlotter, aOffset, fill, aTransform );
        field.SetText( tmp );
    }
}


void LIB_COMPONENT::RemoveDrawItem( LIB_ITEM* aItem, EDA_DRAW_PANEL* aPanel, wxDC* aDc )
{
    wxASSERT( aItem != NULL );

    // none of the MANDATOR_FIELDS may be removed in RAM, but they may be
    // omitted when saving to disk.
    if( aItem->Type() == LIB_FIELD_T )
    {
        LIB_FIELD* field = (LIB_FIELD*) aItem;

        if( field->GetId() < MANDATORY_FIELDS )
        {
            wxLogWarning( _( "An attempt was made to remove the %s field \
from component %s in library %s." ),
                          GetChars( field->GetName() ), GetChars( GetName() ),
                          GetChars( GetLibraryName() ) );
            return;
        }
    }

    LIB_ITEMS::iterator i;

    for( i = drawings.begin(); i < drawings.end(); i++ )
    {
        if( *i == aItem )
        {
            if( aDc != NULL )
                aItem->Draw( aPanel, aDc, wxPoint( 0, 0 ), UNSPECIFIED_COLOR, g_XorMode, NULL, DefaultTransform );

            drawings.erase( i );
            SetModified();
            break;
        }
    }
}


void LIB_COMPONENT::AddDrawItem( LIB_ITEM* aItem )
{
    wxASSERT( aItem != NULL );

    drawings.push_back( aItem );
    drawings.sort();
}


LIB_ITEM* LIB_COMPONENT::GetNextDrawItem( LIB_ITEM* aItem, KICAD_T aType )
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


void LIB_COMPONENT::GetPins( LIB_PINS& aList, int aUnit, int aConvert )
{
    /* Notes:
     * when aUnit == 0: no unit filtering
     * when aConvert == 0: no convert (shape selection) filtering
     * when .m_Unit == 0, the body item is common to units
     * when .m_Convert == 0, the body item is common to shapes
     */
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( item.Type() != LIB_PIN_T )    // we search pins only
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
    wxString pNumber;
    LIB_PINS pinList;

    GetPins( pinList, aUnit, aConvert );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        wxASSERT( pinList[i]->Type() == LIB_PIN_T );

        pinList[i]->PinStringNum( pNumber );

        if( aNumber == pNumber )
            return pinList[i];
    }

    return NULL;
}


bool LIB_COMPONENT::Save( OUTPUTFORMATTER& aFormatter )
{
    LIB_FIELD&  value = GetValueField();

    // First line: it s a comment (component name for readers)
    aFormatter.Print( 0, "#\n# %s\n#\n", TO_UTF8( value.GetText() ) );

    // Save data
    aFormatter.Print( 0, "DEF" );

#if 0 && defined(DEBUG)
    if( value.GetText() == wxT( "R" ) )
    {
        int breakhere = 1;
        (void) breakhere;
    }
#endif

    if( value.IsVisible() )
    {
        aFormatter.Print( 0, " %s", TO_UTF8( value.GetText() ) );
    }
    else
    {
        aFormatter.Print( 0, " ~%s", TO_UTF8( value.GetText() ) );
    }

    LIB_FIELD& reference = GetReferenceField();

    if( !reference.GetText().IsEmpty() )
    {
        aFormatter.Print( 0, " %s", TO_UTF8( reference.GetText() ) );
    }
    else
    {
        aFormatter.Print( 0, " ~" );
    }

    aFormatter.Print( 0, " %d %d %c %c %d %c %c\n",
                      0, m_pinNameOffset,
                      m_showPinNumbers ? 'Y' : 'N',
                      m_showPinNames ? 'Y' : 'N',
                      m_unitCount, m_unitsLocked ? 'L' : 'F',
                      m_options == ENTRY_POWER ? 'P' : 'N' );

    if( !SaveDateAndTime( aFormatter ) )
        return false;

    LIB_FIELDS fields;
    GetFields( fields );

    // Mandatory fields:
    // may have their own save policy so there is a separate loop for them.
    // Empty fields are saved, because the user may have set visibility,
    // size and orientation
    for( int i = 0;  i < MANDATORY_FIELDS;  ++i )
    {
        if( !fields[i].Save( aFormatter ) )
            return false;
    }

    // User defined fields:
    // may have their own save policy so there is a separate loop for them.

    int fieldId = MANDATORY_FIELDS;     // really wish this would go away.

    for( unsigned i = MANDATORY_FIELDS; i < fields.size(); ++i )
    {
        // There is no need to save empty fields, i.e. no reason to preserve field
        // names now that fields names come in dynamically through the template
        // fieldnames.
        if( !fields[i].GetText().IsEmpty() )
        {
            fields[i].SetId( fieldId++ );

            if( !fields[i].Save( aFormatter ) )
                return false;
        }
    }

    // Save the alias list: a line starting by "ALIAS".  The first alias is the root
    // and has the same name as the component.  In the old library file format this
    // alias does not get added to the alias list.
    if( m_aliases.size() > 1 )
    {
        aFormatter.Print( 0, "ALIAS" );

        for( unsigned i = 1; i < m_aliases.size(); i++ )
        {
            aFormatter.Print( 0, " %s", TO_UTF8( m_aliases[i]->GetName() ) );
        }

        aFormatter.Print( 0, "\n" );
    }

    // Write the footprint filter list
    if( m_FootprintList.GetCount() != 0 )
    {
        aFormatter.Print( 0, "$FPLIST\n" );

        for( unsigned i = 0; i < m_FootprintList.GetCount(); i++ )
        {
            aFormatter.Print( 0, " %s\n", TO_UTF8( m_FootprintList[i] ) );
        }

        aFormatter.Print( 0, "$ENDFPLIST\n" );
    }

    // Save graphics items (including pins)
    if( !drawings.empty() )
    {
        /* we sort the draw items, in order to have an edition more easy,
         *  when a file editing "by hand" is made */
        drawings.sort();

        aFormatter.Print( 0, "DRAW\n" );

        BOOST_FOREACH( LIB_ITEM& item, drawings )
        {
            if( item.Type() == LIB_FIELD_T )
                continue;

            if( !item.Save( aFormatter ) )
                return false;
        }

        aFormatter.Print( 0, "ENDDRAW\n" );
    }

    aFormatter.Print( 0, "ENDDEF\n" );

    return true;
}


bool LIB_COMPONENT::Load( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    int      unused;
    char*    p;
    char*    componentName;
    char*    prefix = NULL;
    char*    line;

    bool     result;
    wxString Msg;

    line = aLineReader.Line();

    p = strtok( line, " \t\r\n" );

    if( strcmp( p, "DEF" ) != 0 )
    {
        aErrorMsg.Printf( wxT( "DEF command expected in line %d, aborted." ),
                          aLineReader.LineNumber() );
        return false;
    }

    // Read DEF line:
    char drawnum = 0;
    char drawname = 0;

    if( ( componentName = strtok( NULL, " \t\n" ) ) == NULL  // Part name:
        || ( prefix = strtok( NULL, " \t\n" ) ) == NULL      // Prefix name:
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           // NumOfPins:
        || sscanf( p, "%d", &unused ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           // TextInside:
        || sscanf( p, "%d", &m_pinNameOffset ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           // DrawNums:
        || sscanf( p, "%c", &drawnum ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           // DrawNums:
        || sscanf( p, "%c", &drawname ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL           // m_unitCount:
        || sscanf( p, "%d", &m_unitCount ) != 1 )
    {
        aErrorMsg.Printf( wxT( "Wrong DEF format in line %d, skipped." ),
                          aLineReader.LineNumber() );

        while( (line = aLineReader.ReadLine()) != NULL )
        {
            p = strtok( line, " \t\n" );

            if( stricmp( p, "ENDDEF" ) == 0 )
                break;
        }

        return false;
    }

    // Ensure m_unitCount is >= 1 (could be read as 0 in old libraries)
    if( m_unitCount < 1 )
        m_unitCount = 1;

    m_showPinNumbers = ( drawnum == 'N' ) ? false : true;
    m_showPinNames = ( drawname == 'N' ) ? false : true;

    // Copy part name and prefix.
    LIB_FIELD& value = GetValueField();

    if( componentName[0] != '~' )
    {
        m_name = FROM_UTF8( componentName );

#ifndef KICAD_KEEPCASE
        m_name = m_name.MakeUpper();
#endif

        value.SetText( m_name );
    }
    else
    {
        m_name = FROM_UTF8( &componentName[1] );
        value.SetText( m_name );
        value.SetVisible( false );
    }

    // Add the root alias to the alias list.
    m_aliases.push_back( new LIB_ALIAS( m_name, this ) );

    LIB_FIELD& reference = GetReferenceField();

    if( strcmp( prefix, "~" ) == 0 )
    {
        reference.Empty();
        reference.SetVisible( false );
    }
    else
    {
        reference.SetText( FROM_UTF8( prefix ) );
    }

    // Copy optional infos
    if( ( p = strtok( NULL, " \t\n" ) ) != NULL && *p == 'L' )
        m_unitsLocked = true;

    if( ( p = strtok( NULL, " \t\n" ) ) != NULL  && *p == 'P' )
        m_options = ENTRY_POWER;

    // Read next lines, until "ENDDEF" is found
    while( ( line = aLineReader.ReadLine() ) != NULL )
    {
        p = strtok( line, " \t\r\n" );

        // This is the error flag ( if an error occurs, result = false)
        result = true;

        if( *line == '#' )      // a comment
            continue;

        if( line[0] == 'T'  &&  line[1] == 'i' )
            result = LoadDateAndTime( aLineReader );
        else if( *line == 'F' )
            result = LoadField( aLineReader, Msg );
        else if( strcmp( p, "ENDDEF" ) == 0 )   // End of component description
            goto ok;
        else if( strcmp( p, "DRAW" ) == 0 )
            result = LoadDrawEntries( aLineReader, Msg );
        else if( strncmp( p, "ALIAS", 5 ) == 0 )
        {
            p = strtok( NULL, "\r\n" );
            result = LoadAliases( p, aErrorMsg );
        }
        else if( strncmp( p, "$FPLIST", 5 ) == 0 )
            result = LoadFootprints( aLineReader, Msg );

        // End line or block analysis: test for an error
        if( !result )
        {
            if( Msg.IsEmpty() )
                aErrorMsg.Printf( wxT( "error occurred at line %d " ), aLineReader.LineNumber() );
            else
                aErrorMsg.Printf( wxT( "error <%s> occurred at line %d " ),
                                  GetChars( Msg ), aLineReader.LineNumber() );

            return false;
        }
    }

    return false;

ok:
    // If we are here, this part is O.k. - put it in:
    drawings.sort();

    return true;
}


bool LIB_COMPONENT::LoadDrawEntries( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    char* line;
    LIB_ITEM* newEntry = NULL;

    while( true )
    {
        if( !( line = aLineReader.ReadLine() ) )
        {
            aErrorMsg = wxT( "file ended prematurely loading component draw element" );
            return false;
        }

        if( strncmp( line, "ENDDRAW", 7 ) == 0 )
            break;

        newEntry = NULL;

        switch( line[0] )
        {
        case 'A':    // Arc
            newEntry = ( LIB_ITEM* ) new LIB_ARC( this );
            break;

        case 'C':    // Circle
            newEntry = ( LIB_ITEM* ) new LIB_CIRCLE( this );
            break;

        case 'T':    // Text
            newEntry = ( LIB_ITEM* ) new LIB_TEXT( this );
            break;

        case 'S':    // Square
            newEntry = ( LIB_ITEM* ) new LIB_RECTANGLE( this );
            break;

        case 'X':    // Pin Description
            newEntry = ( LIB_ITEM* ) new LIB_PIN( this );
            break;

        case 'P':    // Polyline
            newEntry = ( LIB_ITEM* ) new LIB_POLYLINE( this );
            break;

        case 'B':    // Bezier Curves
            newEntry = ( LIB_ITEM* ) new LIB_BEZIER( this );
            break;

    case '#':    // Comment
            continue;

        default:
            aErrorMsg.Printf( wxT( "undefined DRAW command %c" ), line[0] );
            return false;
        }

        if( !newEntry->Load( aLineReader, aErrorMsg ) )
        {
            aErrorMsg.Printf( wxT( "error <%s> in DRAW command %c" ),
                              GetChars( aErrorMsg ), line[0] );
            delete newEntry;

            // Flush till end of draw section
            do
            {
                if( !aLineReader.ReadLine() )
                {
                    aErrorMsg = wxT( "file ended prematurely while attempting "
                                     "to flush to end of drawing section." );
                    return false;
                }
            } while( strncmp( line, "ENDDRAW", 7 ) != 0 );

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
        m_aliases.push_back( new LIB_ALIAS( FROM_UTF8( text ), this ) );
        text = strtok( NULL, " \t\r\n" );
    }

    return true;
}


bool LIB_COMPONENT::LoadField( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    LIB_FIELD* field = new LIB_FIELD( this );

    if( !field->Load( aLineReader, aErrorMsg ) )
    {
        delete field;
        return false;
    }

    if( field->GetId() < MANDATORY_FIELDS )
    {
        LIB_FIELD* fixedField = GetField( field->GetId() );

        // this will fire only if somebody broke a constructor or editor.
        // MANDATORY_FIELDS are always present in ram resident components, no
        // exceptions, and they always have their names set, even fixed fields.
        wxASSERT( fixedField );

        *fixedField = *field;

        if( field->GetId() == VALUE )
            m_name = field->GetText();

        delete field;
    }
    else
    {
        drawings.push_back( field );
    }

    return true;
}


bool LIB_COMPONENT::LoadFootprints( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    char* line;
    char* p;

    while( true )
    {
        if( !( line = aLineReader.ReadLine() ) )
        {
            aErrorMsg = wxT( "file ended prematurely while loading footprints" );
            return false;
        }

        p = strtok( line, " \t\r\n" );

        if( stricmp( p, "$ENDFPLIST" ) == 0 )
            break;

        m_FootprintList.Add( FROM_UTF8( p ) );
    }

    return true;
}


EDA_RECT LIB_COMPONENT::GetBoundingBox( int aUnit, int aConvert ) const
{
    EDA_RECT bBox( wxPoint( 0, 0 ), wxSize( 0, 0 ) );

    for( unsigned ii = 0; ii < drawings.size(); ii++  )
    {
        const LIB_ITEM& item = drawings[ii];

        if( ( item.m_Unit > 0 ) && ( ( m_unitCount > 1 ) && ( aUnit > 0 )
                                     && ( aUnit != item.m_Unit ) ) )
            continue;

        if( item.m_Convert > 0 && ( ( aConvert > 0 ) && ( aConvert != item.m_Convert ) ) )
            continue;

        if ( ( item.Type() == LIB_FIELD_T ) && !( ( LIB_FIELD& ) item ).IsVisible() )
            continue;

        bBox.Merge( item.GetBoundingBox() );
    }

    return bBox;
}


EDA_RECT LIB_COMPONENT::GetBodyBoundingBox( int aUnit, int aConvert ) const
{
    EDA_RECT bBox( wxPoint( 0, 0 ), wxSize( 0, 0 ) );

    for( unsigned ii = 0; ii < drawings.size(); ii++  )
    {
        const LIB_ITEM& item = drawings[ii];

        if( ( item.m_Unit > 0 ) && ( ( m_unitCount > 1 ) && ( aUnit > 0 )
                                     && ( aUnit != item.m_Unit ) ) )
            continue;

        if( item.m_Convert > 0 && ( ( aConvert > 0 ) && ( aConvert != item.m_Convert ) ) )
            continue;

        if ( item.Type() == LIB_FIELD_T )
            continue;

        bBox.Merge( item.GetBoundingBox() );
    }

    return bBox;
}


void LIB_COMPONENT::deleteAllFields()
{
    LIB_ITEMS::iterator it;

    for( it = drawings.begin();  it!=drawings.end();  /* deleting */  )
    {
        if( it->Type() != LIB_FIELD_T  )
        {
            ++it;
            continue;
        }

        // 'it' is not advanced, but should point to next in list after erase()
        it = drawings.erase( it );
    }
}


void LIB_COMPONENT::SetFields( const std::vector <LIB_FIELD>& aFields )
{
    deleteAllFields();

    for( unsigned i=0;  i<aFields.size();  ++i )
    {
        // drawings is a ptr_vector, new and copy an object on the heap.
        LIB_FIELD* field = new LIB_FIELD( aFields[i] );

        drawings.push_back( field );
    }

    // Reorder drawings: transparent polygons first, pins and text last.
    // so texts have priority on screen.
    drawings.sort();
}


void LIB_COMPONENT::GetFields( LIB_FIELDS& aList )
{
    LIB_FIELD*  field;

    // The only caller of this function is the library field editor, so it
    // establishes policy here.

    // Grab the MANDATORY_FIELDS first, in expected order given by
    // enum NumFieldType
    for( int id=0;  id<MANDATORY_FIELDS;  ++id )
    {
        field = GetField( id );

        // the MANDATORY_FIELDS are exactly that in RAM.
        wxASSERT( field );

        aList.push_back( *field );
    }

    // Now grab all the rest of fields.
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        field = ( LIB_FIELD* ) &item;

        if( (unsigned) field->GetId() < MANDATORY_FIELDS )
            continue;  // was added above

        aList.push_back( *field );
    }
}


LIB_FIELD* LIB_COMPONENT::GetField( int aId )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        LIB_FIELD* field = ( LIB_FIELD* ) &item;

        if( field->GetId() == aId )
            return field;
    }

    return NULL;
}


LIB_FIELD* LIB_COMPONENT::FindField( const wxString& aFieldName )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        LIB_FIELD* field = ( LIB_FIELD* ) &item;

        if( field->GetName() == aFieldName )
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


bool LIB_COMPONENT::SaveDateAndTime( OUTPUTFORMATTER& aFormatter )
{
    int year, mon, day, hour, min, sec;

    if( m_dateModified == 0 )
        return true;

    sec  = m_dateModified & 63;
    min  = ( m_dateModified >> 6 ) & 63;
    hour = ( m_dateModified >> 12 ) & 31;
    day  = ( m_dateModified >> 17 ) & 31;
    mon  = ( m_dateModified >> 22 ) & 15;
    year = ( m_dateModified >> 26 ) + 1990;

    aFormatter.Print( 0, "Ti %d/%d/%d %d:%d:%d\n", year, mon, day, hour, min, sec );

    return true;
}


bool LIB_COMPONENT::LoadDateAndTime( char* aLine )
{
    int   year, mon, day, hour, min, sec;

    year = mon = day = hour = min = sec = 0;
    strtok( aLine, " \r\t\n" );
    strtok( NULL, " \r\t\n" );

    if( sscanf( aLine, "%d/%d/%d %d:%d:%d", &year, &mon, &day, &hour, &min, &sec ) != 6 )
        return false;

    m_dateModified = ( sec & 63 ) + ( ( min & 63 ) << 6 ) +
                     ( ( hour & 31 ) << 12 ) + ( ( day & 31 ) << 17 ) +
                     ( ( mon & 15 ) << 22 ) + ( ( year - 1990 ) << 26 );

    return true;
}


void LIB_COMPONENT::SetOffset( const wxPoint& aOffset )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
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
    for( unsigned ii = 0; ii < drawings.size(); ii++  )
    {
        const LIB_ITEM& item = drawings[ii];
        if( item.m_Convert > 1 )
            return true;
    }

    return false;
}


void LIB_COMPONENT::ClearStatus()
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        item.m_Flags = 0;
    }
}


int LIB_COMPONENT::SelectItems( EDA_RECT& aRect, int aUnit, int aConvert, bool aEditPinByPin )
{
    int itemCount = 0;

    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        item.ClearFlags( SELECTED );

        if( ( item.m_Unit && item.m_Unit != aUnit )
            || ( item.m_Convert && item.m_Convert != aConvert ) )
        {
            if( item.Type() != LIB_PIN_T )
                continue;

             // Specific rules for pins.
            if( aEditPinByPin || m_unitsLocked
                || ( item.m_Convert && item.m_Convert != aConvert ) )
                continue;
        }

        if( item.Inside( aRect ) )
        {
            item.SetFlags( SELECTED );
            itemCount++;
        }
    }

    return itemCount;
}


void LIB_COMPONENT::MoveSelectedItems( const wxPoint& aOffset )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( !item.IsSelected() )
            continue;

        item.SetOffset( aOffset );
        item.m_Flags = 0;
    }

    drawings.sort();
}


void LIB_COMPONENT::ClearSelectedItems()
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        item.m_Flags = 0;
    }
}


void LIB_COMPONENT::DeleteSelectedItems()
{
    LIB_ITEMS::iterator item = drawings.begin();

    // We *do not* remove the 2 mandatory fields: reference and value
    // so skip them (do not remove) if they are flagged selected.
    // Skip also not visible items.
    // But I think fields must not be deleted by a block delete command or other global command
    // because they are not really graphic items
    while( item != drawings.end() )
    {
        if( item->Type() == LIB_FIELD_T )
        {
#if 0   // Set to 1 to allows fields deletion on block delete or other global command
            LIB_FIELD& field = ( LIB_FIELD& ) *item;

            if( (field.GetId() == REFERENCE) || (field.m_FieldId == VALUE) ||
                (field.m_Attributs & TEXT_NO_VISIBLE) )
#endif
                item->ClearFlags( SELECTED );
        }

        if( !item->IsSelected() )
            item++;
        else
            item = drawings.erase( item );
    }
}


void LIB_COMPONENT::CopySelectedItems( const wxPoint& aOffset )
{
    /* *do not* use iterators here, because new items
     * are added to drawings that is a  boost::ptr_vector.
     * When push_back elements in buffer,
     * a memory reallocation can happen and will break pointers
     */
    unsigned icnt = drawings.size();

    for( unsigned ii = 0; ii < icnt; ii++  )
    {
        LIB_ITEM& item = drawings[ii];

        // We *do not* copy fields because they are unique for the whole component
        // so skip them (do not duplicate) if they are flagged selected.
        if( item.Type() == LIB_FIELD_T )
            item.ClearFlags( SELECTED );

        if( !item.IsSelected() )
            continue;

        item.ClearFlags( SELECTED );
        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
        newItem->SetFlags( SELECTED );
        drawings.push_back( newItem );
    }

    MoveSelectedItems( aOffset );
    drawings.sort();
}



void LIB_COMPONENT::MirrorSelectedItemsH( const wxPoint& aCenter )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( !item.IsSelected() )
            continue;

        item.MirrorHorizontal( aCenter );
        item.m_Flags = 0;
    }

    drawings.sort();
}

void LIB_COMPONENT::MirrorSelectedItemsV( const wxPoint& aCenter )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( !item.IsSelected() )
            continue;

        item.MirrorVertical( aCenter );
        item.m_Flags = 0;
    }

    drawings.sort();
}

void LIB_COMPONENT::RotateSelectedItems( const wxPoint& aCenter )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
    {
        if( !item.IsSelected() )
            continue;

        item.Rotate( aCenter );
        item.m_Flags = 0;
    }

    drawings.sort();
}



LIB_ITEM* LIB_COMPONENT::LocateDrawItem( int aUnit, int aConvert,
                                         KICAD_T aType, const wxPoint& aPoint )
{
    BOOST_FOREACH( LIB_ITEM& item, drawings )
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


LIB_ITEM* LIB_COMPONENT::LocateDrawItem( int aUnit, int aConvert, KICAD_T aType,
                                         const wxPoint& aPoint, const TRANSFORM& aTransform )
{
    /* we use LocateDrawItem( int aUnit, int convert, KICAD_T type, const
     * wxPoint& pt ) to search items.
     * because this function uses DefaultTransform as orient/mirror matrix
     * we temporary copy aTransform in DefaultTransform
     */
    LIB_ITEM* item;
    TRANSFORM transform = DefaultTransform;
    DefaultTransform = aTransform;

    item = LocateDrawItem( aUnit, aConvert, aType, aPoint );

    // Restore matrix
    DefaultTransform = transform;

    return item;
}


void LIB_COMPONENT::SetPartCount( int aCount )
{
    if( m_unitCount == aCount )
        return;

    if( aCount < m_unitCount )
    {
        LIB_ITEMS::iterator i;
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
        int prevCount = m_unitCount;

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
                LIB_ITEM* newItem = (LIB_ITEM*) drawings[ii].Clone();
                newItem->m_Unit = j;
                drawings.push_back( newItem );
            }
        }

        drawings.sort();
    }

    m_unitCount = aCount;
}


void LIB_COMPONENT::SetConversion( bool aSetConvert )
{
    if( aSetConvert == HasConversion() )
        return;

    // Duplicate items to create the converted shape
    if( aSetConvert )
    {
        BOOST_FOREACH( LIB_ITEM& item, drawings )
        {
            // Only pins are duplicated.
            if( item.Type() != LIB_PIN_T )
                continue;

            if( item.m_Convert == 1 )
            {
                LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
                newItem->m_Convert = 2;
                drawings.push_back( newItem );
            }
        }
    }
    else
    {
        // Delete converted shape items because the converted shape does
        // not exist
        LIB_ITEMS::iterator i = drawings.begin();

        while( i != drawings.end() )
        {
            if( i->m_Convert > 1 )
                i = drawings.erase( i );
            else
                i++;
        }
    }
}


wxArrayString LIB_COMPONENT::GetAliasNames( bool aIncludeRoot ) const
{
    wxArrayString names;

    LIB_ALIASES::const_iterator it;

    for( it=m_aliases.begin();  it<m_aliases.end();  ++it )
    {
        if( !aIncludeRoot && (*it)->IsRoot() )
            continue;

        names.Add( (*it)->GetName() );
    }

    return names;
}


bool LIB_COMPONENT::HasAlias( const wxString& aName ) const
{
    wxCHECK2_MSG( !aName.IsEmpty(), return false,
                  wxT( "Cannot get alias with an empty name, bad programmer." ) );

    for( size_t i = 0; i < m_aliases.size(); i++ )
    {
        if( aName.CmpNoCase( m_aliases[i]->GetName() ) == 0 )
            return true;
    }

    return false;
}


void LIB_COMPONENT::SetAliases( const wxArrayString& aAliasList )
{
    wxCHECK_RET( m_library == NULL,
                 wxT( "Component aliases cannot be changed when they are owned by a library." ) );

    if( aAliasList == GetAliasNames() )
        return;

    // Add names not existing in the current component alias list.
    for( size_t i = 0; i < aAliasList.GetCount(); i++ )
    {
        if( HasAlias( aAliasList[ i ] ) )
            continue;

        m_aliases.push_back( new LIB_ALIAS( aAliasList[ i ], this ) );
    }

    // Remove names in the current component that are not in the new alias list.
    LIB_ALIASES::iterator it;

    for( it = m_aliases.begin(); it < m_aliases.end(); it++ )
    {
        int index = aAliasList.Index( (*it)->GetName(), false );

        if( index != wxNOT_FOUND || (*it)->IsRoot() )
            continue;

        it = m_aliases.erase( it );
    }
}


void LIB_COMPONENT::RemoveAlias( const wxString& aName )
{
    wxCHECK_RET( m_library == NULL,
                 wxT( "Component aliases cannot be changed when they are owned by a library." ) );
    wxCHECK_RET( !aName.IsEmpty(), wxT( "Cannot get alias with an empty name." ) );

    LIB_ALIASES::iterator it;

    for( it = m_aliases.begin(); it < m_aliases.end(); it++ )
    {
        if( aName.CmpNoCase( (*it)->GetName() ) == 0 )
        {
            m_aliases.erase( it );
            break;
        }
    }
}


LIB_ALIAS* LIB_COMPONENT::RemoveAlias( LIB_ALIAS* aAlias )
{
    wxCHECK_MSG( aAlias != NULL, NULL, wxT( "Cannot remove alias by NULL pointer." ) );

    LIB_ALIAS* nextAlias = NULL;
    LIB_ALIASES::iterator it = find( m_aliases.begin(), m_aliases.end(), aAlias );

    if( it != m_aliases.end() )
    {
        bool rename = aAlias->IsRoot();

        it = m_aliases.erase( it );
        delete aAlias;

        if( !m_aliases.empty() )
        {
            if( it == m_aliases.end() )
                it = m_aliases.begin();

            nextAlias = (*it);

            if( rename )
                SetName( nextAlias->GetName() );
        }

    }

    return nextAlias;
}


void LIB_COMPONENT::RemoveAllAliases()
{
    // Remove all of the aliases except the root alias.
    while( m_aliases.size() > 1 )
        m_aliases.pop_back();
}


LIB_ALIAS* LIB_COMPONENT::GetAlias( const wxString& aName )
{
    wxCHECK2_MSG( !aName.IsEmpty(), return NULL,
                  wxT( "Cannot get alias with an empty name.  Bad programmer!" ) );

    for( size_t i = 0; i < m_aliases.size(); i++ )
    {
        if( aName.CmpNoCase( m_aliases[i]->GetName() ) == 0 )
            return m_aliases[i];
    }

    return NULL;
}


LIB_ALIAS* LIB_COMPONENT::GetAlias( size_t aIndex )
{
    wxCHECK2_MSG( aIndex < m_aliases.size(), return NULL,
                  wxT( "Illegal alias list index, bad programmer." ) );

    return m_aliases[aIndex];
}


void LIB_COMPONENT::AddAlias( const wxString& aName )
{
    wxCHECK_RET( !HasAlias( aName ),
                 wxT( "Component <" ) + GetName() + wxT( "> already has an alias <" ) +
                 aName + wxT( ">.  Bad programmer." ) );

    m_aliases.push_back( new LIB_ALIAS( aName, this ) );
}


/** Set the separator char between the subpart id and the reference
 * 0 (no separator) or '.' , '-' and '_'
 * and the ascii char value to calculate the subpart symbol id from the part number:
 * 'A' or '1' only are allowed. (to print U1.A or U1.1)
 * if this is a digit, a number is used as id symbol
 * Note also if the subpart symbol is a digit, the separator cannot be null.
 * @param aSep = the separator symbol (0 (no separator) or '.' , '-' and '_')
 * @param aFirstId = the Id of the first part ('A' or '1')
 */
void LIB_COMPONENT::SetSubpartIdNotation( int aSep, int aFirstId )
{
    m_subpartFirstId = 'A';
    m_subpartIdSeparator = 0;

    if( aSep == '.' || aSep == '-' || aSep == '_' )
        m_subpartIdSeparator = aSep;

    if( aFirstId == '1' && aSep != 0 )
        m_subpartFirstId = aFirstId;
}
