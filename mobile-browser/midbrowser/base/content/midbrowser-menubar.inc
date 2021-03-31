# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla.org Code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

#       <menubar id="main-menubar" statusbar="statusbar-display"
#                style="border:0px;padding:0px;margin:0px;-moz-appearance:none">
        <menupopup class="hildonmenu">
            <menuitem command="cmd_newNavigatorTab" label="&tabCmd.label;" accesskey="&tabCmd.accesskey;"/>
            <menuseparator/>
  
            <menu id="view-menu" label="&viewMenu.label;" accesskey="&viewMenu.accesskey;">
              <menupopup id="menu_viewPopup">
                <menuitem id="toggle_taskbar" label="&taskbarCmd.label;" accesskey="&taskbarCmd.accesskey;" type="checkbox" command="cmd_toggleTaskbar"/>
                <menuseparator/>
#                <menu id="viewFullZoomMenu" label="&fullZoom.label;" accesskey="&fullZoom.accesskey;" observes="isImage">
#                  <menupopup>
                    <menuitem key="key_fullZoomEnlarge" label="&fullZoomEnlargeCmd.label;" accesskey="&fullZoomEnlargeCmd.accesskey;"
                              command="cmd_fullZoomEnlarge"/>
                    <menuitem key="key_fullZoomReduce"  label="&fullZoomReduceCmd.label;"  accesskey="&fullZoomReduceCmd.accesskey;"
                              command="cmd_fullZoomReduce"/>
#                    <menuseparator/>
                    <menuitem key="key_fullZoomReset" label="&fullZoomResetCmd.label;" accesskey="&fullZoomResetCmd.accesskey;"
                              command="cmd_fullZoomReset"/>
