# Contributing

Guidelines for contributing to `czi-pyramidizer`.

## Code of Conduct

Follow respectful, inclusive communication. (Optionally link to a CODE_OF_CONDUCT if added.)

## Workflow

1. Fork & branch from `main`.
2. Make focused changes with clear commits.
3. Add / update unit tests.
4. Run formatting / lint (if configured).
5. Open a Pull Request describing motivation & changes.

## Commit Messages

- Use imperative: `Add decimation interface`, `Fix tile bounds check`.
- Reference issues: `Fix #42` when applicable.

## Testing

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build -C Debug --output-on-failure
```

Add new tests under `libpyramidizer-unittests/`.

## Style

- C++17
- Prefer `std::unique_ptr` / `std::shared_ptr` to raw owning pointers.
- Delete copy/move where inappropriate (interfaces) as already done with `IDecimationProcessing`.
- Keep headers minimal; forward declare when possible.

## Adding Dependencies

- Favor small, well-maintained libraries.
- For vcpkg dependencies, update CI workflow caching logic if necessary.

## Documentation

- Update `docs/` Markdown for user-visible changes.
- Keep `mkdocs.yml` navigation in sync.

## Release Process (Proposed)

1. Tag version `vX.Y.Z`.
2. CI builds release artifacts.
3. Update changelog & site.

## Reporting Issues

Include:
- Version / commit hash
- Platform & compiler
- Reproduction steps
- Expected vs actual behavior

## Security

Report potential vulnerabilities privately (define contact method).

## License

State project license and ensure all contributions are compatible.
