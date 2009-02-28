/*
 *  FileEyeTVSocket.h
 *  Plex
 *
 *  Created by Ryan Walklin on 2/28/09.
 *  Copyright 2009 Ryan Walklin. All rights reserved.
 *
 */

#pragma once

#include "IFile.h"

extern "C"
{
}

namespace XFILE
{
	
	class CFileEyeTVSocket : public IFile
		{
		public:
			CFileEyeTVSocket();
			virtual ~CFileEyeTVSocket();
			virtual bool Open(const CURL& url, bool bBinary = true);
			virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
			virtual bool          Exists(const CURL& url)                           { return true; }
			virtual __int64	      Seek(__int64 iFilePosition, int iWhence)          { return -1; }
			virtual int	          Stat(const CURL& url, struct __stat64* buffer)    { errno = ENOENT; return -1; }
			virtual __int64       GetPosition()                                     { return 0; }
			virtual __int64       GetLength()                                       { return 0; }

			virtual void Close();
		protected:
		private:

			
		};
}
