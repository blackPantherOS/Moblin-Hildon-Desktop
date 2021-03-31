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
 * Contributor(s):
 *   Robert O'Callahan (rocallahan@novell.com)
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

#include "nsSystemPref.h"
#include "nsIObserverService.h"
#include "nsIAppStartupNotifier.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"

#include "nsSystemPrefLog.h"
#include "nsString.h"

#include <stdlib.h>

struct SysPrefItem {
    // Saved values on both branches
    PRInt32      savedUserValueScalar;
    char*        savedUserValueString;
    PRInt32      savedDefaultValueScalar;
    char*        savedDefaultValueString;
    // When this is true, then the value was locked originally
    PRPackedBool savedLocked;
    // When this is true, then there was a user value
    PRPackedBool savedUserPresent;
    PRPackedBool ignore;
    
    SysPrefItem() {
        savedUserValueScalar = 0;
        savedUserValueString = nsnull;
        savedDefaultValueScalar = 0;
        savedDefaultValueString = nsnull;
        savedUserPresent = PR_FALSE;
        savedLocked = PR_FALSE;
        ignore = PR_FALSE;
    }

    virtual ~SysPrefItem() {
        nsMemory::Free(savedUserValueString);
        nsMemory::Free(savedDefaultValueString);
    }
};

static const char sSysPrefString[] = "config.use_system_prefs";

PRLogModuleInfo *gSysPrefLog = NULL;

NS_IMPL_ISUPPORTS2(nsSystemPref, nsIObserver, nsISupportsWeakReference)

nsSystemPref::nsSystemPref() : mIgnorePrefSetting(PR_FALSE)
{
   mSavedPrefs.Init();
   mCachedUserPrefBranch = nsnull;
   mCachedDefaultPrefBranch = nsnull;
}

nsSystemPref::~nsSystemPref()
{
}

