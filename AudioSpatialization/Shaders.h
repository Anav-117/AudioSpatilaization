#pragma once
#include <fstream>
#include <vector>
#include <vulkan/vulkan.h>

class Shader {

public:
	std::vector<char> vertexShaderSource;
	std::vector<char> fragmentShaderSource;
	std::vector<char> computeShaderSource;
	std::vector<VkPipelineShaderStageCreateInfo> graphicsShaderStageInfos;
	VkPipelineShaderStageCreateInfo computeShaderStageInfo;

	VkShaderModule vertexShader;
	VkShaderModule fragmentShader;
	VkShaderModule computeShader;

	VkDevice device;

	Shader(const std::string ShaderName, VkDevice device);
	Shader();
	~Shader();
	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(std::vector<char> code, VkDevice device, std::string ShaderName);

};