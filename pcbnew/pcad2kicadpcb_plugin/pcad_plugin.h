/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file pcad_plugin.h
 * @brief Pcbnew PLUGIN for P-Cad 200x ASCII *.pcb format.
 */

#ifndef PCAD_PLUGIN_H_
#define PCAD_PLUGIN_H_


#include <io_mgr.h>

class PCAD_PLUGIN : public PLUGIN
{
public:

    // -----<PUBLIC PLUGIN API>--------------------------------------------------

    const wxString  PluginName() const override;

    BOARD*          Load( const wxString&   aFileName,
                          BOARD*            aAppendToMe,
                          const PROPERTIES* aProperties = NULL ) override;

    const wxString  GetFileExtension() const override;

    // -----</PUBLIC PLUGIN API>-------------------------------------------------

    PCAD_PLUGIN();
    ~PCAD_PLUGIN();

private:
    const PROPERTIES*   m_props;
    BOARD*              m_board;
};

#endif    // PCAD_PLUGIN_H_
