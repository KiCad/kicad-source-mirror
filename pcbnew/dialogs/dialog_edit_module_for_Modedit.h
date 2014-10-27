/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_EDIT_MODULE_FOR_MODEDIT__
#define __DIALOG_EDIT_MODULE_FOR_MODEDIT__

// Include the wxFormBuider header base:
#include <vector>
#include <dialog_edit_module_for_Modedit_base.h>

class DIALOG_MODULE_MODULE_EDITOR : public DIALOG_MODULE_MODULE_EDITOR_BASE
{
private:

    FOOTPRINT_EDIT_FRAME*   m_parent;
    MODULE* m_currentModule;
    TEXTE_MODULE* m_referenceCopy;
    TEXTE_MODULE* m_valueCopy;
    std::vector <S3D_MASTER*> m_shapes3D_list;
    int m_lastSelected3DShapeIndex;
    S3DPOINT_VALUE_CTRL * m_3D_Scale;
    S3DPOINT_VALUE_CTRL * m_3D_Offset;
    S3DPOINT_VALUE_CTRL * m_3D_Rotation;

public:

    // Constructor and destructor
    DIALOG_MODULE_MODULE_EDITOR( FOOTPRINT_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_MODULE_MODULE_EDITOR();

private:
    void BrowseAndAdd3DShapeFile();
    void initModeditProperties();
    void Transfert3DValuesToDisplay( S3D_MASTER * aStruct3DSource );
    void TransfertDisplayTo3DValues( int aIndexSelection );

    // virtual event functions
    void OnEditValue( wxCommandEvent& event );
    void OnEditReference( wxCommandEvent& event );
    void On3DShapeSelection( wxCommandEvent& event );
    void On3DShapeNameSelected( wxCommandEvent& event );
    void Add3DShape( wxCommandEvent& event )
    {
        BrowseAndAdd3DShapeFile();
    }
    void Remove3DShape( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
};


#endif      //  __DIALOG_EDIT_MODULE_FOR_MODEDIT__
