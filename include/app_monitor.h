/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#pragma once

#include <kicommon.h>
#include <string>
#include <wx/string.h>

namespace APP_MONITOR
{
	enum class BREADCRUMB_TYPE
    {
        DEFAULT,
        DBG,
        ERR,
        NAVIGATION,
        INFO,
        QUERY,
        TRANSACTION,
        UI,
        USER,
    };

	enum class BREADCRUMB_LEVEL
	{
        FATAL,
        ERR,
        WARNING,
        INFO,
        DBG
	};

    class TRANSACTION_IMPL;

    class KICOMMON_API TRANSACTION
    {
    public:
        TRANSACTION( const std::string& aName, const std::string& aOperation );
        ~TRANSACTION();

        void Start();
        void StartSpan( const std::string& aOperation, const std::string& aDescription );
        void FinishSpan();
        void Finish();

#ifdef KICAD_USE_SENTRY
    private:
        // We use a IMPL to avoid seeding sentry everywhere
        TRANSACTION_IMPL* m_impl = nullptr;
#endif
    };

	KICOMMON_API void AddBreadcrumb( BREADCRUMB_TYPE aType, const wxString& aMsg, const wxString& aCategory,
                                     BREADCRUMB_LEVEL aLevel = BREADCRUMB_LEVEL::INFO );


    KICOMMON_API void AddNavigationBreadcrumb( const wxString& aMsg, const wxString& aCategory );

    KICOMMON_API void AddTransactionBreadcrumb( const wxString& aMsg, const wxString& aCategory );
}