/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <set>
#include <string>
#include <utility>
#include <math/util.h>

#include <optional>
#include <gal/color4d.h>
#include <settings/json_settings.h>
#include <settings/grid_settings.h>
#include <kicommon.h>

class KICOMMON_API PARAM_BASE
{
public:
    PARAM_BASE( std::string aJsonPath, bool aReadOnly ) :
            m_path( std::move( aJsonPath ) ),
            m_readOnly( aReadOnly ),
            m_clearUnknownKeys( false )
    {}

    virtual ~PARAM_BASE() = default;

    /**
     * Loads the value of this parameter from JSON to the underlying storage
     * @param aSettings is the JSON_SETTINGS object to load from.
     * @param aResetIfMissing if true will set the parameter to its default value if load fails
     */
    virtual void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const = 0;

    /**
     * Stores the value of this parameter to the given JSON_SETTINGS object
     * @param aSettings is the JSON_SETTINGS object to store into.
     */
    virtual void Store( JSON_SETTINGS* aSettings ) const = 0;

    virtual void SetDefault() = 0;

    /**
     * Checks whether the parameter in memory matches the one in a given JSON file
     * @param aSettings is a JSON_SETTINGS to check the JSON file contents of
     * @return true if the parameter in memory matches its value in the file
     */
    virtual bool MatchesFile( const JSON_SETTINGS& aSettings ) const = 0;

    /**
     * @return the path name of the parameter used to store it in the json file
     * mainly useful in error messages
     */
    const std::string& GetJsonPath() const { return m_path; }

    /**
     * @return true if keys should be cleared from source file rather than merged.  Useful for
     *         things such as text variables that are semantically an array but stored as a map.
     */
    bool ClearUnknownKeys() const { return m_clearUnknownKeys; }

    void SetClearUnknownKeys( bool aSet = true ) { m_clearUnknownKeys = aSet; }

protected:
    std::string m_path;               ///< Address of the param in the json files
    bool        m_readOnly;           ///< Indicates param pointer should never be overwritten
    bool        m_clearUnknownKeys;   ///< Keys should be cleared from source rather than merged.
                                      ///<   This is useful for things that are semantically an
                                      ///<   array but stored as a map, such as textVars.
};


template<typename ValueType>
class PARAM : public PARAM_BASE
{
public:
    PARAM( const std::string& aJsonPath, ValueType* aPtr, const ValueType& aDefault,
           bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_min(),
            m_max(),
            m_use_minmax( false ),
            m_ptr( aPtr ),
            m_default( aDefault )
    { }

    PARAM( const std::string& aJsonPath, ValueType* aPtr, const ValueType& aDefault,
           const ValueType& aMin, const ValueType& aMax, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_min( aMin ),
            m_max( aMax ),
            m_use_minmax( true ),
            m_ptr( aPtr ),
            m_default( aDefault )
    { }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::optional<ValueType> optval = aSettings.Get<ValueType>( m_path ) )
        {
            ValueType val = *optval;

            if( m_use_minmax )
            {
                if( m_max < val || val < m_min )
                    val = m_default;
            }

            *m_ptr = val;
        }
        else if( aResetIfMissing )
        {
            *m_ptr = m_default;
        }
    }

    void Store( JSON_SETTINGS* aSettings ) const override
    {
        aSettings->Set<ValueType>( m_path, *m_ptr );
    }

    ValueType GetDefault() const
    {
        return m_default;
    }

    void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<ValueType> optval = aSettings.Get<ValueType>( m_path ) )
            return *optval == *m_ptr;

        return false;
    }

private:
    ValueType  m_min;
    ValueType  m_max;
    bool       m_use_minmax;

protected:
    ValueType* m_ptr;
    ValueType  m_default;
};

/**
 * Stores a path as a string with directory separators normalized to unix-style
 */
