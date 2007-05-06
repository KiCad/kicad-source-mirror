	/***************************************************/
	/* class_text_module.h : texts module description  */
	/***************************************************/

/* Description des Textes sur Modules : */
#define TEXT_is_REFERENCE 0
#define TEXT_is_VALUE 1
#define TEXT_is_DIVERS 2


class TEXTE_MODULE: public EDA_BaseStruct
{
public:
	int m_Layer;			// layer number
	int m_Width;
	wxPoint m_Pos;			// Real coord
	wxPoint m_Pos0;			// coord du debut du texte /ancre, orient 0
	char m_Unused;			// unused (reserved for future extensions)
	char m_Miroir ;			// vue normale / miroir
	char m_NoShow;			// 0: visible 1: invisible  (bool)
	char m_Type;			// 0: ref,1: val, autre = 2..255
	int m_Orient;			// orientation en 1/10 degre
	wxSize m_Size;			// dimensions (en X et Y) du texte
	wxString m_Text;

	public:
	TEXTE_MODULE(MODULE * parent, int text_type = TEXT_is_DIVERS );
	~TEXTE_MODULE(void);

	/* supprime du chainage la structure Struct */
	void UnLink( void );

    void Copy(TEXTE_MODULE * source);	// copy structure
   
	/* Gestion du texte */
	void SetWidth(int new_width);
	int GetLength(void);		/* text length */
	int Pitch(void);			/* retourne le pas entre 2 caracteres */
	int GetDrawRotation(void);	// Return text rotation for drawings and plotting

	void SetDrawCoord(void);	// mise a jour des coordonnées absolues de tracé
								// a partir des coord relatives
	void SetLocalCoord(void);	// mise a jour des coordonnées relatives
								// a partir des coord absolues de tracé

	/* Reading and writing data on files */
	int WriteDescr( FILE * File );
	int ReadDescr( FILE * File, int * LineNum = NULL);

	/* drawing functions */
	void Draw(WinEDA_DrawPanel * panel, wxDC * DC,  wxPoint offset, int draw_mode);

	/* locate functions */
	int Locate(const wxPoint & posref);
};

