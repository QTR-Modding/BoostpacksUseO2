# Corresponding Source

The distributed `O2BoostRecharge.dll` statically incorporates code from these
exact revisions:

- [CommonLibSF](https://github.com/libxse/commonlibsf) commit
  `84f8589b15a6d588d28da16baa7459a1fc414ea7`
- [commonlib-shared](https://github.com/libxse/commonlib-shared) commit
  `40bdbcaf8fa691ee6daadc7b5cea8d43f794bef4`
- [spdlog](https://github.com/gabime/spdlog) v1.16.0, commit
  `486b55554f11c9cccc913e11a87085b2a91f706f`

The Git repository pins CommonLibSF and its nested commonlib-shared submodule.
Because GitHub's automatic source archives do not expand submodules, each binary
release also includes `BoostpacksUseO2-v0.4.1-source.zip`. That asset contains
the complete project source and build scripts plus expanded source trees for
all three linked dependencies.

Build instructions are in [README.md](README.md). Complete license and exception
texts are in `COPYING`, `EXCEPTIONS`, and `LICENSES/`.

Written offers or source questions may be filed through this repository's issue
tracker.
