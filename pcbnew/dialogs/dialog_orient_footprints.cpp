/**
 * @file DIALOG_ORIENT_FOOTPRINTS.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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


#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "macros.h"

#include "class_board.h"
#include "class_module.h"

#include "dialog_orient_footprints_base.h"



/* DIALOG_ORIENT_FOOTPRINTS class declaration
 */

class DIALOG_ORIENT_FOOTPRINTS: public DIALOG_ORIENT_FOOTPRINTS_BASE
{
private:
    PCB_EDIT_FRAME * m_Parent;
    static int newOrientation;

public:
    DIALOG_ORIENT_FOOTPRINTS( PCB_EDIT_FRAME* parent );
    ~DIALOG_ORIENT_FOOTPRINTS() {}

    bool ApplyToLockedModules()
    {
        return m_ApplyToLocked->IsChecked();
    }
    int GetOrientation()
    {
        return newOrientation;
    }
    wxString GetFilter()
    {
        return m_FilterPattern->GetValue();
    }

private:
    void init();

    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
};
int DIALOG_ORIENT_FOOTPRINTS::newOrientation = 0;


DIALOG_ORIENT_FOOTPRINTS::DIALOG_ORIENT_FOOTPRINTS( PCB_EDIT_FRAME* parent )
    : DIALOG_ORIENT_FOOTPRINTS_BASE( parent )
{
    m_Parent = parent;
    wxString txt;
    txt.Printf(wxT("%g"), (double) newOrientation/10);
    m_OrientationCtrl->SetValue(txt);
    SetFocus( );
    GetSizer()->SetSizeHints(this);
    Centre();
}


/****************************************************************/
void PCB_EDIT_FRAME::OnOrientFootprints( wxCommandEvent& event )
/****************************************************************/
/**
 * Function OnOrientFootprints
 * install the dialog box for the comman Orient Footprints
 */
{
    DIALOG_ORIENT_FOOTPRINTS dlg(this);
    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString text = dlg.GetFilter();

    if( ReOrientModules( text, dlg.GetOrientation(), dlg.ApplyToLockedModules() ) )
    {
        m_canvas->Refresh();
        Compile_Ratsnest( NULL, true );
    }
}


/*******************************************************************/
bool PCB_EDIT_FRAME::ReOrientModules( const wxString& ModuleMask,
                                      int Orient, bool include_fixe )
/*******************************************************************/
/**
 * Function ReOrientModules
 * Set the orientation of footprints
 * @param ModuleMask = mask (wildcard allowed) selection
 * @param Orient = new orientation
 * @param include_fixe = true to orient locked footprints
 * @return true if some footprints modified, false if no change
 */
{
    wxString line;
    bool modified = false;

    line.Printf( _( "Ok to set footprints orientation to %.1f degrees ?" ), (double)Orient / 10 );
    if( !IsOK( this, line ) )
        return false;

    for( MODULE* module = GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        if( module->IsLocked() && !include_fixe )
            continue;

        if( WildCompareString( ModuleMask, module->m_Reference->m_Text, FALSE ) )
        {
            modified = true;
            Rotate_Module( NULL, module, Orient, FALSE );
        }
    }

    if ( modified )
        OnModify();

    return modified;
}


void DIALOG_ORIENT_FOOTPRINTS::OnOkClick( wxCommandEvent& event )
{
    double d_orient;
    wxString text = m_OrientationCtrl->GetValue();

    if ( ! text.ToDouble(&d_orient) )
    {
        DisplayError(this, _("Bad value for footprints orientation"));
        return;
    }

    newOrientation = wxRound(d_orient * 10);
    NORMALIZE_ANGLE_180( newOrientation );
    EndModal( wxID_OK );
}

void DIALOG_ORIENT_FOOTPRINTS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}
