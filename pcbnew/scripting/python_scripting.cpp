
#include <python_scripting.h>

/* init functions defined by swig */

extern "C" void init_kicad(void);
extern "C" void init_pcbnew(void);


/* python inittab that links module names to module init functions 
 * we will rebuild it to include the original python modules plus
 * our own ones 
 */

struct _inittab SwigImportInittab[1000];
static int  SwigNumModules = 0;


/* Add a name + initfuction to our SwigImportInittab */

static void swigAddModule(const char *name, void (*initfunc)()) {
        SwigImportInittab[SwigNumModules].name = (char *)name;
        SwigImportInittab[SwigNumModules].initfunc = initfunc;
        SwigNumModules++;
        SwigImportInittab[SwigNumModules].name = (char *) 0;
        SwigImportInittab[SwigNumModules].initfunc = 0;
}

/* Add the builting python modules */

static void swigAddBuiltin() {
        int i = 0;
        while (PyImport_Inittab[i].name) {
                swigAddModule(PyImport_Inittab[i].name, PyImport_Inittab[i].initfunc);
                i++;
        }

}
static void swigAddModules()
{
	//swigAddModule("_kicad",init_kicad);
	swigAddModule("_pcbnew",init_pcbnew);
}

static void swigSwitchPythonBuiltin()
{
        PyImport_Inittab = SwigImportInittab;
}

static PCB_EDIT_FRAME *PcbEditFrame=NULL;

BOARD *GetBoard()
{
	if (PcbEditFrame) return PcbEditFrame->GetBoard();
	else return NULL;
}

void pythonSetPcbEditFrame(PCB_EDIT_FRAME *aPCBEdaFrame)
{
	PcbEditFrame = aPCBEdaFrame;
}



void pcbnewInitPythonScripting()
{
    swigAddBuiltin();
    swigAddModules();
    swigSwitchPythonBuiltin();
    
#if 0
    /* print the list of modules available from python */
    while(PyImport_Inittab[i].name)
    {
	printf("name[%d]=>%s\n",i,PyImport_Inittab[i].name);
	i++;
    }
#endif

    Py_Initialize();

    /* setup the scripting path, we may need to add the installation path 
       of kicad here */

    PyRun_SimpleString("import sys\n"
		       "sys.path.append(\".\")\n"
                       "import pcbnew\n");


}
