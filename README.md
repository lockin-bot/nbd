NBD README
==========

## VSOCK Fork

This is a fork of [NetworkBlockDevice/nbd](https://github.com/NetworkBlockDevice/nbd) that adds **VSOCK (virtio socket) transport support**. The changes are contained in a single commit (`7ebddc70`).

### Why This Fork Exists

AWS Nitro Enclaves have no direct network or filesystem access. The only communication channel to the host is VSOCK. This fork enables NBD to operate over VSOCK, allowing the enclave to mount a block device served from the host.

### What Changed

- **Client:** New CLI options (`--vsock`, `--vsock-cid`, `--vsock-port`) for connecting to an NBD server over VSOCK
- **Server:** Config file support (`vsockcid`, `vsockport`, `vsock` mode) for serving exports over VSOCK
- **CID resolution:** Supports named CIDs (`host`, `hypervisor`, `local`) and numeric values
- **New files:** `vsock_support.c` and `vsock_support.h` -- self-contained VSOCK transport layer
- **Build system:** Nix flake with reproducible builds (deterministic flex/bison output)
- **CI/CD:** GitHub Actions workflows for automated releases

All existing TCP and Unix socket functionality is fully preserved. The VSOCK transport is additive only.

### Security Model

NBD itself is a transport layer -- it serves raw blocks and does not provide encryption. **Data at rest security does not depend on NBD.** In our architecture:

- The enclave applies **LUKS disk encryption** on top of the NBD block device using a master key derived from [seal-kms](https://github.com/lockin-bot/seal-kms)
- The host-side NBD server only sees encrypted (ciphertext) blocks
- Even with full host access, the block device contents cannot be decrypted without the master key, which only exists inside attested enclave memory

### Reproducible Builds

The Nix build system produces deterministic output, which is important for enclave integrity verification. Build with:

```bash
nix build            # Default build
nix build .#minimal  # Minimal build
nix build .#static   # Static binary
```

---

Welcome to the NBD userland support files!

This package contains nbd-server and nbd-client.

To install the package, download the source and do the normal
`configure`/`make`/`make install` dance. You'll need to install it on both the
client and the server.

For compiling from git, do a checkout, install the SGML tools
(docbook2man), and then run './autogen.sh' while inside your checkout.
Then, see above.

Contributing
------------

If you want to send a patch, please do not open a pull request; instead, send
it to the
[mailinglist](https://lists.debian.org/nbd)

Security issues
---------------

If you think you found a security problem in NBD, please contact the
mailinglist. Do *not* just file an issue for this (although you may do
so too if you prefer).

For embargoed issues, please contact Wouter Verhelst <wouter@debian.org>

Using NBD
---------

NBD is quite easy to use. First, on the client, you need to load the module
and, if you're not using udev, to create the device nodes:

    # modprobe nbd
    # cd /dev
    # ./MAKEDEV nbd0

(if you need more than one NBD device, repeat the above command for nbd1,
nbd2, ...)

Next, write a configuration file for the server. An example looks like
this:

    # This is a comment
    [generic]
        # The [generic] section is required, even if nothing is specified
        # there.
        # When either of these options are specified, nbd-server drops
        # privileges to the given user and group after opening ports, but
        # _before_ opening files.
        user = nbd
        group = nbd
    [export1]
        exportname = /export/nbd/export1-file
        authfile = /export/nbd/export1-authfile
        timeout = 30
        filesize = 10000000
        readonly = false
        multifile = false
        copyonwrite = false
        prerun = dd if=/dev/zero of=%s bs=1k count=500
        postrun = rm -f %s
    [otherexport]
        exportname = /export/nbd/experiment
        # The other options are all optional

The configuration file is parsed with GLib's GKeyFile, which parses key
files as they are specified in the Freedesktop.org Desktop Entry
Specification, as can be found at
<http://freedesktop.org/Standards/desktop-entry-spec>. While this format
was not intended to be used for configuration files, the glib API is
flexible enough for it to be used as such.

Now start the server:

    nbd-server -C /path/to/configfile

Note that the filename must be an absolute path; i.e., something like
`/path/to/file`, not `../file`. See the nbd-server manpage for details
on any available options.

Finally, you'll be able to start the client:

    nbd-client <hostname> -N <export name> <nbd device>

e.g.,

    nbd-client 10.0.0.1 -N otherexport /dev/nbd0

will use the second export in the above example (the one that exports
`/export/nbd/experiment`)

`nbd-client` must be ran as root; the same is not true for nbd-server
(but do make sure that /var/run is writeable by the server that
`nbd-server` runs as; otherwise, you won't get a PID file, though the
server will keep running).

There are packages (or similar) available for most current operating
systems; see the "Packaging status" badge below for details.

For questions, please use the [nbd@other.debian.org](mailto:nbd@other.debian.org) mailinglist.

Alternate implementations
=========================

Besides this project, the NBD protocol has been implemented by various
other people. A (probably incomplete) list follows:

* [nbdkit](https://gitlab.com/nbdkit/nbdkit) is a multithreaded NBD
  server with a plugin architecture.
* [libnbd](https://gitlab.com/nbdkit/libnbd) is a library to aid in
  writing NBD clients
* [qemu](https://www.qemu.org) contains an embedded NBD server, an
  embedded NBD client, and a standalone NBD server (`qemu-nbd`). They
  maintain a [status
  document](https://gitlab.com/qemu-project/qemu/-/blob/master/docs/interop/nbd.rst)
  of their NBD implementation.
* A [GEOM gate-based client implementation for
  FreeBSD](https://github.com/freqlabs/nbd-client) exists. It has not
  seen any updates since 2018, and only implements the client side
  (any server should run on FreeBSD unmodified, however).
* A Windows client implementation exists as part of the [RBD
  implementation](https://docs.ceph.com/en/latest/rbd/rbd-windows/) of
  [Ceph for Windows](https://cloudbase.it/ceph-for-windows/).
* [lwNBD](https://github.com/bignaux/lwNBD) is a NBD server library,
  targetting bare metal or OS embedded system. It has a plugin architecture.

Additionally, these implementations once existed but are now no longer
maintained:

* xnbd: This was an NBD implementation with a few extra protocol
  messages that allowed for live migration. Its code repository has
  disappeared.
* enbd: This was an NBD implementation with a few extra protocol
  messages that allowed extra ioctl calls to be passed on (e.g., the
  "eject" message for a CD-ROM device that was being exported through
  NBD). It appears to no longer be maintained.
* Hurd translator: There was a [proof-of-concept
  implementation](https://lists.debian.org/debian-hurd/2001/09/msg00174.html)
  of the NBD protocol once as a translator for The Hurd. We do not know
  what its current status is.
* Christoph Lohmann once wrote a client implementation for Plan 9. The
  link he provided us is now stale; we do not know what its current
  status is.

Badges
======

[![Download Network Block Device](https://img.shields.io/sourceforge/dm/nbd.svg)](https://sourceforge.net/projects/nbd/files/latest/download)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1243/badge.svg)](https://scan.coverity.com/projects/1243)
[![CII badge](https://bestpractices.coreinfrastructure.org/projects/281/badge)](https://bestpractices.coreinfrastructure.org/projects/281)
[![Travis](https://img.shields.io/travis/NetworkBlockDevice/nbd.svg)](https://travis-ci.org/NetworkBlockDevice/nbd)

[![Packaging status](https://repology.org/badge/vertical-allrepos/nbd.svg)](https://repology.org/metapackage/nbd)
