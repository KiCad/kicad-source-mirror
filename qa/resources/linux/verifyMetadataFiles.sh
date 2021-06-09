#!/bin/bash
# WARNING: This script will modify file associations and MIME types on the system it
# runs on. Use at your own risk!

# The root of the code directory is the first argument
KICAD_CODE_DIR=$1

# The root of the build directory is the second argument
KICAD_BUILD_DIR=$2

KICAD_LAUNCHER_DIR=$KICAD_BUILD_DIR/resources/linux/launchers
KICAD_METAINFO_DIR=$KICAD_BUILD_DIR/resources/linux/metainfo
KICAD_MIME_DIR=$KICAD_BUILD_DIR/resources/linux/mime
KICAD_ICON_DIR=$KICAD_CODE_DIR/resources/linux/icons/hicolor

KICAD_MIME_TEST_FILES=$KICAD_CODE_DIR/qa/resources/linux/mimeFiles

###################################################################
# Verify the metainfo file
# There are two different checkers we can use, and each distro prefers
# different ones - so lets just test using them both and make them both
# happy.
###################################################################
METAINFO_VALID=1
echo "Validating metainfo files"

# Test using appstreamcli to see its errors
appstreamcli validate --explain --pedantic $KICAD_METAINFO_DIR/org.kicad.kicad.metainfo.xml
[ $? -ne 0 ] && METAINFO_VALID=0

# Test using the appstream-util package to see its errors
appstream-util validate-strict $KICAD_METAINFO_DIR/org.kicad.kicad.metainfo.xml
[ $? -ne 0 ] && METAINFO_VALID=0

if [ $METAINFO_VALID -eq 1 ]
then
    echo "Metainfo files passed both checkers successfully"
fi


###################################################################
# Verify the launcher files
###################################################################
LAUNCHERS_VALID=1

echo ""
echo "Validating desktop launcher files"

# Bitmap2component
desktop-file-validate $KICAD_LAUNCHER_DIR/org.kicad.bitmap2component.desktop
[ $? -ne 0 ] && LAUNCHERS_VALID=0

# Eeschema standalone
desktop-file-validate $KICAD_LAUNCHER_DIR/org.kicad.eeschema.desktop
[ $? -ne 0 ] && LAUNCHERS_VALID=0

# GerbView
desktop-file-validate $KICAD_LAUNCHER_DIR/org.kicad.gerbview.desktop
[ $? -ne 0 ] && LAUNCHERS_VALID=0

# Main KiCad application
desktop-file-validate $KICAD_LAUNCHER_DIR/org.kicad.kicad.desktop
[ $? -ne 0 ] && LAUNCHERS_VALID=0

# PCB calculator
desktop-file-validate $KICAD_LAUNCHER_DIR/org.kicad.pcbcalculator.desktop
[ $? -ne 0 ] && LAUNCHERS_VALID=0

# Pcbnew standalond
desktop-file-validate $KICAD_LAUNCHER_DIR/org.kicad.pcbnew.desktop
[ $? -ne 0 ] && LAUNCHERS_VALID=0

if [ $LAUNCHERS_VALID -eq 1 ]
then
    echo "All launcher files valid"
else
    echo "Errors with launcher files"
fi


###################################################################
# Install the desktop and MIME-type files and update the databases
###################################################################

GERBER_MIME_INSTALLED=0
KICAD_MIME_INSTALLED=0

echo ""
echo "Installing MIME type files for testing"

mkdir -p ~/.local/share/mime

xdg-mime install --mode user $KICAD_MIME_DIR/kicad-gerbers.xml
[ $? -eq 0 ] && GERBER_MIME_INSTALLED=1

xdg-icon-resource install --mode user --context mimetypes --size 128 $KICAD_ICON_DIR/128x128/mimetypes/application-x-kicad-pcb.png
xdg-icon-resource install --mode user --context mimetypes --size 128 $KICAD_ICON_DIR/128x128/mimetypes/application-x-kicad-project.png
xdg-icon-resource install --mode user --context mimetypes --size 128 $KICAD_ICON_DIR/128x128/mimetypes/application-x-kicad-schematic.png

