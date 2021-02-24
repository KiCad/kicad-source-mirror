/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <plotter.h>
#include <sch_screen.h>
#include <richio.h>
#include <general.h>
#include <template_fieldnames.h>
#include <transform.h>
#include <class_library.h>
#include <lib_pin.h>
#include <lib_arc.h>
#include <settings/color_settings.h>


// the separator char between the subpart id and the reference
// 0 (no separator) or '.' or some other character
int LIB_PART::m_subpartIdSeparator = 0;

// the ascii char value to calculate the subpart symbol id from the part number:
// 'A' or '1' usually. (to print U1.A or U1.1)
// if this a a digit, a number is used as id symbol
int LIB_PART::m_subpartFirstId = 'A';


wxString LIB_PART::GetSearchText()
{
    // Matches are scored by offset from front of string, so inclusion of this spacer
    // discounts matches found after it.
    static const wxString discount( wxT( "        " ) );

    wxString  text = GetKeyWords() + discount + GetDescription();
    wxString  footprint = GetFootprintField().GetText();

    if( !footprint.IsEmpty() )
    {
        text += discount + footprint;
    }

    return text;
}


bool operator<( const LIB_PART& aItem1, const LIB_PART& aItem2 )
{
    return aItem1.GetName() < aItem2.GetName();
}


/// http://www.boost.org/doc/libs/1_55_0/libs/smart_ptr/sp_techniques.html#weak_without_shared
struct null_deleter
{
    void operator()(void const *) const
    {
    }
};


LIB_PART::LIB_PART( const wxString& aName, LIB_PART* aParent, PART_LIB* aLibrary ) :
    EDA_ITEM( LIB_PART_T ),
    m_me( this, null_deleter() ),
    m_includeInBom( true ),
    m_includeOnBoard( true )
{
    m_lastModDate    = 0;
    m_unitCount      = 1;
    m_pinNameOffset  = Mils2iu( DEFAULT_PIN_NAME_OFFSET );
    m_options        = ENTRY_NORMAL;
    m_unitsLocked    = false;
    m_showPinNumbers = true;
    m_showPinNames   = true;

    // Add the MANDATORY_FIELDS in RAM only.  These are assumed to be present
    // when the field editors are invoked.
    m_drawings[LIB_FIELD_T].reserve( 4 );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, VALUE_FIELD ) );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, REFERENCE_FIELD ) );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, FOOTPRINT_FIELD ) );
    m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, DATASHEET_FIELD ) );

    SetName( aName );

    if( aParent )
        SetParent( aParent );

    SetLib( aLibrary );
}


LIB_PART::LIB_PART( const LIB_PART& aPart, PART_LIB* aLibrary ) :
    EDA_ITEM( aPart ),
    m_me( this, null_deleter() )
{
    LIB_ITEM* newItem;

    m_library        = aLibrary;
    m_name           = aPart.m_name;
    m_fpFilters      = wxArrayString( aPart.m_fpFilters );
    m_unitCount      = aPart.m_unitCount;
    m_unitsLocked    = aPart.m_unitsLocked;
    m_pinNameOffset  = aPart.m_pinNameOffset;
    m_showPinNumbers = aPart.m_showPinNumbers;
    m_includeInBom   = aPart.m_includeInBom;
    m_includeOnBoard = aPart.m_includeOnBoard;
    m_showPinNames   = aPart.m_showPinNames;
    m_lastModDate    = aPart.m_lastModDate;
    m_options        = aPart.m_options;
    m_libId          = aPart.m_libId;
    m_description    = aPart.m_description;
    m_keyWords       = aPart.m_keyWords;

    ClearSelected();

    for( const LIB_ITEM& oldItem : aPart.m_drawings )
    {
        if( ( oldItem.GetFlags() & ( IS_NEW | STRUCT_DELETED ) ) != 0 )
            continue;

        try
        {
            newItem = (LIB_ITEM*) oldItem.Clone();
            newItem->ClearSelected();
            newItem->SetParent( this );
            m_drawings.push_back( newItem );
        }
        catch( ... )
        {
            wxFAIL_MSG( "Failed to clone LIB_ITEM." );
        }
    }

    PART_SPTR parent = aPart.m_parent.lock();

    if( parent )
        SetParent( parent.get() );
}


