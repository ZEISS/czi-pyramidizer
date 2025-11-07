# CZI Pyramidizer

A tool for generating and verifying multi-resolution pyramids inside CZI image containers.

## Features

- Detect whether a pyramid is required
- Generate missing pyramid layers
- Copy attachments and base layer tiles
- Progress reporting with colored console output
- Optional Azure / curl based streaming support via libCZI

## Quick Start

```
czi-pyramidizer input.czi output.czi
```

Check only:
```
czi-pyramidizer --check input.czi dummy.czi
```

## Documentation Sections

- Getting Started
- Usage (CLI & Examples)
- Development (Build & Contributing)

## Status

Early-stage documentation. Contributions welcome.

## License

(Insert license summary here.)
