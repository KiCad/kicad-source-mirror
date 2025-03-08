/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
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

#ifndef _ATTRIBUTE_PROVIDER_H_
#define _ATTRIBUTE_PROVIDER_H_

#include "odb_util.h"
#include "stroke_params.h"
#include <wx/string.h>
#include <string>
#include <type_traits>


namespace ODB_ATTR
{

enum class TYPE
{
    FLOAT,
    BOOLEAN,
    TEXT,
    OPTION,
    INTEGER
};

// Base class template for attributes
template <typename T, TYPE AttrType>
struct AttributeBase
{
    using ValueType = T;
    static constexpr TYPE type = AttrType;
    constexpr AttributeBase( T v ) : value( v ) {}
    T value;
};

// Specialized attribute types
template <typename T, unsigned int N>
struct FloatAttribute : AttributeBase<double, TYPE::FLOAT>
{
    static constexpr unsigned int digits = N;
    using AttributeBase<double, TYPE::FLOAT>::AttributeBase;
};

template <typename T>
struct BooleanAttribute : AttributeBase<bool, TYPE::BOOLEAN>
{
    using AttributeBase<bool, TYPE::BOOLEAN>::AttributeBase;
};

template <typename T>
struct TextAttribute : AttributeBase<std::string, TYPE::TEXT>
{
    constexpr TextAttribute( const std::string& t ) :
            AttributeBase<std::string, TYPE::TEXT>( ODB::GenLegalEntityName( t ).ToStdString() )
    {
    }
};

template <typename T>
struct OPTION_Attribute : AttributeBase<int, TYPE::OPTION>
{
    using AttributeBase<int, TYPE::OPTION>::AttributeBase;
};


template <typename T>
struct AttributeName
{
};

// Attribute name and type definitions
template <typename Tag, template <typename, unsigned int> class Attr, TYPE AttrType, unsigned int N>
struct Attribute
{
    using TYPE = Attr<Tag, N>;
};

template <typename Tag, template <typename> class Attr, TYPE AttrType>
struct AttributeSimple
{
    using TYPE = Attr<Tag>;
};

// TYPE traits for attributes
template <typename T>
struct IsFeature : std::false_type
{
};
template <typename T>
struct IsNet : std::false_type
{
};
template <typename T>
struct IsPkg : std::false_type
{
};
template <typename T>
struct IsLayer : std::false_type
{
};
template <typename T>
struct IsStep : std::false_type
{
};
template <typename T>
struct IsComp : std::false_type
{
};
template <typename T>
struct IsProductModel : std::false_type
{
};
template <typename T>
struct IsSymbol : std::false_type
{
};


#define DEFINE_ATTR( Tag, Attr, AttrType, AttrName, ... )                                          \
    struct Tag##_t                                                                                 \
    {                                                                                              \
    };                                                                                             \
    constexpr const char Tag##_name[] = AttrName;                                                  \
    using Tag = Attribute<Tag##_t, Attr, AttrType, __VA_ARGS__>::TYPE;                             \
    template <>                                                                                    \
    struct AttributeName<Tag>                                                                      \
    {                                                                                              \
        static constexpr const char* name = Tag##_name;                                            \
    };

#define DEFINE_ATTR_SIMPLE( Tag, Attr, AttrType, AttrName )                                        \
    struct Tag##_t                                                                                 \
    {                                                                                              \
    };                                                                                             \
    constexpr const char Tag##_name[] = AttrName;                                                  \
    using Tag = AttributeSimple<Tag##_t, Attr, AttrType>::TYPE;                                    \
    template <>                                                                                    \
    struct AttributeName<Tag>                                                                      \
    {                                                                                              \
        static constexpr const char* name = Tag##_name;                                            \
    };


#define DEFINE_FLOAT_ATTR( NAME, N ) DEFINE_ATTR( NAME, FloatAttribute, TYPE::FLOAT, #NAME, N )

#define DEFINE_BOOLEAN_ATTR( NAME )                                                                \
    DEFINE_ATTR_SIMPLE( NAME, BooleanAttribute, TYPE::BOOLEAN, #NAME )

#define DEFINE_TEXT_ATTR( NAME ) DEFINE_ATTR_SIMPLE( NAME, TextAttribute, TYPE::TEXT, #NAME )

#define DEFINE_OPTION_ATTR( NAME )                                                                 \
    struct NAME##_t                                                                                \
    {                                                                                              \
    };                                                                                             \
    template <>                                                                                    \
    struct AttributeSimple<NAME##_t, OPTION_Attribute, TYPE::OPTION>;                              \
    template <>                                                                                    \
    struct AttributeName<NAME>                                                                     \
    {                                                                                              \
        static constexpr const char* name = #NAME;                                                 \
    };


// used by which entity
#define USED_BY_FEATURE_ENTITY( NAME )                                                             \
    template <>                                                                                    \
    struct IsFeature<NAME> : std::true_type                                                        \
    {                                                                                              \
    };

#define USED_BY_NET_ENTITY( NAME )                                                                 \
    template <>                                                                                    \
    struct IsNet<NAME> : std::true_type                                                            \
    {                                                                                              \
    };

#define USED_BY_PKG_ENTITY( NAME )                                                                 \
    template <>                                                                                    \
    struct IsPkg<NAME> : std::true_type                                                            \
    {                                                                                              \
    };

#define USED_BY_CMP_ENTITY( NAME )                                                                 \
    template <>                                                                                    \
    struct IsComp<NAME> : std::true_type                                                            \
    {                                                                                              \
    };

// Attribute definitions
// BOOLEAN ATTRIBUTES
DEFINE_BOOLEAN_ATTR( SMD )
USED_BY_FEATURE_ENTITY( SMD )

DEFINE_BOOLEAN_ATTR( NET_POINT )
USED_BY_FEATURE_ENTITY( NET_POINT )

DEFINE_BOOLEAN_ATTR( ROUT_PLATED )
USED_BY_FEATURE_ENTITY( ROUT_PLATED )

DEFINE_BOOLEAN_ATTR( MECHANICAL )

DEFINE_BOOLEAN_ATTR( MOUNT_HOLE )
USED_BY_FEATURE_ENTITY( MOUNT_HOLE )

DEFINE_BOOLEAN_ATTR( TEAR_DROP )
USED_BY_FEATURE_ENTITY( TEAR_DROP )

DEFINE_BOOLEAN_ATTR( TEST_POINT )
USED_BY_FEATURE_ENTITY( TEST_POINT )

// TEXT ATTRIBUTES
DEFINE_TEXT_ATTR( STRING )
USED_BY_FEATURE_ENTITY( STRING )

DEFINE_TEXT_ATTR( GEOMETRY )
USED_BY_FEATURE_ENTITY( GEOMETRY )

DEFINE_TEXT_ATTR( NET_NAME )
USED_BY_FEATURE_ENTITY( NET_NAME )


// FLOAT ATTRIBUTES
DEFINE_FLOAT_ATTR( BOARD_THICKNESS, 1 ) // 0.0~10.0

DEFINE_FLOAT_ATTR( STRING_ANGLE, 1 ) // 0.0~360.0
USED_BY_FEATURE_ENTITY( STRING_ANGLE )


// OPTION ATTRIBUTES
enum class DRILL
{
    PLATED,
    NON_PLATED,
    VIA
};
DEFINE_OPTION_ATTR( DRILL )
USED_BY_FEATURE_ENTITY( DRILL )

enum class PAD_USAGE
{
    TOEPRINT,
    VIA,
    G_FIDUCIAL,
    L_FIDUCIAL,
    TOOLING_HOLE,
    BOND_FINGER
};
DEFINE_OPTION_ATTR( PAD_USAGE )
USED_BY_FEATURE_ENTITY( PAD_USAGE )

enum class PLATED_TYPE
{
    STANDARD,
    PRESS_FIT
};
DEFINE_OPTION_ATTR( PLATED_TYPE )
USED_BY_FEATURE_ENTITY( PLATED_TYPE )

enum class VIA_TYPE
{
    DRILLED,
    LASER,
    PHOTO
};
DEFINE_OPTION_ATTR( VIA_TYPE )
USED_BY_FEATURE_ENTITY( VIA_TYPE )

enum class COMP_MOUNT_TYPE
{
    OTHER,
    MT_SMD,
    THT,
    PRESSFIT
};
DEFINE_OPTION_ATTR( COMP_MOUNT_TYPE )
USED_BY_CMP_ENTITY( COMP_MOUNT_TYPE )

DEFINE_BOOLEAN_ATTR( NO_POP )
USED_BY_CMP_ENTITY( NO_POP )
} // namespace ODB_ATTR


class ATTR_MANAGER
{
public:
    ATTR_MANAGER() = default;
    virtual ~ATTR_MANAGER() = default;

