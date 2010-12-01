//
//  XBMCMain.m
//  XBMC
//
//  Created by Elan Feingold on 2/27/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>

#define __DEBUGGING__
#include "stdafx.h"
#include "Application.h"

#import "XBMCMain.h"
#import "AppleRemote.h"

#include "Log.h"
#include "CocoaToCppThunk.h"
#include "CocoaUtilsPlus.h"

#define PLEX_SERVICE_PORT 32401
#define PLEX_MEDIA_SERVER_SERVICE_TYPE @"_plexmediasvr._tcp"

using namespace std;

struct BonjourServer
{
  BonjourServer(NSNetService* service)
    : resolved(false) 
  {
    this->name = [[service name] UTF8String];
    this->service = [service retain];
  }
  
  ~BonjourServer()
  {
    CLog::Log(LOGNOTICE, "Bonjour: Whacking - %s", [[service name] UTF8String]);
    [service stopMonitoring];
    [service stop];
    [service release];
  }
  
  string        name;
  string        host;
  string        url;
  bool          resolved;
  NSNetService* service;
};

typedef boost::shared_ptr<BonjourServer> BonjourServerPtr;
map<string, BonjourServerPtr> BonjourServerMap;
typedef pair<string, BonjourServerPtr> string_server_pair;

@implementation XBMCMain

static XBMCMain *_o_sharedMainInstance = nil;

+ (XBMCMain *)sharedInstance
{
  return _o_sharedMainInstance ? _o_sharedMainInstance : [[self alloc] init];
}

- (void)resetBonjourSearch
{
  if (g_application.IsPlayingVideo() == false)
  {
    CLog::Log(LOGNOTICE, "Bonjour: Restarting search.");
    BOOST_FOREACH(string_server_pair pair, BonjourServerMap)
      if (pair.second)
        [pair.second->service resolveWithTimeout:30];
  }
  else
  {
    CLog::Log(LOGNOTICE, "Bonjour: Skipping the search because video is playing.");
  }
  
  [self performSelector:@selector(resetBonjourSearch) withObject:nil afterDelay:60];
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
  
  o_plexMediaServerBrowser = [[NSNetServiceBrowser alloc] init];
  [o_plexMediaServerBrowser setDelegate:self];

  // Let the system stablize a bit before we start searching for media servers
  [self performSelector:@selector(searchForPlexMediaServers) withObject:nil afterDelay:0.1];
	
  // Start the client's Bonjour service
  o_plexNetService = [[NSNetService alloc] initWithDomain:@"local." type:@"_plexclient._tcp" name:@"" port:32401];
  [o_plexNetService setTXTRecordData:
   [NSNetService dataFromTXTRecordDictionary:
    [NSDictionary dictionaryWithObject:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]
                                forKey:@"Version"]]];
  [o_plexNetService publish];
  
  // broadcast launch notification
	CFNotificationCenterPostNotification(CFNotificationCenterGetDistributedCenter(), CFSTR("PlexGUIInit"), CFSTR("PlexTVServer"), NULL, TRUE);
  
  [self performSelector:@selector(resetBonjourSearch) withObject:nil afterDelay:60];

  return _o_sharedMainInstance;
}

- (void)dealloc 
{
  [o_plexNetService release];
  [o_remote release], o_remote = nil;
  [o_plexMediaServerBrowser release], o_plexMediaServerBrowser = nil;
  [super dealloc];
}

- (void)searchForPlexMediaServers
{
  [o_plexMediaServerBrowser searchForServicesOfType:PLEX_MEDIA_SERVER_SERVICE_TYPE inDomain:@"local."];
}

