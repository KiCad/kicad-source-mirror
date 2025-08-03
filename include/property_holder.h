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
    /**
     * @brief Set a property with the given key and value.
     * @tparam T The type of the value to store
     * @param key The property key
     * @param value The value to store
     */
    template<typename T>
    void SetProperty(const std::string& key, T&& value)
    {
        m_properties[key] = std::forward<T>(value);
    }

    /**
     * @brief Get a property value with type checking.
     * @tparam T The expected type of the property
     * @param key The property key
     * @return std::optional<T> containing the value if found and type matches, nullopt otherwise
     */
    template<typename T>
    std::optional<T> GetProperty(const std::string& key) const
    {
        auto it = m_properties.find(key);
        if (it == m_properties.end()) {
            return std::nullopt;
        }

        try {
            return std::any_cast<T>(it->second);
        }
        catch (const std::bad_any_cast&) {
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
    T GetPropertyOr(const std::string& key, T&& defaultValue) const
    {
        if (auto value = GetProperty<T>(key)) {
            return *value;
        }
        return std::forward<T>(defaultValue);
    }

    /**
     * @brief Check if a property exists.
     * @param key The property key
     * @return true if the property exists, false otherwise
     */
    bool HasProperty(const std::string& key) const
    {
        return m_properties.find(key) != m_properties.end();
    }

    /**
     * @brief Remove a property.
     * @param key The property key
     * @return true if the property was removed, false if it didn't exist
     */
    bool RemoveProperty(const std::string& key)
    {
        return m_properties.erase(key) > 0;
    }

    /**
     * @brief Clear all properties.
     */
    void Clear()
    {
        m_properties.clear();
    }

    /**
     * @brief Get the number of stored properties.
     * @return The number of properties
     */
    size_t Size() const
    {
        return m_properties.size();
    }

    /**
     * @brief Check if there are no properties stored.
     * @return true if empty, false otherwise
     */
    bool Empty() const
    {
        return m_properties.empty();
    }

    /**
     * @brief Get all property keys.
     * @return A vector of all property keys
     */
    std::vector<std::string> GetKeys() const
    {
        std::vector<std::string> keys;
        keys.reserve(m_properties.size());
        for (const auto& [key, value] : m_properties) {
            keys.push_back(key);
        }
        return keys;
    }

    /**
     * @brief Get the type information for a property.
     * @param key The property key
     * @return std::optional<std::type_info> containing type info if property exists
     */
    std::optional<std::reference_wrapper<const std::type_info>> GetPropertyType(const std::string& key) const
    {
        auto it = m_properties.find(key);
        if (it == m_properties.end()) {
            return std::nullopt;
        }
        return std::cref(it->second.type());
    }

    /**
     * @brief Check if a property exists and has the expected type.
     * @tparam T The expected type
     * @param key The property key
     * @return true if property exists and has type T
     */
    template<typename T>
    bool HasPropertyOfType(const std::string& key) const
    {
        auto it = m_properties.find(key);
        if (it == m_properties.end()) {
            return false;
        }
        return it->second.type() == typeid(T);
    }

    /**
     * @brief Type-safe property setter that only accepts valid property types
     */
    template<PropertyValueType T>
    void SetTypedProperty(const std::string& key, T&& value)
    {
        SetProperty(key, std::forward<T>(value));
    }

private:
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
    void SetProperty(const std::string& key, T&& value)
    {
        m_propertyHolder.SetProperty(key, std::forward<T>(value));
    }

    template<typename T>
    std::optional<T> GetProperty(const std::string& key) const
    {
        return m_propertyHolder.GetProperty<T>(key);
    }

    template<typename T>
    T GetPropertyOr(const std::string& key, T&& defaultValue) const
    {
        return m_propertyHolder.GetPropertyOr(key, std::forward<T>(defaultValue));
    }

    bool HasProperty(const std::string& key) const
    {
        return m_propertyHolder.HasProperty(key);
    }

    bool RemoveProperty(const std::string& key)
    {
        return m_propertyHolder.RemoveProperty(key);
    }

    template<typename T>
    bool HasPropertyOfType(const std::string& key) const
    {
        return m_propertyHolder.HasPropertyOfType<T>(key);
    }

private:
    PROPERTY_HOLDER m_propertyHolder;
};

#endif // PROPERTY_HOLDER_H