///////////////////////////////////////////////////////////////////////////////
// nsSystemPref::Init
// Setup log and listen on NS_PREFSERVICE_READ_TOPIC_ID from pref service
///////////////////////////////////////////////////////////////////////////////
nsresult
nsSystemPref::Init(void)
{
    nsresult rv;

    if (!gSysPrefLog) {
        gSysPrefLog = PR_NewLogModule("Syspref");
        if (!gSysPrefLog)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIObserverService> observerService = 
        do_GetService("@mozilla.org/observer-service;1", &rv);

    if (observerService) {
        rv = observerService->AddObserver(this, NS_PREFSERVICE_READ_TOPIC_ID,
                                          PR_FALSE);
        rv = observerService->AddObserver(this, "profile-before-change",
                                          PR_FALSE);
        SYSPREF_LOG(("Add Observer for %s\n", NS_PREFSERVICE_READ_TOPIC_ID));
    }
    return(rv);
}

already_AddRefed<nsIPrefBranch2>
nsSystemPref::GetPrefUserBranch()
{
    if (mCachedUserPrefBranch) {
        NS_ADDREF(mCachedUserPrefBranch);
        return mCachedUserPrefBranch;
    }

    nsresult rv;
    nsCOMPtr<nsIPrefService> prefService = 
        do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return nsnull;
    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = prefService->GetBranch(nsnull, getter_AddRefs(prefBranch));
    if (NS_FAILED(rv))
        return nsnull;
    nsCOMPtr<nsIPrefBranch2> pb2(do_QueryInterface(prefBranch));
    if (!pb2)
        return nsnull;
    
    nsIPrefBranch2* result = nsnull;
    pb2.swap(result);
    return result;
}

already_AddRefed<nsIPrefBranch>
nsSystemPref::GetPrefDefaultBranch()
{
    if (mCachedDefaultPrefBranch) {
        NS_ADDREF(mCachedDefaultPrefBranch);
        return mCachedDefaultPrefBranch;
    }

    nsresult rv;
    nsCOMPtr<nsIPrefService> prefService = 
        do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return nsnull;
    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = prefService->GetDefaultBranch(nsnull, getter_AddRefs(prefBranch));
    if (NS_FAILED(rv))
        return nsnull;
    nsIPrefBranch* pb = nsnull;
    prefBranch.swap(pb);
    return pb;
}

///////////////////////////////////////////////////////////////////////////////
// nsSystemPref::Observe
// Observe notifications from mozilla pref system and system prefs (if enabled)
///////////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsSystemPref::Observe(nsISupports *aSubject,
                      const char *aTopic,
                      const PRUnichar *aData)
{
    nsresult rv = NS_OK;

    if (!aTopic)
        return NS_OK;

    nsCOMPtr<nsIPrefBranch2> userBranch = GetPrefUserBranch();
    PRBool enabled;
    rv = userBranch->GetBoolPref(sSysPrefString, &enabled);
    if (NS_FAILED(rv)) {
        SYSPREF_LOG(("...Failed to Get %s\n", sSysPrefString));
        return rv;
    }

    if (!nsCRT::strcmp(aTopic, NS_PREFSERVICE_READ_TOPIC_ID)) {
        // The prefs have just loaded. This is the first thing that
        // happens to us.
        SYSPREF_LOG(("Observed: %s\n", aTopic));

        // listen on changes to use_system_pref. It's OK to
        // hold a strong reference because we don't keep a reference
        // to the pref branch.
        rv = userBranch->AddObserver(sSysPrefString, this, PR_TRUE);
        if (NS_FAILED(rv)) {
            SYSPREF_LOG(("...Failed to add observer for %s\n", sSysPrefString));
            return rv;
        }

        NS_ASSERTION(!mSysPrefService, "Should not be already enabled");
        if (!enabled) {
            // Don't load the system pref service if the preference is
            // not set.
            return NS_OK;
        }

        SYSPREF_LOG(("%s is enabled\n", sSysPrefString));

        rv = LoadSystemPrefs();
        if (NS_FAILED(rv))
            return rv;

        // Lock config.use_system_prefs so the user can't undo
        // it. Really this should already be locked because if it's
        // not locked at a lower level the user can set it to false in
        // their in prefs.js. But only do this if the value was not
        // specially set by the user; if it was set by the user, then
        // locking it would actually unset the value! And the user
        // should be allowed to turn off something they set
        // themselves.
        PRBool hasUserValue;
        rv = userBranch->PrefHasUserValue(sSysPrefString, &hasUserValue);
        if (NS_SUCCEEDED(rv) && !hasUserValue) {
            userBranch->LockPref(sSysPrefString);
        }
    }

    if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) &&
        nsDependentString(aData).EqualsASCII(sSysPrefString)) {
        // sSysPrefString value was changed, update...
        SYSPREF_LOG(("++++++ Notify: topic=%s data=%s\n",
                     aTopic, NS_ConvertUTF16toUTF8(aData).get()));
        if (mSysPrefService && !enabled)
            return RestoreMozillaPrefs();
        if (!mSysPrefService && enabled) {
            // Don't lock it. If the user enabled use_system_prefs,
            // they should be allowed to unlock it.
            return LoadSystemPrefs();
        }

        // didn't change?
        return NS_OK;
    }

    if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        // some other pref changed, tell the backend if there is one
        if (mSysPrefService && !mIgnorePrefSetting) {
            NS_LossyConvertUTF16toASCII tmp(aData);
#ifdef DEBUG
            PRBool isLocked;
            userBranch->PrefIsLocked(tmp.get(), &isLocked);
            NS_ASSERTION(!isLocked, "Locked pref is changing?");
#endif
            SysPrefItem* item;
            if (!mSavedPrefs.Get(tmp, &item)) {
                NS_ERROR("Notified about pref change that we didn't ask about?");
            } else {
                if (!item->ignore) {
                    mSysPrefService->NotifyMozillaPrefChanged(tmp.get());
                }
            }
        }
        return NS_OK;
    }

    if (!nsCRT::strcmp(aTopic,"profile-before-change"))
        return RestoreMozillaPrefs();

    SYSPREF_LOG(("Not needed topic Received %s\n", aTopic));

    return rv;
}

nsresult
nsSystemPref::SetOverridingMozillaBoolPref(const char* aPrefName,
                                           PRBool aValue, PRBool aLock, PRBool aPresent)
{
    return OverridePref(aPrefName, nsIPrefBranch::PREF_BOOL,
                        (void*)aValue, aLock, aPresent);
}

