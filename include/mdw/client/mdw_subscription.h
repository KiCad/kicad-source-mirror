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

/**
 * @file mdw_subscription.h
 * The handle that owns one event handler registration.
 */

#ifndef MDW_SUBSCRIPTION_H
#define MDW_SUBSCRIPTION_H

#include <kicommon.h>

#include <memory>

class MDW_EVENT_BUS;


/**
 * Owns one handler registration on an MDW_EVENT_BUS.
 *
 * Destroying or resetting the handle unregisters the handler. If another thread is
 * running the handler at that moment, Reset() waits for it to return, so the
 * handler's captures are safe to destroy afterwards. A handler must therefore not
 * block on the thread that resets its subscription.
 */
class KICOMMON_API MDW_SUBSCRIPTION
{
public:
    MDW_SUBSCRIPTION() = default;
    ~MDW_SUBSCRIPTION();

    MDW_SUBSCRIPTION( const MDW_SUBSCRIPTION& ) = delete;
    MDW_SUBSCRIPTION& operator=( const MDW_SUBSCRIPTION& ) = delete;
    MDW_SUBSCRIPTION( MDW_SUBSCRIPTION&& aOther ) noexcept;
    MDW_SUBSCRIPTION& operator=( MDW_SUBSCRIPTION&& aOther ) noexcept;

    void Reset();

    bool IsActive() const { return m_entry != nullptr; }

private:
    friend class MDW_EVENT_BUS;

    struct ENTRY;
    struct STATE;

    MDW_SUBSCRIPTION( std::weak_ptr<STATE> aState, std::shared_ptr<ENTRY> aEntry );

    std::weak_ptr<STATE>   m_state;
    std::shared_ptr<ENTRY> m_entry;
};

#endif // MDW_SUBSCRIPTION_H
