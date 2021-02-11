uniform sampler2D Texture;
uniform float NbScanlines;
uniform float Scanline;
uniform float Roundness;
uniform float BorderSharpness;
uniform float Vignetting;
uniform float Brightness;
uniform float Contrast;
uniform float Sharpness;
uniform float GridDep;
uniform float GhostAmount;
uniform float ChromaAmount;
in vec2 Frag_UV;
out vec4 Out_Color;
//#define GridW 560.
//#define GridH 320.
#define GridW 660.
#define GridH 520.
#define GridPow 1.1
#define Gain 1.
#define GHOST_DIST 0.004
#define CHROMA_DIST 0.002

void main()
{
	vec2 uv;
    uv.x = (Frag_UV.x-0.5) / cos(Roundness*abs(Frag_UV.y - .5)) + 0.5;
    uv.y = (Frag_UV.y-0.5) / cos(Roundness*abs(Frag_UV.x - .5)) + 0.5;

    float BorderCoeff = smoothstep(0.0, BorderSharpness, 0.5 - abs(uv.x - 0.5));
    BorderCoeff *= smoothstep(0.0, BorderSharpness, 0.5 - abs(uv.y - 0.5));
    
    float VignetteCoeff = pow(1. - length(uv-vec2(0.5)), Vignetting);

	float H = NbScanlines;
	float W = H * 4. / 3.;
	float s = mix(1, abs(sin(3.14159 * uv.y * H)), Scanline);

    uv.x *= 416. / 512.;
    uv.y = (1. - uv.y) * (NbScanlines / 512.);
    
	vec2 SubUVDelta = (2.-Sharpness)*vec2(.2/W, .2/H);
	Out_Color = texture(Texture, uv);
	Out_Color += texture(Texture, uv + vec2(SubUVDelta.x, 0));
	Out_Color += texture(Texture, uv - vec2(SubUVDelta.x, 0));
	Out_Color += texture(Texture, uv + vec2(0, SubUVDelta.y));
	Out_Color += texture(Texture, uv - vec2(0, SubUVDelta.y));
	Out_Color *= .2;

    Out_Color += ChromaAmount * vec4(1., 0., 0.5, 0.) * texture(Texture, uv + vec2(CHROMA_DIST, 0.));
    Out_Color += ChromaAmount * vec4(0., 1., 0.5, 0.) * texture(Texture, uv - vec2(CHROMA_DIST, 0.));
    Out_Color /= 1. + 2.*ChromaAmount;

	Out_Color += GhostAmount * (0.3-texture(Texture, uv - vec2(GHOST_DIST, 0.)));

	Out_Color *= s;
	float L = .333 * (Out_Color.x + Out_Color.y + Out_Color.z);
	Out_Color *= 1. - Contrast * cos(L * 3.14159);
	Out_Color *= Brightness;
	Out_Color *= BorderCoeff;
    Out_Color *= VignetteCoeff;
    
    vec4 Mask = vec4(0.);
    float P = (uv.x*GridW + floor(uv.y * GridH)) * 3.14159;
    Mask.x = mix(pow(max(0., sin(P)), GridPow), 1.15*Gain, GridDep);
    Mask.y = mix(.97*pow(max(0., sin(P+2.2)), GridPow), 1.25*Gain, GridDep);
    Mask.z = mix(1.2*pow(max(0., sin(P+4.4)), GridPow), 1.25*Gain, GridDep);
    Out_Color *= Mask;
}