nsresult
nsSystemPref::SetOverridingMozillaIntPref(const char* aPrefName,
                                          PRInt32 aValue, PRBool aLock, PRBool aPresent)
{
    return OverridePref(aPrefName, nsIPrefBranch::PREF_INT,
                        (void*)aValue, aLock, aPresent);
}

nsresult
nsSystemPref::SetOverridingMozillaStringPref(const char* aPrefName,
                                             const char* aValue, PRBool aLock, PRBool aPresent)
{
    return OverridePref(aPrefName, nsIPrefBranch::PREF_STRING,
                        (void*)aValue, aLock, aPresent);
}

static nsresult RestorePrefValue(PRInt32 aPrefType,
                                 const char* aPrefName,
                                 SysPrefItem* aItem,
                                 nsIPrefBranch* aUser,
                                 nsIPrefBranch* aDefault)
{
    switch (aPrefType) {
    case nsIPrefBranch::PREF_STRING:
        aDefault->SetCharPref(aPrefName,
                              aItem->savedDefaultValueString);
        if (aItem->savedUserPresent) {
            aUser->SetCharPref(aPrefName, aItem->savedUserValueString);
        }
        break;
    case nsIPrefBranch::PREF_INT:
        aDefault->SetIntPref(aPrefName, aItem->savedDefaultValueScalar);
        if (aItem->savedUserPresent) {
            aUser->SetIntPref(aPrefName, aItem->savedUserValueScalar);
        }
        break;
    case nsIPrefBranch::PREF_BOOL:
        aDefault->SetBoolPref(aPrefName, aItem->savedDefaultValueScalar);
        if (aItem->savedUserPresent) {
            aUser->SetBoolPref(aPrefName, aItem->savedUserValueScalar);
        }
        break;
    default:
        NS_ERROR("Unknown preference type");
        return NS_ERROR_FAILURE;
    }

    if (!aItem->savedUserPresent) {
        aUser->DeleteBranch(aPrefName);
    }

    return NS_OK;
}

static PLDHashOperator PR_CALLBACK RestorePref(const nsACString& aKey,
                                               SysPrefItem* aItem,
                                               void* aClosure)
{
    nsSystemPref* prefs = static_cast<nsSystemPref*>(aClosure);
    nsCOMPtr<nsIPrefBranch2> userBranch = prefs->GetPrefUserBranch();
    const nsCString& prefName = PromiseFlatCString(aKey);
    
    PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
    nsresult rv = userBranch->GetPrefType(prefName.get(), &prefType);
    if (NS_FAILED(rv))
        return PL_DHASH_NEXT;
    PRBool isLocked;
    userBranch->PrefIsLocked(prefName.get(), &isLocked);
    if (NS_FAILED(rv))
        return PL_DHASH_NEXT;

    // Remove our observer before we change the value
    userBranch->RemoveObserver(prefName.get(), prefs);
    // Remember to ignore this item. Because some prefs start with "config.use_system_prefs",
    // which we always observe, even after we remove the observer, changes to the pref will
    // still be observed by us. We must ignore them.
    aItem->ignore = PR_TRUE;

    // Unlock the pref so we can set it
    if (isLocked) {
        userBranch->UnlockPref(prefName.get());
    }

    nsCOMPtr<nsIPrefBranch> defaultBranch = prefs->GetPrefDefaultBranch();

    RestorePrefValue(prefType, prefName.get(), aItem,
                     userBranch, defaultBranch);

    if (aItem->savedLocked) {
        userBranch->LockPref(prefName.get());
    }

    return PL_DHASH_NEXT;
}

nsresult
nsSystemPref::StopOverridingMozillaPref(const char* aPrefName)
{
    SysPrefItem* item;
    nsDependentCString prefNameStr(aPrefName);
    if (!mSavedPrefs.Get(prefNameStr, &item))
        return NS_OK;

    RestorePref(prefNameStr, item, this);
    mSavedPrefs.Remove(prefNameStr);
    //delete item;
    return NS_OK;
}

