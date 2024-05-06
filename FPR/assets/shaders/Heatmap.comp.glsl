#version 460 core

#define TILE_SIZE 16
#define MAX_POINTLIGHTS 1024
#define MAX_POINTLIGHTS_PER_TILE 64

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

///////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////
// PointLight
struct PointLight
{
    vec3 Position;
    float Radius;

    vec3 Colour;
    float Intensity;
};

struct PointLightVisibilty
{
    uint Count;
    uint Indices[MAX_POINTLIGHTS_PER_TILE];
};
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Inputs
///////////////////////////////////////////////////////////////////////
// Set 0
layout(rgba8, set = 0, binding = 0) uniform writeonly image2D u_Image;

layout(std140, set = 0, binding = 1) buffer LightVisibilityBuffer 
{
	uint AmountOfTiles;
    PointLightVisibilty VisiblePointLights[/*Amount of Tiles*/];
} u_Visibility;

// Set 1
layout(std140, set = 1, binding = 0) uniform SceneUniform 
{
    uvec2 ScreenSize;
} u_Scene;
///////////////////////////////////////////////////////////////////////

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    
    // Calculate tile index
    ivec2 location = ivec2(gl_GlobalInvocationID.xy);
    ivec2 itemID = ivec2(gl_LocalInvocationID.xy);
    ivec2 tileID = ivec2(gl_WorkGroupID.xy);
    ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
    uint tileIndex = tileID.y * tileNumber.x + tileID.x;

    // Access visibility data for the current tile
    PointLightVisibilty tileVisibility = u_Visibility.VisiblePointLights[tileIndex];

    // Calculate heatmap value based on the number of visible point lights in this tile
    float heatmapValue = float(tileVisibility.Count)/* / float(MAX_POINTLIGHTS_PER_TILE)*/;

    // Write heatmap value to image
    imageStore(u_Image, pixelCoords, vec4(heatmapValue));
}