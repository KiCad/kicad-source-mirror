/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef PROPERTY_MGR_H
#define PROPERTY_MGR_H

#include <core/wx_stl_compat.h> // Needed for stl hash extensions

#include <wx/string.h>

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>
#include <eda_units.h>

class PROPERTY_BASE;
class TYPE_CAST_BASE;
class ORIGIN_TRANSFORMS;
class INSPECTABLE;
class COMMIT;

///< Unique type identifier
using TYPE_ID = size_t;

using PROPERTY_LISTENER = std::function<void( INSPECTABLE*, PROPERTY_BASE*, COMMIT* )>;

class PROPERTY_COMMIT_HANDLER
{
public:
    PROPERTY_COMMIT_HANDLER( COMMIT* aCommit );

    ~PROPERTY_COMMIT_HANDLER();
};

/**
 * Provide class metadata. Each class handled by PROPERTY_MANAGER
 * needs to be described using AddProperty(), AddTypeCast() and InheritsAfter() methods.
 *
 * Enum types use a dedicated property type (PROPERTY_ENUM), define its possible values
 * with ENUM_MAP class, then describe the type using macros:
 * - DECLARE_ENUM_TO_WXANY (in header files)
 * - IMPLEMENT_ENUM_TO_WXANY (in source files)
 * - ENUM_TO_WXANY (*most often used*; combines DECLARE and IMPLEMENT macros,
 *   if there is no need to share the description using header files)
 *
 * Once all classes are described, the property list must be build using
 * Rebuild() method.
 */
class PROPERTY_MANAGER
{
public:
    static PROPERTY_MANAGER& Instance()
    {
        static PROPERTY_MANAGER pm;
        return pm;
    }

    /**
     * Associate a name with a type.
     *
     * Build a map to provide faster type look-up.
     *
     * @param aType is the type identifier (obtained using TYPE_HASH()).
     * @param aName is the type name.
     */
    void RegisterType( TYPE_ID aType, const wxString& aName );

    /**
     * Return a property for a specific type.
     *
     * @param aType is the type identifier (obtained using TYPE_HASH()).
     * @param aProperty is the property name used during class registration.
     * @return Requested property or null pointer if requested property does not exist.
     */
    PROPERTY_BASE* GetProperty( TYPE_ID aType, const wxString& aProperty ) const;

    /**
     * Return all properties for a specific type.
     *
     * @param aType is the type identifier (obtained using TYPE_HASH()).
     * @return Vector storing all properties of the requested type.
     */
    const std::vector<PROPERTY_BASE*>& GetProperties( TYPE_ID aType ) const;

    const std::map<PROPERTY_BASE*, int>& GetDisplayOrder( TYPE_ID aType ) const;

    const std::vector<wxString>& GetGroupDisplayOrder( TYPE_ID aType ) const;

    /**
     * Cast a type to another type. Used for correct type-casting of types with
     * multi-inheritance. Requires registration of an appropriate converter (AddTypeCast).
     *
     * @param aSource is a pointer to the casted object.
     * @param aBase is aSource type identifier (obtained using TYPE_HASH()).
     * @param aTarget is the desired type identifier (obtained using TYPE_HASH()).
     * @return Properly casted pointer of aTarget type.     *
     *
     * @see AddTypeCast
     */
    const void* TypeCast( const void* aSource, TYPE_ID aBase, TYPE_ID aTarget ) const;

    void* TypeCast( void* aSource, TYPE_ID aBase, TYPE_ID aTarget ) const
    {
        return const_cast<void*>( TypeCast( (const void*) aSource, aBase, aTarget ) );
    }

    /**
     * Register a property.
     * Properties for a given item will be shown in the order they are added.
     * If a group name is supplied, the group will be created if it does not yet exists.
     * Groups will likewise be shown in the order they are added (so, groups first added by a base
     * class will appear before those of a child class).
     *
     * @param aProperty is the property to register.
     * @param aGroup is an optional grouping key for the property
     */
    PROPERTY_BASE& AddProperty( PROPERTY_BASE* aProperty, const wxString& aGroup = wxEmptyString );

