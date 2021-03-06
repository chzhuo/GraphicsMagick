# Note that this is NOT a relocatable package
%define package_name  jasper
%define ver           1.700.2
%define prefix        /usr
%define datadir       %{prefix}/share
%define release       1

Summary: JasPer
Name: %{package_name}
Version: %{ver}
Release: %{release}
Copyright: Modified BSD
Group: Development/Libraries

# FIXME: 
Source: http://www.ece.uvic.ca/~mdadams/jasper/software/jasper-1.650.0.tar.gz

BuildRoot: /var/tmp/%{package_name}-%{version}-root
Requires: libjpeg
BuildRequires: libjpeg-devel
URL: http://www.ece.uvic.ca/~mdadams/jasper/

%description 
JasPer is a collection        
of software (i.e., a library and application programs) for the coding 
and manipulation of images.  This software can handle image data in a 
variety of formats.  One such format supported by JasPer is the JPEG-2000 
format defined in ISO/IEC 15444-1:2000.

%package devel
Summary: Include Files and Documentation
Group: Development/Libraries
Requires: %{package_name} = %{ver}

%description devel
JasPer is a collection        
of software (i.e., a library and application programs) for the coding 
and manipulation of images.  This software can handle image data in a 
variety of formats.  One such format supported by JasPer is the JPEG-2000 
code stream format defined in ISO/IEC 15444-1:2000.

%prep
%setup

./configure --prefix=/usr --enable-shared


# build
%build

#if [ "$SMP" != "" ]; then
#  (make "MAKE=make -k -j $SMP"; exit 0)
#  make
#else
  make
#fi


# install
%install
rm -rf $RPM_BUILD_ROOT

make prefix=$RPM_BUILD_ROOT%{prefix} install


# clean
%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


# files main package
%files
%defattr(-, root, root)

%doc README LICENSE ChangeLog

%{prefix}/bin/*
%{prefix}/lib/lib*.so.*

# files devel package
%files devel
%defattr(-, root, root)

# no API docs yet :(
# %doc doc/html/*

%{prefix}/include/jasper/*
%{prefix}/lib/lib*.so
%{prefix}/lib/lib*.a
%{prefix}/lib/lib*.la

%changelog

* Fri Oct 25 2002 Alexander D. Karaivanov <adk@medical-insight.com>

  - spec file created
