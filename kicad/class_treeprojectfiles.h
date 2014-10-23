/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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

/*
 * file class_treeprojectfiles.h
 */

#ifndef CLASS_TREEPROJECTFILES_H
#define CLASS_TREEPROJECTFILES_H

/** Class TREEPROJECTFILES
 * This is the class to show (as a tree) the files in the project directory
 */
class TREEPROJECTFILES : public wxTreeCtrl
{
    DECLARE_DYNAMIC_CLASS( TREEPROJECTFILES )
private:
    TREE_PROJECT_FRAME* m_Parent;
    wxImageList*     m_ImageList;

public:

    TREE_PROJECT_FRAME* GetParent() const
    {
        return m_Parent;
    }


    TREEPROJECTFILES( TREE_PROJECT_FRAME* parent );
    ~TREEPROJECTFILES();
private:
    /* overridden sort function */
    int OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 );
};

#endif  // CLASS_TREEPROJECTFILES_H
