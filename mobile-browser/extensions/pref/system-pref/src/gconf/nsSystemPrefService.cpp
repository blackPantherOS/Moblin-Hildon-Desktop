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

#include <glib.h>
#include <glib-object.h>
#include <gconf/gconf-client.h>

#include "plstr.h"
#include "nsCOMPtr.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIPrefBranch2.h"
#include "nsISystemPrefService.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsIPermissionManager.h"

#define NS_SYSTEMPREF_SERVICE_CID                  \
  { /* {3724e748-b088-4bf8-9298-aad426b66293} */       \
    0x3724e748,                                        \
    0xb088,                                            \
    0x4bf8,                                            \
        { 0x92, 0x98, 0xaa, 0xd4, 0x26, 0xb6, 0x62, 0x93 } \
  }

#define NS_SYSTEMPREF_SERVICE_CLASSNAME "System Preferences Platform Service"

/**
 * We can link directly to the gconf library. If it's not available,
 * this component just won't load and no system prefs will be offered.
 */

#define NUM_ELEM(a) (sizeof(a)/sizeof(a[0]))

class nsSystemPrefService;

/**
 * List the preferences that have a simple mapping between Moz and gconf.
 * These preferences have the same meaning and their values are
 * automatically converted.
 */
struct SimplePrefMapping {
    const char *mozPrefName;
    const char *gconfPrefName;
    /**
     * If this is PR_FALSE, then we never allow Mozilla to change
     * this setting. The Mozilla pref will always be locked.
     * If this is PR_TRUE then Mozilla will be allowed to change
     * the setting --- but only if it is writable in gconf.
     */
    PRBool allowWritesFromMozilla;
};
typedef nsresult (* ComplexGConfPrefChanged)(nsSystemPrefService* aPrefService,
                                             GConfClient* aClient);
typedef nsresult (* ComplexMozPrefChanged)(nsSystemPrefService* aPrefService,
                                           GConfClient* aClient);
struct ComplexGConfPrefMapping {
    const char* gconfPrefName;
    ComplexGConfPrefChanged callback;
};

struct ComplexMozPrefMapping {
    const char* mozPrefName;
    ComplexMozPrefChanged callback;
};

class nsSystemPrefService : public nsISystemPrefService
{
public:
    NS_DECL_ISUPPORTS

    nsresult Init();

    virtual nsresult LoadSystemPreferences(nsISystemPref* aPrefs);
    virtual nsresult NotifyMozillaPrefChanged(const char* aPrefName);
    virtual nsresult NotifyUnloadSystemPreferences();

    nsSystemPrefService();
    virtual ~nsSystemPrefService();

    nsISystemPref* GetPrefs() { return mPref; }
    SimplePrefMapping* GetSimpleCallbackData(PRUint32 aKey) {
      SimplePrefMapping* result = nsnull;
      mGConfSimpleCallbacks.Get(aKey, &result);
      return result;
    }
    ComplexGConfPrefMapping* GetComplexCallbackData(PRUint32 aKey) {
      ComplexGConfPrefMapping* result = nsnull;
      mGConfComplexCallbacks.Get(aKey, &result);
      return result;
    }

private:
    nsISystemPref* mPref;
    nsDataHashtable<nsUint32HashKey, SimplePrefMapping*> mGConfSimpleCallbacks;
    nsDataHashtable<nsUint32HashKey, ComplexGConfPrefMapping*> mGConfComplexCallbacks;
    // This is set to PR_FALSE temporarily to stop listening to gconf
    // change notifications (while we change gconf values)
    PRPackedBool mListenToGConf;
};

nsSystemPrefService::nsSystemPrefService()
    : mPref(nsnull), mListenToGConf(PR_TRUE)
{
    mGConfSimpleCallbacks.Init();
    mGConfComplexCallbacks.Init();
}

nsSystemPrefService::~nsSystemPrefService()
{
    NotifyUnloadSystemPreferences();
}

nsresult
nsSystemPrefService::Init()
{
    return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsSystemPrefService, nsISystemPrefService)

static GConfClient* GetGConf() {
    return gconf_client_get_default();
}

static PRBool VerifyMatchingTypes(nsISystemPref* aPrefs,
                                  const char* aMozPref, GConfValue* aVal)
{
    nsCOMPtr<nsIPrefBranch2> prefBranch = aPrefs->GetPrefUserBranch();
    PRInt32 type;
    nsresult rv = prefBranch->GetPrefType(aMozPref, &type);
    if (NS_FAILED(rv)) {
        // pref probably doesn't exist. Let gconf set it.
        return PR_TRUE;
    }

    PRBool ok;
    switch (aVal->type) {
    case GCONF_VALUE_STRING:
        ok = type == nsIPrefBranch2::PREF_STRING;
        break;
    case GCONF_VALUE_INT:
        ok = type == nsIPrefBranch2::PREF_INT;
        break;
    case GCONF_VALUE_BOOL:
        ok = type == nsIPrefBranch2::PREF_BOOL;
        break;
    default:
        NS_ERROR("Unhandled gconf preference type");
        return PR_FALSE;
    }
    
    NS_ASSERTION(ok, "Mismatched gconf/Mozilla pref types");
    return ok;
}

/**
 * Map a gconf pref value into the corresponding Mozilla pref.
 */
static nsresult ApplySimpleMapping(SimplePrefMapping* aMap,
                                   nsISystemPref* aPrefs,
                                   GConfClient* aClient)
{
    GConfValue* val = gconf_client_get(aClient, aMap->gconfPrefName, nsnull);
    if (!val) {
        // No gconf key, so there's really nothing to do
        return NS_OK;
    }

    VerifyMatchingTypes(aPrefs, aMap->mozPrefName, val);

    PRBool locked = !aMap->allowWritesFromMozilla ||
        !gconf_client_key_is_writable(aClient, aMap->gconfPrefName, nsnull);
    nsresult rv;
    switch (val->type) {
    case GCONF_VALUE_STRING: {
        const char* str = gconf_value_get_string(val);
        rv = aPrefs->SetOverridingMozillaStringPref(aMap->mozPrefName, str, locked);
        // XXX do we need to free 'str' here?
        break;
    }
    case GCONF_VALUE_INT:
        rv = aPrefs->SetOverridingMozillaIntPref(aMap->mozPrefName,
                                                 gconf_value_get_int(val), locked);
        break;
    case GCONF_VALUE_BOOL:
        rv = aPrefs->SetOverridingMozillaBoolPref(aMap->mozPrefName,
                                                  gconf_value_get_bool(val), locked);
        break;
    default:
        NS_ERROR("Unusable gconf value type");
        rv = NS_ERROR_FAILURE;
        break;
    }
        
    gconf_value_free(val);
    return rv;
}

