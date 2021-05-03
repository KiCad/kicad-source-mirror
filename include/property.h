/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PROPERTY_H
#define PROPERTY_H

#include <core/wx_stl_compat.h>

#include <wx/any.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/font.h>        // required for propgrid
#include <wx/validate.h>    // required for propgrid
#include <wx/propgrid/property.h>

#include <functional>
#include <map>
#include <memory>
#include <typeindex>
#include <type_traits>

class wxPGProperty;
class INSPECTABLE;
class PROPERTY_BASE;

template<typename T>
class ENUM_MAP;

///< Common property types
enum PROPERTY_DISPLAY
{
    DEFAULT,    ///< Default property for a given type
    DISTANCE,   ///< Display value expressed in distance units (mm/inch)
    DEGREE,     ///< Display value expressed in degrees
    DECIDEGREE  ///< Convert decidegrees to degrees for display
};

///< Macro to generate unique identifier for a type
#define TYPE_HASH( x ) typeid( x ).hash_code()
#define TYPE_NAME( x ) typeid( x ).name()
//#define TYPE_HASH( x ) typeid( std::decay<x>::type ).hash_code()

template<typename Owner, typename T>
class GETTER_BASE
{
public:
    virtual ~GETTER_BASE() {}

    virtual T operator()( Owner* aOwner ) const = 0;
};

template<typename Owner, typename T, typename FuncType>
class GETTER : public GETTER_BASE<Owner, T>
{
public:
    GETTER( FuncType aFunc )
        : m_func( aFunc )
    {
    }

    T operator()( Owner* aOwner ) const override
    {
        return ( aOwner->*m_func )();
    }

private:
    FuncType m_func;
};

template<typename Owner, typename T>
class SETTER_BASE
{
public:
    virtual ~SETTER_BASE() {}

    virtual void operator()( Owner* aOwner, T aValue ) = 0;
};

template<typename Owner, typename T, typename FuncType>
class SETTER : public SETTER_BASE<Owner, T>
{
public:
    SETTER( FuncType aFunc )
        : m_func( aFunc )
    {
    }

    void operator()( Owner* aOwner, T aValue ) override
    {
        wxCHECK( m_func, /*void*/ );
        ( aOwner->*m_func )( aValue );
    }

private:
    FuncType m_func;
};


template<typename Owner, typename T, typename Base = Owner>
class METHOD
{
public:
    constexpr static GETTER_BASE<Owner, T>* Wrap( T (Base::*aFunc)() )
    {
        return new GETTER<Owner, T, T (Base::*)()>( aFunc );
    }

    constexpr static GETTER_BASE<Owner, T>* Wrap( const T (Base::*aFunc)() )
    {
        return new GETTER<Owner, T, T (Base::*)()>( aFunc );
    }

    constexpr static GETTER_BASE<Owner, T>* Wrap( const T& (Base::*aFunc)() )
    {
        return new GETTER<Owner, T, const T& (Base::*)()>( aFunc );
    }

    constexpr static GETTER_BASE<Owner, T>* Wrap( T (Base::*aFunc)() const )
    {
        return new GETTER<Owner, T, T (Base::*)() const>( aFunc );
    }

    constexpr static GETTER_BASE<Owner, T>* Wrap( const T (Base::*aFunc)() const )
    {
        return new GETTER<Owner, T, const T (Base::*)() const>( aFunc );
    }

    constexpr static GETTER_BASE<Owner, T>* Wrap( const T& (Base::*aFunc)() const )
    {
        return new GETTER<Owner, T, const T& (Base::*)() const>( aFunc );
    }

    constexpr static SETTER_BASE<Owner, T>* Wrap( void (Base::*aFunc)( T ) )
    {
        return aFunc ? new SETTER<Owner, T, void (Base::*)( T )>( aFunc ) : nullptr;
    }

    constexpr static SETTER_BASE<Owner, T>* Wrap( void (Base::*aFunc)( T& ) )
    {
        return aFunc ? new SETTER<Owner, T, void (Base::*)( T& )>( aFunc ) : nullptr;
    }

    constexpr static SETTER_BASE<Owner, T>* Wrap( void (Base::*aFunc)( const T& ) )
    {
        return aFunc ? new SETTER<Owner, T, void (Base::*)( const T& )>( aFunc ) : nullptr;
    }

    METHOD() = delete;
};


