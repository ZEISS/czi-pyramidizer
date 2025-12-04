# Command Line Interface (CLI)

This page documents the `czi-pyramidizer` command line tool.

## Synopsis

```
czi-pyramidizer.exe [OPTIONS]


  version: 0.1.0

OPTIONS:
  -h, --help        Print this help message and exit

  -s, --source TEXT Source CZI file URI

      --source-stream-class STREAMCLASS
                    Specifies the stream-class used for reading the source
                    CZI-file. If not specified, the default file-reader
                    stream-class is used. Run with argument '--version' to get a
                    list of available stream-classes.

      --propbag-source-stream-creation PROPBAG
                    Specifies the property-bag used for creating the stream used
                    for reading the source CZI-file. The data is given in
                    JSON-notation.

  -d, --destination TEXT
                    Destination CZI file URI

  -n, --mode-of-operation MODE
                    Option controlling the mode of operation - can be either
                    'IfNeeded' meaning that a pyramid generation is attempted
                    irrespective whether a pyramid is present in the source, or
                    'Always' which will discard a potentially existing pyramid.
                    Default is 'IfNeeded'.

  -c, --compressionopts COMPRESSIONOPTIONS
                    A string defining the compression options for the
                    pyramid-subblocks created. C.f. below for the syntax.

  -m, --max_top_level_pyramid_size MAX_TOP_LEVEL_PYRAMID_SIZE
                    The maximum size of the top-level pyramid tile. Pyramid
                    layers are added until the top-level pyramid tile is smaller
                    than this size. Default is 1024.

  -t, --tile_size TILE_SIZE
                    The extent of the pyramid-tiles in pixels. Default is 1024.

  -v, --verbosity VERBOSITYLEVEL
                    Set the verbosity of the console output. The argument is a
                    number between 0 and 5, where higher number indicate that
                    more output is generated.

  -o, --overwrite   Overwrite the destination file if it exists.

  -b, --background-color COLOR
                    Choose the background color, which can be either black or
                    white. Possible values are 'black' or '0', 'white' or '1'
                    and 'auto'. With 'auto', the appropriate background color is
                    attempted to be detected from the source document.

  -p, --threshold-for-pyramid THRESHOLD_SIZE
                    The threshold when a pyramid is required. If the image is
                    smaller than this threshold, no pyramid is necessary.
                    Default is 4096.

      --version     Print extended version-info and supported operations, then
                    exit.

      --check-only  Check the source whether it already contains a pyramid, and
                    exit.


This tool is used to create pyramid layers and put them into a CZI file. The
image data from the source CZI is read, from this an image pyramid is
constructed, and the source image and the pyramid is written into an output CZI.
If the '--compressionopts' argument is given, then the pyramid-subblocks will be
compressed using this specification. If not given, the compression mode is
determined by inspecting the source document. The format (used with the
'--compressionopts' argument) is "compression_method: key=value; ...". It starts
with the name of the compression-method, followed by a colon, then followed by a
list of key-value pairs which are separated by a semicolon.
Examples: "zstd0:ExplicitLevel=3",
"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack", "uncompressed:", "jpgxr:".

The following exit codes are returned from the application:
0 - The operation completed successfully. If running a check (--check-only flag
given), this indicates that there is no need to generate a pyramid - it is
either not needed or already present.
1 - Generic failure code - some kind of error occurred, which is not further
characterized by the exit code.
10 - If checking if a pyramid is needed, this exit code indicates that a pyramid
is needed.
11 - It was determined that no pyramid needs to be created.
99 - There was an error parsing the command-line arguments.

```

## Options

### General Options
- **`-h, --help`**  
  Displays help information and exits.

- **`--version`**  
  Prints extended version information, including supported operations and available stream-classes, then exits.

### Input/Output Options
- **`-s, --source TEXT`**  
  Specifies the URI of the source CZI file.

- **`-d, --destination TEXT`**  
  Specifies the URI of the destination CZI file.

- **`-o, --overwrite`**  
  Overwrites the destination file if it already exists.

### Stream Configuration
- **`--source-stream-class STREAMCLASS`**  
  Specifies the stream class used for reading the source CZI file. If not provided, the default file-reader stream class is used.  
  Run with `--version` to view available stream-classes.  
  Possible stream-classes include:  
  - `curl_http_inputstream`  
  - `azure_blob_inputstream`  
  - `windows_file_inputstream` 
  - `pread_file_inputstream`
  - `c_runtime_file_inputstream`

- **`--propbag-source-stream-creation PROPBAG`**  
  Specifies a JSON-formatted property bag for stream creation when reading the source CZI file.

### Pyramid Generation Options
- **`-n, --mode-of-operation MODE`**  
  Defines the mode of operation:  
  - `IfNeeded` (default): Generates a pyramid only if it's not already present in the source.  
  - `Always`: Always generates a new pyramid, discarding any existing one.

- **`-m, --max_top_level_pyramid_size MAX_TOP_LEVEL_PYRAMID_SIZE`**  
  Sets the maximum size (in pixels) of the top-level pyramid tile. Defaults to **1024**.

- **`-t, --tile_size TILE_SIZE`**  
  Specifies the extent (in pixels) of pyramid tiles. Defaults to **1024**.

- **`-p, --threshold-for-pyramid THRESHOLD_SIZE`**  
  Defines the size threshold (in pixels) for requiring a pyramid. If the image is smaller than this size, no pyramid is created. Defaults to **4096**.

### Compression Options
- **`-c, --compressionopts COMPRESSIONOPTIONS`**  
  Configures the compression for pyramid sub-blocks.  
  - If not provided, the compression mode is derived from the source document.  
  - Format: `"compression_method: key=value; ..."`  
  - Examples:  
    - `"zstd0:ExplicitLevel=3"`  
    - `"zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack"`  
    - `"uncompressed:"`  
    - `"jpgxr:"`

### Other Options
- **`-v, --verbosity VERBOSITYLEVEL`**  
  Controls the verbosity level of console output. Acceptable values are between **0** (minimal output) and **5** (most detailed).

- **`-b, --background-color COLOR`**  
  Specifies the background color for the pyramid. Options:  
  - `black` or `0`  
  - `white` or `1`  
  - `auto` (auto-detect from source document).

- **`--check-only`**  
  Checks whether the source CZI already contains a pyramid and exits without making changes.
## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success / no pyramid needed (check-only mode) |
| 1 | General error |
| 10 | Pyramid needed (check-only mode) |
| 11 | No pyramid needed (with mode-of-operation=IfNeeded) |
| 99 | Error parsing the command line argument |

