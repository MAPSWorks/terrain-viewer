/**
 * init.c
 */
#include <assert.h>
#include <stdio.h>
#include "init.h"
#include "terrain.h"
#include "shader.h"

worldData world;
cameraData camera;

void
init_world_data(worldData * const w) {
    // The length in GL coord of one side of the (x,z) plane
    w->cube_size = 100.0f;

    // Wireframe (0=off, 1=on)
    w->wireframe_mode = 0;

    // Polygon fill mode (0=none, 1=fill, 2=point)
    w->fill_mode = 1;

    // Location and properties of light representing the sun
    w->sun_theta = 0;
    vec4_init( &w->sun_light.position, 0.0f, w->cube_size, 0.0f, 1.0f );
    vec4_init( &w->sun_light.ambient, 1.0f, 1.0f, 1.0f, 1.0f );
    vec4_init( &w->sun_light.diffuse, 1.0f, 1.0f, 1.0f, 1.0f );
    vec4_init( &w->sun_light.specular, 1.0f, 1.0f, 1.0f, 1.0f );
    
    // Light properties of the terrain
    vec4_init( &w->ground_material.ambient, 0.1f, 0.1f, 0.2f, 1.0f );
    vec4_init( &w->ground_material.diffuse, 1.0f, 1.0f, 1.0f, 1.0f );
    vec4_init( &w->ground_material.specular, 0.2f, 0.2f, 0.2f, 1.0f );
    w->ground_material.shininess = 30.0f;
}

void 
init_camera_data(cameraData * const c, GLfloat cube_size) {
    // The location of the camera in world coordiantes
    c->viewer[0] = 0.0f;
    c->viewer[1] = cube_size / 2.5f;
    c->viewer[2] = -cube_size;

    // The rotation of the camera in degrees
    c->theta[0] = 15.0f;
    c->theta[1] = 180.0f;
    c->theta[2] = 0.0f;

    c->last_mouse_x = -1;
    c->last_mouse_y = -1;
}

/**
 *  Load and store map data from a file
 *  @param[out] mData  The map data read from the file
 *  @param[in] fileData  The file to read from
 *  @param[in] worldData  The current world
 */
void 
load_file(mapData * const mData, 
          FILE * const fileData, 
          worldData const * const w) {
    // Read map height/width and initialize scaling coefficients
    fscanf( fileData, "%u", &mData->mapWidth );
    fscanf( fileData, "%u", &mData->mapHeight );

    GLfloat const fx = (GLfloat) mData->mapWidth;
    GLfloat const fz = (GLfloat) mData->mapHeight;

    if(mData->mapWidth > mData->mapHeight) {
        mData->scale = w->cube_size / fx;
    }else {
        mData->scale = w->cube_size / fz;
    }
    mData->xOffset = (mData->scale * fx) / 2.0f;
    mData->zOffset = (mData->scale * fz) / 2.0f;

    // Resolution
    GLfloat resolution;
    fscanf( fileData, "%f", &resolution );
    mData->yScale = mData->scale / resolution;

    GLfloat maxElevation = 0.0f;
    GLfloat minElevation = 0.0f;
    mData->elevationData = malloc(mData->mapHeight 
                                  * sizeof(*mData->elevationData));
    unsigned int row, col;
    for(row = 0; row < mData->mapHeight; row++) {
        mData->elevationData[row] = malloc(mData->mapWidth 
                                        * sizeof(*mData->elevationData[row]));
        for(col = 0; col < mData->mapWidth; col++) {
            GLfloat input;
            fscanf( fileData, "%f", &input ); 
            if(input < 0.0f) {
                input = 0.0f;
            }
            mData->elevationData[row][col] = input;
            if(input > maxElevation) {
                maxElevation = input;
            }

            if(input > 0.0f) {
                if(minElevation == 0.0f || input < minElevation) {
                    minElevation = input;
                }
            }
        }
    }
    fclose( fileData );

    mData->maxElevation = maxElevation;
    mData->minElevation = minElevation;
}