/**
 * Map a Mozilla pref into the corresponding gconf pref, if
 * that's allowed.
 */
static nsresult ReverseApplySimpleMapping(SimplePrefMapping* aMap,
                                          nsISystemPref* aPrefs,
                                          GConfClient* aClient)
{
    // Verify that the gconf key has the right type, if it exists
    GConfValue* val = gconf_client_get(aClient, aMap->gconfPrefName, nsnull);
    if (val) {
        VerifyMatchingTypes(aPrefs, aMap->mozPrefName, val);
        gconf_value_free(val);
    }

    PRBool writable = aMap->allowWritesFromMozilla &&
        gconf_client_key_is_writable(aClient, aMap->gconfPrefName, nsnull);
    if (!writable) {
        NS_ERROR("Gconf key is not writable");
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIPrefBranch2> prefBranch = aPrefs->GetPrefUserBranch();
    PRInt32 type;
    nsresult rv = prefBranch->GetPrefType(aMap->mozPrefName, &type);
    if (NS_FAILED(rv)) {
        NS_ERROR("Writing back a pref that doesn't exist?");
        return rv;
    }

    switch (type) {
    case nsIPrefBranch2::PREF_STRING:
        {
            char* result;
            rv = prefBranch->GetCharPref(aMap->mozPrefName, &result);
            if (NS_FAILED(rv))
                return rv;

            gconf_client_set_string(aClient, aMap->gconfPrefName, result, nsnull);
            nsMemory::Free(result);
        }
        break;
    case nsIPrefBranch2::PREF_INT:
        {
            PRInt32 result;
            rv = prefBranch->GetIntPref(aMap->mozPrefName, &result);
            if (NS_FAILED(rv))
                return rv;

            gconf_client_set_int(aClient, aMap->gconfPrefName, result, nsnull);
        }
        break;
    case nsIPrefBranch2::PREF_BOOL:
        {
            PRBool result;
            rv = prefBranch->GetBoolPref(aMap->mozPrefName, &result);
            if (NS_FAILED(rv))
                return rv;

            gconf_client_set_bool(aClient, aMap->gconfPrefName, result, nsnull);
        }
        break;
    default:
        NS_ERROR("Unhandled gconf preference type");
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

/* BEGIN preference mapping definition area
 *
 * There are a few rules that our preference maps have to obey:
 *
 * 1) Each mapping defines a relationship R between a set of GConf preferences and
 * a set of Mozilla preferences that must *always* be true. Thus, when a Mozilla
 * pref changes or a gconf pref changes, we may need to change something on the
 * other side to preserve R. If a GConf preference is read-only, then we may
 * need to lock one or more Mozilla preferences to avoid a situation where the
 * Mozilla preference changes and we can't update the GConf preference to
 * ensure R continues to hold.
 *
 * 2) If an unlocked Mozilla preference is changed, then we can only
 * preserve R by changing GConf preferences; we are not allowed to
 * change Mozilla preferences.
 *
 * 3) If a GConf preference is changed, then we can only preserve R by
 * changing Moozilla preferences; we are nt allowed to change GConf
 * preferences.
 *
 * For "simple" mappings, the relationship R is just of the form
 * "GConf preference 'A' is equal to Mozilla preference 'B'". R is
 * preserved by setting A to B when B changes, and by setting B to A
 * when A changes. If A is read-only then we lock B (or we may just
 * decide to lock B for other reasons). Thus rules 1-3 are satisfied.
 *
 * For "complex" mappings we have more complicated
 * relationships. These are documented below.
 */

static SimplePrefMapping sSimplePrefMappings[] = {
    // GNOME proxy settings; allow these to be set through the Firefox UI
    {"network.proxy.http", "/system/http_proxy/host", PR_TRUE},
    {"network.proxy.http_port", "/system/http_proxy/port", PR_TRUE},
    {"network.proxy.ftp", "/system/proxy/ftp_host", PR_TRUE},
    {"network.proxy.ftp_port", "/system/proxy/ftp_port", PR_TRUE},
    {"network.proxy.ssl", "/system/proxy/secure_host", PR_TRUE},
    {"network.proxy.ssl_port", "/system/proxy/secure_port", PR_TRUE},
    {"network.proxy.socks", "/system/proxy/socks_host", PR_TRUE},
    {"network.proxy.socks_port", "/system/proxy/socks_port", PR_TRUE},
    {"network.proxy.autoconfig_url", "/system/proxy/autoconfig_url", PR_TRUE},
    
    // GNOME accessibility setting; never allow this to be set by Firefox
    {"config.use_system_prefs.accessibility",
     "/desktop/gnome/interface/accessibility", PR_FALSE},

    // GConf Firefox preferences; allow these to be set through the Firefox UI
    {"security.enable_java", "/apps/firefox/web/java_enabled", PR_TRUE},
    {"javascript.enabled", "/apps/firefox/web/javascript_enabled", PR_TRUE},
    {"browser.startup.homepage", "/apps/firefox/general/homepage_url", PR_TRUE},
    {"browser.cache.disk.capacity", "/apps/firefox/web/cache_size", PR_TRUE},
    {"network.cookie.lifetimePolicy", "/apps/firefox/web/cookie_accept", PR_TRUE},

    // UI lockdown settings; never allow these to be set by Firefox. There is no
    // Firefox UI for these but they could otherwise be set via about:config.
    {"config.lockdown.printing", "/desktop/gnome/lockdown/disable_printing", PR_FALSE},
    {"config.lockdown.printsetup", "/desktop/gnome/lockdown/disable_print_setup", PR_FALSE},
    {"config.lockdown.savepage", "/desktop/gnome/lockdown/disable_save_to_disk", PR_FALSE},
    {"config.lockdown.history", "/apps/firefox/lockdown/disable_history", PR_FALSE},
    {"config.lockdown.toolbarediting", "/apps/firefox/lockdown/disable_toolbar_editing", PR_FALSE},
    {"config.lockdown.urlbar", "/apps/firefox/lockdown/disable_url_bar", PR_FALSE},
    {"config.lockdown.bookmark", "/apps/firefox/lockdown/disable_bookmark_editing", PR_FALSE},
    {"config.lockdown.disable_themes", "/apps/firefox/lockdown/disable_themes", PR_FALSE},
    {"config.lockdown.disable_extensions", "/apps/firefox/lockdown/disable_extensions", PR_FALSE},
    {"config.lockdown.searchbar", "/apps/firefox/lockdown/disable_searchbar", PR_FALSE},
    {"config.lockdown.hidebookmark", "/apps/firefox/lockdown/hide_bookmark", PR_FALSE},
};

static nsresult ApplyListPref(nsSystemPrefService* aPrefService,
                             GConfClient* aClient,
                             const char* aGConfKey, const char* aMozKey,
                             char aSeparator)
{
    GSList* list = gconf_client_get_list(aClient, aGConfKey,
                                         GCONF_VALUE_STRING, nsnull);
    nsCAutoString str;
    for (GSList* l = list; l; l = l->next) {
        str.Append((const char*)l->data);
        if (l->next) {
            str.Append(aSeparator);
        }
    }
    PRBool lock = !gconf_client_key_is_writable(aClient, aGConfKey, nsnull);
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaStringPref(aMozKey, str.get(), lock);
    // XXX does this free the strings? Should it?
    g_slist_free(list);
    return rv;
}
static nsresult ReverseApplyListPref(nsSystemPrefService* aPrefService,
                                    GConfClient* aClient,
                                    const char* aGConfKey, const char* aMozKey,
                                    char aSeparator)
{
    char* data = nsnull;
    nsCOMPtr<nsIPrefBranch2> prefs =
        aPrefService->GetPrefs()->GetPrefUserBranch();
    prefs->GetCharPref(aMozKey, &data);
    if (!data)
        return NS_ERROR_FAILURE;
    nsresult rv = NS_OK;
    GSList* list = nsnull;
    PRInt32 i = 0;
    while (data[i]) {
        const char* nextComma = strchr(data+i, ',');
        PRInt32 tokLen = nextComma ? nextComma - (data+i) : strlen(data+i);
        char* tok = strndup(data+i, tokLen);
        if (!tok)
            break;
        GSList* newList = g_slist_append(list, tok);
        if (!newList) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
        }
        list = newList;
        if (!nextComma)
            break;
        i = nextComma + 1 - data;
    }
    nsMemory::Free(data);
    if (NS_SUCCEEDED(rv)) {
        gconf_client_set_list(aClient, aGConfKey, GCONF_VALUE_STRING, list, nsnull);
    }
    for (GSList* l = list; l; l = l->next) {
        free(l->data);
    }
    g_slist_free(list);
    return rv;
}

/**
 * The relationship R is
 * "network.negotiate-auth.trusted-uris" is the comma-separated concatenation
 * of the elements of the list "/apps/firefox/general/trusted_URIs"
 */
static const char GConfKey_TrustedURIs[] = "/apps/firefox/general/trusted_URIs";
static const char MozKey_TrustedURIs[] = "network.negotiate-auth.trusted-uris";
static nsresult ApplyTrustedURIs(nsSystemPrefService* aPrefService,
                                GConfClient* aClient)
{
    return ApplyListPref(aPrefService, aClient, 
                        GConfKey_TrustedURIs, MozKey_TrustedURIs, ',');
}
static nsresult ReverseApplyTrustedURIs(nsSystemPrefService* aPrefService,
                                       GConfClient* aClient)
{
    return ReverseApplyListPref(aPrefService, aClient, 
                               GConfKey_TrustedURIs, MozKey_TrustedURIs, ',');
}

/**
 * The relationship R is
 * "network.negotiate-auth.delegation-uris" is the comma-separated concatenation
 * of the elements of the list "/apps/firefox/general/delegation_URIs"
 */
static const char GConfKey_DelegationURIs[] = "/apps/firefox/general/delegation_URIs";
static const char MozKey_DelegationURIs[] = "network.negotiate-auth.delegation-uris";
static nsresult ApplyDelegationURIs(nsSystemPrefService* aPrefService,
                                   GConfClient* aClient)
{
    return ApplyListPref(aPrefService, aClient, 
                        GConfKey_DelegationURIs, MozKey_DelegationURIs, ',');
}
static nsresult ReverseApplyDelegationURIs(nsSystemPrefService* aPrefService,
                                          GConfClient* aClient)
{
    return ReverseApplyListPref(aPrefService, aClient, 
                               GConfKey_DelegationURIs, MozKey_DelegationURIs, ',');
}

/**
 * The relationship R is
 * "network.proxy.no_proxies_on" is the comma-separated concatenation
 * of the elements of the list "/system/http_proxy/ignore_hosts"
 */
static const char GConfKey_IgnoreHosts[] = "/system/http_proxy/ignore_hosts";
static const char MozKey_IgnoreHosts[] = "network.proxy.no_proxies_on";
static nsresult ApplyIgnoreHosts(nsSystemPrefService* aPrefService,
                                 GConfClient* aClient)
{
    return ApplyListPref(aPrefService, aClient, 
                        GConfKey_IgnoreHosts, MozKey_IgnoreHosts, ',');
}
static nsresult ReverseApplyIgnoreHosts(nsSystemPrefService* aPrefService,
                                        GConfClient* aClient)
{
    return ReverseApplyListPref(aPrefService, aClient, 
                               GConfKey_IgnoreHosts, MozKey_IgnoreHosts, ',');
}

/**
 * The relationship R is
 * ("/system/proxy/mode" is 'manual' if and only if "network.proxy.type" is eProxyConfig_Manual (1))
 * AND ("/system/proxy/mode" is 'auto' if and only if "network.proxy.type" is eProxyConfig_PAC (2))
 * 
 * [This means 'none' matches any value of "network.proxy.type" other than 1 or 2.]
 */
static const char GConfKey_ProxyMode[] = "/system/proxy/mode";
static const char MozKey_ProxyMode[] = "network.proxy.type";
static nsresult ApplyProxyMode(nsSystemPrefService* aPrefService,
                               GConfClient* aClient)
{
    char* str = gconf_client_get_string(aClient, GConfKey_ProxyMode, nsnull);
    if (!str)
        return NS_ERROR_FAILURE;
    PRInt32 val = -1;
    nsCOMPtr<nsIPrefBranch2> prefs =
        aPrefService->GetPrefs()->GetPrefUserBranch();
    prefs->GetIntPref(MozKey_ProxyMode, &val);
    if (val < 0)
        return NS_ERROR_FAILURE;
    if (!strcmp(str, "manual")) {
        val = 1;
    } else if (!strcmp(str, "auto")) {
        val = 2;
    } else if (strcmp(str, "none")) {
        // invalid value for this gconf pref; do nothing
        g_free(str);
        return NS_OK;
    } else {
        if (val == 1 || val == 2) {
            // We need to make it something that 'none' maps to
            val = 0;
        }
    }
    g_free(str);
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_ProxyMode, nsnull);
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaIntPref(MozKey_ProxyMode, val, lock);
    return rv;
}
static nsresult ReverseApplyProxyMode(nsSystemPrefService* aPrefService,
                                      GConfClient* aClient)
{
    PRInt32 val = -1;
    nsCOMPtr<nsIPrefBranch2> prefs =
        aPrefService->GetPrefs()->GetPrefUserBranch();
    prefs->GetIntPref(MozKey_ProxyMode, &val);
    if (val < 0)
        return NS_ERROR_FAILURE;
    const char* str;
    switch (val) {
    case 1: str = "manual"; break;
    case 2: str = "auto"; break;
    default: str = "none"; break;
    }
    gconf_client_set_string(aClient, GConfKey_ProxyMode, str, nsnull);
    return NS_OK;
}

/**
 * The relationship R is
 * If "/apps/firefox/web/download_defaultfolder" is the empty string, then
 * "browser.download.useDownloadDir" is false;
 * otherwise "browser.download.useDownloadDir" is true and "browser.download.folderList"
 * is (0 if "/apps/firefox/web/download_defaultfolder" is "Desktop";
 *     1 if "/apps/firefox/web/download_defaultfolder" is "My Downloads";
 *     3 if "/apps/firefox/web/download_defaultfolder" is "Home";
 *     otherwise 2 and "browser.download.dir" = "/apps/firefox/web/download_defaultfolder")
 */
static const char GConfKey_DownloadFolder[] = "/apps/firefox/web/download_defaultfolder";
static const char MozKey_UseDownloadDir[] = "browser.download.useDownloadDir";
static const char MozKey_DownloadDirType[] = "browser.download.folderList";
static const char MozKey_DownloadDirExplicit[] = "browser.download.dir";
static nsresult ApplyDownloadFolder(nsSystemPrefService* aPrefService,
                                    GConfClient* aClient)
{
    char* str = gconf_client_get_string(aClient, GConfKey_DownloadFolder, nsnull);
    if (!str)
        return NS_ERROR_FAILURE;
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_DownloadFolder, nsnull);
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_UseDownloadDir, *str != 0, lock);
    if (NS_FAILED(rv)) {
        g_free(str);
        return rv;
    }
    PRInt32 dirType = 0;
    if (!strcmp(str, "Desktop")) {
        dirType = 0;
    } else if (!strcmp(str, "My Downloads")) {
        dirType = 1;
    } else if (!strcmp(str, "Home")) {
        dirType = 3;
    } else {
        dirType = 2;
    }
    // Always set all three Mozilla preferences. This is simpler and avoids
    // problems; e.g., if the gconf value changes from "/home/rocallahan" to "Desktop"
    // we might leave MozKey_DownloadDirType accidentally locked.
    rv = aPrefService->GetPrefs()->
        SetOverridingMozillaIntPref(MozKey_DownloadDirType, dirType, lock);
    if (NS_SUCCEEDED(rv)) {
        rv = aPrefService->GetPrefs()->
            SetOverridingMozillaStringPref(MozKey_DownloadDirExplicit, str, lock);
    }
    g_free(str);
    return rv;
}

