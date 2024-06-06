/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 CERN
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <plotters/plotter.h>
#include <sch_screen.h>
#include <richio.h>
#include <template_fieldnames.h>
#include <transform.h>
#include <symbol_library.h>
#include <lib_pin.h>
#include <settings/color_settings.h>
#include <lib_shape.h>

#include <memory>

std::vector<SEARCH_TERM> LIB_SYMBOL::GetSearchTerms()
{
    std::vector<SEARCH_TERM> terms;

    terms.emplace_back( SEARCH_TERM( GetName(), 8 ) );

    wxStringTokenizer keywordTokenizer( GetKeyWords(), wxS( " " ), wxTOKEN_STRTOK );

    while( keywordTokenizer.HasMoreTokens() )
        terms.emplace_back( SEARCH_TERM( keywordTokenizer.GetNextToken(), 4 ) );

    // TODO(JE) rework this later so we can highlight matches in their column
    std::map<wxString, wxString> fields;
    GetChooserFields( fields );

    for( const auto& [ name, text ] : fields )
        terms.emplace_back( SEARCH_TERM( text, 4 ) );

    // Also include keywords as one long string, just in case
    terms.emplace_back( SEARCH_TERM( GetKeyWords(), 1 ) );
    terms.emplace_back( SEARCH_TERM( GetDescription(), 1 ) );

    wxString  footprint = GetFootprintField().GetText();

    if( !footprint.IsEmpty() )
        terms.emplace_back( SEARCH_TERM( GetFootprintField().GetText(), 1 ) );

    return terms;
}


void LIB_SYMBOL::GetChooserFields( std::map<wxString , wxString>& aColumnMap )
{
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = static_cast<LIB_FIELD*>( &item );

        if( field->ShowInChooser() )
            aColumnMap[field->GetName()] = field->EDA_TEXT::GetShownText( false );
    }
}


bool operator<( const LIB_SYMBOL& aItem1, const LIB_SYMBOL& aItem2 )
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


LIB_SYMBOL::LIB_SYMBOL( const wxString& aName, LIB_SYMBOL* aParent, SYMBOL_LIB* aLibrary ) :
    EDA_ITEM( LIB_SYMBOL_T ),
    m_me( this, null_deleter() ),
    m_excludedFromSim( false ),
    m_excludedFromBOM( false ),
    m_excludedFromBoard( false )
{
    m_lastModDate    = 0;
    m_unitCount      = 1;
    m_pinNameOffset  = schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET );
    m_options        = ENTRY_NORMAL;
    m_unitsLocked    = false;
    m_showPinNumbers = true;
    m_showPinNames   = true;

    // Add the MANDATORY_FIELDS in RAM only.  These are assumed to be present
    // when the field editors are invoked.
    m_drawings[LIB_FIELD_T].reserve( MANDATORY_FIELDS );
    for( int i = 0; i < MANDATORY_FIELDS; i++ )
        m_drawings[LIB_FIELD_T].push_back( new LIB_FIELD( this, i ) );

    SetName( aName );

    if( aParent )
        SetParent( aParent );

    SetLib( aLibrary );
}


LIB_SYMBOL::LIB_SYMBOL( const LIB_SYMBOL& aSymbol, SYMBOL_LIB* aLibrary ) :
    EDA_ITEM( aSymbol ),
    m_me( this, null_deleter() )
{
    LIB_ITEM* newItem;

    m_library        = aLibrary;
    m_name           = aSymbol.m_name;
    m_fpFilters      = wxArrayString( aSymbol.m_fpFilters );
    m_unitCount      = aSymbol.m_unitCount;
    m_unitsLocked    = aSymbol.m_unitsLocked;
    m_pinNameOffset  = aSymbol.m_pinNameOffset;
    m_showPinNumbers = aSymbol.m_showPinNumbers;
    m_excludedFromSim = aSymbol.m_excludedFromSim;
    m_excludedFromBOM = aSymbol.m_excludedFromBOM;
    m_excludedFromBoard = aSymbol.m_excludedFromBoard;
    m_showPinNames      = aSymbol.m_showPinNames;
    m_lastModDate    = aSymbol.m_lastModDate;
    m_options        = aSymbol.m_options;
    m_libId          = aSymbol.m_libId;
    m_keyWords       = aSymbol.m_keyWords;

    aSymbol.CopyUnitDisplayNames( m_unitDisplayNames );

    ClearSelected();

    for( const LIB_ITEM& oldItem : aSymbol.m_drawings )
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
            return;
        }
    }

    LIB_SYMBOL_SPTR parent = aSymbol.m_parent.lock();

    if( parent )
        SetParent( parent.get() );
}


const LIB_SYMBOL& LIB_SYMBOL::operator=( const LIB_SYMBOL& aSymbol )
{
    if( &aSymbol == this )
        return aSymbol;

    LIB_ITEM* newItem;

    m_library        = aSymbol.m_library;
    m_name           = aSymbol.m_name;
    m_fpFilters      = wxArrayString( aSymbol.m_fpFilters );
    m_unitCount      = aSymbol.m_unitCount;
    m_unitsLocked    = aSymbol.m_unitsLocked;
    m_pinNameOffset  = aSymbol.m_pinNameOffset;
    m_showPinNumbers = aSymbol.m_showPinNumbers;
    m_showPinNames   = aSymbol.m_showPinNames;
    m_excludedFromSim = aSymbol.m_excludedFromSim;
    m_excludedFromBOM = aSymbol.m_excludedFromBOM;
    m_excludedFromBoard = aSymbol.m_excludedFromBoard;
    m_lastModDate    = aSymbol.m_lastModDate;
    m_options        = aSymbol.m_options;
    m_libId          = aSymbol.m_libId;
    m_keyWords       = aSymbol.m_keyWords;

    m_unitDisplayNames.clear();
    aSymbol.CopyUnitDisplayNames( m_unitDisplayNames );

    m_drawings.clear();

    for( const LIB_ITEM& oldItem : aSymbol.m_drawings )
    {
        if( ( oldItem.GetFlags() & ( IS_NEW | STRUCT_DELETED ) ) != 0 )
            continue;

        newItem = (LIB_ITEM*) oldItem.Clone();
        newItem->SetParent( this );
        m_drawings.push_back( newItem );
    }

    m_drawings.sort();

    LIB_SYMBOL_SPTR parent = aSymbol.m_parent.lock();

    if( parent )
        SetParent( parent.get() );

    return *this;
}