/* private */

nsresult
nsSystemPref::OverridePref(const char* aPrefName, PRInt32 aType,
                           void* aValue, PRBool aLock, PRBool aPresent)
{
    nsCOMPtr<nsIPrefBranch2> userBranch = GetPrefUserBranch();
    nsCOMPtr<nsIPrefBranch> defaultBranch = GetPrefDefaultBranch();
    PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
    nsresult rv = userBranch->GetPrefType(aPrefName, &prefType);
    if (NS_FAILED(rv))
        return rv;

    PRBool isLocked;
    rv = userBranch->PrefIsLocked(aPrefName, &isLocked);
    if (NS_FAILED(rv))
        return rv;
    PRBool hasUserValue;
    rv = userBranch->PrefHasUserValue(aPrefName, &hasUserValue);
    if (NS_FAILED(rv))
        return rv;

    if (prefType == 0) {
        // Preference does not exist. Allow the system prefs to
        // set it.
    } else {
        NS_ASSERTION(aType == prefType,
                     "System pref engine passed incorrect type for Mozilla pref");
        if (aType != prefType)
            return NS_ERROR_FAILURE;
    }

    if (prefType != 0) {
        nsDependentCString prefNameStr(aPrefName);
        SysPrefItem* item = nsnull;
        if (!mSavedPrefs.Get(prefNameStr, &item)) {
            // Need to save the existing value away
            item = new SysPrefItem();
            if (!item)
                return NS_ERROR_OUT_OF_MEMORY;

            item->savedLocked = isLocked;
            item->savedUserPresent = hasUserValue;
        
            switch (prefType) {
            case nsIPrefBranch::PREF_STRING:
                if (hasUserValue) {
                    userBranch->GetCharPref(aPrefName, &item->savedUserValueString);
                }
                defaultBranch->GetCharPref(aPrefName, &item->savedDefaultValueString);
                break;
            case nsIPrefBranch::PREF_INT:
                if (hasUserValue) {
                    userBranch->GetIntPref(aPrefName, &item->savedUserValueScalar);
                }
                defaultBranch->GetIntPref(aPrefName, &item->savedDefaultValueScalar);
                break;
            case nsIPrefBranch::PREF_BOOL:
                if (hasUserValue) {
                    userBranch->GetBoolPref(aPrefName, &item->savedUserValueScalar);
                }
                defaultBranch->GetBoolPref(aPrefName, &item->savedDefaultValueScalar);
                break;
            default:
                NS_ERROR("Unknown preference type");
                delete item;
                return NS_ERROR_FAILURE;
            }

            mSavedPrefs.Put(prefNameStr, item);

            // Watch the user value in case it changes on the Mozilla side
            // If 'aLock' is true then it shouldn't change and we don't
            // need the observer, but don't bother optimizing for that.
            userBranch->AddObserver(aPrefName, this, PR_TRUE);
        } else {
            if (isLocked != aLock) {
                // restore pref value on user and default branches
                RestorePrefValue(prefType, aPrefName, item,
                                 userBranch, defaultBranch);
            }
        }
    }

    // We need to ignore pref changes due to our own calls here
    mIgnorePrefSetting = PR_TRUE;

    // Unlock it if it's locked, so we can set it
    if (isLocked) {
        rv = userBranch->UnlockPref(aPrefName);
        if (NS_FAILED(rv))
            return rv;
    }

    // Set the pref on the default branch if we're locking it, because
    // only the default branch gets used when the pref is locked.
    // Set the pref on the user branch if we're not locking it, because
    // that's where the user change will go.
    nsIPrefBranch* settingBranch =
        aLock ? defaultBranch.get() : static_cast<nsIPrefBranch*>(userBranch.get());

    if (!aPresent) {
        rv = settingBranch->DeleteBranch(aPrefName);
    } else {
        switch (aType) {
        case nsIPrefBranch::PREF_STRING:
            rv = settingBranch->SetCharPref(aPrefName, (const char*)aValue);
            break;
        case nsIPrefBranch::PREF_INT:
            rv = settingBranch->SetIntPref(aPrefName, (PRInt32)(NS_PTR_TO_INT32(aValue)));
            break;
        case nsIPrefBranch::PREF_BOOL:
            rv = settingBranch->SetBoolPref(aPrefName, (PRBool)(NS_PTR_TO_INT32(aValue)));
            break;
        default:
            NS_ERROR("Unknown preference type");
            mIgnorePrefSetting = PR_FALSE;
            return NS_ERROR_FAILURE;
        }
    }
    if (NS_FAILED(rv))
        return rv;
    if (aLock) {
        rv = userBranch->LockPref(aPrefName);
    }

    mIgnorePrefSetting = PR_FALSE;
    return rv;
}

