/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <string>
#include <iostream>

#include <fctsys.h>
#include <macros.h>

#include "md2html/parser.h"


void ConvertMarkdown2Html( const wxString& aMarkdownInput, wxString& aHtmlOutput )
{
    std::stringstream markdownInput( TO_UTF8( aMarkdownInput ) );

    std::shared_ptr<maddy::Parser> parser = std::make_shared<maddy::Parser>();
    std::string htmlOutput = parser->Parse(markdownInput);

    aHtmlOutput = FROM_UTF8( htmlOutput.c_str() );
}
