/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla.org code.
 *
 * The Initial Developer of the Original Code is Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking <jonas@sicking.cc> (Original Author)
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

#include "nsTObserverArray.h"

void
nsTObserverArray_base::AdjustIterators(PRInt32 aModPos,
                                       PRInt32 aAdjustment)
{
  NS_PRECONDITION(aAdjustment == -1 || aAdjustment == 1,
                  "invalid adjustment");
  Iterator_base* iter = mIterators;
  while (iter) {
    NS_ASSERTION(&(iter->mArray) == this, "wrong array");

    if (iter->mPosition > aModPos) {
      iter->mPosition += aAdjustment;
    }
    iter = iter->mNext;
  }
}

void
nsTObserverArray_base::Clear()
{
  mObservers.Clear();

  Iterator_base* iter = mIterators;
  while (iter) {
    NS_ASSERTION(&(iter->mArray) == this, "wrong array");

    iter->mPosition = 0;
    iter = iter->mNext;
  }
}
