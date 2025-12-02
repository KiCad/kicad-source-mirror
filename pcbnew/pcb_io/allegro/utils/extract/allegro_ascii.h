/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <convert/allegro_db.h>
#include <utils/extract/extract_spec_parser.h>

#include <reporter.h>
#include <richio.h>

namespace ALLEGRO
{

/**
 *
 */
class ASCII_EXTRACTOR
{
public:
    ASCII_EXTRACTOR( const BRD_DB& aBrd, OUTPUTFORMATTER& aFormatter, REPORTER& aReporter ) :
            m_Brd( aBrd ),
            m_Formatter( aFormatter ),
            m_Reporter( aReporter )
    {
    }

    /**
     * Extract according to a single block specification.
     */
    void Extract( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );

private:
    using OBJECT_VISITOR = std::function<bool( const VIEW_OBJS&, std::vector<wxString>& )>;

    /**
     * Create an object visitor for a given block.
     *
     * This is a function the returns a functor that visits objects,
     * according to the block specification (view tpye and field list) and filters.
     */
    OBJECT_VISITOR createObjectVisitor( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );

    /**
     * USe the given visitor to visit an object and produce a line of output.
     */
    void visitForLine( const VIEW_OBJS& aObj, char aLinePrefix, OBJECT_VISITOR& aVisitor );

    /**
     * Extract symbol instances from an Allegro database.
     *
     * This corresponds to SYMBOL ASCII commands.
     */
    void extractSymbolInstances( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );


    /**
     * Extract board-level information from an Allegro database.
     *
     * This is always added as a 'J' line prefix, though it could also be a view
     * as well with the BOARD view.
     */
    void extractBoardInfo();

    /**
     * Extract board-level information according to a block spec.
     */
    void extractBoardInfo( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock, char aLinePrefix );

    void extractComponentPins( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );

    void extractFunctions( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );

    void extractNets( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );

    void extractFullGeometry( const EXTRACT_SPEC_PARSER::IR::BLOCK& aBlock );

    const BRD_DB&    m_Brd;
    OUTPUTFORMATTER& m_Formatter;

    REPORTER& m_Reporter;
};

} // namespace ALLEGRO
