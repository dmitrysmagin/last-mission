#!/bin/sh

OPK_NAME=lastmission.opk

echo ${OPK_NAME}

# create default.gcw0.desktop
cat > default.gcw0.desktop <<EOF
[Desktop Entry]
Name=Last Mission
Comment=Maze shooter
Exec=last-mission.dge
Terminal=false
Type=Application
StartupNotify=true
Icon=graphics/last-mission
Categories=games;
EOF

# create opk
FLIST="data graphics sound"
FLIST="${FLIST} default.gcw0.desktop"
FLIST="${FLIST} last-mission.dge"
FLIST="${FLIST} *.txt"

rm -f ${OPK_NAME}
mksquashfs ${FLIST} ${OPK_NAME} -all-root -no-xattrs -noappend -no-exports

cat default.gcw0.desktop
rm -f default.gcw0.desktop
