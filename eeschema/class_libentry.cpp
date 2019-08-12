/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <macros.h>
#include <kicad_string.h>
#include <sch_draw_panel.h>
#include <plotter.h>
#include <gr_basic.h>
#include <sch_screen.h>
#include <richio.h>
#include <trace_helpers.h>
#include <general.h>
#include <template_fieldnames.h>
#include <transform.h>
#include <class_library.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <lib_arc.h>


// the separator char between the subpart id and the reference
// 0 (no separator) or '.' or some other character
int LIB_PART::m_subpartIdSeparator = 0;

// the ascii char value to calculate the subpart symbol id from the part number:
// 'A' or '1' usually. (to print U1.A or U1.1)
// if this a a digit, a number is used as id symbol
int LIB_PART::m_subpartFirstId = 'A';


LIB_ALIAS::LIB_ALIAS( const wxString& aName, LIB_PART* aRootPart ) :
    EDA_ITEM( LIB_ALIAS_T ),
    shared( aRootPart )
{
    SetName( aName );
}


LIB_ALIAS::LIB_ALIAS( const LIB_ALIAS& aAlias, LIB_PART* aRootPart ) :
    EDA_ITEM( aAlias ),
    shared( aRootPart )
{
    name   = aAlias.name;

    description = aAlias.description;
    keyWords = aAlias.keyWords;
    docFileName = aAlias.docFileName;
}


LIB_ALIAS::~LIB_ALIAS()
{
    wxLogTrace( traceSchLibMem, wxT( "%s: destroying alias:'%s'" ),
                GetChars( wxString::FromAscii( __WXFUNCTION__ ) ), GetChars( GetName() ) );

    wxCHECK_RET( shared, wxT( "~LIB_ALIAS() without a LIB_PART" ) );

    if( shared )
        shared->RemoveAlias( this );
}


wxString LIB_ALIAS::GetLibNickname() const
{
    wxASSERT_MSG( shared, wxT( "LIB_ALIAS without a LIB_PART" ) );

    if( shared )
        return shared->GetLibraryName();

    return wxEmptyString;
}


bool LIB_ALIAS::IsRoot() const
{
    return name == shared->GetName();
}


LIB_ID LIB_ALIAS::GetLibId() const
{
    LIB_ID id = shared->GetLibId();
    id.SetLibItemName( name );
    return id;
}


PART_LIB* LIB_ALIAS::GetLib()
{
    return shared->GetLib();
}


void LIB_ALIAS::SetName( const wxString& aName )
{
    name = LIB_ID::FixIllegalChars( aName, LIB_ID::ID_SCH );
}


int LIB_ALIAS::GetUnitCount()
{
    return shared->GetUnitCount();
}


wxString LIB_ALIAS::GetUnitReference( int aUnit )
{
    return LIB_PART::SubReference( aUnit, false );
}


const EDA_RECT LIB_ALIAS::GetBoundingBox() const
{
    // a LIB_ALIAS does not really have a bounding box.
    // return a 0 size rect.
    EDA_RECT dummy;

    return dummy;
};


const BOX2I LIB_ALIAS::ViewBBox() const
{
    // LIB_ALIAS may be displayed in preview windows, so ensure that it is always
    // selected for drawing.
    BOX2I bbox;
    bbox.SetMaximum();
    return bbox;
}


wxString LIB_ALIAS::GetSearchText()
{
    // Matches are scored by offset from front of string, so inclusion of this spacer
    // discounts matches found after it.
    static const wxString discount( wxT( "        " ) );

    wxString text = GetKeyWords() + discount + GetDescription();

    // If a footprint is defined for the part, add it to the serach string
    if( shared )
    {
        wxString footprint = shared->GetFootprintField().GetText();

        if( !footprint.IsEmpty() )
            text += discount + footprint;
    }

    return text;
}


bool LIB_ALIAS::operator==( const wxChar* aName ) const
{
    return name == aName;
}


bool operator<( const LIB_ALIAS& aItem1, const LIB_ALIAS& aItem2 )
{
    return aItem1.GetName() < aItem2.GetName();
}


