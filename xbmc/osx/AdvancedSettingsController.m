//
//  AdvancedSettingsController.m
//  Plex
//
//  Created by James Clarke on 05/03/2009.
//  Copyright 2009 Plex. All rights reserved.
//

#import "AdvancedSettingsController.h"

#define ADVSETTINGS_FILE @"~/Library/Application Support/Plex/userdata/advancedsettings.xml"

@implementation AdvancedSettingsController

id g_advancedSettingsController;

void setEnabledFromXML(NSXMLDocument* xmlDoc, NSButton* control, NSString* xpath)
{
  NSArray* nodes;
  nodes = [xmlDoc nodesForXPath:xpath error:nil];
  if ([nodes count] > 0)
  {
    if ([[(NSXMLElement*)[nodes objectAtIndex:0] stringValue] isEqualToString:@"true"])
    {
      // NSLog(@"Enabling control for %@", xpath);
      [control setState:NSOnState];
    }
    else
    {
      // NSLog(@"Disabling control for %@", xpath);
      [control setState:NSOffState];
    }
  }
}

void enabledFromControl(NSXMLElement* xmlElement, NSButton* control, NSString* nodeName)
{
  NSString* strValue;
  if ([control state] == NSOnState)
    strValue = @"true";
  else
    strValue = @"false";
  [xmlElement addChild:[NSXMLElement elementWithName:nodeName stringValue:strValue]];
}

void setStringFromXML(NSXMLDocument* xmlDoc, NSTextField* control, NSString* xpath)
{
  NSArray* nodes;
  nodes = [xmlDoc nodesForXPath:xpath error:nil];
  if ([nodes count] > 0)
  {
    NSString* strValue = [(NSXMLElement*)[nodes objectAtIndex:0] stringValue];
    //NSLog(@"String value for control %@ is %@", xpath, strValue);
    [control setStringValue:strValue];
  }
  else
  {
    //NSLog(@"No string value set for control %@", xpath);
    [control setStringValue:@""];
  }
}

void stringFromControl(NSXMLElement* xmlElement, NSTextField* control, NSString* nodeName)
{
  [xmlElement addChild:[NSXMLElement elementWithName:nodeName stringValue:[control stringValue]]];
}

+ (AdvancedSettingsController*)sharedInstance
{
  return (AdvancedSettingsController*)g_advancedSettingsController;
}

- (void)awakeFromNib
{
  g_advancedSettingsController = (id)self;
  m_settingChanged = NO;
  m_shouldClose = NO;
  [self loadSettings];
}

- (void)windowWillClose:(NSNotification *)notification
{
  [NSApp stopModal];
}

- (BOOL)windowShouldClose:(id)theWindow
{
  if (m_settingChanged == YES)
  {
    NSBeginAlertSheet(@"There are unsaved changes to your settings.",
                      @"Save",
                      @"Cancel",
                      @"Discard",
                      window,
                      self,
                      @selector(sheetDidEndShouldSave:returnCode:contextInfo:), 
                      nil,
                      nil,
                      @"Do you want to save these changes?");
    return NO;
  }
  
  if (m_shouldClose == YES || m_settingChanged == NO)
  {
    m_shouldClose = NO;
    return YES;
  }
  return NO;
}

- (void)sheetDidEndShouldSave: (NSWindow *)sheet
                   returnCode: (int)returnCode
                  contextInfo: (void *)contextInfo
{
  if (returnCode == NSAlertDefaultReturn)
  {
    [self saveSettings];
    m_shouldClose = YES;
    [window close];
    [self relaunchPlex];
  }
  else if (returnCode == NSAlertOtherReturn)
  {
    [self loadSettings];
    m_settingChanged = NO;
    m_shouldClose = YES;
    [window close];    
  }
}

-(IBAction)showWindow:(id)sender
{
  [[NSApplication sharedApplication] runModalForWindow:window];
}

-(BOOL)windowIsVisible
{
  return [window isVisible];
}

-(IBAction)settingChanged:(id)sender
{
  NSLog(@"Setting changed");
  m_settingChanged = YES;
}

-(IBAction)restoreDefaults:(id)sender
{
  NSBeginAlertSheet(@"Are you sure you want to restore the default settings?",
                    @"Restore",
                    nil,
                    @"Cancel",
                    window,
                    self,
                    @selector(sheetDidEndShouldRestoreDefaults:returnCode:contextInfo:), 
                    nil,
                    nil,
                    @"Any changes you've made to the advanced settings will be lost. This will not affect the rest of your Plex settings.");
}

