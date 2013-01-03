/*
 * class_treeproject_item.h
 */


/** class TREEPROJECT_ITEM
 * Handle one item (a file or a directory name) for the tree file
 */
class TREEPROJECT_ITEM : public wxTreeItemData
{
public:
    TreeFileType    m_Type;         // = TREE_PROJECT, TREE_DIRECTORY ...
    wxString        m_FileName;     // Filename for a file, or directory name
    bool            m_IsRootFile;   // True if m_Filename is a root schematic (same name as project)
    bool            m_WasPopulated; // True the name is a directory, and its content was read
private:
    wxTreeCtrl*     m_parent;
    int             m_state;
public:

    TREEPROJECT_ITEM( TreeFileType type, const wxString& data,
                      wxTreeCtrl* parent );
    TREEPROJECT_ITEM() : m_parent( NULL ) { }

    TREEPROJECT_ITEM( const TREEPROJECT_ITEM& src ) :
        m_Type( src.m_Type ), m_FileName( src.m_FileName ), m_parent( src.m_parent )
    {
        SetState( src.m_state );
        m_WasPopulated = false;
    }

    TreeFileType GetType() const
    {
        return m_Type;
    }

    void SetType( TreeFileType aType )
    {
        m_Type = aType;
    }

    wxString GetFileName() const
    {
        return m_FileName;
    }

    void SetFileName( const wxString& name )
    {
        m_FileName = name;
    }

    /**
     * @return the path of an item.
     * if this item is a directory, returns the stored filename
     * if this is a file, returns its path
     */
    wxString    GetDir() const;

    bool        Rename( const wxString& name, bool check = true );
    bool        Delete( bool check = true );
    void        Activate( TREE_PROJECT_FRAME* prjframe );
    void SetState( int state );
};
