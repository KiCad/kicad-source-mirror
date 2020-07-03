/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_ITEM_PROTO_H
#define DRC_ITEM_PROTO_H

namespace test {

#include <macros.h>
#include <base_struct.h>
#include <rc_item.h>
#include <marker_base.h>
#include <class_board.h>
#include <pcb_base_frame.h>

#include <drc_proto/drc_engine.h>

class DRC_RULE;
class DRC_TEST_PROVIDER;


class DRC_ITEM : public RC_ITEM
{
public:
    DRC_ITEM( int aErrorCode ) {}// fixme

    DRC_ITEM( const wxString& aErrorText ) {} // fixme

    void SetViolatingRule ( test::DRC_RULE *aRule ) { m_violatingRule = aRule; }
    test::DRC_RULE* GetViolatingRule() const { return m_violatingRule; }

    void SetViolatingTest( test::DRC_TEST_PROVIDER *aProvider ) { m_violatingTest = aProvider; }
    test::DRC_TEST_PROVIDER* GetViolatingTest() const { return m_violatingTest; }

    /**
     * Function GetErrorText
     * returns the string form of a drc error code.
     */
    ::wxString GetErrorText( int aErrorCode = -1, bool aTranslate = true ) const { return ""; } // fixme

    /**
     * Function ShowHtml
     * translates this object into a fragment of HTML suitable for the wxHtmlListBox class.
     * @return wxString - the html text.
     */
    ::wxString FormatHtml( ) const { return ""; } // fixme

protected:

    DRC_RULE *m_violatingRule;
    DRC_TEST_PROVIDER *m_violatingTest;

    //std::vector<BOARD_ITEM*> m_violatingObjects;
};

};

#endif      // DRC_ITEM_H