unsigned LIB_SYMBOL::GetInheritanceDepth() const
{
    unsigned depth = 0;

    LIB_SYMBOL_SPTR parent = GetParent().lock();

    while( parent )
    {
        depth += 1;
        parent = parent->GetParent().lock();
    }

    return depth;
}


#define REPORT( msg ) { if( aReporter ) aReporter->Report( msg ); }
#define ITEM_DESC( item ) ( item )->GetItemDescription( &unitsProvider )

int LIB_SYMBOL::Compare( const LIB_SYMBOL& aRhs, int aCompareFlags, REPORTER* aReporter ) const
{
    UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MILLIMETRES );

    if( m_me == aRhs.m_me )
        return 0;

    if( !aReporter && ( aCompareFlags & LIB_ITEM::COMPARE_FLAGS::ERC ) == 0 )
    {
        if( int tmp = m_name.Cmp( aRhs.m_name ) )
            return tmp;

        if( int tmp = m_libId.compare( aRhs.m_libId ) )
            return tmp;

        if( m_parent.lock() < aRhs.m_parent.lock() )
            return -1;

        if( m_parent.lock() > aRhs.m_parent.lock() )
            return 1;
    }

    int retv = 0;

    if( m_options != aRhs.m_options )
    {
        retv = ( m_options == ENTRY_NORMAL ) ? -1 : 1;
        REPORT( _( "Power flag differs." ) );

        if( !aReporter )
            return retv;
    }

    if( int tmp = m_unitCount - aRhs.m_unitCount )
    {
        retv = tmp;
        REPORT( _( "Unit count differs." ) );

        if( !aReporter )
            return retv;
    }

    // Make sure shapes and pins are sorted. No need with fields as those are
    // matched by id/name.

    std::set<const LIB_ITEM*, LIB_ITEM::cmp_items> aShapes;
    std::set<const LIB_ITEM*>                      aFields;
    std::set<const LIB_ITEM*, LIB_ITEM::cmp_items> aPins;

    for( auto it = m_drawings.begin(); it != m_drawings.end(); ++it )
    {
        if( it->Type() == LIB_SHAPE_T )
            aShapes.insert( &(*it) );
        else if( it->Type() == LIB_FIELD_T )
            aFields.insert( &(*it) );
        else if( it->Type() == LIB_PIN_T )
            aPins.insert( &(*it) );
    }

    std::set<const LIB_ITEM*, LIB_ITEM::cmp_items> bShapes;
    std::set<const LIB_ITEM*>                      bFields;
    std::set<const LIB_ITEM*, LIB_ITEM::cmp_items> bPins;

    for( auto it = aRhs.m_drawings.begin(); it != aRhs.m_drawings.end(); ++it )
    {
        if( it->Type() == LIB_SHAPE_T )
            bShapes.insert( &(*it) );
        else if( it->Type() == LIB_FIELD_T )
            bFields.insert( &(*it) );
        else if( it->Type() == LIB_PIN_T )
            bPins.insert( &(*it) );
    }

    if( int tmp = static_cast<int>( aShapes.size() - bShapes.size() ) )
    {
        retv = tmp;
        REPORT( _( "Graphic item count differs." ) );

        if( !aReporter )
            return retv;
    }
    else
    {
        for( auto aIt = aShapes.begin(), bIt = bShapes.begin(); aIt != aShapes.end(); aIt++, bIt++ )
        {
            if( int tmp2 = (*aIt)->compare( *(*bIt), aCompareFlags ) )
            {
                retv = tmp2;
                REPORT( wxString::Format( _( "%s differs." ), ITEM_DESC( *aIt ) ) );

                if( !aReporter )
                    return retv;
            }
        }
    }

    if( int tmp = static_cast<int>( aPins.size() - bPins.size() ) )
    {
        retv = tmp;
        REPORT( _( "Pin count differs." ) );

        if( !aReporter )
            return retv;
    }
    else
    {
        for( const LIB_ITEM* aPinItem : aPins )
        {
            const LIB_PIN* aPin = static_cast<const LIB_PIN*>( aPinItem );
            const LIB_PIN* bPin = aRhs.GetPin( aPin->GetNumber(), aPin->GetUnit(),
                                               aPin->GetBodyStyle() );

            if( !bPin )
            {
                retv = 1;
                REPORT( wxString::Format( _( "Pin %s not found." ), aPin->GetNumber() ) );

                if( !aReporter )
                    return retv;
            }
            else if( int tmp2 = aPinItem->compare( *bPin, aCompareFlags ) )
            {
                retv = tmp2;
                REPORT( wxString::Format( _( "Pin %s differs." ), aPin->GetNumber() ) );

                if( !aReporter )
                    return retv;
            }
        }
    }

    for( const LIB_ITEM* aFieldItem : aFields )
    {
        const LIB_FIELD* aField = static_cast<const LIB_FIELD*>( aFieldItem );
        const LIB_FIELD* bField = nullptr;
        int              tmp = 0;

        if( aField->GetId() >= 0 && aField->GetId() < MANDATORY_FIELDS )
            bField = aRhs.GetFieldById( aField->GetId() );
        else
            bField = aRhs.FindField( aField->GetName() );

        if( !bField )
        {
            tmp = 1;
        }
        else
        {
            tmp = aFieldItem->compare( *bField, aCompareFlags );
        }

        if( tmp )
        {
            retv = tmp;
            REPORT( wxString::Format( _( "%s field differs." ), aField->GetName( false ) ) );

            if( !aReporter )
                return retv;
        }
    }

    if( int tmp = static_cast<int>( aFields.size() - bFields.size() ) )
    {
        retv = tmp;
        REPORT( _( "Field count differs." ) );

        if( !aReporter )
            return retv;
    }

    if( int tmp = static_cast<int>( m_fpFilters.GetCount() - aRhs.m_fpFilters.GetCount() ) )
    {
        retv = tmp;
        REPORT( _( "Footprint filters differs." ) );

        if( !aReporter )
            return retv;
    }
    else
    {
        for( size_t i = 0; i < m_fpFilters.GetCount(); i++ )
        {
            if( int tmp2 = m_fpFilters[i].Cmp( aRhs.m_fpFilters[i] ) )
            {
                retv = tmp2;
                REPORT( _( "Footprint filters differ." ) );

                if( !aReporter )
                    return retv;
            }
        }
    }

    if( int tmp = m_keyWords.Cmp( aRhs.m_keyWords ) )
    {
        retv = tmp;
        REPORT( _( "Symbol keywords differ." ) );

        if( !aReporter )
            return retv;
    }

    if( int tmp = m_pinNameOffset - aRhs.m_pinNameOffset )
    {
        retv = tmp;
        REPORT( _( "Symbol pin name offsets differ." ) );

        if( !aReporter )
            return retv;
    }

    if( ( aCompareFlags & LIB_ITEM::COMPARE_FLAGS::ERC ) == 0 )
    {
        if( m_showPinNames != aRhs.m_showPinNames )
        {
            retv = ( m_showPinNames ) ? 1 : -1;
            REPORT( _( "Show pin names settings differ." ) );

            if( !aReporter )
                return retv;
        }

        if( m_showPinNumbers != aRhs.m_showPinNumbers )
        {
            retv = ( m_showPinNumbers ) ? 1 : -1;
            REPORT( _( "Show pin numbers settings differ." ) );

            if( !aReporter )
                return retv;
        }

        if( m_excludedFromSim != aRhs.m_excludedFromSim )
        {
            retv = ( m_excludedFromSim ) ? -1 : 1;
            REPORT( _( "Exclude from simulation settings differ." ) );

            if( !aReporter )
                return retv;
        }

        if( m_excludedFromBOM != aRhs.m_excludedFromBOM )
        {
            retv = ( m_excludedFromBOM ) ? -1 : 1;
            REPORT( _( "Exclude from bill of materials settings differ." ) );

            if( !aReporter )
                return retv;
        }

        if( m_excludedFromBoard != aRhs.m_excludedFromBoard )
        {
            retv = ( m_excludedFromBoard ) ? -1 : 1;
            REPORT( _( "Exclude from board settings differ." ) );

            if( !aReporter )
                return retv;
        }
    }

    if( !aReporter )
    {
        if( m_unitsLocked != aRhs.m_unitsLocked )
            return ( m_unitsLocked ) ? 1 : -1;

        // Compare unit display names
        if( m_unitDisplayNames < aRhs.m_unitDisplayNames )
        {
            return -1;
        }
        else if( m_unitDisplayNames > aRhs.m_unitDisplayNames )
        {
            return 1;
        }
    }

    return retv;
}


