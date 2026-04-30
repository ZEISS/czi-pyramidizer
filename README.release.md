# czi-pyramidizer Release Package

This package contains a prebuilt `czi-pyramidizer` command-line executable.

## Contents

- `czi-pyramidizer` or `czi-pyramidizer.exe`
- `LICENSE`
- `THIRD_PARTY_LICENSES.txt`
- `README.release.md`
- `<package-name>.spdx.json`
- `SHA256SUMS`

## License And Third-Party Notices

`czi-pyramidizer` is licensed under the MIT License. See `LICENSE`.

This binary package includes or links to third-party and ZEISS open source
components. See `THIRD_PARTY_LICENSES.txt` for copyright notices, license
texts, and additional open source disclosure information.

## Static libCZI Builds

Release binaries may statically link `libCZI`. When `libCZI` is used under the
GNU Lesser General Public License (LGPL), the project publishes the source and
build information needed to rebuild `czi-pyramidizer` with a modified `libCZI`.

See the release notes and project documentation for the matching release tag,
dependency versions, build options, and relinking instructions.

## Verification

Official GitHub releases include SHA-256 checksums and an SPDX JSON software
bill of materials (SBOM). Verify the archive checksum against the top-level
release `SHA256SUMS` file, then verify the extracted package contents against
the package-local `SHA256SUMS` file before use.

The package-local SBOM describes the release payload files and the direct
dependencies knowingly included or linked into the binary. It does not list the
package-local `SHA256SUMS` file or the SBOM file itself, because those are
generated packaging outputs rather than input payload files.
