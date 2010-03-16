#!/bin/sh
PLEX=../Plex.app

cp ./build/Release/Plex.app/Contents/MacOS/Plex ${PLEX}/Contents/MacOS/
chmod 755 ${PLEX}/Contents/MacOS/Plex
cp ./build/Release/Plex.app/Contents/Info.plist ${PLEX}/Contents/
cp ./build/Release/Plex.app/Contents/_CodeSignature/CodeResources ${PLEX}/Contents/_CodeSignature
cp ./build/Release/Plex.app/Contents/Resources/* ${PLEX}/Contents/Resources
cp ./build/Release/Plex.app/Contents/Resources/Plex/PlexHelper ${PLEX}/Contents/Resources/Plex/
cp ./build/Release/Plex.app/Contents/MacOS/CrashReporter ${PLEX}/Contents/MacOS/
cp ./build/Release/frontrowlauncher ${PLEX}/Contents/MacOS/
cp ./build/Release/relaunch ${PLEX}/Contents/MacOS/

install_name_tool -change /opt/local/lib/libGLEW.1.5.1.dylib @executable_path/../Frameworks/libGLEW.1.5.1.dylib ${PLEX}/Contents/MacOS/Plex 
install_name_tool -change /opt/local/lib/libcdio.6.dylib @executable_path/../Frameworks/libcdio.6.dylib ${PLEX}/Contents/MacOS/Plex 
install_name_tool -change /opt/local/lib/libfreetype.6.dylib @executable_path/../Frameworks/libfreetype.6.dylib ${PLEX}/Contents/MacOS/Plex 
install_name_tool -change /opt/local/lib/libfribidi.0.dylib @executable_path/../Frameworks/libfribidi.0.dylib ${PLEX}/Contents/MacOS/Plex 
install_name_tool -change /opt/local/lib/liblzo.1.dylib @executable_path/../Frameworks/liblzo.1.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libmad.0.dylib @executable_path/../Frameworks/libmad.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libogg.0.dylib @executable_path/../Frameworks/libogg.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libjpeg.62.dylib @executable_path/../Frameworks/libjpeg.62.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libvorbis.0.dylib @executable_path/../Frameworks/libvorbis.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libfaad.0.dylib @executable_path/../Frameworks/libfaad.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libboost_regex-mt.dylib @executable_path/../Frameworks/libboost_regex-mt.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libboost_regex-mt-1_35.dylib @executable_path/../Frameworks/libboost_regex-mt.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libboost_thread-mt.dylib @executable_path/../Frameworks/libboost_thread-mt.dylib ${PLEX}/Contents/MacOS/Plex

install_name_tool -change /opt/local/lib/libpcre.0.dylib @executable_path/../Frameworks/libpcre.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libfontconfig.1.dylib @executable_path/../Frameworks/libfontconfig.1.3.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libiconv.2.dylib /usr/lib/libiconv.2.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libz.1.dylib /usr/lib/libz.1.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libbz2.1.0.dylib /usr/lib/libbz2.1.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libsqlite3.0.dylib /usr/lib/libsqlite3.0.dylib ${PLEX}/Contents/MacOS/Plex

install_name_tool -change /opt/local//lib/libSDL-1.2.0.dylib @executable_path/../Frameworks/libSDL-1.2.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libSDL_image-1.2.0.dylib @executable_path/../Frameworks/libSDL_image-1.2.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/libSDL_mixer-1.2.0.dylib @executable_path/../Frameworks/libSDL_mixer-1.2.0.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/mysql5/mysql/libmysqlclient.15.dylib @executable_path/../Frameworks/libmysqlclient.16.dylib ${PLEX}/Contents/MacOS/Plex
install_name_tool -change /opt/local/lib/samba3/libsmbclient.dylib @executable_path/../Frameworks/libsmbclient.dylib ${PLEX}/Contents/MacOS/Plex

rm ${PLEX}/Contents/Resources/Plex/system/profiles.xml
rm ${PLEX}/Contents/Resources/Plex/system/mediasources.xml

codesign -f -s "Plex" ${PLEX}
