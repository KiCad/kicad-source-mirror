/**************/
/**	protos.h **/
/**************/

#ifndef PROTOS_H
#define PROTOS_H

extern int GenNetlistPcbnew( FILE* f, COMPONENT_LIST& list,
                             bool isEESchemaNetlist = true,
                             bool rightJustify = false );
extern bool LoadComponentFile( const wxString& fileName,
                               COMPONENT_LIST& list );

FOOTPRINT* GetModuleDescrByName( const wxString& FootprintName,
                                 FOOTPRINT_LIST& list );

#endif  // PROTOS_H