    /**
     * Replace an existing property for a specific type.
     *
     * It is used to modify a property that has been inherited from a base class.
     * This method is used instead of AddProperty().
     *
     * @param aBase is the base class type the delivers the original property.
     * @param aName is the name of the replaced property.
     * @param aNew is the property replacing the inherited one.
     * @param aGroup is the group to set for the replaced property.
     */
    PROPERTY_BASE& ReplaceProperty( size_t aBase, const wxString& aName, PROPERTY_BASE* aNew,
                                    const wxString& aGroup = wxEmptyString );

    /**
     * Register a type converter. Required prior TypeCast() usage.
     *
     * @param aCast is the type converter to register.
     */
    void AddTypeCast( TYPE_CAST_BASE* aCast );

    /**
     * Declare an inheritance relationship between types.
     *
     * @param aBase is the base type identifier (obtained using TYPE_HASH()).
     * @param aDerived is the derived type identifier (obtained using TYPE_HASH()).
     */
    void InheritsAfter( TYPE_ID aDerived, TYPE_ID aBase );

    /**
     * Sets a base class property as masked in a derived class.  Masked properties are hidden from
     * the list of editable properties for this class.
     *
     * @param aDerived is the type to apply the mask for.
     * @param aBase is the type that aName belongs to.
     * @param aName is the name of a property on the base class.
     */
    void Mask( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName );

    /**
     * Sets an override availability functor for a base class property of a given derived class.
     *
     * @param aDerived is the type to apply the mask for.
     * @param aBase is the type that aName belongs to.
     * @param aName is the name of a property on the base class.
     * @param aFunc is the new availability functor to apply.
     */
    void OverrideAvailability( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName,
                               std::function<bool( INSPECTABLE* )> aFunc );

    /**
     * Sets an override writeability functor for a base class property of a given derived class.
     *
     * @param aDerived is the type to apply the mask for.
     * @param aBase is the type that aName belongs to.
     * @param aName is the name of a property on the base class.
     * @param aFunc is the new availability functor to apply.
     */
    void OverrideWriteability( TYPE_ID aDerived, TYPE_ID aBase, const wxString& aName,
                               std::function<bool( INSPECTABLE* )> aFunc );

    /**
     * Checks overriden availability and original availability of a property, returns false
     * if the property is unavailable in either case.
     *
     * TODO: This isn't the cleanest API, consider how to merge with PROPERTY_BASE::Available
     */
    bool IsAvailableFor( TYPE_ID aItemClass, PROPERTY_BASE* aProp, INSPECTABLE* aItem );

    /**
     * Checks overriden availability and original availability of a property, returns false
     * if the property is unavailable in either case.
     *
     * TODO: This isn't the cleanest API, consider how to merge with PROPERTY_BASE::Writeable
     */
    bool IsWriteableFor( TYPE_ID aItemClass, PROPERTY_BASE* aProp, INSPECTABLE* aItem );

    /**
     * Return true if aDerived is inherited from aBase.
     */
    bool IsOfType( TYPE_ID aDerived, TYPE_ID aBase ) const;

    /**
     * Rebuild the list of all registered properties. Needs to be called
     * once before GetProperty()/GetProperties() are used.
     */
    void Rebuild();

    struct CLASS_INFO
    {
        wxString name;
        TYPE_ID type;
        std::vector<PROPERTY_BASE*> properties;
    };

    typedef std::vector<CLASS_INFO> CLASSES_INFO;

    CLASSES_INFO GetAllClasses();

    /**
     * Callback to alert the notification system that a property has changed
     * @param aObject is the object whose property just changed
     * @param aProperty is the property that changed
     */
    void PropertyChanged( INSPECTABLE* aObject, PROPERTY_BASE* aProperty );

