# Terrain Viewer
Display elevation data using OpenGL

## Compilation
    $ make

## Usage
    /bin/terrain-viewer [ FILE ]

    FILE
        Formatted elevation file. Reads from standard input if no file is given.
        First number in file should be the number of columns followed by the 
        number of rows. The third number is the resolution of the elevation 
        data. The rest of the file should contain a minimum of (ncols x nrows) 
        elevations.