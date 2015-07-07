/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tool/context_menu.h>

class BOARD;

/**
 * @brief Context menu that displays track and/or via sizes basing on the board design settings
 * of a BOARD object.
 */
class CONTEXT_TRACK_VIA_SIZE_MENU: public CONTEXT_MENU
{
public:
    /**
     * Constructor.
     * @param aTrackSizes decides if the context menu should contain track sizes.
     * @param aTrackSizes decides if the context menu should contain via sizes.
     */
    CONTEXT_TRACK_VIA_SIZE_MENU( bool aTrackSizes, bool aViaSizes );

    virtual ~CONTEXT_TRACK_VIA_SIZE_MENU() {}

    /**
     * Function AppendSizes()
     * Appends the list of tracks/vias (depending on the parameters passed to the constructor).
     * @param aBoard is the BOARD object whose board settings will be used to generate the list.
     */
    virtual void AppendSizes( const BOARD* aBoard );

protected:
    ///> Whether the generated menu should contain track sizes.
    bool m_tracks;

    ///> Whether the generated menu should contain via sizes.
    bool m_vias;
};