static nsresult ReverseApplyDownloadFolder(nsSystemPrefService* aPrefService,
                                           GConfClient* aClient)
{
    PRBool useDownloadDir = PR_FALSE;
    const char* result;
    char* explicitStr = nsnull;
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    prefs->GetBoolPref(MozKey_UseDownloadDir, &useDownloadDir);
    if (!useDownloadDir) {
        result = "";
    } else {
        PRInt32 type = -1;
        prefs->GetIntPref(MozKey_DownloadDirType, &type);
        if (type < 0)
            return NS_ERROR_FAILURE;
        switch (type) {
        case 0: result = "Desktop"; break;
        case 1: result = "My Downloads"; break;
        case 2:
            prefs->GetCharPref(MozKey_DownloadDirExplicit, &explicitStr);
            result = explicitStr;
            break;
        case 3: result = "Home"; break;
        default:
            NS_ERROR("Unknown download dir type");
            return NS_ERROR_FAILURE;
        }
    }
    if (!result)
        return NS_ERROR_FAILURE;
    gconf_client_set_string(aClient, GConfKey_DownloadFolder,
                            result, nsnull);
    nsMemory::Free(explicitStr);
    return NS_OK;
}

/**
 * The relationship R is
 * "/apps/firefox/web/disable_cookies" is true if and only if
 * "network.cookie.cookieBehavior" is 2 ('dontUse')
 */
