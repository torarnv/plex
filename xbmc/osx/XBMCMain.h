//
//  XBMCMain.h
//  XBMC
//
//  Created by Elan Feingold on 2/27/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>
#import "AppleRemoteKeys.h"

@class AppleRemote;
@interface XBMCMain : NSObject 
{
  AppleRemote*        o_remote;
  void*               pApplication;
  NSNetService        *o_plexNetService;
  NSNetServiceBrowser *o_plexMediaServerBrowser;
}

+ (XBMCMain *)sharedInstance;
- (void)searchForPlexMediaServers;
- (void)stopSearchingForPlexMediaServers;
- (void)setApplication:(void*) application;
- (void)resolveService:(NSNetService* )service;

@end
