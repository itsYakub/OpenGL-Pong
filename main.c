#include "SDL2/SDL.h"
#include "glad/glad.h"

const GLchar* shader_code_vertex =
"#version 460 core\n"
"layout (location = 0) in vec2 aPosition;\n"
"void main() {\n"
"	gl_Position = vec4(aPosition, 0.0f, 1.0f);\n"
"}\n";

const GLchar* shader_code_fragment =
"#version 460 core\n"
"out vec4 fFragColor;\n"
"void main() {\n"
"	fFragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
"}\n";

int main(int argc, const char* argv[]) {
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_Window* window = SDL_CreateWindow(
		"Hello, OpenGL!",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800,
		600,
		SDL_WINDOW_OPENGL
	);

	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);
	SDL_GL_SetSwapInterval(1);

	gladLoadGL();

	GLuint shader_program, shader_vertex, shader_fragment;
	GLuint vertex_array, vertex_buffer, element_buffer;

	GLfloat quad_vert[] = {
		-0.5f, -0.5f,
		 0.5f, -0.5f,
		-0.5f,  0.5f,
		 0.5f,  0.5f
	};

	GLuint quad_index[] = {
		0, 1, 2,
		1, 2, 3
	};

	shader_vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_vertex, 1, &shader_code_vertex, NULL);
	glCompileShader(shader_vertex);

	shader_fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader_fragment, 1, &shader_code_fragment, NULL);
	glCompileShader(shader_fragment);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, shader_vertex);
	glAttachShader(shader_program, shader_fragment);
	glLinkProgram(shader_program);

	glDeleteShader(shader_vertex);
	glDeleteShader(shader_fragment);

	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vert), quad_vert, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &element_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_index), quad_index, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*) 0);

	int exit = 0;
	while(!exit) {
		SDL_Event event = { 0 };
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT: {
					exit = 1;
				} break;
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		glUseProgram(shader_program);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(shader_program);
	glDeleteBuffers(1, &vertex_buffer);
	glDeleteBuffers(1, &element_buffer);
	glDeleteVertexArrays(1, &vertex_array);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
