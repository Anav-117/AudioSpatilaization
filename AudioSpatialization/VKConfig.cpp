#include "VkConfig.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <algorithm>
#include <set>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

std::vector<const char*> VulkanClass::getRequiredExtensions() {

	uint32_t glfwExtentionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtentionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;

}

bool VulkanClass::checkValidationLayerSupport() {

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;

}

VulkanClass::VulkanClass() {

}

VulkanClass::VulkanClass(GLFWwindow* win) {

	window = win;
	createInstance();

	createSurface();

	physicalDevice = findPhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createImageViews();

	loadModel();

	createRenderPass();
	createDescriptorSetLayout();
	createAmpDescriptorSetLayout();
	createPosDescriptorSetLayout();
	createDescriptorPools();

	basicShader = new Shader("shader", logicalDevice);

	createDepthResources();
	createFramebuffers();

	createCommandPool();
	createCommandBuffer();

	createVertexBuffer();
	//createIndexBuffer();
	createAmpBuffer();
	createOctree();
	createTriangleBuffer();
	createAuxilaryOctreeBuffers();

	createComputePipeline();
	createGraphicsPipeline();

	createSyncObjects();

	//initImGui();

}

VulkanClass::~VulkanClass() {

	vkDestroyImageView(logicalDevice, depthImageView, nullptr);
	vkDestroyImage(logicalDevice, depthImage, nullptr);
	vkFreeMemory(logicalDevice, depthImageMemory, nullptr);

	for (size_t i = 0; i < swapChain.framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice, swapChain.framebuffers[i], nullptr);
	}

	for (size_t i = 0; i < swapChain.imageViews.size(); i++) {
		vkDestroyImageView(logicalDevice, swapChain.imageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(logicalDevice, swapChain.__swapChain, nullptr);

	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);

	//vkDestroyDescriptorPool(logicalDevice, imguiDescriptorPool, nullptr);
	//ImGui_ImplVulkan_Shutdown();

	vkDestroyBuffer(logicalDevice, ampBuffer, nullptr);
	vkFreeMemory(logicalDevice, ampBufferMemory, nullptr);

	vkDestroyBuffer(logicalDevice, posBuffer, nullptr);
	vkFreeMemory(logicalDevice, posBufferMemory, nullptr);

	vkDestroyBuffer(logicalDevice, midpointsBuffer, nullptr);
	vkFreeMemory(logicalDevice, midpointsBufferMemory, nullptr);

	vkDestroyBuffer(logicalDevice, sizesBuffer, nullptr);
	vkFreeMemory(logicalDevice, sizesBufferMemory, nullptr);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(logicalDevice, transformBuffer[i], nullptr);
		vkFreeMemory(logicalDevice, transformBufferMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(logicalDevice, uniformDescriptorPool, nullptr);
	vkDestroyDescriptorPool(logicalDevice, ampDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, transformDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, AmpDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, posDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, midpointsDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(logicalDevice, sizesDescriptorSetLayout, nullptr);

	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);

	vkDestroyPipeline(logicalDevice, computePipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, computePipelineLayout, nullptr);

	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	delete basicShader;

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(logicalDevice, renderFinishedSempahore[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFence[i], nullptr);
	}

	vkDestroySemaphore(logicalDevice, computeFinishedSemaphore, nullptr);
	vkDestroyFence(logicalDevice, computeInFlightFence, nullptr);

	vkDestroyFence(logicalDevice, imGuiFence, nullptr);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer[i]);
	}
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyInstance(instance, nullptr);

}

void VulkanClass::createInstance() {

	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("Validation Layers Requested But Not Found\n");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Lego Ocean";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.pNext = nullptr;

	auto extensions = getRequiredExtensions();

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan Instance\n");
	}

}

bool VulkanClass::findQueueFamilies(VkPhysicalDevice device) {

	uint32_t physicalDeviceQueueFamilyCount;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, nullptr);

	if (physicalDeviceQueueFamilyCount == 0) {
		throw std::runtime_error("Cannot Find Any Queue Families On Physical Device\n");
	}

	std::vector<VkQueueFamilyProperties> queueFamilies(physicalDeviceQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &physicalDeviceQueueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (auto queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_COMPUTE_BIT > 0) {
			QueueFamilyIndex.graphicsFamily = i;
			return true;
		}

		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			QueueFamilyIndex.presentFamily = i;
		}

		i++;
	}

	return false;

}

bool VulkanClass::checkSwapChainSupport(VkPhysicalDevice device) {

	SwapChainSupport details{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t numFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numFormats, nullptr);
	details.formats.resize(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numFormats, details.formats.data());

	uint32_t numPresentModes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, nullptr);
	details.presentModes.resize(numPresentModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentModes, details.presentModes.data());

	if (!details.formats.empty() && !details.presentModes.empty()) {
		SwapChainDetails = details;
		return true;
	}

	return false;

}

