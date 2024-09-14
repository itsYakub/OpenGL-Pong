/* SECTION: Includes */

#include "SDL2/SDL.h"
#include "SDL_events.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "cglm/ivec2.h"
#include "glad/glad.h"
#include "cglm/cam.h"
#include "cglm/types.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SECTION: Shaders */

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

/* SECTION: Typedefs */
typedef struct {
	SDL_Window* window;
	SDL_GLContext context;

	ivec2 size;
	int exit;
} t_window;

typedef struct {
	GLuint shader_program;

	GLuint vertex_array;
	GLuint vertex_buffer;
	GLuint element_buffer;

	GLfloat* vertex_data;	GLuint vertex_data_max;	GLuint vertex_data_count;
	GLuint*	index_data;		GLuint index_data_max;	GLuint index_data_count;	GLuint indicy_largest;

	mat4 matrix_projection;
} t_renderer;

typedef struct {
	ivec2 position;
	ivec2 size;
} t_rect;

typedef struct {
	unsigned int key_state[SDL_NUM_SCANCODES];
	unsigned int key_state_previous[SDL_NUM_SCANCODES];
} t_input;

typedef enum {
	POSITION_LEFT = 0,
	POSITION_RIGHT
} t_player_pos;

typedef struct {
	t_rect rect;
	int score;
	SDL_Scancode key_up;
	SDL_Scancode key_down;
} t_player;

typedef struct {
	t_rect rect;
	ivec2 direction;
} t_ball;

typedef enum {
	STATE_BEGIN = 0,
	STATE_GAMEPLAY,
	STATE_OVER,
	STATE_COUNT
} t_state_machine;

/* SECTION: Functions - t_window */

t_window ft_window(int w, int h, const char* title);
int ft_window_poll_events(t_window* window, t_input* input);
int ft_window_dispatch(t_window* window);
int ft_window_close(t_window* window);

/* SECTION: Functions - t_renderer */

t_renderer ft_renderer(void);
int ft_renderer_begin(t_renderer* renderer, t_window* window);
int ft_renderer_dispatch(t_renderer* renderer);
int ft_renderer_push_vertices(t_renderer* renderer, GLfloat* vertex_data, GLuint vertex_data_size);
int ft_renderer_push_indices(t_renderer* renderer, GLuint* index_data, GLuint index_data_size);
int ft_renderer_unload(t_renderer* renderer);

/* SECTION: Functions - t_rect */

int ft_rect_move(t_rect* rect, ivec2 position);
int ft_move_size(t_rect* rect, ivec2 size);

int ft_collision_left_right(t_rect a, t_rect b);
int ft_collision_top_bottom(t_rect a, t_rect b);
int ft_collision(t_rect a, t_rect b);
int ft_collision_boundaries(t_rect a, ivec2 bound);

int ft_draw_rect(t_renderer* renderer, t_rect rect, vec4 color);

/* SECTION: Functions - t_input */

int ft_input_keydown(t_input* input, SDL_Scancode scancode);
int ft_input_keyup(t_input* input, SDL_Scancode scancode);
int ft_input_keypress(t_input* input, SDL_Scancode scancode);
int ft_input_keyrelease(t_input* input, SDL_Scancode scancode);

/* SECTION: Functions - t_player */

t_player ft_player(t_player_pos ppos, ivec2 bounds);
int ft_player_update(t_player* player, t_input* input, ivec2 bounds);
int ft_plyaer_incremenet_score(t_player* player);

/* SECTION: Functions - t_ball */

t_ball ft_ball(ivec2 bounds);
int ft_ball_restart(t_ball* ball, ivec2 bounds);
int ft_ball_randomize_direction(t_ball* ball);
int ft_ball_update(t_ball* ball, t_player* p1, t_player* p2, ivec2 bounds);
int ft_ball_out_of_bounds(t_ball* ball, ivec2 bounds);