LIB_PART::~LIB_PART()
{
}


const LIB_PART& LIB_PART::operator=( const LIB_PART& aPart )
{
    if( &aPart == this )
        return aPart;

    LIB_ITEM* newItem;

    m_library        = aPart.m_library;
    m_name           = aPart.m_name;
    m_fpFilters      = wxArrayString( aPart.m_fpFilters );
    m_unitCount      = aPart.m_unitCount;
    m_unitsLocked    = aPart.m_unitsLocked;
    m_pinNameOffset  = aPart.m_pinNameOffset;
    m_showPinNumbers = aPart.m_showPinNumbers;
    m_showPinNames   = aPart.m_showPinNames;
    m_includeInBom   = aPart.m_includeInBom;
    m_includeOnBoard = aPart.m_includeOnBoard;
    m_lastModDate    = aPart.m_lastModDate;
    m_options        = aPart.m_options;
    m_libId          = aPart.m_libId;
    m_description    = aPart.m_description;
    m_keyWords       = aPart.m_keyWords;

    m_drawings.clear();

    for( const LIB_ITEM& oldItem : aPart.m_drawings )
    {
        if( ( oldItem.GetFlags() & ( IS_NEW | STRUCT_DELETED ) ) != 0 )
            continue;

        newItem = (LIB_ITEM*) oldItem.Clone();
        newItem->SetParent( this );
        m_drawings.push_back( newItem );
    }

    PART_SPTR parent = aPart.m_parent.lock();

    if( parent )
        SetParent( parent.get() );

    return *this;
}


int LIB_PART::Compare( const LIB_PART& aRhs ) const
{
    if( m_me == aRhs.m_me )
        return 0;

    int retv = m_name.Cmp( aRhs.m_name );

    if( retv )
        return retv;

    retv = m_libId.compare( aRhs.m_libId );

    if( retv )
        return retv;

    if( m_parent.lock() < aRhs.m_parent.lock() )
        return -1;

    if( m_parent.lock() > aRhs.m_parent.lock() )
        return 1;

    if( m_options != aRhs.m_options )
        return ( m_options == ENTRY_NORMAL ) ? -1 : 1;

    if( m_unitCount != aRhs.m_unitCount )
        return m_unitCount - aRhs.m_unitCount;

    if( m_drawings.size() != aRhs.m_drawings.size() )
        return m_drawings.size() - aRhs.m_drawings.size();

    LIB_ITEMS_CONTAINER::CONST_ITERATOR lhsItem = m_drawings.begin();
    LIB_ITEMS_CONTAINER::CONST_ITERATOR rhsItem = aRhs.m_drawings.begin();

    while( lhsItem != m_drawings.end() )
    {
        if( lhsItem->Type() != rhsItem->Type() )
            return lhsItem->Type() - rhsItem->Type();

        retv = lhsItem->compare( *rhsItem );

        if( retv )
            return retv;

        ++lhsItem;
        ++rhsItem;
    }

    if( m_fpFilters.GetCount() != aRhs.m_fpFilters.GetCount() )
        return m_fpFilters.GetCount() - aRhs.m_fpFilters.GetCount();

    for( size_t i = 0; i < m_fpFilters.GetCount(); i++ )
    {
        retv = m_fpFilters[i].Cmp( aRhs.m_fpFilters[i] );

        if( retv )
            return retv;
    }

    retv = m_description.Cmp( aRhs.m_description );

    if( retv )
        return retv;

    retv = m_keyWords.Cmp( aRhs.m_keyWords );

    if( retv )
        return retv;

    if( m_pinNameOffset != aRhs.m_pinNameOffset )
        return m_pinNameOffset - aRhs.m_pinNameOffset;

    if( m_unitsLocked != aRhs.m_unitsLocked )
        return ( m_unitsLocked ) ? 1 : -1;

    if( m_showPinNames != aRhs.m_showPinNames )
        return ( m_showPinNames ) ? 1 : -1;

    if( m_showPinNumbers != aRhs.m_showPinNumbers )
        return ( m_showPinNumbers ) ? 1 : -1;

    if( m_includeInBom != aRhs.m_includeInBom )
        return ( m_includeInBom ) ? 1 : -1;

    if( m_includeOnBoard != aRhs.m_includeOnBoard )
        return ( m_includeOnBoard ) ? 1 : -1;

    return 0;
}