#                  </menupopup>
#                </menu>

                <menuitem accesskey="&fullScreenCmd.accesskey;"
                          label="&fullScreenCmd.label;" key="key_fullScreen"
                          id="fullScreenItem"
                          type="checkbox"
                          command="View:FullScreen"/>

              </menupopup>
            </menu>

                <menu id="charsetMenu"
                      label="&charsetMenu.label;"
                      accesskey="&charsetMenu.accesskey;"
                      datasources="rdf:charset-menu"
                      ref="NC:BrowserCharsetMenuRoot"
                      oncommand="MultiplexHandler(event)"
                      onpopupshowing="CreateMenu('browser');UpdateMenus(event)"
                      onpopupshown="CreateMenu('more-menu');"
                      observes="isImage">
                    <template>
                        <rule rdf:type="http://home.netscape.com/NC-rdf#BookmarkSeparator">
                            <menupopup>
                            <menuseparator uri="..." />
                            </menupopup>
                        </rule>
                        <rule>
                            <menupopup>
                            <menuitem type="radio" name="charsetGroup" checked="rdf:http://home.netscape.com/NC-rdf#Checked" uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                            </menupopup>
                        </rule>
                    </template>
      
                    <menupopup>
                    <menu label="&charsetMenuAutodet.label;" accesskey="&charsetMenuAutodet.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserAutodetMenuRoot">
                        <template>
                            <rule rdf:type="http://home.netscape.com/NC-rdf#CharsetDetector">
                                <menupopup>
                                <menuitem type="radio" name="detectorGroup" checked="rdf:http://home.netscape.com/NC-rdf#Checked" uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                </menupopup>
                            </rule>
                        </template>
                        <menupopup>
                        </menupopup>
                    </menu>
                    <menu label="&charsetMenuMore.label;" accesskey="&charsetMenuMore.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserMoreCharsetMenuRoot">
                        <template>
                            <rule>
                                <menupopup>
                                <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                </menupopup>
                            </rule>
                        </template>
                        <menupopup>
                            <menu label="&charsetMenuMore1.label;" accesskey="&charsetMenuMore1.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserMore1CharsetMenuRoot">
                                <template>
                                    <rule>
                                        <menupopup>
                                        <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                        </menupopup>
                                    </rule>
                                </template>
                                <menupopup>
                                </menupopup>
                            </menu>
                            <menu label="&charsetMenuMore2.label;" accesskey="&charsetMenuMore2.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserMore2CharsetMenuRoot">
                                <template>
                                    <rule>
                                        <menupopup>
                                        <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                        </menupopup>
                                    </rule>
                                </template>
                                <menupopup>
                                </menupopup>
                            </menu>
                            <menu label="&charsetMenuMore3.label;" accesskey="&charsetMenuMore3.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserMore3CharsetMenuRoot">
                                <template>
                                    <rule>
                                        <menupopup>
                                        <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                        </menupopup>
                                    </rule>
                                </template>
                                <menupopup>
                                </menupopup>
                            </menu>
                            <menu label="&charsetMenuMore4.label;" accesskey="&charsetMenuMore4.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserMore4CharsetMenuRoot">
                                <template>
                                    <rule>
                                        <menupopup>
                                        <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                        </menupopup>
                                    </rule>
                                </template>
                                <menupopup>
                                </menupopup>
                            </menu>
                            <menu label="&charsetMenuMore5.label;" accesskey="&charsetMenuMore5.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserMore5CharsetMenuRoot">
                                <template>
                                    <rule>
                                        <menupopup>
                                        <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                        </menupopup>
                                    </rule>
                                </template>
                                <menupopup>
                                </menupopup>
                            </menu>
                            <menu label="&charsetMenuUnicode.label;" accesskey="&charsetMenuUnicode.accesskey;" datasources="rdf:charset-menu" ref="NC:BrowserUnicodeCharsetMenuRoot">
                                <template>
                                    <rule>
                                        <menupopup>
                                        <menuitem uri="..." label="rdf:http://home.netscape.com/NC-rdf#Name"/>
                                        </menupopup>
                                    </rule>
                                </template>
                                <menupopup>
                                </menupopup>
                            </menu>
                            <menuseparator />
                        </menupopup>
                    </menu>
                    <menuitem name="charsetCustomize" accesskey="&charsetCustomize.accesskey;" label="&charsetCustomize.label;" oncommand="window.openDialog('chrome://global/content/customizeCharset.xul','PrefWindow', 'chrome,modal=yes,resizable=yes', 'browser')"/>
                    </menupopup>
                </menu>
                <menuseparator/>


            <menu id="history-menu"
                  oncommand="var url = event.target.getAttribute('statustext'); if (url) openUILink(url, event, false, true);"
                  onclick="checkForMiddleClick(this, event);"
                  label="&historyMenu.label;"
                  accesskey="&historyMenu.accesskey;">
              <menupopup id="goPopup"
                         type="places"
                         onpopupshowing="HistoryMenu.onPopupShowing(this);"
                         place="place:type=0&amp;sort=4&amp;maxResults=10">
                <menuitem label="&backCmd.label;"
#ifdef XP_MACOSX
                          key="goBackKb2"
#else
                          key="goBackKb"
#endif
                          oncommand="BrowserBack(event, true)"
                          onclick="checkForMiddleClick(this, event);">
                  <observes element="Browser:Back" attribute="disabled" />
                </menuitem>
                <menuitem label="&forwardCmd.label;" 
#ifdef XP_MACOSX
                          key="goForwardKb2"
#else
                          key="goForwardKb"
