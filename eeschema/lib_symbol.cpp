/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <font/outline_font.h>
#include <sch_draw_panel.h>
#include <plotters/plotter.h>
#include <sch_screen.h>
#include <template_fieldnames.h>
#include <transform.h>
#include <settings/color_settings.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <trace_helpers.h>
#include <common.h>
#include <text_eval/text_eval_wrapper.h>

// TODO(JE) remove m_library; shouldn't be needed with legacy remapping
#include <libraries/legacy_symbol_library.h>

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <advanced_config.h>


/**
 * Helper to safely get the root symbol, detecting and logging circular inheritance.
 *
 * @param aSymbol The symbol to start from
 * @param aCallerName Name of the calling function for logging
 * @return The root symbol, or the input symbol if a cycle is detected or no parent exists
 */
static std::shared_ptr<LIB_SYMBOL> GetSafeRootSymbol( const LIB_SYMBOL* aSymbol,
                                                       const char* aCallerName )
{
    std::set<const LIB_SYMBOL*> visited;
    visited.insert( aSymbol );

    std::shared_ptr<LIB_SYMBOL> current = aSymbol->SharedPtr();
    std::shared_ptr<LIB_SYMBOL> parent = aSymbol->GetParent().lock();

    while( parent )
    {
        if( visited.count( parent.get() ) )
        {
            // Build chain description for logging
            wxString chain = aSymbol->GetName();

            for( const LIB_SYMBOL* sym : visited )
            {
                if( sym != aSymbol )
                    chain += wxT( " -> " ) + sym->GetName();
            }

            chain += wxT( " -> " ) + parent->GetName() + wxT( " (CYCLE)" );

            wxLogTrace( traceSymbolInheritance,
                        wxT( "%s: Circular inheritance detected in symbol '%s' (lib: %s). "
                             "Chain: %s" ),
                        aCallerName, aSymbol->GetName(),
                        aSymbol->GetLibId().GetLibNickname().wx_str(), chain );

            // Return current (last valid symbol before cycle)
            return current;
        }

        visited.insert( parent.get() );
        current = parent;
        parent = parent->GetParent().lock();
    }

    return current;
}


wxString LIB_SYMBOL::GetShownDescription( int aDepth ) const
{
    wxString shownText = GetDescriptionField().GetShownText( false, aDepth );

    if( shownText.IsEmpty() && IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            shownText = root->GetDescriptionField().GetShownText( false, aDepth );
    }

    return shownText;
}


wxString LIB_SYMBOL::GetShownKeyWords( int aDepth ) const
{
    wxString text = GetKeyWords();

    std::function<bool( wxString* )> libSymbolResolver = [&]( wxString* token ) -> bool
    {
        return ResolveTextVar( token, aDepth + 1 );
    };

    text = ResolveTextVars( text, &libSymbolResolver, aDepth );

    return text;
}


std::vector<SEARCH_TERM> LIB_SYMBOL::GetSearchTerms()
{
    std::vector<SEARCH_TERM> terms;

    terms.emplace_back( SEARCH_TERM( GetLibNickname(), 4 ) );
    terms.emplace_back( SEARCH_TERM( GetName(), 8 ) );
    terms.emplace_back( SEARCH_TERM( GetLIB_ID().Format(), 16 ) );

    wxStringTokenizer keywordTokenizer( GetShownKeyWords(), " \t\r\n", wxTOKEN_STRTOK );

    while( keywordTokenizer.HasMoreTokens() )
        terms.emplace_back( SEARCH_TERM( keywordTokenizer.GetNextToken(), 4 ) );

    // Also include keywords as one long string, just in case
    terms.emplace_back( SEARCH_TERM( GetShownKeyWords(), 1 ) );
    terms.emplace_back( SEARCH_TERM( GetShownDescription(), 1 ) );

    wxString footprint = GetFootprint();

    if( !footprint.IsEmpty() )
        terms.emplace_back( SEARCH_TERM( footprint, 1 ) );

    return terms;
}


void LIB_SYMBOL::GetChooserFields( std::map<wxString, wxString>& aColumnMap )
{
    for( SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( &item );

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
    void operator()( void const* ) const {}
};


LIB_SYMBOL::LIB_SYMBOL( const wxString& aName, LIB_SYMBOL* aParent, LEGACY_SYMBOL_LIB* aLibrary ) :
        SYMBOL( LIB_SYMBOL_T ),
        m_me( this, null_deleter() )
{
    m_lastModDate = 0;
    m_unitCount = 1;
    m_demorgan = false;
    m_pinNameOffset = schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET );
    m_options = ENTRY_NORMAL;
    m_unitsLocked = false;
    m_duplicatePinNumbersAreJumpers = false;

    auto addField = [&]( FIELD_T id, bool visible )
    {
        SCH_FIELD* field = new SCH_FIELD( this, id );
        field->SetVisible( visible );
        m_drawings[SCH_FIELD_T].push_back( field );
    };

    // construct only the mandatory fields
    addField( FIELD_T::REFERENCE, true );
    addField( FIELD_T::VALUE, true );
    addField( FIELD_T::FOOTPRINT, false );
    addField( FIELD_T::DATASHEET, false );
    addField( FIELD_T::DESCRIPTION, false );

    SetName( aName );
    SetParent( aParent );
    SetLib( aLibrary );
}


LIB_SYMBOL::LIB_SYMBOL( const LIB_SYMBOL& aSymbol, LEGACY_SYMBOL_LIB* aLibrary, bool aCopyEmbeddedFiles ) :
        SYMBOL( aSymbol ),
        EMBEDDED_FILES( aSymbol, aCopyEmbeddedFiles ),
        m_me( this, null_deleter() )
{
    m_library = aLibrary;
    m_name = aSymbol.m_name;
    m_fpFilters = wxArrayString( aSymbol.m_fpFilters );
    m_unitCount = aSymbol.m_unitCount;
    m_demorgan = aSymbol.m_demorgan;
    m_unitsLocked = aSymbol.m_unitsLocked;
    m_lastModDate = aSymbol.m_lastModDate;
    m_options = aSymbol.m_options;
    m_libId = aSymbol.m_libId;
    m_keyWords = aSymbol.m_keyWords;

    m_jumperPinGroups = aSymbol.m_jumperPinGroups;
    m_duplicatePinNumbersAreJumpers = aSymbol.m_duplicatePinNumbersAreJumpers;

    m_unitDisplayNames = aSymbol.GetUnitDisplayNames();
    m_bodyStyleNames = aSymbol.GetBodyStyleNames();

    ClearSelected();

    for( const SCH_ITEM& oldItem : aSymbol.m_drawings )
    {
        if( ( oldItem.GetFlags() & ( IS_NEW | STRUCT_DELETED ) ) != 0 )
            continue;

        try
        {
            SCH_ITEM* newItem = (SCH_ITEM*) oldItem.Clone();
            newItem->ClearSelected();
            newItem->SetParent( this );
            m_drawings.push_back( newItem );
        }
        catch( ... )
        {
            wxFAIL_MSG( "Failed to clone SCH_ITEM." );
            return;
        }
    }

    SetParent( aSymbol.m_parent.lock().get() );
}


const LIB_SYMBOL& LIB_SYMBOL::operator=( const LIB_SYMBOL& aSymbol )
{
    if( &aSymbol == this )
        return aSymbol;

    SYMBOL::operator=( aSymbol );

    m_library = aSymbol.m_library;
    m_name = aSymbol.m_name;
    m_fpFilters = wxArrayString( aSymbol.m_fpFilters );
    m_unitCount = aSymbol.m_unitCount;
    m_demorgan = aSymbol.m_demorgan;
    m_unitsLocked = aSymbol.m_unitsLocked;
    m_lastModDate = aSymbol.m_lastModDate;
    m_options = aSymbol.m_options;
    m_libId = aSymbol.m_libId;
    m_keyWords = aSymbol.m_keyWords;

    m_jumperPinGroups = aSymbol.m_jumperPinGroups;
    m_duplicatePinNumbersAreJumpers = aSymbol.m_duplicatePinNumbersAreJumpers;

    m_unitDisplayNames = aSymbol.GetUnitDisplayNames();
    m_bodyStyleNames = aSymbol.GetBodyStyleNames();

    m_drawings.clear();

    for( const SCH_ITEM& oldItem : aSymbol.m_drawings )
    {
        if( ( oldItem.GetFlags() & ( IS_NEW | STRUCT_DELETED ) ) != 0 )
            continue;

        SCH_ITEM* newItem = (SCH_ITEM*) oldItem.Clone();
        newItem->SetParent( this );
        m_drawings.push_back( newItem );
    }

    m_drawings.sort();

    SetParent( aSymbol.m_parent.lock().get() );

    EMBEDDED_FILES::operator=( aSymbol );

    return *this;
}