int main(int argc, const char* argv[]) {
	t_window window = ft_window(1024, 768, "OpenGL 330 | SDL 2.30.7 | Pong 1.0f");
	t_renderer renderer = ft_renderer();
	t_input input = { 0 };

	t_player p1 = ft_player(POSITION_LEFT, window.size);
	t_player p2 = ft_player(POSITION_RIGHT, window.size);
	t_ball ball = ft_ball(window.size);

	t_state_machine state_machine = STATE_BEGIN;

	while(!window.exit) {
		switch(state_machine) {
			case STATE_BEGIN: {
				if(ft_input_keyrelease(&input, SDL_SCANCODE_SPACE)) {
					ft_ball_randomize_direction(&ball);

					state_machine = STATE_GAMEPLAY;
				}
			} break;

			case STATE_GAMEPLAY: {
				ft_ball_update(&ball, &p1, &p2, window.size);

				if(ft_ball_out_of_bounds(&ball, window.size)) {
					state_machine = STATE_OVER;
				}
			} break;

			case STATE_OVER: {
				if(ft_input_keyrelease(&input, SDL_SCANCODE_R)) {
					ball = ft_ball(window.size);
					p1 = ft_player(POSITION_LEFT, window.size);
					p2 = ft_player(POSITION_RIGHT, window.size);

					state_machine = STATE_BEGIN;
				}
			} break;

			case STATE_COUNT: { } break;
			default: { } break;
		}

		ft_player_update(&p1, &input, window.size);
		ft_player_update(&p2, &input, window.size);

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		ft_renderer_begin(&renderer, &window);

		ft_draw_rect(&renderer, p1.rect, (vec4) { 0.8f, 0.8f, 0.8f, 1.0f });
		ft_draw_rect(&renderer, p2.rect, (vec4) { 0.8f, 0.8f, 0.8f, 1.0f });
		ft_draw_rect(&renderer, ball.rect, (vec4) { 0.8f, 0.8f, 0.8f, 1.0f });

		ft_renderer_dispatch(&renderer);
		ft_window_dispatch(&window);
		ft_window_poll_events(&window, &input);
	}

	ft_renderer_unload(&renderer);
	ft_window_close(&window);

	return 0;
}

/* SECTION: Functions - t_window */

t_window ft_window(int w, int h, const char* title) {
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		return (t_window) { 0 };
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	t_window res = { 0 };

	res.window = SDL_CreateWindow(
		"Hello, OpenGL!",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1024,
		768,
		SDL_WINDOW_OPENGL | SDL_WINDOW_MOUSE_CAPTURE
	);

	res.context = SDL_GL_CreateContext(res.window);
	SDL_GL_MakeCurrent(res.window, res.context);
	SDL_GL_SetSwapInterval(1);

	gladLoadGL();

	SDL_GetWindowSize(res.window, &res.size[0], &res.size[1]);
	res.exit = 0;

	return res;
}

int ft_window_poll_events(t_window* window, t_input* input) {
	if(!window || !input) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	for(int i = 0; i < SDL_NUM_SCANCODES; i++)
		input->key_state_previous[i] = input->key_state[i];

	SDL_Event event = { 0 };
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: {
				window->exit = 1;
			} break;

			case SDL_KEYDOWN: {
				input->key_state[event.key.keysym.scancode] = 1;
			} break;

			case SDL_KEYUP: {
				input->key_state[event.key.keysym.scancode] = 0;
			} break;

			case SDL_WINDOWEVENT: {
				switch(event.window.type) {
					case SDL_WINDOWEVENT_RESIZED: {
						SDL_GetWindowSize(window->window, &window->size[0], &window->size[1]);
					} break;
				}
			}
		}
	}

	return 1;
}

int ft_window_dispatch(t_window* window) {
	if(!window) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	SDL_GL_SwapWindow(window->window);

	return 1;
}

int ft_window_close(t_window* window) {
	if(!window) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	SDL_GL_DeleteContext(window->context);
	SDL_DestroyWindow(window->window);

	SDL_Quit();

	return 1;
}

/* SECTION: Functions - t_renderer */

t_renderer ft_renderer(void) {
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

int ft_renderer_begin(t_renderer* renderer, t_window* window) {
	if(!renderer) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	glBindVertexArray(renderer->vertex_array);
	glUseProgram(renderer->shader_program);
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->element_buffer);

	glm_ortho(0.0f, window->size[0], 0.0f, window->size[1], -1.0f, 1.0f, renderer->matrix_projection);
	glViewport(0, 0, window->size[0], window->size[1]);

	glUniformMatrix4fv(
		glGetUniformLocation(renderer->shader_program, "uMatrixProjection"),
		1,
		GL_FALSE,
	 	&renderer->matrix_projection[0][0]
	);

	return 1;
}

