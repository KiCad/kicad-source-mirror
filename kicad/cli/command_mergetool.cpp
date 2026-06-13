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

#include "command_mergetool.h"

#include <cli/exit_codes.h>
#include <diff_merge/diff_doc_kind.h>
#include <kiface_base.h>
#include <kiway.h>
#include <paths.h>
#include <string_utils.h>

#include <wx/crt.h>
#include <wx/filename.h>
#include <wx/utils.h>


#define ARG_ANCESTOR "ancestor"
#define ARG_OURS     "ours"
#define ARG_THEIRS   "theirs"
#define ARG_OUTPUT_F "--output"


/// Map kicad-side exit codes to the Phase 8 mergetool contract documented in
/// the header. Anything not explicitly remapped passes through so the user
/// can still diagnose unexpected codes.
static int translateExitCode( int aKicadExit )
{
    using namespace CLI::EXIT_CODES;

    switch( aKicadExit )
    {
    case SUCCESS:                       return 0;
    case ERR_RC_VIOLATIONS:             return 2;   // unresolved conflicts
    case ERR_INVALID_INPUT_FILE:
    case ERR_INVALID_OUTPUT_CONFLICT:   return 3;   // I/O or parse error
    default:                            return aKicadExit;
    }
}


static bool supportsMergetoolKind( KICAD_DIFF::DOC_KIND aKind )
{
    switch( aKind )
    {
    case KICAD_DIFF::DOC_KIND::PCB:
    case KICAD_DIFF::DOC_KIND::SCH:
    case KICAD_DIFF::DOC_KIND::SYM_LIB:
    case KICAD_DIFF::DOC_KIND::FOOTPRINT:
        return true;

    default:
        return false;
    }
}


CLI::MERGETOOL_COMMAND::MERGETOOL_COMMAND() :
        COMMAND( "mergetool" )
{
    m_argParser.add_description(
            UTF8STDSTR( _( "Three-way merge driver for `git mergetool`. Detects PCB vs "
                           "schematic vs library item from the output extension, opens "
                           "the resolution dialog in the GUI kicad binary for any "
                           "unresolved conflicts where available, "
                           "and writes the merged file." ) ) );

    m_argParser.add_argument( ARG_ANCESTOR )
            .help( UTF8STDSTR( _( "Common-ancestor file (.kicad_pcb, .kicad_sch, "
                                  ".kicad_sym, or .kicad_mod)" ) ) )
            .metavar( "ANCESTOR" );

    m_argParser.add_argument( ARG_OURS )
            .help( UTF8STDSTR( _( "'Ours' file" ) ) )
            .metavar( "OURS" );

    m_argParser.add_argument( ARG_THEIRS )
            .help( UTF8STDSTR( _( "'Theirs' file" ) ) )
            .metavar( "THEIRS" );

    m_argParser.add_argument( "-o", ARG_OUTPUT_F )
            .required()
            .help( UTF8STDSTR( _( "Output path; extension picks the document type" ) ) )
            .metavar( "PATH" );
}


int CLI::MERGETOOL_COMMAND::doPerform( KIWAY& /* aKiway */ )
{
    const wxString ancestor = From_UTF8( m_argParser.get<std::string>( ARG_ANCESTOR ).c_str() );
    const wxString ours     = From_UTF8( m_argParser.get<std::string>( ARG_OURS ).c_str() );
    const wxString theirs   = From_UTF8( m_argParser.get<std::string>( ARG_THEIRS ).c_str() );
    const wxString output   = From_UTF8( m_argParser.get<std::string>( ARG_OUTPUT_F ).c_str() );

    if( output.IsEmpty() )
    {
        wxFprintf( stderr, _( "--output is required\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    const KICAD_DIFF::DOC_KIND kind = KICAD_DIFF::DocKindFromExtension( output );

    if( !supportsMergetoolKind( kind ) )
    {
        wxFprintf( stderr,
                   _( "Unsupported output extension '%s'. "
                      "Expected .kicad_pcb, .kicad_sch, .kicad_sym, or .kicad_mod\n" ),
                   output );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;   // contract code 3 — bad input
    }

    // The conflict-resolution dialog needs a GUI, which kicad-cli doesn't
    // provide. Re-exec the main `kicad` binary in --mergetool mode so the
    // user gets the interactive dialog. This is the documented Phase 8
    // contract for `git mergetool`.
    const wxString kicadBinPath = PATHS::ResolveSiblingExecutable( wxT( "kicad" ) );

    if( kicadBinPath.IsEmpty() )
    {
        wxFprintf( stderr,
                   _( "Cannot find the main kicad binary alongside kicad-cli.\n" ) );
        return 4;   // contract code 4 — initialization failure
    }

    // Pass argv as an array so paths containing spaces or quotes aren't
    // mis-parsed by the string-form wxExecute's shell-style tokenizer.
    // wxExecute's array form needs c_str()-backed pointers, so we hold
    // wxCharBuffer / wxScopedCharBuffer locals to keep them alive.
    const wxScopedCharBuffer args[] = {
            kicadBinPath.utf8_str(),
            wxScopedCharBuffer::CreateNonOwned( "--mergetool" ),
            ancestor.utf8_str(),
            ours.utf8_str(),
            theirs.utf8_str(),
            output.utf8_str()
    };

    const char* argv[] = { args[0].data(), args[1].data(), args[2].data(),
                           args[3].data(), args[4].data(), args[5].data(),
                           nullptr };

    const long rc = wxExecute( const_cast<char**>( argv ), wxEXEC_SYNC );

    if( rc < 0 )
    {
        wxFprintf( stderr, _( "Failed to launch '%s'\n" ), kicadBinPath );
        return 4;
    }

    return translateExitCode( static_cast<int>( rc ) );
}