- (void)sheetDidEndShouldRestoreDefaults: (NSWindow *)sheet
                              returnCode: (int)returnCode
                             contextInfo: (void *)contextInfo
{
  if (returnCode == NSAlertDefaultReturn)
  {
    [[NSFileManager defaultManager] removeFileAtPath:[ADVSETTINGS_FILE stringByExpandingTildeInPath] handler:nil];
    m_settingChanged = NO;
    m_shouldClose = YES;
    [window close];
    [self relaunchPlex];
  }
}

-(void)loadSettings
{
  // Load the advancedsettings.xml file
  NSXMLDocument* xmlDoc;
  NSError* err = nil;
  NSURL* furl = [NSURL fileURLWithPath:[ADVSETTINGS_FILE stringByExpandingTildeInPath]];
  if (!furl) {
    NSLog(@"Can't create an URL from file.");
    return;
  }
  xmlDoc = [[NSXMLDocument alloc] initWithContentsOfURL:furl
                                                options:(NSXMLNodePreserveWhitespace|NSXMLNodePreserveCDATA)
                                                  error:&err];
  if (xmlDoc == nil) {
    xmlDoc = [[NSXMLDocument alloc] initWithContentsOfURL:furl
                                                  options:NSXMLDocumentTidyXML
                                                    error:&err];
  }
  if (xmlDoc == nil)  {
    if (err) {
      NSLog(@"%@", err);
    }
    return;
  }
  
  if (err) {
    NSLog(@"%@", err);
    return;
  }
  
  // Query the XML document and set GUI control states
  /*
  setEnabledFromXML(xmlDoc, debugLogging, @"./advancedsettings/system/debuglogging");
  setEnabledFromXML(xmlDoc, opticalMedia, @"./advancedsettings/enableopticalmedia");
  setEnabledFromXML(xmlDoc, fakeFullscreen, @"./advancedsettings/fakefullscreen");
  setEnabledFromXML(xmlDoc, cleanOnUpdate, @"./advancedsettings/videolibrary/cleanonupdate");
  setEnabledFromXML(xmlDoc, fileDeletion, @"./advancedsettings/filelists/allowfiledeletion");
  setStringFromXML(xmlDoc, httpProxyUsername, @"./advancedsettings/network/httpproxyusername");
  setStringFromXML(xmlDoc, httpProxyPassword, @"./advancedsettings/network/httpproxypassword");
  */
  [xmlDoc release];
}

-(void)saveSettings
{
  /*
  NSXMLElement *root = (NSXMLElement *)[NSXMLNode elementWithName:@"advancedsettings"];
  NSXMLDocument *xmlDoc = [[NSXMLDocument alloc] initWithRootElement:root];
  
  NSXMLElement* system = [NSXMLElement elementWithName:@"system"];
  NSXMLElement* network = [NSXMLElement elementWithName:@"network"];
  NSXMLElement* filelists = [NSXMLElement elementWithName:@"filelists"];
  NSXMLElement* videolibrary = [NSXMLElement elementWithName:@"videolibrary"];
  
  enabledFromControl(system, debugLogging, @"debuglogging");
  enabledFromControl(root, opticalMedia, @"enableopticalmedia");
  enabledFromControl(root, fakeFullscreen, @"fakefullscreen");
  enabledFromControl(videolibrary, cleanOnUpdate, @"cleanonupdate");
  enabledFromControl(filelists, fileDeletion, @"allowfiledeletion");
  stringFromControl(network, httpProxyUsername, @"httpproxyusername");
  stringFromControl(network, httpProxyPassword, @"httpproxypassword");
  
  [root addChild:system];
  [root addChild:network];
  [root addChild:filelists];
  [root addChild:videolibrary];
  
  //NSLog([xmlDoc XMLStringWithOptions:NSXMLNodePrettyPrint]);
  NSData* xmlData = [xmlDoc XMLDataWithOptions:NSXMLNodePrettyPrint];
  if (![xmlData writeToFile:[ADVSETTINGS_FILE stringByExpandingTildeInPath] atomically:YES]) {
    NSBeep();
    NSLog(@"Could not write document out...");
  }
  
  [xmlDoc release];
   */
  m_settingChanged = NO;
}

-(void)relaunchPlex
{
  if (NSRunAlertPanel(@"Plex needs to be relaunched to apply your changes.", @"Do you want to relaunch Plex now?", @"Relaunch now", @"Later", nil) == NSAlertDefaultReturn)
  {
    NSString* pathToRelaunch = [[NSBundle mainBundle] bundlePath];
    NSString* relaunchPath = [[NSBundle mainBundle] pathForResource:@"relaunch" ofType:nil];
    [NSTask launchedTaskWithLaunchPath:relaunchPath arguments:[NSArray arrayWithObjects:pathToRelaunch, [NSString stringWithFormat:@"%d", [[NSProcessInfo processInfo] processIdentifier]], nil]];
    
    [NSApp terminate:self];
  }
}

@end