wxString LIB_PART::GetUnitReference( int aUnit )
{
    return LIB_PART::SubReference( aUnit, false );
}


void LIB_PART::SetName( const wxString& aName )
{
    wxString validatedName = LIB_ID::FixIllegalChars( aName );

    m_name = validatedName;
    m_libId.SetLibItemName( validatedName, false );

    GetValueField().SetText( validatedName );
}


void LIB_PART::SetParent( LIB_PART* aParent )
{
    if( aParent )
        m_parent = aParent->SharedPtr();
    else
        m_parent.reset();
}


std::unique_ptr< LIB_PART > LIB_PART::Flatten() const
{
    std::unique_ptr< LIB_PART > retv;

    if( IsAlias() )
    {
        PART_SPTR parent = m_parent.lock();

        wxCHECK_MSG( parent, retv,
                     wxString::Format( "Parent of derived symbol '%s' undefined", m_name ) );

        // Copy the parent.
        retv.reset( new LIB_PART( *parent.get() ) );

        retv->SetName( m_name );

        // Now add the inherited part mandatory field (this) information.
        for( int i = 0; i < MANDATORY_FIELDS; i++ )
        {
            wxString tmp = GetField( i )->GetText();

            // If the field isn't defined then inherit the parent field value.
            if( tmp.IsEmpty() )
                retv->GetField( i )->SetText( parent->GetField( i )->GetText() );
            else
                *retv->GetField( i ) = *GetField( i );
        }

        // Grab all the rest of derived symbol fields.
        for( const LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
        {
            const LIB_FIELD* aliasField = dynamic_cast<const LIB_FIELD*>( &item );

            wxCHECK2( aliasField, continue );

            // Mandatory fields were already resolved.
            if( aliasField->IsMandatory() )
                continue;

            LIB_FIELD* newField = new LIB_FIELD( *aliasField );
            newField->SetParent( retv.get() );

            LIB_FIELD* parentField = retv->FindField( aliasField->GetName() );

            if( !parentField )  // Derived symbol field does not exist in parent symbol.
            {
                retv->AddDrawItem( newField );
            }
            else                // Derived symbol field overrides the parent symbol field.
            {
                retv->RemoveDrawItem( parentField );
                retv->AddDrawItem( newField );
            }
        }

        retv->SetKeyWords( m_keyWords.IsEmpty() ? parent->GetKeyWords() : m_keyWords );
        retv->SetDescription( m_description.IsEmpty() ? parent->GetDescription() : m_description );
        retv->SetFPFilters( m_fpFilters.IsEmpty() ? parent->GetFPFilters() : m_fpFilters );
    }
    else
    {
        retv.reset( new LIB_PART( *this ) );
    }

    return retv;
}


const wxString LIB_PART::GetLibraryName() const
{
    if( m_library )
        return m_library->GetName();

    return m_libId.GetLibNickname();
}


bool LIB_PART::IsPower() const
{
    if( PART_SPTR parent = m_parent.lock() )
        return parent->m_options == ENTRY_POWER;

    return m_options == ENTRY_POWER;
}


void LIB_PART::SetPower()
{
    if( PART_SPTR parent = m_parent.lock() )
        parent->m_options = ENTRY_POWER;

    m_options = ENTRY_POWER;
}


bool LIB_PART::IsNormal() const
{
    if( PART_SPTR parent = m_parent.lock() )
        return parent->m_options == ENTRY_NORMAL;

    return m_options == ENTRY_NORMAL;
}


void LIB_PART::SetNormal()
{
    if( PART_SPTR parent = m_parent.lock() )
        parent->m_options = ENTRY_NORMAL;

    m_options = ENTRY_NORMAL;
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


void LIB_PART::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                      int aMulti, int aConvert, const PART_DRAW_OPTIONS& aOpts )
{
    /* draw background for filled items using background option
     * Solid lines will be drawn after the background
     * Note also, background is not drawn when printing in black and white
     */
    if( !GetGRForceBlackPenState() )
    {
        for( LIB_ITEM& drawItem : m_drawings )
        {
            if( drawItem.m_fill != FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
                continue;

            // Do not draw items not attached to the current part
            if( aMulti && drawItem.m_unit && ( drawItem.m_unit != aMulti ) )
                continue;

            if( aConvert && drawItem.m_convert && ( drawItem.m_convert != aConvert ) )
                continue;

            if( drawItem.Type() == LIB_FIELD_T )
                continue;

            // Now, draw only the background for items with
            // m_fill == FILLED_WITH_BG_BODYCOLOR:
            drawItem.Print( aSettings, aOffset, (void*) false, aOpts.transform );
        }
    }

    for( LIB_ITEM& drawItem : m_drawings )
    {
        // Do not draw items not attached to the current part
        if( aMulti && drawItem.m_unit && ( drawItem.m_unit != aMulti ) )
            continue;

        if( aConvert && drawItem.m_convert && ( drawItem.m_convert != aConvert ) )
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
            drawItem.Print( aSettings, aOffset, (void*) &aOpts, aOpts.transform );
        }
        else if( drawItem.Type() == LIB_FIELD_T )
        {
            drawItem.Print( aSettings, aOffset, (void*) NULL, aOpts.transform );
        }
        else
        {
            bool forceNoFill = drawItem.m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR;
            drawItem.Print( aSettings, aOffset, (void*) forceNoFill, aOpts.transform );
        }
    }
}


void LIB_PART::Plot( PLOTTER* aPlotter, int aUnit, int aConvert, const wxPoint& aOffset,
                     const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE ) );
    bool fill = aPlotter->GetColorMode();

    // draw background for filled items using background option
    // Solid lines will be drawn after the background
    for( LIB_ITEM& item : m_drawings )
    {
        // Lib Fields are not plotted here, because this plot function
        // is used to plot schematic items, which have they own fields
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aConvert && item.m_convert && ( item.m_convert != aConvert ) )
            continue;

        if( item.m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
            item.Plot( aPlotter, aOffset, fill, aTransform );
    }

    // Not filled items and filled shapes are now plotted
    // Items that have BG fills only get re-stroked to ensure the edges are in the foreground
    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aConvert && item.m_convert && ( item.m_convert != aConvert ) )
            continue;

        item.Plot( aPlotter, aOffset,
                   fill && ( item.m_fill != FILL_TYPE::FILLED_WITH_BG_BODYCOLOR ), aTransform );
    }
}