    template <typename Tr, typename Ta>
    void AddSystemAttribute( Tr& r, Ta v )
    {
        std::string name = std::string( "." ) + std::string( ODB_ATTR::AttributeName<Ta>::name );
        const auto id = GetAttrNameNumber( name );

        if constexpr( std::is_enum_v<Ta> )
            r.m_ODBattributes.emplace( id, std::to_string( static_cast<int>( v ) ) );
        else
            r.m_ODBattributes.emplace( id, AttrValue2String( v ) );
    }

    template <typename Tr, typename Ta>
    void AddUserDefAttribute( Tr& r, Ta v )
    {
        const auto id = GetAttrNameNumber( ODB_ATTR::AttributeName<Ta>::name );

        if constexpr( std::is_enum_v<Ta> )
            r.m_ODBattributes.emplace( id, std::to_string( static_cast<int>( v ) ) );
        else
            r.m_ODBattributes.emplace( id, AttrValue2String( v ) );
    }

protected:
    size_t GetAttrNameNumber( const wxString& name );

    void WriteAttributes( std::ostream& ost, const std::string& prefix = "" ) const;
    void WriteAttributesName( std::ostream& ost, const std::string& prefix = "" ) const;
    void WriteAttributesText( std::ostream& ost, const std::string& prefix = "" ) const;


private:
    size_t GetAttrTextNumber( const wxString& aName );
    size_t GetTextIndex( std::unordered_map<std::string, size_t>&     aMap,
                         std::vector<std::pair<size_t, std::string>>& aVec,
                         const std::string&                           aText );

    template <typename T, unsigned int n>
    std::string AttrValue2String( ODB_ATTR::FloatAttribute<T, n> a )
    {
        return ODB::Double2String( a.value, a.digits );
    }

    template <typename T>
    std::string AttrValue2String( ODB_ATTR::BooleanAttribute<T> a )
    {
        return "";
    }

    template <typename T>
    std::string AttrValue2String( ODB_ATTR::TextAttribute<T> a )
    {
        return std::to_string( GetAttrTextNumber( a.value ) );
    }


    std::unordered_map<std::string, size_t>     m_attrNames;
    std::vector<std::pair<size_t, std::string>> m_attrNameVec;

    std::unordered_map<std::string, size_t>     m_attrTexts;
    std::vector<std::pair<size_t, std::string>> m_attrTextVec;
};

class ATTR_RECORD_WRITER
{
public:
    ATTR_RECORD_WRITER() = default;
    virtual ~ATTR_RECORD_WRITER() = default;

    void WriteAttributes( std::ostream& ost ) const;

public:
    std::map<unsigned int, std::string> m_ODBattributes;
};


#endif // ATTRIBUTE_PROVIDER_H_