/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#ifndef PROPERTY_H
#define PROPERTY_H

#include <core/wx_stl_compat.h>
#include <origin_transforms.h>
#include <properties/color4d_variant.h>
#include <properties/eda_angle_variant.h>
#include <properties/property_validator.h>

#include <wx/any.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/font.h>        // required for propgrid
#include <wx/validate.h>    // required for propgrid
#include <wx/propgrid/property.h>

#ifdef DEBUG
#include <wx/wxcrt.h>
#endif

#include <functional>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <type_traits>
#include "std_optional_variants.h"

class wxPGProperty;
class INSPECTABLE;
class PROPERTY_BASE;

template<typename T>
class ENUM_MAP;

///< Common property types
enum PROPERTY_DISPLAY
{
    PT_DEFAULT,    ///< Default property for a given type
    PT_SIZE,       ///< Size expressed in distance units (mm/inch)
    PT_AREA,       ///< Area expressed in distance units-squared (mm/inch)
    PT_COORD,      ///< Coordinate expressed in distance units (mm/inch)
    PT_DEGREE,     ///< Angle expressed in degrees
    PT_DECIDEGREE, ///< Angle expressed in decidegrees
    PT_RATIO,
    PT_TIME, ///< Time expressed in ps
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

    virtual T operator()( const Owner* aOwner ) const = 0;
};

template<typename Owner, typename T, typename FuncType>
class GETTER : public GETTER_BASE<Owner, T>
{
public:
    GETTER( FuncType aFunc )
        : m_func( aFunc )
    {
        wxCHECK( m_func, /*void*/ );
    }

    T operator()( const Owner* aOwner ) const override
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
        wxCHECK( m_func, /*void*/ );
    }

    void operator()( Owner* aOwner, T aValue ) override
    {
        ( aOwner->*m_func )( aValue );
    }

private:
    FuncType m_func;
};

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 5266 ) // 'const' qualifier on return type has no effect
#endif

template<typename Owner, typename T, typename Base = Owner>
class METHOD
{
public:
    static GETTER_BASE<Owner, T>* Wrap( T (Base::*aFunc)() )
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
        return new GETTER<Owner, T, T (Base::*)() const>( aFunc );
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

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

class PROPERTY_BASE
{
private:
    ///< Used to generate unique IDs.  Must come up front so it's initialized before ctor.

public:
    PROPERTY_BASE( const wxString& aName, PROPERTY_DISPLAY aDisplay = PT_DEFAULT,
                   ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType = ORIGIN_TRANSFORMS::NOT_A_COORD ) :
            m_name( aName ),
            m_display( aDisplay ),
            m_coordType( aCoordType ),
            m_hideFromPropertiesManager( false ),
            m_hideFromLibraryEditors( false ),
            m_hideFromDesignEditors( false ),
            m_hideFromRulesEditor( false ),
            m_availFunc( [](INSPECTABLE*)->bool { return true; } ),
            m_writeableFunc( [](INSPECTABLE*)->bool { return true; } ),
            m_validator( NullValidator )
    {
    }

    virtual ~PROPERTY_BASE()
    {
    }

    const wxString& Name() const { return m_name; }

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
        return m_choicesFunc != nullptr;
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
    PROPERTY_BASE& SetAvailableFunc( std::function<bool(INSPECTABLE*)> aFunc )
    {
        m_availFunc = std::move( aFunc );
        return *this;
    }

    wxPGChoices GetChoices( INSPECTABLE* aObject ) const
    {
        if( m_choicesFunc )
            return m_choicesFunc( aObject );

        return {};
    }

    PROPERTY_BASE& SetChoicesFunc( std::function<wxPGChoices(INSPECTABLE*)> aFunc )
    {
        m_choicesFunc = std::move( aFunc );
        return *this;
    }

    virtual bool Writeable( INSPECTABLE* aObject ) const
    {
        return m_writeableFunc( aObject );
    }

