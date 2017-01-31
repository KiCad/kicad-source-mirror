/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_EXCHANGE_MODULES_H_
#define DIALOG_EXCHANGE_MODULES_H_

#include <dialog_exchange_modules_base.h>

#include <board_commit.h>

class PCB_EDIT_FRAME;
class MODULE;

class DIALOG_EXCHANGE_MODULE : public DIALOG_EXCHANGE_MODULE_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    MODULE*         m_currentModule;
    static int      m_selectionMode;    // Remember the last exchange option

public:
    DIALOG_EXCHANGE_MODULE( PCB_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_EXCHANGE_MODULE() { };

private:
    void OnSelectionClicked( wxCommandEvent& event ) override;
    void OnOkClick( wxCommandEvent& event ) override;
    void OnQuit( wxCommandEvent& event ) override;
    void BrowseAndSelectFootprint( wxCommandEvent& event ) override;
    void ViewAndSelectFootprint( wxCommandEvent& event ) override;
    void RebuildCmpList( wxCommandEvent& event ) override;
    void init();

    bool changeCurrentFootprint();
    bool changeSameFootprints( bool aUseValue);
    bool changeAllFootprints();
    bool change_1_Module( MODULE*            aModule,
                          const LIB_ID&      aNewFootprintFPID,
                          bool               eShowError );

    BOARD_COMMIT m_commit;
};

#endif // DIALOG_EXCHANGE_MODULES_H_
