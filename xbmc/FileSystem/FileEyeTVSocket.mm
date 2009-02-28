/*
 *  FileEyeTVSocket.mm
 *  Plex
 *
 *  Created by Ryan Walklin on 2/28/09.
 *  Copyright 2009 Ryan Walklin. All rights reserved.
 *  
 *  Based on VLC's EyeTV integration module.
 *
 */


#include "FileEyeTVSocket.h"
#import <Cocoa/Cocoa.h>
#import "log.h"
#import <sys/un.h>


namespace XFILE
{
	
CFileEyeTVSocket::CFileEyeTVSocket()
{

}

CFileEyeTVSocket::~CFileEyeTVSocket()
{
	Close();
	
	initialised = false;
}
	

//*********************************************************************************************
bool CFileEyeTVSocket::Open(const CURL& url, bool bBinary)
{
	
    struct sockaddr_un publicAddr, peerAddr;
    int publicSock;
	
	initialised = false;
	
    
    
    //int val = var_CreateGetInteger( p_access, "eyetv-channel" );
	
    //msg_Dbg( p_access, "coming up" );
	
    //selectChannel( p_this, val );
	
    /* socket */
    memset(&publicAddr, 0, sizeof(publicAddr));
    publicAddr.sun_family = AF_UNIX;
    strncpy(publicAddr.sun_path, "/tmp/.plextvserver-bridge", sizeof(publicAddr.sun_path)-1);
	
    /* remove previous public path if it wasn't cleanly removed */
    if( (0 != unlink(publicAddr.sun_path)) && (ENOENT != errno) )
    {
		CLog::Log(LOGERROR, "local socket path is not usable (errno=%d)", errno);
        return false;
    }
	
    publicSock = socket(AF_UNIX, SOCK_STREAM, 0);
    if( publicSock == -1 )
    {
		CLog::Log(LOGERROR, "create local socket failed (errno=%d)", errno);
        return false;
    }
	
    if( bind(publicSock, (struct sockaddr *)&publicAddr, sizeof(struct sockaddr_un)) == -1 )
    {
		CLog::Log(LOGERROR, "bind local socket failed (errno=%d)", errno);
        close(publicSock);
        return false;
    }
	
    /* we are not expecting more than one connection */
    if( listen(publicSock, 1) == -1 )
    {
		CLog::Log(LOGERROR, "cannot accept connection (errno=%d)", errno);
        close( publicSock );
        return false;
    }
    else
    {
        socklen_t peerSockLen = sizeof(struct sockaddr_un);
        int peerSock;
		
        /* tell the EyeTV plugin to open start sending */
        CFNotificationCenterPostNotification( CFNotificationCenterGetDistributedCenter (),
											 CFSTR("PlexTVServerStartDataSending"),
											 CFSTR("PlexTVServer"),
											 /*userInfo*/ NULL,
											 TRUE );
		
		CLog::Log(LOGINFO, "EyeTV client socket announced");
		
        peerSock = accept(publicSock, (struct sockaddr *)&peerAddr, &peerSockLen);
        if( peerSock == -1 )
        {
			CLog::Log(LOGERROR, "cannot wait for connection (errno=%d)", errno);
            close( publicSock );
            return false;
        }
		
		CLog::Log(LOGINFO, "EyeTV server connected" );
		initialised = true;
		
        eyetvSock = peerSock;
		
        /* remove public access */
        close(publicSock);
        unlink(publicAddr.sun_path);
    }
	
	return initialised;
}



unsigned int CFileEyeTVSocket::Read(void *lpBuf, __int64 uiBufSize)
{
	if (initialised)
	{
		uint64_t len;
		
		/* Read data */
		len = read(eyetvSock, lpBuf, uiBufSize);
		
		if (len < uiBufSize)
		{
			CLog::Log(LOGERROR, "short read from EyeTV server socket (%i KB of %i KB requested)", len / 1024, uiBufSize / 1024);
		}
    	
		return len;
	}
	return 0;
}

void CFileEyeTVSocket::Close()
{
	
	CLog::Log(LOGINFO, "closing EyeTV socket");
	
    /* tell the EyeTV plugin to close its msg port and stop sending */
    CFNotificationCenterPostNotification( CFNotificationCenterGetDistributedCenter (),
										 CFSTR("PlexTVServerStopDataSending"),
										 CFSTR("PlexTVServer"),
										 /*userInfo*/ NULL,
										 TRUE );
	
	CLog::Log(LOGINFO, "EyeTV server notified of shutdown" );
	
	close(eyetvSock);
	
	CLog::Log(LOGINFO, "EyeTV socket closed and freed" );
}
	
}