/**
 *  Convert a point from the (x,z) index to the a 4-dimensional point in
 *  world coordinates
 *  @param[out] v  The 4 dimensional point in world coordinates
 *  @param[in] x  The x index of the point in the map
 *  @param[in] z  The z index of the point in the map
 *  @param[in] mData  The current map
 */
void 
make_vertex(vec4 * const v, int x, int z, mapData const * const mData) {
    v->x = mData->scale * x - mData->xOffset;

    GLfloat const y = mData->elevationData[z][x] - mData->minElevation;
    v->y = mData->yScale * y;

    v->z = mData->scale * z - mData->zOffset;
    v->w = 1.0f;
}

/**
 *  Given (x,z) find the normal for (a)   
 *      + 
 *    a | b
 *  +-(x,z)-+ 
 *    d | c 
 *      +
 *  @param[out] n  The flat normal
 *  @param[in] x  The x index of the point in the map
 *  @param[in] z  The z index of the point in the map
 *  @param[in] mData  The current map
 */
void
make_normal_top_left(vec3 * const n, int x, int z, mapData const * const mData) {
    if(x <= 0 || z <= 0) {
       vec3_init( n, 0.0f, 0.0f, 0.0f );
       return;
    }
    vec4 vertex;
    make_vertex( &vertex, x, z, mData );

    vec4 u;
    vec4 vertex_d;
    make_vertex( &vertex_d, x-1, z, mData );
    vec4_sub(&u, &vertex, &vertex_d );

    vec4 v;
    make_vertex( &vertex_d, x, z-1, mData );
    vec4_sub(&v, &vertex_d, &vertex );
    
    vec3 c;
    vec4_cross( &c, &u, &v );
    vec3_norm( n, &c );
}

/**
 *  Given (x,z) find the normal for (b)   
 *      + 
 *    a | b
 *  +-(x,z)-+ 
 *    d | c 
 *      +
 *  @param[out] n  The flat normal
 *  @param[in] x  The x index of the point in the map
 *  @param[in] z  The z index of the point in the map
 *  @param[in] mData  The current map
 */
void
make_normal_top_right(vec3 * const n, int x, int z, mapData const * const mData) {
    if(x >= mData->mapWidth - 1 || z <= 0) {
       vec3_init( n, 0.0f, 0.0f, 0.0f );
       return;
    }
    vec4 vertex;
    make_vertex( &vertex, x, z, mData );

    vec4 u;
    vec4 vertex_d;
    make_vertex( &vertex_d, x, z-1, mData );
    vec4_sub(&u, &vertex, &vertex_d );

    vec4 v;
    make_vertex( &vertex_d, x+1, z, mData );
    vec4_sub(&v, &vertex_d, &vertex );
    
    vec3 c;
    vec4_cross( &c, &u, &v );
    vec3_norm( n, &c );
}

/**
 *  Given (x,z) find the normal for (c)   
 *      + 
 *    a | b
 *  +-(x,z)-+ 
 *    d | c 
 *      +
 *  @param[out] n  The flat normal
 *  @param[in] x  The x index of the point in the map
 *  @param[in] z  The z index of the point in the map
 *  @param[in] mData  The current map
 */
void
make_normal_bot_right(vec3 * const n, int x, int z, mapData const * const mData) {
    if(x >= mData->mapWidth - 1 || z >= mData->mapHeight - 1) {
       vec3_init( n, 0.0f, 0.0f, 0.0f );
       return;
    }
    vec4 vertex;
    make_vertex( &vertex, x, z, mData );

    vec4 u;
    vec4 vertex_d;
    make_vertex( &vertex_d, x+1, z, mData );
    vec4_sub(&u, &vertex, &vertex_d );

    vec4 v;
    make_vertex( &vertex_d, x, z+1, mData );
    vec4_sub(&v, &vertex_d, &vertex );
    
    vec3 c;
    vec4_cross( &c, &u, &v );
    vec3_norm( n, &c );
}