class KICOMMON_API PARAM_PATH : public PARAM<wxString>
{
public:
    PARAM_PATH( const std::string& aJsonPath, wxString* aPtr, const wxString& aDefault,
                bool aReadOnly = false ) :
            PARAM( aJsonPath, aPtr, aDefault, aReadOnly )
    { }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        PARAM::Load( aSettings, aResetIfMissing );

        *m_ptr = fromFileFormat( *m_ptr );
    }

    void Store( JSON_SETTINGS* aSettings ) const override
    {
        aSettings->Set<wxString>( m_path, toFileFormat( *m_ptr ) );
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<wxString> optval = aSettings.Get<wxString>( m_path ) )
            return fromFileFormat( *optval ) == *m_ptr;

        return false;
    }

private:
    wxString toFileFormat( const wxString& aString ) const
    {
        wxString ret = aString;
        ret.Replace( wxT( "\\" ), wxT( "/" ) );
        return ret;
    }

    wxString fromFileFormat( const wxString& aString ) const
    {
        wxString ret = aString;
#ifdef __WINDOWS__
        if( !ret.Contains( wxT( "://" ) ) )
            ret.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
        return ret;
    }
};

/**
 * Stores an enum as an integer
 */
template<typename EnumType>
class PARAM_ENUM : public PARAM_BASE
{
public:
    PARAM_ENUM( const std::string& aJsonPath, EnumType* aPtr, EnumType aDefault,
                EnumType aMin, EnumType aMax, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_min( aMin ),
            m_max( aMax ),
            m_default( aDefault )
    {
    }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::optional<int> val = aSettings.Get<int>( m_path ) )
        {
            if( *val >= static_cast<int>( m_min ) && *val <= static_cast<int>( m_max ) )
                *m_ptr = static_cast<EnumType>( *val );
            else if( aResetIfMissing )
                *m_ptr = m_default;

        }
        else if( aResetIfMissing )
        {
            *m_ptr = m_default;
        }
    }

    void Store( JSON_SETTINGS* aSettings ) const override
    {
        aSettings->Set<int>( m_path, static_cast<int>( *m_ptr ) );
    }

    EnumType GetDefault() const
    {
        return m_default;
    }

    void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<int> val = aSettings.Get<int>( m_path ) )
            return *val == static_cast<int>( *m_ptr );

        return false;
    }

private:
    EnumType* m_ptr;
    EnumType  m_min;
    EnumType  m_max;
    EnumType  m_default;
};

/**
 * Like a normal param, but with custom getter and setter functions
 * @tparam ValueType is the value to store
 */
template<typename ValueType>
class PARAM_LAMBDA : public PARAM_BASE
{
public:
    PARAM_LAMBDA( const std::string& aJsonPath,  std::function<ValueType()> aGetter,
                  std::function<void( ValueType )> aSetter, ValueType aDefault,
                  bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_default( std::move( aDefault ) ),
            m_getter( std::move( aGetter ) ),
            m_setter( std::move( aSetter ) )
    {
    }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::is_same<ValueType, nlohmann::json>::value )
        {
            if( std::optional<nlohmann::json> optval = aSettings.GetJson( m_path ) )
                m_setter( *optval );
            else
                m_setter( m_default );
        }
        else
        {
            if( std::optional<ValueType> optval = aSettings.Get<ValueType>( m_path ) )
                m_setter( *optval );
            else
                m_setter( m_default );
        }
    }

    void Store( JSON_SETTINGS* aSettings ) const override
    {
        try
        {
            aSettings->Set<ValueType>( m_path, m_getter() );
        }
        catch( ... )
        {
        }
    }

    ValueType GetDefault() const
    {
        return m_default;
    }

    void SetDefault() override
    {
        m_setter( m_default );
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::is_same<ValueType, nlohmann::json>::value )
        {
            if( std::optional<nlohmann::json> optval = aSettings.GetJson( m_path ) )
                return *optval == m_getter();
        }
        else
        {
            if( std::optional<ValueType> optval = aSettings.Get<ValueType>( m_path ) )
                return *optval == m_getter();
        }

        // Not in file
        return false;
    }

