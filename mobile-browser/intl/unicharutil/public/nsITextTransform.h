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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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
#ifndef nsITextTransform_h__
#define nsITextTransform_h__


#include "nsISupports.h"
#include "nsString.h"
#include "nscore.h"

// {CCD4D371-CCDC-11d2-B3B1-00805F8A6670}
#define NS_ITEXTTRANSFORM_IID \
{ 0xccd4d371, 0xccdc, 0x11d2, \
    { 0xb3, 0xb1, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

#define NS_TEXTTRANSFORM_CONTRACTID_BASE "@mozilla.org/intl/texttransform;1?type="

class nsITextTransform : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEXTTRANSFORM_IID)

  NS_IMETHOD Change( const PRUnichar* aText, PRInt32 aTextLength, nsString& aResult) = 0;
  NS_IMETHOD Change( nsString& aText, nsString& aResult) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITextTransform, NS_ITEXTTRANSFORM_IID)

#endif  /* nsITextTransform_h__ */
