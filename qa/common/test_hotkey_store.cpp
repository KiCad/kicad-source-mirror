/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <hotkey_store.h>

// ----------------------------------------------------------------------------
// Dummy Hotkey definitions

static EDA_HOTKEY actionA1( _HKI("A1"), 1001, WXK_F1 );
static EDA_HOTKEY actionA2( _HKI("A2"), 1002, WXK_F2 );

static wxString   sectionATag( "[a]" );
static wxString   sectionATitle( "Section A" );

static EDA_HOTKEY actionB1( _HKI("B1"), 2001, WXK_F10 );
static EDA_HOTKEY actionB2( _HKI("B2"), 2002, WXK_F11 );

static wxString   sectionBTag( "[b]" );
static wxString   sectionBTitle( "Section B" );

// A keycode that is unused by any hotkey
static const int unused_keycode = WXK_F5;

// List of hotkey descriptors for library editor
static EDA_HOTKEY* sectionAHotkeyList[] =
{
    &actionA1,
    &actionA2,
    NULL
};

static EDA_HOTKEY* sectionBHotkeyList[] =
{
    &actionB1,
    &actionB2,
    NULL
};

static EDA_HOTKEY_CONFIG test_hokeys_descr[] =
{
    { &sectionATag,   sectionAHotkeyList,   &sectionATitle  },
    { &sectionBTag,   sectionBHotkeyList,   &sectionBTitle  },
    { NULL,           NULL,                 NULL            }
};

// Number of sections in the above table
static const unsigned num_cfg_sections =
        sizeof( test_hokeys_descr ) / sizeof( EDA_HOTKEY_CONFIG ) - 1;

// ----------------------------------------------------------------------------

struct HotkeyStoreFixture
{
    HotkeyStoreFixture():
        m_hotkey_store( test_hokeys_descr )
    {}

    HOTKEY_STORE m_hotkey_store;
};


/**
 * Declares a struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( HotkeyStore, HotkeyStoreFixture )


/**
 * Check conflict detections
 */
BOOST_AUTO_TEST_CASE( StoreConstruction )
{
    // Should have ingested two sections (A and B)
    BOOST_CHECK_EQUAL( m_hotkey_store.GetSections().size(), num_cfg_sections );
}


/**
 * Check conflict detections
 */
BOOST_AUTO_TEST_CASE( HotkeyConflict )
{
    EDA_HOTKEY* conf_key = nullptr;
    EDA_HOTKEY_CONFIG* conf_sect = nullptr;
    bool conflict;

    conflict = m_hotkey_store.CheckKeyConflicts( unused_keycode, sectionATag,
            &conf_key, &conf_sect );

    // No conflicts
    BOOST_CHECK_EQUAL( conflict, true );

    conflict = m_hotkey_store.CheckKeyConflicts( actionA1.m_KeyCode, sectionATag,
            &conf_key, &conf_sect );

    // See if we conflicted with the correct key in the correct section
    BOOST_CHECK_EQUAL( conflict, false );
    BOOST_CHECK_EQUAL( conf_key->m_Idcommand, actionA1.m_Idcommand );
    BOOST_CHECK_EQUAL( conf_sect, &test_hokeys_descr[0] );
}


/**
 * Check the undo works
 */
BOOST_AUTO_TEST_CASE( HotkeySetUndo )
{
    CHANGED_HOTKEY* hk = m_hotkey_store.FindHotkey( sectionATag, actionA1.m_Idcommand );

    // Found something
    BOOST_CHECK( hk );
    BOOST_CHECK_EQUAL( hk->GetCurrentValue().m_Idcommand, actionA1.m_Idcommand );

    // Change the Key code
    hk->GetCurrentValue().m_KeyCode = unused_keycode;

    // Change everything back
    m_hotkey_store.ResetAllHotkeysToDefault();

    // Check it went back
    BOOST_CHECK_EQUAL( hk->GetCurrentValue().m_Idcommand, actionA1.m_Idcommand );
}


BOOST_AUTO_TEST_SUITE_END()
