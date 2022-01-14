-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: ntp
Binary: ntp, ntpdate, ntp-doc, sntp
Architecture: any all
Version: 1:4.2.8p12+dfsg-4
Maintainer: Debian NTP Team <ntp@packages.debian.org>
Uploaders: Kurt Roeckx <kurt@roeckx.be>, Bernhard Schmidt <berni@debian.org>
Homepage: http://support.ntp.org/
Standards-Version: 4.1.3.0
Vcs-Browser: https://salsa.debian.org/pkg-ntp-team/ntp
Vcs-Git: https://salsa.debian.org/pkg-ntp-team/ntp.git
Build-Depends: autogen, debhelper (>= 10), dh-apparmor, libcap2-dev [linux-any], libedit-dev, libevent-dev, libopts25-dev, libssl-dev, pkg-config, pps-tools [linux-any]
Build-Conflicts: libavahi-compat-libdnssd-dev, libwww-dev, libwww-ssl-dev
Package-List:
 ntp deb net optional arch=any
 ntp-doc deb doc optional arch=all
 ntpdate deb net optional arch=any
 sntp deb net optional arch=any
Checksums-Sha1:
 59aaa1b64990d7a961467a90cb88a145cd7ab614 4366448 ntp_4.2.8p12+dfsg.orig.tar.xz
 f498265b9030a5026351ad513101899eeb59628f 48796 ntp_4.2.8p12+dfsg-4.debian.tar.xz
Checksums-Sha256:
 84648192f1f0b74f42e24575ca2cbcf48a5bee7814bc874364fcbf21bee9bf50 4366448 ntp_4.2.8p12+dfsg.orig.tar.xz
 4500a8b0a2d2d5d0faccd60cf29f42869431ffcccdd5bbfe4a0b707cd9f3a21c 48796 ntp_4.2.8p12+dfsg-4.debian.tar.xz
Files:
 71044a49f4ab09c9bc10ed9862d22939 4366448 ntp_4.2.8p12+dfsg.orig.tar.xz
 bec40e0bda23a893e76843579c68d901 48796 ntp_4.2.8p12+dfsg-4.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJFBAEBCgAvFiEE1uAexRal3873GVbTd1B55bhQvJMFAlyvqSYRHGJlcm5pQGRl
Ymlhbi5vcmcACgkQd1B55bhQvJNBwhAAoJZeW7MILfYjyak0C0YrmUof6eNGSXeG
2jYoKCUFme7iYc2Wb6Psng4KeqzzO2j643KH44gVY5CqlsUW4RoHfs60DxQRmz8U
3XmZRLIFRpBBLbN3e/wa+8mJ/8xFnyI6YnVHOB/Y4eUgFtVNILTRQFjK+euFaCNJ
yrjnJ3Xv9SMrqRDuhrwBcUeacBPgOXVi4D+6eTM6FqiDpaTBtQ/jWKTDZjtm12hF
HVTNlS+/os+vL5cAJGNgMkgb52S5PHokKJ/K++fcaTKGvkFF/p5mGptJE4qvJVQX
0ItSv3XGJV+2+OqwNtt0opuF3M7PFZ/Ty2h+thM0B8hGUeVaAdKESGxZ6C4xSQ/G
3093vKZAV+KxYqQUqUAcK3M+HVT7AxpVBbOs8UYPb+igCHPxHSvHxEfA/7eqXifb
FU3KY/lU2V18AhECfQCRvOE2pZqg1+4ApfXFyI8kBZGQIRJUO+FF3mV9/nylcv1f
9pvbygFQEBme5WQBSCImTvXWVsuuoQ4UCLa/tEa1Ekm1f2bEVCInrye/7N7MjxMj
Jjym97emtt566wvJfqGqQREtpWvWF7Q08i15B5E6IEEaaANA/bzgQ9sXK7YPbEiN
aN4ycNTEdl5LgfWW0NIXkQNcyyzIAFCGw5KVmpUHD/B98tBqhTnq7cM7I3LX81Tu
H8hbexmKQZE=
=9I/5
-----END PGP SIGNATURE-----
