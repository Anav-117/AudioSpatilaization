#include "VKConfig.h"
#include <iostream>

VulkanClass* vk;

namespace win {
	int width = 1920;
	int height = 1080;
}

namespace camera {
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 fwd = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 right;
	glm::vec3 up;
	float angle = 0;
	float Xangle = 0;
}

Transform transform;

namespace hostSwapChain {
	uint32_t currentFrame = 0;
}


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}
	if (key == GLFW_KEY_A) {
		camera::pos -= 0.1f * glm::cross(camera::fwd, glm::vec3(0.0, 1.0, 0.0));
	}
	if (key == GLFW_KEY_D) {
		camera::pos += 0.1f * glm::cross(camera::fwd, glm::vec3(0.0, 1.0, 0.0));
	}
	if (key == GLFW_KEY_Q) {
		camera::pos += glm::vec3(0.0f, 0.1f, 0.0f);
	}
	if (key == GLFW_KEY_E) {
		camera::pos += glm::vec3(0.0f, -0.1f, 0.0f);
	}
	if (key == GLFW_KEY_W) {
		//camera::pos += glm::vec3(0.0f, 0.0f, 0.1f);
		camera::pos += 0.1f * camera::fwd;
	}
	if (key == GLFW_KEY_S) {
		//camera::pos += glm::vec3(0.0f, 0.0f, -0.1f);
		camera::pos -= 0.1f * camera::fwd;
	}
	if (key == GLFW_KEY_LEFT) {
		camera::angle = 0.1f;
		camera::fwd = glm::rotate(camera::fwd, camera::angle, camera::up);
		//camera::fwd = glm::vec3(sin(camera::angle), camera::fwd.y, cos(camera::angle));
	}
	if (key == GLFW_KEY_RIGHT) {
		camera::angle = -0.1f;
		camera::fwd = glm::rotate(camera::fwd, camera::angle, camera::up);
		//camera::fwd = glm::vec3(sin(camera::angle), camera::fwd.y, cos(camera::angle));
	}
	if (key == GLFW_KEY_UP) {
		camera::Xangle = -0.1f;
		if (glm::dot(camera::fwd, glm::vec3(0.0f, -1.0f, 0.0f)) > 0.9) {
			return;
		}
		camera::fwd = glm::rotate(camera::fwd, camera::Xangle, camera::right);
		//camera::fwd = glm::vec3(camera::fwd.x, sin(camera::Xangle), cos(camera::Xangle));
	}
	if (key == GLFW_KEY_DOWN) {
		if (glm::dot(camera::fwd, glm::vec3(0.0f, 1.0f, 0.0f)) > 0.9) {
			return;
		}
		camera::Xangle = 0.1f;
		camera::fwd = glm::rotate(camera::fwd, camera::Xangle, camera::right);
		//camera::fwd = glm::vec3(camera::fwd.x, sin(camera::Xangle), cos(camera::Xangle));
	}

}

void windowResizeCallback(GLFWwindow* window, int width, int height) {

	win::width = width;
	win::height = height;
	vk->framebufferResized = true;

}

void display() {

	vkWaitForFences(vk->getLogicalDevice(), 1, &vk->inFlightFence[hostSwapChain::currentFrame], VK_TRUE, UINT32_MAX);

	vk->dispatch();

	vkWaitForFences(vk->getLogicalDevice(), 1, &vk->computeInFlightFence, VK_TRUE, UINT64_MAX);

	//vk->validateAmpBuffer();

	vk->draw(hostSwapChain::currentFrame);

	hostSwapChain::currentFrame = (hostSwapChain::currentFrame + 1) % vk->getMaxFramesInFlight();

	//vk->drawGui();

}

void idle() {

	camera::right = glm::cross(camera::fwd, glm::vec3(0.0f, 1.0, 0.0f));
	camera::up = glm::cross(camera::right, camera::fwd);

	transform.M = glm::scale(glm::mat4(1.0f), glm::vec3(0.005f)) * glm::mat4(1.0f);
	transform.V = glm::lookAt(camera::pos, camera::pos + camera::fwd, glm::vec3(0.0f, 1.0f, 0.0f));
	transform.P = glm::perspective(glm::radians(45.0f), win::width / (float)win::height, 0.1f, 10000.0f);

	transform.cameraPos = camera::pos;
	transform.cameraFront = camera::fwd;

	vk->transform = transform;

	vk->updateTransform();

}

int main() {

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(win::width, win::height, "GTX", 0, nullptr);



	vk = new VulkanClass(window);
	vk->createTransformBuffer(sizeof(transform));
	vk->createTransformDescriptorSet();
	//vk->createAmpBuffer();
	vk->createAmpDescriptorSet();
	vk->createPosDescriptorSet();

	glfwSetKeyCallback(window, keyboardCallback);
	glfwSetWindowSizeCallback(window, windowResizeCallback);

	while (!glfwWindowShouldClose(window)) {

		idle();
		display();

		glfwPollEvents();

	}

	vkDeviceWaitIdle(vk->getLogicalDevice());

	delete vk;

	return 0;

}