VkPhysicalDevice VulkanClass::findPhysicalDevice() {

	VkPhysicalDevice selectedDevice = NULL;

	uint32_t numSupportedDevices;

	if (vkEnumeratePhysicalDevices(instance, &numSupportedDevices, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("No Supported Physical Devices Found\n");
	}

	std::vector<VkPhysicalDevice> physicalDevices(numSupportedDevices);

	vkEnumeratePhysicalDevices(instance, &numSupportedDevices, physicalDevices.data());

	for (auto device : physicalDevices) {

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		if (!findQueueFamilies(device) || !requiredExtensions.empty() || !(features.tessellationShader) || !checkSwapChainSupport(device) || properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			continue;
		}

		selectedDevice = device;
		std::cout << properties.vendorID << " | " << properties.deviceName << " | " << properties.deviceType << " | " << properties.driverVersion << "\n";
		std::cout << "MAX SSBO SIZE - " << properties.limits.maxStorageBufferRange << "\n";

	}

	if (selectedDevice == NULL) {
		throw std::runtime_error("Cannot Find Suitable Physical Device\n");
	}

	return selectedDevice;

}

void VulkanClass::createLogicalDevice() {

	float queuePriority = 1.0;
	std::vector<VkDeviceQueueCreateInfo> queueInfos;

	std::set<uint32_t> UniqueQueueFamilies = { QueueFamilyIndex.graphicsFamily, QueueFamilyIndex.presentFamily };

	for (auto queue : UniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pQueuePriorities = &queuePriority;
		queueInfo.queueCount = 1;
		queueInfo.queueFamilyIndex = queue;
		queueInfo.flags = 0;
		queueInfo.pNext = nullptr;
		queueInfos.push_back(queueInfo);
	}

	VkPhysicalDeviceFeatures supportedFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	VkPhysicalDeviceFeatures requiredFeatures{};
	requiredFeatures.tessellationShader = VK_TRUE;
	requiredFeatures.fillModeNonSolid = VK_TRUE;
	requiredFeatures.wideLines = VK_TRUE;

	VkDeviceCreateInfo logicalDeviceCreateInfo{};

	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pNext = nullptr;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
	logicalDeviceCreateInfo.pQueueCreateInfos = queueInfos.data();
	logicalDeviceCreateInfo.pEnabledFeatures = &requiredFeatures;
	if (enableValidationLayers) {
		logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		logicalDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		logicalDeviceCreateInfo.enabledLayerCount = 0;
		logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
	}
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	logicalDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Logical Device\n");
	}

	vkGetDeviceQueue(logicalDevice, QueueFamilyIndex.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, QueueFamilyIndex.presentFamily, 0, &presentQueue);
	vkGetDeviceQueue(logicalDevice, QueueFamilyIndex.computeFamily, 0, &computeQueue);

}

VkSurfaceFormatKHR SwapChain::findSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

	for (const auto format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return availableFormats[0];

}

VkPresentModeKHR SwapChain::findSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

	for (const auto& presentMode : availablePresentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;

}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {

	if (false) { //capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

}

void VulkanClass::createSwapChain() {

	swapChain.format = swapChain.findSwapChainFormat(SwapChainDetails.formats);
	swapChain.presentMode = swapChain.findSwapChainPresentMode(SwapChainDetails.presentModes);
	swapChain.extent = swapChain.chooseSwapExtent(SwapChainDetails.capabilities, window);

	uint32_t imageCount = SwapChainDetails.capabilities.minImageCount + 1;

	if (SwapChainDetails.capabilities.maxImageCount > 0 && imageCount > SwapChainDetails.capabilities.maxImageCount) {
		imageCount = SwapChainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainInfo{};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = surface;
	swapChainInfo.minImageCount = imageCount;
	swapChainInfo.imageFormat = swapChain.format.format;
	swapChainInfo.imageColorSpace = swapChain.format.colorSpace;
	swapChainInfo.presentMode = swapChain.presentMode;
	swapChainInfo.imageExtent = swapChain.extent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	uint32_t queueFamilyIndices[] = { QueueFamilyIndex.graphicsFamily, QueueFamilyIndex.presentFamily };
	if (QueueFamilyIndex.graphicsFamily != QueueFamilyIndex.presentFamily) {
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainInfo.preTransform = SwapChainDetails.capabilities.currentTransform;
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &swapChainInfo, nullptr, &swapChain.__swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Swapchain\n");
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapChain.__swapChain, &imageCount, nullptr);
	swapChain.images.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain.__swapChain, &imageCount, swapChain.images.data());

}

void VulkanClass::createSurface() {

	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Window Surface\n");
	}

}

