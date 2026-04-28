# Releasing

This document describes the release policy for binary distributions of
`czi-pyramidizer`.

## Release Targets

Binary releases are produced for the same platforms that are built by the
project's continuous integration workflows:

- Windows x64
- Windows ARM64
- Ubuntu 24.04 x64
- Alpine Linux x64

All release targets should build successfully and run their available tests
before release artifacts are published.

The GitHub Actions package workflows pin the vcpkg registry to a named
upstream release tag. When the pin is advanced, the cache keys and any affected
SBOM dependency versions should be reviewed together so a release tag remains
rebuildable against the same dependency recipes.

## Release Assets

Each GitHub release should provide one archive per supported platform. The
archive should contain:

- `czi-pyramidizer` or `czi-pyramidizer.exe`
- `LICENSE`
- `THIRD_PARTY_LICENSES.txt`
- `README.release.md`
- a package-local SPDX JSON software bill of materials (SBOM)
- a package-local `SHA256SUMS` file

The release should also provide:

- a top-level `SHA256SUMS` file for all published archive assets
- source and build information sufficient to rebuild the released binaries

The repository contains a dedicated GitHub Actions release workflow in
`.github/workflows/release.yml`. It builds all supported platform packages from
the release tag, creates archives, generates per-package SBOMs and checksums,
and publishes the resulting archives to a GitHub Release.

Manual workflow runs support two modes:

- dry run: build a selected source ref, assemble the would-be release assets,
  and upload them as workflow artifacts without creating or updating a GitHub
  Release
- publish: build the selected source ref and create or update the named GitHub
  Release tag

For dry runs, `source_ref` can be the branch under review and `release_tag` can
be left empty; the workflow uses a `dry-run-<run-number>` package suffix. For
published releases, `release_tag` must be set explicitly and should identify
the tag being released.

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

Each release archive should include one package-local SPDX JSON SBOM. The SBOM
should describe the released binary and its relevant build-time and linked
components, including `libCZI`, OpenCV, CLI11, GSL, RapidJSON, and other
platform-specific dependencies.

The SBOM should identify exact versions or source revisions where possible.
It intentionally describes the package payload and excludes release workflow
outputs such as the package-local `SHA256SUMS` file and the SBOM file itself.
Those files are produced after or during SBOM generation, so excluding them
keeps the SBOM inventory deterministic across clean builds and reruns.

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

Each package should also include a package-local `SHA256SUMS` file for the
files inside the extracted package directory. Checksum entries should use bare
file names, without `./` prefixes, so verification instructions are consistent
across Windows, Ubuntu, and Alpine release packages.

## Release Notes

Release notes should include:

- the released version
- supported platforms
- a summary of notable changes
- known limitations
- a reference to the SBOM and checksums
- a note that static `libCZI` releases can be rebuilt with a modified `libCZI`
  by following the documented relinking instructions