/**
 *  Given (x,z) find the normal for (d)   
 *      + 
 *    a | b
 *  +-(x,z)-+ 
 *    d | c 
 *      +
 *  @param[out] n  The flat normal
 *  @param[in] x  The x index of the point in the map
 *  @param[in] z  The z index of the point in the map
 *  @param[in] mData  The current map
 */
void
make_normal_bot_left(vec3 * const n, int x, int z, mapData const * const mData) {
    if(x <= 0 || z >= mData->mapHeight - 1) {
       vec3_init( n, 0.0f, 0.0f, 0.0f );
       return;
    }
    vec4 vertex;
    make_vertex( &vertex, x, z, mData );

    vec4 u;
    vec4 vertex_d;
    make_vertex( &vertex_d, x, z+1, mData );
    vec4_sub(&u, &vertex, &vertex_d );

    vec4 v;
    make_vertex( &vertex_d, x-1, z, mData );
    vec4_sub(&v, &vertex_d, &vertex );
    
    vec3 c;
    vec4_cross( &c, &u, &v );
    vec3_norm( n, &c );
}


/**
 *  Return the normal for a given point by averaging the flat normals of 
 *  the faces surrounding it.
 *  @param[out] v  The averaged normal
 *  @param[in] x  The x index of the point in the map
 *  @param[in] z  The z index of the point in the map
 *  @param[in] mData  The current map 
 */
void
get_average_normal(vec3 * const v, 
                   unsigned int x, 
                   unsigned int z, 
                   mapData const * const mData) {
    vec3 n1 = { 0.0f, 0.0f, 0.0f };
    vec3 n2 = { 0.0f, 0.0f, 0.0f };
    vec3 n3 = { 0.0f, 0.0f, 0.0f };
    vec3 n4 = { 0.0f, 0.0f, 0.0f };
    make_normal_top_left( &n1, x, z, mData );
    make_normal_top_right( &n2, x, z, mData );
    make_normal_bot_left( &n3, x, z, mData );
    make_normal_bot_right( &n4, x, z, mData );

    vec3 sum;
    vec3_add( &sum, &n3, &n4 );
    vec3_add( &sum, &n2, &sum );
    vec3_add( &sum, &n1, &sum );
    vec3_norm( v, &sum );
}

/**
 *  Initialize the display state using elevation data from a FILE
 *  @param[in] file  The file to load the elevation data from. 
 */
