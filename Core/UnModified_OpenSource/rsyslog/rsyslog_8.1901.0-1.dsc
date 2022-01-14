-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: rsyslog
Binary: rsyslog, rsyslog-mysql, rsyslog-pgsql, rsyslog-mongodb, rsyslog-elasticsearch, rsyslog-kafka, rsyslog-gssapi, rsyslog-gnutls, rsyslog-relp, rsyslog-czmq, rsyslog-hiredis
Architecture: any
Version: 8.1901.0-1
Maintainer: Michael Biebl <biebl@debian.org>
Homepage: http://www.rsyslog.com/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/debian/rsyslog
Vcs-Git: https://salsa.debian.org/debian/rsyslog.git
Build-Depends: debhelper-compat (= 12), dh-exec, autoconf-archive, zlib1g-dev, libhiredis-dev, default-libmysqlclient-dev, libpq-dev, libmongoc-dev, libcurl4-gnutls-dev, librdkafka-dev (>= 0.9.1), libkrb5-dev, libgnutls28-dev, librelp-dev (>= 1.2.16), libestr-dev (>= 0.1.9), liblognorm-dev (>= 2.0.3), libfastjson-dev (>= 0.99.8), libczmq-dev (>= 3.0.2), uuid-dev, libsystemd-dev (>= 209) [linux-any], pkg-config, bison, flex, procps <!nocheck>, python <!nocheck>, libfaketime <!nocheck>, systemd [linux-any] <!nocheck>, lsof [linux-any] <!nocheck>
Package-List:
 rsyslog deb admin important arch=any
 rsyslog-czmq deb admin optional arch=any
 rsyslog-elasticsearch deb admin optional arch=any
 rsyslog-gnutls deb admin optional arch=any
 rsyslog-gssapi deb admin optional arch=any
 rsyslog-hiredis deb admin optional arch=any
 rsyslog-kafka deb admin optional arch=any
 rsyslog-mongodb deb admin optional arch=any
 rsyslog-mysql deb admin optional arch=any
 rsyslog-pgsql deb admin optional arch=any
 rsyslog-relp deb admin optional arch=any
Checksums-Sha1:
 7223f77a4ea75a7740130cc04ea3df052e82bdfd 2750872 rsyslog_8.1901.0.orig.tar.gz
 a0672b5e8d25b29e96b7c2d1af56a346d11b8ef0 27252 rsyslog_8.1901.0-1.debian.tar.xz
Checksums-Sha256:
 ab02c1f11e21b54cfaa68797f083d6f73d9d72ce7a1c04037fbe0d4cee6f27c4 2750872 rsyslog_8.1901.0.orig.tar.gz
 a21a35a5b0bf338dd5cdc9ee414929a402bc81941307b6681ed4876402bb3e29 27252 rsyslog_8.1901.0-1.debian.tar.xz
Files:
 f068dadcf81a559db3be760abda0aaf8 2750872 rsyslog_8.1901.0.orig.tar.gz
 be004224710020adecf0d4a404e96d7e 27252 rsyslog_8.1901.0-1.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCAAdFiEECbOsLssWnJBDRcxUauHfDWCPItwFAlx1eugACgkQauHfDWCP
Itw8ixAAhYlrS5k/JD059Ul5sxUdcMgPgW0QBpw6hAEfRIebnSlQQlIcfsGUb8kA
/K4/m4JGecq5CZNIME0oqIirVIX/UfURynxwzc6iDHajM8j6e8By0kP56XakJe9C
yMZuhx/kmdCJkW3smzauJuFUKHau8EENHXdl0TF6Pz8/26hNcc2HU+fD1lVyCaFq
Dqb7IYV8lyWkwXinLVwbKDb6ebqA5g3Ks5xE3l/rz0LnKUUvyR5xjOcp83sSgBkD
Rxz3N1Eu0PKnk7X20vQtRBHWibVsqFM4UWRGoG1MiFq6P0d5zbQhMuOavn0FfJoC
SmoG/IWuViVl8xQ5zw5DOjGF/JQxjbthkAaXKlLLxGRy4C8lBO9De37w9t5lTC82
Mc0GAt5y9pjjEoH3Vlp+cp95qMDnbmz5WSWOjYvPULYzlfU2Xv6pOBx3joc/b/CM
RtF6ayQ9IsywigGEo+UKS7buLsgw4wLJgvuuPb1LVk+xdZjvkWTryc0TuRgxMgJJ
c2T5WWu4GyhbG9elwS7fQrtkEHhIsDinnInrOo8iIrA5rOs17+IV1qOmFi1etmJ6
nyBm7tPZoOW9/G83HuvytIxfo/mAKGKKQeU6WJl0cy+6kNi8/cEaO38d7EFsWkGg
5gK3wPJ2AzN54IV07VSZsm0IUkqUKxtDPqDT5xE51wHTgne7fj4=
=jiYT
-----END PGP SIGNATURE-----
