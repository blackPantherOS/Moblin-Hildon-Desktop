/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Feed Content Sniffer.
 *
 * The Initial Developer of the Original Code is Canonical Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Carl Wong <carl.wong@intel.com>
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

#include <gtk/gtkwindow.h>

#include "nsHildonXWindowService.h"

static nsVoidArray* mWidgetList;

NS_IMPL_ISUPPORTS1(nsHildonXWindowService, nsIHildonXWindowService)


nsHildonXWindowService::nsHildonXWindowService()
{
  if (!mWidgetList)
    mWidgetList = new nsVoidArray();
}

nsHildonXWindowService::~nsHildonXWindowService()
{
  if (mWidgetList)
    delete mWidgetList;
}

nsresult
nsHildonXWindowService::Init()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHildonXWindowService::AddWidget (PRInt32 w)
{
  mWidgetList->AppendElement((void *) w);
  return NS_OK;
}


NS_IMETHODIMP
nsHildonXWindowService::RemoveWidget (PRInt32 w)
{
  mWidgetList->RemoveElement((void *)w);
  return NS_OK;
}

NS_IMETHODIMP
nsHildonXWindowService::RaiseWindows ()
{
  for (int i = 0; i < mWidgetList->Count(); i++)
  {
    GtkWindow *window = (GtkWindow*) mWidgetList->ElementAt (i);
    if (!window)
      continue;

    // bring the window to the foreground
    gtk_window_present (window);
  }

  return NS_OK;
}
