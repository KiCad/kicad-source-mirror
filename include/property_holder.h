/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PROPERTY_HOLDER_H
#define PROPERTY_HOLDER_H

#include <any>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <optional>
#include <concepts>
#include <cstdint>


/**
 * @brief A C++20 property system for arbitrary key-value storage with type safety.
 *
 * This class provides a flexible way to store arbitrary typed properties using
 * string keys. It supports type-safe getters/setters with automatic type checking
 * and optional default values.
 *
 * Example usage:
 * @code
 * PROPERTY_HOLDER props;
 *
 * // Setting properties
 * props.SetProperty("persist", true);
 * props.SetProperty("max_width", 500);
 * props.SetProperty("label", std::string("My Label"));
 *
 * // Getting properties with type checking
 * if (auto persist = props.GetProperty<bool>("persist")) {
 *     std::cout << "Persist: " << *persist << std::endl;
 * }
 *
 * // Getting with default value
 * bool shouldPersist = props.GetPropertyOr("persist", false);
 * int width = props.GetPropertyOr("max_width", 100);
 *
 * // Checking existence
 * if (props.HasProperty("label")) {
 *     // Property exists
 * }
 *
 * // Removing properties
 * props.RemoveProperty("label");
 * props.Clear();
 * @endcode
 */


/**
 * @brief Concept for types that can be used as property values
 */
template<typename T>
concept PropertyValueType = std::copy_constructible<T> && std::destructible<T>;

class PROPERTY_HOLDER
{
public:
    /// Magic value for memory validation (ASCII: "PROP" + "HLDR")
    static constexpr uint64_t MAGIC_VALUE = 0x50524F5048444C52ULL;

    /**
     * @brief Default constructor - initializes magic value
     */
    PROPERTY_HOLDER() :
            m_magic( MAGIC_VALUE )
    {
    }

    /**
     * @brief Copy constructor - maintains magic value
     */
    PROPERTY_HOLDER( const PROPERTY_HOLDER& other ) :
            m_magic( MAGIC_VALUE ),
            m_properties( other.m_properties )
    {
    }

    /**
     * @brief Move constructor - maintains magic value
     */
    PROPERTY_HOLDER( PROPERTY_HOLDER&& other ) noexcept :
            m_magic( MAGIC_VALUE ),
            m_properties( std::move( other.m_properties ) )
    {
    }

    /**
     * @brief Copy assignment operator
     */
    PROPERTY_HOLDER& operator=( const PROPERTY_HOLDER& other )
    {
        if( this != &other )
            m_properties = other.m_properties;

        return *this;
    }

    /**
     * @brief Move assignment operator
     */
    PROPERTY_HOLDER& operator=( PROPERTY_HOLDER&& other ) noexcept
    {
        if( this != &other )
            m_properties = std::move( other.m_properties );

        return *this;
    }

    /**
     * @brief Destructor - clears magic value to detect use-after-free
     */
    ~PROPERTY_HOLDER()
    {
        m_magic = 0xDEADBEEFDEADBEEFULL; // Clear magic to detect use-after-free
    }

    /**
     * @brief Check if this instance has a valid magic value
     * @return true if magic value is valid, false otherwise
     */
    bool IsValid() const noexcept { return m_magic == MAGIC_VALUE; }

    /**
     * @brief Safely cast a void pointer to PROPERTY_HOLDER*
     * @param ptr Pointer to validate and cast
     * @return PROPERTY_HOLDER* if valid, nullptr otherwise
     */
    static PROPERTY_HOLDER* SafeCast( void* aPtr ) noexcept
    {
        if( !aPtr )
            return nullptr;

        try
        {
            PROPERTY_HOLDER* aCandidate = reinterpret_cast<PROPERTY_HOLDER*>( aPtr );
            if( aCandidate->m_magic == MAGIC_VALUE )
            {
                return aCandidate;
            }
        }
        catch( ... )
        {
            // Any exception means invalid memory
        }
        return nullptr;
    }