int ft_renderer_dispatch(t_renderer* renderer) {
	if(!renderer) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

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

int ft_renderer_push_vertices(t_renderer* renderer, GLfloat* vertex_data, GLuint vertex_data_size) {
	if(!renderer) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

    for(int i = 0; i < vertex_data_size; i++) {
        if(renderer->vertex_data_count + 1 >= renderer->vertex_data_max)
            return 0;

        renderer->vertex_data[renderer->vertex_data_count++] = vertex_data[i];
    }

    return 1;
}

int ft_renderer_push_indices(t_renderer* renderer, GLuint* index_data, GLuint index_data_size) {
	if(!renderer) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

    const GLint base = renderer->indicy_largest + (renderer->indicy_largest <= 0 ? 0 : 1);

    for(int i = 0; i < index_data_size; i++) {
        if(renderer->index_data_count + 1 >= renderer->index_data_max)
            return 0;

        renderer->index_data[renderer->index_data_count++] = index_data[i] + base;

        if(renderer->indicy_largest < index_data[i] + base)
            renderer->indicy_largest = index_data[i] + base;
    }

    return 1;
}

int ft_renderer_unload(t_renderer* renderer) {
	if(!renderer) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	free(renderer->vertex_data);
	free(renderer->index_data);

	glDeleteProgram(renderer->shader_program);
	glDeleteBuffers(1, &renderer->vertex_buffer);
	glDeleteBuffers(1, &renderer->element_buffer);
	glDeleteVertexArrays(1, &renderer->vertex_array);

	return 1;
}

/* SECTION: Functions - t_rect */

int ft_rect_move(t_rect* rect, ivec2 position) {
	if(!rect) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	glm_ivec2_copy(position, rect->position);

	return 1;
}

int ft_move_size(t_rect* rect, ivec2 size) {
	if(!rect) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	glm_ivec2_copy(size, rect->size);

	return 1;
}

int ft_collision_left_right(t_rect a, t_rect b) {
	return 	(a.position[0] < b.position[0] + b.size[0]) &&
			(a.position[0] + a.size[0] > b.position[0]);
}

int ft_collision_top_bottom(t_rect a, t_rect b) {
	return	(a.position[1] < b.position[1] + b.size[1]) &&
			(a.position[1] + a.size[1] > b.position[1]);
}

int ft_collision(t_rect a, t_rect b) {
	return 	ft_collision_left_right(a, b) &&
			ft_collision_top_bottom(a, b);
}

int ft_collision_boundaries(t_rect a, ivec2 bound) {
	return 	(a.position[0] <= 0 || a.position[0] + a.size[1] >= bound[0]) ||
			(a.position[1] <= 0 || a.position[1] + a.size[1] >= bound[1]);
}

int ft_draw_rect(t_renderer* renderer, t_rect rect, vec4 color) {
	if(!renderer) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

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

	if(!ft_renderer_push_vertices(renderer, quad_vert, sizeof(quad_vert) / sizeof(GLfloat)))
		return 0;

	if(!ft_renderer_push_indices(renderer, quad_index, sizeof(quad_index) / sizeof(GLuint)))
		return 0;

	return 1;
}

/* SECTION: Functions - t_input */

int ft_input_keydown(t_input* input, SDL_Scancode scancode) {
	if(!input) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	return input->key_state[scancode] == 1;
}

int ft_input_keyup(t_input* input, SDL_Scancode scancode) {
	if(!input) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	return input->key_state[scancode] == 0;
}

int ft_input_keypress(t_input* input, SDL_Scancode scancode) {
	if(!input) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	return input->key_state[scancode] == 1 && input->key_state_previous[scancode] == 0;
}

int ft_input_keyrelease(t_input* input, SDL_Scancode scancode) {
	if(!input) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	return input->key_state[scancode] == 0 && input->key_state_previous[scancode] == 1;
}

/* SECTION: Functions - t_player */

t_player ft_player(t_player_pos ppos, ivec2 bounds) {
	t_player result = { 0 };

	glm_ivec2((ivec2) { 16, 128 }, result.rect.size);
	switch(ppos) {
		case POSITION_LEFT: {
			result.rect.position[0] = 16;
			result.rect.position[1] = bounds[1] / 2 - result.rect.size[1] / 2;
			result.key_up = SDL_SCANCODE_W;
			result.key_down = SDL_SCANCODE_S;
		} break;
		case POSITION_RIGHT: {
			result.rect.position[0] = bounds[0] - 16 - result.rect.size[0];
			result.rect.position[1] = bounds[1] / 2 - result.rect.size[1] / 2;
			result.key_up = SDL_SCANCODE_UP;
			result.key_down = SDL_SCANCODE_DOWN;
		} break;
	}

	result.score = 0;

	return result;
}

int ft_player_update(t_player* player, t_input* input, ivec2 bounds) {
	if(!player) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	player->rect.position[1] += (ft_input_keydown(input, player->key_up) - ft_input_keydown(input, player->key_down)) * 10;
	if(ft_collision_boundaries(player->rect, bounds)) {
		if(player->rect.position[1] <= 0)
			player->rect.position[1] = 0;
		if(player->rect.position[1] + player->rect.size[1] >= bounds[1])
			player->rect.position[1] = bounds[1] - player->rect.size[1];
	}

	return 1;
}

int ft_plyaer_incremenet_score(t_player* player) {
	if(!player) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	player->score++;

	return 1;
}

/* SECTION: Functions - t_ball */

t_ball ft_ball(ivec2 bounds) {
	t_ball res;
	glm_ivec2((ivec2) { 16, 16 }, res.rect.size);
	glm_ivec2((ivec2) { bounds[0] / 2 - res.rect.size[0] / 2, bounds[1] / 2 - res.rect.size[1] / 2 }, res.rect.position);
	glm_ivec2((ivec2) { 0, 0 }, res.direction);

	return res;
}

int ft_ball_restart(t_ball* ball, ivec2 bounds) {
	if(!ball) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	glm_ivec2((ivec2) { bounds[0] / 2 - ball->rect.size[0] / 2, bounds[1] / 2 - ball->rect.size[1] / 2 }, ball->rect.position);
	glm_ivec2((ivec2) { 0, 0 }, ball->direction);

	return 1;
}

int ft_ball_randomize_direction(t_ball* ball) {
	if(!ball) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	int random_x = 0;
	int random_y = 0;

	do {
		random_x = rand() % 2 * (rand() % 2 ? -1 : 1);
		random_y = rand() % 2 * (rand() % 2 ? -1 : 1);
	} while(random_x == 0 || random_y == 0);

	glm_ivec2((ivec2) { random_x, random_y }, ball->direction);

	return 1;
}

int ft_ball_update(t_ball* ball, t_player* p1, t_player* p2, ivec2 bounds) {
	if(!ball || !p1 || !p2) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	if(ft_collision_boundaries(ball->rect, bounds)) {
		if(ball->rect.position[1] <= 0) {
			ball->rect.position[1] = 0;
			ball->direction[1] *= -1;
		} else if(ball->rect.position[1] + ball->rect.size[1] >= bounds[1]) {
			ball->rect.position[1] = bounds[1] - ball->rect.size[1];
			ball->direction[1] *= -1;
		}
	} else if(ft_collision(ball->rect, p1->rect)) {
		if(ft_collision_left_right(ball->rect, p1->rect))
			ball->direction[0] *= -1;
		else if(ft_collision_top_bottom(ball->rect, p1->rect))
			ball->direction[1] *= -1;
	} else if(ft_collision(ball->rect, p2->rect)) {
		if(ft_collision_left_right(ball->rect, p2->rect))
			ball->direction[0] *= -1;
		else if(ft_collision_top_bottom(ball->rect, p2->rect))
			ball->direction[1] *= -1;
	}
	ball->rect.position[0] += 8 * ball->direction[0];
	ball->rect.position[1] += 8 * ball->direction[1];

	return 1;
}

int ft_ball_out_of_bounds(t_ball* ball, ivec2 bounds) {
	if(!ball) {
		fprintf(stderr, "[ERR] %s.%i: %s\n", __FILE_NAME__, __LINE__, strerror(errno));
		return 0;
	}

	return ball->rect.position[0] + ball->rect.size[0] < 0 || ball->rect.position[0] > bounds[0];
}
