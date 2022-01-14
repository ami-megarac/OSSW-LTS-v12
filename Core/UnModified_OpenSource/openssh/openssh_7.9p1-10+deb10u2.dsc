-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: openssh
Binary: openssh-client, openssh-server, openssh-sftp-server, openssh-tests, ssh, ssh-askpass-gnome, openssh-client-udeb, openssh-server-udeb
Architecture: any all
Version: 1:7.9p1-10+deb10u2
Maintainer: Debian OpenSSH Maintainers <debian-ssh@lists.debian.org>
Uploaders: Colin Watson <cjwatson@debian.org>, Matthew Vernon <matthew@debian.org>,
Homepage: http://www.openssh.com/
Standards-Version: 4.1.0
Vcs-Browser: https://salsa.debian.org/ssh-team/openssh
Vcs-Git: https://salsa.debian.org/ssh-team/openssh.git
Testsuite: autopkgtest
Testsuite-Triggers: devscripts, haveged, openssl, putty-tools, python-twisted-conch, sudo, sysvinit-utils
Build-Depends: autotools-dev, debhelper (>= 9.20160709~), dh-autoreconf, dh-exec, dpkg-dev (>= 1.16.1~), libaudit-dev [linux-any], libedit-dev, libgtk-3-dev <!pkg.openssh.nognome>, libkrb5-dev | heimdal-dev, libpam0g-dev | libpam-dev, libselinux1-dev [linux-any], libssl-dev (>= 1.1.0g), libsystemd-dev [linux-any], libwrap0-dev | libwrap-dev, pkg-config, zlib1g-dev (>= 1:1.2.3)
Package-List:
 openssh-client deb net standard arch=any
 openssh-client-udeb udeb debian-installer optional arch=any profile=!noudeb
 openssh-server deb net optional arch=any
 openssh-server-udeb udeb debian-installer optional arch=any profile=!noudeb
 openssh-sftp-server deb net optional arch=any
 openssh-tests deb net optional arch=any
 ssh deb net optional arch=all
 ssh-askpass-gnome deb gnome optional arch=any profile=!pkg.openssh.nognome
Checksums-Sha1:
 993aceedea8ecabb1d0dd7293508a361891c4eaa 1565384 openssh_7.9p1.orig.tar.gz
 e23d601363ab5ff0daae5714e77d06539a9aebe9 683 openssh_7.9p1.orig.tar.gz.asc
 785050c5afc7b7684cb25013d897f500eebf9bc6 174016 openssh_7.9p1-10+deb10u2.debian.tar.xz
Checksums-Sha256:
 6b4b3ba2253d84ed3771c8050728d597c91cfce898713beb7b64a305b6f11aad 1565384 openssh_7.9p1.orig.tar.gz
 4fd584498595450d68f5514b3d79eb14425a3d6aa9e9021d9e928fdd7b4469eb 683 openssh_7.9p1.orig.tar.gz.asc
 2f8d81757e3050aab4ab735692c1b2d521cee54ec3987ef3b08fbaff84abf3c1 174016 openssh_7.9p1-10+deb10u2.debian.tar.xz
Files:
 c6af50b7a474d04726a5aa747a5dce8f 1565384 openssh_7.9p1.orig.tar.gz
 36f798ff56b24004e16c08bf4a5fc021 683 openssh_7.9p1.orig.tar.gz.asc
 e235664e9191eb5a4b91f30c03c9de0a 174016 openssh_7.9p1-10+deb10u2.debian.tar.xz
Dgit: 6d9ca74c48d9911342c6ca5aaac8a25974fa2619 debian archive/debian/1%7.9p1-10+deb10u2 https://git.dgit.debian.org/openssh

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCAAdFiEErApP8SYRtvzPAcEROTWH2X2GUAsFAl40sJIACgkQOTWH2X2G
UAswNg/+Jmt7Jk1Vb1aLZXZt2Vo6NgLzBPpD0w4++BLjb+XJgz9W61aO0kJurssn
LWgNDihzFjKInNjYNo+aXhynH3BF7kNa1lltBvqd4WIAoexnTzlZc7Npw0DyJnmr
9UPg5Y7gDE3Bp7kvE27OxUL2KIaVYYt3+Cuud06cHTFVhiZJyJSLqDom0HuAAS0H
u1rWns1zbW6X+nJV+NfQeJdrStNc68Ek1xpLzT0+vzTEM5TKx8u3Vf5iSeD1Zx34
JLjknNfknJshUW43ueyGwFlJWCSzZwMOdOw4Uo+q5lX313YnzKbf9hIvolp0Ocy6
02UOOwFgz8dr4S1heUsHcRIgagIjJmDb91EFdjKXcJp5fCopKeCL6rvTSYUAkcEd
GPQ9js7ZpXF+oVRDesNggppy9vPBYZezJQjKpJu9IiFG3MQ27SIhcRh18ugD0/o6
i5FaTXviXCovLLB5cngFOW50rMGsiGJIPQo6IWHbwsO2T39hYEIzUVDPr5QH9aXB
1bS0wKI+ZYyOj6u3fv3b+jO2KhAInxcg9wMZSYtSbNtFbfF4ctgXZwvCqoRBc/GH
oQXKyreBLQhmDkygrPFvxc2Z5qgEUIWAj5A7SVjVZ4dPOS7xVEeouIRiuXvVeGTF
GjcRrSMhdnkGRQaMBBNa4uyUzbemyCiD7dU0Usn54XNyfaBjGyQ=
=ZoSl
-----END PGP SIGNATURE-----
