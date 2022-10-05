/*
 * This program source code file is part of kicad2mcad
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/crt.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <sstream>

#include <cli/exit_codes.h>
#include <pgm_base.h>
#include "kicad2step.h"
#include "pcb/kicadpcb.h"
#include <Message.hxx>                  // OpenCascade messenger
#include <Message_PrinterOStream.hxx>   // OpenCascade output messenger
#include <Standard_Failure.hxx>         // In open cascade

#include <Standard_Version.hxx>

#include <locale_io.h>

#define OCC_VERSION_MIN 0x070500

#if OCC_VERSION_HEX < OCC_VERSION_MIN
#include <Message_Messenger.hxx>
#endif



// Horrible hack until we decouple things more
static KICAD2STEP* k2sInstance = nullptr;


void ReportMessage( const wxString& aMessage )
{
    if( k2sInstance != nullptr )
        k2sInstance->ReportMessage( aMessage );
}


class KiCadPrinter : public Message_Printer
{
public:
    KiCadPrinter( KICAD2STEP* aConverter ) : m_converter( aConverter ) {}

protected:
#if OCC_VERSION_HEX < OCC_VERSION_MIN
    virtual void Send( const TCollection_ExtendedString& theString,
                       const Message_Gravity theGravity,
                       const Standard_Boolean theToPutEol ) const override
    {
        Send( TCollection_AsciiString( theString ), theGravity, theToPutEol );
    }

    virtual void Send( const TCollection_AsciiString& theString,
                       const Message_Gravity theGravity,
                       const Standard_Boolean theToPutEol ) const override
#else
        virtual void send( const TCollection_AsciiString& theString,
                           const Message_Gravity theGravity ) const override
#endif
    {
      if( theGravity >= Message_Info )
      {
          m_converter->ReportMessage( theString.ToCString() );

#if OCC_VERSION_HEX < OCC_VERSION_MIN
          if( theToPutEol )
              ReportMessage( wxT( "\n" ) );
#else
          m_converter->ReportMessage( wxT( "\n" ) );
#endif
      }

      if( theGravity >= Message_Alarm )
          m_converter->SetError();

      if( theGravity == Message_Fail )
          m_converter->SetFail();
    }

private:
    KICAD2STEP* m_converter;
};


KICAD2MCAD_PRMS::KICAD2MCAD_PRMS()
{
#ifdef SUPPORTS_IGES
    m_fmtIGES = false;
#endif
    m_overwrite = false;
    m_useGridOrigin = false;
    m_useDrillOrigin = false;
    m_includeVirtual = true;
    m_substModels = false;
    m_xOrigin = 0.0;
    m_yOrigin = 0.0;
    m_minDistance = MIN_DISTANCE;

}


wxString KICAD2MCAD_PRMS::getOutputExt() const
{
#ifdef SUPPORTS_IGES
    if( m_fmtIGES )
        return wxT( "igs" );
    else
#endif
        return wxT( "step" );
}


KICAD2STEP::KICAD2STEP( KICAD2MCAD_PRMS aParams ) :
        m_params( aParams ), m_error( false ), m_fail( false )
{
}


int KICAD2STEP::DoRun()
{
    wxFileName fname( m_params.m_filename );

    if( !fname.FileExists() )
    {
        ReportMessage( wxString::Format(  _( "No such file: %s" ), m_params.m_filename ) );

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    wxFileName out_fname;

    if( m_params.m_outputFile.empty() )
    {
        out_fname.Assign( fname.GetFullPath() );
        out_fname.SetExt( m_params.getOutputExt() );
    }
    else
    {
        out_fname.Assign( m_params.m_outputFile );

        // Set the file extension if the user's requested file name does not have an extension.
        if( !out_fname.HasExt() )
            out_fname.SetExt( m_params.getOutputExt() );
    }

    if( out_fname.FileExists() && !m_params.m_overwrite )
    {
        ReportMessage( _( "** Output already exists. Export aborted. **\n"
                          "Enable the force overwrite flag to overwrite it." ) );

        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    LOCALE_IO dummy;

    wxString outfile = out_fname.GetFullPath();
    KICADPCB pcb( fname.GetName() );

    pcb.SetOrigin( m_params.m_xOrigin, m_params.m_yOrigin );
    // Set the min dist in mm to consider 2 points at the same place
    // This is also the tolerance to consider 2 lines or arcs are connected
    // A min value (0.001mm) is needed to have closed board outlines
    // 0.01 mm is a good value
    pcb.SetMinDistance( std::max( m_params.m_minDistance, MIN_ACCEPTABLE_DISTANCE ) );
    ReportMessage( wxString::Format( _( "Read file: '%s'\n" ), m_params.m_filename ) );

    Message::DefaultMessenger()->RemovePrinters( STANDARD_TYPE( Message_PrinterOStream ) );
    Message::DefaultMessenger()->AddPrinter( new KiCadPrinter( this ) );

    if( pcb.ReadFile( m_params.m_filename ) )
    {
        if( m_params.m_useDrillOrigin )
            pcb.UseDrillOrigin( true );

        if( m_params.m_useGridOrigin )
            pcb.UseGridOrigin( true );

        bool res;

        try
        {
            ReportMessage( _( "Build STEP data\n" ) );

            res = pcb.ComposePCB( m_params.m_includeVirtual, m_params.m_substModels );

            if( !res )
            {
                ReportMessage( _( "\n** Error building STEP board model. Export aborted. **\n" ) );

                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }

            ReportMessage( _( "Write STEP file\n" ) );

#ifdef SUPPORTS_IGES
            if( m_fmtIGES )
                res = pcb.WriteIGES( outfile );
            else
#endif
                res = pcb.WriteSTEP( outfile );

            if( !res )
            {
                ReportMessage( _( "\n** Error writing STEP file. **\n" ) );
                return CLI::EXIT_CODES::ERR_UNKNOWN;
            }
            else
            {
                ReportMessage( wxString::Format( _( "\nSTEP file '%s' created.\n" ), outfile ) );
            }
        }
        catch( const Standard_Failure& e )
        {
            ReportMessage( e.GetMessageString() );
            ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }
        catch( ... )
        {
            ReportMessage( _( "\n** Error exporting STEP file. Export aborted. **\n" ) );
            return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
        }
    }
    else
    {
        ReportMessage( _( "\n** Error reading kicad_pcb file. **\n" ) );
        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    wxString msg;

    if( m_fail )
    {
        msg = _( "Unable to create STEP file.\n"
                 "Check that the board has a valid outline and models." );
    }
    else if( m_error )
    {
        msg = _( "STEP file has been created, but there are warnings." );
    }

    ReportMessage( msg );

    return CLI::EXIT_CODES::SUCCESS;
}


int KICAD2STEP::Run()
{
    k2sInstance = this;

    int diag = DoRun();
    return diag;
}


void KICAD2STEP::ReportMessage( const wxString& aMessage )
{
     wxPrintf( aMessage );
}
