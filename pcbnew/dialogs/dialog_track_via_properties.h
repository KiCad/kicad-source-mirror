/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <dialogs/dialog_track_via_properties_base.h>
#include <wx_unit_binder.h>
#include <boost/optional.hpp>
#include <layers_id_colors_and_visibility.h>

class SELECTION;
class PCB_BASE_FRAME;

class DIALOG_TRACK_VIA_PROPERTIES : public DIALOG_TRACK_VIA_PROPERTIES_BASE
{
public:
    DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_FRAME* aParent, const SELECTION& aItems );

    ///> Applies values from the dialog to the selected items.
    bool Apply();

private:
    void onClose( wxCloseEvent& aEvent );
    void onTrackNetclassCheck( wxCommandEvent& aEvent );
    void onViaNetclassCheck( wxCommandEvent& aEvent );
    void onCancelClick( wxCommandEvent& aEvent );
    void onOkClick( wxCommandEvent& aEvent );

    ///> Checks if the dialog values are correct.
    bool check() const;

    ///> Sets wxTextCtrl to the value stored in boost::optional<T> or "<...>" if it is not available.
    template<typename T>
    void setCommonVal( const boost::optional<T>& aVal, wxTextCtrl* aTxtCtrl, WX_UNIT_BINDER& aBinder )
    {
        if( aVal )
            aBinder.SetValue( *aVal );
        else
            aTxtCtrl->SetValue( "<...>" );
    }

    ///> Selected items to be modified.
    const SELECTION& m_items;

    WX_UNIT_BINDER m_trackStartX, m_trackStartY;
    WX_UNIT_BINDER m_trackEndX, m_trackEndY;
    WX_UNIT_BINDER m_trackWidth;

    WX_UNIT_BINDER m_viaX, m_viaY;
    WX_UNIT_BINDER m_viaDiameter, m_viaDrill;

    ///> Flag that determines if the dialog displays track properties.
    bool m_tracks;

    ///> Flag that determines if the dialog displays via properties.
    bool m_vias;
};
