/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef CLI_COMMAND_H
#define CLI_COMMAND_H

#include <argparse/argparse.hpp>
#include <kiway.h>

#define UTF8STDSTR( s ) ( std::string( s.utf8_str() ) )

#define ARG_VERSION "--version"
#define ARG_HELP "--help"
#define ARG_HELP_SHORT "-h"
#define ARG_HELP_DESC _( "Shows help message and exits" )
#define ARG_OUTPUT "--output"
#define ARG_INPUT "input"
#define ARG_DRAWING_SHEET "--drawing-sheet"
#define ARG_DEFINE_VAR_SHORT "-D"
#define ARG_DEFINE_VAR_LONG "--define-var"
#define ARG_VARIANT "--variant"

namespace CLI
{

class COMMAND
{
public:
    /**
    * Define a new COMMAND instance
    *
    * @param aName The name of the command that is to be used in the cli interface
    */
    COMMAND( const std::string& aName );

    /**
    * Entry point to processing commands from args and doing work
    */
    int Perform( KIWAY& aKiway );

    virtual ~COMMAND() = default;

    argparse::ArgumentParser& GetArgParser() { return m_argParser; }
    const std::string&        GetName() const { return m_name; }

    void PrintHelp();

protected:
    /**
     * Set up the most common of args used across cli
     *
     * @param aInput Configures the input arg
     * @param aOutput Configures the output arg
     * @param aInputCanBeDir Configures whether the input arg description will be for either a
     *                       file or directory
     * @param aOutputIsDir Configures whether the output arg description will be for a file or
     *                     directory
     */
    void addCommonArgs( bool aInput, bool aOutput, bool aInputCanBeDir, bool aOutputIsDir );

    /**
     * Set up the drawing sheet arg used by many of the export commands
     */
    void addDrawingSheetArg();

    /**
     * Set up the drawing sheet arg used by many of the export commands
     */
    void addDefineArg();

    /**
     * Set up the list of variants to output arguement.
     */
    void addVariantsArg();

    /**
     * The internal handler that should be overloaded to implement command specific
     * processing and work.
     *
     * If not overloaded, the command will simply emit the help options by default
     */
    virtual int              doPerform( KIWAY& aKiway );

    /**
     * Name of this command that is exported and used in the cli
     */
    std::string              m_name;

    argparse::ArgumentParser m_argParser;

    /**
     * Whether or not the input arg was added for parsing
     */
    bool                     m_hasInputArg;

    /**
     * Whether or not the output arg was added for parsing
     */
    bool                     m_hasOutputArg;

    /**
     * Whether or not the input arg was added for parsing
     */
    bool                     m_hasDrawingSheetArg;

    /**
     * Whether or not the input arg was added for parsing
     */
    bool                     m_hasDefineArg;

    /**
     * Whether or not the output arg is expecting a directory
     */
    bool                     m_outputArgExpectsDir;

    /**
     * Value of the common input arg if configured
     */
    wxString                 m_argInput;

    /**
     * Value of the output arg if configured
     */
    wxString                 m_argOutput;

    /**
     * Value of the drawing sheet arg if configured
     */
    wxString                 m_argDrawingSheet;

    /**
     * Value of the drawing sheet arg if configured
     */
    std::map<wxString, wxString>    m_argDefineVars;

    /**
     * Whether or not the input argument for variant names was added for parsing.
     */
    bool                     m_hasVariantArg;

    /**
     * The list of variant names to output.
     *
     * An empty vector indicates the default variant.
     */
    std::vector<wxString>    m_argVariantNames;
};

}

#endif
