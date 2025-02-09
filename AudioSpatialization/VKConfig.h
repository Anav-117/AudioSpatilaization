#pragma once

#ifndef VK_CONFIG
	#define VK_CONFIG
#endif

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vector>

#include "Shaders.h"

struct Transform {
	glm::mat4 M;
	glm::mat4 V;
	glm::mat4 P;

	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
};

struct Vertex {
	glm::vec4 pos;
	glm::vec4 normal;

	bool operator == (Vertex v) {
		if (v.pos == pos && v.normal == normal) {
			return true;
		}

		return false;
	}
};

struct ModelExtent {

	float xMin;
	float xMax;
	float yMin;
	float yMax;
	float zMin;
	float zMax;

};

struct Triangle {
	Vertex vertices[3];

	bool operator == (Triangle t) {
		
		for (int i = 0; i < 3; i++) {
			if (t.vertices[i] != vertices[i]) {
				return false;
			}
		}

		return true;
	}
};


struct QueueFamily {

	uint32_t graphicsFamily;
	uint32_t presentFamily;
	uint32_t computeFamily;

};

struct SwapChainSupport {

	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

};

struct AmpVolume {

	float amp;

};

struct SwapChain {

	VkSwapchainKHR __swapChain;

	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;

	const int MAX_FRAMES_IN_FLIGHT = 2;

	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;
 
	VkSurfaceFormatKHR findSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR findSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

};

class VulkanClass {

private:

	bool enableValidationLayers = true;
	std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	QueueFamily QueueFamilyIndex;
	SwapChainSupport SwapChainDetails;

	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;

	VkSurfaceKHR surface;
	GLFWwindow* window;

	SwapChain swapChain;

	VkDescriptorSetLayout transformDescriptorSetLayout;
	VkDescriptorSetLayout AmpDescriptorSetLayout;
	VkDescriptorSetLayout posDescriptorSetLayout;
	VkDescriptorSetLayout midpointsDescriptorSetLayout;
	VkDescriptorSetLayout sizesDescriptorSetLayout;
	VkDescriptorPool uniformDescriptorPool;
	VkDescriptorPool ampDescriptorPool;
	std::vector<VkDescriptorSet> transformDescriptorSet;
	VkDescriptorPool imguiDescriptorPool;
	VkDescriptorSet ampDescriptorSet;
	VkDescriptorSet posDescriptorSet;
	VkDescriptorSet midpointsDescriptorSet;
	VkDescriptorSet sizesDescriptorSet;

	std::vector<VkBuffer> transformBuffer;
	std::vector<VkDeviceMemory> transformBufferMemory;
	std::vector<void*> transformBufferMap;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Triangle> triangles;
	std::vector<Triangle> Octree;
	std::vector<std::vector<float>> midpoints;
	std::vector<float> midpointsGPU;
	std::vector<unsigned int> Sizes;
	
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	void* vertexBufferMap;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	void* indexBufferMap;

	VkBuffer ampBuffer;
	VkDeviceMemory ampBufferMemory;
	void* ampBufferMap;

	unsigned int posBufferSize;
	VkBuffer posBuffer;
	VkDeviceMemory posBufferMemory;
	void* posBufferMap;

	VkBuffer midpointsBuffer;
	VkDeviceMemory midpointsBufferMemory;
	void* midpointsBufferMap;

	VkBuffer sizesBuffer;
	VkDeviceMemory sizesBufferMemory;
	void* sizesBufferMap;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkPipelineLayout computePipelineLayout;
	VkPipeline computePipeline;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffer;
	VkCommandBuffer computeCommandBuffer;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	Shader* basicShader;

	ModelExtent extents;

public:

	bool first = true;

	bool framebufferResized = false;

	std::vector<VkSemaphore> imageAvailableSemaphore;
	std::vector<VkSemaphore> renderFinishedSempahore;
	std::vector<VkFence> inFlightFence;
	VkSemaphore computeFinishedSemaphore;
	VkFence computeInFlightFence;
	VkFence imGuiFence;

	Transform transform;

	const std::string MODEL_PATH = "models/sponza.obj";
	AmpVolume* ampVolume = nullptr;
	size_t ampVolumeSize;

	VulkanClass();
	VulkanClass(GLFWwindow* win);
	~VulkanClass();

	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	bool findQueueFamilies(VkPhysicalDevice device);
	bool checkSwapChainSupport(VkPhysicalDevice device);
	VkDevice getLogicalDevice() { return logicalDevice; }
	uint32_t getMaxFramesInFlight() { return swapChain.MAX_FRAMES_IN_FLIGHT; }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void dispatch();
	void draw(uint32_t& imageIndex);

	//void initVulkan();
	void createInstance();
	VkPhysicalDevice findPhysicalDevice();
	void createLogicalDevice();
	void createSurface();

	void createSwapChain();
	void createImageViews();
	void recreateSwapChain();

	void createRenderPass();
	void createDescriptorSetLayout();
	void createDescriptorPools();
	void createAmpDescriptorSetLayout();
	void createPosDescriptorSetLayout();

	void createTransformBuffer(VkDeviceSize bufferSize);
	void createTransformDescriptorSet();

	void createAmpDescriptorSet();
	void createPosDescriptorSet();

	void updateTransform();

	void createGraphicsPipeline();
	void createComputePipeline();
	void createFramebuffers();

	void createCommandPool();
	void createCommandBuffer();

	void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index, uint32_t currentFrame);

	void createDepthResources();

	void createSyncObjects();

	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createAmpBuffer();
	void createOctree();
	void createTriangleBuffer();
	void createAuxilaryOctreeBuffers();

	void validateAmpBuffer();

	void initImGui();
	void drawGui();

};