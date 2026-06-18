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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <pin_map.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <connection_graph.h>
#include <settings/settings_manager.h>
#include <netlist_exporters/netlist_exporter_base.h>


namespace
{
LIB_ID makeFp( const wxString& aLibNick, const wxString& aName )
{
    LIB_ID id;
    id.SetLibNickname( aLibNick );
    id.SetLibItemName( aName );
    return id;
}


/// Minimal concrete exporter that exposes the protected CreatePinList() for testing.
class TEST_NETLIST_EXPORTER : public NETLIST_EXPORTER_BASE
{
public:
    explicit TEST_NETLIST_EXPORTER( SCHEMATIC* aSchematic ) : NETLIST_EXPORTER_BASE( aSchematic ) {}

    std::vector<PIN_INFO> Pins( SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aPath )
    {
        return CreatePinList( aSymbol, aPath, true );
    }
};
} // namespace


struct PIN_MAP_NETLIST_FIXTURE
{
    PIN_MAP_NETLIST_FIXTURE()
    {
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_settingsManager.Prj() );
        m_schematic->Reset();

        SCH_SHEET*  defaultSheet = m_schematic->GetTopLevelSheet( 0 );
        SCH_SHEET*  root = new SCH_SHEET( m_schematic.get() );
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
        root->SetScreen( screen );

        m_schematic->AddTopLevelSheet( root );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    SETTINGS_MANAGER            m_settingsManager;
    std::unique_ptr<SCHEMATIC>  m_schematic;
    std::unique_ptr<LIB_SYMBOL> m_libSym;
};


BOOST_FIXTURE_TEST_SUITE( PinMapNetlist, PIN_MAP_NETLIST_FIXTURE )


// The headline verification (plan step 3): an op-amp pin mapped to "[4,9]" for a DFN-8-EP package
// must produce netlist entries for BOTH pad 4 and pad 9 on the same net.
BOOST_AUTO_TEST_CASE( MappedPinEmitsBothStackedPads )
{
    SCH_SCREEN*    screen = m_schematic->RootScreen();
    SCH_SHEET_PATH path;
    path.push_back( &m_schematic->Root() );

    const LIB_ID dfn = makeFp( wxS( "Package_DFN_QFN" ), wxS( "DFN-8-1EP" ) );

    m_libSym = std::make_unique<LIB_SYMBOL>( "LM358", nullptr );

    for( const wxString& number : { wxString( "1" ), wxString( "4" ) } )
    {
        SCH_PIN* pin = new SCH_PIN( m_libSym.get() );
        pin->SetNumber( number );
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        pin->SetPosition( VECTOR2I( number == "1" ? -508000 : 508000, 0 ) );
        m_libSym->AddDrawItem( pin );
    }

    PIN_MAP map( wxS( "DFN-8-EP" ) );
    map.SetEntry( wxS( "4" ), wxS( "[4,9]" ) );   // V- maps to pad 4 AND the exposed pad 9
    m_libSym->PinMaps().AddOrReplace( map );
    m_libSym->SetAssociatedFootprints( { { dfn, wxS( "DFN-8-EP" ) } } );

    SCH_SYMBOL* sym = new SCH_SYMBOL( *m_libSym, m_libSym->GetLibId(), &path, 0, 0,
                                      VECTOR2I( 15621000, 6223000 ) );
    sym->GetField( FIELD_T::REFERENCE )->SetText( wxS( "U1" ) );
    sym->SetFootprintFieldText( dfn.GetUniStringLibId() );
    sym->UpdatePins();
    screen->Append( sym );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    TEST_NETLIST_EXPORTER  exporter( m_schematic.get() );
    std::vector<PIN_INFO>  pins = exporter.Pins( sym, sheets[0] );

    wxString netForFour;
    wxString netForNine;
    bool     hasPadOne = false;

    for( const PIN_INFO& info : pins )
    {
        if( info.num == wxS( "1" ) )
            hasPadOne = true;
        else if( info.num == wxS( "4" ) )
            netForFour = info.netName;
        else if( info.num == wxS( "9" ) )
            netForNine = info.netName;
    }

    BOOST_CHECK( hasPadOne );                          // pin 1 resolves by identity to pad 1
    BOOST_CHECK( !netForFour.IsEmpty() );              // V- reaches pad 4
    BOOST_CHECK( !netForNine.IsEmpty() );              // V- also reaches the exposed pad 9
    BOOST_CHECK_EQUAL( netForFour, netForNine );       // both pads are on the same net

    m_libSym.reset();
}


BOOST_AUTO_TEST_SUITE_END()