static const char GConfKey_DisableCookies[] = "/apps/firefox/web/disable_cookies";
static const char MozKey_CookieBehavior[] = "network.cookie.cookieBehavior";
static const char MozKey_CookieExceptions[] = "network.cookie.honorExceptions";
static const char MozKey_CookieViewExceptions[] = "pref.privacy.disable_button.cookie_exceptions";
static nsresult ApplyDisableCookies(nsSystemPrefService* aPrefService,
                                    GConfClient* aClient)
{
    gboolean disable = gconf_client_get_bool(aClient, GConfKey_DisableCookies, nsnull);
    PRInt32 behavior = -1;
    nsCOMPtr<nsIPrefBranch2> prefs =
        aPrefService->GetPrefs()->GetPrefUserBranch();
    prefs->GetIntPref(MozKey_CookieBehavior, &behavior);
    if (behavior < 0)
        return NS_ERROR_FAILURE;
    if (disable) {
        behavior = 2;
    } else {
        if (behavior == 2) {
            behavior = 0;
        }
    }
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_DisableCookies, nsnull);
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_CookieExceptions, !lock, lock);
    if (NS_FAILED(rv))
      return rv;
    rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_CookieViewExceptions, lock, lock);
    if (NS_FAILED(rv))
      return rv;
    return aPrefService->GetPrefs()->
        SetOverridingMozillaIntPref(MozKey_CookieBehavior, behavior, lock);
}
static nsresult ReverseApplyDisableCookies(nsSystemPrefService* aPrefService,
                                           GConfClient* aClient)
{
    PRInt32 behavior = -1;
    nsCOMPtr<nsIPrefBranch2> prefs =
        aPrefService->GetPrefs()->GetPrefUserBranch();
    prefs->GetIntPref(MozKey_CookieBehavior, &behavior);
    if (behavior < 0)
        return NS_ERROR_FAILURE;
    gconf_client_set_bool(aClient, GConfKey_DisableCookies, behavior == 2, nsnull);
    return NS_OK;
}

