#include "SDL2/SDL.h"
#include "cglm/ivec2.h"
#include "glad/glad.h"
#include "cglm/cam.h"
#include "cglm/types.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const GLchar* shader_code_vertex =
"#version 460 core\n"
"layout (location = 0) in vec2 aPosition;\n"
"layout (location = 1) in vec4 aColor;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"layout (location = 3) in float aTexID;\n"
"out vec4 vFragColor;\n"
"out vec2 vTexCoord;\n"
"out float vTexID;\n"
"uniform mat4 uMatrixProjection;\n"
"void main() {\n"
"	gl_Position = uMatrixProjection * vec4(aPosition, 0.0f, 1.0f);\n"
"	vFragColor = aColor;\n"
"	vTexCoord = aTexCoord;\n"
"	vTexID = aTexID;\n"
"}\n";

const GLchar* shader_code_fragment =
"#version 460 core\n"
"in vec4 vFragColor;"
"in vec2 vTexCoord;"
"in float vTexID;"
"out vec4 fFragColor;\n"
"void main() {\n"
"	fFragColor = vec4(vFragColor);\n"
"}\n";

typedef struct t_renderer {
	GLuint shader_program;

	GLuint vertex_array;
	GLuint vertex_buffer;
	GLuint element_buffer;

	GLfloat* vertex_data;	GLuint vertex_data_max;	GLuint vertex_data_count;
	GLuint*	index_data;		GLuint index_data_max;	GLuint index_data_count;	GLuint indicy_largest;
} t_renderer;

typedef struct {
	ivec2 position;
	ivec2 size;
} t_rect;

t_renderer pongRendererLoad(void);
int pongRendererPrepare(t_renderer* renderer);
int pongRendererDispatch(t_renderer* renderer);
int pongRendererPushVertexData(t_renderer* renderer, GLfloat* vertex_data, GLuint vertex_data_size);
int pongRendererPushIndexData(t_renderer* renderer, GLuint* index_data, GLuint index_data_size);
int pongRendererUnload(t_renderer* renderer);

int pongRectMove(t_rect* rect, ivec2 position);
int pongRectResize(t_rect* rect, ivec2 size);
int pongRectRender(t_renderer* renderer, t_rect rect, vec4 color);

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

	t_renderer renderer = pongRendererLoad();

	t_rect p1 = {
		{ 64, 64 },
		{ 16, 128 }
	};

	t_rect p2 = {
		{ 800 - 80, 64 },
		{ 16, 128 }
	};

	t_rect ball = {
		{ 800 / 2 - 16 / 2, 600 / 2 - 16 / 2 },
		{ 16, 16 }
	};


	mat4 matrix_projection;
	glm_ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f, matrix_projection);
	glViewport(0, 0, 800, 600);

	int exit = 0;
	while(!exit) {
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		pongRendererPrepare(&renderer);

		pongRectRender(&renderer, p1, (vec4) { 0.8f, 0.8f, 0.8f, 1.0f });
		pongRectRender(&renderer, p2, (vec4) { 0.8f, 0.8f, 0.8f, 1.0f });
		pongRectRender(&renderer, ball, (vec4) { 0.8f, 0.8f, 0.8f, 1.0f });

		glUniformMatrix4fv(
			glGetUniformLocation(renderer.shader_program, "uMatrixProjection"),
			1,
			GL_FALSE,
		 	&matrix_projection[0][0]
		);

		pongRendererDispatch(&renderer);

		SDL_GL_SwapWindow(window);

		SDL_Event event = { 0 };
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT: {
					exit = 1;
				} break;
			}
		}
	}

	pongRendererUnload(&renderer);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}

t_renderer pongRendererLoad(void) {
	t_renderer result = { 0 };

	result.vertex_data_count = 	0;
	result.vertex_data_max = 	65536;
	result.index_data_count = 	0;
	result.index_data_max = 	65536;
	result.indicy_largest = 0;

	result.vertex_data = (GLfloat*) calloc(65536, sizeof(GLfloat));
	if(!result.vertex_data) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return (t_renderer) { 0 };
	}
	result.index_data = (GLuint*) calloc(65536, sizeof(GLuint));
	if(!result.index_data) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return (t_renderer) { 0 };
	}

	GLuint shader_vertex, shader_fragment;
	shader_vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader_vertex, 1, &shader_code_vertex, NULL);
	glCompileShader(shader_vertex);

	shader_fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader_fragment, 1, &shader_code_fragment, NULL);
	glCompileShader(shader_fragment);

	result.shader_program = glCreateProgram();
	glAttachShader(result.shader_program, shader_vertex);
	glAttachShader(result.shader_program, shader_fragment);
	glLinkProgram(result.shader_program);

	glDeleteShader(shader_vertex);
	glDeleteShader(shader_fragment);

	glUseProgram(result.shader_program);

	glGenVertexArrays(1, &result.vertex_array);
	glBindVertexArray(result.vertex_array);

	glGenBuffers(1, &result.vertex_buffer);
	glGenBuffers(1, &result.element_buffer);

	return result;
}

