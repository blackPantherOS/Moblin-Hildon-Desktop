Summary: GTK 2 based scientific calculator
Name: galculator
Version: 1.2.5.2
Release: 1
Copyright: GPL
Group: Application/Math
Source: http://prdownloads.sourceforge.net/galculator/galculator-1.2.5.2.tar.gz?download
Packager: Victor Soroka <gbs@tnss.kharkov.ua>
BuildRoot: /var/tmp/%{name}-%{version}-root
BuildRequires: gtk2-devel >= 2.0.0 

%description
galculator is a GTK 2 based scientific calculator with "ordinary" and reverse polish notation.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS" 
%configure

%install
rm -rf $RPM_BUILD_ROOT
make -k DESTDIR=$RPM_BUILD_ROOT prefix=%{_prefix} install
strip -s $RPM_BUILD_ROOT%{_bindir}/galculator

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-, root, root)
%doc %attr(0644, root, root) AUTHORS ChangeLog NEWS README COPYING TODO
%{_bindir}/galculator
%{_datadir}/applications/galculator.desktop
%{_datadir}/galculator/glade/*.glade
%{_datadir}/locale/*
%{_mandir}/man1/%{name}.1*

%changelog
* Sat Nov 15 2003 Simon Floery <chimaira@users.sf.net>
- applied patch by Filippo

* Thu Oct 3 2002 Simon Floery <chimaira@users.sf.net>
- description update
