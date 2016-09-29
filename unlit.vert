#version 410

uniform mat4 transform;
in vec3 vertex;
in vec2 texCoord;
out vec2 fragTexCoord;

void main()
{
    fragTexCoord = texCoord;
    //gl_Position = vec4(vertex, 1.0f);
    gl_Position = transform * vec4(vertex, 1.0f);
}