- (void)stopSearchingForPlexMediaServers
{
  [o_plexMediaServerBrowser stop];
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
-(void)netServiceBrowser:(NSNetServiceBrowser *)aBrowser didFindService:(NSNetService *)service moreComing:(BOOL)more 
{
  CLog::Log(LOGNOTICE, "Bonjour: Did find service - %s", [[service name] UTF8String]);
  
  // Add it to the map.
  string name = [[service name] UTF8String];
  if (BonjourServerMap.find(name) == BonjourServerMap.end())
  {
    BonjourServerPtr server = BonjourServerPtr(new BonjourServer(service));
    BonjourServerMap[server->name] = server;
  }
   
  // Set it resolving.
  CLog::Log(LOGNOTICE, "Bonjour: Resolving service - %s", [[service name] UTF8String]);
  service.delegate = self;
  [service resolveWithTimeout:30];
}

-(void)netServiceBrowser:(NSNetServiceBrowser *)aBrowser didRemoveService:(NSNetService *)service moreComing:(BOOL)more 
{
  CLog::Log(LOGNOTICE, "Bonjour: Did remove service - %s", [[service name] UTF8String]);
  
  // Remove it from the map, and take out the source.
  string name = [[service name] UTF8String];
  BonjourServerPtr server = BonjourServerMap[name];
  
  if (server)
  {
    Cocoa_RemoveRemotePlexSources(server->host.c_str());
    BonjourServerMap.erase(name);
  }
}

/* NSNetService Delegate Overrides */
-(void)netServiceDidResolveAddress:(NSNetService *)service 
{
  string name = [[service name] UTF8String];
  BonjourServerPtr server = BonjourServerMap[name];
  
  if (!server)
    return;
  
  server->host = [[service hostName] UTF8String];
  
  CLog::Log(LOGNOTICE, "Bonjour: Did resolve service - %s (type: %s) - with %d addresses, resolved=%d and url=%s", 
      [[service name] UTF8String], [[service type] UTF8String], [[service addresses] count], server->resolved, server->url.c_str());
  
  if ([[service addresses] count] == 0 || !server)
  {
    CLog::Log(LOGNOTICE, "Bonjour: Did not find any addresses for host, or server was not known, ignoring.");
    return;
  }

  string url;

  // Use the first IPv4 address.
  for (NSData* address in [service addresses])
  {
    struct sockaddr_in* address_sin = (struct sockaddr_in *)[address bytes];
    in_port_t port = 0;
    const char* strAddress = 0;
    char buffer[1024];
    
    if (AF_INET == address_sin->sin_family)
    {
      strAddress = inet_ntop(AF_INET, &(address_sin->sin_addr), buffer, sizeof(buffer));
      port = ntohs(address_sin->sin_port);
    
      // Compute the URL.
      if (Cocoa_IsHostLocal([[service hostName] UTF8String]))
        url = "http://127.0.0.1:" + boost::lexical_cast<string>(port);
      else
        url = "http://" + string(strAddress) + ":" + boost::lexical_cast<string>(port);
      
      // We're done now.
      break;
    }
  }
  
  bool urlChanged = false;
  if (url != server->url)
  {
    server->url = url;
    urlChanged = true;
    CLog::Log(LOGNOTICE, "Bonjour: Computed a URL for %s of %s", name.c_str(), url.c_str());
  }
                                                                     
  // See if we need to notify.
  if (server->resolved == false || urlChanged == true)
  {
    // Notify and keep an eye on the TXT record.
    server->resolved = true;
    Cocoa_AutodetectRemotePlexSources([[service hostName] UTF8String], [[service name] UTF8String], server->url.c_str());
    [service startMonitoring];  
  }
}

-(void)netService:(NSNetService *)service didNotResolve:(NSDictionary *)errorDict 
{ 
  CLog::Log(LOGWARNING, "Bonjour: Did NOT resolve service - %s.", [[service name] UTF8String]);
}

- (void)netService:(NSNetService *)service didUpdateTXTRecordData:(NSData *)data
{
  string name = [[service name] UTF8String];
  BonjourServerPtr server = BonjourServerMap[name];

  if (server && server->resolved)
  {
    CLog::Log(LOGNOTICE, "Bonjour: Updated TXT record for - %s", [[service name] UTF8String]);
    Cocoa_AutodetectRemotePlexSources([[service hostName] UTF8String], [[service name] UTF8String], server->url.c_str());
  }
}

- (void)resolveService:(NSNetService* )service
{
  [service resolveWithTimeout:30];
}

@end