void LIB_PART::PlotLibFields( PLOTTER* aPlotter, int aUnit, int aConvert,
                              const wxPoint& aOffset, const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_FIELDS ) );
    bool fill = aPlotter->GetColorMode();

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aConvert && item.m_convert && ( item.m_convert != aConvert ) )
            continue;

        LIB_FIELD& field = (LIB_FIELD&) item;

        // The reference is a special case: we should change the basic text
        // to add '?' and the part id
        wxString tmp = field.GetShownText();

        if( field.GetId() == REFERENCE_FIELD )
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
        if( static_cast<LIB_FIELD*>( aItem )->IsMandatory() )
            return;
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


LIB_ITEM* LIB_PART::GetNextDrawItem( const LIB_ITEM* aItem, KICAD_T aType )
{
    if( aItem == NULL )
    {
        LIB_ITEMS_CONTAINER::ITERATOR it1 = m_drawings.begin( aType );

        return (it1 != m_drawings.end( aType ) ) ? &( *( m_drawings.begin( aType ) ) ) : nullptr;
    }

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
    /* Notes:
     * when aUnit == 0: no unit filtering
     * when aConvert == 0: no convert (shape selection) filtering
     * when m_unit == 0, the body item is common to units
     * when m_convert == 0, the body item is common to shapes
     */
    for( LIB_ITEM& item : m_drawings[ LIB_PIN_T ] )
    {
        // Unit filtering:
        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
             continue;

        // Shape filtering:
        if( aConvert && item.m_convert && ( item.m_convert != aConvert ) )
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

            // Same unit?
            if( eachThisPin->GetUnit() != eachOtherPin->GetUnit() )
                continue;

            // Same body stype?
            if( eachThisPin->GetConvert() != eachOtherPin->GetConvert() )
                continue;

            // Same position?
            if( eachThisPin->GetPosition() != eachOtherPin->GetPosition() )
                continue;

            // Same number?
            if( aTestNums && ( eachThisPin->GetNumber() != eachOtherPin->GetNumber() ) )
                continue;

            // Same name?
            if( aTestNames && ( eachThisPin->GetName() != eachOtherPin->GetName() ) )
                continue;

            // Same electrical type?
            if( aTestType && ( eachThisPin->GetType() != eachOtherPin->GetType() ) )
                continue;

            // Same orientation?
            if( aTestOrientation
              && ( eachThisPin->GetOrientation() != eachOtherPin->GetOrientation() ) )
                continue;

            // Same length?
            if( aTestLength && ( eachThisPin->GetLength() != eachOtherPin->GetLength() ) )
                continue;

            foundMatch = true;
            break;                    // Match found so seach is complete.
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
        if( item.m_unit > 0
                && m_unitCount > 1
                && aUnit > 0
                && aUnit != item.m_unit )
        {
            continue;
        }

        if( item.m_convert > 0 && aConvert > 0 && aConvert != item.m_convert )
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
    aCount      = 6;
    aLayers[0]  = LAYER_DEVICE;
    aLayers[1]  = LAYER_DEVICE_BACKGROUND;
    aLayers[2]  = LAYER_REFERENCEPART;
    aLayers[3]  = LAYER_VALUEPART;
    aLayers[4]  = LAYER_FIELDS;
    aLayers[5]  = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT LIB_PART::GetBodyBoundingBox( int aUnit, int aConvert ) const
{
    EDA_RECT bbox;

    for( const LIB_ITEM& item : m_drawings )
    {
        if( item.m_unit > 0
                && m_unitCount > 1
                && aUnit > 0
                && aUnit != item.m_unit )
        {
            continue;
        }

        if( item.m_convert > 0 && aConvert > 0 && aConvert != item.m_convert )
            continue;

        if( item.Type() == LIB_FIELD_T )
            continue;

        bbox.Merge( item.GetBoundingBox() );
    }

    return bbox;
}


void LIB_PART::deleteAllFields()
{
    m_drawings[ LIB_FIELD_T ].clear();
}


void LIB_PART::AddField( LIB_FIELD* aField )
{
    AddDrawItem( aField );
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


void LIB_PART::GetFields( std::vector<LIB_FIELD*>& aList )
{
    // Grab the MANDATORY_FIELDS first, in expected order given by enum NumFieldType
    for( int id = 0; id < MANDATORY_FIELDS; ++id )
        aList.push_back( GetField( id ) );

    // Now grab all the rest of fields.
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = static_cast<LIB_FIELD*>( &item );

        if( !field->IsMandatory() )
            aList.push_back( field );
    }
}


void LIB_PART::GetFields( std::vector<LIB_FIELD>& aList )
{
    // Grab the MANDATORY_FIELDS first, in expected order given by enum NumFieldType
    for( int id = 0; id < MANDATORY_FIELDS; ++id )
        aList.push_back( *GetField( id ) );

    // Now grab all the rest of fields.
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = static_cast<LIB_FIELD*>( &item );

        if( !field->IsMandatory() )
            aList.push_back( *field );
    }
}


LIB_FIELD* LIB_PART::GetField( int aId ) const
{
    for( const LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
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
        if( static_cast<LIB_FIELD*>( &item )->GetCanonicalName() == aFieldName )
            return static_cast<LIB_FIELD*>( &item );
    }

    return NULL;
}


const LIB_FIELD* LIB_PART::FindField( const wxString& aFieldName ) const
{
    for( const LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        if( static_cast<const LIB_FIELD*>( &item )->GetCanonicalName() == aFieldName )
            return static_cast<const LIB_FIELD*>( &item );
    }

    return NULL;
}


LIB_FIELD& LIB_PART::GetValueField()
{
    LIB_FIELD* field = GetField( VALUE_FIELD );
    wxASSERT( field != NULL );
    return *field;
}


LIB_FIELD& LIB_PART::GetReferenceField()
{
    LIB_FIELD* field = GetField( REFERENCE_FIELD );
    wxASSERT( field != NULL );
    return *field;
}


LIB_FIELD& LIB_PART::GetFootprintField()
{
    LIB_FIELD* field = GetField( FOOTPRINT_FIELD );
    wxASSERT( field != NULL );
    return *field;
}


LIB_FIELD& LIB_PART::GetDatasheetField()
{
    LIB_FIELD* field = GetField( DATASHEET_FIELD );
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
        if( item.m_convert > LIB_ITEM::LIB_CONVERT::BASE )
            return true;
    }

    if( PART_SPTR parent = m_parent.lock() )
    {
        for( const LIB_ITEM& item : parent->GetDrawItems() )
        {
            if( item.m_convert > LIB_ITEM::LIB_CONVERT::BASE )
                return true;
        }
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
        if( ( aUnit && item.m_unit && aUnit != item.m_unit )
                || ( aConvert && item.m_convert && aConvert != item.m_convert )
                || ( item.Type() != aType && aType != TYPE_NOT_INIT ) )
        {
            continue;
        }

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
            if( aInspector( &item, aTestData ) == SEARCH_RESULT::QUIT )
                return SEARCH_RESULT::QUIT;
        }
    }

    return SEARCH_RESULT::CONTINUE;
}


