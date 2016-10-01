#version 410

uniform bool leftEye;
uniform bool overUnder;
uniform mat4 transform;
in vec3 vertex;
in vec2 texCoord;
out vec2 fragTexCoord;

void main()
{
    fragTexCoord = texCoord;

    if (overUnder) {
        if (leftEye) {
            fragTexCoord.t = fragTexCoord.t * 0.5 + 0.5;
        }
        else {
            fragTexCoord.t = fragTexCoord.t * 0.5;
        }
    }

    gl_Position = transform * vec4(vertex, 1.0f);
}
