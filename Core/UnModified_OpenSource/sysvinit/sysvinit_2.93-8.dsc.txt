-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

Format: 3.0 (quilt)
Source: sysvinit
Binary: sysvinit-core, sysvinit-utils, sysv-rc, initscripts, bootlogd
Architecture: any all
Version: 2.93-8
Maintainer: Debian sysvinit maintainers <debian-init-diversity@chiark.greenend.org.uk>
Uploaders:  Ian Jackson <ijackson@chiark.greenend.org.uk>, Benda Xu <heroxbd@gentoo.org>, Dmitry Bogatov <KAction@debian.org>, Vincenzo (KatolaZ) Nicosia <katolaz@freaknet.org>,
Homepage: http://savannah.nongnu.org/projects/sysvinit
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/debian/sysvinit
Vcs-Git: https://salsa.debian.org/debian/sysvinit.git
Build-Depends: debhelper-compat (= 11), libselinux1-dev (>= 1.14) [linux-any], libsepol1-dev [linux-any], po-debconf, quilt (>= 0.40)
Package-List:
 bootlogd deb admin optional arch=any
 initscripts deb admin optional arch=any
 sysv-rc deb admin optional arch=all
 sysvinit-core deb admin optional arch=any
 sysvinit-utils deb admin required arch=any essential=yes
Checksums-Sha1:
 fa706acd2a60f13d4364c0952bf4325d491c6ad2 117580 sysvinit_2.93.orig.tar.xz
 b803c875a1c1b162aeed4304623e862163fda41d 1076 sysvinit_2.93.orig.tar.xz.asc
 00a5f2b1451bba3ad3d01c405b54bef8ce2c8eee 127136 sysvinit_2.93-8.debian.tar.xz
Checksums-Sha256:
 472d460e233d981488509a167125a82925c8c9aba6b5608cb22598fdf326a8ff 117580 sysvinit_2.93.orig.tar.xz
 cf2b374a96276a16e3ef07ad2be596420f0d8d77227aad3144d7ab4ea165a4af 1076 sysvinit_2.93.orig.tar.xz.asc
 2db2ae46048acf743445545151cbc0bc5530eca1f2eec51df3175d8ab26edfa6 127136 sysvinit_2.93-8.debian.tar.xz
Files:
 041dbe36a5dd80b2108aff305bc10620 117580 sysvinit_2.93.orig.tar.xz
 93d42c8c7bf2e85c2395bea0983f2938 1076 sysvinit_2.93.orig.tar.xz.asc
 861e7f2e6ba48ebba02bcca36d5f78bf 127136 sysvinit_2.93-8.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQJHBAEBCgAxFiEEhnHVzDbtdH7ktKj4SBLY3qgmEeYFAlxl11ITHGthY3Rpb25A
ZGViaWFuLm9yZwAKCRBIEtjeqCYR5tgID/4/5pCTlE+qO4BxskL8MNUQhHH9hFqi
7L8LIG/uNvsmQHrx0AeBROm2WeZzXE+4XyP3tykagCFu6zrB81Nk4Udv4EHkpPel
fNt4tMQf5uce/TrnUloVDPkJbn572K3K68FFbuRP4Cmr/MwF5AbDypKANpJuJmVy
yBGNuD+o4VYry13N7P3JNySOK6WFEZ1F0tureB3IPG9g/qOFf9+STDA3F0zyj7Aw
PoN1TSzmuCsuXMzsW973nCYh4EQQVWN8vSY9S65bxuAHWrIqMyLoPvzhdjMlRlmL
PiBOu2CY2r9U0iLAGI0DEv1VW1BG+EYkimfCW8Jff+xv+and5FqrAzDqKmL541m2
m2Q5453gvQcwYq9xniEWcNU9kK4tOXRcQ8cHZ1yh5B1U+z/3XXA7Vi2cE5a/+FHr
t3t7eJy/Vw+pC/kQYWfhGdWejwn7a/NRsw2PndDrWfvStZ9yTWgQVH6/X2VG8p/k
R8Ci4wFHAGDqu66Xo9qZfhUhmnTyICu/w1l1VEmTy/8l4KK1tmJ/7qX24PScwi5I
mu8Y1V4XSerDu+UTzSYQlG1+KabEPb7/TtOZcaAPlhQMRxRY3mEbG5D5ebM2soXA
JTCYyRLsT+xaJOvzLdVL+H1JnMZA5ub0FvG4qq8c2XblxZ0EjKY51hWA3vDQUvrs
X63ZrzX7z90wXA==
=xdZG
-----END PGP SIGNATURE-----
