# Command Line Interface (CLI)

This page documents the `czi-pyramidizer` command line tool.

## Synopsis

```
czi-pyramidizer [options] <source.czi> <destination.czi>
```

## Core Commands

- Pyramidize (default): Generate (or extend) a pyramid for the source CZI and write to destination.
- CheckOnly: Only check whether a pyramid is needed (no writing).

## Options (examples)

| Option | Description |
|--------|-------------|
| `--check` | Run in check-only mode (no modification). |
| `--overwrite` | Allow overwriting an existing destination file. |
| `--source-stream-class <name>` | Use a custom libCZI stream class. |
| `--stream-prop key=value` | Provide stream property pairs (repeatable). |
| `--threads <n>` | Override automatic thread selection. |
| `--help` | Show help and exit. |

(Adapt this list to actual implemented options in `commandlineoptions`.)

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success / no pyramid needed (check mode) |
| 1 | Invalid arguments |
| 2 | Pyramid needed (check mode) |
| 3 | General error |

## Examples

```
# Generate / update pyramid
czi-pyramidizer input.czi output.czi

# Check whether pyramid is needed
czi-pyramidizer --check input.czi dummy.czi
```

## Progress Output

The tool prints dynamic progress lines for:
- Copying layer 0
- Generating upper layers
- Copying attachments

## Logging / Colors

Colorized output is used when a TTY is detected. Redirecting output may disable colors depending on console implementation.

## Future Enhancements

- JSON progress mode
- Quiet / verbose flags
- Performance metrics summary
