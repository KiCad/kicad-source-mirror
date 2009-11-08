#ifndef __dialog_layers_setup2_h_
#define __dialog_layers_setup2_h_

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2009 Kicad Developers, see change_log.txt for contributors.
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


#include "dialog_layers_setup_base2.h"


class DIALOG_LAYERS_SETUP : public DIALOG_LAYERS_SETUP_BASE2
{
private:
    static wxPoint      s_LastPos;
    static wxSize       s_LastSize;

    WinEDA_PcbFrame*    m_Parent;

    int                 m_ActivesLayersCount;

    BOARD*              m_Pcb;
    LAYER_T             m_LayersType[4];
    wxString            m_LayersTypeName[4];

    wxTextCtrl*         m_textCtrl1[NB_COPPER_LAYERS];

    wxCheckBox*         m_checkBox[NB_LAYERS];

    wxChoice*           m_choice5[NB_COPPER_LAYERS];

private:
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOkButtonClick( wxCommandEvent& event );
    void OnLayerCountClick( wxCommandEvent& event );
//		void OnLayerGridLeftClick( wxGridEvent& event ){ event.Skip(); }
//		void OnLayerGridRighttClick( wxGridEvent& event ){ event.Skip(); }
    void Init();
    void SetRoutableLayerStatus( );
    bool TestDataValidity();

public:
    DIALOG_LAYERS_SETUP( WinEDA_PcbFrame* parent );
    ~DIALOG_LAYERS_SETUP( ) { };

};

#endif //__dialog_layers_setup2_h_
