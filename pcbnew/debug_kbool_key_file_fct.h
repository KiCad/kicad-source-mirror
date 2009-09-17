/* debug_kbool_key_file_fct.h
*/

#ifndef _DEBUG_KBOOL_KEY_FILE_FCT_H_
#define _DEBUG_KBOOL_KEY_FILE_FCT_H_

/* This line must be uncommented only if you wan to produce a file
* to debug kbool
*/
//#define CREATE_KBOOL_KEY_FILES

#ifdef CREATE_KBOOL_KEY_FILES

// Allows or not) 0 degree orientation thermal shapes, for kbool tests
//#define CREATE_KBOOL_KEY_FILES_WITH_0_DEG

#define KEYFILE_FILENAME "dbgfile.key"

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
void OpenEntity(const char * aName);
/* close the entity description
*/
void CloseEntity();

/* polygon creations:
*/
void CopyPolygonsFromFilledPolysListToKeyFile( ZONE_CONTAINER* aZone, int aLayer);
void StartPolygon(int aCornersCount, int aLayer);
void AddPointXY( int aXcoord, int aYcoord);
void EndElement();

#endif  // CREATE_KBOOL_KEY_FILES

#endif  // _DEBUG_KBOOL_KEY_FILE_FCT_H_