void LIB_PART::SetUnitCount( int aCount, bool aDuplicateDrawItems )
{
    if( m_unitCount == aCount )
        return;

    if( aCount < m_unitCount )
    {
        LIB_ITEMS_CONTAINER::ITERATOR i = m_drawings.begin();

        while( i != m_drawings.end() )
        {
            if( i->m_unit > aCount )
                i = m_drawings.erase( i );
            else
                ++i;
        }
    }
    else if( aDuplicateDrawItems )
    {
        int prevCount = m_unitCount;

        // Temporary storage for new items, as adding new items directly to
        // m_drawings may cause the buffer reallocation which invalidates the
        // iterators
        std::vector< LIB_ITEM* > tmp;

        for( LIB_ITEM& item : m_drawings )
        {
            if( item.m_unit != 1 )
                continue;

            for( int j = prevCount + 1; j <= aCount; j++ )
            {
                LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
                newItem->m_unit = j;
                tmp.push_back( newItem );
            }
        }

        for( auto item : tmp )
            m_drawings.push_back( item );
    }

    m_unitCount = aCount;
}


int LIB_PART::GetUnitCount() const
{
    if( PART_SPTR parent = m_parent.lock() )
        return parent->GetUnitCount();

    return m_unitCount;
}


void LIB_PART::SetConversion( bool aSetConvert, bool aDuplicatePins )
{
    if( aSetConvert == HasConversion() )
        return;

    // Duplicate items to create the converted shape
    if( aSetConvert )
    {
        if( aDuplicatePins )
        {
            std::vector< LIB_ITEM* > tmp;     // Temporarily store the duplicated pins here.

            for( LIB_ITEM& item : m_drawings )
            {
                // Only pins are duplicated.
                if( item.Type() != LIB_PIN_T )
                    continue;

                if( item.m_convert == 1 )
                {
                    LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
                    newItem->m_convert = 2;
                    tmp.push_back( newItem );
                }
            }

            // Transfer the new pins to the LIB_PART.
            for( unsigned i = 0;  i < tmp.size();  i++ )
                m_drawings.push_back( tmp[i] );
        }
    }
    else
    {
        // Delete converted shape items because the converted shape does
        // not exist
        LIB_ITEMS_CONTAINER::ITERATOR i = m_drawings.begin();

        while( i != m_drawings.end() )
        {
            if( i->m_convert > 1 )
                i = m_drawings.erase( i );
            else
                ++i;
        }
    }
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


std::vector<LIB_ITEM*> LIB_PART::GetUnitItems( int aUnit, int aConvert )
{
    std::vector<LIB_ITEM*> unitItems;

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( ( aConvert == -1 && item.GetUnit() == aUnit )
          || ( aUnit == -1 && item.GetConvert() == aConvert )
          || ( aUnit == item.GetUnit() && aConvert == item.GetConvert() ) )
            unitItems.push_back( &item );
    }

    return unitItems;
}


