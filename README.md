# Trico

Content
-------
* [Introduction](#introduction)
* [Building](#building)
* [Usage](#usage)
* [Tools](#tools)
* [Performance](#performance)
* [Format specification](#format-specification)
* [References](#references)

Introduction
------------
Trico is a C library for fast lossless compression and decompression of triangular 3D meshes and point clouds.
Currently Trico supports the compression of
  - 3D vertices of type `float`
  - 3D vertices of type `double`
  - 3D vertex normals of type `float`
  - 3D vertex normals of type `double`
  - 3D triangle normals of type `float`
  - 3D triangle normals of type `double`
  - uv coordinates per vertex of type `float`
  - uv coordinates per vertex of type `double`
  - uv coordinates per triangle of type `float`
  - uv coordinates per triangle of type `double`
  - vertex colors (rgba represented as a single `uint32_t`)
  - triangle colors (rgba represented as a single `uint32_t`)  
  - triangle indices of type `uint32_t`
  - triangle indices of type `uint64_t`
  - attribute lists of type `float`
  - attribute lists of type `double`
  - attribute lists of type `uint8_t`
  - attribute lists of type `uint16_t`
  - attribute lists of type `uint32_t`
  - attribute lists of type `uint64_t`
  
Compression and decompression with Trico should be fast and lossless. With lossless we mean that the original data can be perfectly reconstructed, i.e. there is no loss of accuracy in the vertex, normal, or uv data, but also the order of triangle indices does not change.

Building
--------
A solution/make file can be generated with CMake. Trico can be built as a shared library or as a static library. Set the CMake variable `TRICO_SHARED` to `yes` for a shared library or `no` for a static library.

Usage
-----
### Compression
Let's assume that we have a 3D mesh in memory that we want to compress. The 3D mesh consists of a number of 3D vertices, where each vertex is a triplet of floats, and a number of 32 bit triangle indices, so that a 3D triangle is represented by three consecutive indices, where each index refers to a vertex. The mesh can be represented by the following data:

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* triangles;

Note that Trico contains functionality to read [STL](https://en.wikipedia.org/wiki/STL_(file_format)) files and [PLY](https://en.wikipedia.org/wiki/PLY_(file_format)) files via the library [trico_io](https://github.com/janm31415/trico/tree/master/trico_io). The above data can be initialized by reading an STL file as follows:

    int read_successfully = trico_read_stl(&nr_of_vertices, &vertices, &nr_of_triangles, &triangles, "my_stl_file.stl");
    
Let's now write this data to a compressed file. We first create a Trico archive that we will need to pass as argument to all the subsequent function calls.

    void* arch = trico_open_archive_for_writing(1024 * 1024);
    
Trico uses an internal buffer to which the compressed data will be written. This buffer is maintained dynamically (i.e. it is enlarged if it is too small to contain the compressed data), but the above function call initializes the size of the buffer to whatever size the user provides as input. In this case the buffer is initialized to 1Mb.
Next we compress the vertices and the triangle indices.

    trico_write_vertices(arch, vertices, nr_of_vertices);
    trico_write_triangles(arch, triangles, nr_of_triangles);
    
We can now free the vertices and triangles that were allocated by our call to `trico_read_stl`.

    trico_free(vertices);
    trico_free(triangles);
    
The call to `trico_free` simply calls `free`, but it allows users to provide special allocation by overriding `trico_malloc`, `trico_calloc`, `trico_realloc`, and `trico_free`.

Next we save the compressed data to file.

    FILE* f = fopen("my_compressed_file.trc", "wb");
    if (f)
      {      
      fwrite((const void*)trico_get_buffer_pointer(arch), trico_get_size(arch), 1, f);
      fclose(f);      
      }
      
Here `trico_get_buffer_pointer` returns the internal buffer, and `trico_get_size` returns its size.

Finally we close the archive:

    trico_close_archive(arch);

### Decompression
To decompress a file, we first need to read it in memory. Getting the size of a file can be tricky. If the filesize is less than 2Gb you can get its size by

    FILE* f = fopen("my_compressed_file.trc", "rb");
    fseek(f, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(f, 0, SEEK_SET);
    
To get the filesize for files larger than 2Gb you can use the method `stat` on Linux, or `_stat64` on Windows:

    #ifdef _WIN32
    #include <sys/types.h>
    #include <sys/stat.h>
    #else
    #include <sys/stat.h>
    #endif
    
    #ifdef _WIN32
    static inline long long fsize(const char *filename)
      {
      struct _stat64 st;
    
      if (_stat64(filename, &st) == 0)
        return st.st_size;
    
      return -1;
      }
    #else
    static inline long long fsize(const char *filename) 
      {
      struct stat st;
    
      if (stat(filename, &st) == 0)
        return st.st_size;
    
      return -1;
      }
    #endif
    
    long long file_size = fsize("my_compressed_file.trc");
    
Once we have the filesize, we can read the file in a single buffer:

    char* buffer = (char*)malloc(file_size); 
    fread(buffer, 1, file_size, f);
    
If you work in C++, you can read the file as follows (assuming it is smaller than 2Gb):

    std::ifstream infile;
    infile.open("my_compressed_file.trc", std::ios::binary | std::ios::in);
    infile.seekg(0,std::ios::end);
    int file_size = infile.tellg();
    infile.seekg(0,std::ios::beg);
    char *buffer = new char[file_size];
    infile.read(data, file_size);
    infile.close();

Next we make a Trico archive for reading:

    void* arch = trico_open_archive_for_reading((const uint8_t*)buffer, file_size);
 
If we know the structure of the Trico archive, we can immediately call the correct reading methods in the right order. Let's assume the Trico archive was created as explained in the Compression section, i.e. first a stream of 3D float vertices, and then a stream of uint32_t triangle indices, then we can decompress as follows:

    uint32_t nr_of_vertices = trico_get_number_of_vertices(arch);
    float* vertices = (float*)malloc(nr_of_vertices * 3 * sizeof(float)); // allocate a buffer for the decompressed vertices
    trico_read_vertices(arch, &vertices);
    
    uint32_t nr_of_triangles = trico_get_number_of_triangles(arch);
    uint32_t* tria_indices = (uint32_t*)malloc(nr_of_triangles * 3 * sizeof(uint32_t)); // allocate a buffer for the decompressed triangle indices
    trico_read_triangles(arch, &tria_indices);
    
Now `vertices` and `tria_indices` contain the decompressed 3D mesh data. It's however possible that you do not exactly know the structure of the Trico archive. Then you can do something like this:

    uint32_t nr_of_vertices;
    float* vertices;
    uint32_t nr_of_triangles;
    uint32_t* tria_indices;
    
    enum trico_stream_type st = trico_get_next_stream_type(arch);
    while (!st == trico_empty)
      {
      switch (st)
        {
        case trico_vertex_float_stream:
          {
          nr_of_vertices = trico_get_number_of_vertices(arch);
          vertices = (float*)malloc(nr_of_vertices * 3 * sizeof(float)); // allocate a buffer for the decompressed vertices
          trico_read_vertices(arch, &vertices);
          break;
          }
        case trico_triangle_uint32_stream:
          {
          nr_of_triangles = trico_get_number_of_triangles(arch);
          tria_indices = (uint32_t*)malloc(nr_of_triangles * 3 * sizeof(uint32_t)); // allocate a buffer for the decompressed triangle indices
          trico_read_triangles(arch, &tria_indices);
          break;
          }
        default:
          {
          trico_skip_next_stream(arch);
          break;
          }
        }
      st = trico_get_next_stream_type(arch);
      }
    
  Finally we close the Trico archive and free the buffer.
  
      trico_close_archive(arch);
      free(buffer);
      
Tools
-----
Currently the source code will create two command line applications: `trico_encoder` and `trico_decoder`. If you run these tools from the command line without arguments you'll get an overview of their usage and options.

### trico_encoder
`trico_encoder` can read binary STL files and binary or ascii PLY files. As output it will generate a Trico-encoded file, containing the compressed data of the input file. There are some restrictions on the input PLY files however: when reading PLY files with double precision data, this double precision data will be converted to single precision data by the internal PLY reader, so there is some loss of accuracy here. Note that Trico can compress double precision data without loss of accuracy, but for simplicity of the reader in the encoding tool I've opted to only fully support single precision PLY files. If you need to compress PLY files with double precision this should be fairly straight forward: Essentially you can take the implementation in file [`ioply.c`](https://github.com/janm31415/trico/blob/master/trico_io/ioply.c) but replace `float` by `double` for the vertices/normals/texture data.

The basic usage of the encoder expects an input file and preferably also an output file. If an output file is omitted, `trico_encoder` will replace the extension of the input file by `.trc` and write to that file, but generally

    ./trico_encoder -i my_data/stl_file.stl  -o out.trc
    
will read the input file, and write the Trico-encoded file to `out.trc`.

There are some extra options available:

The STL file format contains, apart from vertices and triangles, also triangle normals and a 16 bit attribute value per triangle. By default, the triangle normals and the attribute values are not saved in the Trico-encoded output file. The reason being that triangle normals can simply be computed from the vertices and triangles, and thus they do not need to be saved, and the attribute values are mostly ignored. However, if you want to include the triangle normals or the attributes you should use

    ./trico_encoder -i my_data/stl_file.stl  -o out.trc -stladd normal -stladd uint16
    
If you use a PLY file as input for compression, all the recognized streams will be saved to the Trico-encoded output file. With the command `-plyskip` you can choose to skip certain attribute streams, e.g.

    ./trico_encoder -i my_data/ply_file.ply -o out.trc -plyskip color

### trico_decoder
`trico_decoder` reads Trico-encoded files, decompresses the data, and writes the output to a STL or PLY file:

    ./trico_decoder -i in.trc -o out.stl

Performance
-----------
The following results are an indication of performance. The compression ratio depends on the order of the triangles and vertices in the input file, which may vary depending on the program that was used to generate the input file.
The files were taken from the [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/). The Stanford bunny that I used in the tests was obtained from another source, as the original ply file contains 2 extra attribute streams which are ignored by `trico_encoder`.

The compression ratios versus STL files are quite high. This is because the STL format contains a lot of redundant information. First of all triangle normals can be computed from the vertices and the triangles, and thus do not need to be saved. Second, the 16 bit attribute data in the STL file is typically unused, and thus can be removed. Finally, the STL file format saves data per triangle. A typical vertex belongs on average to 6 triangles. The STL file format will thus save this vertex 6 times. The `trico_encoder` tool will read the STL file, and convert it to an indexed format where each vertex is in a list, and triangles refer to vertices via indices (similar to how a PLY file builds its 3D mesh structure). Then the vertex list and triangle list are compressed. This explains the higher compression ratios for STL files.

The true compression ratio measure for Trico is thus the compression ratio compared to the binary PLY file. In the table below I've also included a zipped version of the binary PLY file. Grosso modo the compression ratios of Trico and a zipped PLY file are comparable, but compression and decompression with Trico is much faster than zipping.

Model | Triangles | Vertices | Binary STL | Binary PLY | Binary PLY zipped | Trico | Compression ratio vs STL | Compression ratio vs PLY | Compression ratio vs PLY zipped
----- | --------- | -------- | ---------- | ---------- | ----------------- | ----- | ------------------------ | ------------------------ | -------------------------------
Stanford Bunny | 69451 | 35947 | 3392 KB | 1291 KB | 522 KB | 571 KB | 5.94 | 2.26 | 0.91
Happy Buddha | 1087716 | 543652 | 53112 KB | 20180 KB | 10135 KB | 9146 KB | 5.81 | 2.21 | 1.11
Dragon | 871414 | 437645 | 42550 KB | 16192 KB | 8129 KB | 7274 KB | 5.85 | 2.23 | 1.12
Armadillo | 345944 | 172974 | 16892 KB | 6757 KB | 3794 KB | 4059 KB | 4.16 | 1.66 | 0.93
Lucy | 28055742 | 14027872 | 1369910 KB | 520566 KB | 296014 kB | 230609 KB | 5.94 | 2.26 | 1.28
Asian Dragon | 7219045 | 3609600 | 352493 KB | 133949 KB | 68541 KB | 49896 KB | 7.06 | 2.68 | 1.37
Vellum manuscript* | 4305818 | 2155617 | 210246 KB | 86241 KB | 42783 KB | 23465 KB | 8.96 | 3.68 | 1.82
Thai Statue | 10000000 | 4999996 | 488282 KB | 185548 KB | 104048 KB | 86165 KB | 5.67 | 2.15 | 1.21

\* the PLY and Trico file contain vertex colors, the STL file does not.

Format specification
--------------------
The structure of a Trico-encoded file is as follows:

    <Header>
    <Body>

The header looks as follows:

Offset | Type | Description
------ | ---- | -----------
0 | uint32_t | Magic identifier (`0x6f637254`, or "Trco" when read as ascii)
4 | uint32_t | Version number (Currently '0x00000000')

The body can be empty, but typically it consists of a number of streams of a certain type. These stream types are exactly equal to the `enum trico_stream_type` in file [trico.h](https://github.com/janm31415/trico/blob/master/trico/trico.h). One such stream block looks as follows:

Offset | Type | Description
------ | ---- | -----------
0 | uint8_t | stream type (corresponds with `enum trico_stream_type` in [trico.h](https://github.com/janm31415/trico/blob/master/trico/trico.h)
1 | uint32_t | length data of uncompressed stream 
5 | | compressed stream

The length data does not necessarily equal the number of bytes of the uncompressed stream. For instance for vertex data the length data equals the number of vertices, but the byte length would then be the number of vertices times `3` times `sizeof(float)`.
The length data of uncompressed streams is necessary for the decompression of Trico-encoded files. This allows the user to assign sufficient memory for capturing the decompressed data.

For the compressed stream we distinguish between floating point types and integer types.

Floating point data is first converted from an array of structures to a structure of arrays. Let's take the example of vertices. The input is a list

    x1, y1, z1, x2, y2, z2, ..., xn, yn, zn
    
and this list is converted to

    x1, x2, ..., xn, y1, y2, ..., yn, z1, z2, ..., zn.
    
Arranging the data like this makes it hopefully easier for a prediction algorithm to guess the next floating point value, so that we get better compression. The compression algorithm for a list of floating point values is based on the paper "High Throughput Compression of Double-Precision Floating-Point Data" by Martin Burtscher and Paruj Ratanaworabhan, but with some modifications, as the paper is focused on double precision, and we are mainly interested in single precision.

Integer data is also rearranged first. We apply byte interleaving. Suppose we have an integer array where an integer consists of four bytes `a`, `b`, `c`, and `d`, then we rearrange as follows:

    a1, b1, c1, d1, a2, b2, c2, d2, ..., an, bn, cn, dn
    
is converted to

    a1, a2, ..., an, b1, b2, ..., bn, c1, c2, ..., cn, d1, d2, ..., dn.
    
Next we use [LZ4](https://github.com/lz4/lz4) to compress the integer data.

The floating point and integer compression methods that are used in Trico are designed to be fast. We could have used other compression algorithms such as [Zlib](https://zlib.net/) that give higher compression ratios, but at the cost of speed. If high compression ratio is the goal, and speed is not an issue, then we refer to [OpenCTM](http://openctm.sourceforge.net/).

References
----------
For fast compression of integer types I use [LZ4](https://github.com/lz4/lz4).
The compression of floating point and double precision streams is based on the paper "High Throughput Compression of Double-Precision Floating-Point Data" by Martin Burtscher and Paruj Ratanaworabhan, with some modifications.
For reading ply files I use [rply](http://w3.impa.br/~diego/software/rply/).
