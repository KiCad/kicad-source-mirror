/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2018 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file eeschema/netform.cpp
 * @brief Net list generation code.
 */

#include <fctsys.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <reporter.h>
#include <confirm.h>
#include <kiway.h>

#include <netlist.h>
#include <netlist_exporter.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_pspice.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_generic.h>

#include <invoke_sch_dialog.h>

bool SCH_EDIT_FRAME::WriteNetListFile( NETLIST_OBJECT_LIST* aConnectedItemsList,
                                       int aFormat, const wxString& aFullFileName,
                                       unsigned aNetlistOptions, REPORTER* aReporter )
{
    if( aConnectedItemsList == nullptr )    // Schematic netlist not available.
        return false;

    bool res = true;
    bool executeCommandLine = false;

    wxString    fileName = aFullFileName;

    NETLIST_EXPORTER *helper;

    switch( aFormat )
    {
    case NET_TYPE_PCBNEW:
        helper = new NETLIST_EXPORTER_KICAD( this, aConnectedItemsList,
                                             g_ConnectionGraph );
        break;

    case NET_TYPE_ORCADPCB2:
        helper = new NETLIST_EXPORTER_ORCADPCB2( aConnectedItemsList );
        break;

    case NET_TYPE_CADSTAR:
        helper = new NETLIST_EXPORTER_CADSTAR( aConnectedItemsList );
        break;

    case NET_TYPE_SPICE:
        helper = new NETLIST_EXPORTER_PSPICE( aConnectedItemsList );
        break;

    default:
        {
            wxFileName  tmpFile = fileName;
            tmpFile.SetExt( GENERIC_INTERMEDIATE_NETLIST_EXT );
            fileName = tmpFile.GetFullPath();

            helper = new NETLIST_EXPORTER_GENERIC( this, aConnectedItemsList,
                                                   g_ConnectionGraph );
            executeCommandLine = true;
        }
        break;
    }

    res = helper->WriteNetlist( fileName, aNetlistOptions );
    delete helper;

    // If user provided a plugin command line, execute it.
    if( executeCommandLine && res && !m_netListerCommand.IsEmpty() )
    {
        wxString prj_dir = Prj().GetProjectPath();

        // build full command line from user's format string, e.g.:
        // "xsltproc -o %O /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl %I"
        // becomes, after the user selects /tmp/s1.net as the output file from the file dialog:
        // "xsltproc -o /tmp/s1.net /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl /tmp/s1.xml"
        wxString commandLine = NETLIST_EXPORTER::MakeCommandLine( m_netListerCommand,
                fileName, aFullFileName,
                prj_dir.SubString( 0, prj_dir.Len() - 2 )       // strip trailing '/'
                );

        if( aReporter )
        {
            wxArrayString output, errors;
            int diag = wxExecute( commandLine, output, errors, m_exec_flags );

            wxString msg;

            msg << _( "Run command:" ) << wxT( "\n" ) << commandLine << wxT( "\n\n" );

            aReporter->ReportHead( msg, REPORTER::RPT_ACTION );

            if( diag != 0 )
                aReporter->ReportTail( wxString::Format(
                                    _("Command error. Return code %d" ), diag ),
                                    REPORTER::RPT_ERROR );
            else
                aReporter->ReportTail( _( "Success" ), REPORTER::RPT_INFO );

            if( output.GetCount() )
            {
                msg.Empty();
                msg << wxT( "\n" ) << _( "Info messages:" ) << wxT( "\n" );
                aReporter->Report( msg, REPORTER::RPT_INFO );

                for( unsigned ii = 0; ii < output.GetCount(); ii++ )
                    aReporter->Report( output[ii] + wxT( "\n" ), REPORTER::RPT_INFO );
            }

            if( errors.GetCount() )
            {
                msg.Empty();
                msg << wxT("\n") << _( "Error messages:" ) << wxT( "\n" );
                aReporter->Report( msg, REPORTER::RPT_INFO );

                for( unsigned ii = 0; ii < errors.GetCount(); ii++ )
                    aReporter->Report( errors[ii] + wxT( "\n" ), REPORTER::RPT_ERROR );
            }
        }
        else
            ProcessExecute( commandLine, m_exec_flags );

        DefaultExecFlags(); // Reset flags to default after executing
    }

    return res;
}


