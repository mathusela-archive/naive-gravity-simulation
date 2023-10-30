#include <iostream>
#include <vector>
#include <cmath>
#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <tuple>

#define PRECISION_TYPE long double
const float G = 6.67 * pow(10, -11); 
const float ACCURACY_THRESHOLD = 15.0;

template <typename t_type>
struct Vector {
	t_type i;
	t_type j;

	Vector operator + (Vector<t_type> rhs) {
		Vector out;
		out.i = i + rhs.i;
		out.j = j + rhs.j;
		return out;
	}

	Vector operator - (Vector<t_type> rhs) {
		Vector out;
		out.i = i - rhs.i;
		out.j = j - rhs.j;
		return out;
	}

	Vector operator * (Vector<t_type> rhs) {
		Vector out;
		out.i = i * rhs.i;
		out.j = j * rhs.j;
		return out;
	}

	Vector operator / (Vector<t_type> rhs) {
		Vector out;
		out.i = i / rhs.i;
		out.j = j / rhs.j;
		return out;
	}

	auto operator += (Vector<t_type> rhs) {
		i += rhs.i;
		j += rhs.j;
	}

	Vector power(float index) {
		Vector out;
		out.i = pow(i, index);
		out.j = pow(i, index);
		return out;
	}

	Vector fill(t_type input) {
		Vector out;
		out.i = input;
		out.j = input;
		return out;
	}

	auto length() {
		return sqrt(pow(i, 2) + pow(j, 2));
	}

	Vector normalize() {
		auto thisVec = Vector<t_type>{i, j};
		return thisVec / Vector<t_type>{}.fill(thisVec.length());
	}
};

class Body {
protected:
	float m_mass;
	Vector<PRECISION_TYPE> m_position;
	Vector<PRECISION_TYPE> m_velocity = Vector<PRECISION_TYPE> {0.0, 0.0};

public:
	Body(float mass, Vector<PRECISION_TYPE> position, Vector<PRECISION_TYPE> velocity) {
		m_mass = mass;
		m_position = position;
		m_velocity = velocity;
	}

	Body(float mass, Vector<PRECISION_TYPE> position) {
		m_mass = mass;
		m_position = position;
	}

	auto get_mass() {return m_mass;}
	auto get_position() {return m_position;}
	auto get_velocity() {return m_velocity;}

	auto simulate(std::vector<Body> simulationBodies, float timeStep) {
		Vector<PRECISION_TYPE> acceleration = {0, 0};
		for (auto body : simulationBodies) {
			auto distance = body.get_position() - m_position;
			if (distance.length() >= ACCURACY_THRESHOLD) {
				acceleration += Vector<PRECISION_TYPE>{}.fill( (G * body.get_mass()) / pow(distance.length(), 2) ) * distance.normalize();
			}
		}
		m_velocity += acceleration * Vector<PRECISION_TYPE>{}.fill(timeStep);
		m_position += m_velocity * Vector<PRECISION_TYPE>{}.fill(timeStep);
	}
};

// GUI binding
auto init_window(unsigned int width, unsigned int height, char* title) {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	auto window = glfwCreateWindow(width, height, title, NULL, NULL);
	glfwMakeContextCurrent(window);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glViewport(0, 0, width, height);

	return window;
}

auto setup_opengl_vars() {
	const char *vertexShaderSource = 
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPos;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 world;\n"
		"void main() {\n"
		"   gl_Position = projection * world * vec4(aPos, 1.0f);\n" 
		"}";

	const char *fragmentShaderSource = 
		"#version 330 core\n"
		"uniform vec3 color;\n"
		"out vec4 FragColor;\n"
		"void main() {\n"
		"   FragColor = vec4(color, 1.0f);\n"
		"}";

	const float verticies[] = {
		0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
	};

	unsigned int tris[] = {
		0, 1, 2,
		1, 2, 3
	};

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticies), verticies, GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tris), tris, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
	glEnableVertexAttribArray(0);

	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader); glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader); glDeleteShader(fragmentShader);

	return std::make_pair(VAO, shaderProgram);
}

auto setup_projection(float width, float height) {
	return glm::ortho(0.0f, width, 0.0f, height);
}

template <typename t_type>
auto gen_translation_matrix(Vector<t_type> vecIn) {
	auto out = glm::mat4(1.0);
	out = glm::translate(out, glm::vec3(vecIn.i, vecIn.j, 0.0));
	return out;
}