LIB_SYMBOL_SPTR LIB_SYMBOL::GetRootSymbol() const
{
    const LIB_SYMBOL_SPTR sp = m_parent.lock();

    // Recurse until the parent symbol is empty.
    if( sp )
        return sp->GetRootSymbol();

    return m_me;
}


wxString LIB_SYMBOL::GetUnitReference( int aUnit )
{
    return LIB_SYMBOL::LetterSubReference( aUnit, 'A' );
}


bool LIB_SYMBOL::HasUnitDisplayName( int aUnit )
{
    return ( m_unitDisplayNames.count( aUnit ) == 1 );
}


wxString LIB_SYMBOL::GetUnitDisplayName( int aUnit )
{
    if( HasUnitDisplayName( aUnit ) )
    {
        return m_unitDisplayNames[aUnit];
    }
    else
    {
        return wxString::Format( _( "Unit %s" ), GetUnitReference( aUnit ) );
    }
}


void LIB_SYMBOL::CopyUnitDisplayNames( std::map<int, wxString>& aTarget ) const
{
    for( const auto& it : m_unitDisplayNames )
    {
        aTarget[it.first] = it.second;
    }
}


void LIB_SYMBOL::SetUnitDisplayName( int aUnit, const wxString& aName )
{
    if( aUnit <= GetUnitCount() )
    {
        if( aName.Length() > 0 )
        {
            m_unitDisplayNames[aUnit] = aName;
        }
        else
        {
            m_unitDisplayNames.erase( aUnit );
        }
    }
}


void LIB_SYMBOL::SetName( const wxString& aName )
{
    m_name = aName;
    m_libId.SetLibItemName( aName );
}


void LIB_SYMBOL::SetParent( LIB_SYMBOL* aParent )
{
    if( aParent )
        m_parent = aParent->SharedPtr();
    else
        m_parent.reset();
}


