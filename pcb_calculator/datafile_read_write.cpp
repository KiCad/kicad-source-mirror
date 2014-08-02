/**
 * @file datafile_read_write.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <macros.h>
#include <common.h>
#include <kicad_string.h>
#include <pgm_base.h>
#include <pcb_calculator.h>
#include <pcb_calculator_datafile_lexer.h>
#include <class_regulator_data.h>
#include <datafile_read_write.h>
#include <build_version.h>


using namespace PCBCALC_DATA_T;


static const char* getTokenName( T aTok )
{
    return PCB_CALCULATOR_DATAFILE_LEXER::TokenName( aTok );
}


bool PCB_CALCULATOR_FRAME::ReadDataFile()
{
    FILE* file = wxFopen( GetDataFilename(), wxT( "rt" ) );

    if( file == NULL )
        return false;

    // Switch the locale to standard C (needed to read/write floating point numbers
    LOCALE_IO   toggle;

    PCB_CALCULATOR_DATAFILE * datafile = new PCB_CALCULATOR_DATAFILE( &m_RegulatorList );

   // dataReader dtor will close file
    FILE_LINE_READER dataReader( file, GetDataFilename() );
    PCB_CALCULATOR_DATAFILE_PARSER datafile_parser( &dataReader );

    try
    {
        datafile_parser.Parse( datafile );
    }
    catch( const IO_ERROR& ioe )
    {
        delete datafile;

        wxString msg = ioe.errorText;

        msg += wxChar('\n');
        msg += _("Data file error.");

        wxMessageBox( msg );
        return false;
    }

    delete datafile;

    return true;
}

bool PCB_CALCULATOR_FRAME::WriteDataFile()
{
    // Switch the locale to standard C (needed to read/write floating point numbers
    LOCALE_IO   toggle;

    std::auto_ptr<PCB_CALCULATOR_DATAFILE> datafile( new PCB_CALCULATOR_DATAFILE( &m_RegulatorList ) );

    try
    {
        FILE_OUTPUTFORMATTER    formatter( GetDataFilename() );

        int nestlevel = datafile->WriteHeader( &formatter );

        datafile->Format( &formatter, nestlevel );

        while( nestlevel-- )
            formatter.Print( nestlevel, ")\n" );
    }
    catch( const IO_ERROR& ioe )
    {
        return false;
    }

    m_RegulatorListChanged = false;
    return true;
}


PCB_CALCULATOR_DATAFILE::PCB_CALCULATOR_DATAFILE( REGULATOR_LIST * aList )
{
    m_list = aList;
}

static const char* regtype_str[] =
{
    "normal", "3terminal"
};

int PCB_CALCULATOR_DATAFILE::WriteHeader( OUTPUTFORMATTER* aFormatter ) const throw( IO_ERROR )
{
    int nestlevel = 0;
    aFormatter->Print( nestlevel++, "(datafile\n");
    aFormatter->Print( nestlevel++, "(version 1)\n" );
    aFormatter->Print( nestlevel++, "(date %s)\n",
                       aFormatter->Quotew( DateAndTime() ).c_str() );
    aFormatter->Print( nestlevel++, "(tool %s)\n",
                       aFormatter->Quotew( Pgm().App().GetAppName() +
                                            wxChar(' ') + GetBuildVersion() ).c_str() );

    return nestlevel;
}

void PCB_CALCULATOR_DATAFILE::Format( OUTPUTFORMATTER* aFormatter,
                              int aNestLevel ) const throw( IO_ERROR )
{
    // Write regulators list:
    aFormatter->Print( aNestLevel++, "(%s\n", getTokenName( T_regulators ) );
    for( unsigned ii = 0; ii < m_list->m_List.size(); ii++ )
    {
        REGULATOR_DATA * item = m_list->m_List[ii];
        aFormatter->Print( aNestLevel, "(%s %s\n", getTokenName( T_regulator ),
                           aFormatter->Quotew(item->m_Name ).c_str() );
        aFormatter->Print( aNestLevel+1, "(%s %g)\n", getTokenName( T_reg_vref ),
                           item->m_Vref );
        if( item->m_Iadj != 0 && item->m_Type == 1)
        {
            aFormatter->Print( aNestLevel+1, "(%s %g)\n", getTokenName( T_reg_iadj ),
                               item->m_Iadj );
        }
        aFormatter->Print( aNestLevel+1, "(%s %s)\n", getTokenName( T_reg_type ),
                           regtype_str[item->m_Type] );
        aFormatter->Print( aNestLevel, ")\n" );
   }
    aFormatter->Print( --aNestLevel, ")\n" );
}


void PCB_CALCULATOR_DATAFILE::Parse( PCB_CALCULATOR_DATAFILE_PARSER* aParser )
                              throw( IO_ERROR, PARSE_ERROR )
{
    aParser->Parse( this );
}



// PCB_CALCULATOR_DATAFILE_PARSER

PCB_CALCULATOR_DATAFILE_PARSER::PCB_CALCULATOR_DATAFILE_PARSER( LINE_READER* aReader ) :
    PCB_CALCULATOR_DATAFILE_LEXER( aReader )
{
}


PCB_CALCULATOR_DATAFILE_PARSER::PCB_CALCULATOR_DATAFILE_PARSER( char* aLine, wxString aSource ) :
    PCB_CALCULATOR_DATAFILE_LEXER( aLine, aSource )
{
}


void PCB_CALCULATOR_DATAFILE_PARSER::Parse( PCB_CALCULATOR_DATAFILE* aDataList ) throw( IO_ERROR, PARSE_ERROR )
{
    T token;
    while( ( token = NextTok() ) != T_EOF)
    {

        if( token == T_LEFT )
        {
            token = NextTok();

            if( token == T_regulators )
            {
                ParseRegulatorDescr( aDataList );
                continue;
            }
        }
    }
}

void PCB_CALCULATOR_DATAFILE_PARSER::ParseRegulatorDescr( PCB_CALCULATOR_DATAFILE* aDataList )
    throw( IO_ERROR, PARSE_ERROR )
{
    T token;
    wxString name;
    double vref, iadj;
    int type;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
            Unexpected( T_EOF );

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_regulator )
        {
            type = 0;
            vref = 0.0;

            // Read name
            token = NextTok();
            name = FROM_UTF8( CurText() );

            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_EOF)
                    Unexpected( T_EOF );

                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_reg_vref:   // the voltage reference value
                    token = NextTok();
                    if( token != T_NUMBER )
                        Expecting( T_NUMBER );
                    sscanf( CurText(), "%lf" , &vref);
                    NeedRIGHT();
                    break;

                case T_reg_iadj:   // the Iadj reference value
                    token = NextTok();
                    if( token != T_NUMBER )
                        Expecting( T_NUMBER );
                    sscanf( CurText(), "%lf" , &iadj);
                    NeedRIGHT();
                    break;

                case T_reg_type:   // type: normal or 3 terminal reg
                    token = NextTok();
                   if( stricmp( CurText(), regtype_str[0] ) == 0 )
                        type = 0;
                    else if( stricmp( CurText(), regtype_str[1] ) == 0 )
                        type = 1;
                    else
                        Unexpected( CurText() );
                    NeedRIGHT();
                    break;

                default:
                    Unexpected( CurText() );
                    break;
                }
            }

            if( ! name.IsEmpty() )
            {
                if( type != 1 )
                    iadj = 0.0;
                REGULATOR_DATA * new_item = new REGULATOR_DATA(name, vref, type, iadj );
                aDataList->m_list->Add( new_item );
            }
        }
    }
}
