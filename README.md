# Pokémon Emerald

This is a decompilation of Pokémon Emerald that integrates the Mobile Adapter library (libma).

This branch **needs** to be compiled with `make modern` to work (I don't know how to make it work with `agbcc`, if you do please make a pull request).

Some helper functions have been created in `mobile_adapter.c` to interact with the library with comments to explain each one.
An example function is also included which downloads a Pokémon from a server and adds it to your party. The easiest way to test this would be to make the function a `special` and call it from an overworld script. The function sends a POST request to the server (with content `GIFT`) and the server should respond with the Pokémon data (100 bytes) in the packet's content.

It builds the following ROM:

* [**pokeemerald.gba**](https://datomatic.no-intro.org/index.php?page=show_record&s=23&n=1961) `sha1: f3ae088181bf583e55daf962a92bb46f4f1d07b7`

To set up the repository, see [INSTALL.md](INSTALL.md).

For contacts and other pret projects, see [pret.github.io](https://pret.github.io/).
