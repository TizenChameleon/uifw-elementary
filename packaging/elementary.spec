#sbs-git:slp/pkgs/e/elementary elementary_1.0.0+svn.61256slp2+build26 7a9d4e37a3eaef2856850022c0127a6e3738b0f2
Name:       elementary
Summary:    EFL toolkit for small touchscreens
Version:    0.7.0.svn61256
Release:    1
Group:      TO_BE/FILLED_IN
License:    LGPL
URL:        http://trac.enlightenment.org/e/wiki/Elementary
Source0:    %{name}-%{version}.tar.bz2
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
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(x11)
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

%autogen --disable-static
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
/usr/lib/*.so.0
/usr/share/elementary/images/*
/usr/share/elementary/config/default/*
/usr/share/elementary/config/illume/*
/usr/share/elementary/config/standard/*
/usr/share/elementary/config/profile.cfg
/usr/lib/elementary/modules/*/*/*.so
/usr/lib/libelementary*.so.*
/usr/share/applications/*.desktop
/usr/share/elementary/objects/*.edj
/usr/share/elementary/edje_externals/*.edj
/usr/share/icons/elementary.png
/usr/share/elementary/themes/*.edj
/usr/share/elementary/config/slp/*
/usr/lib/edje/modules/elm/*/module.so

%files devel
%defattr(-,root,root,-)
/usr/include/elementary-0/*.h
/usr/lib/libelementary.so
/usr/lib/pkgconfig/elementary.pc

%files tools
%defattr(-,root,root,-)
/usr/bin/elementary_*
/usr/lib/elementary_testql.so
