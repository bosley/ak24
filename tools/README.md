# AK24 Tools

Utility scripts for managing the AK24 project.

## Version Management

### set_version.sh

Automatically updates version strings across the entire project.

**Usage:**
```bash
./tools/set_version.sh
```

The script reads from `VERSION` file in the project root and updates:
- All kernel module headers (`AK24_*_VERSION` defines)
- All SVG logos (version text)
- CMakeLists.txt (project VERSION)
- Doxyfile (PROJECT_NUMBER)
- Documentation template

**To change the version:**
1. Edit the `VERSION` file in project root
2. Run `./tools/set_version.sh`
3. Run `make docs` to regenerate documentation

**Example:**
```bash
echo "0.1.0-alpha" > VERSION
./tools/set_version.sh
make docs
```

### Version Format

Follow semantic versioning with optional pre-release tags:
- `0.0.1-dev` - Development/unstable
- `0.1.0-alpha` - Alpha release
- `0.1.0-beta` - Beta release
- `0.1.0-rc1` - Release candidate
- `0.1.0` - Stable release
- `1.0.0` - Major stable release

## Notes

- The `VERSION` file is the single source of truth for project version
- CMake reads the version for build configuration
- Doxygen uses it for documentation generation
- All module headers are kept in sync automatically