/**
 * Used as a dummy LIB_SYMBOL when one is not found in library or imported schematic
 *
 * This symbol is a 400 mils square with the text "??"
 */
LIB_SYMBOL* LIB_SYMBOL::GetDummy()
{
    static LIB_SYMBOL* symbol;

    if( !symbol )
    {
        symbol = new LIB_SYMBOL( wxEmptyString );

        SCH_SHAPE* square = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

        square->SetPosition( VECTOR2I( schIUScale.MilsToIU( -200 ), schIUScale.MilsToIU( 200 ) ) );
        square->SetEnd( VECTOR2I( schIUScale.MilsToIU( 200 ), schIUScale.MilsToIU( -200 ) ) );
        symbol->AddDrawItem( square );

        SCH_TEXT* text = new SCH_TEXT( { 0, 0 }, wxT( "??" ), LAYER_DEVICE );

        text->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 150 ), schIUScale.MilsToIU( 150 ) ) );
        symbol->AddDrawItem( text );
    }

    return symbol;
}


unsigned LIB_SYMBOL::GetInheritanceDepth() const
{
    unsigned depth = 0;
    std::set<const LIB_SYMBOL*> visited;
    visited.insert( this );

    std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock();

    while( parent )
    {
        // Cycle detected - parent chain points back to an already visited symbol
        if( visited.count( parent.get() ) )
        {
            wxLogTrace( traceSymbolInheritance,
                        wxT( "GetInheritanceDepth: Circular inheritance detected in symbol '%s' "
                             "(lib: %s)" ),
                        m_name, m_libId.GetLibNickname().wx_str() );
            break;
        }

        visited.insert( parent.get() );
        depth++;
        parent = parent->m_parent.lock();
    }

    return depth;
}


std::shared_ptr<LIB_SYMBOL> LIB_SYMBOL::GetRootSymbol() const
{
    std::set<const LIB_SYMBOL*> visited;
    visited.insert( this );

    std::shared_ptr<LIB_SYMBOL> current = m_me;
    std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock();

    while( parent )
    {
        // Cycle detected - parent chain points back to an already visited symbol
        if( visited.count( parent.get() ) )
        {
            wxLogTrace( traceSymbolInheritance,
                        wxT( "GetRootSymbol: Circular inheritance detected in symbol '%s' "
                             "(lib: %s)" ),
                        m_name, m_libId.GetLibNickname().wx_str() );
            break;
        }

        visited.insert( parent.get() );
        current = parent;
        parent = parent->m_parent.lock();
    }

    return current;
}


wxString LIB_SYMBOL::GetUnitDisplayName( int aUnit, bool aLabel ) const
{
    if( m_unitDisplayNames.contains( aUnit ) )
        return m_unitDisplayNames.at( aUnit );
    else if( aLabel )
        return wxString::Format( _( "Unit %s" ), LIB_SYMBOL::LetterSubReference( aUnit, 'A' ) );
    else
        return LIB_SYMBOL::LetterSubReference( aUnit, 'A' );
}


wxString LIB_SYMBOL::GetBodyStyleDescription( int aBodyStyle, bool aLabel ) const
{
    if( HasDeMorganBodyStyles() )
    {
        if( aBodyStyle == BODY_STYLE::DEMORGAN )
            return aLabel ? _( "Alternate" ) : wxString( _HKI( "Alternate" ) );
        else if( aBodyStyle == BODY_STYLE::BASE )
            return aLabel ? _( "Standard" ) : wxString( _HKI( "Standard" ) );
    }
    else if( IsMultiBodyStyle() )
    {
        if( aBodyStyle <= (int) m_bodyStyleNames.size() )
            return m_bodyStyleNames[aBodyStyle - 1];
    }

    return wxT( "?" );
}


void LIB_SYMBOL::SetName( const wxString& aName )
{
    m_name = aName;
    m_libId.SetLibItemName( aName );
}


void LIB_SYMBOL::SetParent( LIB_SYMBOL* aParent )
{
    if( aParent )
    {
        // Prevent circular inheritance by checking if aParent is derived from this symbol
        std::set<const LIB_SYMBOL*> visited;
        visited.insert( this );

        std::shared_ptr<LIB_SYMBOL> ancestor = aParent->SharedPtr();

        while( ancestor )
        {
            if( visited.count( ancestor.get() ) )
            {
                wxLogTrace( traceSymbolInheritance,
                            wxT( "SetParent: Rejecting parent '%s' for symbol '%s' - would create "
                                 "circular inheritance (lib: %s)" ),
                            aParent->GetName(), m_name, m_libId.GetLibNickname().wx_str() );

                // Don't set the parent - it would create circular inheritance
                return;
            }

            visited.insert( ancestor.get() );
            ancestor = ancestor->m_parent.lock();
        }

        m_parent = aParent->SharedPtr();
    }
    else
    {
        m_parent.reset();
    }
}


