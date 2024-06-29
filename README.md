# Pokémon Emerald

This is a decompilation of Pokémon Emerald that integrates the Mobile Adapter library (libma).

Some helper functions have been created in [mobile_adapter.c](src/mobile_adapter.c) to interact with the library with comments to explain each one, so it's recommended to read that file first.
An example function is also included which downloads a Pokémon from a server and adds it to your party. The easiest way to test this would be to make the function a `special` and call it from an overworld script. The function sends a POST request to the server (with content `GIFT`) and the server should respond with the Pokémon data (100 bytes) in the packet's content.

This branch is only for the library integration and helper functions to act as a base for people to build on. I am planning to extend the features in Emerald (Online trading and battling etc.) which I'll make another branch for.

## Requirements
This branch **needs** to be compiled with `make modern` to work and so will require devkitARM (check [INSTALL.md](INSTALL.md) for how to install that). I don't know how to make it work with `agbcc`, if you do please make a pull request.

To test this you will need a DNS server that resolves domain queries and a HTTP server that recieves and responds to the HTTP requests.
You will also need to use [this fork](https://github.com/Wit-MKW/mgba) of mGBA which has Mobile Adapter functionality. The Mobile Adapter settings would need to have the IP address of your DNS server added as the primary and secondary DNS. The adapter is normally setup with the [Mobile Trainer](https://bulbapedia.bulbagarden.net/wiki/Mobile_Game_Boy_Adapter#Mobile_Trainer), but haven't tested yet if that's strictly necessary.

### Credits
pfero - Decompiling libma and helping with troubleshooting

To set up the repository, see [INSTALL.md](INSTALL.md).