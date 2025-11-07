# Usage Examples

Practical scenarios for `czi-pyramidizer`.

## 1. Basic Pyramid Generation

```
czi-pyramidizer sample.czi sample_pyramid.czi
```

If the source already contains a full pyramid, the tool exits quickly.

## 2. Force Overwrite Destination

```
czi-pyramidizer --overwrite sample.czi sample_pyramid.czi
```

## 3. Check Only (Validation in Pipelines)

```
if czi-pyramidizer --check sample.czi dummy.czi; then
  echo "No pyramid needed"
else
  rc=$?
  if [ "$rc" -eq 2 ]; then
    echo "Pyramid required"
  else
    echo "Error (exit $rc)" >&2
  fi
fi
```

## 4. Custom Stream Class

```
czi-pyramidizer \
  --source-stream-class MyRemoteStream \
  --stream-prop endpoint=https://storage.example.com/blob.czi \
  --stream-prop sas_token=...? \
  remote.czi local_output.czi
```

## 5. CI Integration (GitHub Actions)

Example job step:
```
- name: Pyramidize CZI
  run: |
    czi-pyramidizer input.czi output.czi
```

## 6. Batch Processing Script

```
for f in *.czi; do
  out="${f%.czi}_pyr.czi"
  echo "Processing $f ? $out"
  if ! czi-pyramidizer "$f" "$out"; then
    echo "Failed: $f" >&2
  fi
done
```

## 7. Collect Timing (Linux)

```
/usr/bin/time -v czi-pyramidizer big.czi big_pyr.czi
```

## 8. Dry Run Planning (Future Idea)

A planned `--plan` option could list expected layers & tiles without executing.

## Tips

- Place temporary files on fast local SSD if input is remote.
- Ensure enough free disk space (pyramid can add significant size initially).
- Use appropriate thread settings if running in a constrained container.
