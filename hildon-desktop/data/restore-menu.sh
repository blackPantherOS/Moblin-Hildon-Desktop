#!/bin/sh

OSSO_CONF_DIR="${HOME}/.osso"
MENU_CONF_DIR="${OSSO_CONF_DIR}/menus"
OSSO_SOFTWARE_VERSION_FILE="/etc/osso_software_version"

if [ ! $2 ]
then
  exit 0
fi

if [ ! -e $OSSO_SOFTWARE_VERSION_FILE ]
then
  exit 0
fi

OSSO_VERSION=`cat ${OSSO_SOFTWARE_VERSION_FILE}`

if [ $2 != $OSSO_VERSION ]
then
  rm -f "${MENU_CONF_DIR}/applications.menu"
fi
