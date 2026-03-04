# The OSRM Build Process Explained

> [!TIP]
> To learn more about the build process, start with studying these files:
>
> - `/CMakeLists.txt`
> - `/conanfile.py`
> - `.github/workflows/osrm-backend.yml`
> - all files under `.github/actions`
> - `scripts/ci/decode_matrix.py`

## The Complication

The build must work with most of these combinations:

| Location | OS      | Deps    | Builder | Compiler | Linker | Type    |
| -------- | ------- | ------- | ------- | -------- | ------ | ------- |
| home     | Linux   | Conan   | Make    | clang    | static | Release |
| github   | macOS   | apt-get | Ninja   | gcc      | shared | Debug   |
|          | Windows |         | MSBuild | msvc     |        |         |
|          |         |         | Xcode   |          |        |         |

### Communication between build stages

The `build` directory is a hardcoded 'well-known'
location. The `build` directory is used for communication
between the stages of the build toolchain even if the actual build of
the binaries should take place in `build/Release`,
`build/Debug`, or elsewhere. As the build progresses, more
and more information becomes available in `build/`.

| Program            | produces             | and                            |
| ------------------ | -------------------- | ------------------------------ |
| `decode_matrix.py` | `build/matrix.env`   | `/CMakePresets.json`           |
| `conan install`    | `build/conan.env`    | `generators/CMakePresets.json` |
| `cmake -B`         | `build/osrm-run.env` | `build/osrm-run-env.sh`        |

The `.env` files only contain `KEY=VALUE` pairs, so that they can be safely sourced into
a shell, or piped into `$GITHUB_ENV`. `CMakePresets.json` files are consumed by `cmake`
(and also by `conan`). The scripts `build/osrm-run-env.sh` is a convenient way to source
the whole environment in one step.

There is a gotcha: if the `file.env` file contains a `KEY="Value with spaces"` pair
(note the quotes), `source file.env` _will_ remove those quotes but
`cat file.env >> $GITHUB_ENV` _will not_ remove those quotes.