private:
    ValueType                        m_default;
    std::function<ValueType()>       m_getter;
    std::function<void( ValueType )> m_setter;
};

#ifdef __WINDOWS__
template class KICOMMON_API PARAM_LAMBDA<bool>;
template class KICOMMON_API PARAM_LAMBDA<int>;
template class KICOMMON_API PARAM_LAMBDA<nlohmann::json>;
template class KICOMMON_API PARAM_LAMBDA<std::string>;
#else
extern template class APIVISIBLE PARAM_LAMBDA<bool>;
extern template class APIVISIBLE PARAM_LAMBDA<int>;
extern template class APIVISIBLE PARAM_LAMBDA<nlohmann::json>;
extern template class APIVISIBLE PARAM_LAMBDA<std::string>;
#endif

/**
 * Represents a parameter that has a scaling factor between the value in the file and the
 * value used internally (i.e. the value pointer).  This basically only makes sense to use
 * with int or double as ValueType.
 * @tparam ValueType is the internal type: the file always stores a double.
 */
template<typename ValueType>
class PARAM_SCALED: public PARAM_BASE
{
public:
    PARAM_SCALED( const std::string& aJsonPath, ValueType* aPtr, ValueType aDefault,
                  double aScale = 1.0, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_default( aDefault ),
            m_min(),
            m_max(),
            m_use_minmax( false ),
            m_scale( aScale ),
            m_invScale( 1.0 / aScale )
    { }

    PARAM_SCALED( const std::string& aJsonPath, ValueType* aPtr, ValueType aDefault,
                  ValueType aMin, ValueType aMax, double aScale = 1.0, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_default( aDefault ),
            m_min( aMin ),
            m_max( aMax ),
            m_use_minmax( true ),
            m_scale( aScale ),
            m_invScale( 1.0 / aScale )
    { }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        double dval = m_default / m_invScale;

        if( std::optional<double> optval = aSettings.Get<double>( m_path ) )
            dval = *optval;
        else if( !aResetIfMissing )
            return;

        ValueType val = KiROUND<double, ValueType>( dval * m_invScale );

        if( m_use_minmax )
        {
            if( val > m_max || val < m_min )
                val = m_default;
        }

        *m_ptr = val;
    }

    void Store( JSON_SETTINGS* aSettings) const override
    {
        aSettings->Set<double>( m_path, *m_ptr / m_invScale );
    }

    ValueType GetDefault() const
    {
        return m_default;
    }

    virtual void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<double> optval = aSettings.Get<double>( m_path ) )
            return *optval == ( *m_ptr / m_invScale );

        return false;
    }

private:
    ValueType* m_ptr;
    ValueType  m_default;
    ValueType  m_min;
    ValueType  m_max;
    bool       m_use_minmax;
    double     m_scale;
    double     m_invScale;
};

template<typename Type>
class PARAM_LIST : public PARAM_BASE
{
public:
    PARAM_LIST( const std::string& aJsonPath, std::vector<Type>* aPtr,
                std::initializer_list<Type> aDefault, bool aResetIfEmpty = false ) :
            PARAM_BASE( aJsonPath, false ),
            m_ptr( aPtr ),
            m_default( aDefault ),
            m_resetIfEmpty( aResetIfEmpty )
    { }

    PARAM_LIST( const std::string& aJsonPath, std::vector<Type>* aPtr,
                std::vector<Type> aDefault, bool aResetIfEmpty = false ) :
            PARAM_BASE( aJsonPath, false ),
            m_ptr( aPtr ),
            m_default( std::move( aDefault ) ),
            m_resetIfEmpty( aResetIfEmpty )
    { }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
        {
            std::vector<Type> val;

            if( js->is_array() )
            {
                for( const auto& el : js->items() )
                    val.push_back( el.value().get<Type>() );
            }

            if( val.empty() && m_resetIfEmpty )
                *m_ptr = m_default;
            else
                *m_ptr = val;
        }
        else if( aResetIfMissing )
        {
            *m_ptr = m_default;
        }
    }

    void Store( JSON_SETTINGS* aSettings ) const override
    {
        nlohmann::json js = nlohmann::json::array();

        for( const auto& el : *m_ptr )
            js.push_back( el );

        aSettings->Set<nlohmann::json>( m_path, js );
    }

    void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
        {
            if( js->is_array() )
            {
                std::vector<Type> val;

                for( const auto& el : js->items() )
                {
                    try
                    {
                        val.emplace_back( el.value().get<Type>() );
                    }
                    catch( ... )
                    {
                        // Probably typecast didn't work; skip this element
                    }
                }

                return val == *m_ptr;
            }
        }

        return false;
    }

