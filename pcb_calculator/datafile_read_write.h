#ifndef DATAFILE_READ_WRITE_H_
#define DATAFILE_READ_WRITE_H_
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pcb_calculator_datafile_lexer.h>

class PCB_CALCULATOR_DATAFILE_PARSER;

/**
 * PCB_CALCULATOR_DATAFILE
 * handles data to calculate regulators parameters
 */
class PCB_CALCULATOR_DATAFILE
{
public:
    PCB_CALCULATOR_DATAFILE( REGULATOR_LIST * aList );

    int         WriteHeader( OUTPUTFORMATTER* aFormatter ) const ;
    void        Format( OUTPUTFORMATTER* aFormatter, int aNestLevel ) const ;
    void        Parse( PCB_CALCULATOR_DATAFILE_PARSER* aParser );

private:
    friend class PCB_CALCULATOR_DATAFILE_PARSER;

protected:
     REGULATOR_LIST* m_list;
};


/**
 * Parser for PCB_CALCULATOR_DATAFILE.
 */
class PCB_CALCULATOR_DATAFILE_PARSER : public PCB_CALCULATOR_DATAFILE_LEXER
{
public:
    PCB_CALCULATOR_DATAFILE_PARSER( LINE_READER* aReader );
    PCB_CALCULATOR_DATAFILE_PARSER( char* aLine, const wxString& aSource );
    LINE_READER* GetReader() { return reader; };
    void Parse( PCB_CALCULATOR_DATAFILE* aDataList );
    void ParseRegulatorDescr( PCB_CALCULATOR_DATAFILE* aDataList );
};

#endif // PDATAFILE_READ_WRITE_H_