    /**
     * @brief Safely cast a const void pointer to const PROPERTY_HOLDER*
     * @param ptr Pointer to validate and cast
     * @return const PROPERTY_HOLDER* if valid, nullptr otherwise
     */
    static const PROPERTY_HOLDER* SafeCast( const void* aPtr ) noexcept
    {
        if( !aPtr )
            return nullptr;

        try
        {
            const PROPERTY_HOLDER* aCandidate = reinterpret_cast<const PROPERTY_HOLDER*>( aPtr );
            if( aCandidate->m_magic == MAGIC_VALUE )
            {
                return aCandidate;
            }
        }
        catch( ... )
        {
            // Any exception means invalid memory
        }
        return nullptr;
    }

    /**
     * @brief Safely delete a PROPERTY_HOLDER from client data
     * @param ptr Pointer from client data to validate and delete
     * @return true if successfully deleted, false if invalid pointer
     */
    static bool SafeDelete( void* aPtr ) noexcept
    {
        PROPERTY_HOLDER* aHolder = SafeCast( aPtr );

        if( aHolder )
        {
            delete aHolder;
            return true;
        }

        return false;
    }

    static bool SafeDelete( PROPERTY_HOLDER* aHolder ) noexcept
    {
        if( aHolder )
        {
            delete aHolder;
            return true;
        }

        return false;
    }

    /**
     * @brief Set a property with the given key and value.
     * @tparam T The type of the value to store
     * @param key The property key
     * @param value The value to store
     * @return true if property was set, false if object is invalid
     */
    template <typename T>
    bool SetProperty( const std::string& aKey, T&& aValue )
    {
        if( !IsValid() )
            return false;

        m_properties[aKey] = std::forward<T>( aValue );
        return true;
    }

    /**
     * @brief Get a property value with type checking.
     * @tparam T The expected type of the property
     * @param key The property key
     * @return std::optional<T> containing the value if found and type matches, nullopt otherwise
     */
    template <typename T>
    std::optional<T> GetProperty( const std::string& aKey ) const
    {
        if( !IsValid() )
            return std::nullopt;

        auto it = m_properties.find( aKey );
        if( it == m_properties.end() )
        {
            return std::nullopt;
        }

        try
        {
            return std::any_cast<T>( it->second );
        }
        catch( const std::bad_any_cast& )
        {
            return std::nullopt;
        }
    }

    /**
     * @brief Get a property value with a default fallback.
     * @tparam T The expected type of the property
     * @param key The property key
     * @param defaultValue The value to return if property doesn't exist or type mismatch
     * @return The property value or the default value
     */
    template<typename T>
    T GetPropertyOr(const std::string& aKey, T&& aDefaultValue) const
    {
        if( auto aValue = GetProperty<T>( aKey ) )
            return *aValue;

        return std::forward<T>(aDefaultValue);
    }

    /**
     * @brief Check if a property exists.
     * @param key The property key
     * @return true if the property exists and object is valid, false otherwise
     */
    bool HasProperty( const std::string& aKey ) const
    {
        if( !IsValid() )
            return false;

        return m_properties.find( aKey ) != m_properties.end();
    }

    /**
     * @brief Remove a property.
     * @param key The property key
     * @return true if the property was removed, false if it didn't exist or object is invalid
     */
    bool RemoveProperty( const std::string& aKey )
    {
        if( !IsValid() )
            return false;

        return m_properties.erase( aKey ) > 0;
    }

    /**
     * @brief Clear all properties.
     * @return true if cleared successfully, false if object is invalid
     */
    bool Clear()
    {
        if( !IsValid() )
            return false;

        m_properties.clear();
        return true;
    }

    /**
     * @brief Get the number of stored properties.
     * @return The number of properties, or 0 if object is invalid
     */
    size_t Size() const
    {
        if( !IsValid() )
            return 0;
        return m_properties.size();
    }

