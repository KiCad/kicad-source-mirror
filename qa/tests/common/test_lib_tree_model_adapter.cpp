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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <eda_pattern_match.h>
#include <lib_id.h>
#include <lib_tree_item.h>
#include <lib_tree_model.h>
#include <lib_tree_model_adapter.h>
#include <project.h>
#include <settings/app_settings.h>
#include <wx/tokenzr.h>


/**
 * Minimal concrete adapter exposing the loaded column widths.  Construction runs
 * loadColumnConfig() against the injected settings, which is the path that previously
 * applied a corrupt persisted width.
 */
class TEST_LIB_TREE_MODEL_ADAPTER : public LIB_TREE_MODEL_ADAPTER
{
public:
    TEST_LIB_TREE_MODEL_ADAPTER( APP_SETTINGS_BASE::LIB_TREE& aSettings ) :
            LIB_TREE_MODEL_ADAPTER( nullptr, wxT( "pinned" ), aSettings )
    {
    }

    PROJECT::LIB_TYPE_T getLibType() override { return PROJECT::SYMBOL_LIB; }

    int LoadedWidth( const wxString& aColumn ) const
    {
        auto it = m_colWidths.find( aColumn );
        return it != m_colWidths.end() ? it->second : -1;
    }

    LIB_TREE_NODE_ROOT& GetTree() { return m_tree; }

    /// Offer Lifecycle after Item as a team library row does, and say whether a fit is armed for it
    bool OffersLifecycleFit()
    {
        offerColumn( wxT( "Lifecycle" ), wxT( "Item" ) );
        return m_fitOffered.has_value();
    }

    bool IsAutoWidth( const wxString& aHeader ) const { return isAutoWidth( aHeader ); }

    bool MayNarrowItem() const { return mayNarrow( wxT( "Lifecycle" ), wxT( "Item" ) ); }
};


/**
 * Stands in for LIB_SYMBOL, reproducing LIB_SYMBOL::cacheSearchTerms() term-for-term so the
 * test exercises the same search-term shape (name, LIB_ID, individual keyword tokens, keyword
 * blob, description) that a real symbol library produces.
 */
class TEST_LIB_TREE_ITEM : public LIB_TREE_ITEM
{
public:
    TEST_LIB_TREE_ITEM( const wxString& aLibNickname, const wxString& aName,
                        const wxString& aKeywords, const wxString& aDesc ) :
            m_libNickname( aLibNickname ),
            m_name( aName ),
            m_keywords( aKeywords ),
            m_desc( aDesc )
    {}

    LIB_ID GetLIB_ID() const override { return LIB_ID( m_libNickname, m_name ); }
    wxString GetName() const override { return m_name; }
    wxString GetLibNickname() const override { return m_libNickname; }
    wxString GetDesc() override { return m_desc; }