std::unique_ptr< LIB_SYMBOL > LIB_SYMBOL::Flatten() const
{
    std::unique_ptr< LIB_SYMBOL > retv;

    if( IsAlias() )
    {
        LIB_SYMBOL_SPTR parent = m_parent.lock();

        wxCHECK_MSG( parent, retv,
                     wxString::Format( "Parent of derived symbol '%s' undefined", m_name ) );

        // Copy the parent.
        if( parent->IsAlias() )
            retv = parent->Flatten();
        else
            retv = std::make_unique<LIB_SYMBOL>( *parent.get() );

        retv->m_name = m_name;
        retv->SetLibId( m_libId );

        // Now add the inherited part mandatory field (this) information.
        for( int i = 0; i < MANDATORY_FIELDS; i++ )
        {
            wxString tmp = GetFieldById( i )->GetText();

            // If the field isn't defined then inherit the parent field value.
            if( tmp.IsEmpty() )
                retv->GetFieldById( i )->SetText( retv->GetFieldById( i )->GetText() );
            else
                *retv->GetFieldById( i ) = *GetFieldById( i );
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
        retv->SetFPFilters( m_fpFilters.IsEmpty() ? parent->GetFPFilters() : m_fpFilters );

        retv->SetExcludedFromSim( parent->GetExcludedFromSim() );
        retv->SetExcludedFromBOM( parent->GetExcludedFromBOM() );
        retv->SetExcludedFromBoard( parent->GetExcludedFromBoard() );

        retv->UpdateFieldOrdinals();
        retv->m_parent.reset();
    }
    else
    {
        retv = std::make_unique<LIB_SYMBOL>( *this );
    }

    return retv;
}


void LIB_SYMBOL::ClearCaches()
{
    for( LIB_ITEM& item : m_drawings )
    {
        if( EDA_TEXT* eda_text = dynamic_cast<EDA_TEXT*>( &item ) )
        {
            eda_text->ClearBoundingBoxCache();
            eda_text->ClearRenderCache();
        }
    }
}


const wxString LIB_SYMBOL::GetLibraryName() const
{
    if( m_library )
        return m_library->GetName();

    return m_libId.GetLibNickname();
}


bool LIB_SYMBOL::IsPower() const
{
    std::shared_ptr<LIB_SYMBOL> parent;

    if( !m_parent.expired() && ( parent = m_parent.lock() ) )
    {
        if( parent->IsRoot() )
            return parent->m_options == ENTRY_POWER;
        else
            return parent->IsPower();
    }

    return m_options == ENTRY_POWER;
}


void LIB_SYMBOL::SetPower()
{
    if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
    {
        if( parent->IsRoot() )
            parent->m_options = ENTRY_POWER;
        else
            parent->SetPower();
    }

    m_options = ENTRY_POWER;
}


bool LIB_SYMBOL::IsNormal() const
{
    if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
    {
        if( parent->IsRoot() )
            return parent->m_options == ENTRY_NORMAL;
        else
            return parent->IsNormal();
    }

    return m_options == ENTRY_NORMAL;
}


void LIB_SYMBOL::SetNormal()
{
    if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
    {
        if( parent->IsRoot() )
            parent->m_options = ENTRY_NORMAL;
        else
            parent->SetNormal();
    }

    m_options = ENTRY_NORMAL;
}


wxString LIB_SYMBOL::LetterSubReference( int aUnit, int aFirstId )
{
    // use letters as notation. To allow more than 26 units, the sub ref
    // use one letter if letter = A .. Z or a ... z, and 2 letters otherwise
    // first letter is expected to be 'A' or 'a' (i.e. 26 letters are available)
    int      u;
    wxString suffix;

    do
    {
        u = ( aUnit - 1 ) % 26;
        suffix = wxChar( aFirstId + u ) + suffix;
        aUnit = ( aUnit - u ) / 26;
    } while( aUnit > 0 );

    return suffix;
}


void LIB_SYMBOL::PrintBackground( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset,
                                  int aUnit, int aBodyStyle, const LIB_SYMBOL_OPTIONS& aOpts,
                                  bool aDimmed )
{
    /* draw background for filled items using background option
     * Solid lines will be drawn after the background
     * Note also, background is not drawn when printing in black and white
     */
    if( !GetGRForceBlackPenState() )
    {
        for( LIB_ITEM& item : m_drawings )
        {
            // Do not print private items
            if( item.IsPrivate() )
                continue;

            if( item.Type() == LIB_SHAPE_T )
            {
                LIB_SHAPE& shape = static_cast<LIB_SHAPE&>( item );

                // Do not draw items not attached to the current part
                if( aUnit && shape.m_unit && ( shape.m_unit != aUnit ) )
                    continue;

                if( aBodyStyle && shape.m_bodyStyle && ( shape.m_bodyStyle != aBodyStyle ) )
                    continue;

                if( shape.GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
                    shape.Print( aSettings, aOffset, (void*) false, aOpts.transform, aDimmed );
            }
        }
    }
}


void LIB_SYMBOL::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, int aUnit,
                        int aBodyStyle, const LIB_SYMBOL_OPTIONS& aOpts, bool aDimmed )
{

    for( LIB_ITEM& item : m_drawings )
    {
        // Do not print private items
        if( item.IsPrivate() )
            continue;

        // Do not draw items not attached to the current part
        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aBodyStyle && item.m_bodyStyle && ( item.m_bodyStyle != aBodyStyle ) )
            continue;

        if( item.Type() == LIB_FIELD_T )
        {
            LIB_FIELD& field = static_cast<LIB_FIELD&>( item );

            if( field.IsVisible() && !aOpts.draw_visible_fields )
                continue;

            if( !field.IsVisible() && !aOpts.draw_hidden_fields )
                continue;
        }

        if( item.Type() == LIB_PIN_T )
        {
            item.Print( aSettings, aOffset, (void*) &aOpts, aOpts.transform, aDimmed );
        }
        else if( item.Type() == LIB_FIELD_T )
        {
            item.Print( aSettings, aOffset, nullptr, aOpts.transform, aDimmed );
        }
        else if( item.Type() == LIB_SHAPE_T )
        {
            LIB_SHAPE& shape = static_cast<LIB_SHAPE&>( item );
            bool       forceNoFill = shape.GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR;

            shape.Print( aSettings, aOffset, (void*) forceNoFill, aOpts.transform, aDimmed );
        }
        else
        {
            item.Print( aSettings, aOffset, (void*) false, aOpts.transform, aDimmed );
        }
    }
}


void LIB_SYMBOL::Plot( PLOTTER *aPlotter, int aUnit, int aBodyStyle, bool aBackground,
                       const VECTOR2I &aOffset, const TRANSFORM &aTransform, bool aDimmed ) const
{
    wxASSERT( aPlotter != nullptr );

    COLOR4D color = aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE );
    COLOR4D bg = aPlotter->RenderSettings()->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );

    for( const LIB_ITEM& item : m_drawings )
    {
        // Do not plot private items
        if( item.IsPrivate() )
            continue;

        // LIB_FIELDs are not plotted here, because this plot function is used to plot schematic
        // items which have their own SCH_FIELDs
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aBodyStyle && item.m_bodyStyle && ( item.m_bodyStyle != aBodyStyle ) )
            continue;

        item.Plot( aPlotter, aBackground, aOffset, aTransform, aDimmed );
    }
}


void LIB_SYMBOL::PlotLibFields( PLOTTER* aPlotter, int aUnit, int aBodyStyle, bool aBackground,
                                const VECTOR2I& aOffset, const TRANSFORM& aTransform, bool aDimmed,
                                bool aPlotHidden )
{
    wxASSERT( aPlotter != nullptr );

    COLOR4D color = aPlotter->RenderSettings()->GetLayerColor( LAYER_FIELDS );
    COLOR4D bg = aPlotter->RenderSettings()->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate( );
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() != LIB_FIELD_T )
            continue;

        if( !aPlotHidden && !( (LIB_FIELD&) item ).IsVisible() )
            continue;

        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aBodyStyle && item.m_bodyStyle && ( item.m_bodyStyle != aBodyStyle ) )
            continue;

        LIB_FIELD& field = (LIB_FIELD&) item;

        // The reference is a special case: we should change the basic text
        // to add '?' and the part id
        wxString tmp = field.GetShownText( true );

        if( field.GetId() == REFERENCE_FIELD )
        {
            wxString text = field.GetFullText( aUnit );
            field.SetText( text );
        }

        item.Plot( aPlotter, aBackground, aOffset, aTransform, aDimmed );
        field.SetText( tmp );
    }
}


