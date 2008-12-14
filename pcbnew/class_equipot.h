/*************************************************/
/*	classe EQUIPOT: Class to handle info on nets */
/*************************************************/

class EQUIPOT : public BOARD_ITEM
{
private:
    int        m_NetCode;       // this is a number equivalent to the net name
                                // Used for fast comparisons in rastnest and DRC computations.
    wxString   m_Netname;       // Full net name like /mysheet/mysubsheet/vout used by eeschema
    wxString   m_ShortNetname;  // short net name, like vout from /mysheet/mysubsheet/vout


public:
    int        status;          // no route, hight light...
    int        m_NbNodes;       // Pads count for this net
    int        m_NbLink;        // Ratsnets count for this net
    int        m_NbNoconn;      // Ratsnets remaining to route count
    int        m_Masque_Layer;  // couches interdites (bit 0 = layer 0...)
    int        m_Masque_Plan;   // couches mises en plan de cuivre
    int        m_ForceWidth;    // specific width (O = default width)
    LISTE_PAD* m_PadzoneStart;  // pointeur sur debut de liste pads du net
    LISTE_PAD* m_PadzoneEnd;    // pointeur sur fin de liste pads du net
    CHEVELU*   m_RatsnestStart; // pointeur sur debut de liste ratsnests du net
    CHEVELU*   m_RatsnestEnd;   // pointeur sur fin de liste ratsnests du net

    EQUIPOT( BOARD_ITEM* aParent );
    ~EQUIPOT();

    EQUIPOT*    Next() const { return (EQUIPOT*) Pnext; }
    EQUIPOT*    Back() const { return (EQUIPOT*) Pback; }

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return wxPoint& - The position of this object, non-const so it
     *          can be changed
     * A dummy to satisfy pure virtual BOARD::GetPosition()
     */
    wxPoint& GetPosition();

    /* Readind and writing data on files */
    int   ReadDescr( FILE* File, int* LineNum );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;


    /** function Draw
     * @todo we actually could show a NET, simply show all the tracks and pads or net name on pad and vias
     */
    void Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                      int aDrawMode, const wxPoint& offset = ZeroOffset );


    /**
     * Function GetNet
     * @return int - the netcode
     */
    int GetNet() const { return m_NetCode; }
    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }

    /**
     * Function GetNetname
     * @return const wxString * , a pointer to the full netname
     */
    wxString GetNetname() const { return m_Netname; }
    /**
     * Function GetShortNetname
     * @return const wxString * , a pointer to the short netname
     */
    wxString GetShortNetname() const { return m_ShortNetname; }

    /**
     * Function SetNetname
     * @param const wxString : the new netname
     */
    void SetNetname( const wxString & aNetname );


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT("NET");
    }


#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );
#endif

};

