/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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

#ifndef GAL_DISPLAY_OPTIONS_H__
#define GAL_DISPLAY_OPTIONS_H__

#include <observable.h>

class wxConfigBase;
class wxString;

namespace KIGFX {

    class GAL_DISPLAY_OPTIONS;

    enum class OPENGL_ANTIALIASING_MODE : long
    {
        NONE = 0,
        SUBSAMPLE_HIGH = 1,
        SUBSAMPLE_ULTRA = 2,
        SUPERSAMPLING_X2 = 3,
        SUPERSAMPLING_X4 = 4
    };

    class GAL_DISPLAY_OPTIONS_OBSERVER
    {
    public:
        virtual void OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& ) = 0;
    };

    class GAL_DISPLAY_OPTIONS : public UTIL::OBSERVABLE<GAL_DISPLAY_OPTIONS_OBSERVER>
    {
    public:
        GAL_DISPLAY_OPTIONS();

        OPENGL_ANTIALIASING_MODE gl_antialiasing_mode;

        void ReadConfig ( wxConfigBase* aCfg, wxString aBaseName );
        void WriteConfig( wxConfigBase* aCfg, wxString aBaseName );

        void NotifyChanged();
    };

}

#endif