void LIB_SYMBOL::FixupDrawItems()
{
    std::vector<LIB_SHAPE*> potential_top_items;
    std::vector<LIB_ITEM*> bottom_items;

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_SHAPE_T )
        {
            LIB_SHAPE& shape = static_cast<LIB_SHAPE&>( item );

            if( shape.GetFillMode() == FILL_T::FILLED_WITH_COLOR )
                potential_top_items.push_back( &shape );
            else
                bottom_items.push_back( &item );
        }
        else
        {
            bottom_items.push_back( &item );
        }
    }

    std::sort( potential_top_items.begin(), potential_top_items.end(),
               []( LIB_ITEM* a, LIB_ITEM* b )
               {
                   return a->GetBoundingBox().GetArea() > b->GetBoundingBox().GetArea();
               } );


    for( LIB_SHAPE* item : potential_top_items )
    {
        for( LIB_ITEM* bottom_item : bottom_items )
        {
            if( item->GetBoundingBox().Contains( bottom_item->GetBoundingBox() ) )
            {
                item->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
                break;
            }
        }
    }
}


void LIB_SYMBOL::RemoveDrawItem( LIB_ITEM* aItem )
{
    wxASSERT( aItem != nullptr );

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
        if( &*i == aItem )
        {
            items.erase( i );
            break;
        }
    }
}


void LIB_SYMBOL::AddDrawItem( LIB_ITEM* aItem, bool aSort )
{
    wxCHECK( aItem, /* void */ );

    m_drawings.push_back( aItem );

    if( aSort )
        m_drawings.sort();
}


void LIB_SYMBOL::GetPins( LIB_PINS& aList, int aUnit, int aBodyStyle ) const
{
    /* Notes:
     * when aUnit == 0: no unit filtering
     * when aBodyStyle == 0: no body style filtering
     * when m_unit == 0, the item is common to all units
     * when m_bodyStyle == 0, the item is common to all body styles
     */

    LIB_SYMBOL_SPTR            parent = m_parent.lock();
    const LIB_ITEMS_CONTAINER& drawItems = parent ? parent->m_drawings : m_drawings;

    for( const LIB_ITEM& item : drawItems[LIB_PIN_T] )
    {
        // Unit filtering:
        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        // De Morgan variant filtering:
        if( aBodyStyle && item.m_bodyStyle && ( item.m_bodyStyle != aBodyStyle ) )
            continue;

        aList.push_back( (LIB_PIN*) &item );
    }
}


std::vector<LIB_PIN*> LIB_SYMBOL::GetAllLibPins() const
{
    std::vector<LIB_PIN*> pinList;

    GetPins( pinList, 0, 0 );
    return pinList;
}


int LIB_SYMBOL::GetPinCount()
{
    std::vector<LIB_PIN*> pinList;

    GetPins( pinList, 0, 1 );   // All units, but a single convert
    return pinList.size();
}


LIB_PIN* LIB_SYMBOL::GetPin( const wxString& aNumber, int aUnit, int aBodyStyle ) const
{
    LIB_PINS pinList;

    GetPins( pinList, aUnit, aBodyStyle );

    for( LIB_PIN* pin : pinList )
    {
        wxASSERT( pin->Type() == LIB_PIN_T );

        if( aNumber == pin->GetNumber() )
            return pin;
    }

    return nullptr;
}


bool LIB_SYMBOL::PinsConflictWith( const LIB_SYMBOL& aOtherPart, bool aTestNums, bool aTestNames,
                                   bool aTestType, bool aTestOrientation, bool aTestLength ) const
{
    LIB_PINS thisPinList;
    GetPins( thisPinList, /* aUnit */ 0, /* aBodyStyle */ 0 );

    for( const LIB_PIN* eachThisPin : thisPinList )
    {
        wxASSERT( eachThisPin );
        LIB_PINS otherPinList;
        aOtherPart.GetPins( otherPinList, /* aUnit */ 0, /* aBodyStyle */ 0 );
        bool foundMatch = false;

        for( const LIB_PIN* eachOtherPin : otherPinList )
        {
            wxASSERT( eachOtherPin );

            // Same unit?
            if( eachThisPin->GetUnit() != eachOtherPin->GetUnit() )
                continue;

            // Same body stype?
            if( eachThisPin->GetBodyStyle() != eachOtherPin->GetBodyStyle() )
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
            break;                    // Match found so search is complete.
        }

        if( !foundMatch )
        {
            // This means there was not an identical (according to the arguments)
            // pin at the same position in the other symbol.
            return true;
        }
    }

    // The loop never gave up, so no conflicts were found.
    return false;
}


const BOX2I LIB_SYMBOL::GetUnitBoundingBox( int aUnit, int aBodyStyle,
                                            bool aIgnoreHiddenFields ) const
{
    BOX2I bBox;     // Start with a fresh BOX2I so the Merge algorithm works

    for( const LIB_ITEM& item : m_drawings )
    {
        if( item.m_unit > 0
                && m_unitCount > 1
                && aUnit > 0
                && aUnit != item.m_unit )
        {
            continue;
        }

        if( item.m_bodyStyle > 0 && aBodyStyle > 0 && aBodyStyle != item.m_bodyStyle )
            continue;

        if( aIgnoreHiddenFields && ( item.Type() == LIB_FIELD_T )
            && !( (LIB_FIELD&) item ).IsVisible() )
            continue;

        bBox.Merge( item.GetBoundingBox() );
    }

    return bBox;
}


void LIB_SYMBOL::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 0;
    aLayers[ aCount++ ] = LAYER_DEVICE;
    aLayers[ aCount++ ] = LAYER_DEVICE_BACKGROUND;
    aLayers[ aCount++ ] = LAYER_REFERENCEPART;
    aLayers[ aCount++ ] = LAYER_VALUEPART;
    aLayers[ aCount++ ] = LAYER_FIELDS;
    aLayers[ aCount++ ] = LAYER_PRIVATE_NOTES;
    aLayers[ aCount++ ] = LAYER_NOTES_BACKGROUND;
    aLayers[ aCount++ ] = LAYER_SELECTION_SHADOWS;
}


