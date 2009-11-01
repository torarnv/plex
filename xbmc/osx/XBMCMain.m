//
//  XBMCMain.m
//  XBMC
//
//  Created by Elan Feingold on 2/27/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "XBMCMain.h"
#import "AppleRemote.h"
#include "CocoaToCppThunk.h"

#define PLEX_SERVICE_PORT 32401
#define PLEX_SERVICE_TYPE @"_plexmediasvr._tcp"

@implementation XBMCMain

static XBMCMain *_o_sharedMainInstance = nil;

+ (XBMCMain *)sharedInstance
{
  return _o_sharedMainInstance ? _o_sharedMainInstance : [[self alloc] init];
}

- (id)init
{
  if( _o_sharedMainInstance)
      [self dealloc];
  else
      _o_sharedMainInstance = [super init];

  o_remote = [[AppleRemote alloc] init];
  [o_remote setClickCountEnabledButtons: kRemoteButtonPlay];
  [o_remote setDelegate: _o_sharedMainInstance];
  
  o_plexMediaServers = [[NSMutableArray alloc] initWithCapacity:10];
  o_plexMediaServerHosts = [[NSMutableDictionary alloc] initWithCapacity:10];
  o_plexMediaServerBrowser = [[NSNetServiceBrowser alloc] init];
  [o_plexMediaServerBrowser setDelegate:self];

  // Let the system stablize a bit before we start searching for media servers
  [self performSelector:@selector(searchForPlexMediaServers) withObject:nil afterDelay:0.1];
	
  // broadcast launch notification
	CFNotificationCenterPostNotification( CFNotificationCenterGetDistributedCenter (),
										 CFSTR("PlexGUIInit"), 
										 CFSTR("PlexTVServer"), 
										 /*userInfo*/ NULL, 
										 TRUE );

  // Start listening in exclusive mode.
  //[o_remote startListening: self];
  
  return _o_sharedMainInstance;
}

- (void)dealloc {
  [o_remote release], o_remote = nil;
  [o_plexMediaServerBrowser release], o_plexMediaServers = nil;
  [o_plexMediaServers release], o_plexMediaServers = nil;
  [o_plexMediaServerHosts release], o_plexMediaServerHosts = nil;
  [super dealloc];
}

- (void)searchForPlexMediaServers;
{
  [o_plexMediaServerBrowser searchForServicesOfType:PLEX_SERVICE_TYPE inDomain:@""];
}

- (NSArray *)plexMediaServers
{
  return o_plexMediaServers;
}

/* Apple Remote callback */
- (void) appleRemoteButton: (AppleRemoteEventIdentifier)buttonIdentifier
               pressedDown: (BOOL) pressedDown
                clickCount: (unsigned int) count
{
    // Pass event to thunk.
    Cocoa_OnAppleRemoteKey(pApplication, buttonIdentifier, pressedDown, count);
}

- (void)setApplication: (void*) application;
{
  pApplication = application;
}

/* NSNetServiceBrowser Delegate Overrides */
-(void)netServiceBrowser:(NSNetServiceBrowser *)aBrowser didFindService:(NSNetService *)service moreComing:(BOOL)more {
  NSLog(@"Did Find Service: %@", [service name]);
  [o_plexMediaServers addObject:service];
  service.delegate = self;
  [service resolveWithTimeout:30];
}

-(void)netServiceBrowser:(NSNetServiceBrowser *)aBrowser didRemoveService:(NSNetService *)service moreComing:(BOOL)more {
  NSLog(@"Did Remove Service: %@", [service name]);
  [o_plexMediaServers removeObject:service];
  if ([o_plexMediaServerHosts objectForKey:[service name]])
  {
    Cocoa_RemoveRemotePlexSources([[o_plexMediaServerHosts objectForKey:[service name]] UTF8String]);
    [o_plexMediaServerHosts removeObjectForKey:[service name]];
  }
}

/* NSNetService Delegate Overrides */
-(void)netServiceDidResolveAddress:(NSNetService *)service {
  NSLog(@"Service Did Resolve: %@", [service name]);
  [o_plexMediaServerHosts setObject:[service hostName] forKey:[service name]];
  Cocoa_AutodetectRemotePlexSources([[service hostName] UTF8String], [[service name] UTF8String]);
}

-(void)netService:(NSNetService *)service didNotResolve:(NSDictionary *)errorDict {
  NSLog(@"Service Did Not Resolve: %@ (%@)", [service name], errorDict);
}

@end