void LIB_ALIAS::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // An alias's fields don't know how to fetch their parent's values so we don't let
    // them draw themselves.  This means the alias always has to draw them, which means
    // it has to "own" their layers as well.
    aCount      = 6;
    aLayers[0]  = LAYER_DEVICE;
    aLayers[1]  = LAYER_DEVICE_BACKGROUND;
    aLayers[2]  = LAYER_REFERENCEPART;
    aLayers[3]  = LAYER_VALUEPART;
    aLayers[4]  = LAYER_FIELDS;
    aLayers[5]  = LAYER_SELECTION_SHADOWS;
}


/// http://www.boost.org/doc/libs/1_55_0/libs/smart_ptr/sp_techniques.html#weak_without_shared
struct null_deleter
{
    void operator()(void const *) const
    {
    }
};


LIB_PART::LIB_PART( const wxString& aName, PART_LIB* aLibrary ) :
    EDA_ITEM( LIB_PART_T ),
    m_me( this, null_deleter() )
{
    m_dateLastEdition     = 0;
    m_unitCount           = 1;
    m_pinNameOffset       = 40;
    m_options             = ENTRY_NORMAL;
    m_unitsLocked         = false;
    m_showPinNumbers      = true;
    m_showPinNames        = true;

    // Add the MANDATORY_FIELDS in RAM only.  These are assumed to be present
    // when the field editors are invoked.
    m_drawings[LIB_FIELD_T].reserve( 4 );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, VALUE ) );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, REFERENCE ) );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, FOOTPRINT ) );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, DATASHEET ) );

    SetLib( aLibrary );
    SetName( aName );
}


LIB_PART::LIB_PART( LIB_PART& aPart, PART_LIB* aLibrary ) :
    EDA_ITEM( aPart ),
    m_me( this, null_deleter() )
{
    LIB_ITEM* newItem;

    m_library             = aLibrary;
    m_FootprintList       = aPart.m_FootprintList;
    m_unitCount           = aPart.m_unitCount;
    m_unitsLocked         = aPart.m_unitsLocked;
    m_pinNameOffset       = aPart.m_pinNameOffset;
    m_showPinNumbers      = aPart.m_showPinNumbers;
    m_showPinNames        = aPart.m_showPinNames;
    m_dateLastEdition     = aPart.m_dateLastEdition;
    m_options             = aPart.m_options;
    m_libId               = aPart.m_libId;

    for( LIB_ITEM& oldItem : aPart.m_drawings )
    {
        if( ( oldItem.GetFlags() & ( IS_NEW | STRUCT_DELETED ) ) != 0 )
            continue;

        newItem = (LIB_ITEM*) oldItem.Clone();
        newItem->SetParent( this );
        m_drawings.push_back( newItem );
    }

    for( size_t i = 0; i < aPart.m_aliases.size(); i++ )
    {
        LIB_ALIAS* alias = new LIB_ALIAS( *aPart.m_aliases[i], this );
        m_aliases.push_back( alias );
    }
}


LIB_PART::~LIB_PART()
{
    wxLogTrace( traceSchLibMem,
                wxT( "%s: destroying symbol with alias list count of %llu" ),
                GetChars( wxString::FromAscii( __WXFUNCTION__ ) ),
                (long long unsigned) m_aliases.size() );

    // If the part is being deleted directly rather than through the library,
    // delete all of the aliases.
    while( m_aliases.size() )
    {
        LIB_ALIAS* alias = m_aliases.back();
        m_aliases.pop_back();
        delete alias;
    }
}


const wxString LIB_PART::GetLibraryName()
{
    if( m_library )
        return m_library->GetName();

    return m_libId.GetLibNickname();
}


wxString LIB_PART::SubReference( int aUnit, bool aAddSeparator )
{
    wxString subRef;

    if( m_subpartIdSeparator != 0 && aAddSeparator )
        subRef << wxChar( m_subpartIdSeparator );

    if( m_subpartFirstId >= '0' && m_subpartFirstId <= '9' )
        subRef << aUnit;
    else
    {
        // use letters as notation. To allow more than 26 units, the sub ref
        // use one letter if letter = A .. Z or a ... z, and 2 letters otherwise
        // first letter is expected to be 'A' or 'a' (i.e. 26 letters are available)
        int u;
        aUnit -= 1;     // Unit number starts to 1. now to 0.

        while( aUnit >= 26 )    // more than one letter are needed
        {
            u = aUnit / 26;
            subRef << wxChar( m_subpartFirstId + u -1 );
            aUnit %= 26;
        }

        u = m_subpartFirstId + aUnit;
        subRef << wxChar( u );
    }

    return subRef;
}