xdg-mime install --mode user $KICAD_MIME_DIR/kicad-kicad.xml
[ $? -eq 0 ] && KICAD_MIME_INSTALLED=1

# Install the desktop files
echo ""
echo "Installing desktop files for testing"

# Ensure the directory exists (it might not in the CI image)
mkdir -p ~/.local/share/applications

# For some reason, desktop-file-install doesn't seem to actually install it properly, so just copy them
cp $KICAD_LAUNCHER_DIR/org.kicad.bitmap2component.desktop ~/.local/share/applications/
cp $KICAD_LAUNCHER_DIR/org.kicad.eeschema.desktop ~/.local/share/applications/
cp $KICAD_LAUNCHER_DIR/org.kicad.gerbview.desktop ~/.local/share/applications/
cp $KICAD_LAUNCHER_DIR/org.kicad.kicad.desktop ~/.local/share/applications/
cp $KICAD_LAUNCHER_DIR/org.kicad.pcbcalculator.desktop ~/.local/share/applications/
cp $KICAD_LAUNCHER_DIR/org.kicad.pcbnew.desktop ~/.local/share/applications/

# Force a database update to get the desktop files and MIME types associated
update-desktop-database ~/.local/share/applications/
update-mime-database ~/.local/share/mime/


###################################################################
# Validate the MIME types
###################################################################
KICAD_MIME_VALID=1
GERBER_MIME_VALID=1

# Fake a Gnome desktop environment to force xdg-mime to query using gio and ensure the MIME types are installed
# for a desktop environment. Otherwise it fallsback to using the "file --mime-type" command, which has prebuilt
# magic files from https://github.com/file/file/blob/master/magic/Magdir/kicad and isn't affected by the MIME
# type files we just installed.
export DE=gnome

# Test install the gerber MIME file to test with it
echo ""

if [ $GERBER_MIME_INSTALLED -eq 1 ]
then
    echo "Validating Gerber MIME-type"

    # Test the drl extension default
    OUT_STR=$(xdg-mime query default application/x-excellon 2>&1)
    printf "    Testing drill file default application: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "org.kicad.gerbview.desktop" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the gerber extension default
    OUT_STR=$(xdg-mime query default application/x-gerber 2>&1)
    printf "    Testing gerber file default application: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "org.kicad.gerbview.desktop" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the drl extension
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/drillFiles/drillFile.drl 2>&1)
    printf "    Testing drill file with extension drl: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-excellon" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the parsing of the header for M48
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/drillFiles/drillFileNoExt 2>&1)
    printf "    Testing drill file header M48 check: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-excellon" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the official gbr extension
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerber.gbr 2>&1)
    printf "    Testing gerber file with gbr extension: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the parsing of the header for a comment (G04)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerberCommentNoExt 2>&1)
    printf "    Testing gerber file header G04 check: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the parsing of the header for %FSLA
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerberFSLANoExt 2>&1)
    printf "    Testing gerber file header %%FLSA check: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the parsing of the header for %MO
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerberMONoExt 2>&1)
    printf "    Testing gerber file header %%MO check: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the parsing of the header for %TF.
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerberTFNoExt 2>&1)
    printf "    Testing gerber file header %%TF. check: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test the parsing of the header for G75*.
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerberG75NoExt 2>&1)
    printf "    Testing gerber file header G75* check: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    # Test parsing another popular gerber extension (will require a lookup)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/gerbers/gerberFiles/gerber.gts 2>&1)
    printf "    Testing gerber file with gts extension (not glob'd): %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-gerber" ]] && echo "        ERROR" && GERBER_MIME_VALID=0

    if [ $GERBER_MIME_VALID -eq 1 ]
    then
        echo "All Gerber MIME-type files valid"
    else
        echo "Errors with Gerber MIME-type files"
    fi
