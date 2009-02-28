/*
 *  FileEyeTVSocket.mm
 *  Plex
 *
 *  Created by Ryan Walklin on 2/28/09.
 *  Copyright 2009 Ryan Walklin. All rights reserved.
 *
 */

#include "FileEyeTVSocket.h"

namespace XFILE
{
	
CFileEyeTVSocket::CFileEyeTVSocket()
{

}

CFileEyeTVSocket::~CFileEyeTVSocket()
{
	Close();
}

//*********************************************************************************************
bool CFileEyeTVSocket::Open(const CURL& url, bool bBinary)
{
	
	return true;
}



unsigned int CFileEyeTVSocket::Read(void *lpBuf, __int64 uiBufSize)
{
	return 0;
}

void CFileEyeTVSocket::Close()
{

}
	
}
