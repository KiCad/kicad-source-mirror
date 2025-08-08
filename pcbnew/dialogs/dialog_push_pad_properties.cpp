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

#include <dialog_push_pad_properties.h>

#include <pad.h>
#include <macros.h>
#include <pcb_edit_frame.h>


DIALOG_PUSH_PAD_PROPERTIES::DIALOG_PUSH_PAD_PROPERTIES( PCB_BASE_FRAME* aParent ) :
        DIALOG_PUSH_PAD_PROPERTIES_BASE( aParent ),
        m_parent( aParent )
{
    SetupStandardButtons( { { wxID_OK,     _( "Change Pads on Current Footprint" )    },
                            { wxID_APPLY,  _( "Change Pads on Identical Footprints" ) } } );

    if( aParent->IsType( FRAME_FOOTPRINT_EDITOR ) )
        m_sdbSizer1Apply->Show( false );

    m_sdbSizer1->Layout();

    finishDialogSettings();
}


void DIALOG_PUSH_PAD_PROPERTIES::PadPropertiesAccept( wxCommandEvent& event )
{
    int returncode = 0;

    switch( event.GetId() )
    {
    case wxID_APPLY:
        returncode = 1;
        KI_FALLTHROUGH;

    case wxID_OK:
        if( IsQuasiModal() )
            EndQuasiModal( returncode );
        else
            EndDialog( returncode );

        break;
    }

    m_parent->OnModify();
}
