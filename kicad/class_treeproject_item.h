#ifndef TREEPROJECT_ITEM_H_
#define TREEPROJECT_ITEM_H_

/**
 * Class TREEPROJECT_ITEM
 * handles one item (a file or a directory name) for the tree file
 */
class TREEPROJECT_ITEM : public wxTreeItemData
{
    //friend class KICAD_MANAGER_FRAME;

public:

    TREEPROJECT_ITEM( TreeFileType type, const wxString& data,
                      wxTreeCtrl* parent );

    TREEPROJECT_ITEM() : m_parent( NULL ) { }

    TREEPROJECT_ITEM( const TREEPROJECT_ITEM& src ) :
        m_Type( src.m_Type ), m_file_name( src.m_file_name ), m_parent( src.m_parent )
    {
        SetState( src.m_state );
        m_IsPopulated = false;
    }

    TreeFileType GetType() const                { return m_Type; }
    void SetType( TreeFileType aType )          { m_Type = aType; }

    const wxString& GetFileName() const         { return m_file_name; }
    void SetFileName( const wxString& name )
    {
        m_file_name = name;
        // DBG(printf("%s: '%s'\n", __func__, TO_UTF8( name ) );)
    }

    bool IsRootFile() const                     { return m_IsRootFile; }
    void SetRootFile( bool aValue )             { m_IsRootFile = aValue; }

    bool IsPopulated() const                    { return m_IsPopulated; }
    void SetPopulated( bool aValue )            { m_IsPopulated = aValue; }

    /**
     * @return the path of an item.
     * if this item is a directory, returns the stored filename
     * if this is a file, returns its path
     */
    const wxString GetDir() const;

    bool Rename( const wxString& name, bool check = true );
    bool Delete( bool check = true );
    void Activate( TREE_PROJECT_FRAME* prjframe );
    void SetState( int state );


private:
    TreeFileType    m_Type;         // = TREE_PROJECT, TREE_DIRECTORY ...
    wxString        m_file_name;    // Filename for a file, or directory name
    bool            m_IsRootFile;   // True if m_Filename is a root schematic (same name as project)
    bool            m_IsPopulated;  // True if the name is a directory, and its content was read
    wxTreeCtrl*     m_parent;
    int             m_state;
};

#endif  // TREEPROJECT_ITEM_H_