const wxString& LIB_PART::GetName() const
{
    static wxString dummy;

    wxCHECK_MSG( m_aliases.size(), dummy, "no aliases defined for symbol" );

    return m_aliases[0]->GetName();
}


void LIB_PART::SetName( const wxString& aName )
{
    // The LIB_ALIAS that is the LIB_PART name has to be created so create it.
    if( m_aliases.empty() )
        m_aliases.push_back( new LIB_ALIAS( aName, this ) );
    else
        m_aliases[0]->SetName( aName );

    wxString validatedName = LIB_ID::FixIllegalChars( aName, LIB_ID::ID_SCH );
    m_libId.SetLibItemName( validatedName, false );

    GetValueField().SetText( validatedName );
}


void LIB_PART::Print( wxDC* aDc, const wxPoint& aOffset, int aMulti, int aConvert,
                      const PART_DRAW_OPTIONS& aOpts )
{
    /* draw background for filled items using background option
     * Solid lines will be drawn after the background
     * Note also, background is not drawn when printing in black and white
     */
    if( !GetGRForceBlackPenState() )
    {
        for( LIB_ITEM& drawItem : m_drawings )
        {
            if( drawItem.m_Fill != FILLED_WITH_BG_BODYCOLOR )
                continue;

            // Do not draw items not attached to the current part
            if( aMulti && drawItem.m_Unit && ( drawItem.m_Unit != aMulti ) )
                continue;

            if( aConvert && drawItem.m_Convert && ( drawItem.m_Convert != aConvert ) )
                continue;

            if( drawItem.Type() == LIB_FIELD_T )
                continue;

            // Now, draw only the background for items with
            // m_Fill == FILLED_WITH_BG_BODYCOLOR:
            drawItem.Print( aDc, aOffset, (void*) false, aOpts.transform );
        }
    }

    for( LIB_ITEM& drawItem : m_drawings )
    {
        // Do not draw items not attached to the current part
        if( aMulti && drawItem.m_Unit && ( drawItem.m_Unit != aMulti ) )
            continue;

        if( aConvert && drawItem.m_Convert && ( drawItem.m_Convert != aConvert ) )
            continue;

        if( drawItem.Type() == LIB_FIELD_T )
        {
            LIB_FIELD& field = static_cast<LIB_FIELD&>( drawItem );

            if( field.IsVisible() && !aOpts.draw_visible_fields )
                continue;

            if( !field.IsVisible() && !aOpts.draw_hidden_fields )
                continue;
        }

        if( drawItem.Type() == LIB_PIN_T )
        {
            drawItem.Print( aDc, aOffset, (void*) &aOpts, aOpts.transform );
        }
        else if( drawItem.Type() == LIB_FIELD_T )
        {
            drawItem.Print( aDc, aOffset, (void*) NULL, aOpts.transform );
        }
        else
        {
            bool forceNoFill = drawItem.m_Fill == FILLED_WITH_BG_BODYCOLOR;
            drawItem.Print( aDc, aOffset, (void*) forceNoFill, aOpts.transform );
        }
    }
}


void LIB_PART::Plot( PLOTTER* aPlotter, int aUnit, int aConvert,
                     const wxPoint& aOffset, const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
    bool fill = aPlotter->GetColorMode();

    // draw background for filled items using background option
    // Solid lines will be drawn after the background
    for( LIB_ITEM& item : m_drawings )
    {
        // Lib Fields are not plotted here, because this plot function
        // is used to plot schematic items, which have they own fields
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
            continue;

        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        if( item.m_Fill == FILLED_WITH_BG_BODYCOLOR )
            item.Plot( aPlotter, aOffset, fill, aTransform );
    }

    // Not filled items and filled shapes are now plotted
    // (plot only items which are not already plotted)
    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
            continue;

        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        if( item.m_Fill != FILLED_WITH_BG_BODYCOLOR )
            item.Plot( aPlotter, aOffset, fill, aTransform );
    }
}


