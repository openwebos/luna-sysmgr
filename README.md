Summary
========

This repository contains the webOS System Manager, which is a key webOS component responsible for:

* Managing the application and services interface for physical devices such as keys, accelerometer, vibrator, etc...
* Managing the running of applications and passing of messages between applications
* Managing the installation and removal of applications
* Managing the display of applications
* Managing the sharing of system resources between different applications and services
* Managing the dock mode status
* Managing the security policy and access to a locked device
* Providing for the display of notifications
* Providing for system menus
* Providing for the coordinated rendering of applications
* Rendering webOS card view, lock screen, status bar, system menus, virtual keyboard, notifications, and launcher, in addition to other system management features that viewable in the System Manager User interface

WebAppMgr is provided by System Manager and is responsible for running Enyo applications.


LunaSysmgr
==========

This component supports the following methods, which are described in detail in the generated documentation:  

* com.palm.ambientLightSensor/control/status
* com.palm.appDataBackup/postRestore
* com.palm.appDataBackup/preBackup
* com.palm.appinstaller/getUserInstalledAppSizes
* com.palm.appinstaller/queryInstallCapacity
* com.palm.appinstaller/install
* com.palm.appinstaller/installNoVerify
* com.palm.appinstaller/installProgressQuery
* com.palm.appinstaller/isInstalled
* com.palm.appinstaller/notifyOnChange
* com.palm.appinstaller/remove
* com.palm.appinstaller/revoke
* com.palm.applicationManager/addDockModeLaunchPoint
* com.palm.applicationManager/addLaunchPoint
* com.palm.applicationManager/addRedirectHandler
* com.palm.applicationManager/addResourceHandler
* com.palm.applicationManager/clearMimeTable
* com.palm.applicationManager/close
* com.palm.applicationManager/dumpMimeTable
* com.palm.applicationManager/forceSingleAppScan
* com.palm.applicationManager/getAppBasePath
* com.palm.applicationManager/getAppInfo
* com.palm.applicationManager/getHandlerForExtension
* com.palm.applicationManager/getHandlerForMimeType
* com.palm.applicationManager/getHandlerForMimeTypeByVerb
* com.palm.applicationManager/getHandlerForUrl
* com.palm.applicationManager/getHandlerForUrlByVerb
* com.palm.applicationManager/getResourceInfo
* com.palm.applicationManager/getSizeOfApps
* com.palm.applicationManager/inspect
* com.palm.applicationManager/install
* com.palm.applicationManager/launch
* com.palm.applicationManager/launchPointChanges
* com.palm.applicationManager/listAllHandlersForMime
* com.palm.applicationManager/listAllHandlersForMimeByVerb
* com.palm.applicationManager/listAllHandlersForMultipleMime
* com.palm.applicationManager/listAllHandlersForMultipleUrlPattern
* com.palm.applicationManager/listAllHandlersForUrl
* com.palm.applicationManager/listAllHandlersForUrlByVerb
* com.palm.applicationManager/listAllHandlersForUrlPattern
* com.palm.applicationManager/listApps
* com.palm.applicationManager/listDockModeLaunchPoints
* com.palm.applicationManager/listDockPoints
* com.palm.applicationManager/listExtensionMap
* com.palm.applicationManager/listLaunchPoints
* com.palm.applicationManager/listPackages
* com.palm.applicationManager/listPendingLaunchPoints
* com.palm.applicationManager/listRedirectHandlers
* com.palm.applicationManager/listResourceHandlers
* com.palm.applicationManager/mimeTypeForExtension
* com.palm.applicationManager/open
* com.palm.applicationManager/registerVerbsForRedirect
* com.palm.applicationManager/registerVerbsForResource
* com.palm.applicationManager/removeDockModeLaunchPoint
* com.palm.applicationManager/removeHandlersForAppId
* com.palm.applicationManager/removeLaunchPoint
* com.palm.applicationManager/rescan
* com.palm.applicationManager/resetToMimeDefaults
* com.palm.applicationManager/restoreMimeTable
* com.palm.applicationManager/running
* com.palm.applicationManager/saveMimeTable
* com.palm.applicationManager/searchApps
* com.palm.applicationManager/swapRedirectHandler
* com.palm.applicationManager/swapResourceHandler
* com.palm.applicationManager/updateLaunchPointIcon
* com.palm.display/status
* com.palm.display/control/getProperty
* com.palm.display/control/setProperty
* com.palm.display/control/setState
* com.palm.display/control/status
* com.palm.keys/audio/status
* com.palm.keys/headset/status
* com.palm.keys/media/status
* com.palm.keys/switches/status
* com.palm.systemmanager/applicationHasBeenTerminated
* com.palm.systemmanager/clearCache
* com.palm.systemmanager/dismissModalApp
* com.palm.systemmanager/getAnimationValues
* com.palm.systemmanager/getAppRestoreNeeded
* com.palm.systemmanager/getBootStatus
* com.palm.systemmanager/getDeviceLockMode
* com.palm.systemmanager/getDockModeStatus
* com.palm.systemmanager/getForegroundApplication
* com.palm.systemmanager/getLockStatus
* com.palm.systemmanager/getSecurityPolicy
* com.palm.systemmanager/getSystemStatus
* com.palm.systemmanager/launchModalApp
* com.palm.systemmanager/lockButtonTriggered
* com.palm.systemmanager/matchDevicePasscode
* com.palm.systemmanager/publishToSystemUI
* com.palm.systemmanager/runProgressAnimation
* com.palm.systemmanager/setAnimationValues
* com.palm.systemmanager/setDevicePasscode
* com.palm.systemmanager/setJavascriptFlags
* com.palm.systemmanager/subscribeToSystemUI
* com.palm.systemmanager/subscribeTurboMode
* com.palm.systemmanager/systemUi
* com.palm.systemmanager/takeScreenShot
* com.palm.systemmanager/touchToShareAppUrlTransferred
* com.palm.systemmanager/touchToShareDeviceInRange
* com.palm.systemmanager/updatePinAppState
* com.palm.vibrate/vibrate
* com.palm.vibrate/vibrateNamedEffect


