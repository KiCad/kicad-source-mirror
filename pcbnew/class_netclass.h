/*************************************/
/* class to Net Classes */
/**************************************/

#ifndef CLASS_NETCLASS_H
#define CLASS_NETCLASS_H

/* this small class NET_DESIGN_PARAMS handles netclass parameters.
 *  This is a separate class because these parameters are also duplicated
 *  (for calculation time consideration) in each NETINFO_ITEM when making tests DRC and routing
 */
class NET_DESIGN_PARAMS
{
public:
    int m_TracksWidth;              // "Default" value for tracks thickness used to route this net
    int m_TracksMinWidth;           // Minimum value for tracks thickness (used in DRC)
    int m_ViasSize;                 // "Default" value for vias sizes used to route this net
    int m_ViasMinSize;              // Minimum value for  vias sizes (used in DRC)
    int m_Clearance;                // "Default" clearance when routing
    int m_MinClearance;             // Minimum value for clearance (used in DRC)
public:
    NET_DESIGN_PARAMS();
    ~NET_DESIGN_PARAMS() {}

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function ReadDescr
     * reads the data structures for this object from a FILE in "*.brd" format.
     * @param aFile The FILE to read to.
     * @return bool - true if success reading else false.
     */
    bool ReadDescr( FILE* aFile, int* aLineNum );
};

/**
 * @info A NETCLASS handles a list of nets and the parameters used to route or test these nets
 */
class NETCLASS
{
public:
    BOARD*            m_Parent;
    wxString          m_Name;               // Name of the net class
    wxArrayString     m_MembersNetNames;    // List of nets members of this class
    NET_DESIGN_PARAMS m_NetParams;          // values of net classes parameters

public:
    NETCLASS( BOARD* aParent, const wxString& aName = wxT( "default" ) );
    ~NETCLASS();

    /** Function GetMembersCount
     *@return the number of nets using this rule
     */
    unsigned GetMembersCount() const
    {
        return m_MembersNetNames.GetCount();
    }


    void ClearMembersList()
    {
        m_MembersNetNames.Clear();
    }


    void AddMember( const wxString& aNetname )
    {
        m_MembersNetNames.Add( aNetname );
    }


    wxString GetMemberName( unsigned aIdx ) const
    {
        if( aIdx < GetMembersCount() )
            return m_MembersNetNames[aIdx];
        else
            return wxEmptyString;
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function ReadDescr
     * reads the data structures for this object from a FILE in "*.brd" format.
     * @param aFile The FILE to read to.
     * @return bool - true if success reading else false.
     */
    bool ReadDescr( FILE* aFile, int* aLineNum );
};

/* This NETCLASS_LIST handles the list of NETCLASS for the board
 * Note: the NETCLASS_LIST is owner of all NETCLASS in list
 */
class NETCLASS_LIST
{
public:
    BOARD* m_Parent;
    std::vector <NETCLASS*> m_Netclass_List;

public:
    NETCLASS_LIST( BOARD* aParent = NULL );
    ~NETCLASS_LIST();
    void ClearList();

    /** Function GetNetClassCount()
     * @return the number of existing netclasses
     */
    unsigned GetNetClassCount()
    {
        return m_Netclass_List.size();
    }


    /** Function GetNetClass()
     * @param aIdx = the index in netclass list
     * @return a NETCLASS* pointer on the netclass
     */
    NETCLASS* GetNetClass( unsigned aIdx )
    {
        if( GetNetClassCount() && aIdx < GetNetClassCount() )
            return m_Netclass_List[aIdx];
        else
            return NULL;
    }


    /** Function AddNetclass()
     * @param aNetclass = a pointer to the netclass to add
     * @return true if Ok, false if cannot be added (mainly because a netclass with the same name exists)
     */
    bool AddNetclass( NETCLASS* aNetclass );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;
};


#endif      // #ifndef CLASS_NETCLASS_H
