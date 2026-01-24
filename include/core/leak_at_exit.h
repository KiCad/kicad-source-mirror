/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_LEAK_AT_EXIT_H
#define KICAD_LEAK_AT_EXIT_H

#include <mutex>

/**
 * @file leak_at_exit.h
 * @brief Utilities for intentionally "leaking" memory at program exit.
 *
 * When a program exits, the OS reclaims all process memory anyway. Running
 * destructors that only free memory is wasted work that slows down shutdown.
 * This is especially noticeable for large caches like library symbol/footprint
 * data structures with deep object hierarchies.
 *
 * When built with address sanitizer (KICAD_SANITIZE_ADDRESS), these utilities
 * use __lsan_ignore_object() to mark the memory as intentionally leaked,
 * preventing false positive leak reports.
 */

#ifdef KICAD_SANITIZE_ADDRESS
#include <sanitizer/lsan_interface.h>
#define LSAN_IGNORE( ptr ) __lsan_ignore_object( ptr )
#else
#define LSAN_IGNORE( ptr ) (void)( ptr )
#endif


/**
 * A wrapper for static data that should not be destroyed at program exit.
 *
 * Usage:
 *   // Instead of:
 *   static std::map<wxString, Data> GlobalData;
 *
 *   // Use:
 *   static LEAK_AT_EXIT<std::map<wxString, Data>> GlobalData;
 *
 * The wrapped object is heap-allocated on first access and never freed.
 * This avoids running potentially expensive destructors during static
 * destruction, speeding up program exit.
 *
 * Each LEAK_AT_EXIT instance has its own storage, so multiple static
 * variables of the same type will each get their own heap allocation.
 *
 * Thread-safe for concurrent first access (uses std::call_once).
 */
template <typename T>
class LEAK_AT_EXIT
{
public:
    LEAK_AT_EXIT() : m_instance( nullptr ) {}

    // Non-copyable, non-movable
    LEAK_AT_EXIT( const LEAK_AT_EXIT& ) = delete;
    LEAK_AT_EXIT& operator=( const LEAK_AT_EXIT& ) = delete;
    LEAK_AT_EXIT( LEAK_AT_EXIT&& ) = delete;
    LEAK_AT_EXIT& operator=( LEAK_AT_EXIT&& ) = delete;

    /**
     * Access the underlying object, creating it on first call.
     * The object is intentionally never destroyed.
     */
    T& Get()
    {
        std::call_once( m_initFlag, [this]()
        {
            m_instance = new T();
            LSAN_IGNORE( m_instance );
        } );

        return *m_instance;
    }

    T& operator*() { return Get(); }
    T* operator->() { return &Get(); }

    // Allow implicit conversion to reference for compatibility
    operator T&() { return Get(); }

private:
    T*             m_instance;
    std::once_flag m_initFlag;
};


#endif // KICAD_LEAK_AT_EXIT_H
