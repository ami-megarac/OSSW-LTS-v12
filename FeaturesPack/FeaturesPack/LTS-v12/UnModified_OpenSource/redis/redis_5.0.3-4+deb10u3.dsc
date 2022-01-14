-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA256

Format: 3.0 (quilt)
Source: redis
Binary: redis, redis-sentinel, redis-server, redis-tools
Architecture: any all
Version: 5:5.0.3-4+deb10u3
Maintainer: Chris Lamb <lamby@debian.org>
Homepage: https://redis.io/
Standards-Version: 4.3.0
Vcs-Browser: https://salsa.debian.org/lamby/pkg-redis
Vcs-Git: https://salsa.debian.org/lamby/pkg-redis.git
Testsuite: autopkgtest
Build-Depends: debhelper-compat (= 12), dpkg-dev (>= 1.17.14), libhiredis-dev (>= 0.14.0), libjemalloc-dev [linux-any], liblua5.1-dev, lua-bitop-dev, lua-cjson-dev, procps <!nocheck>, tcl <!nocheck>
Package-List:
 redis deb database optional arch=all
 redis-sentinel deb database optional arch=any
 redis-server deb database optional arch=any
 redis-tools deb database optional arch=any
Checksums-Sha1:
 f38800839cb85492da9bc5299507299dd54f726c 1977218 redis_5.0.3.orig.tar.gz
 36d53254d1e89acdf5dae32b257a74789811b874 29168 redis_5.0.3-4+deb10u3.debian.tar.xz
Checksums-Sha256:
 7084e8bd9e5dedf2dbb2a1e1d862d0c46e66cc0872654bdc677f4470d28d84c5 1977218 redis_5.0.3.orig.tar.gz
 ace500eef79f6f442f6a5d57e368e613bd1033d217a1b358f84fe66940f5c188 29168 redis_5.0.3-4+deb10u3.debian.tar.xz
Files:
 f2a79cdec792e7c58dd5cad3b6ce47ad 1977218 redis_5.0.3.orig.tar.gz
 ba0ae9535c66a34b6fd8bc4d3bcd93b0 29168 redis_5.0.3-4+deb10u3.debian.tar.xz

-----BEGIN PGP SIGNATURE-----

iQIzBAEBCAAdFiEEwv5L0nHBObhsUz5GHpU+J9QxHlgFAmBUeP4ACgkQHpU+J9Qx
HliG+g//cNsyXGFmkso/sgz4vpVlg3M+PyOAD1cknDPeQlNhWM3usRmss4em4GAd
1O2+LT88K2jnDBu3/x0kQ0OQ9A8Yb6XAZ4gPRu7z1FgPLDESWCdvWPwpzmtNi9r3
F4VJB//BR4BpLixpBCeeu3vMPGklMSIXR9D3/hJtrW/VBO58x1PxfWe7fAPfMDoy
oe5wA6rH7G6BGboBeN1C1n8uVar9L0C1ApRsQ+c8pgO6Q5kMZcIGBnKasG13aSSC
bupch61vRX6Mzk/WjIgx10Ce/AVRjblxViWWjDKmf1SnFhAX3AZAJm8a+It/juwK
WVhKOV4xkiq4W+263VQqEtsdkOeVVHNydvhCXgvqdF4ubcnmOLXgNM04xzgJSGp6
BYAP7kQIrt0MvyarbuviKhNitTvQW0z4y+bcMXZEZBEiNt2MC0JDAYTWJNmC+wKl
5HzGFbI0cvAWV+jqIgC/ftgl2UGhi4nRnmmhpYVXbbw6Y+Xg/1vOMCqaHi/6btwW
3xCvib2eIWFiHDfzoayzxMAaiknzefF1Rkn9cCkxC9jjx7FuRbU6IG0reO0AIy/6
uRiecPKkN5drmDBDdmNFCe/oDY2WhlXRRXBSjB2J10+Hp2SK0QoVPTpNdPJPs/vs
1GqV2usHC54RpixlT5Jc4dOjF64PcHHTQcoQy/PmRXAVH5myQ4A=
=Y8hv
-----END PGP SIGNATURE-----
