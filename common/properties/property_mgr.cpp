/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inspectable.h>
#include <properties/property_mgr.h>
#include <properties/property.h>

#include <algorithm>
#include <ranges>
#include <utility>
#include <wx/wx.h>


// Global to prevent simultaneous multi-threaded static initialization
static const std::vector<PROPERTY_BASE*>   EMPTY_PROP_LIST;
static const std::map<PROPERTY_BASE*, int> EMPTY_PROP_DISPLAY_ORDER;
std::vector<wxString>                      EMPTY_GROUP_DISPLAY_ORDER;


void PROPERTY_MANAGER::RegisterType( TYPE_ID aType, const wxString& aName )
{
    wxASSERT( m_classNames.count( aType ) == 0 );
    m_classNames.emplace( aType, aName );
}


PROPERTY_BASE* PROPERTY_MANAGER::GetProperty( TYPE_ID aType, const wxString& aProperty ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return nullptr;

    const CLASS_DESC& classDesc = it->second;

    for( PROPERTY_BASE* property : classDesc.m_allProperties )
    {
        if( !aProperty.CmpNoCase( property->Name() ) )
            return property;
    }

    return nullptr;
}


const std::vector<PROPERTY_BASE*>& PROPERTY_MANAGER::GetProperties( TYPE_ID aType ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return EMPTY_PROP_LIST;

    return it->second.m_allProperties;
}


const std::map<PROPERTY_BASE*, int>& PROPERTY_MANAGER::GetDisplayOrder( TYPE_ID aType ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return EMPTY_PROP_DISPLAY_ORDER;

    return it->second.m_displayOrder;
}


const std::vector<wxString>& PROPERTY_MANAGER::GetGroupDisplayOrder( TYPE_ID aType ) const
{
    if( m_dirty )
        const_cast<PROPERTY_MANAGER*>( this )->Rebuild();

    auto it = m_classes.find( aType );

    if( it == m_classes.end() )
        return EMPTY_GROUP_DISPLAY_ORDER;

    return it->second.m_groupDisplayOrder;
}


const void* PROPERTY_MANAGER::TypeCast( const void* aSource, TYPE_ID aBase, TYPE_ID aTarget ) const
{
    if( aBase == aTarget )
        return aSource;

    auto classDesc = m_classes.find( aBase );

    if( classDesc == m_classes.end() )
        return aSource;

    const std::map<TYPE_ID, std::unique_ptr<TYPE_CAST_BASE>>& converters = classDesc->second.m_typeCasts;
    auto converter = converters.find( aTarget );

    if( converter == converters.end() )     // explicit type cast not found
        return IsOfType( aBase, aTarget ) ? aSource : nullptr;

    return (*converter->second)( aSource );
}


PROPERTY_BASE& PROPERTY_MANAGER::AddProperty( PROPERTY_BASE* aProperty, const wxString& aGroup )
{
    const wxString& name = aProperty->Name();
    TYPE_ID         hash = aProperty->OwnerHash();
    CLASS_DESC&     classDesc = getClass( hash );

    classDesc.m_ownProperties.emplace( name, aProperty );
    classDesc.m_ownDisplayOrder.emplace_back( aProperty );

    aProperty->SetGroup( aGroup );

    if( !classDesc.m_groups.count( aGroup ) )
    {
        classDesc.m_groupDisplayOrder.emplace_back( aGroup );
        classDesc.m_groups.insert( aGroup );
    }

    m_dirty = true;
    return *aProperty;
}


PROPERTY_BASE& PROPERTY_MANAGER::ReplaceProperty( size_t aBase, const wxString& aName, PROPERTY_BASE* aNew,
                                                  const wxString& aGroup )
{
    CLASS_DESC& classDesc = getClass( aNew->OwnerHash() );
    classDesc.m_replaced.insert( std::make_pair( aBase, aName ) );
    return AddProperty( aNew, aGroup );
}


void PROPERTY_MANAGER::AddTypeCast( TYPE_CAST_BASE* aCast )
{
    TYPE_ID     derivedHash = aCast->DerivedHash();
    CLASS_DESC& classDesc = getClass( aCast->BaseHash() );

    wxASSERT_MSG( classDesc.m_typeCasts.count( derivedHash ) == 0, wxT( "Such converter already exists" ) );
    classDesc.m_typeCasts.emplace( derivedHash, aCast );
}


void PROPERTY_MANAGER::InheritsAfter( TYPE_ID aDerived, TYPE_ID aBase )
{
    wxASSERT_MSG( aDerived != aBase, wxT( "Class cannot inherit from itself" ) );

    CLASS_DESC& derived = getClass( aDerived );
    derived.m_bases.push_back( getClass( aBase ) );
    m_dirty = true;

    wxASSERT_MSG( derived.m_bases.size() == 1 || derived.m_typeCasts.count( aBase ) == 1,
                  wxT( "You need to add a TYPE_CAST for classes inheriting from multiple bases" ) );
}


