%define gettext_package gnome-games

%define glib2_version 2.2.0
%define pango_version 1.2.0
%define gtk2_version 2.2.0
%define libgnomeui_version 2.2.0
%define desktop_file_utils_version 0.2.90

%define localstatedir /var/lib

Summary: GNOME games.
Name: gnome-games
Version: 0.1
Release: 1
Epoch: 1
Copyright: GPL
Group: Amusements/Games
Source:	ftp://ftp.gnome.org/pub/GNOME/sources/%{name}/%{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Obsoletes: gnome-games-devel
URL: http://www.gnome.org

Prereq:         scrollkeeper >= 0.1.4
Prereq:         GConf2
Prereq:         /usr/bin/gconftool-2

BuildRequires:  glib2-devel >= %{glib2_version}
BuildRequires:  pango-devel >= %{pango_version}
BuildRequires:  gtk2-devel >= %{gtk2_version}
BuildRequires:  libgnomeui-devel >= %{libgnomeui_version}
# BuildRequires:  desktop-file-utils >= %{desktop_file_utils_version}
BuildRequires:  guile-devel
BuildRequires: expat-devel

%description

Gnome-games is a collection of small games that take advantage of the 
GNOME desktop environment. It contains a mixture of puzzle games, card
games, and some arcade favorites. You must have the GNOME libraries 
installed to play these games but they will work with any desktop 
environment.

%prep
%setup -q

%build

%configure --localstatedir=%{localstatedir}  --disable-schemas-install --disable-setgid
make

%install
rm -rf $RPM_BUILD_ROOT

# configure option should take care of this
#export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
%makeinstall localstatedir=$RPM_BUILD_ROOT%{localstatedir}
#unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

## things we just don't want in the package
rm -rf $RPM_BUILD_ROOT%{_libdir}/libgdkcardimage.*a
rm -rf $RPM_BUILD_ROOT%{localstatedir}/scrollkeeper

# install desktop files
#  need a fix for bug 114322 for this to work
#desktop-file-install --vendor gnome --delete-original       \
#  --dir $RPM_BUILD_ROOT%{_datadir}/applications             \
#  --add-category X-Red-Hat-Base                             \
#  $RPM_BUILD_ROOT%{_datadir}/applications/*
# In the meantime the following works
#perl -pi -e 's@^(Categories=.*)$@$1;X-Red-Hat-Base@g' $RPM_BUILD_ROOT%{_datadir}/applications/*

%find_lang %{gettext_package}

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig
scrollkeeper-update

export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
SCHEMAS="aisleriot.schemas blackjack.schemas glines.schemas gnect.schemas
gnibbles.schemas gnobots2.schemas gnometris.schemas gnomine.schemas 
gnotravex.schemas gtali.schemas iagno.schemas mahjongg.schemas same-gnome.schemas"
for S in $SCHEMAS; do
  gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/$S > /dev/null
done

%postun
/sbin/ldconfig
scrollkeeper-update

%files -f %{gettext_package}.lang
%defattr(-, root, root)

%doc AUTHORS COPYING ChangeLog

