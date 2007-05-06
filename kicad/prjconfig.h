	/**********************************************************/
	/* prjconfig.h : configuration: definition des structures */
	/**********************************************************/



/* Liste des parametres */

static PARAM_CFG_WXSTRING SchematicRootFileNameCfg
(
	wxT("RootSch"),			  /* identification */
	&g_SchematicRootFileName /* Adresse du parametre */
);

static PARAM_CFG_WXSTRING BoardFileNameCfg
(
	wxT("BoardNm"),		/* identification */
	&g_BoardFileName	/* Adresse du parametre */
);


static PARAM_CFG_BASE * CfgParamList[] =
{
	& SchematicRootFileNameCfg,
	& BoardFileNameCfg,
	NULL
};