static char const* windowOpenFeatures[] = {
    "dom.disable_window_open_feature.close",
    "dom.disable_window_open_feature.directories",
    "dom.disable_window_open_feature.location",
    "dom.disable_window_open_feature.menubar",
    "dom.disable_window_open_feature.minimizable",
    "dom.disable_window_open_feature.personalbar",
    "dom.disable_window_open_feature.resizable",
    "dom.disable_window_open_feature.scrollbars",
    "dom.disable_window_open_feature.status",
    "dom.disable_window_open_feature.titlebar",
    "dom.disable_window_open_feature.toolbar"
};
/**
 * The relationship R is
 * "/apps/firefox/lockdown/disable_javascript_chrome" is true if and only if
 * all of windowOpenFeatures are true
 */
static const char GConfKey_DisableJSChrome[] =
    "/apps/firefox/lockdown/disable_javascript_chrome";
static nsresult ApplyWindowOpen(nsSystemPrefService* aPrefService,
                                GConfClient* aClient)
{
    gboolean disable = gconf_client_get_bool(aClient, GConfKey_DisableJSChrome, nsnull);
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_DisableJSChrome, nsnull);
    PRBool curValues[NUM_ELEM(windowOpenFeatures)];
    PRUint32 i;
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRBool allDisabled = PR_TRUE;
    for (i = 0; i < NUM_ELEM(windowOpenFeatures); ++i) {
        nsresult rv = prefs->GetBoolPref(windowOpenFeatures[i], &curValues[i]);
        if (NS_FAILED(rv))
            return rv;
        if (!curValues[i]) {
            allDisabled = PR_FALSE;
        }
    }
    for (i = 0; i < NUM_ELEM(windowOpenFeatures); ++i) {
        PRBool newVal = curValues[i];
        if (disable) {
            newVal = PR_TRUE;
        } else if (allDisabled) {
            // If all disable-window-open-feature prefs are currently
            // PR_TRUE, then we need to set at least one of them to
            // PR_FALSE. Set all of them to PR_FALSE.
            newVal = PR_FALSE;
        } // If at least one disable-window-open-feature pref is
          // currently PR_FALSE, then we don't need to change anything
          // when the gconf pref says don't disable
        nsresult rv = aPrefService->GetPrefs()->
            SetOverridingMozillaBoolPref(windowOpenFeatures[i], newVal, lock);
        if (NS_FAILED(rv))
            return rv;
    }
    return NS_OK;
}

static nsresult ReverseApplyWindowOpen(nsSystemPrefService* aPrefService,
                                       GConfClient* aClient)
{
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRBool allDisabled = PR_TRUE;
    PRBool curValues[NUM_ELEM(windowOpenFeatures)];
    for (PRUint32 i = 0; i < NUM_ELEM(windowOpenFeatures); ++i) {
        nsresult rv = prefs->GetBoolPref(windowOpenFeatures[i], &curValues[i]);
        if (NS_FAILED(rv))
            return rv;
        if (!curValues[i]) {
            allDisabled = PR_FALSE;
        }
    }
    gconf_client_set_bool(aClient, GConfKey_DisableJSChrome, allDisabled, nsnull);
    return NS_OK;
}

/**
 * The relationship R is
 * If "/apps/firefox/lockdown/disable_unsafe_protocol" is true then
 * -- "network.protocol-handler.blocked-default" is true
 * -- "network.protocol-handler.blocked.XYZ" is false if and only if
 * XYZ is a builtin non-disablable protocol or in
 * "/apps/firefox/lockdown/additional_safe_protocols"
 * AND if "/apps/firefox/lockdown/disable_unsafe_protocol" is false then
 * -- "network.protocol-handler.blocked-default" is false
 * -- if "network.protocol-handler.blocked.XYZ" exists then it is false
 */
static const char GConfKey_DisableUnsafeProtocols[] =
    "/apps/firefox/lockdown/disable_unsafe_protocol";
static const char GConfKey_AdditionalSafeProtocols[] =
    "/apps/firefox/lockdown/additional_safe_protocols";
static const char MozKey_BlockedDefault[] =
    "network.protocol-handler.blocked-default";
static const char MozKey_BlockedPrefix[] =
    "network.protocol-handler.blocked.";
static const char* nonDisablableBuiltinProtocols[] =
    { "about", "data", "jar", "keyword", "resource", "viewsource",
      "chrome", "moz-icon", "javascript", "file" };
static PRBool FindString(const char** aList, PRInt32 aCount,
                         const char* aStr)
{
    for (PRInt32 i = 0; i < aCount; ++i) {
        if (!strcmp(aStr, aList[i]))
            return PR_TRUE;
    }
    return PR_FALSE;
}
typedef nsDataHashtable<nsCStringHashKey,int> StringSet;
/** Collect the set of protocol names that we want to set preferences for */
static nsresult AddAllProtocols(nsSystemPrefService* aPrefService,
                                const char* aSafeProtocols,
                                StringSet* aProtocolSet,
                                StringSet* aSafeSet)
{
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRUint32 childCount;
    char **childArray = nsnull;
    nsresult rv = prefs->GetChildList(MozKey_BlockedPrefix, &childCount, &childArray);
    if (NS_FAILED(rv))
        return rv;
    PRUint32 i;
    for (i = 0; i < childCount; ++i) {
        nsDependentCString tmp(childArray[i] + NUM_ELEM(MozKey_BlockedPrefix)-1);
        aProtocolSet->Put(tmp, 1); // copies
    }
    for (i = 0; i < NUM_ELEM(nonDisablableBuiltinProtocols); ++i) {
        nsDependentCString tmp(nonDisablableBuiltinProtocols[i]);
        aProtocolSet->Put(tmp, 1);
    }
    i = 0;
    while (aSafeProtocols[i]) {
        const char* nextComma = strchr(aSafeProtocols+i, ',');
        PRUint32 tokLen = nextComma ? nextComma - (aSafeProtocols+i)
            : strlen(aSafeProtocols+i);
        nsCAutoString tok(aSafeProtocols+i, tokLen);
        aProtocolSet->Put(tok, 1);
        aSafeSet->Put(tok, 1);
        if (nextComma) {
            i = nextComma - aSafeProtocols + 1;
        } else {
            break;
        }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(childCount, childArray);
    return NS_OK;
}

struct ProtocolPrefClosure {
    StringSet safeProtocolSet;
    nsIPrefBranch2* prefs;
    nsISystemPref* prefSetter;
    PRPackedBool disableUnsafe;
    PRPackedBool lock;
};

static PLDHashOperator PR_CALLBACK SetProtocolPref(const nsACString& aKey,
                                                   int aItem,
                                                   void* aClosure)
{
    ProtocolPrefClosure* closure = static_cast<ProtocolPrefClosure*>(aClosure);
    const nsCString& protocol = PromiseFlatCString(aKey);
    PRBool blockProtocol = PR_FALSE;
    if (closure->disableUnsafe &&
        !FindString(nonDisablableBuiltinProtocols,
                    NUM_ELEM(nonDisablableBuiltinProtocols), protocol.get()) &&
        !closure->safeProtocolSet.Get(aKey, nsnull)) {
        blockProtocol = PR_TRUE;
    }

    nsCAutoString prefName;
    prefName.Append(MozKey_BlockedPrefix);
    prefName.Append(protocol);
    closure->prefSetter->SetOverridingMozillaBoolPref(prefName.get(), blockProtocol,
                                                      closure->lock);
    return PL_DHASH_NEXT;
}
static nsresult ApplyUnsafeProtocols(nsSystemPrefService* aPrefService,
                                     GConfClient* aClient)
{
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_DisableUnsafeProtocols, nsnull)
        || !gconf_client_key_is_writable(aClient, GConfKey_AdditionalSafeProtocols, nsnull);
    gboolean disable = gconf_client_get_bool(aClient, GConfKey_DisableUnsafeProtocols, nsnull);
    char* protocols = gconf_client_get_string(aClient, GConfKey_AdditionalSafeProtocols, nsnull);
    if (!protocols)
        return NS_ERROR_FAILURE;
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_BlockedDefault, disable, lock);
    StringSet protocolSet;
    ProtocolPrefClosure closure;
    protocolSet.Init();
    closure.safeProtocolSet.Init();
    if (NS_SUCCEEDED(rv)) {
        rv = AddAllProtocols(aPrefService, protocols, &protocolSet,
                             &closure.safeProtocolSet);
    }
    if (NS_SUCCEEDED(rv)) {
        closure.disableUnsafe = disable;
        closure.lock = lock;
        closure.prefSetter = aPrefService->GetPrefs();
        nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
        closure.prefs = prefs;
        protocolSet.EnumerateRead(SetProtocolPref, &closure);
    }
    g_free(protocols);
    return rv;
}