//Imported function:
int TestDuplicateSheetNames( bool aCreateMarker );


bool SCH_EDIT_FRAME::prepareForNetlist()
{
    SCH_SCREENS schematic;

    // Ensure all symbol library links for all sheets valid:
    schematic.UpdateSymbolLinks();

    // Ensure all power symbols have a valid reference
    SCH_SHEET_LIST sheets( g_RootSheet );
    sheets.AnnotatePowerSymbols();

    // Performs some controls:
    if( CheckAnnotate( NULL_REPORTER::GetInstance(), 0 ) )
    {
        // Schematic must be annotated: call Annotate dialog and tell the user why.
        ModalAnnotate( _( "Exporting the netlist requires a completely annotated schematic." ) );

        if( CheckAnnotate( NULL_REPORTER::GetInstance(), 0 ) )
            return false;
    }

    // Test duplicate sheet names:
    if( TestDuplicateSheetNames( false ) > 0 )
    {
        if( !IsOK( this, _( "Error: duplicate sheet names. Continue?" ) ) )
            return false;
    }

    return true;
}


void SCH_EDIT_FRAME::sendNetlistToCvpcb()
{
    NETLIST_OBJECT_LIST*   net_atoms = BuildNetListBase();
    NETLIST_EXPORTER_KICAD exporter( this, net_atoms, g_ConnectionGraph );
    STRING_FORMATTER       formatter;

    // @todo : trim GNL_ALL down to minimum for CVPCB
    exporter.Format( &formatter, GNL_ALL );

    std::string packet = formatter.GetString();  // an abbreviated "kicad" (s-expr) netlist
    Kiway().ExpressMail( FRAME_CVPCB, MAIL_EESCHEMA_NETLIST, packet, this );
}


NETLIST_OBJECT_LIST* SCH_EDIT_FRAME::CreateNetlist( bool aSilent,
                                                    bool aSilentAnnotate )
{
    if( !aSilent ) // checks for errors and invokes annotation dialog as neccessary
    {
        if( !prepareForNetlist() )
            return nullptr;
    }
    else // performs similar function as prepareForNetlist but without a dialog.
    {
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks();
        SCH_SHEET_LIST sheets( g_RootSheet );
        sheets.AnnotatePowerSymbols();

        if( aSilentAnnotate )
            AnnotateComponents( true, UNSORTED, INCREMENTAL_BY_REF, 0, false, false, true,
                                NULL_REPORTER::GetInstance() );
    }

    // TODO(JE) This is really going to turn into "PrepareForNetlist"
    // when the old netlister (BuildNetListBase) is removed

    return BuildNetListBase();
}


NETLIST_OBJECT_LIST* SCH_EDIT_FRAME::BuildNetListBase( bool updateStatusText )
{
    // Ensure netlist is up to date
    RecalculateConnections();

    // I own this list until I return it to the new owner.
    std::unique_ptr<NETLIST_OBJECT_LIST> ret( new NETLIST_OBJECT_LIST() );

    // Creates the flattened sheet list:
    SCH_SHEET_LIST aSheets( g_RootSheet );

    // Build netlist info
    bool success = ret->BuildNetListInfo( aSheets );

    if( !success )
    {
        if( updateStatusText )
            SetStatusText( _( "No Objects" ) );
        return ret.release();
    }

    wxString msg = wxString::Format( _( "Net count = %d" ), int( ret->size() ) );

    if( updateStatusText )
         SetStatusText( msg );

    return ret.release();
}

