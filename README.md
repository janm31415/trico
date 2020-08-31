# Trico

Introduction
------------
Trico is a C library for lossless compression and decompression of triangular 3D meshes and point clouds.
Currently Trico supports the compression of
  - 3D vertices of type `float`
  - 3D vertices of type `double`
  - 3D normals of type `float`
  - 3D vertices of type `double`
  - uv coordinates of type `float`
  - uv coordinates of type `double`
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

Note that Trico contains functionality to read [STL](https://en.wikipedia.org/wiki/STL_(file_format)) files. The above data can be initialized by reading an STL file as follows:

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
    
The call to `trico_free` simply calls `free`, but it allows users to provide special allocation by overriding `trico_malloc`, `trico_realloc`, and `trico_free`.

Next we save the compressed data to file.

    FILE* f = fopen("compressed.trc", "wb");
    if (f)
      {      
      fwrite((const void*)trico_get_buffer_pointer(arch), trico_get_size(arch), 1, f);
      fclose(f);      
      }
      
Here `trico_get_buffer_pointer` returns the internal buffer, and `trico_get_size` returns its size.

Finally we close the archive:

    trico_close_archive(arch);