void VulkanClass::createImageViews() {

	swapChain.imageViews.resize(swapChain.images.size());

	for (size_t i = 0; i < swapChain.imageViews.size(); i++) {
		VkImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.image = swapChain.images[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = swapChain.format.format;
		imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;

		if (vkCreateImageView(logicalDevice, &imageViewInfo, nullptr, &swapChain.imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Image View\n");
		}
	}

}

void VulkanClass::createRenderPass() {

	VkAttachmentDescription attachmentInfo{};
	attachmentInfo.format = swapChain.format.format;
	attachmentInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentInfo.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentInfo{};
	depthAttachmentInfo.format = findDepthFormat();
	depthAttachmentInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentInfo.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentInfo.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentInfo.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachmentRef{};
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentRef.attachment = 0;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachmentRef.attachment = 1;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependencies{};
	dependencies.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies.dstSubpass = 0;
	dependencies.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies.srcAccessMask = 0;
	dependencies.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments = { attachmentInfo, depthAttachmentInfo };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependencies;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Render Pass\n");
	}

}

void VulkanClass::createDescriptorSetLayout() {

	VkDescriptorSetLayoutBinding transformLayoutBinding{};
	transformLayoutBinding.binding = 0;
	transformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	transformLayoutBinding.descriptorCount = 1;
	transformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo transformLayoutInfo{};
	transformLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	transformLayoutInfo.bindingCount = 1;
	transformLayoutInfo.pBindings = &transformLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &transformLayoutInfo, nullptr, &transformDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Transform Descriptor Set layout\n");
	}
}

void VulkanClass::createPosDescriptorSetLayout() {

	VkDescriptorSetLayoutBinding posLayoutBinding{};
	posLayoutBinding.binding = 0;
	posLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	posLayoutBinding.descriptorCount = 1;
	posLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo posLayoutInfo{};
	posLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	posLayoutInfo.bindingCount = 1;
	posLayoutInfo.pBindings = &posLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &posLayoutInfo, nullptr, &posDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Pos Descriptor Set layout\n");
	}

	if (vkCreateDescriptorSetLayout(logicalDevice, &posLayoutInfo, nullptr, &midpointsDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Pos Descriptor Set layout\n");
	}

	if (vkCreateDescriptorSetLayout(logicalDevice, &posLayoutInfo, nullptr, &sizesDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Pos Descriptor Set layout\n");
	}

}

void VulkanClass::createAmpDescriptorSetLayout() {

	VkDescriptorSetLayoutBinding ampLayoutBinding{};
	ampLayoutBinding.binding = 0;
	ampLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ampLayoutBinding.descriptorCount = 1;
	ampLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo ampLayoutInfo{};
	ampLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ampLayoutInfo.bindingCount = 1;
	ampLayoutInfo.pBindings = &ampLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &ampLayoutInfo, nullptr, &AmpDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Amplitude Descriptor Set layout\n");
	}

}

void VulkanClass::createDescriptorPools() {

	VkDescriptorPoolSize poolSize;
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &uniformDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Uniform Descriptor Pool\n");
	}

	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize.descriptorCount = 4;

	poolInfo.maxSets = 4;

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &ampDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Amplitude Descriptor Pool\n");
	}

}

uint32_t VulkanClass::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");

}

void VulkanClass::createTransformBuffer(VkDeviceSize bufferSize) {

	transformBuffer.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	transformBufferMemory.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	transformBufferMap.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &transformBuffer[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed To create Transform Uniform Buffer\n");

		VkMemoryRequirements memreq;
		vkGetBufferMemoryRequirements(logicalDevice, transformBuffer[i], &memreq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memreq.size;
		allocInfo.memoryTypeIndex = findMemoryType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &transformBufferMemory[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to Allocate Transform Uniform Buffer Memory\n6");

		vkBindBufferMemory(logicalDevice, transformBuffer[i], transformBufferMemory[i], 0);

		vkMapMemory(logicalDevice, transformBufferMemory[i], 0, bufferSize, 0, &transformBufferMap[i]);
	}

}

void VulkanClass::createPosDescriptorSet() {

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = ampDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &posDescriptorSetLayout;

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &posDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Transform Descriptor Set\n");
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = posBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(Triangle) * triangles.size();

	int floatSize = sizeof(float);
	int size = sizeof(Triangle);

	VkWriteDescriptorSet ampWrite{};
	ampWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ampWrite.dstSet = posDescriptorSet;
	ampWrite.dstBinding = 0;
	ampWrite.dstArrayElement = 0;
	ampWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ampWrite.descriptorCount = 1;
	ampWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(logicalDevice, 1, &ampWrite, 0, nullptr);

	allocInfo.pSetLayouts = &midpointsDescriptorSetLayout;

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &midpointsDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Transform Descriptor Set\n");
	}

	bufferInfo.buffer = midpointsBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(float) * midpointsGPU.size();

	ampWrite.dstSet = midpointsDescriptorSet;
	ampWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(logicalDevice, 1, &ampWrite, 0, nullptr);

	allocInfo.pSetLayouts = &sizesDescriptorSetLayout;

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &sizesDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Transform Descriptor Set\n");
	}

	bufferInfo.buffer = sizesBuffer;
	bufferInfo.range = sizeof(unsigned int) * Sizes.size();

	ampWrite.dstSet = sizesDescriptorSet;
	ampWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(logicalDevice, 1, &ampWrite, 0, nullptr);

}

