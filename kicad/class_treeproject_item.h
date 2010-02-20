/*
 * class_treeproject_item.h
 */


/** class TREEPROJECT_ITEM
 * Handle one item (a file or a directory name) for the tree file
 */
class TREEPROJECT_ITEM : public wxTreeItemData
{
public:
    TreeFileType m_Type;        // = TREE_PROJECT, TREE_DIRECTORY ...
    wxString     m_FileName;    // Filename for a file, or directory name
    bool         m_IsRootFile;      // True if m_Filename is a root schematic (same name as project)
    bool         m_WasPopulated;    // True the name is a directory, and its containt was read

private:
    wxTreeCtrl*  m_Parent;
    wxMenu       m_fileMenu;
    int          m_State;

public:

    TREEPROJECT_ITEM( TreeFileType type, const wxString& data,
                      wxTreeCtrl* parent );
    TREEPROJECT_ITEM() : m_Parent( NULL ) { }

    TREEPROJECT_ITEM( const TREEPROJECT_ITEM& src ) :
        m_Type( src.m_Type ),
        m_FileName( src.m_FileName ),
        m_Parent( src.m_Parent )
    {
        SetState( src.m_State );
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


    wxString GetDir() const;

    void     OnRename( wxTreeEvent& event, bool check = true );
    bool     Rename( const wxString& name, bool check = true );
    bool     Delete( bool check = true );
    void     Move( TREEPROJECT_ITEM* dest );
    void     Activate( TREE_PROJECT_FRAME* prjframe );

    const wxMenu* GetMenu()
    {
        return &m_fileMenu;
    }


    void                  SetState( int state );

#ifdef KICAD_PYTHON
    boost::python::object GetFileNamePy() const;
    bool                  RenamePy( const boost::python::str& newname,
                                    bool                      check = true );

    boost::python::object GetDirPy() const;

    boost::python::object GetIdPy() const;

    boost::python::object GetMenuPy();

#endif
};
