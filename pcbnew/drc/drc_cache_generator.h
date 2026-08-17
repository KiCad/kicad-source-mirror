/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#pragma once

#include <drc/drc_test_provider.h>


class DRC_CACHE_GENERATOR : public DRC_TEST_PROVIDER
{
public:
    DRC_CACHE_GENERATOR() :
            DRC_TEST_PROVIDER()
    {}

    virtual ~DRC_CACHE_GENERATOR() = default;

    /**
     * Discard the board's run-time DRC caches and regenerate them from scratch.
     *
     * Safe to call repeatedly; each run starts from a clean slate, so callers don't need to
     * invalidate the caches themselves first.
     */
    virtual bool Run() override;
};
