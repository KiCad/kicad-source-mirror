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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24330

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <locale_io.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_pads.h>
#include <reporter.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <wx/ffile.h>
#include <wx/filename.h>


struct ISSUE24330_FIXTURE
{
    ISSUE24330_FIXTURE() = default;

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


template <typename Exporter>
static wxString writeAndReadNetlist( SCHEMATIC* aSch, const wxString& aSuffix,
                                     const wxString& aExt )
{
    wxFileName netFile = aSch->Project().GetProjectFullName();
    netFile.SetName( netFile.GetName() + aSuffix );
    netFile.SetExt( aExt );

    WX_STRING_REPORTER       reporter;
    std::unique_ptr<Exporter> exporter = std::make_unique<Exporter>( aSch );

    BOOST_REQUIRE( exporter->WriteNetlist( netFile.GetFullPath(), 0, reporter ) );
    BOOST_REQUIRE( reporter.GetMessages().IsEmpty() );

    wxString contents;
    {
        wxFFile fp( netFile.GetFullPath(), wxT( "rb" ) );
        BOOST_REQUIRE( fp.IsOpened() );
        fp.ReadAll( &contents );
    }

    wxRemoveFile( netFile.GetFullPath() );

    return contents;
}


BOOST_FIXTURE_TEST_SUITE( Issue24330NetlistPinTruncation, ISSUE24330_FIXTURE )


BOOST_AUTO_TEST_CASE( OrcadPCB2_PreservesLongPinNumbers )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "netlists/issue24330/issue24330" ),
                            m_schematic );

    wxString netlist = writeAndReadNetlist<NETLIST_EXPORTER_ORCADPCB2>(
            m_schematic.get(), wxT( "_test_orcad" ), wxT( "net" ) );

    // OrcadPCB2 emits "( <pin_num> <net_name> )", so the trailing space distinguishes a
    // genuine pin-number token from a coincidental substring inside a net name.
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "AAB10 " ) ),
                         "OrcadPCB2: pin number AAB10 missing or truncated" );
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "AAB11 " ) ),
                         "OrcadPCB2: pin number AAB11 missing or truncated" );
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "AAB12 " ) ),
                         "OrcadPCB2: pin number AAB12 missing or truncated" );
}


BOOST_AUTO_TEST_CASE( Cadstar_PreservesLongPinNumbers )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "netlists/issue24330/issue24330" ),
                            m_schematic );

    wxString netlist = writeAndReadNetlist<NETLIST_EXPORTER_CADSTAR>(
            m_schematic.get(), wxT( "_test_cadstar" ), wxT( "frp" ) );

    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "AAB10" ) ),
                         "CadStar: pin number AAB10 missing or truncated" );
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "AAB11" ) ),
                         "CadStar: pin number AAB11 missing or truncated" );
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "AAB12" ) ),
                         "CadStar: pin number AAB12 missing or truncated" );
}


BOOST_AUTO_TEST_CASE( PADS_PreservesLongPinNumbers )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "netlists/issue24330/issue24330" ),
                            m_schematic );

    wxString netlist = writeAndReadNetlist<NETLIST_EXPORTER_PADS>(
            m_schematic.get(), wxT( "_test_pads" ), wxT( "asc" ) );

    // PADS uses "REF.PIN" connection format; check the full pin numbers are emitted.
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "U1.AAB10" ) ),
                         "PADS: pin connection U1.AAB10 missing or truncated" );
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "U1.AAB11" ) ),
                         "PADS: pin connection U1.AAB11 missing or truncated" );
    BOOST_CHECK_MESSAGE( netlist.Contains( wxT( "U1.AAB12" ) ),
                         "PADS: pin connection U1.AAB12 missing or truncated" );
}


BOOST_AUTO_TEST_SUITE_END()