class PROPERTY_BASE
{
private:
    ///< Used to generate unique IDs.  Must come up front so it's initialized before ctor.

public:
    PROPERTY_BASE( const wxString& aName, PROPERTY_DISPLAY aDisplay = DEFAULT ) :
        m_name( aName ),
        m_display( aDisplay ),
        m_availFunc( [](INSPECTABLE*)->bool { return true; } )
    {
    }

    virtual ~PROPERTY_BASE()
    {
    }

    const wxString& Name() const
    {
        return m_name;
    }

    /**
     * Return a limited set of possible values (e.g. enum). Check with HasChoices() if a particular
     * PROPERTY provides such set.
     */
    virtual const wxPGChoices& Choices() const
    {
        static wxPGChoices empty;
        return empty;
    }

    /**
     * Set the possible values for for the property.
     */
    virtual void SetChoices( const wxPGChoices& aChoices )
    {
        wxFAIL; // only possible for PROPERTY_ENUM
    }

    /**
     * Return true if this PROPERTY has a limited set of possible values.
     * @see PROPERTY_BASE::Choices()
     */
    virtual bool HasChoices() const
    {
        return false;
    }

    /**
     * Return true if aObject offers this PROPERTY.
     */
    bool Available( INSPECTABLE* aObject ) const
    {
        return m_availFunc( aObject );
    }

    /**
     * Set a callback function to determine whether an object provides this property.
     */
    void SetAvailableFunc( std::function<bool(INSPECTABLE*)> aFunc )
    {
        m_availFunc = aFunc;
    }

    /**
     * Return type-id of the Owner class.
     */
    virtual size_t OwnerHash() const = 0;

    /**
     * Return type-id of the Base class.
     */
    virtual size_t BaseHash() const = 0;

    /**
     * Return type-id of the property type.
     */
    virtual size_t TypeHash() const = 0;

    virtual bool IsReadOnly() const = 0;

    PROPERTY_DISPLAY GetDisplay() const
    {
        return m_display;
    }

protected:
    template<typename T>
    void set( void* aObject, T aValue )
    {
        wxAny a = aValue;
        setter( aObject, a );
    }

    template<typename T>
    T get( void* aObject )
    {
        wxAny a = getter( aObject );

        if ( !( std::is_enum<T>::value && a.CheckType<int>() ) && !a.CheckType<T>() )
            throw std::invalid_argument( "Invalid requested type" );

        return wxANY_AS(a, T);
    }

    virtual void setter( void* aObject, wxAny& aValue ) = 0;
    virtual wxAny getter( void* aObject ) const = 0;

private:
    const wxString                    m_name;
    const PROPERTY_DISPLAY            m_display;

    std::function<bool(INSPECTABLE*)> m_availFunc;   ///< Eval to determine if prop is available

    friend class INSPECTABLE;
};


template<typename Owner, typename T, typename Base = Owner>
class PROPERTY : public PROPERTY_BASE
{
public:
    typedef typename std::decay<T>::type BASE_TYPE;
    typedef void (Base::*SETTER)( T );

    template<typename SetType, typename GetType>
    PROPERTY( const wxString& aName,
            void ( Base::*aSetter )( SetType ), GetType( Base::*aGetter )(),
            PROPERTY_DISPLAY aDisplay = DEFAULT )
        : PROPERTY( aName, METHOD<Owner, T, Base>::Wrap( aSetter ),
                           METHOD<Owner, T, Base>::Wrap( aGetter ), aDisplay )
    {
    }

    template<typename SetType, typename GetType>
    PROPERTY( const wxString& aName,
            void ( Base::*aSetter )( SetType ), GetType( Base::*aGetter )() const,
            PROPERTY_DISPLAY aDisplay = DEFAULT )
        : PROPERTY( aName, METHOD<Owner, T, Base>::Wrap( aSetter ),
                           METHOD<Owner, T, Base>::Wrap( aGetter ), aDisplay )
    {
    }

    size_t OwnerHash() const override
    {
        return m_ownerHash;
    }

    size_t BaseHash() const override
    {
        return m_baseHash;
    }

    size_t TypeHash() const override
    {
        return m_typeHash;
    }

    bool IsReadOnly() const override
    {
        return !m_setter;
    }

protected:
    PROPERTY( const wxString& aName, SETTER_BASE<Owner, T>* s, GETTER_BASE<Owner, T>* g,
            PROPERTY_DISPLAY aDisplay )
        : PROPERTY_BASE( aName, aDisplay ), m_setter( s ), m_getter( g ),
                m_ownerHash( TYPE_HASH( Owner ) ), m_baseHash( TYPE_HASH( Base ) ),
                m_typeHash( TYPE_HASH( BASE_TYPE ) )
    {
    }