protected:
    std::vector<Type>* m_ptr;
    std::vector<Type>  m_default;
    bool               m_resetIfEmpty;
};


#ifdef __WINDOWS__
template class KICOMMON_API PARAM_LIST<bool>;
template class KICOMMON_API PARAM_LIST<int>;
template class KICOMMON_API PARAM_LIST<double>;
template class KICOMMON_API PARAM_LIST<KIGFX::COLOR4D>;
template class KICOMMON_API PARAM_LIST<GRID>;
template class KICOMMON_API PARAM_LIST<wxString>;
#else
extern template class APIVISIBLE PARAM_LIST<bool>;
extern template class APIVISIBLE PARAM_LIST<int>;
extern template class APIVISIBLE PARAM_LIST<double>;
extern template class APIVISIBLE PARAM_LIST<KIGFX::COLOR4D>;
extern template class APIVISIBLE PARAM_LIST<GRID>;
extern template class APIVISIBLE PARAM_LIST<wxString>;
#endif


template<typename Type>
class PARAM_SET : public PARAM_BASE
{
public:
    PARAM_SET( const std::string& aJsonPath, std::set<Type>* aPtr,
               std::initializer_list<Type> aDefault, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_default( aDefault )
    { }

    PARAM_SET( const std::string& aJsonPath, std::set<Type>* aPtr,
               std::set<Type> aDefault, bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_default( aDefault )
    { }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
        {
            std::set<Type> val;

            if( js->is_array() )
            {
                for( const auto& el : js->items() )
                    val.insert( el.value().get<Type>() );
            }

            *m_ptr = val;
        }
        else if( aResetIfMissing )
            *m_ptr = m_default;
    }

    void Store( JSON_SETTINGS* aSettings) const override
    {
        nlohmann::json js = nlohmann::json::array();

        for( const auto& el : *m_ptr )
            js.push_back( el );

        aSettings->Set<nlohmann::json>( m_path, js );
    }


    void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
        {
            if( js->is_array() )
            {
                std::set<Type> val;

                for( const auto& el : js->items() )
                    val.insert( el.value().get<Type>() );

                return val == *m_ptr;
            }
        }

        return false;
    }

protected:
    std::set<Type>* m_ptr;
    std::set<Type>  m_default;
};

#ifdef __WINDOWS__
template class KICOMMON_API PARAM_SET<wxString>;
#else
extern template class APIVISIBLE PARAM_SET<wxString>;
#endif

/**
 * Represents a list of strings holding directory paths.
 * Normalizes paths to unix directory separator style in the file.
 */
class KICOMMON_API PARAM_PATH_LIST : public PARAM_LIST<wxString>
{
public:
    PARAM_PATH_LIST( const std::string& aJsonPath, std::vector<wxString>* aPtr,
                     std::initializer_list<wxString> aDefault ) :
            PARAM_LIST( aJsonPath, aPtr, aDefault )
    { }

    PARAM_PATH_LIST( const std::string& aJsonPath, std::vector<wxString>* aPtr,
                     std::vector<wxString> aDefault ) :
            PARAM_LIST( aJsonPath, aPtr, aDefault )
    { }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        PARAM_LIST::Load( aSettings, aResetIfMissing );