void LIB_PART::PlotLibFields( PLOTTER* aPlotter, int aUnit, int aConvert,
                              const wxPoint& aOffset, const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    aPlotter->SetColor( GetLayerColor( LAYER_FIELDS ) );
    bool fill = aPlotter->GetColorMode();

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
            continue;

        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        // The reference is a special case: we should change the basic text
        // to add '?' and the part id
        LIB_FIELD& field = (LIB_FIELD&) item;
        wxString tmp = field.GetShownText();
        if( field.GetId() == REFERENCE )
        {
            wxString text = field.GetFullText( aUnit );
            field.SetText( text );
        }
        item.Plot( aPlotter, aOffset, fill, aTransform );
        field.SetText( tmp );
    }
}


void LIB_PART::RemoveDrawItem( LIB_ITEM* aItem )
{
    wxASSERT( aItem != NULL );

    // none of the MANDATORY_FIELDS may be removed in RAM, but they may be
    // omitted when saving to disk.
    if( aItem->Type() == LIB_FIELD_T )
    {
        LIB_FIELD* field = (LIB_FIELD*) aItem;

        if( field->GetId() < MANDATORY_FIELDS )
        {
            wxLogWarning( _(
                "An attempt was made to remove the %s field from component %s in library %s." ),
                GetChars( field->GetName() ), GetChars( GetName() ),
                GetChars( GetLibraryName() ) );
            return;
        }
    }

    LIB_ITEMS& items = m_drawings[ aItem->Type() ];

    for( LIB_ITEMS::iterator i = items.begin(); i != items.end(); i++ )
    {
        if( *i == aItem )
        {
            items.erase( i );
            SetModified();
            break;
        }
    }
}


void LIB_PART::AddDrawItem( LIB_ITEM* aItem )
{
    if( !aItem )
        return;

    m_drawings.push_back( aItem );
}


LIB_ITEM* LIB_PART::GetNextDrawItem( LIB_ITEM* aItem, KICAD_T aType )
{
    if( m_drawings.empty( aType ) )
        return NULL;

    if( aItem == NULL )
        return &( *( m_drawings.begin( aType ) ) );

    // Search for the last item, assume aItem is of type aType
    wxASSERT( ( aType == TYPE_NOT_INIT ) || ( aType == aItem->Type() ) );
    LIB_ITEMS_CONTAINER::ITERATOR it = m_drawings.begin( aType );

    while( ( it != m_drawings.end( aType ) ) && ( aItem != &( *it ) ) )
        ++it;

    // Search the next item
    if( it != m_drawings.end( aType ) )
    {
        ++it;

        if( it != m_drawings.end( aType ) )
            return &( *it );
    }

    return NULL;
}


void LIB_PART::GetPins( LIB_PINS& aList, int aUnit, int aConvert )
{
    if( m_drawings.empty( LIB_PIN_T ) )
        return;

    /* Notes:
     * when aUnit == 0: no unit filtering
     * when aConvert == 0: no convert (shape selection) filtering
     * when .m_Unit == 0, the body item is common to units
     * when .m_Convert == 0, the body item is common to shapes
     */
    for( LIB_ITEM& item : m_drawings[ LIB_PIN_T ] )
    {
        // Unit filtering:
        if( aUnit && item.m_Unit && ( item.m_Unit != aUnit ) )
             continue;

        // Shape filtering:
        if( aConvert && item.m_Convert && ( item.m_Convert != aConvert ) )
            continue;

        aList.push_back( (LIB_PIN*) &item );
    }
}


LIB_PIN* LIB_PART::GetPin( const wxString& aNumber, int aUnit, int aConvert )
{
    LIB_PINS pinList;

    GetPins( pinList, aUnit, aConvert );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        wxASSERT( pinList[i]->Type() == LIB_PIN_T );

        if( aNumber == pinList[i]->GetNumber() )
            return pinList[i];
    }

    return NULL;
}