    PROPERTY_BASE& SetWriteableFunc( std::function<bool(INSPECTABLE*)> aFunc )
    {
        m_writeableFunc = std::move( aFunc );
        return *this;
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

    PROPERTY_DISPLAY Display() const { return m_display; }
    PROPERTY_BASE& SetDisplay( PROPERTY_DISPLAY aDisplay ) { m_display = aDisplay; return *this; }

    ORIGIN_TRANSFORMS::COORD_TYPES_T CoordType() const { return m_coordType; }
    PROPERTY_BASE& SetCoordType( ORIGIN_TRANSFORMS::COORD_TYPES_T aType )
    {
        m_coordType = aType;
        return *this;
    }

    bool IsHiddenFromPropertiesManager() const { return m_hideFromPropertiesManager; }
    PROPERTY_BASE& SetIsHiddenFromPropertiesManager( bool aHide = true )
    {
        m_hideFromPropertiesManager = aHide;
        return *this;
    }

    bool IsHiddenFromRulesEditor() const { return m_hideFromRulesEditor; }
    PROPERTY_BASE& SetIsHiddenFromRulesEditor( bool aHide = true )
    {
        m_hideFromRulesEditor = aHide;
        return *this;
    }

    bool IsHiddenFromLibraryEditors() const { return m_hideFromLibraryEditors; }
    PROPERTY_BASE& SetIsHiddenFromLibraryEditors( bool aIsHidden = true )
    {
        m_hideFromLibraryEditors = aIsHidden;
        return *this;
    }

    bool IsHiddenFromDesignEditors() const { return m_hideFromDesignEditors; }
    PROPERTY_BASE& SetIsHiddenFromDesignEditors( bool aIsHidden = true )
    {
        m_hideFromDesignEditors = aIsHidden;
        return *this;
    }

    wxString Group() const { return m_group; }
    PROPERTY_BASE& SetGroup( const wxString& aGroup ) { m_group = aGroup; return *this; }

    PROPERTY_BASE& SetValidator( PROPERTY_VALIDATOR_FN&& aValidator )
    {
        m_validator = aValidator;
        return *this;
    }

    VALIDATOR_RESULT Validate( const wxAny&& aValue, EDA_ITEM* aItem )
    {
        return m_validator( std::move( aValue ), aItem );
    }

    static VALIDATOR_RESULT NullValidator( const wxAny&& aValue, EDA_ITEM* aItem )
    {
        return std::nullopt;
    }

protected:
    template<typename T>
    void set( void* aObject, T aValue )
    {
        wxAny a = aValue;

        // wxVariant will be type "long" even if the property is supposed to be
        // unsigned.  Let's trust that we're coming from the property grid where
        // we used a UInt editor.
        if( std::is_same<T, wxVariant>::value )
        {
            wxVariant var = static_cast<wxVariant>( aValue );
            wxAny pv = getter( aObject );

            if( pv.CheckType<unsigned>() )
            {
                a = static_cast<unsigned>( var.GetLong() );
            }
            else if( pv.CheckType<std::optional<int>>() )
            {
                auto* data = static_cast<STD_OPTIONAL_INT_VARIANT_DATA*>( var.GetData() );
                a = data->Value();
            }
            else if( pv.CheckType<std::optional<double>>() )
            {
                auto* data = static_cast<STD_OPTIONAL_DOUBLE_VARIANT_DATA*>( var.GetData() );
                a = data->Value();
            }
            else if( pv.CheckType<EDA_ANGLE>() )
            {
                EDA_ANGLE_VARIANT_DATA* ad = static_cast<EDA_ANGLE_VARIANT_DATA*>( var.GetData() );
                a = ad->Angle();
            }
            else if( pv.CheckType<KIGFX::COLOR4D>() )
            {
                COLOR4D_VARIANT_DATA* cd = static_cast<COLOR4D_VARIANT_DATA*>( var.GetData() );
                a = cd->Color();
            }
        }

        setter( aObject, a );
    }

    template<typename T>
    T get( const void* aObject ) const
    {
        wxAny a = getter( aObject );

        // We don't currently have a bool type, so change it to a numeric
        if( a.CheckType<bool>() )
            a = a.RawAs<bool>() ? 1 : 0;

        if ( !( std::is_enum<T>::value && a.CheckType<int>() ) && !a.CheckType<T>() )
            throw std::invalid_argument( "Invalid requested type" );

        return wxANY_AS( a, T );
    }

private:
    virtual void setter( void* aObject, wxAny& aValue ) = 0;
    virtual wxAny getter( const void* aObject ) const = 0;

private:
    /**
     * Permanent identifier for this property.  Property names are an API contract; changing them
     * after release will impact the Custom DRC Rules system as well as the automatic API binding
     * system.  Never rename properties; instead deprecate them and hide them from the GUI.
     */
    const wxString m_name;

    /// The display style controls how properties are edited in the properties manager GUI
    PROPERTY_DISPLAY m_display;

    /// The coordinate type controls how distances are mapped to the user coordinate system
    ORIGIN_TRANSFORMS::COORD_TYPES_T m_coordType;

    bool m_hideFromPropertiesManager;   // Do not show in Properties Manager
    bool m_hideFromLibraryEditors;      // Do not show in Properties Manager of symbol or
                                        //   footprint editors
    bool m_hideFromDesignEditors;       // Do not show in Properties Manager of schematic or
                                        //   board editors
    bool m_hideFromRulesEditor;         // Do not show in Custom Rules editor autocomplete

    /// Optional group identifier
    wxString m_group;

    std::function<bool(INSPECTABLE*)> m_availFunc;   ///< Eval to determine if prop is available

    std::function<bool(INSPECTABLE*)> m_writeableFunc;   ///< Eval to determine if prop is read-only

    std::function<wxPGChoices(INSPECTABLE*)> m_choicesFunc;

    PROPERTY_VALIDATOR_FN m_validator;

    friend class INSPECTABLE;
};


template<typename Owner, typename T, typename Base = Owner>
class PROPERTY : public PROPERTY_BASE
{
public:
    using BASE_TYPE = typename std::decay<T>::type;

