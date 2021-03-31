#!/bin/bash

if [ "$1" == "" ] || [ "$2" == "" ]
then
    echo ussage: build_release.sh APPNAME VERSION
    exit 1
fi

cd ..
tar cvjf $1-$2-source.tar.bz2 --exclude=.git --exclude=.cvsignore --exclude=CVS  mozilla
