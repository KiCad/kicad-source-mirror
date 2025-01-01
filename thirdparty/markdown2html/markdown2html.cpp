/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <macros.h>
#include <string>
#include <string_utils.h>

#include "markdown.h"
#include "html.h"
#include "buffer.h"


void ConvertMarkdown2Html( const wxString& aMarkdownInput, wxString& aHtmlOutput )
{
    std::string markdownInput( TO_UTF8( aMarkdownInput ) );

    /* performing markdown parsing */
    struct sd_callbacks callbacks;
    struct html_renderopt   options;

#define OUTPUT_UNIT 64
    struct buf* ob = bufnew( OUTPUT_UNIT );

    sdhtml_renderer( &callbacks, &options, 0 );
    struct sd_markdown* markdown = sd_markdown_new( MKDEXT_TABLES, 16, &callbacks, &options );

    sd_markdown_render( ob, (uint8_t*)markdownInput.data(), markdownInput.size(), markdown );
    sd_markdown_free( markdown );

    std::string out( (char*)ob->data, ob->size );
    aHtmlOutput = From_UTF8( out.data() );

    /* cleanup */
    bufrelease( ob );

}
