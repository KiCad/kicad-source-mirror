/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/string.h>

#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>
#include <eda_units.h>


#include <common.h> // Needed for stl hash extensions

class PROPERTY_BASE;
class TYPE_CAST_BASE;

///< Unique type identifier
using TYPE_ID = size_t;

using PROPERTY_LIST = std::vector<PROPERTY_BASE*>;

using PROPERTY_SET = std::set<std::pair<size_t, wxString>>;

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
     * Return name of a type.
     *
     * @param aType is the type identifier (obtained using TYPE_HASH()).
     * @return Name of the type or empty string, if not available.
     */
    const wxString& ResolveType( TYPE_ID aType ) const;

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
    const PROPERTY_LIST& GetProperties( TYPE_ID aType ) const;

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
     *
     * @param aProperty is the property to register.
     */
    void AddProperty( PROPERTY_BASE* aProperty );

    /**
     * Replace an existing property for a specific type.
     *
     * It is used to modify a property that has been inherited from a base class.
     * This method is used instead of AddProperty().
     *
     * @param aBase is the base class type the delivers the original property.
     * @param aName is the name of the replaced property.
     * @param aNew is the property replacing the inherited one.
     */
    void ReplaceProperty( size_t aBase, const wxString& aName, PROPERTY_BASE* aNew );

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
     * Return true if aDerived is inherited from aBase.
     */
    bool IsOfType( TYPE_ID aDerived, TYPE_ID aBase ) const;

    EDA_UNITS GetUnits() const
    {
        return m_units;
    }

    void SetUnits( EDA_UNITS aUnits )
    {
        m_units = aUnits;
    }

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

    std::vector<TYPE_ID> GetMatchingClasses( PROPERTY_BASE* aProperty );

private:
    PROPERTY_MANAGER()
        : m_dirty( false ), m_units( EDA_UNITS::MILLIMETRES )
    {
    }

    ///< Structure holding type meta-data
    struct CLASS_DESC
    {
        CLASS_DESC( TYPE_ID aId )
            : m_id( aId )
        {
        }

        ///< Unique type identifier (obtained using TYPE_HASH)
        const TYPE_ID m_id;

        ///< Types after which this type inherits
        std::vector<std::reference_wrapper<CLASS_DESC>> m_bases;

        ///< Properties unique to this type (i.e. not inherited)
        std::map<wxString, std::unique_ptr<PROPERTY_BASE>> m_ownProperties;

        ///< Type converters available for this type
        std::map<TYPE_ID, std::unique_ptr<TYPE_CAST_BASE>> m_typeCasts;

        ///< All properties (both unique to the type and inherited)
        std::vector<PROPERTY_BASE*> m_allProperties;

        ///< Replaced properties (TYPE_ID / name)
        PROPERTY_SET m_replaced;

        ///< Recreates the list of properties
        void rebuild();

        ///< Traverses the class inheritance hierarchy bottom-to-top, gathering
        ///< all properties available to a type
        void collectPropsRecur( PROPERTY_LIST& aResult, PROPERTY_SET& aReplaced ) const;
    };

    ///< Returns metadata for a specific type
    CLASS_DESC& getClass( TYPE_ID aTypeId );

    std::unordered_map<TYPE_ID, wxString> m_classNames;

    ///< Map of all available types
    std::unordered_map<TYPE_ID, CLASS_DESC> m_classes;

    /// Flag indicating that the list of properties needs to be rebuild (RebuildProperties())
    bool m_dirty;

    EDA_UNITS m_units;
};


///< Helper macro to map type hashes to names
#define REGISTER_TYPE(x) PROPERTY_MANAGER::Instance().RegisterType(TYPE_HASH(x), TYPE_NAME(x))

#endif /* PROPERTY_MGR_H */
