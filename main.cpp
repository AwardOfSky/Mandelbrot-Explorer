#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <Math/MathAll.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define START_CANVAS_X -2.1f
#define START_CANVAS_Y -1.4f
#define PI 3.1415926535f
#define ITER 256
#define MANDEL 0
#define JULIA 1
#define PANNING 0
#define MOUSE_OVER 1

// time variables
unsigned int frameCount = 0;
unsigned int fps = 0;
bool panning = false;
clock_t lastTime;

// window management
GLFWwindow* window;
const char *window_title = "Mandelbrot Set";
const unsigned int imageW = 1200, imageH = 1000;	// Image width and height

double last_mouse_x = 0.0;						// Mouse vars
double last_mouse_y = 0.0;

// mandel funcs
int zoom_level, expoente, mode, julia_mouse;
float2 start, c;
float canvas_b, zoom_factor;

// shader vars
const char *fragment_shader_file = "fragment.shader";
const char *vertex_shader_file = "vertex.shader";

// Shader management
static unsigned int compile_shader(unsigned int type, const char *string);
static unsigned int create_shader(const char *vertex_shader, const char *fragment_shader);
char *file_to_string(const char *filename);

// initializations functions
int init(float *vertex_positions, unsigned int *vertex_buffer, unsigned int *shader);
void init_shaders(float *vertex_positions, unsigned int *vertex_buffer, unsigned int *shader);
int initGL_context(void);
void set_vars(int set_mode);
void print_instuctions(void);
void print_information(void);

// input management
static void mouse_function(GLFWwindow *window, double x, double y);
static void scroll_function(GLFWwindow * window, double x, double y);
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
void zoom_function(double factor, double x, double y);

void timer(void);

int main(void) {

	// Generate vertex buffer shaders, and position
	// Looks ugly, but I don't like to have this global
	unsigned int vertex_buffer, shader;
	float vertex_positions[8] = {
		-1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f,  1.0f,
		 1.0f, -1.0f
	};
	if(init(vertex_positions, &vertex_buffer, &shader)) {
		return 1;
	}
	print_instuctions();
	
	/* Set up the locations for the shader uniform variables */
	int stats_loc = glGetUniformLocation(shader, "stats");
	int exp_loc = glGetUniformLocation(shader, "exp_o");
	int mode_loc = glGetUniformLocation(shader, "mode");
	int c__loc = glGetUniformLocation(shader, "c_"); // naming is not my strong suit

	/* Game/Shader loop */
	while (!glfwWindowShouldClose(window)) {

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		/* Pass uniform variables to shader */
		glUniform3f(stats_loc, start.x, start.y + imageH*canvas_b, canvas_b);
		glUniform1i(exp_loc, expoente);
		glUniform1i(mode_loc, mode);
		glUniform2f(c__loc, c.x, c.y);

		glDrawArrays(GL_QUADS, 0, 4);

		/* timer func */
		timer();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
		++frameCount;
	}

	glDeleteProgram(shader);

	glfwTerminate();
	return 0;
}

void print_instuctions(void) {
	printf("\n\nInput\t\tAction\n\n");
	printf("[I]\t\tIncrement expoent\n");
	printf("[O]\t\tDecrement expoent\n");
	printf("[M]\t\tChange from Mandelbrot to Julia set and vs.\n");
	printf("[J]\t\tChange mouse mode from panning to mouseover (only for Julia set)\n");
	printf("[R]\t\tReset to initial configuration\n");
	printf("[P]\t\tPrint current configuration\n");
	printf("[K]\t\tIncrease zoom factor\n");
	printf("[L]\t\tDecrease zoom factor\n");
	printf("[H]\t\tPrint these instructions\n");
	printf("[Mouse Drag]\tMove canvas around / Define c in Julia set mouseover mode\n");
	printf("[Mouse Scroll]\tZoom in/out\n\n");
}

int init(float *vertex_positions, unsigned int *vertex_buffer, unsigned int *shader) {
	if (initGL_context()) {
		return 1;
	}

	init_shaders(vertex_positions, vertex_buffer, shader);
	set_vars(MANDEL);
	return 0;
}

void set_vars(int set_mode) {
	mode = set_mode;
	expoente = 2;
	start = float2(START_CANVAS_X, START_CANVAS_Y);
	c = start;
	canvas_b = abs(START_CANVAS_Y) * 2.0f / imageH;
	zoom_factor = 0.1f;
	zoom_level = 0;
	panning = false;
}

void init_shaders(float *vertex_positions, unsigned int *vertex_buffer, unsigned int *shader) {
	// set vertex buffer
	glGenBuffers(1, vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertex_positions, GL_STATIC_DRAW);

	// enable attribute positions of vertex
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void *)0);

	// set the shader
	*shader = create_shader(file_to_string(vertex_shader_file), file_to_string(fragment_shader_file));
	glUseProgram(*shader);
}

int initGL_context(void) {
	/* Initialize the library */
	if (!glfwInit()) {
		printf("There was a problem initializing glfw! Exiting...\n");
		return 1;
	}

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(imageW, imageH, window_title, NULL, NULL);
	if (!window) {
		printf("There was a problem creating the window! Exiting...\n");
		glfwTerminate();
		return 1;
	}

	/* Set window position on success */
	glfwSetWindowPos(window, 0, 28);

	/* set mouse and keyboard callback */
	glfwSetCursorPosCallback(window, mouse_function);
	glfwSetScrollCallback(window, scroll_function);
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetKeyCallback(window, keyboard);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		printf("There was a problem initializing glew! Exiting...\n");
		return 1;
	}
	printf("OpenGL version: %s graphics driver\n", glGetString(GL_VERSION)); // print OpenGL version and graphics driver
	return 0;
}