void PROPERTY_MANAGER::Mask( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName )
{
    wxASSERT_MSG( aDerived != aBase, wxT( "Class cannot mask from itself" ) );

    CLASS_DESC& derived = getClass( aDerived );
    derived.m_maskedBaseProperties.insert( std::make_pair( aBase, aName ) );
    m_dirty = true;
}


void PROPERTY_MANAGER::OverrideAvailability( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName,
                                             std::function<bool( INSPECTABLE* )> aFunc )
{
    wxASSERT_MSG( aDerived != aBase, wxT( "Class cannot override from itself" ) );

    CLASS_DESC& derived = getClass( aDerived );
    derived.m_availabilityOverrides[std::make_pair( aBase, aName )] = std::move( aFunc );
    m_dirty = true;
}


void PROPERTY_MANAGER::OverrideWriteability( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName,
                                             std::function<bool( INSPECTABLE* )> aFunc )
{
    wxASSERT_MSG( aDerived != aBase, wxT( "Class cannot override from itself" ) );

    CLASS_DESC& derived = getClass( aDerived );
    derived.m_writeabilityOverrides[std::make_pair( aBase, aName )] = std::move( aFunc );
    m_dirty = true;
}


bool PROPERTY_MANAGER::IsAvailableFor( TYPE_ID aItemClass, PROPERTY_BASE* aProp, INSPECTABLE* aItem )
{
    if( !aProp->Available( aItem ) )
        return false;

    CLASS_DESC& derived = getClass( aItemClass );

    auto it = derived.m_availabilityOverrides.find( std::make_pair( aProp->BaseHash(), aProp->Name() ) );

    if( it != derived.m_availabilityOverrides.end() )
        return it->second( aItem );

    return true;
}


bool PROPERTY_MANAGER::IsWriteableFor( TYPE_ID aItemClass, PROPERTY_BASE* aProp, INSPECTABLE* aItem )
{
    if( !aProp->Writeable( aItem ) )
        return false;

    CLASS_DESC& derived = getClass( aItemClass );

    auto it = derived.m_writeabilityOverrides.find( std::make_pair( aProp->BaseHash(), aProp->Name() ) );

    if( it != derived.m_writeabilityOverrides.end() )
        return it->second( aItem );

    return true;
}


bool PROPERTY_MANAGER::IsOfType( TYPE_ID aDerived, TYPE_ID aBase ) const
{
    if( aDerived == aBase )
        return true;

    auto derived = m_classes.find( aDerived );
    wxCHECK( derived != m_classes.end(), false );   // missing class description

    // traverse the hierarchy seeking for the base class
    for( const std::reference_wrapper<CLASS_DESC>& base : derived->second.m_bases )
    {
        if( IsOfType( base.get().m_id, aBase ) )
            return true;
    }

    return false;
}


void PROPERTY_MANAGER::Rebuild()
{
    for( std::pair<const TYPE_ID, CLASS_DESC>& classEntry : m_classes )
        classEntry.second.rebuild();

    m_dirty = false;
}


PROPERTY_MANAGER::CLASS_DESC& PROPERTY_MANAGER::getClass( TYPE_ID aTypeId )
{
    auto it = m_classes.find( aTypeId );

    if( it == m_classes.end() )
        tie( it, std::ignore ) = m_classes.emplace( aTypeId, CLASS_DESC( aTypeId ) );

    return it->second;
}


void PROPERTY_MANAGER::CLASS_DESC::rebuild()
{
    std::set<std::pair<size_t, wxString>> replaced;
    std::set<std::pair<size_t, wxString>> masked;
    m_allProperties.clear();
    collectPropsRecur( m_allProperties, replaced, m_displayOrder, masked );

    // We need to keep properties sorted to be able to use std::set_* functions
    sort( m_allProperties.begin(), m_allProperties.end() );

    std::vector<wxString> displayOrder;
    std::set<wxString> groups;

    auto collectGroups =
            [&]( std::set<wxString>& aSet, std::vector<wxString>& aResult )
            {
                auto collectGroupsRecursive =
                        []( auto& aSelf, std::set<wxString>& aSetR, std::vector<wxString>& aResultR,
                            const CLASS_DESC& aClassR ) -> void
                        {
                            for( const wxString& group : aClassR.m_groupDisplayOrder )
                            {
                                if( !aSetR.count( group ) )
                                {
                                    aSetR.insert( group );
                                    aResultR.emplace_back( group );
                                }
                            }

                            for( const CLASS_DESC& base : aClassR.m_bases )
                                aSelf( aSelf, aSetR, aResultR, base );
                        };

                collectGroupsRecursive( collectGroupsRecursive, aSet, aResult, *this );
            };

    // TODO(JE): This currently relies on rebuild() happening after all properties are added
    // separate out own groups vs. all groups to fix
    collectGroups( groups, displayOrder );
    m_groupDisplayOrder = std::move( displayOrder );
}


