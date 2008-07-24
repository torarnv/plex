#!/opt/local/bin/python

import os, sys
from Foundation import NSMutableDictionary

version = os.popen4("/opt/local/bin/git rev-parse HEAD")[1].read().strip()
version = "%s (%s)" % (sys.argv[1], version[0:7])
info = os.environ['BUILT_PRODUCTS_DIR']+"/"+os.environ['WRAPPER_NAME']+"/Contents/Info.plist"

plist = NSMutableDictionary.dictionaryWithContentsOfFile_(info)
plist['CFBundleVersion'] = "Version " + version
plist['CFBundleShortVersionString'] = sys.argv[1]
plist.writeToFile_atomically_(info, 1)