std::unique_ptr<LIB_SYMBOL> LIB_SYMBOL::Flatten() const
{
    std::unique_ptr<LIB_SYMBOL> retv;

    if( IsDerived() )
    {
        // Build parent chain with cycle detection - collect from this symbol up to root
        std::vector<const LIB_SYMBOL*> parentChain;
        std::set<const LIB_SYMBOL*> visited;
        visited.insert( this );

        std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock();

        wxCHECK_MSG( parent, retv, wxString::Format( "Parent of derived symbol '%s' undefined", m_name ) );

        while( parent )
        {
            // Cycle detected - parent chain points back to an already visited symbol
            if( visited.count( parent.get() ) )
            {
                wxLogTrace( traceSymbolInheritance,
                            wxT( "Flatten: Circular inheritance detected in symbol '%s' (lib: %s)" ),
                            m_name, m_libId.GetLibNickname().wx_str() );
                break;
            }

            visited.insert( parent.get() );
            parentChain.push_back( parent.get() );
            parent = parent->m_parent.lock();
        }

        // Start with the root (last in chain) and work down
        if( !parentChain.empty() )
        {
            retv = std::make_unique<LIB_SYMBOL>( *parentChain.back() );

            // Apply each derived symbol's overrides from root down (skip the root itself)
            for( int i = static_cast<int>( parentChain.size() ) - 2; i >= 0; --i )
            {
                const LIB_SYMBOL* derived = parentChain[i];

                // Overwrite parent's mandatory fields for fields which are defined in derived.
                for( FIELD_T fieldId : MANDATORY_FIELDS )
                {
                    if( !derived->GetField( fieldId )->GetText().IsEmpty() )
                        *retv->GetField( fieldId ) = *derived->GetField( fieldId );
                }

                // Grab all the rest of derived symbol fields.
                for( const SCH_ITEM& item : derived->m_drawings[SCH_FIELD_T] )
                {
                    const SCH_FIELD* field = static_cast<const SCH_FIELD*>( &item );

                    if( field->IsMandatory() )
                        continue;

                    SCH_FIELD* newField = new SCH_FIELD( *field );
                    newField->SetParent( retv.get() );

                    SCH_FIELD* parentField = retv->GetField( field->GetName() );

                    if( !parentField )
                    {
                        retv->AddDrawItem( newField );
                    }
                    else
                    {
                        retv->RemoveDrawItem( parentField );
                        retv->AddDrawItem( newField );
                    }
                }

                if( !derived->m_keyWords.IsEmpty() )
                    retv->SetKeyWords( derived->m_keyWords );

                if( !derived->m_fpFilters.IsEmpty() )
                    retv->SetFPFilters( derived->m_fpFilters );
            }
        }
        else
        {
            // Cycle detected at immediate parent level - just copy this symbol
            retv = std::make_unique<LIB_SYMBOL>( *this );
            retv->m_parent.reset();
            return retv;
        }

        // Now apply this symbol's overrides (the original "this" symbol)
        retv->m_name = m_name;
        retv->SetLibId( m_libId );

        // Overwrite parent's mandatory fields for fields which are defined in this.
        for( FIELD_T fieldId : MANDATORY_FIELDS )
        {
            if( !GetField( fieldId )->GetText().IsEmpty() )
                *retv->GetField( fieldId ) = *GetField( fieldId );
        }

        // Grab all the rest of derived symbol fields.
        for( const SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
        {
            const SCH_FIELD* field = static_cast<const SCH_FIELD*>( &item );

            // Mandatory fields were already resolved.
            if( field->IsMandatory() )
                continue;

            SCH_FIELD* newField = new SCH_FIELD( *field );
            newField->SetParent( retv.get() );

            SCH_FIELD* parentField = retv->GetField( field->GetName() );

            if( !parentField ) // Derived symbol field does not exist in parent symbol.
            {
                retv->AddDrawItem( newField );
            }
            else // Derived symbol field overrides the parent symbol field.
            {
                retv->RemoveDrawItem( parentField );
                retv->AddDrawItem( newField );
            }
        }

        // Use this symbol's keywords/filters if set, otherwise keep inherited ones
        if( !m_keyWords.IsEmpty() )
            retv->SetKeyWords( m_keyWords );

        if( !m_fpFilters.IsEmpty() )
            retv->SetFPFilters( m_fpFilters );

        for( const auto& file : EmbeddedFileMap() )
        {
            EMBEDDED_FILES::EMBEDDED_FILE* newFile = new EMBEDDED_FILES::EMBEDDED_FILE( *file.second );
            retv->AddFile( newFile );
        }

        // Get excluded flags from the immediate parent (first in chain)
        if( !parentChain.empty() )
        {
            retv->SetExcludedFromSim( parentChain.front()->GetExcludedFromSim() );
            retv->SetExcludedFromBOM( parentChain.front()->GetExcludedFromBOM() );
            retv->SetExcludedFromBoard( parentChain.front()->GetExcludedFromBoard() );
        }

        retv->m_parent.reset();
    }
    else
    {
        retv = std::make_unique<LIB_SYMBOL>( *this );
    }

    return retv;
}


const wxString LIB_SYMBOL::GetLibraryName() const
{
    if( m_library )
        return m_library->GetName();

    return m_libId.GetLibNickname();
}


bool LIB_SYMBOL::IsLocalPower() const
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->IsLocalPower();
    }

    return m_options == ENTRY_LOCAL_POWER;
}


void LIB_SYMBOL::SetLocalPower()
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
        {
            root->SetLocalPower();
            return;
        }
    }

    m_options = ENTRY_LOCAL_POWER;
}


bool LIB_SYMBOL::IsGlobalPower() const
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->IsGlobalPower();
    }

    return m_options == ENTRY_GLOBAL_POWER;
}


bool LIB_SYMBOL::IsPower() const
{
    return IsLocalPower() || IsGlobalPower();
}


void LIB_SYMBOL::SetGlobalPower()
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
        {
            root->SetGlobalPower();
            return;
        }
    }

    m_options = ENTRY_GLOBAL_POWER;
}


bool LIB_SYMBOL::IsNormal() const
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->IsNormal();
    }

    return m_options == ENTRY_NORMAL;
}


void LIB_SYMBOL::SetNormal()
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
        {
            root->SetNormal();
            return;
        }
    }

    m_options = ENTRY_NORMAL;
}


wxString LIB_SYMBOL::LetterSubReference( int aUnit, wxChar aInitialLetter )
{
    // use letters as notation. To allow more than 26 units, the sub ref
    // use one letter if letter = A .. Z or a ... z, and 2 letters otherwise
    // first letter is expected to be 'A' or 'a' (i.e. 26 letters are available)
    int      u;
    wxString suffix;

    do
    {
        u = ( aUnit - 1 ) % 26;
        suffix = wxChar( aInitialLetter + u ) + suffix;
        aUnit = ( aUnit - u ) / 26;
    } while( aUnit > 0 );

    return suffix;
}


