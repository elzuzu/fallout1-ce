#pragma once
#include <vulkan/vulkan.h>
#include <fstream>
#include <string>

class VulkanErrorReporter {
public:
    static std::string translateVulkanError(VkResult result);
    static void generateDiagnosticReport(const std::string& error);

private:
    static std::string getOSVersion();
    static std::string getCPUInfo();
    static bool isVulkanRuntimeInstalled();
    static std::string getVulkanVersion();
    static void listAvailableDevices(std::ofstream& report);
    static void logInfo(const std::string& msg);
};
