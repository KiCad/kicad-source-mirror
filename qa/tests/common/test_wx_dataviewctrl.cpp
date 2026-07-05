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

#include <widgets/wx_dataviewctrl.h>


BOOST_AUTO_TEST_SUITE( WxDataViewCtrl )


namespace
{
// The canceller only exercises notification plumbing, so the accessors can be inert stubs.
class EMPTY_MODEL : public wxDataViewModel
{
public:
    void GetValue( wxVariant&, const wxDataViewItem&, unsigned int ) const override {}
    bool SetValue( const wxVariant&, const wxDataViewItem&, unsigned int ) override { return false; }
    wxDataViewItem GetParent( const wxDataViewItem& ) const override { return wxDataViewItem(); }
    bool           IsContainer( const wxDataViewItem& ) const override { return false; }
    unsigned int GetChildren( const wxDataViewItem&, wxDataViewItemArray& ) const override
    {
        return 0;
    }
};
} // namespace


// Must fire on every item-removing notification (#24246 / #24805 crash) and stay silent on adds
// and value changes, which leave the pending scroll valid.
BOOST_AUTO_TEST_CASE( EnsureVisibleCancellerFiresOnItemRemoval )
{
    EMPTY_MODEL* model = new EMPTY_MODEL; // wxRefCounter starts owned at refcount 1
    int          cancels = 0;

    model->AddNotifier( new WX_ENSURE_VISIBLE_CANCELLER( [&]() { cancels++; } ) );

    const wxDataViewItem root;
    const wxDataViewItem item( reinterpret_cast<void*>( 0x1 ) );

    model->ItemAdded( root, item );
    BOOST_CHECK_EQUAL( cancels, 0 );

    model->ItemChanged( item );
    BOOST_CHECK_EQUAL( cancels, 0 );

    model->ItemDeleted( root, item );
    BOOST_CHECK_EQUAL( cancels, 1 );

    wxDataViewItemArray items;
    items.Add( item );
    model->ItemsDeleted( root, items );
    BOOST_CHECK_EQUAL( cancels, 2 );

    model->Cleared();
    BOOST_CHECK_EQUAL( cancels, 3 );

    // BeforeReset drops the scroll while items are live; AfterReset defaults to Cleared.
    model->BeforeReset();
    BOOST_CHECK_EQUAL( cancels, 4 );

    model->AfterReset();
    BOOST_CHECK_EQUAL( cancels, 5 );

    model->DecRef(); // deletes the model, which deletes the notifier
}


BOOST_AUTO_TEST_SUITE_END()
