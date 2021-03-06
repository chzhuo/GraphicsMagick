%define name libwmf
%define version 0.2.5
%define release 1
%define prefix /usr
%define bindir %{prefix}/bin
%define libdir %{prefix}/lib
%define includedir %{prefix}/include
%define datadir %{prefix}/share
%define wmffontdir %{datadir}/%{name}/fonts

Name: %{name}
Summary: library and utilities for displaying and converting metafile images
Version: %{version}
Release: %{release}
Source: %{name}-%{version}.tar.gz
Group: System Environment/Libraries
URL: http://wvware.sourceforge.net/
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
License: LGPL
Prefix: %{prefix}
Epoch: 1

%description
libwmf is a library for interpreting metafile images and either displaying them
using X or converting them to standard formats such as PNG, JPEG, PS, EPS and
SVG(Z)...

%package devel
Summary: Development tools for programs to manipulate metafile images
Group: Development/Libraries
Requires: %{name} = %{PACKAGE_VERSION}
Serial: 1

%description devel
The libwmf-devel package contains the header files and static
libraries necessary for developing programs using libwmf.

If you want to develop programs which will manipulate metafile images, you
should install libwmf-devel. You'll also need to install the libwmf package.

%prep
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%setup -q -n %{name}-%{version}
./configure \
	--prefix=%{prefix} \
	--bindir=%{bindir} \
	--libdir=%{libdir} \
	--includedir=%{includedir} \
	--datadir=%{datadir} \
	--with-fontdir=%{wmffontdir}

%build
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

%files
%defattr(-,root,root)
%{bindir}/wmf2*
%{bindir}/libwmf-fontmap
%{libdir}/libwmf*so*
%{libdir}/libwmf*la
%{wmffontdir}
%doc README COPYING ChangeLog
%doc doc/*.html doc/*.png doc/*.gif
%doc doc/caolan/*.html doc/caolan/*.gif
%doc doc/caolan/pics/weave.jpg
%doc doc/html/*.html doc/html/doxygen.css doc/html/doxygen.gif

%files devel
%defattr(-,root,root)
%{bindir}/libwmf-config
%{includedir}/libwmf
%{libdir}/libwmf*.a

%changelog
* Thu Feb 21 2002 Francis James Franklin <fjf@alinameridon.com>
* Wed Feb 20 2002 Francis James Franklin <fjf@alinameridon.com>
- incorporating suggestions by Michal Jaegermann <michal@harddata.com>

* Mon Feb 18 2002 Francis James Franklin <fjf@alinameridon.com>
- spec adapted from AbiWord's spec template