const BOX2I LIB_SYMBOL::GetBodyBoundingBox( int aUnit, int aBodyStyle, bool aIncludePins,
                                            bool aIncludePrivateItems ) const
{
    BOX2I bbox;

    for( const LIB_ITEM& item : m_drawings )
    {
        if( item.m_unit > 0 && aUnit > 0 && aUnit != item.m_unit )
            continue;

        if( item.m_bodyStyle > 0 && aBodyStyle > 0 && aBodyStyle != item.m_bodyStyle )
            continue;

        if( item.IsPrivate() && !aIncludePrivateItems )
            continue;

        if( item.Type() == LIB_FIELD_T )
            continue;

        if( item.Type() == LIB_PIN_T )
        {
            const LIB_PIN& pin = static_cast<const LIB_PIN&>( item );

            if( pin.IsVisible() )
            {
                // Note: the roots of the pins are always included for symbols that don't have
                // a well-defined body.

                if( aIncludePins )
                    bbox.Merge( pin.GetBoundingBox( false, false, false ) );
                else
                    bbox.Merge( pin.GetPinRoot() );
            }
        }
        else
        {
            bbox.Merge( item.GetBoundingBox() );
        }
    }

    return bbox;
}


void LIB_SYMBOL::deleteAllFields()
{
    m_drawings[ LIB_FIELD_T ].clear();
}


void LIB_SYMBOL::AddField( LIB_FIELD* aField )
{
    AddDrawItem( aField );
}


void LIB_SYMBOL::SetFields( const std::vector<LIB_FIELD>& aFields )
{
    deleteAllFields();

    for( size_t ii = 0; ii < aFields.size(); ++ii )
    {
        // drawings is a ptr_vector, new and copy an object on the heap.
        LIB_FIELD* field = new LIB_FIELD( aFields[ ii ] );

        field->SetParent( this );
        m_drawings.push_back( field );
    }

    m_drawings.sort();
}


void LIB_SYMBOL::GetFields( std::vector<LIB_FIELD*>& aList )
{
    // Grab the MANDATORY_FIELDS first, in expected order given by enum MANDATORY_FIELD_T
    for( int id = 0; id < MANDATORY_FIELDS; ++id )
        aList.push_back( GetFieldById( id ) );

    // Now grab all the rest of fields.
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = static_cast<LIB_FIELD*>( &item );

        if( !field->IsMandatory() )
            aList.push_back( field );
    }
}


void LIB_SYMBOL::GetFields( std::vector<LIB_FIELD>& aList )
{
    // Grab the MANDATORY_FIELDS first, in expected order given by enum MANDATORY_FIELD_T
    for( int id = 0; id < MANDATORY_FIELDS; ++id )
        aList.push_back( *GetFieldById( id ) );

    // Now grab all the rest of fields.
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = static_cast<LIB_FIELD*>( &item );

        if( !field->IsMandatory() )
            aList.push_back( *field );
    }
}


LIB_FIELD* LIB_SYMBOL::GetFieldById( int aId ) const
{
    for( const LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = ( LIB_FIELD* ) &item;

        if( field->GetId() == aId )
            return field;
    }

    return nullptr;
}


LIB_FIELD* LIB_SYMBOL::FindField( const wxString& aFieldName, bool aCaseInsensitive )
{
    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        if( aCaseInsensitive )
        {
            if( static_cast<LIB_FIELD*>( &item )->GetCanonicalName().Upper() == aFieldName.Upper() )
                return static_cast<LIB_FIELD*>( &item );
        }
        else
        {
            if( static_cast<LIB_FIELD*>( &item )->GetCanonicalName() == aFieldName )
                return static_cast<LIB_FIELD*>( &item );
        }
    }

    return nullptr;
}


const LIB_FIELD* LIB_SYMBOL::FindField( const wxString& aFieldName,
                                        const bool      aCaseInsensitive ) const
{
    for( const LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        if( aCaseInsensitive )
        {
            if( static_cast<const LIB_FIELD*>( &item )->GetCanonicalName().Upper()
                == aFieldName.Upper() )
                return static_cast<const LIB_FIELD*>( &item );
        }
        else
        {
            if( static_cast<const LIB_FIELD*>( &item )->GetCanonicalName() == aFieldName )
                return static_cast<const LIB_FIELD*>( &item );
        }
    }

    return nullptr;
}


LIB_FIELD& LIB_SYMBOL::GetValueField()
{
    LIB_FIELD* field = GetFieldById( VALUE_FIELD );
    wxASSERT( field != nullptr );
    return *field;
}


LIB_FIELD& LIB_SYMBOL::GetReferenceField()
{
    LIB_FIELD* field = GetFieldById( REFERENCE_FIELD );
    wxASSERT( field != nullptr );
    return *field;
}


LIB_FIELD& LIB_SYMBOL::GetFootprintField()
{
    LIB_FIELD* field = GetFieldById( FOOTPRINT_FIELD );
    wxASSERT( field != nullptr );
    return *field;
}


LIB_FIELD& LIB_SYMBOL::GetDatasheetField()
{
    LIB_FIELD* field = GetFieldById( DATASHEET_FIELD );
    wxASSERT( field != nullptr );
    return *field;
}


LIB_FIELD& LIB_SYMBOL::GetDescriptionField()
{
    LIB_FIELD* field = GetFieldById( DESCRIPTION_FIELD );
    wxASSERT( field != nullptr );
    return *field;
}


wxString LIB_SYMBOL::GetPrefix()
{
    wxString refDesignator = GetFieldById( REFERENCE_FIELD )->GetText();

    refDesignator.Replace( wxS( "~" ), wxS( " " ) );

    wxString prefix = refDesignator;

    while( prefix.Length() )
    {
        wxUniCharRef last = prefix.Last();

        if( ( last >= '0' && last <= '9' ) || last == '?' || last == '*' )
            prefix.RemoveLast();
        else
            break;
    }

    // Avoid a prefix containing trailing/leading spaces
    prefix.Trim( true );
    prefix.Trim( false );

    return prefix;
}


void LIB_SYMBOL::RunOnChildren( const std::function<void( LIB_ITEM* )>& aFunction )
{
    for( LIB_ITEM& item : m_drawings )
        aFunction( &item );
}


int LIB_SYMBOL::UpdateFieldOrdinals()
{
    int retv = 0;
    int lastOrdinal = MANDATORY_FIELDS;

    for( LIB_ITEM& item : m_drawings[ LIB_FIELD_T ] )
    {
        LIB_FIELD* field = dynamic_cast<LIB_FIELD*>( &item );

        wxCHECK2( field, continue );

        // Mandatory fields were already resolved always have the same ordinal values.
        if( field->IsMandatory() )
            continue;

        if( field->GetId() != lastOrdinal )
        {
            field->SetId( lastOrdinal );
            retv += 1;
        }

        lastOrdinal += 1;
    }

    return retv;
}