bool LIB_PART::PinsConflictWith( LIB_PART& aOtherPart, bool aTestNums, bool aTestNames,
        bool aTestType, bool aTestOrientation, bool aTestLength )
{
    LIB_PINS thisPinList;
    GetPins( thisPinList, /* aUnit */ 0, /* aConvert */ 0 );

    for( LIB_PIN* eachThisPin : thisPinList )
    {
        wxASSERT( eachThisPin );
        LIB_PINS otherPinList;
        aOtherPart.GetPins( otherPinList, /* aUnit */ 0, /* aConvert */ 0 );
        bool foundMatch = false;

        for( LIB_PIN* eachOtherPin : otherPinList )
        {
            wxASSERT( eachOtherPin );
            // Same position?
            if( eachThisPin->GetPosition() != eachOtherPin->GetPosition() )
                continue;

            // Same number?
            if( aTestNums && ( eachThisPin->GetNumber() != eachOtherPin->GetNumber() ))
                continue;

            // Same name?
            if( aTestNames && ( eachThisPin->GetName() != eachOtherPin->GetName() ))
                continue;

            // Same electrical type?
            if( aTestType && ( eachThisPin->GetType() != eachOtherPin->GetType() ))
                continue;

            // Same orientation?
            if( aTestOrientation && ( eachThisPin->GetOrientation() != eachOtherPin->GetOrientation() ))
                continue;

            // Same length?
            if( aTestLength && ( eachThisPin->GetLength() != eachOtherPin->GetLength() ))
                continue;

            foundMatch = true;
        }

        if( !foundMatch )
        {
            // This means there was not an identical (according to the arguments)
            // pin at the same position in the other component.
            return true;
        }
    }

    // The loop never gave up, so no conflicts were found.
    return false;
}


const EDA_RECT LIB_PART::GetUnitBoundingBox( int aUnit, int aConvert ) const
{
    EDA_RECT bBox;
    bool initialized = false;

    for( const LIB_ITEM& item : m_drawings )
    {
        if( ( item.m_Unit > 0 ) && ( ( m_unitCount > 1 ) && ( aUnit > 0 )
                                     && ( aUnit != item.m_Unit ) ) )
            continue;

        if( item.m_Convert > 0 && ( ( aConvert > 0 ) && ( aConvert != item.m_Convert ) ) )
            continue;

        if ( ( item.Type() == LIB_FIELD_T ) && !( ( LIB_FIELD& ) item ).IsVisible() )
            continue;

        if( initialized )
            bBox.Merge( item.GetBoundingBox() );
        else
        {
            bBox = item.GetBoundingBox();
            initialized = true;
        }
    }

    return bBox;
}


void LIB_PART::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount      = 3;
    aLayers[0]  = LAYER_DEVICE;
    aLayers[1]  = LAYER_DEVICE_BACKGROUND;
    aLayers[2]  = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT LIB_PART::GetBodyBoundingBox( int aUnit, int aConvert ) const
{
    EDA_RECT bBox;
    bool initialized = false;

    for( const LIB_ITEM& item : m_drawings )
    {
        if( ( item.m_Unit > 0 ) && ( ( m_unitCount > 1 ) && ( aUnit > 0 )
                                     && ( aUnit != item.m_Unit ) ) )
            continue;

        if( item.m_Convert > 0 && ( ( aConvert > 0 ) && ( aConvert != item.m_Convert ) ) )
            continue;

        if( item.Type() == LIB_FIELD_T )
            continue;

        if( initialized )
            bBox.Merge( item.GetBoundingBox() );
        else
        {
            bBox = item.GetBoundingBox();
            initialized = true;
        }
    }

    return bBox;
}


void LIB_PART::deleteAllFields()
{
    m_drawings[ LIB_FIELD_T ].clear();
}


void LIB_PART::SetFields( const std::vector <LIB_FIELD>& aFields )
{
    deleteAllFields();

    for( unsigned i=0;  i<aFields.size();  ++i )
    {
        // drawings is a ptr_vector, new and copy an object on the heap.
        LIB_FIELD* field = new LIB_FIELD( aFields[i] );

        field->SetParent( this );
        m_drawings.push_back( field );
    }
}


void LIB_PART::GetFields( LIB_FIELDS& aList )
{
    LIB_FIELD*  field;

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
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        field = ( LIB_FIELD* ) &item;

        if( (unsigned) field->GetId() < MANDATORY_FIELDS )
            continue;  // was added above

        aList.push_back( *field );
    }
}