template <typename t_type>
auto gen_translation_matrix(Vector<t_type> vecIn, float scale) {
	auto out = glm::mat4(1.0);
	out = glm::translate(out, glm::vec3(vecIn.i, vecIn.j, 0.0));
	out = glm::scale(out, glm::vec3(scale));
	return out;
}

class RenderBody : public Body {
public:
	glm::mat4 translationMatrix;

	auto update_translation_matrix(double scale) {
		translationMatrix = gen_translation_matrix(get_position(), scale);
	}

	auto draw(unsigned int shaderProgram, float scale, glm::vec3 color) {
		update_translation_matrix(scale);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "world"), 1, GL_FALSE, glm::value_ptr(translationMatrix));
		glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	RenderBody(float mass, Vector<PRECISION_TYPE> position, Vector<PRECISION_TYPE> velocity) : Body(mass, position, velocity) {}

	RenderBody(float mass, Vector<PRECISION_TYPE> position) : Body(mass, position) {}
};

int main() {
	auto window = init_window(1000, 1000, "Orbital Mechanics");
	auto openglVars = setup_opengl_vars();
	auto VAO = openglVars.first; auto shaderProgram = openglVars.second;
	// auto projectionMatrix = setup_projection((149600000000.0 + 360000000.0)*4.0, (149600000000.0 + 360000000.0)*4.0);
	auto projectionMatrix = setup_projection(1000, 1000);

	// RenderBody earth(5.972 * pow(10, 24), Vector<PRECISION_TYPE> {149600000000.0 + ((149600000000.0 + 360000000.0)*2.0), 0.0 + ((149600000000.0 + 360000000.0)*2.0)}, Vector<PRECISION_TYPE> {0.0, 29780.0});
	// RenderBody moon(7.348 * pow(10, 22), Vector<PRECISION_TYPE> {149600000000.0 + 360000000.0 + ((149600000000.0 + 360000000.0)*2.0), 0.0 + ((149600000000.0 + 360000000.0)*2.0)}, Vector<PRECISION_TYPE> {0.0, 29780.0 + 1083.0});
	// RenderBody sun(1.989 * pow(10, 30), Vector<PRECISION_TYPE> {((149600000000.0 + 360000000.0)*2.0), ((149600000000.0 + 360000000.0)*2.0)}, Vector<PRECISION_TYPE> {0.0, 0.0});
	RenderBody testBody1(100000000000000, Vector<PRECISION_TYPE> {500, 500}, Vector<PRECISION_TYPE> {0.0, 0.0});
	RenderBody testBody2(1000000000000, Vector<PRECISION_TYPE> {500, 600}, Vector<PRECISION_TYPE> {10.0, 0.0});
	RenderBody testBody3(3000000000000, Vector<PRECISION_TYPE> {600, 700}, Vector<PRECISION_TYPE> {-5.0, 0.0});

	float timeStep = 1.0;
	float fps = (1/60.0);
	// float fps = (1/60);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		
		// earth.simulate(std::vector<Body> {moon, sun}, timeStep);
		// moon.simulate(std::vector<Body> {earth, sun}, timeStep);
		// sun.simulate(std::vector<Body> {earth, moon}, timeStep);
		testBody1.simulate(std::vector<Body> {testBody2, testBody3}, timeStep);
		testBody2.simulate(std::vector<Body> {testBody1, testBody3}, timeStep);
		testBody3.simulate(std::vector<Body> {testBody1, testBody2}, timeStep);

		glBindVertexArray(VAO);
		glUseProgram(shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		// earth.draw(shaderProgram, + ((149600000000.0 + 360000000.0)*4.0)/250.0, glm::vec3(0.0, 1.0, 0.0));
		// moon.draw(shaderProgram, + ((149600000000.0 + 360000000.0)*4.0)/250.0, glm::vec3(1.0, 1.0, 1.0));
		// sun.draw(shaderProgram, + ((149600000000.0 + 360000000.0)*4.0)/250.0, glm::vec3(1.0, 1.0, 0.0));
		testBody1.draw(shaderProgram, 10.0, glm::vec3(1.0, 0.0, 0.0));
		testBody2.draw(shaderProgram, 10.0, glm::vec3(0.0, 1.0, 0.0));
		testBody3.draw(shaderProgram, 10.0, glm::vec3(0.0, 0.0, 1.0));

		glfwSwapBuffers(window);
		glfwPollEvents();

		Sleep(1000 * fps);
	}
	
	glfwTerminate();
	return 0;
}