nsresult
nsSystemPref::FixupLockdownPrefs()
{
    nsCOMPtr<nsIPrefBranch2> userPrefs = GetPrefUserBranch();
    nsCOMPtr<nsIPrefBranch2> defaultPrefs = GetPrefUserBranch();
    PRUint32 childCount;
    char **childArray = nsnull;
    nsresult rv = userPrefs->GetChildList("config.lockdown.",
                                          &childCount, &childArray);
    if (NS_FAILED(rv))
        return rv;
    for (PRUint32 i = 0; i < childCount; ++i) {
        PRInt32 type;
        rv = defaultPrefs->GetPrefType(childArray[i], &type);
        if (NS_FAILED(rv))
            return rv;
        NS_ASSERTION(type == nsIPrefBranch2::PREF_BOOL,
                     "All config.lockdown.* prefs should be boolean");
        if (type == nsIPrefBranch2::PREF_BOOL) {
            rv = defaultPrefs->SetBoolPref(childArray[i], PR_FALSE);
            if (NS_FAILED(rv))
                return rv;
        }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(childCount, childArray);
    return NS_OK;
}

nsresult
nsSystemPref::LoadSystemPrefs()
{
    SYSPREF_LOG(("\n====Now Use system prefs==\n"));
    NS_ASSERTION(!mSysPrefService,
                 "Shouldn't have the pref service here");
    nsresult rv;
    mSysPrefService = do_GetService(NS_SYSTEMPREF_SERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !mSysPrefService) {
        FixupLockdownPrefs();
        SYSPREF_LOG(("...No System Pref Service\n"));
        return NS_OK;
    }

    // Cache the pref-branch while we load up the system prefs.
    NS_ASSERTION(!mCachedUserPrefBranch,
                 "Shouldn't have a cache here");
    nsCOMPtr<nsIPrefBranch2> userBranch = GetPrefUserBranch();
    nsCOMPtr<nsIPrefBranch> defaultBranch = GetPrefDefaultBranch();
    mCachedDefaultPrefBranch = defaultBranch;
    mCachedUserPrefBranch = userBranch;
    rv = mSysPrefService->LoadSystemPreferences(this);
    mCachedDefaultPrefBranch = nsnull;
    mCachedUserPrefBranch = nsnull;

    if (NS_FAILED(rv)) {
        // Restore all modified preferences to their original values
        mSavedPrefs.EnumerateRead(RestorePref, this);
        mSavedPrefs.Clear();
        mSysPrefService = nsnull;
    }
        
    return rv;
}

nsresult
nsSystemPref::RestoreMozillaPrefs()
{
    SYSPREF_LOG(("\n====Now rollback to Mozilla prefs==\n"));

    NS_ASSERTION(mSysPrefService,
                 "Should have the pref service here");
    if (!mSysPrefService)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIPrefBranch2> userBranch = GetPrefUserBranch();
    nsCOMPtr<nsIPrefBranch> defaultBranch = GetPrefDefaultBranch();
    mCachedDefaultPrefBranch = defaultBranch;
    mCachedUserPrefBranch = userBranch;

    mSysPrefService->NotifyUnloadSystemPreferences();
    // Restore all modified preferences to their original values
    mSavedPrefs.EnumerateRead(RestorePref, this);
    mSavedPrefs.Clear();

    mCachedDefaultPrefBranch = nsnull;
    mCachedUserPrefBranch = nsnull;
    
    mSysPrefService = nsnull;

    FixupLockdownPrefs();

    return NS_OK;
}