int LIB_SYMBOL::GetNextAvailableFieldId() const
{
    int retv = MANDATORY_FIELDS;

    while( GetFieldById( retv ) )
        retv += 1;

    return retv;
}


void LIB_SYMBOL::SetOffset( const VECTOR2I& aOffset )
{
    for( LIB_ITEM& item : m_drawings )
        item.Offset( aOffset );
}


void LIB_SYMBOL::RemoveDuplicateDrawItems()
{
    m_drawings.unique();
}


bool LIB_SYMBOL::HasAlternateBodyStyle() const
{
    for( const LIB_ITEM& item : m_drawings )
    {
        if( item.m_bodyStyle > LIB_ITEM::BODY_STYLE::BASE )
            return true;
    }

    if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
    {
        for( const LIB_ITEM& item : parent->GetDrawItems() )
        {
            if( item.m_bodyStyle > LIB_ITEM::BODY_STYLE::BASE )
                return true;
        }
    }

    return false;
}


int LIB_SYMBOL::GetMaxPinNumber() const
{
    int                        maxPinNumber = 0;
    LIB_SYMBOL_SPTR            parent = m_parent.lock();
    const LIB_ITEMS_CONTAINER& drawItems = parent ? parent->m_drawings : m_drawings;

    for( const LIB_ITEM& item : drawItems[LIB_PIN_T] )
    {
        const LIB_PIN* pin = static_cast<const LIB_PIN*>( &item );
        long           currentPinNumber = 0;

        if( pin->GetNumber().ToLong( &currentPinNumber ) )
            maxPinNumber = std::max( maxPinNumber, (int) currentPinNumber );
    }

    return maxPinNumber;
}


void LIB_SYMBOL::ClearTempFlags()
{
    for( LIB_ITEM& item : m_drawings )
        item.ClearTempFlags();
}


void LIB_SYMBOL::ClearEditFlags()
{
    for( LIB_ITEM& item : m_drawings )
        item.ClearEditFlags();
}


LIB_ITEM* LIB_SYMBOL::LocateDrawItem( int aUnit, int aBodyStyle, KICAD_T aType,
                                      const VECTOR2I& aPoint )
{
    for( LIB_ITEM& item : m_drawings )
    {
        if( ( aUnit && item.m_unit && aUnit != item.m_unit )
                || ( aBodyStyle && item.m_bodyStyle && aBodyStyle != item.m_bodyStyle )
                || ( item.Type() != aType && aType != TYPE_NOT_INIT ) )
        {
            continue;
        }

        if( item.HitTest( aPoint ) )
            return &item;
    }

    return nullptr;
}


LIB_ITEM* LIB_SYMBOL::LocateDrawItem( int aUnit, int aBodyStyle, KICAD_T aType,
                                      const VECTOR2I& aPoint, const TRANSFORM& aTransform )
{
    /* we use LocateDrawItem( int aUnit, int convert, KICAD_T type, const
     * VECTOR2I& pt ) to search items.
     * because this function uses DefaultTransform as orient/mirror matrix
     * we temporary copy aTransform in DefaultTransform
     */
    LIB_ITEM* item;
    TRANSFORM transform = DefaultTransform;
    DefaultTransform = aTransform;

    item = LocateDrawItem( aUnit, aBodyStyle, aType, aPoint );

    // Restore matrix
    DefaultTransform = transform;

    return item;
}


INSPECT_RESULT LIB_SYMBOL::Visit( INSPECTOR aInspector, void* aTestData,
                                  const std::vector<KICAD_T>& aScanTypes )
{
    // The part itself is never inspected, only its children
    for( LIB_ITEM& item : m_drawings )
    {
        if( item.IsType( aScanTypes ) )
        {
            if( aInspector( &item, aTestData ) == INSPECT_RESULT::QUIT )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


void LIB_SYMBOL::SetUnitCount( int aCount, bool aDuplicateDrawItems )
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
                LIB_ITEM* newItem = (LIB_ITEM*) item.Duplicate();
                newItem->m_unit = j;
                tmp.push_back( newItem );
            }
        }

        for( LIB_ITEM* item : tmp )
            m_drawings.push_back( item );
    }

    m_drawings.sort();
    m_unitCount = aCount;
}


int LIB_SYMBOL::GetUnitCount() const
{
    if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
        return parent->GetUnitCount();

    return m_unitCount;
}


void LIB_SYMBOL::SetHasAlternateBodyStyle( bool aHasAlternate, bool aDuplicatePins )
{
    if( aHasAlternate == HasAlternateBodyStyle() )
        return;

    // Duplicate items to create the converted shape
    if( aHasAlternate )
    {
        if( aDuplicatePins )
        {
            std::vector< LIB_ITEM* > tmp;     // Temporarily store the duplicated pins here.

            for( LIB_ITEM& item : m_drawings )
            {
                // Only pins are duplicated.
                if( item.Type() != LIB_PIN_T )
                    continue;

                if( item.m_bodyStyle == 1 )
                {
                    LIB_ITEM* newItem = (LIB_ITEM*) item.Duplicate();
                    newItem->m_bodyStyle = 2;
                    tmp.push_back( newItem );
                }
            }

            // Transfer the new pins to the LIB_SYMBOL.
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
            if( i->m_bodyStyle > 1 )
                i = m_drawings.erase( i );
            else
                ++i;
        }
    }

    m_drawings.sort();
}


std::vector<LIB_ITEM*> LIB_SYMBOL::GetUnitDrawItems( int aUnit, int aBodyStyle )
{
    std::vector<LIB_ITEM*> unitItems;

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( ( aBodyStyle == -1 && item.GetUnit() == aUnit )
                || ( aUnit == -1 && item.GetBodyStyle() == aBodyStyle )
                || ( aUnit == item.GetUnit() && aBodyStyle == item.GetBodyStyle() ) )
        {
            unitItems.push_back( &item );
        }
    }

    return unitItems;
}


