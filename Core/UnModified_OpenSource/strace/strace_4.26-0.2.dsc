-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: strace
Binary: strace, strace64, strace-udeb
Architecture: linux-any
Version: 4.26-0.2
Maintainer: Steve McIntyre <93sam@debian.org>
Homepage: https://strace.io
Standards-Version: 4.1.3
Vcs-Browser: https://salsa.debian.org/debian/strace
Vcs-Git: https://salsa.debian.org/debian/strace.git
Build-Depends: libc6-dev (>= 2.2.2) [!alpha !ia64], libc6.1-dev (>= 2.2.2) [alpha ia64], gcc-multilib [amd64 i386 powerpc ppc64 s390 sparc sparc64 x32], debhelper (>= 10.0.0), gawk, libunwind-dev [amd64], libiberty-dev [amd64], libbluetooth-dev
Package-List:
 strace deb utils optional arch=linux-any
 strace-udeb udeb debian-installer optional arch=linux-any
 strace64 deb utils optional arch=i386,powerpc,s390,sparc
Checksums-Sha1:
 b4a054adb74563fc121bcd19b158f58955a20a33 1444216 strace_4.26.orig.tar.xz
 1d0382ce3a5df252ce2a66f80e5c02c41dc299e8 801 strace_4.26.orig.tar.xz.asc
 ca20d46eaabc112ee1f66f179b38ac434631dc06 20432 strace_4.26-0.2.debian.tar.xz
Checksums-Sha256:
 7c4d2ffeef4f7d1cdc71062ca78d1130eb52f947c2fca82f59f6a1183bfa1e1c 1444216 strace_4.26.orig.tar.xz
 510bd54bef4cee57677e9cc72ee8210d9694d5b984dbad65f074befb3d286bcf 801 strace_4.26.orig.tar.xz.asc
 87d280200da703c9bc703688d82ccfc73af76e82eed7c3759377256105275959 20432 strace_4.26-0.2.debian.tar.xz
Files:
 daa51acc0c7c696221ec03cf0b30a7af 1444216 strace_4.26.orig.tar.xz
 09a00725189b7c9f42798dbb1d0927c6 801 strace_4.26.orig.tar.xz.asc
 fd3f4daa6d670e43e28e7d0d9cd2d016 20432 strace_4.26-0.2.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJFBAEBCgAvFiEE+uHltkZSvnmOJ4zCC8R9xk0TUwYFAlw7DC0RHGFuZHJlYXNA
ZmF0YWwuc2UACgkQC8R9xk0TUwbjTg/+JAoWTZkSx1pNDv/IhRIXStAUdEgk5986
gJWhJ9i9Q7jECkkiqoGfFYTm4PBhmyFWjGQy144YQKmaaZtt6Pk7TFJMy+jNMRYZ
RSSg4z8mdaubZXO6LP0b5F5pLjU0cAYiJFu+3SV6jTth/pQ7yGmt/XHDQ60zk+ms
aG+cN3Lx+BNTWP/u/lIsz5L2VZFfw1Pbcx0pWapw1Onxc4oYPvJeWn/50Qx8KNQW
EwlXLHbTC5g5qYKQp3YA2x4qbuc9Cpzo+BmgaQNw4i33osK7LLHANQCr5QR0ih3q
ERsSSx7hjMFhY2Jl/ssVibWw3w1iK80U4BLlEhIIuzTc/rqV0S+IvoaeH/BBzlDA
Gw7HeJA02jLmM6YiaeboGe+bNo3TQv2PnjOBg1U35DcIFzlpHQVoIziPAfVMWYda
grXh9F7c6Ofo89SLOwlLE4tzdcSVo1zZK83bCK0dmjpLBGVZgxCNHy/4c98gBf1g
/ws2nrSriK+l+16JTTNKh2+Pyo5IA2VQFKP9fKFOp3JXXEBIKr0V1xeRTwF2At+5
8S0yAtVXuSOadtGrj8ApAjIuXI9AngMiv0yBGRCeCJx74KpjCiQetIm6MxjrAjF1
K8CPHo3DYYtOMH3kaxGJ1TzQYXkZzz+CLkK5hwQTxwfpf3H27EUnEQE014Tf/kdJ
G+EnIiTxVMo=
=oplR
-----END PGP SIGNATURE-----
