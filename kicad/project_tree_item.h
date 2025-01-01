/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras
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

#ifndef PROJECT_TREE_ITEM_H
#define PROJECT_TREE_ITEM_H


#include <wx/treebase.h>

#include "tree_file_type.h"

class PROJECT_TREE_PANE;
class wxTreeCtrl;

/**
 * Handle one item (a file or a directory name) for the tree file.
 */
class PROJECT_TREE_ITEM : public wxTreeItemData
{
public:

    PROJECT_TREE_ITEM( TREE_FILE_TYPE type, const wxString& data, wxTreeCtrl* parent );

    PROJECT_TREE_ITEM() :
            m_parent( nullptr )
    { }

    PROJECT_TREE_ITEM( const PROJECT_TREE_ITEM& src ) :
            m_type( src.m_type ),
            m_file_name( src.m_file_name ),
            m_parent( src.m_parent )
    {
        SetState( src.m_state );
        m_isPopulated = false;
    }

    TREE_FILE_TYPE GetType() const              { return m_type; }
    void SetType( TREE_FILE_TYPE aType )        { m_type = aType; }

    const wxString& GetFileName() const         { return m_file_name; }
    void SetFileName( const wxString& name )    { m_file_name = name; }

    bool IsRootFile() const                     { return m_isRootFile; }
    void SetRootFile( bool aValue )             { m_isRootFile = aValue; }

    bool IsPopulated() const                    { return m_isPopulated; }
    void SetPopulated( bool aValue )            { m_isPopulated = aValue; }

    /**
     * @return the path of an item.
     * if this item is a directory, returns the stored filename
     * if this is a file, returns its path
     */
    const wxString GetDir() const;

    bool Rename( const wxString& name, bool check = true );
    void Delete();
    void Print();
    void Activate( PROJECT_TREE_PANE* aTreePrjFrame );
    void SetState( int state );

    /**
     * Determine if a file can be deleted via the project tree pane.
     *
     * @note Any of the files that could potentially break a project are flagged as cannot delete
     *       or rename.
     *
     * @return false if the file managed by this item cannot be deleted or true if it can.
     */
    bool CanDelete() const;
    bool CanRename() const { return CanDelete(); }

private:
    TREE_FILE_TYPE  m_type;         // = TREE_PROJECT, TREE_DIRECTORY ...
    wxString        m_file_name;    // Filename for a file, or directory name
    bool            m_isRootFile;   // True if m_Filename is a root schematic (same name as project)
    bool            m_isPopulated;  // True if the name is a directory, and its content was read
    wxTreeCtrl*     m_parent;
    int             m_state;
};

#endif  // PROJECT_TREE_ITEM_H
