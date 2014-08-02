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
