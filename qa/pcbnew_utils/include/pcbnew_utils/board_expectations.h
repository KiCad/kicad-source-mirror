/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once


#include <memory>
#include <string>
#include <vector>

#include <json_common.h>


class BOARD;

namespace KI_TEST
{

/**
 * A single expectation about a board, which can be run as a test against a parsed BOARD.
 */
class BOARD_EXPECTATION
{
public:
    // This class can be handled via the base class pointer
    virtual ~BOARD_EXPECTATION() = default;

    virtual void RunTest( const BOARD& aBrd ) const = 0;

    virtual std::string GetName() const = 0;

    /**
     * Set a comment to be included in the test output for this expectation, which can be used to provide more
     * details about the expectation.
     */
    void               SetComment( std::string aComment ) { m_Comment = std::move( aComment ); }
    const std::string& GetComment() const { return m_Comment; }

private:
    std::string m_Comment;
};


class BOARD_EXPECTATION_TEST
{
public:
    /**
     * Lightweight descriptor for a BOARD_EXPECTATION_TEST, which can be used to refer to the test
     * unambiguously. Intended to be used for registering the test with the test runner at static init.
     */
    struct DESCRIPTOR
    {
        /// If the test has a name, it's that, else an index - this is for naming the test for filtering
        std::string m_TestName;
        /// Tags associated with the test, which can be used for filtering
        std::vector<std::string> m_Tags;
        /// Handy ref to the JSON entry for this expectations test, which saves looking it up again
        const nlohmann::json& m_TestJson;
    };

    BOARD_EXPECTATION_TEST( const std::string& aBrdName ) :
            m_BrdName( aBrdName )
    {
    }

    /**
     * Runs the test against the given board
     */
    void RunTest( const BOARD& aBrd ) const;

    static std::unique_ptr<BOARD_EXPECTATION_TEST> CreateFromJson( const std::string&    aBrdName,
                                                                   const nlohmann::json& aBrdExpectations );

    /**
     * Extracts expectation tests from the given JSON array and returns a list of test references that can be used to run the tests.
     *
     * This is intended to be used to extract the expectation tests from the JSON at static init time to
     * register the tests with the test runner.
     * This does not actually create the expectation test objects, just extracts the minimal information needed
     * to register them.
     */
    static std::vector<DESCRIPTOR> ExtractExpectationTestsFromJson( const nlohmann::json& aExpectationArray );

    /**
     * Constructs a BOARD_EXPECTATION_TEST from the given JSON definition, and runs it on the given board.
     */
    static void RunFromRef( const std::string& aBrdName, const BOARD& aBoard,
                            const BOARD_EXPECTATION_TEST::DESCRIPTOR& aExpectationTestRef );

private:
    std::unique_ptr<BOARD_EXPECTATION> m_expectation;
    std::string                        m_BrdName;
    bool                               m_skip = false;
};

} // namespace KI_TEST
