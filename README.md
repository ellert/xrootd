<p align="center">
  <img src="https://xrootd.github.io/images/xrootd-logo.png"/>
</p>

## XRootD: eXtended ROOT Daemon

The [XRootD](http://xrootd.org) project provides a high-performance,
fault-tolerant, and secure solution for handling massive amounts of data
distributed across multiple storage resources, such as disk servers, tape
libraries, and remote sites. It enables efficient data access and movement in a
transparent and uniform manner, regardless of the underlying storage technology
or location. It was initially developed by the High Energy Physics (HEP)
community to meet the data storage and access requirements of the BaBar
experiment at SLAC and later extended to meet the needs of experiments at the
Large Hadron Collider (LHC) at CERN. XRootD is the core technology powering the
[EOS](https://eos-web.web.cern.ch/) distributed filesystem, which is the storage
solution used by LHC experiments and the storage backend for
[CERNBox](https://cernbox.web.cern.ch/). XRootD is also used as the core
technology for global CDN deployments across multiple science domains.

XRootD is based on a scalable architecture that supports multi-protocol
communications. XRootD provides a set of plugins and tools that allows the user
to configure it freely to deploy data access clusters of any size, and which can
include sophisticated features such as erasure coded files, various methods of
authentication and authorization, as well as integration with other storage
systems like [ceph](https://ceph.io).

## Documentation

General documentation such as configuration reference guides, and user manuals
can be found on the XRootD website at http://xrootd.org/docs.html.

## Supported Operating Systems

XRootD is officially supported on the following platforms:

 * RedHat Enterprise Linux 8 or later and their derivatives
 * Debian 11 and Ubuntu 22.04 or later
 * macOS 13 (Ventura) or later

Support for other operating systems is provided on a best-effort basis
and by contributions from the community.

## Installation Instructions

XRootD is available via official channels in most operating systems.
Installation via your system's package manager should be preferred.

In RPM-based distributions, like Alma, Rocky, Fedora, CentOS Stream,
and RHEL, one can install XRootD with

```sh
dnf install xrootd-client xrootd-server python3-xrootd
```

In RHEL-based distributions, it will be necessary to first install the EPEL
release repository with

```sh
dnf install epel-release
```

On Debian 11 or later, and Ubuntu 22.04 or later, XRootD can be installed via apt

```sh
apt install xrootd-client xrootd-server python3-xrootd
```

If you would like to use our official repositories for XRootD packages instead,
please follow the instructions on our website at one of the links below:

- RPM repositories: http://xrootd.org/dload.html#official-rpm-repositories
- DEB repositories: http://xrootd.org/dload.html#official-deb-repositories

On macOS, XRootD is available via Homebrew
```sh
brew install xrootd
```

XRootD can also be installed with conda, as it is available in conda-forge:
```sh
conda config --add channels conda-forge
conda config --set channel_priority strict
conda install xrootd
```

Finally, it is possible to install the XRootD python bindings from PyPI using pip:
```sh
pip install xrootd
```

For detailed instructions on how to build and install XRootD from source code,
please see [docs/INSTALL.md](https://github.com/xrootd/xrootd/blob/master/docs/INSTALL.md)
in the main repository on GitHub.

## User Support and Bug Reports

Bugs should be reported using [GitHub issues](https://github.com/xrootd/xrootd/issues).
You can open a new ticket by clicking [here](https://github.com/xrootd/xrootd/issues/new).

For general questions about XRootD, please send a message to our user mailing
list at xrootd-l@slac.stanford.edu or open a new [discussion](https://github.com/xrootd/xrootd/discussions)
on GitHub. Please check XRootD's contact page at http://xrootd.org/contact.html
for further information.

## Contributing

User contributions can be submitted via pull request on GitHub. We recommend
that you create your own fork of XRootD on GitHub and use it to submit your
patches. For more detailed instructions on how to contribute, please refer to
the file [docs/CONTRIBUTING.md](https://github.com/xrootd/xrootd/blob/master/docs/CONTRIBUTING.md).
