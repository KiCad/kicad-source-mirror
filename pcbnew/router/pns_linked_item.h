/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCBNEW_ROUTER_PNS_LINKED_ITEM_H_
#define PCBNEW_ROUTER_PNS_LINKED_ITEM_H_

#include "pns_item.h"


namespace PNS
{
class LINKED_ITEM : public ITEM
{
public:

    typedef uint64_t UNIQ_ID;

    LINKED_ITEM( PnsKind aKind ) :
        ITEM( aKind )
    {
        m_uid = genNextUid();
    }

    LINKED_ITEM( const LINKED_ITEM& aOther ) :
        ITEM( aOther ),
        m_uid( aOther.m_uid )
    {}

    void ResetUid()
    {
        m_uid = genNextUid();
    }

    UNIQ_ID Uid() const { return m_uid; }

    virtual void SetWidth( int aWidth )
    {};

    virtual int Width() const
    {
        return 0;
    }

private:
    static UNIQ_ID genNextUid();
protected:
    UNIQ_ID m_uid;
};

} // namespace PNS
#endif /* PCBNEW_ROUTER_PNS_LINKED_ITEM_H_ */
