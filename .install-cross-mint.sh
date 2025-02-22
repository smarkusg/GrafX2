#!/bin/sh

SUDO=$(which sudo || echo "")
if [ -z "$SUDO" ] ; then
  echo "no sudo"
else
  echo "sudo: $SUDO"
fi

DISTRIB=$(awk 'NR==1{print $0}' /etc/issue)
echo "DISTRIB=$DISTRIB"

# from the apt-key(8) man page :
# add filename (deprecated)
# Note: Instead of using this command a keyring should be placed directly
#       in the /etc/apt/trusted.gpg.d/ directory with a descriptive name
#       and either "gpg" or "asc" as file extension.
cat <<EOF | $SUDO gpg --dearmour -o /etc/apt/trusted.gpg.d/vincent.riviere.gpg
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v1.4.10 (GNU/Linux)

mQENBEvdU9gBCACzsGfqY8sucAudjQCzloQW4WJ8mUheNbJEqKkaxf4fZg5Y4D1k
MiN+bMIoQiTODV23uVY9DIXIMxwp+mzAEI67H7meyJRn7wKeAxbEB0UNGXvf+szV
epHbufXlHcQFieT4pciheQiab76hDbXARCOap160/YmDDc3UxZwxeF2qdQHvKEOB
ja/Bnoi1+9gKgyAUWd6XJ5U3X0T/mVcEtXfP1Ky51mqo+KTW3Co/mzZ9SXwVqPQf
41xap7SxBH8tWsKmO0uBeVEfsWMMCNnIJr1ZN/yAlb0KcDYvpSEkxCk3MavurwMk
kQspNf9TKHBgcOQzecuV6mBsl+fqriCgGyBXABEBAAG0LVZpbmNlbnQgUml2aWVy
ZSA8dmluY2VudC5yaXZpZXJlQGZyZWVzYmVlLmZyPokBOAQTAQIAIgUCS91T2AIb
AwYLCQgHAwIGFQgCCQoLBBYCAwECHgECF4AACgkQDSIY5je7SxL0FQgAlydUWHCg
QGic9kKRrf87q0Lm+0D4+LzlP1ouglRlIJTeh1fxPKnaJ2yCv+oyjEXoFvHANyFz
SU96HjqDGH1yQCUY9kfmqCXCwZHIb/nSASinm4I32GFYq0NwicaoWVXQ4QtVZQx3
/QXCrc+lnnlUvXLRLmOgDBp5eFpOHUPFxAif4QITD4I9wJQOcgPqGLJ1INdxCVGw
tk8t0q0UFPBUA/RaVG69IxrigQeZFyKhaDflP/2OvJRxeJRQ9ABTzGTY3D+aYrF5
PP8ymOoqhXv/4Ib/V10QaG0EP6Ewy/9PxByRveHIG/OQVr4Sbz+uUUbn89YsE8LE
DnP6Vn2TYHSkZLkBDQRL3VPYAQgAy+YH4C6IhvQITf9f2qB6QjeYoyxhMVF2TPTo
97A5SuA9voMCQllYbXA8oreU1ALbZjOc4xTXo75Ri2S/Tf6Pq4tIMhEmTnUvfZya
C01mtQsrXgkB4kIbZGl4yGeitYOGr62UaoKrjW5OM+C7gyf+g9qiufJok2bKfCWh
mpEDXbVPsl+OEdunKu1OujVu250n4Yp+yrNjDVu/tZjF2ypOz18DGDTIjlZLBzQD
f9YlhsM5r6fg2flOa2USAQ5nPHLTjgURGSkV/lytjG2jlOaPhFk14yASLuxndAFY
NsFE4PxEK6NcF9dO8RECBEdWudjmnSGDCB9XLRAwcuQZIqn72wARAQABiQEfBBgB
AgAJBQJL3VPYAhsMAAoJEA0iGOY3u0sSU2MH/iYjFcZzox1QHhjXPSMS2wEmeb+u
wobY4x51Y6mqjm6vVJJw/65rSfjkxtd9G4H39ScT7DxS1ocdYOcOyNqHfkbnvxD7
uel6iBRDR++pKr/r7tNRG3SS1zGkXvwStqE2M9T9WM5J57R2ie8BhzuGE56qH/VP
OAOOupTil1IbPhiICEEBU1zWx9b/PdBKF3QLt9Pqld9w/lXDvOWKVBnCDmnPmmdp
8BhXEequPfzDYVxaWmOzM3eG5eTYVY7EoeeNCVqlT/i4I8QcDByV/HcTWqCSSZmA
cf+H6gqLMIpF/APBmS35VffaUgVgKdQIC99jCd6QlTg0LYRSU4saxs4kXsM=
=Ma9z
-----END PGP PUBLIC KEY BLOCK-----
EOF

# Vincent Rivière's m68k-atari-mint cross-tools
# http://vincent.riviere.free.fr/soft/m68k-atari-mint/ubuntu.php
if echo "$DISTRIB" | grep Ubuntu ; then
	$SUDO add-apt-repository -y ppa:vriviere/ppa
else
	CODENAME=$(lsb_release -c -s)
	if [ -z "$SUDO" ] ; then
		echo "deb http://vincent.riviere.free.fr/apt/ ${CODENAME} contrib" >> /etc/apt/sources.list
		echo "deb-src http://vincent.riviere.free.fr/apt/ ${CODENAME} contrib" >> /etc/apt/sources.list
	else
		echo "deb http://vincent.riviere.free.fr/apt/ ${CODENAME} contrib" | $SUDO tee -a /etc/apt/sources.list
		echo "deb-src http://vincent.riviere.free.fr/apt/ ${CODENAME} contrib" | $SUDO tee -a /etc/apt/sources.list
	fi
fi

$SUDO apt-get update -ym || exit 1
$SUDO apt-get install -ym cross-mint-essential || exit 1
