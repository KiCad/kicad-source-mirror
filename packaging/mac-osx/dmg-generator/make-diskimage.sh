# Create a read-only disk image of the contents of a folder
#
# Usage: make-diskimage <image_file>
#                       <src_folder>
#                       <volume_name>
#                       <applescript>
#                       <artpath>
#                       <eula_resource_file>

set -e;

DMG_DIRNAME=`dirname $1`
DMG_DIR=`cd $DMG_DIRNAME > /dev/null; pwd`
DMG_NAME=`basename $1`
DMG_TEMP_NAME=${DMG_DIR}/rw.${DMG_NAME}
SRC_FOLDER=`cd $2 > /dev/null; pwd`
VOLUME_NAME=$3

# optional arguments
APPLESCRIPT=$4
ART_PATH=$5
EULA_RSRC=$6

# Create the image
echo "Creating disk image..."
rm -f "${DMG_TEMP_NAME}"
hdiutil create -srcfolder "${SRC_FOLDER}" -volname "${VOLUME_NAME}" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW "${DMG_TEMP_NAME}"

# mount it
echo "Mounting disk image..."
MOUNT_DIR="/Volumes/${VOLUME_NAME}"
DEV_NAME=`hdiutil attach -readwrite -noverify -noautoopen "${DMG_TEMP_NAME}" | egrep '^/dev/' | sed 1q | awk '{print $1}'`

cp RightDS_Store "/Volumes/${VOLUME_NAME}/.DS_Store"

# run applescript
if [ ! -z "${APPLESCRIPT}" -a "${APPLESCRIPT}" != "-null-" ]; then
#	osascript "${APPLESCRIPT}"
    #   pass the applescript our volume name and our artwork path, to its process_disk_image function
    echo "Running Applescript: ./AdiumApplescriptRunner \"${APPLESCRIPT}\" process_disk_image \"${VOLUME_NAME}\""
    ./AdiumApplescriptRunner "${APPLESCRIPT}" process_disk_image "${VOLUME_NAME}" "${ART_PATH}" || true
    echo "Done running the applescript..."
fi


# run shell script
# if [ ! -z "${SHELLSCRIPT}" -a "${SHELLSCRIPT}" != "-null-" ]; then
#   ./${SHELLSCRIPT} \"${VOLUME_NAME}\"
# fi

# make sure it's not world writeable
echo "Fixing permissions..."
chmod -Rf go-w "${MOUNT_DIR}" || true

# make the top window open itself on mount:
if [ -x /usr/local/bin/openUp ]; then
    /usr/local/bin/openUp "${MOUNT_DIR}"
fi

# unmount
echo "Unmounting disk image..."
hdiutil detach "${DEV_NAME}"

# compress image
echo "Compressing disk image..."
hdiutil convert "${DMG_TEMP_NAME}" -format UDBZ -o "${DMG_DIR}/${DMG_NAME}"
rm -f "${DMG_TEMP_NAME}"

# adding EULA resources
if [ ! -z "${EULA_RSRC}" -a "${EULA_RSRC}" != "-null-" ]; then
        echo "adding EULA resources"
        hdiutil unflatten "${DMG_DIR}/${DMG_NAME}"
        /Developer/Tools/ResMerger -a "${EULA_RSRC}" -o "${DMG_DIR}/${DMG_NAME}"
        hdiutil flatten "${DMG_DIR}/${DMG_NAME}"
fi

echo "Disk image done"
exit 0
