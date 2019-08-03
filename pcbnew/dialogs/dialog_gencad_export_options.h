/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __DIALOG_GENCAD_EXPORT_OPTIONS_H__
#define __DIALOG_GENCAD_EXPORT_OPTIONS_H__

#include <dialog_shim.h>

class PCB_EDIT_FRAME;
class wxFilePickerCtrl;

///> Settings for GenCAD exporter
enum GENCAD_EXPORT_OPT
{
    FLIP_BOTTOM_PADS,       // flip bottom components padstacks geometry
    UNIQUE_PIN_NAMES,       // generate unique pin names
    INDIVIDUAL_SHAPES,      // generate a shape for each component
    USE_AUX_ORIGIN,         // use auxiliary axis as origin
    STORE_ORIGIN_COORDS     // saves the origin point coordinates or (0, 0)
};


class DIALOG_GENCAD_EXPORT_OPTIONS : public DIALOG_SHIM
{
public:
    DIALOG_GENCAD_EXPORT_OPTIONS( PCB_EDIT_FRAME* aParent, const wxString& aPath );
    ~DIALOG_GENCAD_EXPORT_OPTIONS();

    ///> Checks whether an option has been selected
    bool GetOption( GENCAD_EXPORT_OPT aOption ) const;

    ///> Returns all export settings
    std::map<GENCAD_EXPORT_OPT, bool> GetAllOptions() const;

    ///> Returns the selected file path
    wxString GetFileName() const;

protected:
    bool TransferDataFromWindow() override;

    ///> Creates checkboxes for GenCAD export options
    void createOptCheckboxes();

    std::map<GENCAD_EXPORT_OPT, wxCheckBox*> m_options;

    // Widgets
    wxGridSizer*      m_optsSizer;
    wxFilePickerCtrl* m_filePicker;
};

#endif //__DIALOG_GENCAD_EXPORT_OPTIONS_H__
