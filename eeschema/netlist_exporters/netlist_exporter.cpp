/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 jp.charras at wanadoo.fr
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <netlist_exporter.h>

#include <confirm.h>
#include <fctsys.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <refdes_utils.h>

#include <class_library.h>
#include <connection_graph.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <schematic.h>


wxString NETLIST_EXPORTER::MakeCommandLine( const wxString& aFormatString,
            const wxString& aNetlistFile, const wxString& aFinalFile, const wxString& aProjectPath )
{
    // Expand format symbols in the command line:
    // %B => base filename of selected output file, minus path and extension.
    // %P => project directory name, without trailing '/' or '\'.
    // %I => full filename of the input file (the intermediate net file).
    // %O => complete filename and path (but without extension) of the user chosen output file.

    wxString   ret  = aFormatString;
    wxFileName in   = aNetlistFile;
    wxFileName out  = aFinalFile;
    wxString str_out  = out.GetFullPath();

    ret.Replace( "%P", aProjectPath, true );
    ret.Replace( "%B", out.GetName(), true );
    ret.Replace( "%I", in.GetFullPath(), true );

#ifdef __WINDOWS__
    // A ugly hack to run xsltproc that has a serious bug on Window since a long time:
    // the filename given after -o option (output filename) cannot use '\' in filename
    // so replace if by '/' if possible (I mean if the filename does not start by "\\"
    // that is a filename on a Windows server)

    if( !str_out.StartsWith( "\\\\" ) )
        str_out.Replace( "\\", "/" );
#endif

    ret.Replace( "%O", str_out, true );

    return ret;
}


SCH_COMPONENT* NETLIST_EXPORTER::findNextComponent( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref;

    if( aItem->Type() != SCH_COMPONENT_T )
        return nullptr;

    // found next component
    SCH_COMPONENT* comp = (SCH_COMPONENT*) aItem;

    // Power symbols and other components which have the reference starting
    // with "#" are not included in netlist (pseudo or virtual components)
    ref = comp->GetRef( aSheetPath );

    if( ref[0] == wxChar( '#' ) )
        return nullptr;

    // if( Component->m_FlagControlMulti == 1 )
    //    continue;                                      /* yes */
    // removed because with multiple instances of one schematic
    // (several sheets pointing to 1 screen), this will be erroneously be
    // toggled.

    if( !comp->GetPartRef() )
        return nullptr;

    // If component is a "multi parts per package" type
    if( comp->GetPartRef()->GetUnitCount() > 1 )
    {
        // test if this reference has already been processed, and if so skip
        if( m_ReferencesAlreadyFound.Lookup( ref ) )
            return nullptr;
    }

    // record the usage of this library component entry.
    m_LibParts.insert( comp->GetPartRef().get() ); // rejects non-unique pointers

    return comp;
}
