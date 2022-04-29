SPK Compiler v1.1


-- ABOUT --

A simple program to easily convert full-layer bitmaps directly to spk layers
without the hassle of manually manipulating individual stars. SpkComp will
also break up larger images, which may have clipping/refresh issues within
StarCraft, into images no larger than 8x8 pixels.



-- COMPILING --

Drag & drop the first sequentially numbered bitmap on to the EXE, and it will
automatically detect subsequent images and create an SPK file with the same
filename. Dragging more than one file will work, but there is no way to
guarantee what order they will be loaded.

You may also explicitly list each layer on the command line and optionally
specify the SPK output filename.
Example:
SpkComp layer1.bmp layer2.bmp bob.bmp george.bmp out.spk

* Images must be 8-bit bitmaps with no compression and the appropriate palette.
  SpkComp does no color correction or conversion and will copy the image data
  as-is.
* Layers are 648x488 pixels. Images smaller than this will be positioned in the
  top-left corner of the layer, and larger images will be clipped.
* StarCraft supports a maximum of 5 layers, but SpkComp has an arbitrary limit
  of 20.



-- DECOMPILING --

Drag & drop an SPK file on to the EXE, and it will render and export each layer
as a separate bitmap with the same filename and layer ID.

As with compiling, you may use the command-line to first specify the SPK file
and use subsequent arguments to specify the bitmap output filenames.
Example:
SpkComp in.spk layer1.bmp layer2.bmp bob.bmp george.bmp

* As of right now there is no support for specifying a custom WPE, and SpkComp
  will defaultly load "platform.wpe" when creating bitmaps.





Special thanks to Corbo for testing & encouraging me to make this.







Version History:

1.1.0: - Fixed SPK format
       - Increased layer count to 20

1.0.0: - Initial release