void
init(FILE* const file) {
    init_world_data( &world );
    init_camera_data( &camera, world.cube_size );

    mapData mData;
    load_file( &mData, file, &world );

    world.num_vertices = (mData.mapHeight - 1) * (mData.mapWidth * 2) + 2;

    vec4* const vertices = malloc(world.num_vertices * sizeof(*vertices));
    vec3* const normals = malloc(world.num_vertices * sizeof(*normals));

    // Calculate position of each vertex and the associated normal
    int v_index = 0;
    unsigned int z, x;
    for(z = 0; z < mData.mapHeight - 1; z++) {
        // Strip triangles
        if(z % 2 == 0) {
            // Even rows go left to right
            for(x = 0; x < mData.mapWidth; x++) {
                make_vertex( &vertices[v_index], x, z, &mData );
                get_average_normal( &normals[v_index], x, z, &mData );
                v_index++;

                make_vertex( &vertices[v_index], x, z+1, &mData );
                get_average_normal( &normals[v_index], x, z+1, &mData );
                v_index++;
            }

            // Add degenerate triangles at end of row
            if(z != mData.mapHeight - 2) {
                make_vertex( &vertices[v_index], x-1, z, &mData );
                get_average_normal( &normals[v_index], x-1, z, &mData );
                v_index++;
            }else {
                make_vertex( &vertices[v_index], x-1, z, &mData );
                get_average_normal( &normals[v_index], x-1, z, &mData );
                v_index++;
                make_vertex( &vertices[v_index], x-1, z+1, &mData );
                get_average_normal( &normals[v_index], x-1, z+1, &mData );
                v_index++;
            }
        }else {
            // Odd rows go right to left
            for(x = mData.mapWidth - 1; x > 0; x--) {
                make_vertex( &vertices[v_index], x, z, &mData );
                get_average_normal( &normals[v_index], x, z, &mData );
                v_index++;
                make_vertex( &vertices[v_index], x, z+1, &mData );
                get_average_normal( &normals[v_index], x, z+1, &mData );
                v_index++;
            }

            if(z != mData.mapHeight - 2) {
                make_vertex( &vertices[v_index], x, z, &mData );
                get_average_normal( &normals[v_index], x, z, &mData );
                v_index++;
            }else {
                make_vertex( &vertices[v_index], x, z, &mData );
                get_average_normal( &normals[v_index], x, z, &mData );
                v_index++;
                make_vertex(&vertices[v_index], x, z+1, &mData);
                get_average_normal( &normals[v_index], x, z+1, &mData );
                v_index++;
            }
        }
    }

    // Create a vertex array object
    GLuint vao[1];
    glGenVertexArrays( 1, &vao[0] );
    glBindVertexArray( vao[0] );
    
    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );

    // Allocate buffer
    size_t vertexSize = world.num_vertices * sizeof(vec4);
    size_t normalSize = world.num_vertices * sizeof(vec3);
    glBufferData( GL_ARRAY_BUFFER,vertexSize + normalSize,NULL,GL_STATIC_DRAW );

        // Store the vertices as sub buffers
        glBufferSubData( GL_ARRAY_BUFFER, 0, vertexSize, vertices );
        glBufferSubData( GL_ARRAY_BUFFER, vertexSize, normalSize, normals );

    GLuint const program = init_shader( "shaders/vshader_gradient.glsl",
                                        "shaders/fshader_gradient.glsl" );
    glUseProgram( program );
    
    // Initialize the vertex position attribute from the vertex shader
    GLuint const vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, 
                           BUFFER_OFFSET(0) );
    
    // Initialize the normal position attribute from the vertex shader
    GLuint const vNormal = glGetAttribLocation( program, "vNormal" );
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE,0, 
                           BUFFER_OFFSET(vertexSize) );

    // Send max elevation in world coordinates so that shader can compute
    // the correct gradient color
    GLfloat const max_elevation = mData.yScale 
                                  * (mData.maxElevation - mData.minElevation);
    glUniform1f( glGetUniformLocation( program, "max_elevation" ),
                 max_elevation );

    // Calculate products for lighting and send them to shader
    vec4 ambient_product;
    vec4_mult( &ambient_product, 
               &world.sun_light.ambient, 
               &world.ground_material.ambient );

    vec4 diffuse_porduct;
    vec4_mult( &diffuse_porduct, 
               &world.sun_light.diffuse, 
               &world.ground_material.diffuse );

    vec4 specular_product;
    vec4_mult( &specular_product, 
               &world.sun_light.specular, 
               &world.ground_material.specular );

    glUniform4fv( glGetUniformLocation( program, "ambient_product" ), 
                  1, (GLfloat*) &ambient_product );
    glUniform4fv( glGetUniformLocation( program, "diffuse_product" ), 
                  1, (GLfloat*) &diffuse_porduct );
    glUniform4fv( glGetUniformLocation( program, "specular_product" ), 
                  1, (GLfloat*) &specular_product );

    glUniform4fv( glGetUniformLocation( program, "light_position" ), 
                  1, (GLfloat*) &world.sun_light.position );

    world.shininess_pos = glGetUniformLocation( program, "shininess" );
    glUniform1f( world.shininess_pos, world.ground_material.shininess );
            
    // Get the address of the uniform cmt used for translating
    // and rotating the object, then set the defaults
    camera.model_view_pos = glGetUniformLocation( program, "model_view" );
    world.projection_pos = glGetUniformLocation( program, "projection" );
    world.wireframe_pos = glGetUniformLocation( program, "wireframe" );
    world.light_pos = glGetUniformLocation( program, "light_position" );

    // Set a white background at the start
    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
    glutSwapBuffers();

    free( normals );
    free( vertices );
    unsigned int i;
    for(i=0; i<mData.mapHeight; i++) {
        free( mData.elevationData[i] );
    }
    free( mData.elevationData );
}
