#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Carl Zeiss Microscopy GmbH
#
# SPDX-License-Identifier: MIT

"""Generate the release SPDX SBOM from explicit project metadata.

The release archives contain statically linked C++ binaries. Generic binary or
directory scanners do not reliably recover statically linked components such as
libCZI, OpenCV, CLI11, GSL, or RapidJSON from the final executable. This helper
therefore creates a small, reviewable SPDX 2.3 JSON document from
``sbom-components.json`` and the files staged for a release artifact.

The generated SBOM is intentionally conservative: it lists the direct
dependencies that the project declares or knowingly links, records file
checksums for the release package contents, and leaves unpinned package-manager
versions as ``NOASSERTION``. Transitive dependency capture from vcpkg, apt, and
apk should be added separately once those inputs are pinned or exported in a
machine-readable form.
"""

import argparse
import datetime
import hashlib
import json
import pathlib
import re


def spdx_id(value):
    value = value.replace("++", "plusplus").replace("+", "plus")
    normalized = re.sub(r"[^A-Za-z0-9.-]+", "-", value).strip("-")
    return f"SPDXRef-{normalized}"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def include_component(component, platform):
    platforms = component.get("platforms")
    return not platforms or platform in platforms


def package_from_component(component):
    package = {
        "name": component["name"],
        "SPDXID": spdx_id(f"Package-{component['name']}"),
        "versionInfo": component.get("version", "NOASSERTION"),
        "supplier": component.get("supplier", "NOASSERTION"),
        "downloadLocation": component.get("downloadLocation", "NOASSERTION"),
        "filesAnalyzed": False,
        "licenseConcluded": component.get("licenseConcluded", "NOASSERTION"),
        "licenseDeclared": component.get("licenseDeclared", "NOASSERTION"),
        "copyrightText": component.get("copyrightText", "NOASSERTION"),
        "primaryPackagePurpose": component.get("purpose", "LIBRARY"),
    }
    return package


def main():
    parser = argparse.ArgumentParser(
        description="Generate a simple SPDX JSON SBOM from project release metadata."
    )
    parser.add_argument("--metadata", required=True, type=pathlib.Path)
    parser.add_argument("--artifact-dir", required=True, type=pathlib.Path)
    parser.add_argument("--artifact-name", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()

    metadata = json.loads(args.metadata.read_text(encoding="utf-8"))
    created = datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0)
    document_id = spdx_id(f"DocumentRoot-{args.artifact_name}")

    packages = [
        {
            "name": args.artifact_name,
            "SPDXID": document_id,
            "supplier": metadata["document"].get("supplier", "NOASSERTION"),
            "downloadLocation": metadata["document"].get(
                "downloadLocation", "NOASSERTION"
            ),
            "filesAnalyzed": False,
            "licenseConcluded": "NOASSERTION",
            "licenseDeclared": "NOASSERTION",
            "copyrightText": "NOASSERTION",
            "primaryPackagePurpose": "FILE",
        }
    ]

    component_packages = [
        package_from_component(component)
        for component in metadata["components"]
        if include_component(component, args.platform)
    ]
    packages.extend(component_packages)

    files = []
    relationships = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": document_id,
        }
    ]

    for path in sorted(args.artifact_dir.iterdir()):
        if not path.is_file() or path == args.output:
            continue
        file_id = spdx_id(f"File-{path.name}")
        files.append(
            {
                "fileName": path.name,
                "SPDXID": file_id,
                "checksums": [
                    {
                        "algorithm": "SHA256",
                        "checksumValue": sha256(path),
                    }
                ],
                "licenseConcluded": "NOASSERTION",
                "licenseInfoInFiles": ["NOASSERTION"],
                "copyrightText": "NOASSERTION",
            }
        )
        relationships.append(
            {
                "spdxElementId": document_id,
                "relationshipType": "CONTAINS",
                "relatedSpdxElement": file_id,
            }
        )

    app_id = spdx_id("Package-czi-pyramidizer")
    for package in component_packages:
        relationships.append(
            {
                "spdxElementId": document_id,
                "relationshipType": "CONTAINS",
                "relatedSpdxElement": package["SPDXID"],
            }
        )
        if package["SPDXID"] == app_id:
            continue
        relationships.append(
            {
                "spdxElementId": app_id,
                "relationshipType": "DEPENDS_ON",
                "relatedSpdxElement": package["SPDXID"],
            }
        )

    document = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": args.artifact_name,
        "documentNamespace": (
            "https://github.com/ZEISS/czi-pyramidizer/sbom/"
            f"{args.artifact_name}-{created.strftime('%Y%m%d%H%M%S')}"
        ),
        "creationInfo": {
            "creators": [
                "Organization: Carl Zeiss Microscopy GmbH",
                "Tool: czi-pyramidizer-generate-spdx-sbom",
            ],
            "created": created.isoformat().replace("+00:00", "Z"),
        },
        "packages": packages,
        "files": files,
        "relationships": relationships,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
