	/*************************************************************************/
	/*	classe EQUIPOT: definition des elements relatifs aux equipotentielles */
	/*************************************************************************/


/* Representation des descriptions des equipotentielles */

class EQUIPOT: public EDA_BaseStruct
{
public:
	wxString m_Netname;		// nom du net
	int status;				// no route, hight light...
	int m_NetCode;			// numero de code interne du net
	int m_NbNodes;			// nombre de pads appartenant au net
	int m_NbLink;			// nombre de chevelus
	int m_NbNoconn;			// nombre de chevelus actifs
	int m_Masque_Layer;		// couches interdites (bit 0 = layer 0...)
	int m_Masque_Plan;		// couches mises en plan de cuivre
	int m_ForceWidth;		// specific width (O = default width)
	LISTE_PAD * m_PadzoneStart;// pointeur sur debut de liste pads du net
	LISTE_PAD * m_PadzoneEnd;	// pointeur sur fin de liste pads du net
	CHEVELU * m_RatsnestStart;	// pointeur sur debut de liste ratsnests du net
	CHEVELU * m_RatsnestEnd;	// pointeur sur fin de liste ratsnests du net

	EQUIPOT(EDA_BaseStruct * StructFather);
	~EQUIPOT(void);

	/* Effacement memoire de la structure */
	void UnLink( void );

	/* Readind and writing data on files */
   int ReadEquipotDescr(FILE * File, int * LineNum);
	int WriteEquipotDescr(FILE * File);
};


