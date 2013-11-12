varying vec3 fragPos;

void main()
{
    gl_Position = ftransform();
    fragPos.xyz=gl_Vertex.xyz;
}
