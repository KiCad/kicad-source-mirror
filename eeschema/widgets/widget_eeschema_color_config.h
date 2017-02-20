/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 G. Harland
 * Copyright (C) 1992-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef WIDGET_EESCHEMA_COLOR_CONFIG_H_
#define WIDGET_EESCHEMA_COLOR_CONFIG_H_

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/colordlg.h>
#include <wx/clrpicker.h>

class wxBoxSizer;
class wxStaticLine;
class wxStdDialogButtonSizer;


/***********************************************/
/* Derived class for the frame color settings. */
/***********************************************/

class WIDGET_EESCHEMA_COLOR_CONFIG : public wxPanel
{
private:
    EDA_DRAW_FRAME*         m_drawFrame;
    wxColourPickerCtrl*     m_SelBgColor;
    wxBoxSizer*             m_mainBoxSizer;

    // Creates the controls and sizers
    void CreateControls();

    void    SetColor( wxCommandEvent& aEvent );

    virtual EDA_DRAW_FRAME* GetDrawFrame() { return m_drawFrame; }

public:
    // Constructors and destructor
    WIDGET_EESCHEMA_COLOR_CONFIG( wxWindow* aParent, EDA_DRAW_FRAME* aDrawFrame );

    bool TransferDataFromControl();

    /**
     * Method InstallOnPanel
     * Install this WIDGET_EESCHEMA_COLOR_CONFIG onto an empty panel. This is useful
     * when combining with wxFormBuilder, as an empty panel can be left as a
     * placeholder in the layout.
     */
    void InstallOnPanel( wxPanel* aPanel );
};

#endif    // WIDGET_EESCHEMA_COLOR_CONFIG_H_