int pongRendererPrepare(t_renderer* renderer) {
	glBindVertexArray(renderer->vertex_array);
	glUseProgram(renderer->shader_program);
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->element_buffer);

	return 1;
}

int pongRendererDispatch(t_renderer* renderer) {
	glBufferData(GL_ARRAY_BUFFER, renderer->vertex_data_count * sizeof(GLfloat), renderer->vertex_data, GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderer->index_data_count * sizeof(GLuint), renderer->index_data, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (0 * sizeof(GLfloat)));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (2 * sizeof(GLfloat)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (6 * sizeof(GLfloat)));
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (void*) (8 * sizeof(GLfloat)));

	glDrawElements(GL_TRIANGLES, renderer->index_data_count, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	glBindVertexArray(0);

	renderer->vertex_data_count = 0;
	renderer->index_data_count = 0;
	renderer->indicy_largest = 0;

	return 1;
}

int pongRendererPushVertexData(t_renderer* renderer, GLfloat* vertex_data, GLuint vertex_data_size) {
    for(int i = 0; i < vertex_data_size; i++) {
        if(renderer->vertex_data_count + 1 >= renderer->vertex_data_max) {
            return 0;
        }

        renderer->vertex_data[renderer->vertex_data_count++] = vertex_data[i];
    }

    return 1;
}

int pongRendererPushIndexData(t_renderer* renderer, GLuint* index_data, GLuint index_data_size) {
    const GLint base = renderer->indicy_largest + (renderer->indicy_largest <= 0 ? 0 : 1);

    for(int i = 0; i < index_data_size; i++) {
        if(renderer->index_data_count + 1 >= renderer->index_data_max) {
            return 0;
        }

        renderer->index_data[renderer->index_data_count++] = index_data[i] + base;

        if(renderer->indicy_largest < index_data[i] + base) {
            renderer->indicy_largest = index_data[i] + base;
        }
    }

    return 1;
}

int pongRendererUnload(t_renderer* renderer) {
	free(renderer->vertex_data);
	free(renderer->index_data);

	glDeleteProgram(renderer->shader_program);
	glDeleteBuffers(1, &renderer->vertex_buffer);
	glDeleteBuffers(1, &renderer->element_buffer);
	glDeleteVertexArrays(1, &renderer->vertex_array);

	return 1;
}


int pongRectMove(t_rect* rect, ivec2 position) {
	if(!rect)
		return 0;

	glm_ivec2_copy(position, rect->position);

	return 1;
}

int pongRectResize(t_rect* rect, ivec2 size) {
	if(!rect)
		return 0;

	glm_ivec2_copy(size, rect->size);

	return 1;
}

int pongRectRender(t_renderer* renderer, t_rect rect, vec4 color) {
	if(!renderer)
		return 0;

	GLfloat quad_vert[] = {
		rect.position[0], rect.position[1], 									color[0], color[1], color[2], color[3],		0.0f, 0.0f,		0.0f,
		rect.position[0] + rect.size[0], rect.position[1], 						color[0], color[1], color[2], color[3],		1.0f, 0.0f,		0.0f,
		rect.position[0], rect.position[1] + rect.size[1], 						color[0], color[1], color[2], color[3], 	0.0f, 1.0f,		0.0f,
		rect.position[0] + rect.size[0], rect.position[1] + rect.size[1],	 	color[0], color[1], color[2], color[3],		1.0f, 1.0f,		0.0f,
	};

	GLuint quad_index[] = {
		0, 1, 2,
		1, 2, 3
	};

	if(!pongRendererPushVertexData(renderer, quad_vert, sizeof(quad_vert) / sizeof(GLfloat)))
		return 0;

	if(!pongRendererPushIndexData(renderer, quad_index, sizeof(quad_index) / sizeof(GLuint)))
		return 0;

	return 1;
}