%{_datadir}/gnome
%{_datadir}/applications
%{_datadir}/gnect
%{_datadir}/pixmaps
%{_datadir}/gnobots2
%{_datadir}/sounds
%{_datadir}/mime-info
%{_datadir}/gnibbles
%{_datadir}/sol-games
%{_datadir}/omf
%{_datadir}/blackjack
%{_libdir}/*.so.*
%{_sysconfdir}/gconf/schemas/*
%config %{_sysconfdir}/sound/events/*
%config(noreplace) %attr(664, games, games) %{localstatedir}/games/*

# could be a devel package
%{_includedir}/gdkcardimage
%{_libdir}/*.so

# these are not setgid games
%defattr (-, root,games)
%{_bindir}/gnect
%{_bindir}/blackjack
%{_bindir}/gnome-sudoku
%{_bindir}/glchess
%{_bindir}/sol

# these are setgid games
%defattr (2551, root, games)
%{_bindir}/gnomine
%{_bindir}/same-gnome
%{_bindir}/mahjongg
%{_bindir}/gtali
%{_bindir}/gnobots2
%{_bindir}/gnometris
%{_bindir}/gnotravex
%{_bindir}/gnotski
%{_bindir}/gnibbles
%{_bindir}/glines
%{_bindir}/iagno

%changelog
* Fri Jun 27 2003 Callum McKenzie <callum@physics.otago.ac.nz>
- Fixups to remove some Red Hat specifics.

* Thu Jun 26 2003 William Jon McCann <mccann@jhu.edu>
- Synch vendor and CVS versions

* Wed Jun 04 2003 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Mon Feb 24 2003 Elliot Lee <sopwith@redhat.com>
- debuginfo rebuild

* Sun Feb 23 2003 Jeremy Katz <katzj@redhat.com> 1:2.2.0-2
- apply the gnect schemas (#84905)
- gnome-stones shouldn't crash on startup (#84904)

* Wed Feb  5 2003 Havoc Pennington <hp@redhat.com> 1:2.2.0-1
- 2.2.0

* Wed Jan 22 2003 Tim Powers <timp@redhat.com>
- rebuilt

* Thu Jan  9 2003 Havoc Pennington <hp@redhat.com>
- 2.1.5

* Wed Dec  4 2002 Havoc Pennington <hp@redhat.com>
- munge empty string default value out of gtali.schemas
- increment version numbers on requirements

* Tue Dec  3 2002 Bill Nottingham <notting@redhat.com> 1:2.1.3-2
- rebuild against new guile

* Mon Dec  2 2002 Tim Powers <timp@redhat.com> 1:2.1.3-1
- update to 2.1.3

* Tue Aug 13 2002 Havoc Pennington <hp@redhat.com>
- add some OnlyShowIn

* Mon Aug 12 2002 Havoc Pennington <hp@redhat.com>
- 2.0.3 from gnome 2.0.1

* Tue Aug  6 2002 Havoc Pennington <hp@redhat.com>
- 2.0.2

* Tue Jul 23 2002 Havoc Pennington <hp@redhat.com>
- gnect doesn't like being setgid games
- obsolete gnome-games-devel

* Fri Jul 12 2002 Havoc Pennington <hp@redhat.com>
- add gnect

* Fri Jun 21 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Sun Jun 16 2002 Havoc Pennington <hp@redhat.com>
- 2.0.0
- remove noreplace from the .soundlist files
- add missing schemas
- get rid of gnometris again
- use desktop-file-install

* Fri Jun 07 2002 Havoc Pennington <hp@redhat.com>
- rebuild in different environment

* Wed Jun  5 2002 Havoc Pennington <hp@redhat.com>
- 1.93.0
- remove empty NEWS/README
- fix ldconfig in post

* Sun May 26 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Tue May 21 2002 Havoc Pennington <hp@redhat.com>
- rebuild in different environment

* Tue May 21 2002 Havoc Pennington <hp@redhat.com>
- 1.92.0

* Fri May  3 2002 Havoc Pennington <hp@redhat.com>
- 1.91.0

* Fri Apr 19 2002 Havoc Pennington <hp@redhat.com>
- GNOME 2 version
- spec file cleanups
- no devel package
- don't run auto*, just use the "rm from buildroot" approach to lose xbill

* Tue Apr 09 2002 Phil Knirsch <pknirsch@redhat.com>
- Bumped version number for rebuild and relink agains new guile lib

* Tue Aug 14 2001 Jonathan Blandford <jrb@redhat.com>
- Add BuildRequires on ncurses-devel

* Mon Jul 23 2001 Jonathan Blandford <jrb@redhat.com>
- Add BuildRequires

* Sun Jun 24 2001 Elliot Lee <sopwith@redhat.com>
- Bump release + rebuild.

* Fri Apr 20 2001  <jrb@redhat.com>
- New version (1.4.0)

* Tue Apr 17 2001 Jonathan Blandford <jrb@redhat.com>
- New Version.

* Tue Feb 27 2001 Trond Eivind Glomsr?d <teg@redhat.com>
- use %%{_tmppath}
- langify

* Mon Aug 21 2000 Nalin Dahyabhai <nalin@redhat.com>
- run ldconfig in post and postun (#16589)
- don't put the post and postun scripts in the middle of the files list --
  that tends to break things (oops)

* Fri Aug 11 2000 Jonathan Blandford <jrb@redhat.com>
- Up Epoch and release

* Fri Aug 04 2000 Havoc Pennington <hp@redhat.com>
- Remove .desktop for gturing

* Mon Jul 17 2000 Jonathan Blandford <jrb@redhat.com>
- Mark high-score files as %config(noreplace).

* Thu Jul 13 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Tue Jul 11 2000 Nalin Dahyabhai <nalin@redhat.com>
- rebuild in new environment

* Sat Jul 8 2000 Havoc Pennington <hp@redhat.com>
- Remove Docdir

* Tue Jul 03 2000 Nalin Dahyabhai <nalin@redhat.com>
- rebuild in new environment

* Thu Jun 15 2000 Owen Taylor <otaylor@redhat.com>
- 1.2.0
- remove gnometris, xbill from subdirs since we don't install
  them and they cause problems with new C compiler
- update autoconf stuff
- remove gturing, add gnotski to the file list

* Thu May 11 2000 Matt Wilson <msw@redhat.com>
- 1.1.90

* Thu Feb 10 2000 Preston Brown <pbrown@redhat.com>
- mark sound event files as config files

* Tue Sep 21 1999 Michael Fulbright <drmike@redhat.com>
- fixed gnotravex to not loop infinitely

* Mon Sep 20 1999 Elliot Lee <sopwith@redhat.com>
- Update to 1.0.40

* Sat Apr 10 1999 Jonathan Blandford <jrb@redhat.com>
- added new sol games and a fix for the old ones.

* Mon Mar 29 1999 Michael Fulbright <drmike@redhat.com>
- removed more offending t*tris stuff

* Thu Mar 18 1999 Michael Fulbright <drmike@redhat.com>
- version 1.0.2
- made gnibbles have correct attr since its setgid
- strip binaries

* Sun Mar 14 1999 Michael Fulbright <drmike@redhat.com>
- added score files to file list

* Thu Mar 04 1999 Michael Fulbright <drmike@redhat.com>
- Version 1.0.1

* Fri Feb 19 1999 Michael Fulbright <drmike@redhat.com>
- removed *tris games

* Mon Feb 15 1999 Michael Fulbright <drmike@redhat.com>
- version 0.99.8
- added sound event lists to file list
- touched up file list some more

* Wed Feb 03 1999 Michael Fulbright <drmike@redhat.com>
- added gnibbles data to file list

* Wed Feb 03 1999 Michael Fulbright <drmike@redhat.com>
- updated to 0.99.7

* Wed Feb 03 1999 Michael Fulbright <drmike@redhat.com>
- updated to 0.99.5

* Mon Jan 18 1999 Michael Fulbright <drmike@redhat.com>
- updated to 0.99.3

* Wed Jan 06 1999 Michael Fulbright <drmike@redhat.com>
- updated to 0.99.1

* Thu Dec 16 1998 Michael Fulbright <drmike@redhat.com>
- updated to 0.99.0 in prep for GNOME 1.0

* Sat Nov 21 1998 Michael Fulbright <drmike@redhat.com>
- updated for 0.30 tree

* Fri Nov 20 1998 Pablo Saratxaga <srtxg@chanae.alphanet.ch>
- use --localstatedir=/var/lib in config state (score files for games
  for exemple will go there).

* Mon Mar 16 1998 Marc Ewing <marc@redhat.com>
- Integrate into gnome-games CVS source tree


