#version 330 core

// Depth-only pass: the empty main lets the fixed-function pipeline write
// gl_FragCoord.z into the depth texture. gl_FragDepth is deliberately not
// overwritten so early-z stays effective.
void main()
{
}