void VulkanClass::createAmpDescriptorSet() {

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = ampDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &AmpDescriptorSetLayout;

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &ampDescriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Transform Descriptor Set\n");
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = ampBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(AmpVolume) * ampVolumeSize;

	VkWriteDescriptorSet ampWrite{};
	ampWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	ampWrite.dstSet = ampDescriptorSet;
	ampWrite.dstBinding = 0;
	ampWrite.dstArrayElement = 0;
	ampWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	ampWrite.descriptorCount = 1;
	ampWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(logicalDevice, 1, &ampWrite, 0, nullptr);

}

void VulkanClass::createTransformDescriptorSet() {

	std::vector<VkDescriptorSetLayout> layouts(static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT), transformDescriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = uniformDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	transformDescriptorSet.resize(static_cast<uint32_t>(swapChain.MAX_FRAMES_IN_FLIGHT));

	if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, transformDescriptorSet.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Transform Descriptor Set\n");
	}

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = transformBuffer[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(transform);

		VkWriteDescriptorSet transformWrite{};
		transformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		transformWrite.dstSet = transformDescriptorSet[i];
		transformWrite.dstBinding = 0;
		transformWrite.dstArrayElement = 0;
		transformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		transformWrite.descriptorCount = 1;
		transformWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(logicalDevice, 1, &transformWrite, 0, nullptr);
	}

}

void VulkanClass::updateTransform() {

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++) {
		memcpy(transformBufferMap[i], &transform, sizeof(transform));
	}

}

void VulkanClass::createComputePipeline() {

	VkPipelineLayoutCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	std::vector<VkDescriptorSetLayout> setLayouts = { AmpDescriptorSetLayout, posDescriptorSetLayout, midpointsDescriptorSetLayout, sizesDescriptorSetLayout, transformDescriptorSetLayout };
	pipelineInfo.setLayoutCount = setLayouts.size();
	pipelineInfo.pSetLayouts = setLayouts.data();

	if (vkCreatePipelineLayout(logicalDevice, &pipelineInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Pipeline Layout\n");
	}

	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.layout = computePipelineLayout;
	computePipelineInfo.stage = basicShader->computeShaderStageInfo;

	VkResult computeCreate = vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline);

	if (computeCreate != VK_SUCCESS) {
		std::cout << "Failed to Create Compute Pipeline | ERROR - " << computeCreate << "\n";
		throw std::runtime_error("Failed to Create Compute Pipeline\n");
	}

	std::cout << "compute pipeline created\n";

}


void VulkanClass::createGraphicsPipeline() {

	if (graphicsPipeline != nullptr) {
		vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	}

	VkVertexInputBindingDescription vertexBindingInfo{};
	vertexBindingInfo.binding = 0;
	vertexBindingInfo.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindingInfo.stride = sizeof(Vertex);

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingInfo;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;// VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain.extent.width);
	viewport.height = static_cast<float>(swapChain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissorRect{};
	scissorRect.extent = swapChain.extent;
	scissorRect.offset = { 0,0 };

	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pScissors = &scissorRect;
	viewportStateInfo.pViewports = &viewport;

	VkPipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterInfo.lineWidth = 3.0f;
	rasterInfo.cullMode = VK_CULL_MODE_NONE;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;

	VkPipelineDepthStencilStateCreateInfo depthInfo{};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthInfo.depthBoundsTestEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampleInfo{};
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlend{};
	colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlend.blendEnable = VK_TRUE;
	colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo colorBlendGlobal{};
	colorBlendGlobal.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendGlobal.logicOpEnable = VK_FALSE;
	colorBlendGlobal.attachmentCount = 1;
	colorBlendGlobal.pAttachments = &colorBlend;

	VkPipelineTessellationStateCreateInfo tessellationInfo{};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.patchControlPoints = 3;

	std::vector<VkDescriptorSetLayout> layouts = { transformDescriptorSetLayout, AmpDescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutInfo.pSetLayouts = layouts.data();

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Pipeline Layout\n");
	}


	//CREATING GRAPHICS PIPELINE

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = basicShader->graphicsShaderStageInfos.size();
	graphicsPipelineInfo.pStages = basicShader->graphicsShaderStageInfos.data();
	//graphicsPipelineInfo.pTessellationState = VK_NULL_HANDLE; &tessellationInfo;
	graphicsPipelineInfo.pDynamicState = &dynamicState;
	graphicsPipelineInfo.pColorBlendState = &colorBlendGlobal;
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pDepthStencilState = &depthInfo;
	graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
	graphicsPipelineInfo.pRasterizationState = &rasterInfo;
	graphicsPipelineInfo.pViewportState = &viewportStateInfo;
	graphicsPipelineInfo.layout = pipelineLayout;
	graphicsPipelineInfo.renderPass = renderPass;
	graphicsPipelineInfo.subpass = 0;

	VkResult createGraphics = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &graphicsPipeline);

	if (createGraphics != VK_SUCCESS) {
		std::cout << "Faile to create Graphics Pipeline | ERROR - " << createGraphics << "\n";
		throw std::runtime_error("Failed To Create Graphics Pipeline\n");
	}

}

