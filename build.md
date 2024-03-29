# Building From Source

1) `git clone --recursive https://github.com/alex-free/dreamcast-cdi-burner`

2) `cd dreamcast-cdi-burner`

3) `./build`

All build dependencies will be downloaded (if you have the APT package manager, so Ubuntu/Debian/derivatives like Pop!OS). Feel free to open a [pull request](https://github.com/alex-free/dreamcast-cdi-burner/pulls) to add support for other package managers/distributions.

All software dcdib depends on will be compiled and then made portable by my [PLED](https://github.com/alex-free/pled) tool.

A release `.zip` file will be generated by the `build` script. A release directory will also be created, allowing you to immediately use the DCDIB open source toolkit.

If you want to clean your DCDIB source tree of all built binaries/releases, simply execute `./build clean`.  