About the 'unstable' branch
===========================

Minor, low-risk changes are submitted directly to the 'master' branch.
Periodically we carefully review a set of changes and create a tag,
which corresponds to a stable point.

The 'unstable' branch is used for significant changes while those changes
are under active development, and allows for the ongoing review of these changes.  

Presently, the 'unstable' branch of luna-sysmgr is being used for the development
of the Pluggable Keyboard capability, and should be viewed in conjunction with the
'unstable' branch of luna-webkit-api and the 'master' branch of keyboard-efigs.  


How to Build on Linux
=====================

### Building the latest "stable" version

Clone the repository openwebos/build-desktop and follow the instructions in the README file.

### Building your local clone

First follow the directions to build the latest "stable" version.

To build your local clone of luna-sysmgr instead of the "stable" version installed with the build-webos-desktop script:

* Open the build-webos-desktop.sh script with a text editor
* Locate the function build_luna-sysmgr
* Change the line "cd $BASE/luna-sysmgr" to use the folder containing your clone, for example "cd ~/github/luna-sysmgr"
* Close the text editor
* Remove the file ~/luna-desktop-binaries/luna-sysmgr/luna-desktop-build*.stamp
* Start the build

Cautions:

* When you re-clone openwebos/build-desktop, you'll have to overwrite your changes and reapply them
* Components often advance in parallel with each other, so be prepared to keep your cloned repositories updated
* Fetch and rebase frequently

# Copyright and License Information

All content, including all source code files and documentation files in this repository except otherwise noted are: 

 Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.

All content, including all source code files and documentation files in this repository except otherwise noted are:
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this content except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