#endif
                          oncommand="BrowserForward(event, true)"
                          onclick="checkForMiddleClick(this, event);">
                  <observes element="Browser:Forward" attribute="disabled" />
                </menuitem>

                <menuitem label="&historyHomeCmd.label;"
                          command="Browser:Home"
                          key="goHome"/>
                <menuseparator id="startHistorySeparator" builder="start"/>
                <menuseparator id="endHistorySeparator" builder="end"/>
                <menu id="historyUndoMenu" label="&historyUndoMenu.label;" disabled="true">
                  <menupopup id="historyUndoPopup" onpopupshowing="HistoryMenu.populateUndoSubmenu();"/>
                </menu>
                <menuseparator id="endUndoSeparator"/>
                <menuitem label="&showAllHistoryCmd.label;"
                          command="viewHistorySidebar"/>
              </menupopup>
            </menu>

  <menu id="bookmarksMenu" 
        label="&bookmarksMenu.label;" accesskey="&bookmarksMenu.accesskey;"
        ondragenter="PlacesMenuDNDController.onBookmarksMenuDragEnter(event);"
        ondragdrop="nsDragAndDrop.drop(event, BookmarksMenuDropHandler);"
        ondragover="nsDragAndDrop.dragOver(event, BookmarksMenuDropHandler);"
        ondragexit="PlacesMenuDNDController.onDragExit(event);">
    <menupopup id="bookmarksMenuPopup"
               type="places" asyncinit="true"
               place="place:folder=2&amp;expandQueries=1"
               context="placesContext"
               openInTabs="children"
               oncommand="BookmarksEventHandler.onCommand(event);"
               onclick="BookmarksEventHandler.onClick(event);"
               onpopupshowing="BookmarksEventHandler.onPopupShowing(event);">
      <menuitem label="&addCurPageAsCmd.label;" 
                command="Browser:AddBookmarkAs" key="addBookmarkAsKb"/>
      <menuitem id="subscribeToPageMenuitem"
                label="&subscribeToPageMenuitem.label;"
                oncommand="return FeedHandler.subscribeToFeed(null, event);"
                onclick="checkForMiddleClick(this, event);"
                disabled="true"/>
      <menu id="subscribeToPageMenupopup"
            label="&subscribeToPageMenupopup.label;"
            hidden="true">
        <menupopup id="subscribeToPageSubmenuMenupopup" 
                   onpopupshowing="return FeedHandler.buildFeedList(event.target);" 
                   oncommand="return FeedHandler.subscribeToFeed(null, event);"
                   onclick="checkForMiddleClick(this, event);"/>
      </menu>
      <menuitem label="&addCurPagesCmd.label;" 
                command="Browser:BookmarkAllTabs" key="bookmarkAllTabsKb"/>
      <menuitem label="&showAllBookmarksCmd.label;" 
                command="viewBookmarksSidebar" key="manBookmarkKb"/>
      <menuseparator builder="start"/>
    </menupopup>
  </menu>
        
            <menu id="tools-menu" label="&toolsMenu.label;" accesskey="&toolsMenu.accesskey;">
              <menupopup id="menu_ToolsPopup">
              <menuitem id="menu_openDownloads" label="&downloads.label;"
                        accesskey="&downloads.accesskey;"
                        key="key_openDownloads" command="Tools:Downloads"/>
              <menuitem id="menu_openAddons" label="&addons.label;"
                        accesskey="&addons.accesskey;" command="Tools:Addons"/>
              <menuseparator/>
                <menuitem id="menu_find" label="&findOnCmd.label;" accesskey="&findOnCmd.accesskey;" key="key_find" command="cmd_find"/>

              <menuseparator id="sanitizeSeparator"/>
              <menuitem id="sanitizeItem"
                        accesskey="&clearPrivateDataCmd.accesskey;"
                        label="&clearPrivateDataCmd.label;"
                        key="key_sanitize" command="Tools:Sanitize"/>

              <menuseparator id="prefSep"/>
              <menuitem id="menu_preferences"
                        label="&preferencesCmdUnix.label;"
                        accesskey="&preferencesCmdUnix.accesskey;"
                        oncommand="openPreferences();"/>
              </menupopup>
            </menu>
    
             <menuseparator/>
            <menuitem id="aboutName"
                      accesskey="&aboutCmd.accesskey;"
                      label="&aboutCmd.label;"
                      oncommand="openAboutDialog();"/>

            <menuitem id="goOfflineMenuitem"
                      label="&goOfflineCmd.label;" accesskey="&goOfflineCmd.accesskey;" 
                      type="checkbox" oncommand="BrowserOffline.toggleOfflineStatus();"/>

            <menuitem id="menu_FileQuitItem" 
                          label="&quitApplicationCmd.label;"
                          accesskey="&quitApplicationCmd.accesskey;"
                          oncommand="goQuitApplication();"/>
        </menupopup>
#       </menubar>