else
    echo "Gerber MIME type files not installed, skipping validation"
    GERBER_MIME_VALID=0
fi


# Test install the KiCad MIME file to test with it
echo ""

if [ $KICAD_MIME_INSTALLED -eq 1 ]
then
    echo "Validating KiCad MIME-type"

    # Test the KiCad project file extension default
    OUT_STR=$(xdg-mime query default application/x-kicad-project 2>&1)
    printf "    Testing KiCad project file default application: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "org.kicad.kicad.desktop" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the KiCad schematic file extension default
    OUT_STR=$(xdg-mime query default application/x-kicad-schematic 2>&1)
    printf "    Testing KiCad schematic file default application: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "org.kicad.eeschema.desktop" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the KiCad project file extension default
    OUT_STR=$(xdg-mime query default application/x-kicad-pcb 2>&1)
    printf "    Testing KiCad board file default application: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "org.kicad.pcbnew.desktop" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the old pcbnew board file extension (brd) - (will cause lookup since not glob'd)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/boardFiles/brd.brd 2>&1)
    printf "    Testing old Pcbnew board file with extension brd: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-pcb" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the old pcbnew board file with header check
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/boardFiles/brdNoExt 2>&1)
    printf "    Testing old Pcbnew board file header: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-pcb" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the new pcbnew board file extension (kicad_pcb)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/boardFiles/kicadpcb.kicad_pcb 2>&1)
    printf "    Testing new Pcbnew board file with extension kicad_pcb: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-pcb" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the new pcbnew board file with header check
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/boardFiles/kicadpcbNoExt 2>&1)
    printf "    Testing new Pcbnew board file header: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-pcb" ]] && echo "        ERROR" && KICAD_MIME_VALID=0


    # Test the old eeschema schematic file extension (sch)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/schematicFiles/sch.sch 2>&1)
    printf "    Testing old Eeschema schematic file with extension sch: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-schematic" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the brd extension
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/schematicFiles/schNoExt 2>&1)
    printf "    Testing old Eeschema schematic file header: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-schematic" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the new Eeschema schematic file extension (kicad_sch)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/schematicFiles/kicadsch.kicad_sch 2>&1)
    printf "    Testing new Eeschema schematic file with extension kicad_sch: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-schematic" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the new Eeschema board file with header check
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/schematicFiles/kicadschNoExt 2>&1)
    printf "    Testing new Eeschema schematic file header: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-schematic" ]] && echo "        ERROR" && KICAD_MIME_VALID=0


    # Test the old project file extension (pro)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/projectFiles/pro.pro 2>&1)
    printf "    Testing old KiCad project file with extension pro: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-project" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    # Test the new project file extension (kicad_pro)
    OUT_STR=$(xdg-mime query filetype $KICAD_MIME_TEST_FILES/kicad/projectFiles/kicadpro.kicad_pro 2>&1)
    printf "    Testing new KiCad project file with extension kicad_pro: %s\n" "$OUT_STR"
    [[ "$OUT_STR" != "application/x-kicad-project" ]] && echo "        ERROR" && KICAD_MIME_VALID=0

    if [ $KICAD_MIME_VALID -eq 1 ]
    then
        echo "All KiCad MIME-type files valid"
    else
        echo "Errors with KiCad MIME-type files"
    fi
else
    echo "KiCad MIME type files not installed, skipping validation"
    KICAD_MIME_VALID=0
fi


###################################################################
# Process return codes to flag errors for CI
###################################################################
[ $METAINFO_VALID -ne 0 ] && exit 1
[ $LAUNCHERS_VALID -ne 0 ] && exit 1
[ $KICAD_MIME_VALID -ne 0 ] && exit 1
[ $GERBER_MIME_VALID -ne 0 ] && exit 1

exit 0
