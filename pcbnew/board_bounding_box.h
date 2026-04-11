/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef BOARD_BOUNDING_BOX_H
#define BOARD_BOUNDING_BOX_H

#include <board_item.h>
#include <memory>


class BOARD_BOUNDING_BOX : public EDA_ITEM
{
public:
    BOARD_BOUNDING_BOX( BOX2I const& aBoundingBox );

    BOARD_BOUNDING_BOX( const BOARD_BOUNDING_BOX& aOther );

    BOARD_BOUNDING_BOX& operator=( const BOARD_BOUNDING_BOX& aOther );

    EDA_ITEM* Clone() const override;

    ~BOARD_BOUNDING_BOX() override;

    const BOX2I GetBoundingBox() const override;

    void SetBoundingBox( BOX2I const& aBoundingBox );

    wxString GetClass() const override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;


#if defined( DEBUG )

    void Show( int nestLevel, std::ostream& os ) const override;

#endif


private:
    std::shared_ptr<BOX2I> m_boundingBox;
};

#endif