    /**
     * Registers a listener for the given type
     * @param aType is the type to add the listener for
     * @param aListenerFunc will be called every time a property on aType is changed
     */
    void RegisterListener( TYPE_ID aType, PROPERTY_LISTENER aListenerFunc )
    {
        m_listeners[aType].emplace_back( aListenerFunc );
    }

    void UnregisterListeners( TYPE_ID aType )
    {
        m_listeners[aType].clear();
    }

private:
    PROPERTY_MANAGER() :
            m_dirty( false ),
            m_managedCommit( nullptr )
    {
    }

    friend class PROPERTY_COMMIT_HANDLER;

    ///< Structure holding type meta-data
    struct CLASS_DESC
    {
        CLASS_DESC( TYPE_ID aId )
            : m_id( aId )
        {
            m_groupDisplayOrder.emplace_back( wxEmptyString );
            m_groups.insert( wxEmptyString );
        }

        ///< Unique type identifier (obtained using TYPE_HASH)
        const TYPE_ID m_id;

        ///< Types after which this type inherits
        std::vector<std::reference_wrapper<CLASS_DESC>> m_bases;

        ///< Properties unique to this type (i.e. not inherited)
        std::map<wxString, std::unique_ptr<PROPERTY_BASE>> m_ownProperties;

        ///< Type converters available for this type
        std::map<TYPE_ID, std::unique_ptr<TYPE_CAST_BASE>> m_typeCasts;

        ///< Properties from bases that should be masked (hidden) on this subclass
        std::set<std::pair<size_t, wxString>> m_maskedBaseProperties;

        ///< Overrides for base class property availabilities
        std::map<std::pair<size_t, wxString>, std::function<bool( INSPECTABLE* )>> m_availabilityOverrides;

        ///< Overrides for base class property writeable status
        std::map<std::pair<size_t, wxString>, std::function<bool( INSPECTABLE* )>> m_writeabilityOverrides;

        ///< All properties (both unique to the type and inherited)
        std::vector<PROPERTY_BASE*> m_allProperties;

        ///< Compiled display order for all properties
        std::map<PROPERTY_BASE*, int> m_displayOrder;

        ///< List of property groups provided by this class in display order
        std::vector<wxString> m_groupDisplayOrder;

        ///< Non-owning list of classes's direct properties in display order
        std::vector<PROPERTY_BASE*> m_ownDisplayOrder;

        ///< The property groups provided by this class
        std::set<wxString> m_groups;

        ///< Replaced properties (TYPE_ID / name)
        std::set<std::pair<size_t, wxString>> m_replaced;

        ///< Recreates the list of properties
        void rebuild();

        ///< Traverses the class inheritance hierarchy bottom-to-top, gathering
        ///< all properties available to a type
        void collectPropsRecur( std::vector<PROPERTY_BASE*>& aResult,
                                std::set<std::pair<size_t, wxString>>& aReplaced,
                                std::map<PROPERTY_BASE*, int>& aDisplayOrder,
                                std::set<std::pair<size_t, wxString>>& aMasked ) const;
    };

    ///< Returns metadata for a specific type
    CLASS_DESC& getClass( TYPE_ID aTypeId );

    std::unordered_map<TYPE_ID, wxString> m_classNames;

    ///< Map of all available types
    std::unordered_map<TYPE_ID, CLASS_DESC> m_classes;

    /// Flag indicating that the list of properties needs to be rebuild (RebuildProperties())
    bool m_dirty;

    std::map<TYPE_ID, std::vector<PROPERTY_LISTENER>> m_listeners;

    COMMIT* m_managedCommit;
};


///< Helper macro to map type hashes to names
#define REGISTER_TYPE(x) PROPERTY_MANAGER::Instance().RegisterType(TYPE_HASH(x), TYPE_NAME(x))

#endif /* PROPERTY_MGR_H */
