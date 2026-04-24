# Releasing

This document describes the release policy for binary distributions of
`czi-pyramidizer`.

## Release Targets

Binary releases are produced for the same platforms that are built by the
project's continuous integration workflows:

- Windows x64
- Windows ARM64
- Linux x64
- Alpine Linux x64

All release targets should build successfully and run their available tests
before release artifacts are published.

## Release Assets

Each GitHub release should provide one archive per supported platform. The
archive should contain:

- `czi-pyramidizer` or `czi-pyramidizer.exe`
- `LICENSE`
- `THIRD_PARTY_LICENSES.txt`
- release notes or a release README

The release should also provide:

- an SPDX JSON software bill of materials (SBOM)
- SHA-256 checksums for all release assets
- source and build information sufficient to rebuild the released binaries

The repository contains a dedicated GitHub Actions release workflow in
`.github/workflows/release.yml`. It builds all supported platform packages from
the release tag, creates archives, generates per-package SBOMs and checksums,
and publishes the resulting archives to a GitHub Release.

## Static Linking

Release binaries are built with static linking where practical. In particular,
`czi-pyramidizer` may statically link `libCZI`.

`libCZI` is a ZEISS library made available as open source software. When a
release uses `libCZI` under the GNU Lesser General Public License (LGPL), the
release must preserve the recipient's ability to rebuild or relink
`czi-pyramidizer` with a modified LGPL-covered `libCZI`.

The relinking procedure is documented in
[Static LGPL Relinking](release-lgpl-relinking.md).

## SBOM

Each release should include one SPDX JSON SBOM for the release artifacts. The
SBOM should describe the released binaries and their relevant build-time and
linked components, including `libCZI`, OpenCV, CLI11, GSL, RapidJSON, and other
platform-specific dependencies.

The SBOM should identify exact versions or source revisions where possible.

### SBOM Next Steps

The first release SBOM intentionally lists direct dependencies that the project
declares or knowingly links. Future release improvements should add reliable
transitive dependency capture from the package managers used by each platform,
including vcpkg, apt, and apk. That work should be based on pinned package
manifests or generated dependency reports so that the SBOM remains accurate and
reviewable.

## Checksums

The release should include a `SHA256SUMS` file with checksums for all published
release assets. Users can use the checksums to verify that downloaded assets
match the files published by the project.

## Release Notes

Release notes should include:

- the released version
- supported platforms
- a summary of notable changes
- known limitations
- a reference to the SBOM and checksums
- a note that static `libCZI` releases can be rebuilt with a modified `libCZI`
  by following the documented relinking instructions
