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
# The Original Code is the Mozilla Installer code.
#
# The Initial Developer of the Original Code is Mozilla Foundation
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#  Robert Strong <robert.bugzilla@gmail.com>
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

# NSIS branding defines for official release builds.
# The nightly build branding.nsi is located in browser/installer/windows/nsis/
# The unofficial build branding.nsi is located in browser/branding/unofficial/
!define BrandShortName        "Firefox"
# BrandFullNameInternal is used for some registry and file system values
# instead of BrandFullName and typically should not be modified.
!define BrandFullNameInternal "Mozilla Firefox"
!define CompanyName           "Mozilla Corporation"
!define URLInfoAbout          "http://${AB_CD}.www.mozilla.com/${AB_CD}/"
!define URLUpdateInfo         "http://${AB_CD}.www.mozilla.com/${AB_CD}/firefox/"
!define SurveyURL             "https://survey.mozilla.com/1/Mozilla%20Firefox/${AppVersion}/${AB_CD}/exit.html"

# Everything below this line may be modified for Alpha / Beta releases.
!define BrandFullName         "Mozilla Firefox 3 Beta 2"

# Add !define NO_INSTDIR_FROM_REG to prevent finding a non-default installation
# directory in the registry and using that as the default. This prevents
# Beta releases built with official branding from finding an existing install
# of an official release and defaulting to its installation directory.
!define NO_INSTDIR_FROM_REG
