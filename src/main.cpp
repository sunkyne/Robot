#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include "MatrixStack.h"
#include "Program.h"


#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

char* vertShaderPath = "../shaders/shader.vert";
char* fragShaderPath = "../shaders/shader.frag";

GLFWwindow *window;
double currentXpos, currentYpos;
glm::vec3 eye(0.0f, 0.0f, 8.0f);
glm::vec3 center(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 right;

Program program;
MatrixStack modelViewProjectionMatrix;


// Draw cube on screen
void DrawCube(glm::mat4& modelViewProjectionMatrix)
{
	program.SendUniformData(modelViewProjectionMatrix, "mvp");
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

class RobotElements
{
private:
	std::vector<RobotElements*> children;
	RobotElements* parent;
	glm::vec3 jt; // joint translation
	glm::vec3 ja; // joint angle
	glm::vec3 t; // translation
	glm::vec3 s; // scale
public:
	RobotElements() {
		this->jt = glm::vec3(0.0f);
		this->ja = glm::vec3(0.0f);
		this->t = glm::vec3(0.0f);
		this->s = glm::vec3(1.0f);
		this->parent = nullptr;
	}
	RobotElements(RobotElements &parent) {
		this->jt = glm::vec3(0.0f);
		this->ja = glm::vec3(0.0f);
		this->t = glm::vec3(0.0f);
		this->s = glm::vec3(1.0f);
		this->parent = &parent;
	}
	RobotElements(glm::vec3 jt, glm::vec3 ja, glm::vec3 t, glm::vec3 s) {
		this->jt = jt;
		this->ja = ja;
		this->t = t;
		this->s = s;
		this->parent = nullptr;
	}
	RobotElements(glm::vec3 jt, glm::vec3 ja, glm::vec3 t, glm::vec3 s, RobotElements &parent) {
		this->jt = jt;
		this->ja = ja;
		this->t = t;
		this->s = s;
		this->parent = &parent;
	}
	~RobotElements() {}
	void addChild(RobotElements *child) {
		children.push_back(child);
	}
	std::vector<RobotElements*> getChildren() {
		return children;
	}
	RobotElements* getParent() {
		return this->parent;
	}
	void setJointTranslation(glm::vec3 jt) {
		this->jt = jt;
	}
	void setJointAngle(glm::vec3 ja) {
		this->ja = ja;
	}
	void incXAngle(float k) {
		ja.x += k;
	}
	void incYAngle(float k) {
		ja.y += k;
	}
	void incZAngle(float k) {
		ja.z += k;
	}
	void setTranslation(glm::vec3 t) {
		this->t = t;
	}
	void setScale(glm::vec3 s) {
		this->s = s;
	}
	void select() {
		this->s *= 1.1;
	}
	void deselect() {
		this->s /= 1.1;
	}
	void DrawRobotElement(MatrixStack& modelViewProjectionMatrix) {
		modelViewProjectionMatrix.pushMatrix();
		modelViewProjectionMatrix.translate(t);
		modelViewProjectionMatrix.translate(-jt);
		modelViewProjectionMatrix.rotateX(glm::radians(ja.x));
		modelViewProjectionMatrix.rotateY(glm::radians(ja.y));
		modelViewProjectionMatrix.rotateZ(glm::radians(ja.z));
		modelViewProjectionMatrix.translate(jt);
		modelViewProjectionMatrix.pushMatrix();
		modelViewProjectionMatrix.scale(s);
		DrawCube(modelViewProjectionMatrix.topMatrix());
		modelViewProjectionMatrix.popMatrix();
		if (!children.empty()) {
			for (int i = 0; i < children.size(); i++) {
				children[i]->DrawRobotElement(modelViewProjectionMatrix);
			}
		}
		modelViewProjectionMatrix.popMatrix();
	}
};

std::vector<RobotElements*> q;
std::vector<RobotElements*>::iterator i;
RobotElements* selected;
RobotElements* root;

void qInit(RobotElements* k) {
	q.push_back(k);
	std::vector<RobotElements*> children = k->getChildren();
	for (int i = 0; i < children.size(); i++) {
		qInit(children[i]);
	}
}

void ConstructRobot()
{
	RobotElements* torso = new RobotElements();
	torso->setScale(glm::vec3(1.0f, 1.5f, 0.5f));
	
	RobotElements* head = new RobotElements(*torso);
	torso->addChild(head);
	head->setTranslation(glm::vec3(0.0f, 2.5f, 0.0f));
	head->setJointTranslation(glm::vec3(0.0f, 1.0f, 0.0f));
	head->setScale(glm::vec3(1.0f, 1.0f, 1.0f));
	RobotElements* ula = new RobotElements(*torso);
	torso->addChild(ula);
	ula->setTranslation(glm::vec3(-1.75f, 1.0f, 0.0f));
	ula->setJointTranslation(glm::vec3(-0.75f, 0.0f, 0.0f));
	ula->setScale(glm::vec3(0.75f, 0.5f, 0.5f));
	RobotElements* ura = new RobotElements(*torso);
	torso->addChild(ura);
	ura->setTranslation(glm::vec3(1.75f, 1.0f, 0.0f));
	ura->setJointTranslation(glm::vec3(0.75f, 0.0f, 0.0f));
	ura->setScale(glm::vec3(0.75f, 0.5f, 0.5f));
	RobotElements* ull = new RobotElements(*torso);
	torso->addChild(ull);
	ull->setTranslation(glm::vec3(-0.5f, -2.25f, 0.0f));
	ull->setJointTranslation(glm::vec3(0.0f, -0.75f, 0.0f));
	ull->setScale(glm::vec3(0.5f, 0.75f, 0.5f));
	RobotElements* url = new RobotElements(*torso);
	torso->addChild(url);
	url->setTranslation(glm::vec3(0.5f, -2.25f, 0.0f));
	url->setJointTranslation(glm::vec3(0.0f, -0.75f, 0.0f));
	url->setScale(glm::vec3(0.5f, 0.75f, 0.5f));

	RobotElements* lla = new RobotElements(*ula);
	ula->addChild(lla);
	lla->setTranslation(glm::vec3(-1.25f, 0.0f, 0.0f));
	lla->setJointTranslation(glm::vec3(-0.5f, 0.0f, 0.0f));
	lla->setScale(glm::vec3(0.5f, 0.5f, 0.5f));
	RobotElements* lra = new RobotElements(*ura);
	ura->addChild(lra);
	lra->setTranslation(glm::vec3(1.25f, 0.0f, 0.0f));
	lra->setJointTranslation(glm::vec3(0.5f, 0.0f, 0.0f));
	lra->setScale(glm::vec3(0.5f, 0.5f, 0.5f));
	RobotElements* lll = new RobotElements(*ull);
	ull->addChild(lll);
	lll->setTranslation(glm::vec3(0.0f, -1.5f, 0.0f));
	lll->setJointTranslation(glm::vec3(0.0f, -0.75f, 0.0f));
	lll->setScale(glm::vec3(0.5f, 0.75f, 0.5f));
	RobotElements* lrl = new RobotElements(*url);
	url->addChild(lrl);
	lrl->setTranslation(glm::vec3(0.0f, -1.5f, 0.0f));
	lrl->setJointTranslation(glm::vec3(0.0f, -0.75f, 0.0f));
	lrl->setScale(glm::vec3(0.5f, 0.75f, 0.5f));

	root = torso;
}

void traverseUp() {
	if (i != q.begin()) {
		selected->deselect();
		selected = selected->getParent() != nullptr ? selected->getParent() : selected;
		i = std::find(q.begin(), q.end(), selected) + 1;
		selected->select();
	}
}

void traverseDown() {
	if (i != q.end()) {
		if (selected != nullptr) {
			selected->deselect();
		}
		selected = *i;
		i++;
		selected->select();
	}
}

void Display()
{	
	program.Bind();

	modelViewProjectionMatrix.loadIdentity();
	modelViewProjectionMatrix.pushMatrix();

	// Setting the view and Projection matrices
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	modelViewProjectionMatrix.Perspective(glm::radians(60.0f), float(width) / float(height), 0.1f, 100.0f);
	modelViewProjectionMatrix.LookAt(eye, center, up);
	
	root->DrawRobotElement(modelViewProjectionMatrix);

	modelViewProjectionMatrix.popMatrix();

	program.Unbind();
	
}

double prevX = 0.0;
double prevY = 0.0;

// Mouse callback function
void MouseCallback(GLFWwindow* lWindow, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && GLFW_PRESS == action)
		std::cout << "Mouse left button is pressed." << std::endl;
}

// Mouse position callback function
void CursorPositionCallback(GLFWwindow* lWindow, double xpos, double ypos)
{
	glm::vec3 a = eye - center;
	glm::vec3 right = glm::normalize(glm::cross(a, up));
	glm::vec3 nup = glm::normalize(up);
	float x = (xpos - prevX) * 0.01f;
	float y = (ypos - prevY) * 0.01f;

	int lc = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (lc == GLFW_PRESS) {
		double angle1 = x;
		glm::mat3 rot1;
		double c1 = cos(angle1);
		double s1 = sin(angle1);
		rot1[0] = glm::vec3((1-c1)*nup.x*nup.x + c1, (1-c1)*nup.x*nup.y + nup.z*s1, (1-c1)*nup.x*nup.z-nup.y*s1);
		rot1[1] = glm::vec3((1-c1)*nup.x*nup.y - nup.z*s1, (1-c1)*nup.y*nup.y + c1, (1-c1)*nup.y*nup.z + nup.x*s1);
		rot1[2] = glm::vec3((1-c1)*nup.x*nup.z + nup.y*s1, (1-c1)*nup.y*nup.z - nup.x*s1, (1-c1)*nup.z*nup.z + c1);

		double angle2 = -y;
		glm::mat3 rot2;
		double c2 = cos(angle2);
		double s2 = sin(angle2);
		rot2[0] = glm::vec3((1 - c2) * right.x * right.x + c2, (1 - c2) * right.x * right.y + right.z * s2, (1 - c2) * right.x * right.z - right.y * s2);
		rot2[1] = glm::vec3((1 - c2) * right.x * right.y - right.z * s2, (1 - c2) * right.y * right.y + c2, (1 - c2) * right.y * right.z + right.x * s2);
		rot2[2] = glm::vec3((1 - c2) * right.x * right.z + right.y * s2, (1 - c2) * right.y * right.z - right.x * s2, (1 - c2) * right.z * right.z + c2);

		a = a * rot1 * rot2 + center;
		eye = a;
		up = glm::normalize(glm::cross(right, a));
	}

	int rc = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
	if (rc == GLFW_PRESS) {
		eye += right * x;
		center += right * x;
		eye += up * y;
		center += up * y;
	}

	prevX = xpos;
	prevY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	glm::vec3 a = eye - center;
	if (yoffset < 0) {
		a *= 1.1;
	}
	else if (yoffset > 0) {
		a /= 1.1;
	}
	a += center;
	eye = a;
}

// Keyboard character callback function
void CharacterCallback(GLFWwindow* lWindow, unsigned int key)
{
	std::cout << "Key " << (char)key << " is pressed." << std::endl;
	switch (key) {
		case '.':
			traverseDown();
			break;
		case ',':
			traverseUp();
			break;
		case 'x':
			if (selected != nullptr) {
				selected->incXAngle(0.5f);
			}
			break;
		case 'X':
			if (selected != nullptr) {
				selected->incXAngle(-0.5f);
			}
			break;
		case 'y':
			if (selected != nullptr) {
				selected->incYAngle(0.5f);
			}
			break;
		case 'Y':
			if (selected != nullptr) {
				selected->incYAngle(-0.5f);
			}
			break;
		case 'z':
			if (selected != nullptr) {
				selected->incZAngle(0.5f);
			}
			break;
		case 'Z':
			if (selected != nullptr) {
				selected->incZAngle(-0.5f);
			}
			break;
	}
}

void CreateCube()
{
	// x, y, z, r, g, b, ...
	float cubeVerts[] = {
		// Face x-
		-1.0f,	+1.0f,	+1.0f,	0.8f,	0.2f,	0.2f,
		-1.0f,	+1.0f,	-1.0f,	0.8f,	0.2f,	0.2f,
		-1.0f,	-1.0f,	+1.0f,	0.8f,	0.2f,	0.2f,
		-1.0f,	-1.0f,	+1.0f,	0.8f,	0.2f,	0.2f,
		-1.0f,	+1.0f,	-1.0f,	0.8f,	0.2f,	0.2f,
		-1.0f,	-1.0f,	-1.0f,	0.8f,	0.2f,	0.2f,
		// Face x+
		+1.0f,	+1.0f,	+1.0f,	0.8f,	0.2f,	0.2f,
		+1.0f,	-1.0f,	+1.0f,	0.8f,	0.2f,	0.2f,
		+1.0f,	+1.0f,	-1.0f,	0.8f,	0.2f,	0.2f,
		+1.0f,	+1.0f,	-1.0f,	0.8f,	0.2f,	0.2f,
		+1.0f,	-1.0f,	+1.0f,	0.8f,	0.2f,	0.2f,
		+1.0f,	-1.0f,	-1.0f,	0.8f,	0.2f,	0.2f,
		// Face y-
		+1.0f,	-1.0f,	+1.0f,	0.2f,	0.8f,	0.2f,
		-1.0f,	-1.0f,	+1.0f,	0.2f,	0.8f,	0.2f,
		+1.0f,	-1.0f,	-1.0f,	0.2f,	0.8f,	0.2f,
		+1.0f,	-1.0f,	-1.0f,	0.2f,	0.8f,	0.2f,
		-1.0f,	-1.0f,	+1.0f,	0.2f,	0.8f,	0.2f,
		-1.0f,	-1.0f,	-1.0f,	0.2f,	0.8f,	0.2f,
		// Face y+
		+1.0f,	+1.0f,	+1.0f,	0.2f,	0.8f,	0.2f,
		+1.0f,	+1.0f,	-1.0f,	0.2f,	0.8f,	0.2f,
		-1.0f,	+1.0f,	+1.0f,	0.2f,	0.8f,	0.2f,
		-1.0f,	+1.0f,	+1.0f,	0.2f,	0.8f,	0.2f,
		+1.0f,	+1.0f,	-1.0f,	0.2f,	0.8f,	0.2f,
		-1.0f,	+1.0f,	-1.0f,	0.2f,	0.8f,	0.2f,
		// Face z-
		+1.0f,	+1.0f,	-1.0f,	0.2f,	0.2f,	0.8f,
		+1.0f,	-1.0f,	-1.0f,	0.2f,	0.2f,	0.8f,
		-1.0f,	+1.0f,	-1.0f,	0.2f,	0.2f,	0.8f,
		-1.0f,	+1.0f,	-1.0f,	0.2f,	0.2f,	0.8f,
		+1.0f,	-1.0f,	-1.0f,	0.2f,	0.2f,	0.8f,
		-1.0f,	-1.0f,	-1.0f,	0.2f,	0.2f,	0.8f,
		// Face z+
		+1.0f,	+1.0f,	+1.0f,	0.2f,	0.2f,	0.8f,
		-1.0f,	+1.0f,	+1.0f,	0.2f,	0.2f,	0.8f,
		+1.0f,	-1.0f,	+1.0f,	0.2f,	0.2f,	0.8f,
		+1.0f,	-1.0f,	+1.0f,	0.2f,	0.2f,	0.8f,
		-1.0f,	+1.0f,	+1.0f,	0.2f,	0.2f,	0.8f,
		-1.0f,	-1.0f,	+1.0f,	0.2f,	0.2f,	0.8f
	};

	GLuint vertBufferID;
	glGenBuffers(1, &vertBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
	GLint posID = glGetAttribLocation(program.GetPID(), "position");
	glEnableVertexAttribArray(posID);
	glVertexAttribPointer(posID, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	GLint colID = glGetAttribLocation(program.GetPID(), "color");
	glEnableVertexAttribArray(colID);
	glVertexAttribPointer(colID, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));

}

void FrameBufferSizeCallback(GLFWwindow* lWindow, int width, int height)
{
	glViewport(0, 0, width, height);
}

void Init()
{
	glfwInit();
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Assignment2 - Kyne Sun", NULL, NULL);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorPositionCallback);
	glfwSetCharCallback(window, CharacterCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	program.SetShadersFileName(vertShaderPath, fragShaderPath);
	program.Init();

	CreateCube();
	ConstructRobot();
	qInit(root);
	i = q.begin();
}


int main()
{	
	Init();
	while ( glfwWindowShouldClose(window) == 0) 
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		Display();
		glFlush();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}