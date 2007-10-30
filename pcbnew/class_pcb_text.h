/************************************/
/* fonctions de la classe TEXTE_PCB */
/************************************/
#ifndef CLASS_PCB_TEXT_H
#define CLASS_PCB_TEXT_H

#include "base_struct.h"

class TEXTE_PCB : public BOARD_ITEM, public EDA_TextStruct
{
public:
	TEXTE_PCB( BOARD_ITEM* parent );
	TEXTE_PCB( TEXTE_PCB* textepcb );
	~TEXTE_PCB();

	/* supprime du chainage la structure Struct */
	void UnLink();

	/* duplicate structure */
	void Copy( TEXTE_PCB* source );

	void Draw( WinEDA_DrawPanel * panel, wxDC * DC,
					const wxPoint & offset, int DrawMode );

	// File Operations:
	int ReadTextePcbDescr( FILE* File, int* LineNum );
    
    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.pcb" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */ 
    bool Save( FILE* aFile ) const;
    
    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */ 
    void    Display_Infos( WinEDA_DrawFrame* frame );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos )
    {
        return EDA_TextStruct::HitTest( refPos );
    }
    
    
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT("PTEXT");
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

#endif 	// #define CLASS_PCB_TEXT_H
