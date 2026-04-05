/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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


#ifndef KICAD_API_TEST_UTILS_H
#define KICAD_API_TEST_UTILS_H

#include <boost/test/unit_test.hpp>
#include <boost/bimap.hpp>
#include <google/protobuf/any.pb.h>
#include <memory>
#include <set>
#include <magic_enum.hpp>

#include <api/api_enums.h>
#include <qa_utils/wx_utils/wx_assert.h>

/**
 * Checks if a KiCad enum has been properly mapped to a Protobuf enum
 * @tparam KiCadEnum is an enum type
 * @tparam ProtoEnum is a Protobuf enum type
 * @param aPartiallyMapped is true if only some of the KiCad enum values are exposed to the API
 */
template <typename KiCadEnum, typename ProtoEnum>
void testEnums( bool aPartiallyMapped = false )
{
    boost::bimap<ProtoEnum, KiCadEnum> protoToKiCadSeen;
    std::set<ProtoEnum>                seenProtos;

    for( ProtoEnum value : magic_enum::enum_values<ProtoEnum>() )
    {
        BOOST_TEST_CONTEXT( magic_enum::enum_type_name<ProtoEnum>() << "::" << magic_enum::enum_name( value ) )
        {
            std::string name( magic_enum::enum_name( value ) );
            auto        splitPos = name.find_first_of( '_' );

            // Protobuf enum names should be formatted as PREFIX_KEY
            BOOST_REQUIRE_MESSAGE( splitPos != std::string::npos, "Proto enum name doesn't have a prefix" );

            std::string suffix = name.substr( splitPos );

            // Protobuf enum with the value 0 should not map to anything
            if( static_cast<int>( value ) == 0 )
            {
                BOOST_REQUIRE_MESSAGE( suffix.compare( "_UNKNOWN" ) == 0,
                                       "Proto enum with value 0 must be named <PREFIX>_UNKNOWN" );
                continue;
            }

            KiCadEnum result;
            // Every non-unknown Proto value should map to a valid KiCad value
            BOOST_REQUIRE_NO_THROW( result = ( FromProtoEnum<KiCadEnum, ProtoEnum>( value ) ) );

            // There should be a 1:1 mapping
            BOOST_REQUIRE( !protoToKiCadSeen.left.count( value ) );
            protoToKiCadSeen.left.insert( { value, result } );
        }
    }

    for( KiCadEnum value : magic_enum::enum_values<KiCadEnum>() )
    {
        BOOST_TEST_CONTEXT( magic_enum::enum_type_name<KiCadEnum>() << "::" << magic_enum::enum_name( value ) )
        {
            ProtoEnum result;

            if( aPartiallyMapped )
            {
                try
                {
                    result = ToProtoEnum<KiCadEnum, ProtoEnum>( value );
                }
                catch( KI_TEST::WX_ASSERT_ERROR )
                {
                    // If it wasn't mapped from KiCad to Proto, it shouldn't be mapped the other way
                    BOOST_REQUIRE_MESSAGE( !protoToKiCadSeen.right.count( value ),
                                           "Proto enum is mapped to this KiCad enum, but not vice versa" );
                    continue;
                }
            }
            else
            {
                // Every KiCad enum value should map to a non-unknown Protobuf value
                BOOST_REQUIRE_NO_THROW( result = ( ToProtoEnum<KiCadEnum, ProtoEnum>( value ) ) );
            }

            // Protobuf "unknown" should always be zero value by convention
            BOOST_REQUIRE( result != static_cast<ProtoEnum>( 0 ) );

            // There should be a 1:1 mapping
            BOOST_REQUIRE( !seenProtos.count( result ) );
            seenProtos.insert( result );

            // Round-tripping should work
            KiCadEnum roundTrip = FromProtoEnum<KiCadEnum, ProtoEnum>( result );
            BOOST_REQUIRE( roundTrip == value );
        }
    }
}


template<typename ProtoClass, typename KiCadClass, typename Factory>
void testProtoFromKiCadObject( KiCadClass* aInput, Factory&& aCreateOutput )
{
    BOOST_TEST_CONTEXT( aInput->GetFriendlyName() << ": " << aInput->m_Uuid.AsStdString() )
    {
        google::protobuf::Any any;
        BOOST_REQUIRE_NO_THROW( aInput->Serialize( any ) );

        ProtoClass proto;
        BOOST_REQUIRE_MESSAGE( any.UnpackTo( &proto ),
                               "Any message did not unpack into the requested type" );

        std::unique_ptr<KiCadClass> output = aCreateOutput();

        bool deserializeResult = false;
        BOOST_REQUIRE_NO_THROW( deserializeResult = output->Deserialize( any ) );
        BOOST_REQUIRE_MESSAGE( deserializeResult, "Deserialize failed" );

        google::protobuf::Any outputAny;
        BOOST_REQUIRE_NO_THROW( output->Serialize( outputAny ) );

        if( !( outputAny.SerializeAsString() == any.SerializeAsString() ) )
        {
            BOOST_TEST_MESSAGE( "Input: " << any.Utf8DebugString() );
            BOOST_TEST_MESSAGE( "Output: " << outputAny.Utf8DebugString() );
            BOOST_TEST_FAIL( "Round-tripped protobuf does not match" );
        }

        if( !( *output == *aInput ) )
            BOOST_TEST_FAIL( "Round-tripped object does not match" );
    }
}


template<typename ProtoClass, typename KiCadClass, typename ParentClass>
void testProtoFromKiCadObject( KiCadClass* aInput, ParentClass* aParent, bool aStrict = true )
{
    if( aStrict )
    {
        testProtoFromKiCadObject<ProtoClass>( aInput,
                [aParent]() { return std::make_unique<KiCadClass>( aParent ); } );
    }
    else
    {
        testProtoFromKiCadObject<ProtoClass>( aInput,
                [aInput]()
                {
                    std::unique_ptr<KiCadClass> cloned( static_cast<KiCadClass*>( aInput->Clone() ) );
                    return std::make_unique<KiCadClass>( *cloned );
                } );
    }
}

#endif //KICAD_API_TEST_UTILS_H
