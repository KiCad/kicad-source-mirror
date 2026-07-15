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

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <dialog_shim.h>

#include <wx/display.h>
#include <wx/textctrl.h>

// Creating windows requires a display connection, which headless CI does not have
static bool IsDisplayAvailable()
{
#ifdef __WXGTK__
    return wxDisplay::GetCount() > 0;
#endif
    return true;
}

#define SKIP_IF_HEADLESS()                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        if( !IsDisplayAvailable() )                                                                                    \
        {                                                                                                              \
            BOOST_TEST_MESSAGE( "Skipping test - no display available" );                                              \
            return;                                                                                                    \
        }                                                                                                              \
    } while( 0 )


namespace
{
class TEST_PERSIST_DIALOG : public DIALOG_SHIM
{
public:
    TEST_PERSIST_DIALOG( const wxString& aTitle, bool aOptOutFileName ) :
            DIALOG_SHIM( nullptr, wxID_ANY, aTitle )
    {
        // Two controls so we can watch one restore while the other opts out.
        // Keys are generated from class name plus sibling index, so identically
        // shaped dialogs share persisted state.
        m_plainCtrl = new wxTextCtrl( this, wxID_ANY );
        m_fileNameCtrl = new wxTextCtrl( this, wxID_ANY );

        if( aOptOutFileName )
            OptOut( m_fileNameCtrl );
    }

    wxTextCtrl* m_plainCtrl;
    wxTextCtrl* m_fileNameCtrl;
};
} // namespace


BOOST_AUTO_TEST_SUITE( DialogShimControlPersistence )


// Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/24860
// LoadControlState() restored the last-typed value from kicad_common over a
// value the dialog had already loaded from the project file.  Controls backed
// by project settings opt out, and that opt-out must hold on both the load
// and the save side.
//
// Dialogs are heap-allocated and released with Destroy() because destroying a
// top-level window directly crashes in this harness (no running event loop).
BOOST_AUTO_TEST_CASE( OptOutControlNotRestored )
{
    SKIP_IF_HEADLESS();

    wxString title = wxS( "PersistTestOptOutLoad" );

    TEST_PERSIST_DIALOG* seed = new TEST_PERSIST_DIALOG( title, false );
    seed->m_plainCtrl->SetValue( wxS( "stale_plain" ) );
    seed->m_fileNameCtrl->SetValue( wxS( "stale_from_common.csv" ) );
    seed->SaveControlState();
    seed->Destroy();

    TEST_PERSIST_DIALOG* dlg = new TEST_PERSIST_DIALOG( title, true );
    dlg->m_plainCtrl->SetValue( wxS( "project_plain" ) );
    dlg->m_fileNameCtrl->SetValue( wxS( "value_from_project.csv" ) );
    dlg->LoadControlState();

    // The non-opted control restores, proving LoadControlState actually ran
    BOOST_CHECK_EQUAL( dlg->m_plainCtrl->GetValue(), wxString( "stale_plain" ) );

    // The opted-out control keeps the value set by the dialog
    BOOST_CHECK_EQUAL( dlg->m_fileNameCtrl->GetValue(), wxString( "value_from_project.csv" ) );

    dlg->Destroy();
}


BOOST_AUTO_TEST_CASE( OptOutControlNotSaved )
{
    SKIP_IF_HEADLESS();

    wxString title = wxS( "PersistTestOptOutSave" );

    TEST_PERSIST_DIALOG* seed = new TEST_PERSIST_DIALOG( title, true );
    seed->m_plainCtrl->SetValue( wxS( "saved_plain" ) );
    seed->m_fileNameCtrl->SetValue( wxS( "must_not_be_saved.csv" ) );
    seed->SaveControlState();
    seed->Destroy();

    TEST_PERSIST_DIALOG* dlg = new TEST_PERSIST_DIALOG( title, false );
    dlg->m_plainCtrl->SetValue( wxS( "overwrite_me" ) );
    dlg->m_fileNameCtrl->SetValue( wxS( "value_from_project.csv" ) );
    dlg->LoadControlState();

    // Sanity check that this dialog's persisted state exists
    BOOST_CHECK_EQUAL( dlg->m_plainCtrl->GetValue(), wxString( "saved_plain" ) );

    // Nothing was persisted for the opted-out control, so nothing restores
    // over it even without an opt-out on the loading side
    BOOST_CHECK_EQUAL( dlg->m_fileNameCtrl->GetValue(), wxString( "value_from_project.csv" ) );

    dlg->Destroy();
}


BOOST_AUTO_TEST_SUITE_END()
