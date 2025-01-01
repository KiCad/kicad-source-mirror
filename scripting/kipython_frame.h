/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCRIPTING_KIPYTHON_FRAME_H_
#define SCRIPTING_KIPYTHON_FRAME_H_

#include <kiway_player.h>

class wxWindow;
class APP_SETTINGS_BASE;
class KIWAY_EXPRESS;

class KIPYTHON_FRAME : public KIWAY_PLAYER
{


public:
    KIPYTHON_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~KIPYTHON_FRAME() override;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override {}
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override {}

    wxWindow* GetToolCanvas() const override { return nullptr;}

    void ExecuteRemoteCommand( const char* cmdline ) override {}

    void KiwayMailIn( KIWAY_EXPRESS& aEvent ) override {}
    void ProjectChanged() override {}

    void ShowChangedLanguage() override {};

    void SetupPythonEditor();
private:

    void redirectStdio();
    bool canCloseWindow( wxCloseEvent& aCloseEvent ) override { return true; }
    void doCloseWindow() override {}

};



#endif /* SCRIPTING_KIPYTHON_FRAME_H_ */
