/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <schframe.h>
#include <reporter.h>

#include <netlist.h>
#include <netlist_exporter.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_pspice.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_generic.h>

bool SCH_EDIT_FRAME::WriteNetListFile( NETLIST_OBJECT_LIST* aConnectedItemsList,
                                       int aFormat, const wxString& aFullFileName,
                                       unsigned aNetlistOptions, REPORTER* aReporter )
{
    bool res = true;
    bool executeCommandLine = false;

    wxString    fileName = aFullFileName;

    NETLIST_EXPORTER *helper;

    switch( aFormat )
    {
    case NET_TYPE_PCBNEW:
        helper = new NETLIST_EXPORTER_KICAD( aConnectedItemsList, Prj().SchLibs() );
        break;

    case NET_TYPE_ORCADPCB2:
        helper = new NETLIST_EXPORTER_ORCADPCB2( aConnectedItemsList, Prj().SchLibs() );
        break;

    case NET_TYPE_CADSTAR:
        helper = new NETLIST_EXPORTER_CADSTAR( aConnectedItemsList, Prj().SchLibs() );
        break;

    case NET_TYPE_SPICE:
        helper = new NETLIST_EXPORTER_PSPICE( aConnectedItemsList, Prj().SchLibs() );
        break;

    default:
        {
            wxFileName  tmpFile = fileName;
            tmpFile.SetExt( GENERIC_INTERMEDIATE_NETLIST_EXT );
            fileName = tmpFile.GetFullPath();

            helper = new NETLIST_EXPORTER_GENERIC( aConnectedItemsList, Prj().SchLibs() );
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
            int diag = wxExecute (commandLine, output, errors, wxEXEC_SYNC );

            wxString msg;

            msg << _("Run command:") << wxT("\n") << commandLine << wxT("\n\n");

            aReporter->Report( msg, REPORTER::RPT_ACTION );

            if( diag != 0 )
                aReporter->Report( wxString::Format( _("Command error. Return code %d"), diag ), REPORTER::RPT_ERROR );
            else
                aReporter->Report( _("Success"), REPORTER::RPT_INFO );

            *aReporter << wxT("\n");

            if( output.GetCount() )
            {
                msg << wxT("\n") << _("Info messages:") << wxT("\n");
                aReporter->Report( msg, REPORTER::RPT_INFO );

                for( unsigned ii = 0; ii < output.GetCount(); ii++ )
                    aReporter->Report( output[ii], REPORTER::RPT_INFO );
            }

            if( errors.GetCount() )
            {
                msg << wxT("\n") << _("Error messages:") << wxT("\n");
                aReporter->Report( msg, REPORTER::RPT_INFO );

                for( unsigned ii = 0; ii < errors.GetCount(); ii++ )
                    aReporter->Report( errors[ii], REPORTER::RPT_ERROR );

            }
        }
        else
            ProcessExecute( commandLine, wxEXEC_SYNC );
    }

    return res;
}
