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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <component_classes/component_class.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <footprint.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct PCB_COMPONENT_CLASS_FIXTURE
{
    PCB_COMPONENT_CLASS_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( ComponentClasses, PCB_COMPONENT_CLASS_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "component_classes", m_board );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    std::vector<wxString> allClasses{ "ANY",          "CAP_1",         "CAP_2",   "CLASS_1",
                                      "CLASS_2",      "CLASS_3",       "CLASS_4", "MULTI_REF",
                                      "REF_WILDCARD", "REF_WILDCARD2", "RES_1",   "RES_2",
                                      "RES_3",        "RES_4" };

    auto testClasses = [&allClasses](const wxString& ref, const COMPONENT_CLASS* compClass, std::vector<wxString> assignedClasses)
    {
        std::vector<wxString> unassignedClasses;
        std::ranges::set_difference(allClasses, assignedClasses, std::back_inserter(unassignedClasses));

        for( const wxString& className : assignedClasses )
        {
            if( !compClass->ContainsClassName( className ) )
            {
                BOOST_ERROR( wxString::Format(
                        "%s component class failed (%s expected but not found - full class %s)",
                        ref, className, compClass->GetName() ) );
            }
        }

        for( const wxString& className : unassignedClasses )
        {
            if( compClass->ContainsClassName( className ) )
            {
                BOOST_ERROR( wxString::Format(
                        "%s component class failed (%s found but not expected - full class %s)",
                        ref, className, compClass->GetName() ) );
            }
        }
    };

    for( const auto fp : m_board->Footprints() )
    {
        if( fp->Reference().GetText() == wxT( "C1" ) )
        {
            testClasses( "C1", fp->GetComponentClass(), {"CAP_1", "CLASS_3", "CLASS_4"});
        }

        if( fp->Reference().GetText() == wxT( "C2" ) )
        {
            testClasses( "C2", fp->GetComponentClass(), {"CAP_2", "CLASS_3"});
        }

        if( fp->Reference().GetText() == wxT( "C3" ) )
        {
            testClasses( "C2", fp->GetComponentClass(), {});
        }

        if( fp->Reference().GetText() == wxT( "R8" ) )
        {
            testClasses( "R8", fp->GetComponentClass(), {"RES_1", "RES_2"});
        }

        if( fp->Reference().GetText() == wxT( "R88" ) )
        {
            testClasses( "R88", fp->GetComponentClass(), {"RES_2"});
        }

        if( fp->Reference().GetText() == wxT( "R2" ) )
        {
            testClasses( "R2", fp->GetComponentClass(), {"CLASS_1", "RES_1", "RES_2", "RES_3"});
        }

        if( fp->Reference().GetText() == wxT( "R1" ) )
        {
            testClasses( "R1", fp->GetComponentClass(), {"CLASS_1", "CLASS_2", "RES_1", "RES_2", "RES_4"});
        }

        if( fp->Reference().GetText() == wxT( "U1" ) )
        {
            testClasses( "U1", fp->GetComponentClass(), { "ANY" } );
        }

        if( fp->Reference().GetText() == wxT( "U2" ) )
        {
            testClasses( "U2", fp->GetComponentClass(), { "ANY" } );
        }

        if( fp->Reference().GetText() == wxT( "U3" ) )
        {
            testClasses( "U3", fp->GetComponentClass(), { "MULTI_REF" } );
        }

        if( fp->Reference().GetText() == wxT( "U4" ) )
        {
            testClasses( "U4", fp->GetComponentClass(), { "MULTI_REF" } );
        }

        if( fp->Reference().GetText() == wxT( "U55" ) )
        {
            testClasses( "U55", fp->GetComponentClass(), { "REF_WILDCARD", "REF_WILDCARD2" } );
        }

        if( fp->Reference().GetText() == wxT( "U555" ) )
        {
            testClasses( "U555", fp->GetComponentClass(), { "REF_WILDCARD2" } );
        }

        if( fp->Reference().GetText() == wxT( "R3" ) )
        {
            testClasses( "R3", fp->GetComponentClass(), { "/SHEET1/", "RES_1", "RES_2" } );
        }
    }

}