std::vector<struct PART_UNITS> LIB_PART::GetUnitDrawItems()
{
    std::vector<struct PART_UNITS> units;

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        int unit = item.GetUnit();
        int convert = item.GetConvert();

        auto it = std::find_if( units.begin(), units.end(),
                [unit, convert] ( const auto& a ) {
                    return a.m_unit == unit && a.m_convert == convert;
                } );

        if( it == units.end() )
        {
            struct PART_UNITS newUnit;
            newUnit.m_unit = item.GetUnit();
            newUnit.m_convert = item.GetConvert();
            newUnit.m_items.push_back( &item );
            units.emplace_back( newUnit );
        }
        else
        {
            it->m_items.push_back( &item );
        }
    }

    return units;
}


std::vector<struct PART_UNITS> LIB_PART::GetUniqueUnits()
{
    int unitNum;
    size_t i;
    struct PART_UNITS unit;
    std::vector<LIB_ITEM*> compareDrawItems;
    std::vector<LIB_ITEM*> currentDrawItems;
    std::vector<struct PART_UNITS> uniqueUnits;

    // The first unit is guarenteed to be unique so always include it.
    unit.m_unit = 1;
    unit.m_convert = 1;
    unit.m_items = GetUnitItems( 1, 1 );

    // There are no unique units if there are no draw items other than fields.
    if( unit.m_items.size() == 0 )
        return uniqueUnits;

    uniqueUnits.emplace_back( unit );

    if( ( GetUnitCount() == 1 || UnitsLocked() ) && !HasConversion() )
        return uniqueUnits;

    currentDrawItems = unit.m_items;

    for( unitNum = 2; unitNum <= GetUnitCount(); unitNum++ )
    {
        compareDrawItems = GetUnitItems( unitNum, 1 );

        wxCHECK2_MSG( compareDrawItems.size() != 0, continue,
                      "Multiple unit symbol defined with empty units." );

        if( currentDrawItems.size() != compareDrawItems.size() )
        {
            unit.m_unit = unitNum;
            unit.m_convert = 1;
            unit.m_items = compareDrawItems;
            uniqueUnits.emplace_back( unit );
        }
        else
        {
            for( i = 0; i < currentDrawItems.size(); i++ )
            {
                if( currentDrawItems[i]->compare( *compareDrawItems[i],
                                                  LIB_ITEM::COMPARE_FLAGS::UNIT ) != 0 )
                {
                    unit.m_unit = unitNum;
                    unit.m_convert = 1;
                    unit.m_items = compareDrawItems;
                    uniqueUnits.emplace_back( unit );
                }
            }
        }
    }

    if( HasConversion() )
    {
        currentDrawItems = GetUnitItems( 1, 2 );

        if( ( GetUnitCount() == 1 || UnitsLocked() ) )
        {
            unit.m_unit = 1;
            unit.m_convert = 2;
            unit.m_items = currentDrawItems;
            uniqueUnits.emplace_back( unit );

            return uniqueUnits;
        }

        for( unitNum = 2; unitNum <= GetUnitCount(); unitNum++ )
        {
            compareDrawItems = GetUnitItems( unitNum, 2 );

            wxCHECK2_MSG( compareDrawItems.size() != 0, continue,
                          "Multiple unit symbol defined with empty units." );

            if( currentDrawItems.size() != compareDrawItems.size() )
            {
                unit.m_unit = unitNum;
                unit.m_convert = 2;
                unit.m_items = compareDrawItems;
                uniqueUnits.emplace_back( unit );
            }
            else
            {
                for( i = 0; i < currentDrawItems.size(); i++ )
                {
                    if( currentDrawItems[i]->compare( *compareDrawItems[i],
                                                      LIB_ITEM::COMPARE_FLAGS::UNIT ) != 0 )
                    {
                        unit.m_unit = unitNum;
                        unit.m_convert = 2;
                        unit.m_items = compareDrawItems;
                        uniqueUnits.emplace_back( unit );
                    }
                }
            }
        }
    }

    return uniqueUnits;
}
