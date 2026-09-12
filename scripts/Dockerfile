# Uses Debian bullseye for GCC 10 / GLIBCXX_3.4.29 compatibility with Move device
FROM debian:bullseye-slim

# Debian 11 reached END OF LTS on 2026-08-31 and its pool is being withdrawn.
# bullseye-security went first and already 404s; main is still served for now,
# so this image builds today by luck of its package set. The failure, when it
# comes, is a confusing one: the INDEX still resolves, so `apt-get update`
# succeeds and `install` 404s on every .deb, which reads as a flaky mirror
# rather than as a withdrawn suite. It cost a release in schwung-sfz, whose
# larger set (build-essential -> libc6-dev from security) hit it first.
#
# Pin both suites to snapshot.debian.org, which serves them as they stood on a
# given day. Repointing at archive.debian.org instead is NOT enough and fails
# in a third way: archive carries no bullseye-security, so the base image's
# libc6 (2.31-13+deb11u14, from the security suite) outranks anything archive
# can offer libc6-dev from, and apt reports "held broken packages". That
# variant passes on an amd64 runner and fails on an arm64 host, so CI alone
# would not have caught it.
#
# Bump the date deliberately. The base image stays bullseye ON PURPOSE (see
# above): bookworm would move glibc 2.31 -> 2.36 and GLIBCXX past what Move's
# runtime provides, so "fix the build by bumping the base" yields a module
# that loads nowhere.
ENV DEBIAN_SNAPSHOT=20260825T000000Z
RUN printf 'deb http://snapshot.debian.org/archive/debian/%s bullseye main\n\
deb http://snapshot.debian.org/archive/debian-security/%s bullseye-security main\n' \
        "$DEBIAN_SNAPSHOT" "$DEBIAN_SNAPSHOT" > /etc/apt/sources.list \
    && apt-get -o Acquire::Check-Valid-Until=false update \
    && apt-get install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    make \
    && rm -rf /var/lib/apt/lists/*

ENV CROSS_PREFIX=aarch64-linux-gnu-

WORKDIR /build
