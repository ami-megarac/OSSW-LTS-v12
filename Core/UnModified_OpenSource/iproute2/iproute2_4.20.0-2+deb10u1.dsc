-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: iproute2
Binary: iproute2, iproute2-doc
Architecture: linux-any all
Version: 4.20.0-2+deb10u1
Maintainer: Alexander Wirt <formorer@debian.org>
Uploaders: Luca Boccassi <bluca@debian.org>
Homepage: https://wiki.linuxfoundation.org/networking/iproute2
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/debian/iproute2.git
Vcs-Git: https://salsa.debian.org/debian/iproute2.git
Testsuite: autopkgtest
Testsuite-Triggers: @builddeps@, build-essential, dpkg-dev, kmod, sudo
Build-Depends: bison, debhelper (>= 10~), flex, iptables-dev, libatm1-dev, libcap-dev, libdb-dev, libelf-dev, libmnl-dev, libselinux1-dev, linux-libc-dev, pkg-config, po-debconf, zlib1g-dev
Package-List:
 iproute2 deb net important arch=linux-any
 iproute2-doc deb doc optional arch=all
Checksums-Sha1:
 3713570a68fd31539737fcf639c15abef566b606 707016 iproute2_4.20.0.orig.tar.xz
 72c40646da999d3d8f49367e2282574ce2cad4de 146688 iproute2_4.20.0-2+deb10u1.debian.tar.xz
Checksums-Sha256:
 c8adaa6a40f888476b23acb283cfa30c0dd55f07b5aa20663ed5ba2ef1f6fda8 707016 iproute2_4.20.0.orig.tar.xz
 d01f9c4b17519156cc0aadd5103cb38e928ceb3e86efb6b6e7479358794658e3 146688 iproute2_4.20.0-2+deb10u1.debian.tar.xz
Files:
 f3dab4c812812bbb5873cb90f471bcbf 707016 iproute2_4.20.0.orig.tar.xz
 8dd7fe4ce79f658464244a3b762d64a7 146688 iproute2_4.20.0-2+deb10u1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQFFBAEBCgAvFiEE6g0RLAGYhL9yp9G8SylmgFB4UWIFAl/uBt0RHGJsdWNhQGRl
Ymlhbi5vcmcACgkQSylmgFB4UWLP6Qf+Mlfi/yM1LviWsLPWzsB6Y3Y/UpRLTR3X
uOvXvsckeaB2bx13WyCQ/2Ds6l5Aj9ax/G6KnYflIDOVTfKOJPkq0aP4Gp0ctWku
H8BcO7yZJ5PSiviqOLdwj8n5H2Wrgdw4X4t4ZVL2OycZ+EhNimxteozuKfHmXUGp
cG585d2J07p3z1siPlMhphDffQvg9ZPHMsH76uBoP7nsE7/oSU1g0lWpC3ddB1Oc
fL6cN7X/VIiShyOvkKsWGGJQ9XuibCqyw5pBnarY6EPSDnMdQKQFPH6iPlVywQ6t
WIn2NAXqU9mLfg2AG0yLV5JIgsK13uQxxVSC3tctKMksbr/uBwtHzg==
=Tafw
-----END PGP SIGNATURE-----
