#!/bin/sh
cp libass/.libs/libass.1.0.0.dylib ./libass-osx.so
install_name_tool -id libass.1.dylib libass-osx.so
install_name_tool -change /opt/local/lib/libiconv.2.dylib /usr/lib/libiconv.dylib libass-osx.so
install_name_tool -change /opt/local/lib/libfontconfig.1.dylib @executable_path/../Frameworks/libfontconfig.1.3.0.dylib libass-osx.so
install_name_tool -change /opt/local/lib/libz.1.dylib /usr/lib/libz.1.dylib libass-osx.so
install_name_tool -change /opt/local/lib/libexpat.1.dylib /usr/lib/libexpat.1.dylib libass-osx.so
install_name_tool -change /opt/local/lib/libfreetype.6.dylib @executable_path/../Frameworks/libfreetype.6.dylib libass-osx.so