void VulkanClass::createFramebuffers() {

	swapChain.framebuffers.resize(swapChain.imageViews.size());

	for (size_t i = 0; i < swapChain.framebuffers.size(); i++) {
		std::vector<VkImageView> attachments = { swapChain.imageViews[i], depthImageView };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain.extent.width;
		framebufferInfo.height = swapChain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChain.framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Framebuffer\n");
		}

	}

}

void VulkanClass::recreateSwapChain() {

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(logicalDevice);

	vkDestroyImageView(logicalDevice, depthImageView, nullptr);
	vkDestroyImage(logicalDevice, depthImage, nullptr);
	vkFreeMemory(logicalDevice, depthImageMemory, nullptr);

	for (size_t i = 0; i < swapChain.framebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice, swapChain.framebuffers[i], nullptr);
	}
	for (size_t i = 0; i < swapChain.imageViews.size(); i++) {
		vkDestroyImageView(logicalDevice, swapChain.imageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(logicalDevice, swapChain.__swapChain, nullptr);

	if (!checkSwapChainSupport(physicalDevice)) {
		throw std::runtime_error("SwapChain not Supported\n");
	}

	createSwapChain();
	createImageViews();
	createDepthResources();
	createFramebuffers();

}

void VulkanClass::createCommandPool() {

	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = QueueFamilyIndex.graphicsFamily;

	if (vkCreateCommandPool(logicalDevice, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Create Command Pool\n");
	}

}

void VulkanClass::recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Begin Recording Compute Command Buffer\n");
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

	std::vector<VkDescriptorSet> descriptorSets = { ampDescriptorSet, posDescriptorSet, midpointsDescriptorSet, sizesDescriptorSet, transformDescriptorSet[0] };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, 0);

	vkCmdDispatch(commandBuffer, 523, 105, 522);// 372, 155, 228);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Record Compute Command Buffer\n");
	}

}

void VulkanClass::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index, uint32_t currentFrame) {

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Being Recording Command Buffer\n");
	}

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = swapChain.framebuffers[index];
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = swapChain.extent;

	std::vector<VkClearValue> clearColor = { {{0.2f, 0.3f, 0.3f, 1.0f}}, {1.0f, 0} };
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearColor.size());
	renderPassBeginInfo.pClearValues = clearColor.data();

	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.buffer = ampBuffer;
	barrier.size = sizeof(AmpVolume) * ampVolumeSize;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain.extent.width);
	viewport.height = static_cast<float>(swapChain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissorRect{};
	scissorRect.extent = swapChain.extent;
	scissorRect.offset = { 0,0 };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

	//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	//vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32

	std::vector<VkDescriptorSet> descriptorSets = { transformDescriptorSet[currentFrame] , ampDescriptorSet };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, 0);

	vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Record Command Buffer\n");
	}

}

void VulkanClass::createCommandBuffer() {

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	commandBuffer.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Allocate Command Buffer\n");
		}
	}

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed To Allocate Compute Command Buffer\n");
	}

}

void VulkanClass::createSyncObjects() {

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	imageAvailableSemaphore.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	renderFinishedSempahore.resize(swapChain.MAX_FRAMES_IN_FLIGHT);
	inFlightFence.resize(swapChain.MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < swapChain.MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSempahore[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed To Create Sync Objects\n");
		}
	}

	if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &computeFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(logicalDevice, &fenceInfo, nullptr, &computeInFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Compute Sync Objects\n");
	}

	vkCreateFence(logicalDevice, &fenceInfo, nullptr, &imGuiFence);

}

