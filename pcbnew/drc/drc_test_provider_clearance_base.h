/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef DRC_TEST_PROVIDER_CLEARANCE_BASE__H
#define DRC_TEST_PROVIDER_CLEARANCE_BASE__H

#include <drc/drc_test_provider.h>
#include <settings/color_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_creepage_utils.h>

class BOARD;


class DRC_TEST_PROVIDER_CLEARANCE_BASE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_CLEARANCE_BASE () :
            DRC_TEST_PROVIDER(),
            m_board( nullptr ),
            m_boardOutlineValid( false )
    {
    }

    virtual ~DRC_TEST_PROVIDER_CLEARANCE_BASE()
    {
    }

protected:
    BOARD* m_board;
    bool   m_boardOutlineValid;

    void ReportAndShowPathCuToCu( std::shared_ptr<DRC_ITEM>& aDrce, const VECTOR2I& aMarkerPos,
                                  int aMarkerLayer, const BOARD_ITEM* aItem1,
                                  const BOARD_ITEM* aItem2, PCB_LAYER_ID layer, int aDistance );

    void ShowPathDRC( const std::vector<PCB_SHAPE>& aShapes, const VECTOR2I& aStart,
                      const VECTOR2I& aEnd, int aLength );


    DRC_GRAPHICS_HANDLER m_GraphicsHandlerBuffer;
};


#endif // DRC_TEST_PROVIDER_CLEARANCE_BASE__H
