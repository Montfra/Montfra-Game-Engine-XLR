#version 330 core

layout(location = 0) in vec3 aPos;

// Matrice de projection mise à jour côté CPU en fonction de la taille de la fenêtre
uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * vec4(aPos, 1.0);
}

