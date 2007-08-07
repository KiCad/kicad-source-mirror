/************************************/
/* fonctions de la classe TEXTE_PCB */
/************************************/
#ifndef CLASS_PCB_TEXT_H
#define CLASS_PCB_TEXT_H

#include "base_struct.h"

class TEXTE_PCB: public EDA_BaseStruct, public EDA_TextStruct
{
public:
	TEXTE_PCB(EDA_BaseStruct * parent);
	TEXTE_PCB(TEXTE_PCB * textepcb);
	~TEXTE_PCB(void);

	/* supprime du chainage la structure Struct */
	void UnLink( void );

	/* duplicate structure */
	void Copy(TEXTE_PCB * source);

	void Draw(WinEDA_DrawPanel * panel, wxDC * DC,
					const wxPoint & offset, int DrawMode);

	// File Operations:
	int ReadTextePcbDescr(FILE * File, int * LineNum);
	int WriteTextePcbDescr(FILE * File);

#if defined(DEBUG)
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT("PTEXT");
    }

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
