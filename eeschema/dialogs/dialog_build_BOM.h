/**
 * @file dialog_build_BOM.h
 */

/* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef _DIALOG_BUILD_BOM_H_
#define _DIALOG_BUILD_BOM_H_

#include <dialog_build_BOM_base.h>


class EDA_DRAW_FRAME;
class SCH_COMPONENT;
class wxConfig;

class DIALOG_BUILD_BOM : public DIALOG_BUILD_BOM_BASE
{
private:
    EDA_DRAW_FRAME* m_parent;
    wxConfig*       m_config;
    wxString        m_listFileName;     // The full filename of the file report.

private:
    void    OnRadioboxSelectFormatSelected( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );

    void    SavePreferences();
    void    Init();
    void    Create_BOM_Lists( int   aTypeFile,
                              bool  aIncludeSubComponents,
                              char  aExportSeparatorSymbol,
                              bool  aRunBrowser );

    void    CreatePartsAndLabelsFullList( bool aIncludeSubComponents );

    /**
     * Function CreateSpreadSheetPartsFullList
     * prints a list of components, in a form which can be imported by a
     * spreadsheet.  Form is:
     *  reference; cmp value; \<footprint\>; \<field1\>; ...;
     * Components are sorted by reference
     * @param aIncludeSubComponents = true to print sub components
     * @param aPrintLocation = true to print components location
     *        (only possible when aIncludeSubComponents == true)
     * @param aGroupRefs = true to group components references, when other fieds
     *          have the same value
     */
    void    CreateSpreadSheetPartsFullList( bool aIncludeSubComponents,
                                            bool aPrintLocation,
                                            bool aGroupRefs );

    /**
     * Function CreateSpreadSheetPartsShortList
     * prints a list of components, in a form which can be imported by a spreadsheet.
     * components having the same value and the same footprint
     * are grouped on the same line
     * Form is:
     *  value; number of components; list of references; \<footprint\>; \<field1\>; ...;
     * list is sorted by values
     */
    void    CreateSpreadSheetPartsShortList();

    bool    IsFieldChecked( int aFieldId );

public:
    DIALOG_BUILD_BOM( EDA_DRAW_FRAME* parent );

    // ~DIALOG_BUILD_BOM() {};
};

#endif  // _DIALOG_BUILD_BOM_H_