    std::vector<SEARCH_TERM>& GetSearchTerms() override
    {
        m_searchTerms.clear();
        m_searchTerms.emplace_back( SEARCH_TERM( m_libNickname, 4 ) );
        m_searchTerms.emplace_back( SEARCH_TERM( m_name, 8, true ) );
        m_searchTerms.emplace_back( SEARCH_TERM( GetLIB_ID().Format(), 16, true ) );

        wxStringTokenizer tokenizer( m_keywords, wxT( " \t\r\n" ), wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
            m_searchTerms.emplace_back( SEARCH_TERM( tokenizer.GetNextToken(), 4 ) );

        m_searchTerms.emplace_back( SEARCH_TERM( m_keywords, 1 ) );
        m_searchTerms.emplace_back( SEARCH_TERM( m_desc, 1 ) );

        return m_searchTerms;
    }

private:
    wxString                 m_libNickname;
    wxString                 m_name;
    wxString                 m_keywords;
    wxString                 m_desc;
    std::vector<SEARCH_TERM> m_searchTerms;
};


BOOST_AUTO_TEST_SUITE( LibTreeModelAdapter )

/**
 * A persisted column width can become corrupt after a mixed-DPI monitor change (issue 24702
 * reported width 427218649), which pushes the tree content off-screen and leaves the
 * symbol/footprint chooser blank.  Loading such a value must fall back to the sane default
 * instead of applying it, while a legitimate persisted width is honoured.
 */
BOOST_AUTO_TEST_CASE( CorruptPersistedColumnWidthIgnoredOnLoad )
{
    APP_SETTINGS_BASE::LIB_TREE settings;
    settings.column_widths[ wxT( "Item" ) ] = 427218649;
    settings.column_widths[ wxT( "Description" ) ] = 450;

    TEST_LIB_TREE_MODEL_ADAPTER* adapter = new TEST_LIB_TREE_MODEL_ADAPTER( settings );

    // The corrupt width must not overwrite the default; the legitimate one must be applied.
    BOOST_CHECK_EQUAL( adapter->LoadedWidth( wxT( "Item" ) ), 300 );
    BOOST_CHECK_EQUAL( adapter->LoadedWidth( wxT( "Description" ) ), 450 );

    adapter->DecRef();
}


/**
 * Typing a bare device-name letter (R, L, C, D) into the symbol chooser must land on the
 * matching Device: symbol, not on some unrelated, "busier" symbol whose keywords happen to
 * tokenize down to that same letter (issue 24404, still reproducible after the exact-match
 * tier was introduced: a keyword token equalling the query earned the same tier as an item's
 * actual name, and then the busier item's higher accumulated score won the tie-break).
 *
 * The decoys are the real ki_keywords/Description text of shipped symbols that collide this
 * way: Potentiometer_Digital:AD5293 ("R POT"), Interface_USB:AP33771 ("USB Type C PD Sink"),
 * and 74xGxx:74AUC1G74 ("Single D Flip-Flop D CMOS").
 */
BOOST_AUTO_TEST_CASE( SingleLetterSearchMatchesDeviceNameOverKeywordCollision )
{
    LIB_TREE_NODE_ROOT root;

    LIB_TREE_NODE_LIBRARY& device = root.AddLib( wxT( "Device" ), wxT( "Basic devices" ) );

    TEST_LIB_TREE_ITEM r( wxT( "Device" ), wxT( "R" ), wxT( "R res resistor" ), wxT( "Resistor" ) );
    TEST_LIB_TREE_ITEM l( wxT( "Device" ), wxT( "L" ), wxT( "inductor choke coil reactor magnetic" ),
                          wxT( "Inductor" ) );
    TEST_LIB_TREE_ITEM c( wxT( "Device" ), wxT( "C" ), wxT( "cap capacitor" ), wxT( "Unpolarized capacitor" ) );
    TEST_LIB_TREE_ITEM d( wxT( "Device" ), wxT( "D" ), wxT( "diode" ), wxT( "Diode" ) );

    device.AddItem( &r );
    device.AddItem( &l );
    device.AddItem( &c );
    device.AddItem( &d );

    LIB_TREE_NODE_LIBRARY& potLib = root.AddLib( wxT( "Potentiometer_Digital" ), wxT( "Digital potentiometers" ) );
    TEST_LIB_TREE_ITEM digiPot( wxT( "Potentiometer_Digital" ), wxT( "AD5293" ), wxT( "R POT" ),
                               wxT( "Digital potentiometer 1024 pos (SPI), TSSOP-14" ) );
    potLib.AddItem( &digiPot );

    LIB_TREE_NODE_LIBRARY& usbLib = root.AddLib( wxT( "Interface_USB" ), wxT( "USB interface ICs" ) );

    // "USB Type C PD Sink" is AP33771's real ki_keywords field (the "C" of "Type C" tokenizes
    // to a standalone, query-colliding "C"); "Converter Controller CMOS Clock capacitor charger
    // coupled" are appended from other real shipped keyword fields (see e.g. buck/boost
    // converter and MCU symbols) to model a busier real-world part clearing Device:C's score.
    TEST_LIB_TREE_ITEM usbPd( wxT( "Interface_USB" ), wxT( "AP33771" ),
                             wxT( "USB Type C PD Sink Converter Controller CMOS Clock capacitor "
                                  "charger coupled" ),
                             wxT( "USB Type-C PD Sink Controller, QFN-24" ) );
    usbLib.AddItem( &usbPd );

    LIB_TREE_NODE_LIBRARY& logicLib = root.AddLib( wxT( "74xGxx" ), wxT( "Little logic" ) );
    TEST_LIB_TREE_ITEM dFlipFlop( wxT( "74xGxx" ), wxT( "74AUC1G74" ), wxT( "Single D Flip-Flop D CMOS" ),
                                  wxT( "Single D Flip-Flop, Low-Voltage CMOS" ) );
    logicLib.AddItem( &dFlipFlop );

    // Mirrors LIB_TREE_MODEL_ADAPTER::AssignIntrinsicRanks(): rebuild search terms for the
    // libraries, then for the items nested inside each of them.
    root.AssignIntrinsicRanks( {} );

    for( std::unique_ptr<LIB_TREE_NODE>& lib : root.m_Children )
        lib->AssignIntrinsicRanks( {} );

    for( const wxString& letter : { wxString( wxT( "r" ) ), wxString( wxT( "l" ) ),
                                    wxString( wxT( "c" ) ), wxString( wxT( "d" ) ) } )
    {
        std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>> matchers;
        matchers.emplace_back( std::make_unique<EDA_COMBINED_MATCHER>( letter, CTX_LIBITEM ) );

        root.UpdateScore( matchers, nullptr );
        root.SortNodes( true );

        BOOST_REQUIRE( !root.m_Children.empty() );

        LIB_TREE_NODE* topLib = root.m_Children[0].get();

        BOOST_REQUIRE( !topLib->m_Children.empty() );

        LIB_TREE_NODE* topItem = topLib->m_Children[0].get();

        BOOST_TEST_CONTEXT( "search letter '" << letter << "'" )
        {
            BOOST_CHECK_EQUAL( topLib->m_Name, wxString( wxT( "Device" ) ) );
            BOOST_CHECK_EQUAL( topItem->m_Name, letter.Upper() );
            BOOST_CHECK( topItem->m_ExactMatch );
        }
    }
}

// A part in Recently Used or Already Placed shows the state its library row shows
BOOST_AUTO_TEST_CASE( RecentAndPlacedRowsShowTheirLibraryState )
{
    APP_SETTINGS_BASE::LIB_TREE  settings;
    TEST_LIB_TREE_MODEL_ADAPTER* adapter = new TEST_LIB_TREE_MODEL_ADAPTER( settings );
    TEST_LIB_TREE_ITEM           c( wxT( "acme-passives" ), wxT( "C" ), wxT( "cap capacitor" ),
                                    wxT( "Unpolarized capacitor" ) );
    std::vector<LIB_TREE_ITEM*>  items{ &c };

    adapter->DoAddLibrary( wxT( "-- Recently Used --" ), wxEmptyString, items, false, true )
            .m_IsRecentlyUsedGroup = true;
    adapter->DoAddLibrary( wxT( "-- Already Placed --" ), wxEmptyString, items, false, true )
            .m_IsAlreadyPlacedGroup = true;

    LIB_TREE_NODE_LIBRARY& library = adapter->DoAddLibrary( wxT( "acme-passives" ), wxEmptyString, items, false, true );

    // As the team's listing set it on the library row
    const wxString state = wxT( "Not for New Designs" );
    library.m_Children[0]->m_Fields[wxT( "Lifecycle" )] = state;
    adapter->RefreshLifecycle( library );

    int groups = 0;

    for( const std::unique_ptr<LIB_TREE_NODE>& group : adapter->GetTree().m_Children )
    {
        if( !group->m_IsRecentlyUsedGroup && !group->m_IsAlreadyPlacedGroup )
            continue;

        ++groups;
        BOOST_REQUIRE_EQUAL( group->m_Children.size(), 1 );
        BOOST_CHECK_EQUAL( group->m_Children[0]->m_Fields[wxT( "Lifecycle" )], state );
    }

    BOOST_CHECK_EQUAL( groups, 2 );

    adapter->DecRef();
}


// A fitted width carries over only while the user leaves it, and settings without the record fit again
BOOST_AUTO_TEST_CASE( LifecycleFitsAgainUnlessTheUserSizedIt )
{
    auto settingsWith = []( const wxString& aHeader, std::optional<int> aSaved, std::optional<int> aFitted )
    {
        APP_SETTINGS_BASE::LIB_TREE settings;

        if( aSaved )
            settings.column_widths[aHeader] = *aSaved;

        if( aFitted )
            settings.auto_widths[aHeader] = *aFitted;

        return settings;
    };

    auto fits = [&]( std::optional<int> aSaved, std::optional<int> aFitted )
    {
        APP_SETTINGS_BASE::LIB_TREE  settings = settingsWith( wxT( "Lifecycle" ), aSaved, aFitted );
        TEST_LIB_TREE_MODEL_ADAPTER* adapter = new TEST_LIB_TREE_MODEL_ADAPTER( settings );
        bool                         armed = adapter->OffersLifecycleFit();

        adapter->DecRef();
        return armed;
    };

    // The fit may narrow Item only while Item's width is automatic too
    auto itemIsAuto = [&]( std::optional<int> aSaved, std::optional<int> aFitted )
    {
        APP_SETTINGS_BASE::LIB_TREE  settings = settingsWith( wxT( "Item" ), aSaved, aFitted );
        TEST_LIB_TREE_MODEL_ADAPTER* adapter = new TEST_LIB_TREE_MODEL_ADAPTER( settings );
        bool                         automatic = adapter->IsAutoWidth( wxT( "Item" ) );

        adapter->DecRef();
        return automatic;
    };

    BOOST_CHECK( fits( std::nullopt, std::nullopt ) );
    BOOST_CHECK( fits( 96, 96 ) );
    BOOST_CHECK( !fits( 140, 96 ) );

    // A round M Lifecycle width was fitted and saved with no record
    BOOST_CHECK( fits( 96, std::nullopt ) );

    BOOST_CHECK( itemIsAuto( std::nullopt, std::nullopt ) );
    BOOST_CHECK( itemIsAuto( 200, 200 ) );
    BOOST_CHECK( !itemIsAuto( 300, std::nullopt ) );
    BOOST_CHECK( !itemIsAuto( 350, 200 ) );

    // Lifecycle never offered yet, so the saved Item width of every earlier chooser must make room for it once
    APP_SETTINGS_BASE::LIB_TREE  firstOffer = settingsWith( wxT( "Item" ), 300, std::nullopt );
    TEST_LIB_TREE_MODEL_ADAPTER* adapter = new TEST_LIB_TREE_MODEL_ADAPTER( firstOffer );
    BOOST_CHECK( adapter->MayNarrowItem() );
    adapter->DecRef();

    // That fit recorded both widths, and the user has since widened Item
    APP_SETTINGS_BASE::LIB_TREE laterOffer = settingsWith( wxT( "Item" ), 320, 180 );
    laterOffer.column_widths[wxT( "Lifecycle" )] = 140;
    laterOffer.auto_widths[wxT( "Lifecycle" )] = 140;
    adapter = new TEST_LIB_TREE_MODEL_ADAPTER( laterOffer );
    BOOST_CHECK( !adapter->MayNarrowItem() );
    adapter->DecRef();
}


// Widths of the Windows run, where a 250 px footprint pane clipped a claimed state at half the pane and a chooser
// clipped "Not for New Designs" once a 17 px scrollbar appeared
BOOST_AUTO_TEST_CASE( LifecycleFitTakesTheRoomItemCanSpare )
{
    using FIT = LIB_TREE_MODEL_ADAPTER;

    FIT::FITTED_WIDTHS pane = FIT::FitWidths( 250, 17, 151, 79, 300, 50, true );
    BOOST_CHECK_EQUAL( pane.offered, 151 );
    BOOST_CHECK_LE( pane.offered + pane.after, 250 - 17 );
    BOOST_CHECK_GE( pane.after, ( 250 - 17 ) / 3 );

    FIT::FITTED_WIDTHS chooser = FIT::FitWidths( 400, 17, 126, 79, 300, 50, true );
    BOOST_CHECK_EQUAL( chooser.offered, 126 );
    BOOST_CHECK_EQUAL( chooser.offered + chooser.after, 400 - 17 );

    // A state too long for the pane leaves Item its third, and a width the user gave Item is kept
    FIT::FITTED_WIDTHS longState = FIT::FitWidths( 250, 17, 400, 79, 300, 50, true );
    BOOST_CHECK_EQUAL( longState.after, ( 250 - 17 ) / 3 );
    BOOST_CHECK_EQUAL( FIT::FitWidths( 250, 17, 151, 79, 217, 50, false ).after, 217 );
}


BOOST_AUTO_TEST_SUITE_END()
