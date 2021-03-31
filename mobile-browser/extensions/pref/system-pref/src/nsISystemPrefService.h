/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:expandtab:shiftwidth=4:tabstop=4:
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Novell
 * Portions created by Novell are Copyright (C) 2005 Novell,
 * All Rights Reserved.
 *
 * Original Author: Robert O'Callahan (rocallahan@novell.com)
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsISystemPrefService_h__
#define nsISystemPrefService_h__

#include "nsCOMPtr.h"
#include "nsIPrefBranchInternal.h"

#define NS_SYSTEMPREF_SERVICE_CONTRACTID "@mozilla.org/system-preferences-service;1"

#define NS_ISYSTEMPREFSERVICE_IID   \
{ 0x006e1cfd, 0xd66a, 0x40b9, \
    { 0x84, 0xa1, 0x84, 0xf3, 0xe6, 0xa2, 0xca, 0xbc } }

class nsISystemPref {
public:
    /**
     * Call one of these three methods to override a Mozilla
     * preference with a system value. You can call it multiple
     * times to change the value of a given preference to track
     * the underlying system value.
     *
     * If aLocked is true then we set the default preference and
     * lock it so the user value is ignored. If aLocked is false
     * then we unlock the Mozilla preference and set the Mozilla
     * user value.
     */
    virtual nsresult SetOverridingMozillaBoolPref(const char* aPrefName,
                                                  PRBool aValue, PRBool aLocked,
                                                  PRBool aPresent = PR_TRUE) = 0;
    virtual nsresult SetOverridingMozillaIntPref(const char* aPrefName,
                                                 PRInt32 aValue, PRBool aLocked,
                                                 PRBool aPresent = PR_TRUE) = 0;
    virtual nsresult SetOverridingMozillaStringPref(const char* aPrefName,
                                                    const char* aValue, PRBool aLocked,
                                                    PRBool aPresent = PR_TRUE) = 0;
    virtual nsresult StopOverridingMozillaPref(const char* aPrefName) = 0;
    virtual already_AddRefed<nsIPrefBranch2> GetPrefUserBranch() = 0;
    virtual already_AddRefed<nsIPrefBranch> GetPrefDefaultBranch() = 0;
};

class nsISystemPrefService : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISYSTEMPREFSERVICE_IID)

    /**
     * Load the system prefs from the store into their corresponding
     * Mozilla prefs, calling SetOverridingMozillaPref on each
     * such pref.
     */
    virtual nsresult LoadSystemPreferences(nsISystemPref* aPrefs) = 0;

    /**
     * Notify that a Mozilla user pref that is being overridden by the
     * store has changed.  The new value of the Mozilla pref should be
     * written back to the store.
     */
    virtual nsresult NotifyMozillaPrefChanged(const char* aPrefName) = 0;

    /**
     * Notify that we're about to stop using the system prefs.  After
     * this, nsSystemPref will automatically stop overriding all
     * Mozilla prefs that are being overridden.
     */
    virtual nsresult NotifyUnloadSystemPreferences() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISystemPrefService, NS_ISYSTEMPREFSERVICE_IID)

#endif
