/* debug_kbool_key_file_fct.h
*/

#ifndef _DEBUG_KBOOL_KEY_FILE_FCT_H_
#define _DEBUG_KBOOL_KEY_FILE_FCT_H_

/* These line must be uncommented only if you want to produce a file
* to debug kbool in zone filling algorithms
*/
//#define CREATE_KBOOL_KEY_FILES_FIRST_PASS 1
//#define CREATE_KBOOL_KEY_FILES 1

#if defined (CREATE_KBOOL_KEY_FILES) || (CREATE_KBOOL_KEY_FILES_FIRST_PASS)

// Allows (or not) 0 degree orientation thermal shapes, for kbool tests
//#define CREATE_KBOOL_KEY_FILES_WITH_0_DEG

#define KEYFILE_FILENAME "pcbnew_dbgfile.key"

/** function CreateKeyFile
 * open KEYFILE_FILENAME file
 * and create header
 */
void CreateKeyFile();

/** function CloseKeyFile
 * close KEYFILE_FILENAME file
 */
void CloseKeyFile();

/* create header to start an entity description
*/
void OpenKeyFileEntity(const char * aName);
/* close the entity description
*/
void CloseKeyFileEntity();

/* polygon creations:
*/
void CopyPolygonsFromFilledPolysListToKeyFile( ZONE_CONTAINER* aZone, int aLayer);
void StartKeyFilePolygon( int aLayer);
void AddKeyFilePointXY( int aXcoord, int aYcoord);
void EndKeyFilePolygon();

#endif  // CREATE_KBOOL_KEY_FILES

#endif  // _DEBUG_KBOOL_KEY_FILE_FCT_H_

