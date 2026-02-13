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
    // This class c an be handled via the base class pointer
    virtual ~BOARD_EXPECTATION() = default;

    virtual void RunTest( const BOARD& aBrd ) const = 0;

    virtual std::string GetName() const = 0;
};


class BOARD_EXPECTATION_TEST
{
public:
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

private:
    std::vector<std::unique_ptr<BOARD_EXPECTATION>> m_expectations;

    std::string m_BrdName;
};

} // namespace KI_TEST
