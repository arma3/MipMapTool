```
Hewo! I'm the MipMap tool and I'll Map Mips for you.

I can unpack your textures like so:
  mipmaptool unpack "P:/file1_co.paa" "P:\file2_co.paa"
I can combine the best mipmaps like so:
  mipmaptool "P:/tex_mip4096_co.paa" "P:\tex_mip2048_co.paa" "P:/tex_mip4_co.paa"
 These filenames have to be in a specific format xxx_mip1234_yy.paa, output file will be xxx_yy.paa
 (for you nerds out there the regex is (.*)_mip(\d*)(.*)\.paa )
Or I also can combine the best mipmaps like so:
  mipmaptool output "P:/output_co.paa" "P:/file1_co.paa" "P:\file2_co.paa"
 (which writes into specified output file)
File paths have to always be encased in quotes like shown.
Windows does that automatically if you just drag&drop files onto the binary.
As you can see / and \ are allowed, anything that works in windows should work.
```

<a href="https://www.patreon.com/join/dedmen">
    <img src="https://c5.patreon.com/external/logo/become_a_patron_button.png" alt="Become a Patron!">
</a>