void VulkanClass::dispatch() {

	vkResetFences(logicalDevice, 1, &computeInFlightFence);

	vkResetCommandBuffer(computeCommandBuffer, 0);
	recordComputeCommandBuffer(computeCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore signalSemaphores[] = { computeFinishedSemaphore };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &computeCommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, computeInFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Submit Compute Command\n");
	}

}

void VulkanClass::draw(uint32_t& imageIndex) {

	uint32_t index;

	VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain.__swapChain, UINT32_MAX, imageAvailableSemaphore[imageIndex], VK_NULL_HANDLE, &index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		std::cout << "NO WORK SUBMITTED\n";
		return;
	}

	vkResetFences(logicalDevice, 1, &inFlightFence[imageIndex]);

	vkResetCommandBuffer(commandBuffer[imageIndex], 0);

	recordCommandBuffer(commandBuffer[imageIndex], index, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[imageIndex] };
	VkSemaphore signalSemaphores[] = { renderFinishedSempahore[imageIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence[imageIndex]) != VK_SUCCESS) {
		VkResult error = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence[imageIndex]);
		throw std::runtime_error("Failed To Submit Draw Command\n");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.__swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &index;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}

}

void VulkanClass::initImGui() {

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(logicalDevice, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create ImGui Descriptor Pool\n");
	}

	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, false);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = logicalDevice;
	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = imguiDescriptorPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	VkCommandBuffer commandBuffer;
	VkCommandBufferAllocateInfo imguiCmdBufferInfo{};
	imguiCmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	imguiCmdBufferInfo.commandPool = commandPool;
	imguiCmdBufferInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(logicalDevice, &imguiCmdBufferInfo, &commandBuffer);

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	vkResetFences(logicalDevice, 1, &imGuiFence);
	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);

	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo cmdinfo{};
	cmdinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	cmdinfo.commandBufferCount = 1;
	cmdinfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &cmdinfo, imGuiFence);

	vkWaitForFences(logicalDevice, 1, &imGuiFence, true, UINT64_MAX);

	// clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

}

void VulkanClass::drawGui() {

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();

}

void VulkanClass::loadModel() {

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	float maxX = 0.0, maxY = 0.0, maxZ = 0.0;
	float minX = 0.0, minY = 0.0, minZ = 0.0;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
		std::cout << warn + err << "\n";
		throw std::runtime_error(warn + err);
	}

	float max = 0;
	float count = 0;

	for (const auto& shape : shapes) {
		int i = 0;
		Triangle triangle{};
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				-attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
				0.0
			};

			if (vertex.pos.x > maxX)
				maxX = vertex.pos.x;
			if (vertex.pos.y > maxY)
				maxY = vertex.pos.y;
			if (vertex.pos.z > maxZ)
				maxZ = vertex.pos.z;
			if (vertex.pos.x < minX)
				minX = vertex.pos.x;
			if (vertex.pos.y < minY)
				minY = vertex.pos.y;
			if (vertex.pos.z < minZ)
				minZ = vertex.pos.z;

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2],
				0.0
			};

			vertices.push_back(vertex);
			indices.push_back(indices.size());

			triangle.vertices[i] = vertex;

			if (i == 2) {
				triangles.push_back(triangle);
				i = 0;
				count++;
			}
			else {
				i++;
			}
		}
	}

	posBufferSize = vertices.size();

	extents.xMax = maxX;
	extents.xMin = minX;
	extents.yMax = maxY;
	extents.yMin = minY;
	extents.zMax = maxZ;
	extents.zMin = minZ;

	int x = (maxX - minX) / 10.0;
	int y = (maxY - minY) / 10.0;
	int z = (maxZ - minZ) / 10.0;

	ampVolumeSize = (x * y * z);

	ampVolume = (AmpVolume*)malloc(ampVolumeSize * sizeof(AmpVolume));

	std::cout << "MINIMUMS - " << extents.xMin << " | " << extents.yMin << " | " << extents.zMin << "\n";

	std::cout << "AMPLITUDE VOLUME SIZE - " << (maxX - minX) / 10.0 << " X " << (maxY - minY) / 10.0 << " X " << (maxZ - minZ) / 10.0 << " = " << ampVolumeSize << "\n";

	srand(glfwGetTime());

	float densities[101];

	for (unsigned int i = 0; i < 101; i++) {
		densities[i] = -1.0f;
	}

	/*for (unsigned int i = 0; i < ampVolumeSize; i++) {
		ampVolume[i].amp = 0.0;
	}*/

	int index = 0;

	for (unsigned int i = 0; i < x; i++) {
		for (unsigned int j = 0; j < y; j++) {
			for (unsigned int k = 0; k < z; k++) {
				int yStride = int(maxX - minX) / 10;
				int zStride = (int(maxY - minY) / 10) * (int(maxX - minX) / 10);
				index = i + j * yStride + k * zStride;
				int factor = ampVolumeSize / 100;
				if (index >= 0 && index < ampVolumeSize) {
					ampVolume[index].amp = densities[(int)(index / factor)];
				}
			}
		}
	}
}

