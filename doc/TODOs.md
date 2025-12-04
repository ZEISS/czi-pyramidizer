
* add "mask-support" (i.e. generation of valid-pixel-mask)
* use "tile-caching" (c.f. https://github.com/ZEISS/libczi/issues/77 , https://github.com/ZEISS/libczi/blob/8474da3d3dd2033491ac48bfe41b2723273979de/Src/CZICmd/executePlaneScan.cpp#L29C6-L35C10)
* ~~make curl/azure-streams available~~
* ~~have some progress reporting~~
* ~~error handling~~
* test with "a real documet" (-> F:\CZIcmd.exe --command CreateCZI --createbounds "T0:1C0:1"  --createtileinfo "8x8;10%"  --compressionopts "zstd1:ExplicitLevel=2;PreProcess=HiLoByteUnpack" --generatorpixeltype Gray8  --createsubblocksize "1024x1024" -o F:\pyramidizer\testczi)
* ~~fix "uneven coordinate mess"~~ at least some attempts made to migate this
* implement "in-place operation"
* implement "temporarily swap tiles"
* ~~tweak "pyramid-metadata"~~
* improve performance...
