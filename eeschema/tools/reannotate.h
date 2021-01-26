/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EESCHEMA_TOOLS_REANNOTATE_H_
#define EESCHEMA_TOOLS_REANNOTATE_H_

#include <reporter.h>
/**
 * A wrapper for reporting to a wxString object.
 */
class WX_STRING_REPORTER_FILTERED : public REPORTER
{
    SEVERITY m_MinSeverity;

public:
    WX_STRING_REPORTER_FILTERED( SEVERITY aSeverity );
    virtual ~WX_STRING_REPORTER_FILTERED();

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
    wxString  m_string = "";
    bool      HasMessage() const override;
};

///< Backannotate the schematic with a netlist sent from Pcbnew.
///< Reply with a string consisting of errors or warnings. If empty no errors
void ReannotateFromPCBNew( SCH_EDIT_FRAME* aFrame, std::string& aNetlist );

#endif /* EESCHEMA_TOOLS_REANNOTATE_H_ */
