# Pokémon Emerald

This is a decompilation of Pokémon Emerald that restores JAPANESE e-Reader functionnalities. This Multi-Language repo is based on two different projects :
* AsparagusEduardo's pokeemerald-pret_europe for english, french, italian and spanish support. https://github.com/AsparagusEduardo/pokeemerald/tree/pret_europe
* Paccy's pokeemerald multiboot-fix for japanese e-Reader restoration. https://github.com/Artrios/pokeemerald/tree/multiboot_fix

By default this is building an english rom.
To change language, edit the line 8 of the Makefile then save :
* LANGUAGE    ?= FRENCH
* LANGUAGE    ?= ITALIAN
* LANGUAGE    ?= SPANISH

The e-cards can be found :
* ENGLISH [here](https://github.com/Youpileouf/Pokemon-e-Cards-English)
* FRENCH [here](https://github.com/Youpileouf/Pokemon-e-Cards-France)
* ITALIAN work in progress
* SPANISH work in progress

It builds the following ROMs:

* English: [**pokeemerald.gba**](https://datomatic.no-intro.org/index.php?page=show_record&s=23&n=1961) `sha1: f3ae088181bf583e55daf962a92bb46f4f1d07b7`
* French: [**pokeemerald_fr.gba**](https://datomatic.no-intro.org/index.php?page=show_record&s=23&n=2161) `sha1: ca666651374d89ca439007bed54d839eb7bd14d0`
* Italian: [**pokeemerald_it.gba**](https://datomatic.no-intro.org/index.php?page=show_record&s=23&n=2181) `sha1: 1692db322400c3141c5de2db38469913ceb1f4d4`
* Spanish: [**pokeemerald_es.gba**](https://datomatic.no-intro.org/index.php?page=show_record&s=23&n=2164) `sha1: fe1558a3dcb0360ab558969e09b690888b846dd9`

To set up the repository, see [INSTALL.md](INSTALL.md).

For contacts and other pret projects, see [pret.github.io](https://pret.github.io/).