void PROPERTY_MANAGER::CLASS_DESC::collectPropsRecur( std::vector<PROPERTY_BASE*>& aResult,
                                                      std::set<std::pair<size_t, wxString>>& aReplaced,
                                                      std::map<PROPERTY_BASE*, int>& aDisplayOrder,
                                                      std::set<std::pair<size_t, wxString>>& aMasked ) const
{
    for( const std::pair<size_t, wxString>& replacedEntry : m_replaced )
        aReplaced.emplace( replacedEntry );

    for( const std::pair<size_t, wxString>& maskedEntry : m_maskedBaseProperties )
        aMasked.emplace( maskedEntry );

    /*
     * We want to insert our own properties in forward order, but earlier than anything already in
     * the list (which will have been added by a subclass of us)
     */
    int displayOrderStart = 0;

    if( !aDisplayOrder.empty() )
    {
        int firstSoFar = std::min_element( aDisplayOrder.begin(), aDisplayOrder.end(),
                                           []( const std::pair<PROPERTY_BASE*, int>& aFirst,
                                               const std::pair<PROPERTY_BASE*, int>& aSecond )
                                           {
                                               return aFirst.second < aSecond.second;
                                           } )->second;

        displayOrderStart = firstSoFar - m_ownProperties.size();
    }

    int idx = 0;

    for( PROPERTY_BASE* property : m_ownDisplayOrder )
    {
        std::set<std::pair<size_t, wxString>>::key_type propertyKey = std::make_pair( property->OwnerHash(),
                                                                                      property->Name() );
        // Do not store replaced properties
        if( aReplaced.count( propertyKey ) )
            continue;

        // Do not store masked properties
        if( aMasked.count( propertyKey ) )
            continue;

        aDisplayOrder[property] = displayOrderStart + idx++;
        aResult.push_back( property );
    }

    // Iterate backwards so that replaced properties appear before base properties
    for( std::reference_wrapper<CLASS_DESC> base : std::ranges::reverse_view( m_bases ) )
        base.get().collectPropsRecur( aResult, aReplaced, aDisplayOrder, aMasked );
}


PROPERTY_MANAGER::CLASSES_INFO PROPERTY_MANAGER::GetAllClasses()
{
    CLASSES_INFO rv;

    for( std::pair<const TYPE_ID, CLASS_DESC>& classEntry : m_classes )
    {
        CLASS_INFO info;

        info.type = classEntry.first;
        info.name = m_classNames[classEntry.first];

        for( PROPERTY_BASE* prop : classEntry.second.m_allProperties )
            info.properties.push_back( prop );

        rv.push_back( info );
    }

    return rv;
}


void PROPERTY_MANAGER::PropertyChanged( INSPECTABLE* aObject, PROPERTY_BASE* aProperty )
{
    auto callListeners =
            [&]( TYPE_ID typeId )
            {
                auto listeners = m_listeners.find( typeId );

                if( listeners != m_listeners.end() )
                {
                    for( const PROPERTY_LISTENER& listener : listeners->second )
                        listener( aObject, aProperty, m_managedCommit );
                }
            };

    CLASS_DESC& objectClass = getClass( TYPE_HASH( *aObject ) );

    callListeners( objectClass.m_id );

    for( CLASS_DESC& superClass : objectClass.m_bases )
        callListeners( superClass.m_id );
}


PROPERTY_COMMIT_HANDLER::PROPERTY_COMMIT_HANDLER( COMMIT* aCommit )
{
    wxCHECK2_MSG( PROPERTY_MANAGER::Instance().m_managedCommit == nullptr,
                  return, wxT( "Can't have more than one managed commit at a time!" ) );

    PROPERTY_MANAGER::Instance().m_managedCommit = aCommit;
}


PROPERTY_COMMIT_HANDLER::~PROPERTY_COMMIT_HANDLER()
{
    wxASSERT_MSG( PROPERTY_MANAGER::Instance().m_managedCommit != nullptr,
                  wxT( "Something went wrong: m_managedCommit already null!" ) );

    PROPERTY_MANAGER::Instance().m_managedCommit = nullptr;
}