    virtual ~PROPERTY() {}

    virtual void setter( void* obj, wxAny& v ) override
    {
        wxCHECK( !IsReadOnly(), /*void*/ );

        if( !v.CheckType<T>() )
            throw std::invalid_argument( "Invalid type requested" );

        Owner* o = reinterpret_cast<Owner*>( obj );
        BASE_TYPE value = wxANY_AS(v, BASE_TYPE);
        (*m_setter)( o, value );
    }

    virtual wxAny getter( void* obj ) const override
    {
        Owner* o = reinterpret_cast<Owner*>( obj );
        wxAny res = (*m_getter)( o );
        return res;
    }

    ///< Set method
    std::unique_ptr<SETTER_BASE<Owner, T>> m_setter;

    ///< Get method
    std::unique_ptr<GETTER_BASE<Owner, T>> m_getter;

    ///< Owner class type-id
    const size_t m_ownerHash;

    ///< Base class type-id
    const size_t m_baseHash;

    ///< Property value type-id
    const size_t m_typeHash;
};


template<typename Owner, typename T, typename Base = Owner>
class PROPERTY_ENUM : public PROPERTY<Owner, T, Base>
{
public:
    template<typename SetType, typename GetType>
    PROPERTY_ENUM( const wxString& aName,
            void ( Base::*aSetter )( SetType ), GetType( Base::*aGetter )(),
            PROPERTY_DISPLAY aDisplay = PROPERTY_DISPLAY::DEFAULT )
        : PROPERTY<Owner, T, Base>( aName, METHOD<Owner, T, Base>::Wrap( aSetter ),
                                     METHOD<Owner, T, Base>::Wrap( aGetter ), aDisplay )
    {
        if ( std::is_enum<T>::value )
        {
            m_choices = ENUM_MAP<T>::Instance().Choices();
            wxASSERT_MSG( m_choices.GetCount() > 0, "No enum choices defined" );
        }
    }

    template<typename SetType, typename GetType>
    PROPERTY_ENUM( const wxString& aName,
            void ( Base::*aSetter )( SetType ), GetType( Base::*aGetter )() const,
            PROPERTY_DISPLAY aDisplay = PROPERTY_DISPLAY::DEFAULT )
        : PROPERTY<Owner, T, Base>( aName, METHOD<Owner, T, Base>::Wrap( aSetter ),
                                     METHOD<Owner, T, Base>::Wrap( aGetter ), aDisplay )
    {
        if ( std::is_enum<T>::value )
        {
            m_choices = ENUM_MAP<T>::Instance().Choices();
            wxASSERT_MSG( m_choices.GetCount() > 0, "No enum choices defined" );
        }
    }

    virtual void setter( void* obj, wxAny& v ) override
    {
        wxCHECK( !( PROPERTY<Owner, T, Base>::IsReadOnly() ), /*void*/ );
        Owner* o = reinterpret_cast<Owner*>( obj );

        if( v.CheckType<T>() )
        {
            T value = wxANY_AS(v, T);
            (*PROPERTY<Owner, T, Base>::m_setter)( o, value );
        }
        else if (v.CheckType<int>() )
        {
            int value = wxANY_AS(v, int);
            (*PROPERTY<Owner, T, Base>::m_setter)( o, static_cast<T>( value ) );
        }
        else
        {
            throw std::invalid_argument( "Invalid type requested" );
        }
    }

    virtual wxAny getter( void* obj ) const override
    {
        Owner* o = reinterpret_cast<Owner*>( obj );
        wxAny res = static_cast<T>( (*PROPERTY<Owner, T, Base>::m_getter)( o ) );
        return res;
    }

    const wxPGChoices& Choices() const override
    {
        return m_choices;
    }

    void SetChoices( const wxPGChoices& aChoices ) override
    {
        m_choices = aChoices;
    }

    bool HasChoices() const override
    {
        return m_choices.GetCount() > 0;
    }

protected:
    wxPGChoices m_choices;
};


class TYPE_CAST_BASE
{
public:
    virtual ~TYPE_CAST_BASE() {}
    virtual void* operator()( void* aPointer ) const = 0;
    virtual const void* operator()( const void* aPointer ) const = 0;
    virtual size_t BaseHash() const = 0;
    virtual size_t DerivedHash() const = 0;
};