bool LIB_SYMBOL::ResolveTextVar( wxString* token, int aDepth ) const
{
    wxString footprint;

    for( const SCH_ITEM& item : m_drawings )
    {
        if( item.Type() == SCH_FIELD_T )
        {
            const SCH_FIELD& field = static_cast<const SCH_FIELD&>( item );

            if( field.GetId() == FIELD_T::FOOTPRINT )
                footprint = field.GetShownText( nullptr, false, aDepth + 1 );

            if( token->IsSameAs( field.GetCanonicalName().Upper() ) || token->IsSameAs( field.GetName(), false ) )
            {
                *token = field.GetShownText( nullptr, false, aDepth + 1 );
                return true;
            }
        }
    }

    // Consider missing simulation fields as empty, not un-resolved
    if( token->IsSameAs( wxT( "SIM.DEVICE" ) ) || token->IsSameAs( wxT( "SIM.TYPE" ) )
        || token->IsSameAs( wxT( "SIM.PINS" ) ) || token->IsSameAs( wxT( "SIM.PARAMS" ) )
        || token->IsSameAs( wxT( "SIM.LIBRARY" ) ) || token->IsSameAs( wxT( "SIM.NAME" ) ) )
    {
        *token = wxEmptyString;
        return true;
    }

    if( token->IsSameAs( wxT( "FOOTPRINT_LIBRARY" ) ) )
    {
        wxArrayString parts = wxSplit( footprint, ':' );

        if( parts.Count() > 0 )
            *token = parts[0];
        else
            *token = wxEmptyString;

        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        wxArrayString parts = wxSplit( footprint, ':' );

        if( parts.Count() > 1 )
            *token = parts[std::min( 1, (int) parts.size() - 1 )];
        else
            *token = wxEmptyString;

        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_LIBRARY" ) ) )
    {
        *token = m_libId.GetUniStringLibNickname();
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_NAME" ) ) )
    {
        *token = m_libId.GetUniStringLibItemName();
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_DESCRIPTION" ) ) )
    {
        *token = GetShownDescription( aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "SYMBOL_KEYWORDS" ) ) )
    {
        *token = GetShownKeyWords( aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_BOM" ) ) )
    {
        *token = this->GetExcludedFromBOM() ? _( "Excluded from BOM" ) : wxString( "" );
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_BOARD" ) ) )
    {
        *token = this->GetExcludedFromBoard() ? _( "Excluded from board" ) : wxString( "" );
        return true;
    }
    else if( token->IsSameAs( wxT( "EXCLUDE_FROM_SIM" ) ) )
    {
        *token = this->GetExcludedFromSim() ? _( "Excluded from simulation" ) : wxString( "" );
        return true;
    }
    else if( token->IsSameAs( wxT( "DNP" ) ) )
    {
        *token = this->GetDNP() ? _( "DNP" ) : wxString( "" );
        return true;
    }

    return false;
}


void LIB_SYMBOL::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts, int aUnit, int aBodyStyle,
                       const VECTOR2I& aOffset, bool aDimmed )
{
    wxASSERT( aPlotter != nullptr );

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR4D              color = renderSettings->GetLayerColor( LAYER_DEVICE );
    COLOR4D              bg = renderSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( color.m_text.has_value() && Schematic() )
        color = COLOR4D( ResolveText( color.m_text.value(), &Schematic()->CurrentSheet() ) );

    if( aDimmed )
    {
        color.Desaturate();
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );

    for( SCH_ITEM& item : m_drawings )
    {
        // Do not plot private items
        if( item.IsPrivate() )
            continue;

        // LIB_FIELDs are not plotted here, because this plot function is used to plot schematic
        // items which have their own SCH_FIELDs
        if( item.Type() == SCH_FIELD_T )
            continue;

        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        if( aBodyStyle && item.m_bodyStyle && ( item.m_bodyStyle != aBodyStyle ) )
            continue;

        item.Plot( aPlotter, aBackground, aPlotOpts, aUnit, aBodyStyle, aOffset, aDimmed );
    }
}


void LIB_SYMBOL::PlotFields( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts, int aUnit,
                             int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    wxASSERT( aPlotter != nullptr );

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    COLOR4D              color = renderSettings->GetLayerColor( LAYER_FIELDS );
    COLOR4D              bg = renderSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || !aPlotter->GetColorMode() )
        bg = COLOR4D::WHITE;

    if( aDimmed )
    {
        color.Desaturate();
        color = color.Mix( bg, 0.5f );
    }

    aPlotter->SetColor( color );

    for( SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        SCH_FIELD& field = static_cast<SCH_FIELD&>( item );

        if( !renderSettings->m_ShowHiddenFields && !field.IsVisible() )
            continue;

        // The reference is a special case: we should change the basic text
        // to add '?' and the part id
        wxString tmp = field.GetText();

        field.SetText( field.GetFullText( aUnit ) );
        item.Plot( aPlotter, aBackground, aPlotOpts, aUnit, aBodyStyle, aOffset, aDimmed );

        field.SetText( tmp );
    }
}


void LIB_SYMBOL::FixupDrawItems()
{
    std::vector<SCH_SHAPE*> potential_top_items;
    std::vector<SCH_ITEM*>  bottom_items;

    for( SCH_ITEM& item : m_drawings )
    {
        if( item.Type() == SCH_SHAPE_T )
        {
            SCH_SHAPE& shape = static_cast<SCH_SHAPE&>( item );

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
               []( SCH_ITEM* a, SCH_ITEM* b )
               {
                   return a->GetBoundingBox().GetArea() > b->GetBoundingBox().GetArea();
               } );

    for( SCH_SHAPE* item : potential_top_items )
    {
        for( SCH_ITEM* bottom_item : bottom_items )
        {
            if( item->GetBoundingBox().Contains( bottom_item->GetBoundingBox() ) )
            {
                item->SetFillMode( FILL_T::FILLED_WITH_BG_BODYCOLOR );
                break;
            }
        }
    }
}


void LIB_SYMBOL::RemoveDrawItem( SCH_ITEM* aItem )
{
    wxASSERT( aItem != nullptr );

    // none of the MANDATORY_FIELDS may be removed in RAM, but they may be
    // omitted when saving to disk.
    if( aItem->Type() == SCH_FIELD_T )
    {
        if( static_cast<SCH_FIELD*>( aItem )->IsMandatory() )
            return;
    }

    LIB_ITEMS& items = m_drawings[aItem->Type()];

    for( LIB_ITEMS::iterator i = items.begin(); i != items.end(); i++ )
    {
        if( &*i == aItem )
        {
            items.erase( i );
            break;
        }
    }
}


void LIB_SYMBOL::AddDrawItem( SCH_ITEM* aItem, bool aSort )
{
    if( aItem )
    {
        aItem->SetParent( this );

        m_drawings.push_back( aItem );

        if( aSort )
            m_drawings.sort();
    }
}


std::vector<SCH_PIN*> LIB_SYMBOL::GetGraphicalPins( int aUnit, int aBodyStyle ) const
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->GetGraphicalPins( aUnit, aBodyStyle );
    }

    std::vector<SCH_PIN*> pins;

    /* Notes:
     * when aUnit == 0: no unit filtering
     * when aBodyStyle == 0: no body style filtering
     * when m_unit == 0, the item is common to all units
     * when m_bodyStyle == 0, the item is common to all body styles
     */

    for( const SCH_ITEM& item : m_drawings[SCH_PIN_T] )
    {
        // Unit filtering:
        if( aUnit && item.m_unit && ( item.m_unit != aUnit ) )
            continue;

        // De Morgan variant filtering:
        if( aBodyStyle && item.m_bodyStyle && ( item.m_bodyStyle != aBodyStyle ) )
            continue;

        // TODO: get rid of const_cast.  (It used to be a C-style cast so was less noticeable.)
        SCH_PIN* pin = const_cast<SCH_PIN*>( static_cast<const SCH_PIN*>( &item ) );
        wxLogTrace(
                traceStackedPins,
                wxString::Format(
                        "GetGraphicalPins: lib='%s' unit=%d body=%d -> include pin name='%s' number='%s' shownNum='%s'",
                        GetLibId().Format().wx_str(), aUnit, aBodyStyle, pin->GetName(), pin->GetNumber(),
                        pin->GetShownNumber() ) );
        pins.push_back( pin );
    }

    return pins;
}


std::vector<LIB_SYMBOL::UNIT_PIN_INFO> LIB_SYMBOL::GetUnitPinInfo() const
{
    std::vector<UNIT_PIN_INFO> units;

    int unitCount = std::max( GetUnitCount(), 1 );

    auto compareByPosition = []( SCH_PIN* a, SCH_PIN* b )
    {
        VECTOR2I positionA = a->GetPosition();
        VECTOR2I positionB = b->GetPosition();

        if( positionA.x != positionB.x )
            return positionA.x < positionB.x;

        return positionA.y < positionB.y;
    };

    for( int unitIdx = 1; unitIdx <= unitCount; ++unitIdx )
    {
        UNIT_PIN_INFO unitInfo;
        unitInfo.m_unitName = GetUnitDisplayName( unitIdx, false );

        std::vector<SCH_PIN*> pinList = GetGraphicalPins( unitIdx, 0 );

        std::sort( pinList.begin(), pinList.end(), compareByPosition );

        std::unordered_set<wxString> seenNumbers;

        for( SCH_PIN* basePin : pinList )
        {
            bool                  stackedValid = false;
            std::vector<wxString> expandedNumbers = basePin->GetStackedPinNumbers( &stackedValid );

            if( stackedValid && !expandedNumbers.empty() )
            {
                for( const wxString& number : expandedNumbers )
                {
                    if( seenNumbers.insert( number ).second )
                        unitInfo.m_pinNumbers.push_back( number );
                }

                continue;
            }

            const wxString& number = basePin->GetNumber();

            if( !number.IsEmpty() && seenNumbers.insert( number ).second )
                unitInfo.m_pinNumbers.push_back( number );
        }

        units.push_back( std::move( unitInfo ) );
    }

    return units;
}


std::vector<LIB_SYMBOL::LOGICAL_PIN> LIB_SYMBOL::GetLogicalPins( int aUnit, int aBodyStyle ) const
{
    std::vector<LOGICAL_PIN> out;

    for( SCH_PIN* pin : GetGraphicalPins( aUnit, aBodyStyle ) )
    {
        bool                  valid = false;
        std::vector<wxString> expanded = pin->GetStackedPinNumbers( &valid );

        if( valid && !expanded.empty() )
        {
            for( const wxString& num : expanded )
            {
                out.push_back( LOGICAL_PIN{ pin, num } );
                wxLogTrace( traceStackedPins,
                            wxString::Format( "GetLogicalPins: base='%s' -> '%s'", pin->GetShownNumber(), num ) );
            }
        }
        else
        {
            out.push_back( LOGICAL_PIN{ pin, pin->GetShownNumber() } );
            wxLogTrace( traceStackedPins,
                        wxString::Format( "GetLogicalPins: base='%s' (no expansion)", pin->GetShownNumber() ) );
        }
    }

    return out;
}


int LIB_SYMBOL::GetPinCount()
{
    int count = 0;

    for( SCH_PIN* pin : GetGraphicalPins( 0 /* all units */, 1 /* single body style */ ) )
    {
        int pinCount = pin->GetStackedPinCount();
        count += pinCount;
    }

    wxLogTrace( "CVPCB_PINCOUNT", "LIB_SYMBOL::GetPinCount total for lib='%s' => %d", GetLibId().Format().wx_str(),
                count );

    return count;
}


SCH_PIN* LIB_SYMBOL::GetPin( const wxString& aNumber, int aUnit, int aBodyStyle ) const
{
    for( SCH_PIN* pin : GetGraphicalPins( aUnit, aBodyStyle ) )
    {
        if( aNumber == pin->GetNumber() )
            return pin;
    }

    return nullptr;
}


bool LIB_SYMBOL::PinsConflictWith( const LIB_SYMBOL& aOtherPart, bool aTestNums, bool aTestNames, bool aTestType,
                                   bool aTestOrientation, bool aTestLength ) const
{
    for( const SCH_PIN* pin : GetGraphicalPins() )
    {
        wxASSERT( pin );
        bool foundMatch = false;

        for( const SCH_PIN* otherPin : aOtherPart.GetGraphicalPins() )
        {
            wxASSERT( otherPin );

            // Same unit?
            if( pin->GetUnit() != otherPin->GetUnit() )
                continue;

            // Same body stype?
            if( pin->GetBodyStyle() != otherPin->GetBodyStyle() )
                continue;

            // Same position?
            if( pin->GetPosition() != otherPin->GetPosition() )
                continue;

            // Same number?
            if( aTestNums && ( pin->GetNumber() != otherPin->GetNumber() ) )
                continue;

            // Same name?
            if( aTestNames && ( pin->GetName() != otherPin->GetName() ) )
                continue;

            // Same electrical type?
            if( aTestType && ( pin->GetType() != otherPin->GetType() ) )
                continue;

            // Same orientation?
            if( aTestOrientation && ( pin->GetOrientation() != otherPin->GetOrientation() ) )
                continue;

            // Same length?
            if( aTestLength && ( pin->GetLength() != otherPin->GetLength() ) )
                continue;

            foundMatch = true;
            break; // Match found so search is complete.
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

std::vector<SCH_PIN*> LIB_SYMBOL::GetPins() const
{
    // Back-compat shim: return graphical pins for all units/body styles
    return GetGraphicalPins( 0, 0 );
}


const BOX2I LIB_SYMBOL::GetUnitBoundingBox( int aUnit, int aBodyStyle, bool aIgnoreHiddenFields,
                                            bool aIgnoreLabelsOnInvisiblePins ) const
{
    BOX2I bBox; // Start with a fresh BOX2I so the Merge algorithm works

    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            bBox = root->GetUnitBoundingBox( aUnit, aBodyStyle, aIgnoreHiddenFields, aIgnoreLabelsOnInvisiblePins );
    }

    for( const SCH_ITEM& item : m_drawings )
    {
        if( item.m_unit > 0 && m_unitCount > 1 && aUnit > 0 && aUnit != item.m_unit )
            continue;

        if( item.m_bodyStyle > 0 && aBodyStyle > 0 && aBodyStyle != item.m_bodyStyle )
            continue;

        if( aIgnoreHiddenFields && item.Type() == SCH_FIELD_T )
        {
            if( !static_cast<const SCH_FIELD&>( item ).IsVisible() )
                continue;
        }

        if( item.Type() == SCH_PIN_T && !aIgnoreLabelsOnInvisiblePins )
        {
            const SCH_PIN& pin = static_cast<const SCH_PIN&>( item );
            bBox.Merge( pin.GetBoundingBox( true, true, false ) );
        }
        else
        {
            bBox.Merge( item.GetBoundingBox() );
        }
    }

    return bBox;
}


const BOX2I LIB_SYMBOL::GetBodyBoundingBox( int aUnit, int aBodyStyle, bool aIncludePins,
                                            bool aIncludePrivateItems ) const
{
    BOX2I bbox;

    for( const SCH_ITEM& item : m_drawings )
    {
        if( item.m_unit > 0 && aUnit > 0 && aUnit != item.m_unit )
            continue;

        if( item.m_bodyStyle > 0 && aBodyStyle > 0 && aBodyStyle != item.m_bodyStyle )
            continue;

        if( item.IsPrivate() && !aIncludePrivateItems )
            continue;

        if( item.Type() == SCH_FIELD_T )
            continue;

        if( item.Type() == SCH_PIN_T )
        {
            const SCH_PIN& pin = static_cast<const SCH_PIN&>( item );

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
    m_drawings[SCH_FIELD_T].clear();
}


void LIB_SYMBOL::AddField( SCH_FIELD* aField )
{
    AddDrawItem( aField );
}


void LIB_SYMBOL::SetFields( const std::vector<SCH_FIELD>& aFieldsList )
{
    deleteAllFields();

    for( const SCH_FIELD& src : aFieldsList )
    {
        // drawings is a ptr_vector, new and copy an object on the heap.
        SCH_FIELD* field = new SCH_FIELD( src );

        field->SetParent( this );
        m_drawings.push_back( field );
    }

    m_drawings.sort();
}


void LIB_SYMBOL::GetFields( std::vector<SCH_FIELD*>& aList, bool aVisibleOnly ) const
{
    for( const SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        const SCH_FIELD* field = static_cast<const SCH_FIELD*>( &item );

        if( aVisibleOnly )
        {
            if( !field->IsVisible() || field->GetText().IsEmpty() )
                continue;
        }

        aList.push_back( const_cast<SCH_FIELD*>( field ) );
    }

    std::sort( aList.begin(), aList.end(),
               []( SCH_FIELD* lhs, SCH_FIELD* rhs )
               {
                   return lhs->GetOrdinal() < rhs->GetOrdinal();
               } );
}


void LIB_SYMBOL::CopyFields( std::vector<SCH_FIELD>& aList )
{
    std::vector<SCH_FIELD*> orderedFields;

    GetFields( orderedFields );

    for( SCH_FIELD* field : orderedFields )
        aList.emplace_back( *field );
}


int LIB_SYMBOL::GetNextFieldOrdinal() const
{
    int ordinal = 42; // Arbitrarily larger than any mandatory FIELD_T id

    for( const SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
        ordinal = std::max( ordinal, static_cast<const SCH_FIELD*>( &item )->GetOrdinal() + 1 );

    return ordinal;
}


const SCH_FIELD* LIB_SYMBOL::GetField( FIELD_T aFieldType ) const
{
    for( const SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        const SCH_FIELD* field = static_cast<const SCH_FIELD*>( &item );

        if( field->GetId() == aFieldType )
            return field;
    }

    return nullptr;
}


SCH_FIELD* LIB_SYMBOL::GetField( FIELD_T aFieldType )
{
    for( SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        SCH_FIELD* field = static_cast<SCH_FIELD*>( &item );

        if( field->GetId() == aFieldType )
            return field;
    }

    return nullptr;
}


const SCH_FIELD* LIB_SYMBOL::GetField( const wxString& aFieldName ) const
{
    for( const SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        const SCH_FIELD& field = static_cast<const SCH_FIELD&>( item );

        if( field.GetName() == aFieldName )
            return &field;
    }

    return nullptr;
}


SCH_FIELD* LIB_SYMBOL::GetField( const wxString& aFieldName )
{
    for( SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        SCH_FIELD& field = static_cast<SCH_FIELD&>( item );

        if( field.GetName() == aFieldName )
            return &field;
    }

    return nullptr;
}


SCH_FIELD* LIB_SYMBOL::FindFieldCaseInsensitive( const wxString& aFieldName )
{
    for( SCH_ITEM& item : m_drawings[SCH_FIELD_T] )
    {
        SCH_FIELD& field = static_cast<SCH_FIELD&>( item );

        if( field.GetCanonicalName().IsSameAs( aFieldName, false ) )
            return &field;
    }

    return nullptr;
}


const SCH_FIELD& LIB_SYMBOL::GetValueField() const
{
    const SCH_FIELD* field = GetField( FIELD_T::VALUE );
    wxASSERT( field != nullptr );
    return *field;
}


const SCH_FIELD& LIB_SYMBOL::GetReferenceField() const
{
    const SCH_FIELD* field = GetField( FIELD_T::REFERENCE );
    wxASSERT( field != nullptr );
    return *field;
}


const SCH_FIELD& LIB_SYMBOL::GetFootprintField() const
{
    const SCH_FIELD* field = GetField( FIELD_T::FOOTPRINT );
    wxASSERT( field != nullptr );
    return *field;
}


const SCH_FIELD& LIB_SYMBOL::GetDatasheetField() const
{
    const SCH_FIELD* field = GetField( FIELD_T::DATASHEET );
    wxASSERT( field != nullptr );
    return *field;
}


const SCH_FIELD& LIB_SYMBOL::GetDescriptionField() const
{
    const SCH_FIELD* field = GetField( FIELD_T::DESCRIPTION );
    wxASSERT( field != nullptr );
    return *field;
}


wxString LIB_SYMBOL::GetPrefix()
{
    wxString refDesignator = GetField( FIELD_T::REFERENCE )->GetText();

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


void LIB_SYMBOL::RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode )
{
    for( SCH_ITEM& item : m_drawings )
        aFunction( &item );
}


void LIB_SYMBOL::Move( const VECTOR2I& aOffset )
{
    for( SCH_ITEM& item : m_drawings )
        item.Move( aOffset );
}


// Before V10 we didn't store the number of body styles in a symbol -- we just looked through all
// its drawings each time we wanted to know.  This is now only used to set the count when a legacy
// symbol is first read.  (Legacy symbols also didn't support arbitrary body styles, so the count
// is always 1 or 2, and when 2 it is always a De Morgan pair.)
bool LIB_SYMBOL::HasLegacyAlternateBodyStyle() const
{
    for( const SCH_ITEM& item : m_drawings )
    {
        if( item.m_bodyStyle > BODY_STYLE::BASE )
            return true;
    }

    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->HasLegacyAlternateBodyStyle();
    }

    return false;
}


int LIB_SYMBOL::GetMaxPinNumber() const
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->GetMaxPinNumber();
    }

    int maxPinNumber = 0;

    for( const SCH_ITEM& item : m_drawings[SCH_PIN_T] )
    {
        const SCH_PIN* pin = static_cast<const SCH_PIN*>( &item );
        long           currentPinNumber = 0;

        if( pin->GetNumber().ToLong( &currentPinNumber ) )
            maxPinNumber = std::max( maxPinNumber, (int) currentPinNumber );
    }

    return maxPinNumber;
}


void LIB_SYMBOL::ClearTempFlags()
{
    SCH_ITEM::ClearTempFlags();

    for( SCH_ITEM& item : m_drawings )
        item.ClearTempFlags();
}


void LIB_SYMBOL::ClearEditFlags()
{
    SCH_ITEM::ClearEditFlags();

    for( SCH_ITEM& item : m_drawings )
        item.ClearEditFlags();
}


SCH_ITEM* LIB_SYMBOL::LocateDrawItem( int aUnit, int aBodyStyle, KICAD_T aType, const VECTOR2I& aPoint )
{
    for( SCH_ITEM& item : m_drawings )
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


SCH_ITEM* LIB_SYMBOL::LocateDrawItem( int aUnit, int aBodyStyle, KICAD_T aType, const VECTOR2I& aPoint,
                                      const TRANSFORM& aTransform )
{
    /* we use LocateDrawItem( int aUnit, int convert, KICAD_T type, const
     * VECTOR2I& pt ) to search items.
     * because this function uses DefaultTransform as orient/mirror matrix
     * we temporary copy aTransform in DefaultTransform
     */
    TRANSFORM transform = DefaultTransform;
    DefaultTransform = aTransform;

    SCH_ITEM* item = LocateDrawItem( aUnit, aBodyStyle, aType, aPoint );

    // Restore matrix
    DefaultTransform = transform;

    return item;
}


INSPECT_RESULT LIB_SYMBOL::Visit( INSPECTOR aInspector, void* aTestData, const std::vector<KICAD_T>& aScanTypes )
{
    // The part itself is never inspected, only its children
    for( SCH_ITEM& item : m_drawings )
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
        // Iterate each drawing-type bucket and erase items that belong to units > aCount.
        for( int type = LIB_ITEMS_CONTAINER::FIRST_TYPE; type <= LIB_ITEMS_CONTAINER::LAST_TYPE; ++type )
        {
            auto it = m_drawings.begin( type );

            while( it != m_drawings.end( type ) )
            {
                if( it->m_unit > aCount )
                    it = m_drawings.erase( it ); // returns next iterator
                else
                    ++it;
            }
        }
    }
    else if( aDuplicateDrawItems )
    {
        int prevCount = m_unitCount;

        // Temporary storage for new items, as adding new items directly to
        // m_drawings may cause the buffer reallocation which invalidates the
        // iterators
        std::vector<SCH_ITEM*> tmp;

        for( SCH_ITEM& item : m_drawings )
        {
            if( item.m_unit != 1 )
                continue;

            for( int j = prevCount + 1; j <= aCount; j++ )
            {
                SCH_ITEM* newItem = item.Duplicate( IGNORE_PARENT_GROUP );
                newItem->m_unit = j;
                tmp.push_back( newItem );
            }
        }

        for( SCH_ITEM* item : tmp )
            m_drawings.push_back( item );
    }

    m_drawings.sort();
    m_unitCount = aCount;
}


int LIB_SYMBOL::GetUnitCount() const
{
    if( IsDerived() )
    {
        std::shared_ptr<LIB_SYMBOL> root = GetSafeRootSymbol( this, __FUNCTION__ );

        if( root.get() != this )
            return root->GetUnitCount();
    }

    return m_unitCount;
}


void LIB_SYMBOL::SetBodyStyleCount( int aCount, bool aDuplicateDrawItems, bool aDuplicatePins )
{
    if( GetBodyStyleCount() == aCount )
        return;

    // Duplicate items to create the converted shape
    if( GetBodyStyleCount() < aCount )
    {
        if( aDuplicateDrawItems || aDuplicatePins )
        {
            std::vector<SCH_ITEM*> tmp; // Temporarily store the duplicated pins here.

            for( SCH_ITEM& item : m_drawings )
            {
                if( item.Type() != SCH_PIN_T && !aDuplicateDrawItems )
                    continue;

                if( item.m_bodyStyle == 1 )
                {
                    SCH_ITEM* newItem = item.Duplicate( IGNORE_PARENT_GROUP );
                    newItem->m_bodyStyle = 2;
                    tmp.push_back( newItem );
                }
            }

            // Transfer the new pins to the LIB_SYMBOL.
            for( SCH_ITEM* item : tmp )
                m_drawings.push_back( item );
        }
    }
    else
    {
        // Delete converted shape items because the converted shape does not exist
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


std::vector<SCH_ITEM*> LIB_SYMBOL::GetUnitDrawItems( int aUnit, int aBodyStyle )
{
    std::vector<SCH_ITEM*> unitItems;

    for( SCH_ITEM& item : m_drawings )
    {
        if( item.Type() == SCH_FIELD_T )
            continue;

        if( ( aBodyStyle == -1 && item.GetUnit() == aUnit ) || ( aUnit == -1 && item.GetBodyStyle() == aBodyStyle )
            || ( aUnit == item.GetUnit() && aBodyStyle == item.GetBodyStyle() ) )
        {
            unitItems.push_back( &item );
        }
    }

    return unitItems;
}


std::vector<LIB_SYMBOL_UNIT> LIB_SYMBOL::GetUnitDrawItems()
{
    std::vector<LIB_SYMBOL_UNIT> units;

    for( SCH_ITEM& item : m_drawings )
    {
        if( item.Type() == SCH_FIELD_T )
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
            LIB_SYMBOL_UNIT newUnit;
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


#define REPORT( msg )                                                                                                  \
    {                                                                                                                  \
        if( aReporter )                                                                                                \
            aReporter->Report( msg );                                                                                  \
    }
#define ITEM_DESC( item ) ( item )->GetItemDescription( &unitsProvider, false )

int LIB_SYMBOL::Compare( const LIB_SYMBOL& aRhs, int aCompareFlags, REPORTER* aReporter ) const
{
    UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MM );

    if( m_me == aRhs.m_me )
        return 0;

    if( !aReporter && ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC ) == 0 )
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

    // Make sure shapes are sorted. No need with fields or pins as those are matched by id/name and number.

    std::set<const SCH_ITEM*, SCH_ITEM::cmp_items> aShapes;
    std::set<const SCH_FIELD*>                     aFields;
    std::set<const SCH_PIN*>                       aPins;

    for( auto it = m_drawings.begin(); it != m_drawings.end(); ++it )
    {
        if( it->Type() == SCH_SHAPE_T )
            aShapes.insert( &( *it ) );
        else if( it->Type() == SCH_FIELD_T )
            aFields.insert( static_cast<const SCH_FIELD*>( &( *it ) ) );
        else if( it->Type() == SCH_PIN_T )
            aPins.insert( static_cast<const SCH_PIN*>( &( *it ) ) );
    }

    std::set<const SCH_ITEM*, SCH_ITEM::cmp_items> bShapes;
    std::set<const SCH_FIELD*>                     bFields;
    std::set<const SCH_PIN*>                       bPins;

    for( auto it = aRhs.m_drawings.begin(); it != aRhs.m_drawings.end(); ++it )
    {
        if( it->Type() == SCH_SHAPE_T )
            bShapes.insert( &( *it ) );
        else if( it->Type() == SCH_FIELD_T )
            bFields.insert( static_cast<const SCH_FIELD*>( &( *it ) ) );
        else if( it->Type() == SCH_PIN_T )
            bPins.insert( static_cast<const SCH_PIN*>( &( *it ) ) );
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
            if( int tmp2 = ( *aIt )->compare( *( *bIt ), aCompareFlags ) )
            {
                retv = tmp2;
                REPORT( wxString::Format( _( "Graphic item differs: %s; %s." ), ITEM_DESC( *aIt ),
                                          ITEM_DESC( *bIt ) ) );

                if( !aReporter )
                    return retv;
            }
        }
    }

    for( const SCH_PIN* aPin : aPins )
    {
        const SCH_PIN* bPin = aRhs.GetPin( aPin->GetNumber(), aPin->GetUnit(), aPin->GetBodyStyle() );

        if( !bPin )
        {
            retv = 1;
            REPORT( wxString::Format( _( "Extra pin in schematic symbol: %s." ), ITEM_DESC( aPin ) ) );

            if( !aReporter )
                return retv;
        }
        else if( int tmp = aPin->SCH_ITEM::compare( *bPin, aCompareFlags ) )
        {
            retv = tmp;
            REPORT( wxString::Format( _( "Pin %s differs: %s; %s" ), aPin->GetNumber(), ITEM_DESC( aPin ),
                                      ITEM_DESC( bPin ) ) );

            if( !aReporter )
                return retv;
        }
    }

    for( const SCH_PIN* bPin : bPins )
    {
        const SCH_PIN* aPin = aRhs.GetPin( bPin->GetNumber(), bPin->GetUnit(), bPin->GetBodyStyle() );

        if( !aPin )
        {
            retv = 1;
            REPORT( wxString::Format( _( "Missing pin in schematic symbol: %s." ), ITEM_DESC( bPin ) ) );

            if( !aReporter )
                return retv;
        }
    }

    for( const SCH_FIELD* aField : aFields )
    {
        const SCH_FIELD* bField = nullptr;

        if( aField->IsMandatory() )
            bField = aRhs.GetField( aField->GetId() );
        else
            bField = aRhs.GetField( aField->GetName() );

        if( !bField )
        {
            retv = 1;
            REPORT( wxString::Format( _( "Extra field in schematic symbol: %s." ), ITEM_DESC( aField ) ) );

            if( !aReporter )
                return retv;
        }
        else
        {
            int tmp = 0;

            // For EQUALITY comparison, we need to compare field content directly
            // since SCH_ITEM::compare() returns 0 for EQUALITY flag
            if( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::EQUALITY )
            {
                // Compare field text content
                tmp = aField->GetText().compare( bField->GetText() );
            }

            if( tmp == 0 )
            {
                // Fall back to base class comparison for other properties
                tmp = aField->SCH_ITEM::compare( *bField, aCompareFlags );
            }

            if( tmp != 0 )
            {
                retv = tmp;
                REPORT( wxString::Format( _( "Field '%s' differs: %s; %s." ), aField->GetName( false ),
                                          ITEM_DESC( aField ), ITEM_DESC( bField ) ) );

                if( !aReporter )
                    return retv;
            }
        }
    }

    for( const SCH_FIELD* bField : bFields )
    {
        const SCH_FIELD* aField = nullptr;

        if( bField->IsMandatory() )
            aField = aRhs.GetField( bField->GetId() );
        else
            aField = aRhs.GetField( bField->GetName() );

        if( !aField )
        {
            retv = 1;
            REPORT( wxString::Format( _( "Missing field in schematic symbol: %s." ), ITEM_DESC( bField ) ) );

            if( !aReporter )
                return retv;
        }
    }

    if( int tmp = static_cast<int>( m_fpFilters.GetCount() - aRhs.m_fpFilters.GetCount() ) )
    {
        retv = tmp;
        REPORT( _( "Footprint filter count differs." ) );

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

    if( ( aCompareFlags & SCH_ITEM::COMPARE_FLAGS::ERC ) == 0 )
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

        if( m_excludedFromPosFiles != aRhs.m_excludedFromPosFiles )
        {
            retv = ( m_excludedFromPosFiles ) ? -1 : 1;
            REPORT( _( "Exclude from position files settings differ." ) );

            if( !aReporter )
                return retv;
        }
    }

    if( !aReporter )
    {
        if( m_unitsLocked != aRhs.m_unitsLocked )
            return ( m_unitsLocked ) ? 1 : -1;

        // Compare unit display names...
        if( m_unitDisplayNames < aRhs.m_unitDisplayNames )
            return -1;
        else if( m_unitDisplayNames > aRhs.m_unitDisplayNames )
            return 1;

        // ... and body style names.
        if( m_bodyStyleNames < aRhs.m_bodyStyleNames )
            return -1;
        else if( m_bodyStyleNames > aRhs.m_bodyStyleNames )
            return 1;
    }

    return retv;
}


int LIB_SYMBOL::compare( const SCH_ITEM& aOther, int aCompareFlags ) const
{
    if( Type() != aOther.Type() )
        return Type() - aOther.Type();

    const LIB_SYMBOL* tmp = static_cast<const LIB_SYMBOL*>( &aOther );

    return Compare( *tmp, aCompareFlags );
}


double LIB_SYMBOL::Similarity( const SCH_ITEM& aOther ) const
{
    wxCHECK( aOther.Type() == LIB_SYMBOL_T, 0.0 );

    const LIB_SYMBOL& other = static_cast<const LIB_SYMBOL&>( aOther );
    double            similarity = 0.0;
    int               totalItems = 0;

    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    for( const SCH_ITEM& item : m_drawings )
    {
        totalItems += 1;
        double max_similarity = 0.0;

        for( const SCH_ITEM& otherItem : other.m_drawings )
        {
            double temp_similarity = item.Similarity( otherItem );
            max_similarity = std::max( max_similarity, temp_similarity );

            if( max_similarity == 1.0 )
                break;
        }

        similarity += max_similarity;
    }

    for( const SCH_PIN* pin : GetPins() )
    {
        totalItems += 1;
        double max_similarity = 0.0;

        for( const SCH_PIN* otherPin : other.GetPins() )
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

    if( m_excludedFromBoard != other.m_excludedFromBoard )
        similarity *= 0.9;

    if( m_excludedFromBOM != other.m_excludedFromBOM )
        similarity *= 0.9;

    if( m_excludedFromSim != other.m_excludedFromSim )
        similarity *= 0.9;

    if( m_excludedFromPosFiles != other.m_excludedFromPosFiles )
        similarity *= 0.9;

    if( m_flags != other.m_flags )
        similarity *= 0.9;

    if( m_unitCount != other.m_unitCount )
        similarity *= 0.5;

    if( GetBodyStyleCount() != other.GetBodyStyleCount() )
        similarity *= 0.5;
    else if( m_bodyStyleNames != other.m_bodyStyleNames )
        similarity *= 0.9;

    if( m_pinNameOffset != other.m_pinNameOffset )
        similarity *= 0.9;

    if( m_showPinNames != other.m_showPinNames )
        similarity *= 0.9;

    if( m_showPinNumbers != other.m_showPinNumbers )
        similarity *= 0.9;

    return similarity;
}


EMBEDDED_FILES* LIB_SYMBOL::GetEmbeddedFiles()
{
    return static_cast<EMBEDDED_FILES*>( this );
}


const EMBEDDED_FILES* LIB_SYMBOL::GetEmbeddedFiles() const
{
    return static_cast<const EMBEDDED_FILES*>( this );
}


void LIB_SYMBOL::AppendParentEmbeddedFiles( std::vector<EMBEDDED_FILES*>& aStack ) const
{
    std::set<const LIB_SYMBOL*> visited;
    visited.insert( this );

    std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock();

    while( parent )
    {
        // Cycle detected - parent chain points back to an already visited symbol
        if( visited.count( parent.get() ) )
        {
            wxLogTrace( traceSymbolInheritance,
                        wxT( "AppendParentEmbeddedFiles: Circular inheritance detected in "
                             "symbol '%s' (lib: %s)" ),
                        m_name, m_libId.GetLibNickname().wx_str() );
            break;
        }

        visited.insert( parent.get() );
        aStack.push_back( parent->GetEmbeddedFiles() );
        parent = parent->GetParent().lock();
    }
}


std::set<KIFONT::OUTLINE_FONT*> LIB_SYMBOL::GetFonts() const
{
    using EMBEDDING_PERMISSION = KIFONT::OUTLINE_FONT::EMBEDDING_PERMISSION;

    std::set<KIFONT::OUTLINE_FONT*> fonts;

    for( const SCH_ITEM& item : m_drawings )
    {
        if( item.Type() == SCH_TEXT_T )
        {
            const SCH_TEXT& text = static_cast<const SCH_TEXT&>( item );

            if( auto* font = text.GetFont(); font && !font->IsStroke() )
            {
                auto* outline = static_cast<KIFONT::OUTLINE_FONT*>( font );
                auto  permission = outline->GetEmbeddingPermission();

                if( permission == EMBEDDING_PERMISSION::EDITABLE || permission == EMBEDDING_PERMISSION::INSTALLABLE )
                {
                    fonts.insert( outline );
                }
            }
        }
    }

    return fonts;
}


void LIB_SYMBOL::EmbedFonts()
{
    std::set<KIFONT::OUTLINE_FONT*> fonts = GetFonts();

    for( KIFONT::OUTLINE_FONT* font : fonts )
    {
        auto file = GetEmbeddedFiles()->AddFile( font->GetFileName(), false );
        file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::FONT;
    }
}


std::optional<const std::set<wxString>> LIB_SYMBOL::GetJumperPinGroup( const wxString& aPinNumber ) const
{
    for( const std::set<wxString>& group : m_jumperPinGroups )
    {
        if( group.contains( aPinNumber ) )
            return group;
    }

    return std::nullopt;
}

static struct LIB_SYMBOL_DESC
{
    LIB_SYMBOL_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( LIB_SYMBOL );
        propMgr.InheritsAfter( TYPE_HASH( LIB_SYMBOL ), TYPE_HASH( SYMBOL ) );

        const wxString groupFields = _HKI( "Fields" );

        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, wxString>( _HKI( "Reference" ), &LIB_SYMBOL::SetRefProp,
                                                                 &LIB_SYMBOL::GetRefProp ),
                             groupFields );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, wxString>( _HKI( "Value" ), &LIB_SYMBOL::SetValueProp,
                                                                 &LIB_SYMBOL::GetValueProp ),
                             groupFields );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, wxString>( _HKI( "Footprint" ), &LIB_SYMBOL::SetFootprintProp,
                                                                 &LIB_SYMBOL::GetFootprintProp ),
                             groupFields );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, wxString>( _HKI( "Datasheet" ), &LIB_SYMBOL::SetDatasheetProp,
                                                                 &LIB_SYMBOL::GetDatasheetProp ),
                             groupFields );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, wxString>( _HKI( "Keywords" ), &LIB_SYMBOL::SetKeywordsProp,
                                                                 &LIB_SYMBOL::GetKeywordsProp ),
                             groupFields );

        const wxString groupSymbolDef = _HKI( "Symbol Definition" );

        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Define as Power Symbol" ),
                                                             &LIB_SYMBOL::SetPowerSymbolProp,
                                                             &LIB_SYMBOL::GetPowerSymbolProp ),
                             groupSymbolDef );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Define as Local Power Symbol" ),
                                                             &LIB_SYMBOL::SetLocalPowerSymbolProp,
                                                             &LIB_SYMBOL::GetLocalPowerSymbolProp ),
                             groupSymbolDef );

        const wxString groupPinDisplay = _HKI( "Pin Display" );

        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Show Pin Number" ), &SYMBOL::SetShowPinNumbers,
                                                         &SYMBOL::GetShowPinNumbers ),
                             groupPinDisplay );
        propMgr.AddProperty( new PROPERTY<SYMBOL, bool>( _HKI( "Show Pin Name" ), &SYMBOL::SetShowPinNames,
                                                         &SYMBOL::GetShowPinNames ),
                             groupPinDisplay );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Place Pin Names Inside" ),
                                                             &LIB_SYMBOL::SetPinNamesInsideProp,
                                                             &LIB_SYMBOL::GetPinNamesInsideProp ),
                             groupPinDisplay );
        propMgr.AddProperty( new PROPERTY<SYMBOL, int>( _HKI( "Pin Name Position Offset" ), &SYMBOL::SetPinNameOffset,
                                                        &SYMBOL::GetPinNameOffset, PROPERTY_DISPLAY::PT_SIZE ),
                             groupPinDisplay );

        const wxString groupAttributes = _HKI( "Attributes" );

        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Exclude from Simulation" ),
                                                             &LIB_SYMBOL::SetExcludedFromSimProp,
                                                             &LIB_SYMBOL::GetExcludedFromSimProp ),
                             groupAttributes );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Exclude from Board" ),
                                                             &LIB_SYMBOL::SetExcludedFromBoardProp,
                                                             &LIB_SYMBOL::GetExcludedFromBoardProp ),
                             groupAttributes );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Exclude from Bill of Materials" ),
                                                             &LIB_SYMBOL::SetExcludedFromBOMProp,
                                                             &LIB_SYMBOL::GetExcludedFromBOMProp ),
                             groupAttributes );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Exclude from Position Files" ),
                                                             &LIB_SYMBOL::SetExcludedFromPosFilesProp,
                                                             &LIB_SYMBOL::GetExcludedFromPosFilesProp ),
                             groupAttributes );

        const wxString groupUnits = _HKI( "Units and Body Styles" );

        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, int>( _HKI( "Number of Symbol Units" ), &LIB_SYMBOL::SetUnitProp,
                                                            &LIB_SYMBOL::GetUnitProp ),
                             groupUnits );
        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, bool>( _HKI( "Units are Interchangeable" ),
                                                             &LIB_SYMBOL::SetUnitsInterchangeableProp,
                                                             &LIB_SYMBOL::GetUnitsInterchangeableProp ),
                             groupUnits );

        auto multiBodyStyle = [=]( INSPECTABLE* aItem ) -> bool
        {
            if( LIB_SYMBOL* symbol = dynamic_cast<LIB_SYMBOL*>( aItem ) )
                return symbol->IsMultiBodyStyle();

            return false;
        };

        propMgr.AddProperty( new PROPERTY<LIB_SYMBOL, wxString>( _HKI( "Body Styles" ), &LIB_SYMBOL::SetBodyStyleProp,
                                                                 &LIB_SYMBOL::GetBodyStyleProp ),
                             groupUnits )
                .SetAvailableFunc( multiBodyStyle )
                .SetChoicesFunc(
                        []( INSPECTABLE* aItem )
                        {
                            wxPGChoices choices;

                            if( LIB_SYMBOL* symbol = dynamic_cast<LIB_SYMBOL*>( aItem ) )
                            {
                                for( int ii = 1; ii <= symbol->GetBodyStyleCount(); ii++ )
                                    choices.Add( symbol->GetBodyStyleDescription( ii, false ) );
                            }

                            return choices;
                        } );
    }
} _LIB_SYMBOL_DESC;
