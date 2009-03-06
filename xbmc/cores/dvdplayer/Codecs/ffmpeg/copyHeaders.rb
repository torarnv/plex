#!/usr/bin/env ruby

files = %w[avcodec.h avio.h common.h intfloat_readwrite.h mathematics.h postprocess.h rgb2rgb.h rtsp.h swscale.h avformat.h avutil.h integer.h log.h mem.h rational.h rtp.h rtspcodes.h]

files.each do |file|
  find_cmd = IO.popen("find . -name #{file} 2>&1").readline().gsub("\n", "")
  src = find_cmd
  dst = "/Users/elan/Code/C++/Eclipse/Plex/xbmc/lib/libffmpeg-OSX#{src[1,find_cmd.length]}"
  
  system("cp #{src} #{dst}")
end