/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <sexpr/sexpr_parser.h>

namespace KI_TEST
{

inline std::string ReadGoldenText( const std::string& aPath )
{
    std::ifstream file( aPath );

    if( !file.is_open() )
        throw std::runtime_error( "Cannot read downgrade fixture: " + aPath );

    return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}


inline std::string GoldenNodeHead( const SEXPR::SEXPR& aNode )
{
    if( aNode.IsList() && aNode.GetNumberOfChildren() && aNode.GetChild( 0 )->IsSymbol() )
        return aNode.GetChild( 0 )->GetSymbol();

    return {};
}


inline const SEXPR::SEXPR* FindGoldenChild( const SEXPR::SEXPR& aNode, const std::string& aHead )
{
    if( aNode.IsList() )
    {
        for( const SEXPR::SEXPR* child : *aNode.GetChildren() )
        {
            if( GoldenNodeHead( *child ) == aHead )
                return child;
        }
    }

    return nullptr;
}


inline double SerializedImageScale( const std::string& aContent, const std::string& aUuid )
{
    SEXPR::PARSER parser;
    auto          root = parser.Parse( aContent );

    if( root && root->IsList() )
    {
        for( const SEXPR::SEXPR* child : *root->GetChildren() )
        {
            if( GoldenNodeHead( *child ) != "image" )
                continue;

            const SEXPR::SEXPR* uuid = FindGoldenChild( *child, "uuid" );

            if( !uuid || uuid->GetNumberOfChildren() != 2 || !uuid->GetChild( 1 )->IsString()
                || uuid->GetChild( 1 )->GetString() != aUuid )
                continue;

            const SEXPR::SEXPR* scale = FindGoldenChild( *child, "scale" );

            if( scale && scale->GetNumberOfChildren() == 2 )
            {
                const SEXPR::SEXPR& value = *scale->GetChild( 1 );

                if( value.IsInteger() )
                    return value.GetLongInteger();

                if( value.IsDouble() )
                    return value.GetDouble();
            }

            throw std::runtime_error( "Image has no numeric scale: " + aUuid );
        }
    }

    throw std::runtime_error( "Serialized image is missing: " + aUuid );
}


// Whitelist non-design differences ONLY in their actual contexts. Keep version stamps,
// UUIDs, nets, coordinates, drawing order, attributes, and every other setting observable.
inline bool IgnoreGoldenNode( const SEXPR::SEXPR& aNode, const std::string& aParent )
{
    if( !aNode.IsList() )
        return false;

    const std::string head = GoldenNodeHead( aNode );

    // Every design and library root carries the writing application's version. A downgraded file
    // is still written by the current build, so this never matches the target's own stamp.
    if( ( aParent == "kicad_pcb" || aParent == "kicad_sch" || aParent == "footprint" || aParent == "kicad_symbol_lib" )
        && head == "generator_version" )
    {
        return true;
    }

    // The shared modern plot-settings formatter adds these even to frozen PCB writers.
    // The target releases have no PNG plot output and harmlessly skip these settings.
    if( aParent != "pcbplotparams" )
        return false;

    if( head == "pngdpi" || head == "pngantialias" )
        return true;

    // Removed modern options that KiCad 9 itself writes at their default values. Missing
    // entries reload to those SAME defaults. Never ignore a non-default reference value.
    if( aNode.GetNumberOfChildren() != 2 )
        return false;

    const SEXPR::SEXPR& value = *aNode.GetChild( 1 );

    if( head == "plotinvisibletext" )
        return value.IsSymbol() && value.GetSymbol() == "no";

    if( !value.IsInteger() && !value.IsDouble() )
        return false;

    const double number = value.IsInteger() ? value.GetLongInteger() : value.GetDouble();
    return ( head == "hpglpennumber" && number == 1 ) || ( head == "hpglpenspeed" && number == 20 )
           || ( head == "hpglpendiameter" && number == 15 );
}


inline std::string GoldenSexprDifference( const SEXPR::SEXPR& aExpected, const SEXPR::SEXPR& aActual,
                                          const std::string& aPath = "", const std::string& aParent = "",
                                          size_t aIndex = 0 )
{
    if( aExpected.IsList() && aActual.IsList() )
    {
        const std::string                head = GoldenNodeHead( aExpected );
        const std::string                path = aPath + "/" + head;
        std::vector<const SEXPR::SEXPR*> expected;
        std::vector<const SEXPR::SEXPR*> actual;

        for( const SEXPR::SEXPR* child : *aExpected.GetChildren() )
        {
            if( !IgnoreGoldenNode( *child, head ) )
                expected.push_back( child );
        }

        for( const SEXPR::SEXPR* child : *aActual.GetChildren() )
        {
            if( !IgnoreGoldenNode( *child, GoldenNodeHead( aActual ) ) )
                actual.push_back( child );
        }

        if( expected.size() != actual.size() )
            return path + ": child count " + std::to_string( expected.size() )
                   + " != " + std::to_string( actual.size() );

        for( size_t i = 0; i < expected.size(); ++i )
        {
            std::string difference =
                    GoldenSexprDifference( *expected[i], *actual[i], path + "[" + std::to_string( i ) + "]", head, i );

            if( !difference.empty() )
                return difference;
        }

        return {};
    }

    if( ( aExpected.IsInteger() || aExpected.IsDouble() ) && ( aActual.IsInteger() || aActual.IsDouble() ) )
    {
        const double expected = aExpected.IsInteger() ? aExpected.GetLongInteger() : aExpected.GetDouble();
        const double actual = aActual.IsInteger() ? aActual.GetLongInteger() : aActual.GetDouble();

        // The affine-to-legacy coordinate round trip can round by one internal PCB unit.
        // Permit at most two nanometres in XY coordinates, NOT sizes, angles or settings.
        const bool xy =
                ( aParent == "at" || aParent == "xy" || aParent == "start" || aParent == "end" || aParent == "mid" )
                && ( aIndex == 1 || aIndex == 2 );

        if( expected == actual || ( xy && std::abs( expected - actual ) <= 0.000002001 ) )
            return {};
    }
    else if( aExpected.AsString() == aActual.AsString() )
    {
        return {};
    }

    return aPath + ": expected " + aExpected.AsString() + ", got " + aActual.AsString();
}


inline std::string GoldenFileDifference( const std::string& aExpected, const std::string& aActual )
{
    SEXPR::PARSER parser;
    auto          expected = parser.Parse( ReadGoldenText( aExpected ) );
    auto          actual = parser.Parse( ReadGoldenText( aActual ) );

    if( !expected || !actual )
        throw std::runtime_error( "Empty downgrade golden or output" );

    return GoldenSexprDifference( *expected, *actual );
}

} // namespace KI_TEST