void VulkanClass::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);

}

void VulkanClass::createVertexBuffer() {

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.size = sizeof(Vertex) * vertices.size();
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Vertex Buffer\n");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, vertexBuffer, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &vertexBufferMemory);

	vkBindBufferMemory(logicalDevice, vertexBuffer, vertexBufferMemory, 0);

	vkMapMemory(logicalDevice, vertexBufferMemory, 0, bufferInfo.size, 0, &vertexBufferMap);
	memcpy(vertexBufferMap, vertices.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(logicalDevice, vertexBufferMemory);

}

void VulkanClass::createOctree() {

	std::vector<std::vector<Triangle>> octree;
	for (int i = 0; i < (8 * 8 * 8); i++) {
		octree.push_back(std::vector<Triangle>(0));
	}

	midpoints.push_back(std::vector<float>({ extents.xMin, extents.xMax }));
	midpoints.push_back(std::vector<float>({ extents.yMin, extents.yMax }));
	midpoints.push_back(std::vector<float>({ extents.zMin, extents.zMax }));

	for (int i = 0; i < 3; i++) {
		int head = 0;
		midpoints[i].insert(midpoints[i].begin() + 1, (midpoints[i][head] + midpoints[i][midpoints[i].size() - 1]) / 2.0);
		midpoints[i].insert(midpoints[i].begin() + 1, (midpoints[i][head] + midpoints[i][head + 1]) / 2.0);
		midpoints[i].insert(midpoints[i].end() - 1, (midpoints[i][midpoints[i].size() - 2] + midpoints[i][midpoints[i].size() - 1]) / 2.0);
		midpoints[i].insert(midpoints[i].begin() + 1, (midpoints[i][head] + midpoints[i][head + 1]) / 2.0);
		midpoints[i].insert(midpoints[i].begin() + 3, (midpoints[i][head + 2] + midpoints[i][head + 3]) / 2.0);
		midpoints[i].insert(midpoints[i].end() - 1, (midpoints[i][midpoints[i].size() - 2] + midpoints[i][midpoints[i].size() - 1]) / 2.0);
		midpoints[i].insert(midpoints[i].end() - 3, (midpoints[i][midpoints[i].size() - 4] + midpoints[i][midpoints[i].size() - 3]) / 2.0);

		for (int j = 0; j < midpoints[i].size(); j++) {
			midpointsGPU.push_back(midpoints[i][j]);
		}
	}

	for (unsigned int i = 0; i < triangles.size(); i++) {

		for (unsigned int l = 0; l < 3; l++) {
			unsigned int Index[3] = {0, 0, 0};
			for (unsigned int j = 0; j < 3; j++) {
				float coord = triangles[i].vertices[l].pos[j];
				for (unsigned int k = 0; k < midpoints[j].size() - 1; k++) {
					if (coord > midpoints[j][k] && coord < midpoints[j][k + 1]) {
						Index[j] = k;
					}
				}
			}

			unsigned int flattenedIndex = Index[0] + 8 * Index[1] + 64 * Index[2];
			if (std::find(octree[flattenedIndex].begin(), octree[flattenedIndex].end(), triangles[i]) == octree[flattenedIndex].end())
				octree[flattenedIndex].push_back(triangles[i]);
		}

	}

	Octree = octree[0];
	Sizes.push_back(octree[0].size());

	for (int i = 1; i < (8 * 8 * 8); i++) {
		Octree.insert(Octree.end(), octree[i].begin(), octree[i].end());
		Sizes.push_back(octree[i].size());
	}

	//std::cout << "OCTREE VIEW\n";

	//for (int i = 0; i < 8; i++) {
	//	for (int j = 0; j < 8; j++) {
	//		for (int k = 0; k < 8; k++) {
	//			std::cout << Sizes[k + j * 8 + i * 64] << " ";
	//		}
	//		std::cout << "\n";
	//	}
	//	std::cout << "\n\n\n ////////////////// \n\n\n";
	//}

	std::cout << "OCTREE COMPLETE\n";

}

void VulkanClass::createAuxilaryOctreeBuffers() {

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.size = sizeof(float) * midpointsGPU.size();
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &midpointsBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Vertex Buffer\n");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, midpointsBuffer, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &midpointsBufferMemory);

	vkBindBufferMemory(logicalDevice, midpointsBuffer, midpointsBufferMemory, 0);

	vkMapMemory(logicalDevice, midpointsBufferMemory, 0, bufferInfo.size, 0, &midpointsBufferMap);
	memcpy(midpointsBufferMap, midpointsGPU.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(logicalDevice, midpointsBufferMemory);

	bufferInfo.size = sizeof(unsigned int) * Sizes.size();

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &sizesBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Vertex Buffer\n");
	}

	vkGetBufferMemoryRequirements(logicalDevice, sizesBuffer, &memRequirements);

	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &sizesBufferMemory);

	vkBindBufferMemory(logicalDevice, sizesBuffer, sizesBufferMemory, 0);

	vkMapMemory(logicalDevice, sizesBufferMemory, 0, bufferInfo.size, 0, &sizesBufferMap);
	memcpy(sizesBufferMap, Sizes.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(logicalDevice, sizesBufferMemory);

}

void VulkanClass::createTriangleBuffer() {

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.size = sizeof(Triangle) * triangles.size();
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &posBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to Create Vertex Buffer\n");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, posBuffer, &memRequirements);

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &posBufferMemory);

	vkBindBufferMemory(logicalDevice, posBuffer, posBufferMemory, 0);

	vkMapMemory(logicalDevice, posBufferMemory, 0, bufferInfo.size, 0, &posBufferMap);
	memcpy(posBufferMap, Octree.data(), (size_t)bufferInfo.size);
	vkUnmapMemory(logicalDevice, posBufferMemory);

}