void timer(void) {
	++fps;
	if (clock() - lastTime > CLOCKS_PER_SEC) {
		lastTime = clock();
		char title[64];
		sprintf(title, "%s - FPS: %d", window_title, fps);
		glfwSetWindowTitle(window, title);
		fps = 0;
	}
}

static unsigned int compile_shader(unsigned int type, const char *string) {
	unsigned int id = glCreateShader(type);
	const char *src = string;
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char *message = (char *)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		printf("Failed to compile %s shader!\n",
			(type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
		printf("%s\n", message);
		glDeleteShader(id);
		id = 0;
	}

	return id;	// TODO: Error handling
}

static unsigned int create_shader(const char *vertex_shader, const char *fragment_shader) {
	unsigned int program = glCreateProgram();
	unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
	unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

char *file_to_string(const char *filename) {
	char *buffer = NULL;
	unsigned int string_size;
	FILE *f_handler = fopen(filename, "rb"); // needs to be read in binary
	if(f_handler) {							 // because windows is retarded
		fseek(f_handler, 0, SEEK_END); // point to last byte of file
		string_size = ftell(f_handler); // size of current pos from start in bytes (file size)
		rewind(f_handler); // go back to the start of the file
		buffer = (char *)malloc((string_size + 1) * sizeof(char)); // +1 cuz null terminator
		fread(buffer, sizeof(char), string_size, f_handler); // read file in 1 go
		buffer[string_size] = '\0'; // null terminate it
		fclose(f_handler); // close file
	} else {
		printf("Error in 'file_to_string' function: Could not open file: '%s'\n", filename);
	}
	return buffer;
}

static void mouse_function(GLFWwindow *window, double x, double y) {
	if (mode == MANDEL || ((mode == JULIA) && (julia_mouse == PANNING))) {
		if (panning) {
			start.x -= (x - last_mouse_x) * canvas_b;
			start.y -= (y - last_mouse_y) * canvas_b;
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			panning = true;
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
			panning = false;
		}
	} else {
		c = float2(start.x + x*canvas_b, start.y + y*canvas_b);
	}

	last_mouse_x = x;
	last_mouse_y = y;
}

void zoom_function(double factor, double x, double y) {
	float2 mouse_pos = float2(x, y);
	float2 mouse_point = start + (mouse_pos * canvas_b);
	canvas_b *= factor;
	start = mouse_point - (mouse_point - start)*factor;
	if (factor > 1.0) {
		--zoom_level;
	} else if (factor < 1.0) {
		++zoom_level;
	}
}

static void scroll_function(GLFWwindow * window, double x, double y) {
	double x_pos, y_pos;
	glfwGetCursorPos(window, &x_pos, &y_pos);
	if (y > 0) { // zoom in
		zoom_function(1 - zoom_factor, x_pos, y_pos);
	} else if (y < 0) { // zoom out
		zoom_function(1 + zoom_factor, x_pos, y_pos);
	}
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_I:
			if (expoente < 20) {
				++expoente;
			}
			break;
		case GLFW_KEY_O:
			if (expoente > 1) {
				--expoente;
			}
			break;
		case GLFW_KEY_M:
			mode = 1 - mode;
			set_vars(mode);
			if (mode == JULIA) {
				c = float2(0.285f, 0.01f);
				julia_mouse = PANNING;
			}
			break;
		case GLFW_KEY_J:
			julia_mouse = 1 - julia_mouse;
			break;
		case GLFW_KEY_R:
			set_vars(MANDEL);
			break;
		case GLFW_KEY_P:
			print_information();
			break;
		case GLFW_KEY_K:
			if (zoom_factor < 0.5f) {
				zoom_factor += 0.05f;
			}
			break;
		case GLFW_KEY_L:
			if (zoom_factor > 0.0f) {
				zoom_factor -= 0.05f;
			}
			break;
		case GLFW_KEY_H:
			print_instuctions();
			break;
		default:
			break;
		}
	}
}

void print_information(void) {
	printf("\n\nCurrently rendering:\t\t%s set\n", (mode == MANDEL) ? "Mandelbrot" : "Julia");
	printf("Expoent in use (z^n + c):\t%d\n", expoente);
	if (mode == JULIA) {
		printf("Mouse mode:\t\t\t%s\n", (julia_mouse == PANNING) ? "Panning" : "Mouseover");
		printf("C value:\t\t\t(%.6f, %.6f)\n", c.x, c.y);
	}
	printf("Start of canvas:\t\t(%.6f, %.6fi)\n", start.x, start.y);
	printf("Distance beetween pixels:\t%.12f\n", canvas_b);
	printf("Zoom Level:\t\t\t%d\n", zoom_level);
	printf("Zoom Factor:\t\t\t%.6f\n", (1.0f + zoom_factor));
	printf("Max convergence iterations:\t%d\n\n", ITER);
	printf("Resolution:\t\t\t%dx%d px\n", imageW, imageH);
}