template<typename Base, typename Derived>
class TYPE_CAST : public TYPE_CAST_BASE
{
public:
    TYPE_CAST()
    {
    }

    void* operator()( void* aPointer ) const override
    {
        Base* base = reinterpret_cast<Base*>( aPointer );
        return static_cast<Derived*>( base );
    }

    const void* operator()( const void* aPointer ) const override
    {
        const Base* base = reinterpret_cast<const Base*>( aPointer );
        return static_cast<const Derived*>( base );
    }

    size_t BaseHash() const override
    {
        return TYPE_HASH( Base );
    }

    size_t DerivedHash() const override
    {
        return TYPE_HASH( Derived );
    }
};


template<typename T>
class ENUM_MAP
{
public:
    static ENUM_MAP<T>& Instance()
    {
        static ENUM_MAP<T> inst;
        return inst;
    }

    ENUM_MAP& Map( T aValue, const wxString& aName )
    {
        m_choices.Add( aName, static_cast<int>( aValue ) );
        m_reverseMap[ aName ] = aValue;
        return *this;
    }

    ENUM_MAP& Undefined( T aValue )
    {
        m_undefined = aValue;
        return *this;
    }

    const wxString& ToString( T value ) const
    {
        static const wxString s_undef = "UNDEFINED";

        int idx = m_choices.Index( static_cast<int>( value ) );

        if( idx >= 0 && idx < (int) m_choices.GetCount() )
            return m_choices.GetLabel( static_cast<int>( idx ) );
        else
            return s_undef;
    }

    bool IsValueDefined( T value ) const
    {
        int idx = m_choices.Index( static_cast<int>( value ) );

        if( idx >= 0 && idx < (int) m_choices.GetCount() )
            return true;

        return false;
    }

    const T ToEnum( const wxString value )
    {
        if( m_reverseMap.count( value ) )
            return m_reverseMap[ value ];
        else
            return m_undefined;
    }

    wxPGChoices& Choices()
    {
        return m_choices;
    }

private:
    wxPGChoices                     m_choices;
    std::unordered_map<wxString, T> m_reverseMap;
    T                               m_undefined;      // Returned if the string is not recognized

    ENUM_MAP<T>()
    {
    }
};


// Helper macros to handle enum types
#define DECLARE_ENUM_TO_WXANY( type )                                                       \
    template <>                                                                             \
    class wxAnyValueTypeImpl<type> : public wxAnyValueTypeImplBase<type>                    \
    {                                                                                       \
        WX_DECLARE_ANY_VALUE_TYPE( wxAnyValueTypeImpl<type> )                               \
    public:                                                                                 \
        wxAnyValueTypeImpl() : wxAnyValueTypeImplBase<type>() {}                            \
        virtual ~wxAnyValueTypeImpl() {}                                                    \
        virtual bool ConvertValue( const wxAnyValueBuffer& src, wxAnyValueType* dstType,    \
                                   wxAnyValueBuffer& dst ) const override                   \
        {                                                                                   \
            type            value = GetValue( src );                                        \
            ENUM_MAP<type>& conv = ENUM_MAP<type>::Instance();                              \
            if( ! conv.IsValueDefined( value ) )                                            \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
            if( dstType->CheckType<wxString>() )                                            \
            {                                                                               \
                wxAnyValueTypeImpl<wxString>::SetValue( conv.ToString( value ), dst );      \
                return true;                                                                \
            }                                                                               \
            if( dstType->CheckType<int>() )                                                 \
            {                                                                               \
                wxAnyValueTypeImpl<int>::SetValue( static_cast<int>( value ), dst );        \
                return true;                                                                \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                return false;                                                               \
            }                                                                               \
        }                                                                                   \
    };

#define IMPLEMENT_ENUM_TO_WXANY( type ) WX_IMPLEMENT_ANY_VALUE_TYPE( wxAnyValueTypeImpl<type> )

#define ENUM_TO_WXANY( type )                                                               \
    DECLARE_ENUM_TO_WXANY( type )                                                           \
    IMPLEMENT_ENUM_TO_WXANY( type )

///< Macro to define read-only fields (no setter method available)
#define NO_SETTER( owner, type ) ( ( void ( owner::* )( type ) ) nullptr )

/*
#define DECLARE_PROPERTY(owner,type,name,getter,setter) \
namespace anonymous {\
static PROPERTY<owner,type> prop##_owner##_name_( "##name#", setter, getter );\
};
*/
#endif /* PROPERTY_H */