LIB_FIELD* LIB_PART::GetField( int aId )
{
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = ( LIB_FIELD* ) &item;

        if( field->GetId() == aId )
            return field;
    }

    return NULL;
}


LIB_FIELD* LIB_PART::FindField( const wxString& aFieldName )
{
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = ( LIB_FIELD* ) &item;

        if( field->GetName() == aFieldName )
            return field;
    }

    return NULL;
}


LIB_FIELD& LIB_PART::GetValueField()
{
    LIB_FIELD* field = GetField( VALUE );
    wxASSERT( field != NULL );
    return *field;
}


LIB_FIELD& LIB_PART::GetReferenceField()
{
    LIB_FIELD* field = GetField( REFERENCE );
    wxASSERT( field != NULL );
    return *field;
}


LIB_FIELD& LIB_PART::GetFootprintField()
{
    LIB_FIELD* field = GetField( FOOTPRINT );
    wxASSERT( field != NULL );
    return *field;
}


void LIB_PART::SetOffset( const wxPoint& aOffset )
{
    for( LIB_ITEM& item : m_drawings )
        item.Offset( aOffset );
}


void LIB_PART::RemoveDuplicateDrawItems()
{
    m_drawings.unique();
}


bool LIB_PART::HasConversion() const
{
    for( const LIB_ITEM& item : m_drawings )
    {
        if( item.m_Convert > LIB_ITEM::LIB_CONVERT::BASE )
            return true;
    }

    return false;
}

void LIB_PART::ClearTempFlags()
{
    for( LIB_ITEM& item : m_drawings )
        item.ClearTempFlags();
}

void LIB_PART::ClearEditFlags()
{
    for( LIB_ITEM& item : m_drawings )
        item.ClearEditFlags();
}

