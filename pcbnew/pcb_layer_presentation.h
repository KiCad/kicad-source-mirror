/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_LAYER_PRESENTATION_H
#define PCB_LAYER_PRESENTATION_H

#include <lseq.h>
#include <widgets/layer_presentation.h>

class LAYER_PAIR;
class PCB_BASE_FRAME;

/**
 * Class that manages the presentation of PCB layers in a PCB frame.
 */
class PCB_LAYER_PRESENTATION : public LAYER_PRESENTATION
{
public:
    PCB_LAYER_PRESENTATION( PCB_BASE_FRAME* aFrame );

    COLOR4D getLayerColor( int aLayer ) const override;

    wxString getLayerName( int aLayer ) const override;

    LSEQ getOrderedEnabledLayers() const;

    /**
     * Annoying post-ctor initialization (for when PCB_LAYER_BOX_SELECTOR doesn't
     * have access to the PCB_BASE_FRAME at construction time).
     */
    void SetBoardFrame( PCB_BASE_FRAME* aFrame ) { m_boardFrame = aFrame; }

    wxString getLayerPairName( const LAYER_PAIR& aPair ) const;

private:
    PCB_BASE_FRAME* m_boardFrame;
};

#endif // PCB_LAYER_PRESENTATION_H