void VulkanClass::createIndexBuffer() {

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.size = bufferSize;

	vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &stagingBuffer);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &stagingBufferMemory);

	vkBindBufferMemory(logicalDevice, stagingBuffer, stagingBufferMemory, 0);

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferInfo.size = bufferSize;

	vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &indexBuffer);

	vkGetBufferMemoryRequirements(logicalDevice, indexBuffer, &memReq);

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &indexBufferMemory);

	vkBindBufferMemory(logicalDevice, indexBuffer, indexBufferMemory, 0);


	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

}

void VulkanClass::createAmpBuffer() {

	VkDeviceSize bufferSize = sizeof(AmpVolume) * ampVolumeSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.size = bufferSize;

	vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &stagingBuffer);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &stagingBufferMemory);

	vkBindBufferMemory(logicalDevice, stagingBuffer, stagingBufferMemory, 0);

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, ampVolume, bufferSize);

	//for (unsigned int i = 0; i < ampVolumeSize; i++) {
	//	//std::cout << static_cast<float>(*((float*)data + sizeof(float)*i)) << "\n";
	//}

	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferInfo.size = bufferSize;

	vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &ampBuffer);

	vkGetBufferMemoryRequirements(logicalDevice, ampBuffer, &memReq);

	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &ampBufferMemory);

	vkBindBufferMemory(logicalDevice, ampBuffer, ampBufferMemory, 0);

	copyBuffer(stagingBuffer, ampBuffer, bufferSize);

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);

}

void VulkanClass::validateAmpBuffer() {

	VkDeviceSize bufferSize = sizeof(AmpVolume) * ampVolumeSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferInfo.size = bufferSize;

	vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &stagingBuffer);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &stagingBufferMemory);

	vkBindBufferMemory(logicalDevice, stagingBuffer, stagingBufferMemory, 0);

	copyBuffer(ampBuffer, stagingBuffer, bufferSize);

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	//memcpy(data, ampVolume, bufferSize);

	AmpVolume* ampBufferValidation = reinterpret_cast<AmpVolume*>(data);

	//std::cout << "FIRST ELEMENT - " << ampBufferValidation[0].amp << "\n";

	std::cout << midpointsGPU[0] << "\n";

	std::vector<AmpVolume> ampBufferVector(ampBufferValidation, ampBufferValidation + ampVolumeSize);

	float max = 0;
	float min = 1;
	std::vector<float> count;

	for (unsigned int i = 0; i < ampVolumeSize; i++) {
		if (ampBufferVector[i].amp > max) {
			max = ampBufferVector[i].amp;
		}
		if (ampBufferVector[i].amp < min) {
			min = ampBufferVector[i].amp;
		}
	}

	std::cout << triangles[0].vertices[2].pos.z;
	std::cout << triangles[2].vertices[0].pos.z;

	std::cout << max << "\n";
	std::cout << min << "\n";

	for (unsigned int i = 0; i < ampVolumeSize; i++) {
		if (ampBufferVector[i].amp < 0 || ampBufferVector[i].amp > 1) {
			count.push_back(ampBufferVector[i].amp);
		}
	}

	std::cout << "count of irregular values - " << count.size() << "\n";

	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void VulkanClass::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(logicalDevice, image, imageMemory, 0);
}

VkImageView VulkanClass::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkFormat VulkanClass::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}

	}

	throw std::runtime_error("failed to find supported format!");

}

bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanClass::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void VulkanClass::createDepthResources() {

	VkFormat depthFormat = findDepthFormat();

	createImage(swapChain.extent.width, swapChain.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

}