        for( size_t i = 0; i < m_ptr->size(); i++ )
            ( *m_ptr )[i] = fromFileFormat( ( *m_ptr )[i] );
    }

    void Store( JSON_SETTINGS* aSettings) const override;

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override;

private:
    wxString toFileFormat( const wxString& aString ) const
    {
        wxString ret = aString;
        ret.Replace( wxT( "\\" ), wxT( "/" ) );
        return ret;
    }

    wxString fromFileFormat( const wxString& aString ) const
    {
        wxString ret = aString;
#ifdef __WINDOWS__
        ret.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
        return ret;
    }
};

/**
 * Represents a map of <std::string, Value>.  The key parameter has to be a string in JSON.
 *
 * The key must be stored in UTF-8 format, so any translated strings or strings provided by the
 * user as a key must be converted to UTF-8 at the site where they are placed in the underlying
 * map that this PARAM_MAP points to.
 *
 * Values must also be in UTF-8, but if you use wxString as the value type, this conversion will
 * be done automatically by the to_json and from_json helpers defined in json_settings.cpp
 *
 * @tparam Value is the value type of the map
 */
template<typename Value>
class PARAM_MAP : public PARAM_BASE
{
public:
    PARAM_MAP( const std::string& aJsonPath, std::map<std::string, Value>* aPtr,
               std::initializer_list<std::pair<const std::string, Value>> aDefault,
               bool aReadOnly = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_default( aDefault )
    {
        SetClearUnknownKeys( true );
    }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override
    {
        if( m_readOnly )
            return;

        if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
        {
            if( js->is_object() )
            {
                m_ptr->clear();

                for( const auto& el : js->items() )
                    ( *m_ptr )[el.key()] = el.value().get<Value>();
            }
        }
        else if( aResetIfMissing )
            *m_ptr = m_default;
    }

    void Store( JSON_SETTINGS* aSettings) const override
    {
        nlohmann::json js( {} );

        for( const auto& el : *m_ptr )
            js[el.first] = el.second;

        aSettings->Set<nlohmann::json>( m_path, js );
    }

    virtual void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override
    {
        if( std::optional<nlohmann::json> js = aSettings.GetJson( m_path ) )
        {
            if( js->is_object() )
            {
                if( m_ptr->size() != js->size() )
                    return false;

                std::map<std::string, Value> val;

                for( const auto& el : js->items() )
                    val[el.key()] = el.value().get<Value>();

                return val == *m_ptr;
            }
        }

        return false;
    }

private:
    std::map<std::string, Value>* m_ptr;
    std::map<std::string, Value>  m_default;
};


#ifdef __WINDOWS__
template class KICOMMON_API PARAM_MAP<int>;
template class KICOMMON_API PARAM_MAP<double>;
template class KICOMMON_API PARAM_MAP<bool>;
#else
extern template class APIVISIBLE PARAM_MAP<int>;
extern template class APIVISIBLE PARAM_MAP<double>;
extern template class APIVISIBLE PARAM_MAP<bool>;
#endif


/**
 * A helper for <wxString, wxString> maps
 */
class KICOMMON_API PARAM_WXSTRING_MAP : public PARAM_BASE
{
public:
    PARAM_WXSTRING_MAP( const std::string& aJsonPath, std::map<wxString, wxString>* aPtr,
                        std::initializer_list<std::pair<const wxString, wxString>> aDefault,
                        bool aReadOnly = false, bool aArrayBehavior = false ) :
            PARAM_BASE( aJsonPath, aReadOnly ),
            m_ptr( aPtr ),
            m_default( aDefault )
    {
        m_clearUnknownKeys = aArrayBehavior;
    }

    void Load( const JSON_SETTINGS& aSettings, bool aResetIfMissing = true ) const override;

    void Store( JSON_SETTINGS* aSettings) const override;

    virtual void SetDefault() override
    {
        *m_ptr = m_default;
    }

    bool MatchesFile( const JSON_SETTINGS& aSettings ) const override;

private:
    std::map<wxString, wxString>* m_ptr;
    std::map<wxString, wxString>  m_default;
};

#endif
