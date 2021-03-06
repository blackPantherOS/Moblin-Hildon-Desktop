/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is nsCacheSession.h, released
 * February 23, 2001.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Gordon Sheridan <gordon@netscape.com>
 *   Patrick Beard   <beard@netscape.com>
 *   Darin Fisher    <darin@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsCacheSession.h"
#include "nsCacheService.h"
#include "nsCRT.h"

NS_IMPL_ADDREF(nsCacheSession)
NS_IMPL_RELEASE(nsCacheSession)

NS_INTERFACE_MAP_BEGIN(nsCacheSession)
    NS_INTERFACE_MAP_ENTRY(nsICacheSession)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(
        nsIOfflineCacheSession, (StoragePolicy() == nsICache::STORE_OFFLINE))
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICacheSession)
NS_INTERFACE_MAP_END

nsCacheSession::nsCacheSession(const char *         clientID,
                               nsCacheStoragePolicy storagePolicy,
                               PRBool               streamBased)
    : mClientID(clientID),
      mInfo(0)
{
  SetStoragePolicy(storagePolicy);

  if (streamBased) MarkStreamBased();
  else SetStoragePolicy(nsICache::STORE_IN_MEMORY);

  MarkDoomEntriesIfExpired();
}

nsCacheSession::~nsCacheSession()
{
  /* destructor code */
    // notify service we are going away?
}


NS_IMETHODIMP nsCacheSession::GetDoomEntriesIfExpired(PRBool *result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = WillDoomEntriesIfExpired();
    return NS_OK;
}


NS_IMETHODIMP nsCacheSession::SetDoomEntriesIfExpired(PRBool doomEntriesIfExpired)
{
    if (doomEntriesIfExpired)  MarkDoomEntriesIfExpired();
    else                       ClearDoomEntriesIfExpired();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheSession::OpenCacheEntry(const nsACString &         key, 
                               nsCacheAccessMode          accessRequested,
                               PRBool                     blockingMode,
                               nsICacheEntryDescriptor ** result)
{
    nsresult rv;
    rv =  nsCacheService::OpenCacheEntry(this,
                                         key,
                                         accessRequested,
                                         blockingMode,
                                         nsnull, // no listener
                                         result);
    return rv;
}


NS_IMETHODIMP nsCacheSession::AsyncOpenCacheEntry(const nsACString & key,
                                                  nsCacheAccessMode accessRequested,
                                                  nsICacheListener *listener)
{
    nsresult rv;
    rv = nsCacheService::OpenCacheEntry(this,
                                        key,
                                        accessRequested,
                                        nsICache::BLOCKING,
                                        listener,
                                        nsnull); // no result

    if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) rv = NS_OK;
    return rv;
}

NS_IMETHODIMP nsCacheSession::EvictEntries()
{
    return nsCacheService::EvictEntriesForSession(this);
}


NS_IMETHODIMP nsCacheSession::IsStorageEnabled(PRBool *result)
{

    return nsCacheService::IsStorageEnabledForPolicy(StoragePolicy(), result);
}

NS_IMETHODIMP nsCacheSession::GetOwnerDomains(PRUint32 * count,
                                              char *** domains)
{
    return nsCacheService::GetOfflineOwnerDomains(this, count, domains);
}

NS_IMETHODIMP nsCacheSession::GetOwnerURIs(const nsACString & domain,
                                           PRUint32 * count,
                                           char *** uris)
{
    return nsCacheService::GetOfflineOwnerURIs(this, domain, count, uris);
}

NS_IMETHODIMP nsCacheSession::SetOwnedKeys(const nsACString & domain,
                                           const nsACString & uri,
                                           PRUint32 count,
                                           const char ** keys)
{
    return nsCacheService::SetOfflineOwnedKeys(this, domain, uri, count, keys);
}

NS_IMETHODIMP nsCacheSession::GetOwnedKeys(const nsACString & domain,
                                           const nsACString & uri,
                                           PRUint32 * count,
                                           char *** keys)
{
    return nsCacheService::GetOfflineOwnedKeys(this, domain, uri, count, keys);
}

NS_IMETHODIMP nsCacheSession::AddOwnedKey(const nsACString & domain,
                                          const nsACString & uri,
                                          const nsACString & key)
{
    return nsCacheService::AddOfflineOwnedKey(this, domain, uri, key);
}

NS_IMETHODIMP nsCacheSession::RemoveOwnedKey(const nsACString & domain,
                                             const nsACString & uri,
                                             const nsACString & key)
{
    return nsCacheService::RemoveOfflineOwnedKey(this, domain, uri, key);
}

NS_IMETHODIMP nsCacheSession::KeyIsOwned(const nsACString & domain,
                                         const nsACString & uri,
                                         const nsACString & key,
                                         PRBool * isOwned)
{
    return nsCacheService::OfflineKeyIsOwned(this, domain, uri, key, isOwned);
}

NS_IMETHODIMP nsCacheSession::ClearKeysOwnedByDomain(const nsACString & domain)
{
    return nsCacheService::ClearOfflineKeysOwnedByDomain(this, domain);
}

NS_IMETHODIMP nsCacheSession::EvictUnownedEntries()
{
    return nsCacheService::EvictUnownedOfflineEntries(this);
}

NS_IMETHODIMP nsCacheSession::MergeTemporaryClientID(const nsACString& fromClientID)
{
    return nsCacheService::MergeTemporaryClientID(this, fromClientID);
}