static nsresult ReverseApplyUnsafeProtocols(nsSystemPrefService* aPrefService,
                                            GConfClient* aClient)
{
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRBool blockedDefault;
    nsresult rv = prefs->GetBoolPref(MozKey_BlockedDefault, &blockedDefault);
    if (NS_FAILED(rv))
        return rv;
    nsCAutoString enabledProtocols;
    PRUint32 childCount;
    char **childArray = nsnull;
    rv = prefs->GetChildList(MozKey_BlockedPrefix, &childCount, &childArray);
    if (NS_FAILED(rv))
        return rv;
    for (PRUint32 i = 0; i < childCount; ++i) {
        PRBool val = PR_FALSE;
        prefs->GetBoolPref(childArray[i], &val);
        if (val) {
            if (enabledProtocols.Length() > 0) {
                enabledProtocols.Append(',');
            }
            enabledProtocols.Append(childArray[i] + NUM_ELEM(MozKey_BlockedPrefix)-1);
        }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(childCount, childArray);
    gconf_client_set_bool(aClient, GConfKey_DisableUnsafeProtocols, blockedDefault, nsnull);
    gconf_client_set_string(aClient, GConfKey_AdditionalSafeProtocols,
                            enabledProtocols.get(), nsnull);
    return NS_OK;
}

/**
 * Set config.lockdown.setwallpaper if and only if
 * /desktop/gnome/background/picture_filename is write-only. Always
 * lock it.
 */
static const char MozKey_LockdownWallpaper[] = "config.lockdown.setwallpaper";
static const char GConfKey_WallpaperSetting[] =
    "/desktop/gnome/background/picture_filename";
static nsresult ApplyWallpaper(nsSystemPrefService* aPrefService,
                               GConfClient* aClient)
{
    PRBool canSetWallpaper =
        gconf_client_key_is_writable(aClient, GConfKey_WallpaperSetting, nsnull);
    return aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_LockdownWallpaper,
                                     !canSetWallpaper, PR_TRUE);
}
// No ReverseApplyWallpaper because this Mozilla pref can never be
// modified

/**
 * The relationship R is 
 * "signon.rememberSignons" is true if and only if "/apps/firefox/web/disable_save_password"
 * is false.
 */
static const char MozKey_RememberSignons[] = "signon.rememberSignons";
static const char GConfKey_DisableSavePassword[] = "/apps/firefox/web/disable_save_password";
static nsresult ApplyDisableSavePassword(nsSystemPrefService* aPrefService,
                                         GConfClient* aClient)
{
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_DisableSavePassword, nsnull);
    gboolean disable = gconf_client_get_bool(aClient, GConfKey_DisableSavePassword, nsnull);
    return aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_RememberSignons, !disable, lock);
}

static nsresult ReverseApplyDisableSavePassword(nsSystemPrefService* aPrefService,
                                                GConfClient* aClient)
{
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRBool remember;
    nsresult rv = prefs->GetBoolPref(MozKey_RememberSignons, &remember);
    if (NS_FAILED(rv))
        return rv;
    gconf_client_set_bool(aClient, GConfKey_DisableSavePassword, !remember, nsnull);
    return NS_OK;
}

/**
 * The relationship R is
 * "permissions.default.image" is 1 (nsIPermissionManager::ALLOW_ACTION) if and only if
 * "/apps/firefox/web/images_load" is 0, AND
 * "permissions.default.image" is 2 (nsIPermissionManager::DENY_ACTION) if and only if
 * "/apps/firefox/web/images_load" is 2, AND
 * "permissions.default.image" is 3 if and only if "/apps/firefox/web/images_load" is 1
 *
 * Also, we set pref.advanced.images.disable_button.view_image iff
 * /apps/firefox/web/images_load is read-only
 * And we set permissions.default.honorExceptions iff
 * /apps/firefox/web/images_load is not read-only
 */
static const char MozKey_ImagePermissions[] = "permissions.default.image";
static const char MozKey_ImageExceptions[] = "permissions.honorExceptions.image";
static const char MozKey_ImageViewExceptions[] = "pref.advanced.images.disable_button.view_image";
static const char GConfKey_LoadImages[] = "/apps/firefox/web/images_load";
static nsresult ApplyLoadImages(nsSystemPrefService* aPrefService,
                                 GConfClient* aClient)
{
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_LoadImages, nsnull);
    // 0 == accept, 1 == no-foreign, 2 == reject
    gint setting = gconf_client_get_int(aClient, GConfKey_LoadImages, nsnull);
    PRInt32 pref;
    switch (setting) {
      case 0: pref = nsIPermissionManager::ALLOW_ACTION; break;
      case 2: pref = nsIPermissionManager::DENY_ACTION; break;
      case 1: pref = 3; break;
      default: return NS_ERROR_FAILURE;
    }
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_ImageExceptions, !lock, lock);
    if (NS_FAILED(rv))
      return rv;
    rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_ImageViewExceptions, lock, lock);
    if (NS_FAILED(rv))
      return rv;
    return aPrefService->GetPrefs()->
        SetOverridingMozillaIntPref(MozKey_ImagePermissions, pref, lock);
}

