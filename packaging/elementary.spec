#sbs-git:slp/pkgs/e/elementary elementary 1.0.0+svn.67547slp2+build16 598cc91bf431f150cf48064ab672b1e4df5dc4a2
Name:       elementary
Summary:    EFL toolkit for small touchscreens
Version:    1.0.0+svn.70492slp2+build01
Release:    1
Group:      System/Libraries
License:    LGPLv2.1
URL:        http://trac.enlightenment.org/e/wiki/Elementary
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-evas)
BuildRequires:  pkgconfig(ecore-fb)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(efreet)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(ethumb)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(appsvc)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(icu-i18n)
BuildRequires:  edje-tools
BuildRequires:  embryo
BuildRequires:  eet-tools
BuildRequires:  libjpeg-devel
BuildRequires:  desktop-file-utils

%description
Elementary - a basic widget set that is easy to use based on EFL for mobile This package contains devel content.

%package devel
Summary:    EFL toolkit (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
EFL toolkit for small touchscreens (devel)

%package tools
Summary:    EFL toolkit (tools)
Group:      Development/Tools
Requires:   %{name} = %{version}-%{release}
Provides:   %{name}-bin
Obsoletes:  %{name}-bin

%description tools
EFL toolkit for small touchscreens (tools)


%prep
%setup -q

%build
export CFLAGS+=" -fPIC -Wall"
export LDFLAGS+=" -Wl,--hash-style=both -Wl,--as-needed"

%autogen --disable-static
%configure --disable-static \
	--enable-dependency-tracking \
	--disable-web

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
/usr/lib/libelementary*
/usr/lib/elementary/modules/*/*/*.so
/usr/lib/edje/modules/elm/*/module.so
/usr/share/*

%files devel
%defattr(-,root,root,-)
/usr/include/*
/usr/lib/libelementary.so
/usr/lib/pkgconfig/elementary.pc

%files tools
%defattr(-,root,root,-)
/usr/bin/elementary_*
/usr/lib/elementary_testql.so

