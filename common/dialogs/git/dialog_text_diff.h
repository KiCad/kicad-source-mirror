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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_TEXT_DIFF_H
#define DIALOG_TEXT_DIFF_H

#include <vector>

#include <dialogs/git/dialog_text_diff_base.h>
#include <git/text_diff.h>

class wxStyledTextCtrl;

/// Fill aCtrl with a line diff, added and removed lines in the diff colours, and make it read only.
void FillTextDiffControl( wxStyledTextCtrl* aCtrl, const std::vector<KIGIT::TEXT_DIFF_LINE>& aLines );


/// Read-only line diff, for the file types that have no visual diff.
class DIALOG_TEXT_DIFF : public DIALOG_TEXT_DIFF_BASE
{
public:
    DIALOG_TEXT_DIFF( wxWindow* aParent, const wxString& aTitle, const std::vector<KIGIT::TEXT_DIFF_LINE>& aLines );
};

#endif // DIALOG_TEXT_DIFF_H
