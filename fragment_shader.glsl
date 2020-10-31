#version 130
uniform vec2 windowSize;
uniform float values[256];
out vec3 fragColor;

float cubicInterpolate(float t, float y0, float y1, float y2, float y3)
{
    return (
        (-y0 + 3 * y1 - 3 * y2 + y3) * t * t * t
        + (2 * y0 - 5 * y1 + 4 * y2 - y3) * t * t
        + (-y0 + y2) * t
        + 2 * y1
    ) * 0.5;
}

void main()
{
    vec2 pos = gl_FragCoord.xy / windowSize;

    float x = pos.x * (values.length() - 1);
    int i1 = int(x);
    float t = x - i1;
    int i2 = i1 + 1;
    int i0 = max(i1 - 1, 0);
    int i3 = min(i1 + 2, values.length() - 1);
    float y = cubicInterpolate(t, values[i0], values[i1], values[i2], values[i3]);

    float lineTop = y * windowSize.y + 1.5;
    float lineBottom = y * windowSize.y - 1.5;
    if (gl_FragCoord.y < lineBottom) {
        fragColor = vec3(0.05 + 0.1 * pos.y);
    } else if (gl_FragCoord.y < lineTop) {
        fragColor = vec3(1.0);
    } else {
        fragColor = vec3(0.05);
    }
}
