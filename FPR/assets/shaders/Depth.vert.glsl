#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;

///////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////
struct Camera
{
    mat4 View;
    mat4 Projection;
	vec2 DepthUnpackConsts;
};
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Inputs
///////////////////////////////////////////////////////////////////////
// Set 0
layout(std140, set = 0, binding = 0) uniform ModelSettings
{
    mat4 Model;
} u_Model;

// Set 1
layout(std140, set = 1, binding = 0) uniform CameraSettings
{
    Camera Camera;
} u_Camera;
///////////////////////////////////////////////////////////////////////

void main() 
{
    gl_Position = u_Camera.Camera.Projection * u_Camera.Camera.View * u_Model.Model * vec4(a_Position, 1.0);
}