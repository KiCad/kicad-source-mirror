/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_ASSISTANT_H__
#define __DIALOG_ASSISTANT_H__

#include <map>
#include <vector>

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <wx/frame.h>

#define ASSISTANT_STYLE wxFRAME_TOOL_WINDOW|wxSTAY_ON_TOP|wxFRAME_NO_TASKBAR|wxCLOSE_BOX

class HINT;

class KICAD_ASSISTANT : public wxFrame
{
    public:
        enum HINT_CATEGORY { WELCOME, TIP_OF_THE_DAY, ROUTING, UNDO, MOVING };

        KICAD_ASSISTANT( wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxString& title = wxT("KiCad Assistant"),
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = ASSISTANT_STYLE|wxCAPTION );
        ~KICAD_ASSISTANT();

        void DisplayHint( const wxString& aMsg, int aTimeout = 0, int aChance = 100 );
        void RandomHint( HINT_CATEGORY aCategory, int aTimeout = 0, int aChance = 100 );

        void OnClose(wxCloseEvent& event);
        void OnMove(wxMoveEvent& event);
        void onButton( wxMouseEvent& aEvent );
    private:

    protected:
        wxStaticBitmap* m_imgPaperclip;
        std::map<HINT_CATEGORY, std::vector<wxString> > m_hints;
        HINT* m_hint;
        wxWindow* m_parent;
};

#endif //__DIALOG_ASSISTANT_H__