LIB_ITEM* LIB_PART::LocateDrawItem( int aUnit, int aConvert,
                                    KICAD_T aType, const wxPoint& aPoint )
{
    for( LIB_ITEM& item : m_drawings )
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


LIB_ITEM* LIB_PART::LocateDrawItem( int aUnit, int aConvert, KICAD_T aType,
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


SEARCH_RESULT LIB_PART::Visit( INSPECTOR aInspector, void* aTestData, const KICAD_T aFilterTypes[] )
{
    // The part itself is never inspected, only its children
    for( LIB_ITEM& item : m_drawings )
    {
        if( item.IsType( aFilterTypes ) )
        {
            if( aInspector( &item, aTestData ) == SEARCH_QUIT )
                return SEARCH_QUIT;
        }
    }

    return SEARCH_CONTINUE;
}


void LIB_PART::SetUnitCount( int aCount )
{
    if( m_unitCount == aCount )
        return;

    if( aCount < m_unitCount )
    {
        LIB_ITEMS_CONTAINER::ITERATOR i = m_drawings.begin();

        while( i != m_drawings.end() )
        {
            if( i->m_Unit > aCount )
                i = m_drawings.erase( i );
            else
                ++i;
        }
    }
    else
    {
        int prevCount = m_unitCount;

        // Temporary storage for new items, as adding new items directly to
        // m_drawings may cause the buffer reallocation which invalidates the
        // iterators
        std::vector< LIB_ITEM* > tmp;

        for( LIB_ITEM& item : m_drawings )
        {
            if( item.m_Unit != 1 )
                continue;

            for( int j = prevCount + 1; j <= aCount; j++ )
            {
                LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
                newItem->m_Unit = j;
                tmp.push_back( newItem );
            }
        }

        for( auto item : tmp )
            m_drawings.push_back( item );
    }

    m_unitCount = aCount;
}


void LIB_PART::SetConversion( bool aSetConvert )
{
    if( aSetConvert == HasConversion() )
        return;

    // Duplicate items to create the converted shape
    if( aSetConvert )
    {
        std::vector< LIB_ITEM* > tmp;     // Temporarily store the duplicated pins here.

        for( LIB_ITEM& item : m_drawings )
        {
            // Only pins are duplicated.
            if( item.Type() != LIB_PIN_T )
                continue;

            if( item.m_Convert == 1 )
            {
                LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
                newItem->m_Convert = 2;
                tmp.push_back( newItem );
            }
        }

        // Transfer the new pins to the LIB_PART.
        for( unsigned i = 0;  i < tmp.size();  i++ )
            m_drawings.push_back( tmp[i] );
    }
    else
    {
        // Delete converted shape items because the converted shape does
        // not exist
        LIB_ITEMS_CONTAINER::ITERATOR i = m_drawings.begin();

        while( i != m_drawings.end() )
        {
            if( i->m_Convert > 1 )
                i = m_drawings.erase( i );
            else
                ++i;
        }
    }
}


wxArrayString LIB_PART::GetAliasNames( bool aIncludeRoot ) const
{
    wxArrayString names;

    LIB_ALIASES::const_iterator it;

    for( it=m_aliases.begin();  it != m_aliases.end();  ++it )
    {
        if( !aIncludeRoot && (*it)->IsRoot() )
            continue;

        names.Add( (*it)->GetName() );
    }

    return names;
}


bool LIB_PART::HasAlias( const wxString& aName ) const
{
    wxCHECK2_MSG( !aName.IsEmpty(), return false,
                  wxT( "Cannot get alias with an empty name, bad programmer." ) );

    for( size_t i = 0; i < m_aliases.size(); i++ )
    {
        if( aName == m_aliases[i]->GetName() )
            return true;
    }

    return false;
}


void LIB_PART::RemoveAlias( const wxString& aName )
{
    LIB_ALIAS* a = GetAlias( aName );

    if( a )
        RemoveAlias( a );
}


LIB_ALIAS* LIB_PART::RemoveAlias( LIB_ALIAS* aAlias )
{
    wxCHECK_MSG( aAlias, NULL, wxT( "Cannot remove alias by NULL pointer." ) );

    LIB_ALIAS* nextAlias = NULL;

    LIB_ALIASES::iterator it = find( m_aliases.begin(), m_aliases.end(), aAlias );

    if( it != m_aliases.end() )
    {
        bool rename = aAlias->IsRoot();

        wxLogTrace( traceSchLibMem,
                    wxT( "%s: symbol:'%s', alias:'%s', alias count %llu, reference count %ld." ),
                    GetChars( wxString::FromAscii( __WXFUNCTION__ ) ),
                    GetChars( GetName() ),
                    GetChars( aAlias->GetName() ),
                    (long long unsigned) m_aliases.size(),
                    m_me.use_count() );

        it = m_aliases.erase( it );

        if( !m_aliases.empty() )
        {
            if( it == m_aliases.end() )
                it = m_aliases.begin();

            nextAlias = *it;

            if( rename )
                SetName( nextAlias->GetName() );
        }
    }

    return nextAlias;
}


void LIB_PART::RemoveAllAliases()
{
    // Remove all of the aliases except the root alias.
    while( m_aliases.size() > 1 )
        m_aliases.pop_back();
}


LIB_ALIAS* LIB_PART::GetAlias( const wxString& aName )
{
    wxCHECK2_MSG( !aName.IsEmpty(), return NULL,
                  wxT( "Cannot get alias with an empty name.  Bad programmer!" ) );

    for( size_t i = 0; i < m_aliases.size(); i++ )
    {
        if( aName == m_aliases[i]->GetName() )
            return m_aliases[i];
    }

    return NULL;
}


LIB_ALIAS* LIB_PART::GetAlias( size_t aIndex )
{
    wxCHECK2_MSG( aIndex < m_aliases.size(), return NULL,
                  wxT( "Illegal alias list index, bad programmer." ) );

    return m_aliases[aIndex];
}


void LIB_PART::AddAlias( const wxString& aName )
{
    m_aliases.push_back( new LIB_ALIAS( aName, this ) );
}


void LIB_PART::AddAlias( LIB_ALIAS* aAlias )
{
    m_aliases.push_back( aAlias );
}


void LIB_PART::SetSubpartIdNotation( int aSep, int aFirstId )
{
    m_subpartFirstId = 'A';
    m_subpartIdSeparator = 0;

    if( aSep == '.' || aSep == '-' || aSep == '_' )
        m_subpartIdSeparator = aSep;

    if( aFirstId == '1' && aSep != 0 )
        m_subpartFirstId = aFirstId;
}
