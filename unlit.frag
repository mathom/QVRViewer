#version 410

uniform sampler2D diffuse;

in vec2 fragTexCoord;

out vec4 fragColor;

void main()
{
    fragColor = texture2D(diffuse, fragTexCoord);
    //fragColor = vec4(1, 0, 0, 1);
}