static nsresult ReverseApplyLoadImages(nsSystemPrefService* aPrefService,
                                        GConfClient* aClient)
{
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRInt32 pref;
    nsresult rv = prefs->GetIntPref(MozKey_ImagePermissions, &pref);
    if (NS_FAILED(rv))
        return rv;
    gint setting;
    switch (pref) {
      case nsIPermissionManager::ALLOW_ACTION: setting = 0; break;
      case nsIPermissionManager::DENY_ACTION: setting = 2; break;
      case 3: setting = 1; break;
      default: return NS_ERROR_FAILURE;
    }
    gconf_client_set_int(aClient, GConfKey_LoadImages, setting, nsnull);
    return NS_OK;
}

/**
 * The relationship R is 
 * "/apps/firefox/web/disable_popups" is true if and only if
 * "dom.disable_open_during_load" is true
 * AND if "/apps/firefox/web/disable_popups" is true then
 * "privacy.popups.showBrowserMessage" is false.
 */
static const char MozKey_DisablePopups[] = "dom.disable_open_during_load";
static const char MozKey_DisableBrowserPopupMessage[] = "privacy.popups.showBrowserMessage";
static const char GConfKey_DisablePopups[] = "/apps/firefox/web/disable_popups";
static nsresult ApplyDisablePopups(nsSystemPrefService* aPrefService,
                                   GConfClient* aClient)
{
    PRBool lock = !gconf_client_key_is_writable(aClient, GConfKey_DisablePopups, nsnull);
    gboolean disable = gconf_client_get_bool(aClient, GConfKey_DisablePopups, nsnull);
    nsresult rv = aPrefService->GetPrefs()->
        SetOverridingMozillaBoolPref(MozKey_DisablePopups, disable, lock);
    if (NS_SUCCEEDED(rv)) {
        if (disable) {
            rv = aPrefService->GetPrefs()->
                SetOverridingMozillaBoolPref(MozKey_DisableBrowserPopupMessage, PR_TRUE, lock);
        } else {
            rv = aPrefService->GetPrefs()->
                StopOverridingMozillaPref(MozKey_DisableBrowserPopupMessage);
        }
    }
    return rv;
}

static nsresult ReverseApplyDisablePopups(nsSystemPrefService* aPrefService,
                                          GConfClient* aClient)
{
    nsCOMPtr<nsIPrefBranch2> prefs = aPrefService->GetPrefs()->GetPrefUserBranch();
    PRBool disabled;
    nsresult rv = prefs->GetBoolPref(MozKey_DisablePopups, &disabled);
    if (NS_FAILED(rv))
        return rv;
    gconf_client_set_bool(aClient, GConfKey_DisablePopups, disabled, nsnull);
    return NS_OK;
}

static ComplexGConfPrefMapping sComplexGConfPrefMappings[] = {
    {GConfKey_TrustedURIs, ApplyTrustedURIs},
    {GConfKey_DelegationURIs, ApplyDelegationURIs},
    {GConfKey_IgnoreHosts, ApplyIgnoreHosts},
    {GConfKey_ProxyMode, ApplyProxyMode},
    {GConfKey_DownloadFolder, ApplyDownloadFolder},
    {GConfKey_DisableCookies, ApplyDisableCookies},
    {GConfKey_DisableJSChrome, ApplyWindowOpen},
    {GConfKey_DisableUnsafeProtocols, ApplyUnsafeProtocols},
    {GConfKey_AdditionalSafeProtocols, ApplyUnsafeProtocols},
    {GConfKey_WallpaperSetting, ApplyWallpaper},
    {GConfKey_DisableSavePassword, ApplyDisableSavePassword},
    {GConfKey_LoadImages, ApplyLoadImages},
    {GConfKey_DisablePopups, ApplyDisablePopups}
};
static ComplexMozPrefMapping sComplexMozPrefMappings[] = {
    {MozKey_TrustedURIs, ReverseApplyTrustedURIs},
    {MozKey_DelegationURIs, ReverseApplyDelegationURIs},
    {MozKey_IgnoreHosts, ReverseApplyIgnoreHosts},
    {MozKey_ProxyMode, ReverseApplyProxyMode},
    {MozKey_UseDownloadDir, ReverseApplyDownloadFolder},
    {MozKey_DownloadDirType, ReverseApplyDownloadFolder},
    {MozKey_DownloadDirExplicit, ReverseApplyDownloadFolder},
    {MozKey_CookieBehavior, ReverseApplyDisableCookies},
    {MozKey_RememberSignons, ReverseApplyDisableSavePassword},
    {MozKey_ImagePermissions, ReverseApplyLoadImages},
    {MozKey_DisablePopups, ReverseApplyDisablePopups}
};
// The unsafe protocol preferences are handled specially because
// they affect an unknown number of Mozilla preferences
// Window opener permissions are also handled specially so we don't have to
// repeat the windowOpenFeatures list.

static PR_CALLBACK void GConfSimpleNotification(GConfClient* client,
                                                guint cnxn_id,
                                                GConfEntry *entry,
                                                gpointer user_data)
{
    nsSystemPrefService* service = static_cast<nsSystemPrefService*>(user_data);
    SimplePrefMapping* map = static_cast<SimplePrefMapping*>(
                                            service->GetSimpleCallbackData(cnxn_id));
    NS_ASSERTION(map, "Can't find mapping for callback");
    if (!map)
        return;

    ApplySimpleMapping(map, service->GetPrefs(), client);
}

static PR_CALLBACK void GConfComplexNotification(GConfClient* client,
                                                 guint cnxn_id,
                                                 GConfEntry *entry,
                                                 gpointer user_data)
{
    nsSystemPrefService* service = static_cast<nsSystemPrefService*>(user_data);
    ComplexGConfPrefMapping* map = static_cast<ComplexGConfPrefMapping*>(
                                                  service->GetComplexCallbackData(cnxn_id));
    NS_ASSERTION(map, "Can't find mapping for callback");
    if (!map)
        return;

    map->callback(service, GetGConf());
}

static const char GConfKey_Homepage_Url[] =
    "/apps/firefox/general/homepage_url";

