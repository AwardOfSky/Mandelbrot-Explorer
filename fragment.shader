#version 420 compatibility

uniform vec3 stats;		// start pos(x, y), canvas b
uniform vec2 c_;		// only used in julia set
uniform int exp_o;		// expoent used in z^n + c
uniform int mode;		// Mandelbrot or Julia set

#define MANDEL_ITE 256	// Max iterations
const vec3 color_v = vec3(0.625f, 0.625f, 0.1171875f) * MANDEL_ITE; // rgb porpotions

void main(void) {
	
	const int exp = exp_o;
	// in GLSL, grid start at the bottom left of the screen (mathematical grid)
	vec2 z = vec2(stats.x + gl_FragCoord.x * stats.z, stats.y - gl_FragCoord.y * stats.z);
	const vec2 c = (mode == 1) ? c_ : z; // support for julia sets (mode 1 = Julia)
	float i;

	if (exp == 2) { // Small otimization for expoent 2 
		for (i = 0.0f; i < MANDEL_ITE; i += 1.0f) {
			z = vec2(z.x * z.x - z.y * z.y, 2.0f * z.y * z.x) + c;
			if (dot(z, z) > 4.0f) {
				break;
			}
		}
	} else { // generic code
		for (i = 0.0f; i < MANDEL_ITE; i += 1.0f) {
			vec2 zt = z;
			for (int j = 1; j < exp; ++j) {
				z = vec2(z.x * zt.x - z.y * zt.y, z.y * zt.x + z.x * zt.y);
			}
			z += c;
			if (dot(z, z) > 4.0f) {
				break;
			}
		}
	}

	// Simple coloring based on the Khan Acedemy program (2013)
	vec3 col = vec3(0.0f);
	if (i < MANDEL_ITE) {
		vec3 a = vec3(i) / color_v;
		float high = 45.0f / pow(((i - 25.0f) * 0.1f), 2.0f);
		col = vec3(
			a.x + high * 0.00392156862f,
			a.y + high * 0.00008714596f,
			a.z + 0.17647058823f - high*0.00392156862f);
	}
	gl_FragColor = vec4(col, 1.0f);
}