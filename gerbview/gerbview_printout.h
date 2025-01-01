/*
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef GERBVIEW_PRINTOUT_H
#define GERBVIEW_PRINTOUT_H

#include <board_printout.h>

class GBR_LAYOUT;
class LSET;

class GERBVIEW_PRINTOUT : public BOARD_PRINTOUT
{
public:
    GERBVIEW_PRINTOUT( GBR_LAYOUT* aLayout, const BOARD_PRINTOUT_SETTINGS& aParams,
            const KIGFX::VIEW* aView, const wxString& aTitle );

    bool OnPrintPage( int aPage ) override;

protected:
    int milsToIU( double aMils ) const override;

    void setupViewLayers( KIGFX::VIEW& aView, const LSET& aLayerSet ) override;

    void setupGal( KIGFX::GAL* aGal ) override;

     BOX2I getBoundingBox() override;

    std::unique_ptr<KIGFX::PAINTER> getPainter( KIGFX::GAL* aGal ) override;

private:
    GBR_LAYOUT* m_layout;
};

#endif /* GERBVIEW_PRINTOUT_H */
