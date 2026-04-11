#ifndef PCB_PLOT_PARAMS_PARSER_H_
#define PCB_PLOT_PARAMS_PARSER_H_
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcb_plot_params_lexer.h>


class wxString;
class PCB_PLOT_PARAMS;
class LINE_READER;


/**
 * The parser for PCB_PLOT_PARAMS.
 */
class PCB_PLOT_PARAMS_PARSER : public PCB_PLOT_PARAMS_LEXER
{
public:
    PCB_PLOT_PARAMS_PARSER( LINE_READER* aReader, int aBoardFileVersion );
    PCB_PLOT_PARAMS_PARSER( char* aLine, const wxString& aSource );

    LINE_READER* GetReader() { return reader; };

    void Parse( PCB_PLOT_PARAMS* aPcbPlotParams );

private:
    bool parseBool();

    /**
     * Parse an integer and constrains it between two values.
     *
     * @param aMin is the smallest return value.
     * @param aMax is the largest return value.
     * @return the parsed integer.
     */
    int parseInt( int aMin, int aMax );

    /**
     * Parse a double precision floating point number.
     *
     * @return the parsed double.
     */
    double parseDouble();

    /**
     * Skip the current token level.
     *
     * Search for the RIGHT parenthesis which closes the current description.
     */
    void skipCurrent();

    int m_boardFileVersion;
};

#endif // PCB_PLOT_PARAMS_PARSER_H_