std::vector<struct LIB_SYMBOL_UNIT> LIB_SYMBOL::GetUnitDrawItems()
{
    std::vector<struct LIB_SYMBOL_UNIT> units;

    for( LIB_ITEM& item : m_drawings )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        int unit = item.GetUnit();
        int bodyStyle = item.GetBodyStyle();

        auto it = std::find_if( units.begin(), units.end(),
                [unit, bodyStyle]( const LIB_SYMBOL_UNIT& a )
                {
                    return a.m_unit == unit && a.m_bodyStyle == bodyStyle;
                } );

        if( it == units.end() )
        {
            struct LIB_SYMBOL_UNIT newUnit;
            newUnit.m_unit = item.GetUnit();
            newUnit.m_bodyStyle = item.GetBodyStyle();
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


std::vector<struct LIB_SYMBOL_UNIT> LIB_SYMBOL::GetUniqueUnits()
{
    int unitNum;
    size_t i;
    struct LIB_SYMBOL_UNIT unit;
    std::vector<LIB_ITEM*> compareDrawItems;
    std::vector<LIB_ITEM*> currentDrawItems;
    std::vector<struct LIB_SYMBOL_UNIT> uniqueUnits;

    // The first unit is guaranteed to be unique so always include it.
    unit.m_unit = 1;
    unit.m_bodyStyle = 1;
    unit.m_items = GetUnitDrawItems( 1, 1 );

    // There are no unique units if there are no draw items other than fields.
    if( unit.m_items.size() == 0 )
        return uniqueUnits;

    uniqueUnits.emplace_back( unit );

    if( ( GetUnitCount() == 1 || UnitsLocked() ) && !HasAlternateBodyStyle() )
        return uniqueUnits;

    currentDrawItems = unit.m_items;

    for( unitNum = 2; unitNum <= GetUnitCount(); unitNum++ )
    {
        compareDrawItems = GetUnitDrawItems( unitNum, 1 );

        wxCHECK2_MSG( compareDrawItems.size() != 0, continue,
                      "Multiple unit symbol defined with empty units." );

        if( currentDrawItems.size() != compareDrawItems.size() )
        {
            unit.m_unit = unitNum;
            unit.m_bodyStyle = 1;
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
                    unit.m_bodyStyle = 1;
                    unit.m_items = compareDrawItems;
                    uniqueUnits.emplace_back( unit );
                }
            }
        }
    }

    if( HasAlternateBodyStyle() )
    {
        currentDrawItems = GetUnitDrawItems( 1, 2 );

        if( ( GetUnitCount() == 1 || UnitsLocked() ) )
        {
            unit.m_unit = 1;
            unit.m_bodyStyle = 2;
            unit.m_items = currentDrawItems;
            uniqueUnits.emplace_back( unit );

            return uniqueUnits;
        }

        for( unitNum = 2; unitNum <= GetUnitCount(); unitNum++ )
        {
            compareDrawItems = GetUnitDrawItems( unitNum, 2 );

            wxCHECK2_MSG( compareDrawItems.size() != 0, continue,
                          "Multiple unit symbol defined with empty units." );

            if( currentDrawItems.size() != compareDrawItems.size() )
            {
                unit.m_unit = unitNum;
                unit.m_bodyStyle = 2;
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
                        unit.m_bodyStyle = 2;
                        unit.m_items = compareDrawItems;
                        uniqueUnits.emplace_back( unit );
                    }
                }
            }
        }
    }

    return uniqueUnits;
}


bool LIB_SYMBOL::operator==( const LIB_SYMBOL& aOther ) const
{
    if( m_libId != aOther.m_libId )
        return false;

    if( m_excludedFromBoard != aOther.m_excludedFromBoard )
        return false;

    if( m_excludedFromBOM != aOther.m_excludedFromBOM )
        return false;

    if( m_excludedFromSim != aOther.m_excludedFromSim )
        return false;

    if( m_flags != aOther.m_flags )
        return false;

    if( m_unitCount != aOther.m_unitCount )
        return false;

    if( m_pinNameOffset != aOther.m_pinNameOffset )
        return false;

    if( m_showPinNames != aOther.m_showPinNames )
        return false;

    if( m_showPinNumbers != aOther.m_showPinNumbers )
        return false;

    if( m_drawings.size() != aOther.m_drawings.size() )
        return false;

    for( auto it1 = m_drawings.begin(), it2 = aOther.m_drawings.begin();
         it1 != m_drawings.end(); ++it1, ++it2 )
    {
        if( !( *it1 == *it2 ) )
            return false;
    }

    const LIB_PINS thisPinList = GetAllLibPins();
    const LIB_PINS otherPinList = aOther.GetAllLibPins();

    if( thisPinList.size() != otherPinList.size() )
        return false;

    for( auto it1 = thisPinList.begin(), it2 = otherPinList.begin();
         it1 != thisPinList.end(); ++it1, ++it2 )
    {
        if( !( **it1 == **it2 ) )
            return false;
    }
    for( size_t ii = 0; ii < thisPinList.size(); ++ii )
    {
        if( !( *thisPinList[ii] == *otherPinList[ii] ) )
            return false;
    }

    return true;
}


double LIB_SYMBOL::Similarity( const LIB_SYMBOL& aOther ) const
{
    double similarity = 0.0;
    int    totalItems = 0;

    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    for( const LIB_ITEM& item : m_drawings )
    {
        totalItems += 1;
        double max_similarity = 0.0;

        for( const LIB_ITEM& otherItem : aOther.m_drawings )
        {
            double temp_similarity = item.Similarity( otherItem );
            max_similarity = std::max( max_similarity, temp_similarity );

            if( max_similarity == 1.0 )
                break;
        }

        similarity += max_similarity;
    }

    for( const LIB_PIN* pin : GetAllLibPins() )
    {
        totalItems += 1;
        double max_similarity = 0.0;

        for( const LIB_PIN* otherPin : aOther.GetAllLibPins() )
        {
            double temp_similarity = pin->Similarity( *otherPin );
            max_similarity = std::max( max_similarity, temp_similarity );

            if( max_similarity == 1.0 )
                break;
        }

        similarity += max_similarity;
    }

    if( totalItems == 0 )
        similarity = 0.0;
    else
        similarity /= totalItems;

    if( m_excludedFromBoard != aOther.m_excludedFromBoard )
        similarity *= 0.9;

    if( m_excludedFromBOM != aOther.m_excludedFromBOM )
        similarity *= 0.9;

    if( m_excludedFromSim != aOther.m_excludedFromSim )
        similarity *= 0.9;

    if( m_flags != aOther.m_flags )
        similarity *= 0.9;

    if( m_unitCount != aOther.m_unitCount )
        similarity *= 0.5;

    if( m_pinNameOffset != aOther.m_pinNameOffset )
        similarity *= 0.9;

    if( m_showPinNames != aOther.m_showPinNames )
        similarity *= 0.9;

    if( m_showPinNumbers != aOther.m_showPinNumbers )
        similarity *= 0.9;

    return similarity;
}
