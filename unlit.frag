#version 410

uniform sampler2D diffuse;

in vec2 fragTexCoord;

out vec4 fragColor;

void main()
{
    fragColor = texture2D(diffuse, fragTexCoord);
}