    /**
     * @brief Check if there are no properties stored.
     * @return true if empty or object is invalid, false otherwise
     */
    bool Empty() const
    {
        if( !IsValid() )
            return true;
        return m_properties.empty();
    }

    /**
     * @brief Get all property keys.
     * @return A vector of all property keys (empty if object is invalid)
     */
    std::vector<std::string> GetKeys() const
    {
        if( !IsValid() )
            return {};

        std::vector<std::string> keys;
        keys.reserve( m_properties.size() );

        for( const auto& [key, value] : m_properties )
            keys.push_back( key );

        return keys;
    }

    /**
     * @brief Get the type information for a property.
     * @param key The property key
     * @return std::optional<std::type_info> containing type info if property exists and object is valid
     */
    std::optional<std::reference_wrapper<const std::type_info>> GetPropertyType( const std::string& aKey ) const
    {
        if( !IsValid() )
            return std::nullopt;

        auto it = m_properties.find( aKey );

        if( it == m_properties.end() )
            return std::nullopt;

        return std::cref( it->second.type() );
    }

    /**
     * @brief Check if a property exists and has the expected type.
     * @tparam T The expected type
     * @param key The property key
     * @return true if property exists, has type T, and object is valid
     */
    template <typename T>
    bool HasPropertyOfType( const std::string& aKey ) const
    {
        if( !IsValid() )
            return false;

        auto it = m_properties.find( aKey );

        if( it == m_properties.end() )
            return false;

        return it->second.type() == typeid( T );
    }

    /**
     * @brief Type-safe property setter that only accepts valid property types
     */
    template<PropertyValueType T>
    bool SetTypedProperty(const std::string& aKey, T&& aValue)
    {
        return SetProperty(aKey, std::forward<T>(aValue));
    }

private:
    uint64_t m_magic; ///< Magic value for memory validation
    /**
     * @brief Internal storage for properties using string keys and any values.
     *
     * This uses std::any to allow storing any type of value, with type safety
     * provided by the GetProperty and SetProperty methods.
     */
    std::unordered_map<std::string, std::any> m_properties;
};

/**
 * @brief Mixin class to add property support to any class.
 *
 * Example usage:
 * @code
 * class MyWidget : public SomeBaseClass, public PROPERTY_MIXIN
 * {
 * public:
 *     MyWidget() {
 *         SetProperty("default_width", 200);
 *     }
 * };
 *
 * MyWidget widget;
 * widget.SetProperty("persist", false);
 * bool persist = widget.GetPropertyOr("persist", true);
 * @endcode
 */
class PROPERTY_MIXIN
{
public:
    /**
     * @brief Get the property holder for this object.
     * @return Reference to the property holder
     */
    PROPERTY_HOLDER& GetPropertyHolder() { return m_propertyHolder; }
    const PROPERTY_HOLDER& GetPropertyHolder() const { return m_propertyHolder; }

    // Convenience methods that delegate to the property holder
    template<typename T>
    void SetProperty(const std::string& aKey, T&& aValue)
    {
        m_propertyHolder.SetProperty(aKey, std::forward<T>(aValue));
    }

    template<typename T>
    std::optional<T> GetProperty(const std::string& aKey) const
    {
        return m_propertyHolder.GetProperty<T>(aKey);
    }

    template<typename T>
    T GetPropertyOr(const std::string& aKey, T&& aDefaultValue) const
    {
        return m_propertyHolder.GetPropertyOr(aKey, std::forward<T>(aDefaultValue));
    }

    bool HasProperty(const std::string& aKey) const
    {
        return m_propertyHolder.HasProperty(aKey);
    }

    bool RemoveProperty(const std::string& aKey)
    {
        return m_propertyHolder.RemoveProperty(aKey);
    }

    template<typename T>
    bool HasPropertyOfType(const std::string& aKey) const
    {
        return m_propertyHolder.HasPropertyOfType<T>(aKey);
    }

private:
    PROPERTY_HOLDER m_propertyHolder;
};

#endif // PROPERTY_HOLDER_H