nsresult nsSystemPrefService::LoadSystemPreferences(nsISystemPref* aPrefs)
{
    mPref = aPrefs;

    GConfClient* client = GetGConf();
    PRUint32 i;
    nsCOMPtr<nsIPrefBranch2> userPrefs = aPrefs->GetPrefUserBranch();

    // Update gconf settings with any Mozilla settings that have
    // changed from the default. Do it before we register our
    // gconf notifications.
    for (i = 0; i < NUM_ELEM(sSimplePrefMappings); ++i) {
        gconf_client_add_dir(client, sSimplePrefMappings[i].gconfPrefName,
                             GCONF_CLIENT_PRELOAD_NONE, nsnull);

        PRBool hasUserPref = PR_FALSE;
        nsresult rv =
            userPrefs->PrefHasUserValue(sSimplePrefMappings[i].mozPrefName,
                                        &hasUserPref);
        if (NS_FAILED(rv))
            return rv;
        if (hasUserPref && sSimplePrefMappings[i].allowWritesFromMozilla &&
            gconf_client_key_is_writable(client,
                                         sSimplePrefMappings[i].gconfPrefName,
                                         nsnull)) {
            rv = ReverseApplySimpleMapping(&sSimplePrefMappings[i],
                                           aPrefs, client);
            if (NS_FAILED(rv))
                return rv;
        }
    }
    for (i = 0; i < NUM_ELEM(sComplexGConfPrefMappings); ++i) {
        gconf_client_add_dir(client, sComplexGConfPrefMappings[i].gconfPrefName,
                             GCONF_CLIENT_PRELOAD_NONE, nsnull);
    }
    ComplexMozPrefChanged lastMozCallback = nsnull;
    for (i = 0; i < NUM_ELEM(sComplexMozPrefMappings); ++i) {
        PRBool hasUserPref = PR_FALSE;
        nsresult rv =
            userPrefs->PrefHasUserValue(sComplexMozPrefMappings[i].mozPrefName,
                                        &hasUserPref);
        if (NS_FAILED(rv))
            return rv;
        if (hasUserPref) {
            ComplexMozPrefChanged cb = sComplexMozPrefMappings[i].callback;
            if (cb != lastMozCallback) {
                cb(this, client);
                lastMozCallback = cb;
            }
        }
    }
 
    // Register simple mappings and callbacks
    for (i = 0; i < NUM_ELEM(sSimplePrefMappings); ++i) {
        guint cx = gconf_client_notify_add(client,
                                           sSimplePrefMappings[i].gconfPrefName,
                                           GConfSimpleNotification, this,
                                           nsnull, nsnull);
        mGConfSimpleCallbacks.Put(cx, &sSimplePrefMappings[i]);
        nsresult rv = ApplySimpleMapping(&sSimplePrefMappings[i], aPrefs, client);
        if (NS_FAILED(rv))
            return rv;
    }

    ComplexGConfPrefChanged lastCallback = nsnull;
    for (i = 0; i < NUM_ELEM(sComplexGConfPrefMappings); ++i) {
        guint cx = gconf_client_notify_add(client,
                                           sComplexGConfPrefMappings[i].gconfPrefName,
                                           GConfComplexNotification, this,
                                           nsnull, nsnull);
        mGConfComplexCallbacks.Put(cx, &sComplexGConfPrefMappings[i]);
        ComplexGConfPrefChanged cb = sComplexGConfPrefMappings[i].callback;
        if (cb != lastCallback) {
            cb(this, client);
            lastCallback = cb;
        }
    }

    if (!gconf_client_get(client, GConfKey_DisablePopups, nsnull))
    {
        gconf_client_set_bool(client, GConfKey_DisablePopups, true, nsnull);
    }

    if (!gconf_client_get(client, GConfKey_Homepage_Url, nsnull))
    {
        gconf_client_set_string(client, GConfKey_Homepage_Url, "", nsnull);
    }

    ApplyUnsafeProtocols(this, client);

    return NS_OK;
}

nsresult nsSystemPrefService::NotifyMozillaPrefChanged(const char* aPrefName)
{
    PRUint32 i;
    GConfClient* client = GetGConf();

    for (i = 0; i < NUM_ELEM(sSimplePrefMappings); ++i) {
        if (!strcmp(aPrefName, sSimplePrefMappings[i].mozPrefName)) {
            ReverseApplySimpleMapping(&sSimplePrefMappings[i],
                                      mPref, client);
        }
    }

    for (i = 0; i < NUM_ELEM(sComplexMozPrefMappings); ++i) {
        if (!strcmp(aPrefName, sComplexMozPrefMappings[i].mozPrefName)) {
            sComplexMozPrefMappings[i].callback(this, client);
        }
    }

    for (i = 0; i < NUM_ELEM(windowOpenFeatures); ++i) {
        if (!strcmp(aPrefName, windowOpenFeatures[i])) {
            ReverseApplyWindowOpen(this, client);
        }
    }

    ReverseApplyUnsafeProtocols(this, client);

    return NS_OK;
}

static PLDHashOperator PR_CALLBACK UnregisterSimple(const PRUint32& aKey,
                                                    SimplePrefMapping* aData,
                                                    void* aClosure)
{
    GConfClient* client = GetGConf();
    gconf_client_notify_remove(client, aKey);
    gconf_client_remove_dir(client, aData->gconfPrefName, nsnull);
    return PL_DHASH_NEXT;
}

static PLDHashOperator PR_CALLBACK UnregisterComplex(const PRUint32& aKey,
                                                     ComplexGConfPrefMapping* aData,
                                                     void* aClosure)
{
    GConfClient* client = GetGConf();
    gconf_client_notify_remove(client, aKey);
    gconf_client_remove_dir(client, aData->gconfPrefName, nsnull);
    return PL_DHASH_NEXT;
}

nsresult nsSystemPrefService::NotifyUnloadSystemPreferences()
{
    // Unregister callbacks
    mGConfSimpleCallbacks.EnumerateRead(UnregisterSimple, this);
    mGConfSimpleCallbacks.Clear();
    mGConfComplexCallbacks.EnumerateRead(UnregisterComplex, this);
    mGConfComplexCallbacks.Clear();

    return NS_OK;
}

// Factory stuff

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemPrefService, Init)

static const nsModuleComponentInfo components[] = {
    { NS_SYSTEMPREF_SERVICE_CLASSNAME,
      NS_SYSTEMPREF_SERVICE_CID,
      NS_SYSTEMPREF_SERVICE_CONTRACTID,
      nsSystemPrefServiceConstructor,
    },
};

NS_IMPL_NSGETMODULE(nsSystemPrefServiceModule, components)
