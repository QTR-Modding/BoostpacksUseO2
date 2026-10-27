# Corresponding Source

The distributed `O2BoostRecharge.dll` statically incorporates code from these
exact revisions:

- [CommonLibSF](https://github.com/QTR-Modding/commonlibsf) commit
  `84f8589b15a6d588d28da16baa7459a1fc414ea7`
- [commonlib-shared](https://github.com/libxse/commonlib-shared) commit
  `40bdbcaf8fa691ee6daadc7b5cea8d43f794bef4`
- [SFSE-MCP](https://github.com/QTR-Modding/SFSE-MCP) commit
  `5bf93bf2b22f2c70c2c67f9b6cefa353d164f85c`
- [spdlog](https://github.com/gabime/spdlog) v1.16.0, commit
  `486b55554f11c9cccc913e11a87085b2a91f706f`

The Git repository pins CommonLibSF, its nested commonlib-shared submodule, and
SFSE-MCP. Because GitHub's automatic source archives do not expand submodules,
clone the repository, check out the matching build commit, and run
`git submodule update --init --recursive` to retrieve the linked dependencies.

Build instructions are in [README.md](README.md). Complete license and exception
texts are in `COPYING`, `EXCEPTIONS`, and `LICENSES/`.

Written offers or source questions may be filed through this repository's issue
tracker.
