/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:expandtab:shiftwidth=4:tabstop=4:
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Initial Developer of the Original Code is Sun Microsystems, Inc.
 * Portions created by Sun Microsystems are Copyright (C) 2003 Sun
 * Microsystems, Inc. All Rights Reserved.
 *
 * Original Author: Bolian Yin (bolian.yin@sun.com)
 *
 * Contributor(s): Robert O'Callahan/Novell (rocallahan@novell.com)
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

#ifndef __SYSTEM_PREF_H__
#define __SYSTEM_PREF_H__

#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsCRT.h"
#include "nsWeakReference.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsMemory.h"

#include "nsISystemPrefService.h"
#include "nsIObserver.h"

struct SysPrefItem;

//////////////////////////////////////////////////////////////////////////
//
// nsSystemPref, as an extension of mozilla pref service, reads some mozilla
// prefs from host system when the feature is enabled ("config.system-pref").
//
// nsSystemPref listens on NS_PREFSERVICE_READ_TOPIC_ID. When
// notified, nsSystemPref will start the nsSystemPrefService (platform
// specific) and tell it to override Mozilla prefs with its own
// settings.
//
// When overriding a Mozilla preference the prefservice can request the
// pref be locked or unlocked. If the pref is locked then we set the default
// value and lock it in Mozilla so the user value is ignored and the user cannot
// change the value. If the pref is unlocked then we set the user value
// and unlock it in Mozilla so the user can change it. If the user changes it,
// then the prefservice is notified so it can copy the value back to its
// underlying store.
//
// We detect changes to Mozilla prefs by observing pref changes in the
// user branch.
//
// For testing purposes, if the user toggles on
// config.use_system_prefs then we save the current preferences before
// overriding them from gconf, and if the user toggles off
// config.use_system_prefs *in the same session* then we restore the
// preferences. If the user exits without turning off use_system_prefs
// then the saved values are lost and the new values are permanent.
//
//////////////////////////////////////////////////////////////////////////

class nsSystemPref : public nsIObserver,
                     public nsSupportsWeakReference,
                     public nsISystemPref
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsSystemPref();
    virtual ~nsSystemPref();
    nsresult Init(void);

    // nsISystemPref
    virtual nsresult SetOverridingMozillaBoolPref(const char* aPrefName,
                                                  PRBool aValue, PRBool aLocked,
                                                  PRBool aPresent = PR_TRUE);
    virtual nsresult SetOverridingMozillaIntPref(const char* aPrefName,
                                                 PRInt32 aValue, PRBool aLocked,
                                                 PRBool aPresent = PR_TRUE);
    virtual nsresult SetOverridingMozillaStringPref(const char* aPrefName,
                                                    const char* aValue, PRBool aLocked,
                                                    PRBool aPresent = PR_TRUE);
    virtual nsresult StopOverridingMozillaPref(const char* aPrefName);
    virtual already_AddRefed<nsIPrefBranch2> GetPrefUserBranch();
    virtual already_AddRefed<nsIPrefBranch> GetPrefDefaultBranch();

private:
    // If we don't load the system prefs for any reason, then
    // set all config.lockdown.* preferences to PR_FALSE so that
    // residual lockdown settings are removed.
    nsresult FixupLockdownPrefs();

    nsresult LoadSystemPrefs();

    nsresult RestoreMozillaPrefs();

    nsresult OverridePref(const char* aPrefName, PRInt32 aType,
                          void* aValue, PRBool aLock, PRBool aPresent);

    nsCOMPtr<nsISystemPrefService>  mSysPrefService;
    nsClassHashtable<nsCStringHashKey,SysPrefItem> mSavedPrefs;
    // weak pointers to cached prefbranches
    nsIPrefBranch2* mCachedUserPrefBranch;
    nsIPrefBranch* mCachedDefaultPrefBranch;
    PRPackedBool mIgnorePrefSetting;
};

#define NS_SYSTEMPREF_CID                  \
  { /* {549abb24-7c9d-4aba-915e-7ce0b716b32f} */       \
    0x549abb24,                                        \
    0x7c9d,                                            \
    0x4aba,                                            \
    { 0x91, 0x5e, 0x7c, 0xe0, 0xb7, 0x16, 0xb3, 0x2f } \
  }

#define NS_SYSTEMPREF_CONTRACTID "@mozilla.org/system-preferences;1"
#define NS_SYSTEMPREF_CLASSNAME "System Preferences"

#endif  /* __SYSTEM_PREF_H__ */
