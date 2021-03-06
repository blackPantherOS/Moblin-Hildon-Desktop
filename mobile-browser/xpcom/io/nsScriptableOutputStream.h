/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef ___nsscriptableoutputstream___h_
#define ___nsscriptableoutputstream___h_

#include "nsIScriptableStreams.h"
#include "nsIOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIUnicharOutputStream.h"
#include "nsCOMPtr.h"

#define NS_SCRIPTABLEOUTPUTSTREAM_CID        \
{ 0xaea1cfe2, 0xf727, 0x4b94, { 0x93, 0xff, 0x41, 0x8d, 0x96, 0x87, 0x94, 0xd1 } }

#define NS_SCRIPTABLEOUTPUTSTREAM_CONTRACTID "@mozilla.org/scriptableoutputstream;1"
#define NS_SCRIPTABLEOUTPUTSTREAM_CLASSNAME "Scriptable Output Stream"

class nsScriptableOutputStream : public nsIScriptableIOOutputStream,
                                 public nsISeekableStream,
                                 public nsIOutputStream
{
public:
  // nsISupports methods
  NS_DECL_ISUPPORTS

  // nsIOutputStream methods
  NS_DECL_NSIOUTPUTSTREAM

  // nsIScriptableIOOutputStream methods
  NS_DECL_NSISCRIPTABLEIOOUTPUTSTREAM

  // nsISeekableStream methods
  NS_DECL_NSISEEKABLESTREAM

  // nsScriptableOutputStream methods
  nsScriptableOutputStream() {}

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
  ~nsScriptableOutputStream() {}

  nsresult WriteFully(const char *aBuf, PRUint32 aCount);

  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsCOMPtr<nsIUnicharOutputStream> mUnicharOutputStream;
};

#endif // ___nsscriptableoutputstream___h_