    template<typename SetType, typename GetType>
    PROPERTY( const wxString& aName,
              void ( Base::*aSetter )( SetType ),
              GetType( Base::*aGetter )(),
              PROPERTY_DISPLAY aDisplay = PT_DEFAULT,
              ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType = ORIGIN_TRANSFORMS::NOT_A_COORD ) :
        PROPERTY( aName,
                  METHOD<Owner, T, Base>::Wrap( aSetter ),
                  METHOD<Owner, T, Base>::Wrap( aGetter ),
                  aDisplay, aCoordType )
    {
    }

    template<typename SetType, typename GetType>
    PROPERTY( const wxString& aName,
              void ( Base::*aSetter )( SetType ),
              GetType( Base::*aGetter )() const,
              PROPERTY_DISPLAY aDisplay = PT_DEFAULT,
              ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType = ORIGIN_TRANSFORMS::NOT_A_COORD ) :
        PROPERTY( aName,
                  METHOD<Owner, T, Base>::Wrap( aSetter ),
                  METHOD<Owner, T, Base>::Wrap( aGetter ),
                  aDisplay, aCoordType )
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

    bool Writeable( INSPECTABLE* aObject ) const override
    {
        return m_setter && PROPERTY_BASE::Writeable( aObject );
    }

protected:
    PROPERTY( const wxString& aName,
              SETTER_BASE<Owner, T>* s,
              GETTER_BASE<Owner, T>* g,
              PROPERTY_DISPLAY aDisplay, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) :
        PROPERTY_BASE( aName, aDisplay, aCoordType ),
        m_setter( s ),
        m_getter( g ),
        m_ownerHash( TYPE_HASH( Owner ) ),
        m_baseHash( TYPE_HASH( Base ) ),
        m_typeHash( TYPE_HASH( BASE_TYPE ) )
    {
    }

    virtual ~PROPERTY() {}

    virtual void setter( void* obj, wxAny& v ) override
    {
        wxCHECK( m_setter, /*void*/ );

        BASE_TYPE value;

        if( !v.GetAs( &value ) )
            throw std::invalid_argument( "Invalid type requested" );

        Owner* o = reinterpret_cast<Owner*>( obj );
        (*m_setter)( o, value );
    }

    virtual wxAny getter( const void* obj ) const override
    {
        const Owner* o = reinterpret_cast<const Owner*>( obj );
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
                   void ( Base::*aSetter )( SetType ),
                   GetType( Base::*aGetter )(),
                   PROPERTY_DISPLAY aDisplay = PT_DEFAULT ) :
          PROPERTY<Owner, T, Base>( aName,
                                    METHOD<Owner, T, Base>::Wrap( aSetter ),
                                    METHOD<Owner, T, Base>::Wrap( aGetter ),
                                    aDisplay )
    {
        if ( std::is_enum<T>::value )
        {
            m_choices = ENUM_MAP<T>::Instance().Choices();
            wxASSERT_MSG( m_choices.GetCount() > 0, wxT( "No enum choices defined" ) );
        }
    }

    template<typename SetType, typename GetType>
    PROPERTY_ENUM( const wxString& aName,
                   void ( Base::*aSetter )( SetType ),
                   GetType( Base::*aGetter )() const,
                   PROPERTY_DISPLAY aDisplay = PT_DEFAULT,
                   ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType = ORIGIN_TRANSFORMS::NOT_A_COORD ) :
            PROPERTY<Owner, T, Base>( aName,
                                      METHOD<Owner, T, Base>::Wrap( aSetter ),
                                      METHOD<Owner, T, Base>::Wrap( aGetter ),
                                      aDisplay, aCoordType )
    {
        if ( std::is_enum<T>::value )
        {
            m_choices = ENUM_MAP<T>::Instance().Choices();
            wxASSERT_MSG( m_choices.GetCount() > 0, wxT( "No enum choices defined" ) );
        }
    }

    void setter( void* obj, wxAny& v ) override
    {
        wxCHECK( ( PROPERTY<Owner, T, Base>::m_setter ), /*void*/ );
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

    wxAny getter( const void* obj ) const override
    {
        const Owner* o = reinterpret_cast<const Owner*>( obj );
        wxAny res = static_cast<T>( (*PROPERTY<Owner, T, Base>::m_getter)( o ) );
        return res;
    }

    const wxPGChoices& Choices() const override
    {
        return m_choices.GetCount() > 0 ? m_choices : ENUM_MAP<T>::Instance().Choices();
    }

    void SetChoices( const wxPGChoices& aChoices ) override
    {
        m_choices = aChoices;
    }

    bool HasChoices() const override
    {
        return Choices().GetCount() > 0;
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

    T ToEnum( const wxString value )
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

    ENUM_MAP()
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
