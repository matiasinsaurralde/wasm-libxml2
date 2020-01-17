wasm-libxml2
==

A quick experiment to build and run `libxml2` as a WebAssembly module.

## Building libxml2

I've used the following commands and flags (from [wasienv](https://github.com/wasienv/wasienv)), using the source from `libxml-2.9.9`.

```
$ wasiconfigure  ./configure --enable-static --without-http --without-ftp --without-modules --without-python --without-zlib --without-lzma --without-threads --host=x86_64

$ wasimake make

$ wasicc HTMLparser.o HTMLtree.o SAX.o SAX2.o buf.o c14n.o catalog.o chvalid.o debugXML.o dict.o encoding.o entities.o error.o globals.o hash.o legacy.o list.o parser.o parserInternals.o pattern.o relaxng.o schematron.o threads.o tree.o uri.o valid.o xinclude.o xlink.o xmlIO.o xmlcatalog.o xmlmemory.o xmlmodule.o xmlreader.o xmlregexp.o xmlsave.o xmlschemas.o xmlschemastypes.o xmlstring.o xmlunicode.o xmlwriter.o xpath.o xpointer.o xzlib.o -Wl,--whole-archive,--export-all -o libxml2.wasm
```

## Calling libxml2

A toy program is available in `main.c`, it runs a slightly modified version of [wasm3](https://github.com/wasm3/wasm3).

Some simple function wrappers were written on top of `libxml2` in order to simplify this PoC.

`wasm_new_schema_parser2` instantiates a `xmlSchemaParserCtxtPtr`, a `xmlSchemaPtr` and a `xmlSchemaNewValidCtxt` on top of these two. This function takes two arguments, the first one is a buffer containing the XSD contents and the second one is the buffer length.
Returns a `xmlSchemaValidCtxtPtr`.

`wasm_validate_xml` takes a buffer too. Additionally the third parameter is expected to be a `xmlSchemaValidCtxtPtr` (as returned by `wasm_new_schema_parser2`).
The buffer is expected to be the XML contents, this is loaded using `xmlParseMemory` and finally validated with `xmlSchemaValidateDoc`. Negative return values represent internal or API errors ([source](http://www.xmlsoft.org/html/libxml-xmlschemas.html#xmlSchemaValidateDoc)).

## Building the toy program

`build.sh` will do the trick:

```
$ sh ./build.sh
```

The output looks as follows (note that there's an error in `input.xml` in order to trigger the parser error):

```
% ./main 
WASM module loaded
Runtime allocated memory: 458752 bytes
XSD file loaded (3231 bytes)
Allocated 458768 bytes, xsd_buf_ptr = 3231
Runtime allocated memory: 524288 bytes
XML file loaded (521 bytes)
Allocated 521 bytes, xml_buf_ptr = 489280
Entity: line 13: parser error : Opening and ending tag mismatch: namex line 13 and name
        <namex>Item 2</name>
                            ^
wasm_validate_xml = -1
```