/**
 * @file renderers/vulkan/vk_renderer.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-28
 * @brief Engine module implementation
 */
#include "renderers/vulkan/vk_renderer.hpp"

#include <cstring> // TODO: For memcpy remove when buffers for instancing sorted
#include <thread>

#include "debug/debug.hpp"
#include "math/math.hpp"
#include "renderers/vulkan/gouda_vk_wrapper.hpp"
#include "renderers/vulkan/vk_utils.hpp"

namespace gouda::vk {

VulkanRenderer::VulkanRenderer()
    : p_instance{nullptr},
      p_device{nullptr},
      p_buffer_manager{nullptr},
      p_swapchain{nullptr},
      p_depth_resources{nullptr},
      p_command_buffer_manager{nullptr},
      p_pipeline{nullptr},
      p_static_vertex_buffer{nullptr},
      p_static_index_buffer{nullptr},

      p_window{nullptr},
      p_render_pass{VK_NULL_HANDLE},
      p_vertex_shader{VK_NULL_HANDLE},
      p_fragment_shader{VK_NULL_HANDLE},
      p_copy_command_buffer{VK_NULL_HANDLE},
      p_imgui_pool{VK_NULL_HANDLE},

      m_queue{},
      m_frame_fences{},
      m_framebuffers{},
      m_command_buffers{},

      m_uniform_buffers{},
      m_instance_buffers{},
      m_mapped_instance_data{},

      m_framebuffer_size{0, 0},
      m_current_frame{0},

      m_clear_colour{},
      m_max_instances{1000},
      m_vsync_mode{VSyncMode::DISABLED},
      m_vertex_count{0},
      m_index_count{0},
      m_use_indexed_drawing{false},
      m_is_initialized{false}
{
}

VulkanRenderer::~VulkanRenderer()
{
    ENGINE_LOG_DEBUG("Cleaning up Vulkan renderer.");

    if (m_is_initialized) {

        for (auto &buffer : m_uniform_buffers) {
            buffer.Destroy(p_device->GetDevice());
        }
        ENGINE_LOG_DEBUG("Uniform buffers destroyed.");

        for (auto &buffer : m_instance_buffers) {
            buffer.Destroy(p_device->GetDevice());
        }
        ENGINE_LOG_DEBUG("Instance buffers destroyed.");

        if (p_static_vertex_buffer != nullptr) {
            p_static_vertex_buffer->Destroy(p_device->GetDevice());
            ENGINE_LOG_DEBUG("Static vertex buffer destroyed.");
        }

        if (p_static_index_buffer != nullptr) {
            p_static_index_buffer->Destroy(p_device->GetDevice());
            ENGINE_LOG_DEBUG("Static index buffer destroyed.");
        }

        for (auto &fence : m_frame_fences) {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(p_device->GetDevice(), fence, nullptr);
                fence = VK_NULL_HANDLE;
            }
        }
        ENGINE_LOG_DEBUG("Frame fences destroyed.");

        if (p_render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(p_device->GetDevice(), p_render_pass, nullptr);
            p_render_pass = VK_NULL_HANDLE;
            ENGINE_LOG_DEBUG("Render pass destroyed.");
        }

        DestroyFramebuffers();

        DestroyShaderModule(p_vertex_shader);
        DestroyShaderModule(p_fragment_shader);

        // Clean up IMGUI if enabled
        DestroyImGUI();
    }
}

void VulkanRenderer::Initialize(GLFWwindow *window_ptr, std::string_view app_name, SemVer vulkan_api_version,
                                VSyncMode vsync_mode)
{
    ASSERT(window_ptr, "Window pointer cannot be null.");
    ASSERT(!app_name.empty(), "Application name cannot be empty or null.");

    ENGINE_PROFILE_SESSION("Gouda", "debug/profiling/results.json");

    ENGINE_PROFILE_SCOPE("Init renderer");

    p_window = window_ptr;
    m_vsync_mode = vsync_mode;

    // Firstly query and cache the current frame buffer size for further initialization
    CacheFrameBufferSize();

    p_instance = std::make_unique<VulkanInstance>(app_name, vulkan_api_version, p_window);
    ENGINE_LOG_DEBUG("VulkanInstance initialized.");

    p_device = std::make_unique<VulkanDevice>(*p_instance, VK_QUEUE_GRAPHICS_BIT);
    ENGINE_LOG_DEBUG("VulkanDevice initialized.");

    p_buffer_manager = std::make_unique<BufferManager>(p_device.get(), &m_queue);
    ENGINE_LOG_DEBUG("Buffer manager initialized.");

    // Swapchain
    p_swapchain = std::make_unique<VulkanSwapchain>(p_window, p_device.get(), p_instance.get(), p_buffer_manager.get(),
                                                    vsync_mode);
    ENGINE_LOG_DEBUG("Swapchain initialized.");

    // Queue
    m_queue.Initialize(p_device->GetDevice(), p_swapchain->GetHandle(), p_device->GetQueueFamily(), 0);

    std::vector<Vertex> quad_vertices = {
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // Top-left
        {{0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // Top-right
        {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}, // Bottom-right
        {{0.0f, 0.5f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}  // Bottom-left
    };
    VkDeviceSize vertex_buffer_size{sizeof(Vertex) * quad_vertices.size()};
    Buffer static_buffer{p_buffer_manager->CreateVertexBuffer(quad_vertices.data(), vertex_buffer_size)};
    p_static_vertex_buffer = std::make_unique<Buffer>(std::move(static_buffer));

    std::vector<uint32_t> quad_indices = {
        0, 1, 2, // First triangle
        0, 2, 3  // Second triangle
    };

    VkDeviceSize index_buffer_size{sizeof(u32) * quad_indices.size()};

    Buffer index_buffer{p_buffer_manager->CreateIndexBuffer(quad_indices.data(), index_buffer_size)};

    p_static_index_buffer = std::make_unique<Buffer>(std::move(index_buffer));

    // Set flags and counts for indexed drawing
    m_use_indexed_drawing = true;
    m_index_count = static_cast<u32>(quad_indices.size()); // 6 indices

    // Instance rendering
    CreateInstanceBuffers();

    // Command buffers
    p_command_buffer_manager =
        std::make_unique<CommandBufferManager>(p_device.get(), &m_queue, p_device->GetQueueFamily());
    p_command_buffer_manager->AllocateBuffers(1, &p_copy_command_buffer);

    // Depth resources
    p_depth_resources = std::make_unique<VulkanDepthResources>(p_device.get(), p_instance.get(), p_buffer_manager.get(),
                                                               p_swapchain.get());

    p_render_pass = CreateRenderPass();

    CreateFramebuffers();
    CreateCommandBuffers();
    CreateFences();

    SetClearColour({0.0f, 0.0f, 0.0f, 0.0f}); // Set a default black clear value

    // initialize IMGUI if enabled (currently debug build only!)
    InitializeImGUI();

    m_is_initialized = true;
    ENGINE_LOG_DEBUG("VulkanRenderer initialized.");
}

void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer command_buffer, u32 image_index, u32 instance_count,
                                         ImDrawData *draw_data)
{
    ENGINE_PROFILE_SCOPE("Record command buffer");

    gouda::vk::BeginCommandBuffer(command_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = m_clear_colour;
    clear_values[1].depthStencil = {1.0f, 0};

    VkExtent2D extent{p_swapchain->GetExtent()};
    VkRenderPassBeginInfo render_pass_begin_info{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                                 .renderPass = p_render_pass,
                                                 .framebuffer = m_framebuffers[image_index],
                                                 .renderArea = {{0, 0}, extent},
                                                 .clearValueCount = static_cast<u32>(clear_values.size()),
                                                 .pClearValues = clear_values.data()};

    VkViewport viewport{.x = 0.0f,
                        .y = 0.0f,
                        .width = static_cast<f32>(extent.width),
                        .height = static_cast<f32>(extent.height),
                        .minDepth = 0.0f,
                        .maxDepth = 1.0f};

    VkRect2D scissor{{0, 0}, extent};

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    p_pipeline->Bind(command_buffer, image_index);

    VkBuffer buffers[]{p_static_vertex_buffer.get()->p_buffer, m_instance_buffers[image_index].p_buffer};
    VkDeviceSize offsets[]{0, 0};
    vkCmdBindVertexBuffers(command_buffer, 0, 2, buffers, offsets);

    if (m_use_indexed_drawing) {
        vkCmdBindIndexBuffer(command_buffer, p_static_index_buffer->p_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffer, m_index_count, instance_count, 0, 0, 0);
    }
    else {
        vkCmdDraw(command_buffer, 6, instance_count, 0, 0); // Fallback for non-indexed drawing
    }

    // Render ImGui if draw data is available
    if (draw_data != nullptr) {
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
    }

    vkCmdEndRenderPass(command_buffer);
    gouda::vk::EndCommandBuffer(command_buffer);
}

void VulkanRenderer::Render(f32 delta_time, const UniformData &uniform_data, const std::vector<InstanceData> &instances)
{
    while (!p_swapchain->IsValid()) {
        std::this_thread::yield();
        ENGINE_LOG_INFO("Rendering paused, waiting for valid swapchain");
    }

    vkWaitForFences(p_device->GetDevice(), 1, &m_frame_fences[m_current_frame], VK_TRUE, Constants::u64_max);
    vkResetFences(p_device->GetDevice(), 1, &m_frame_fences[m_current_frame]);

    u32 image_index{m_queue.AcquireNextImage(m_current_frame)};
    if (image_index == Constants::u32_max) {
        // TODO: Rebuild swapchain such here
    }

    m_uniform_buffers[image_index].Update(p_device->GetDevice(), &uniform_data, sizeof(uniform_data));
    VkDeviceSize instance_data_size{sizeof(InstanceData) * instances.size()};
    ASSERT(instance_data_size <= sizeof(InstanceData) * m_max_instances, "Instance count exceeds maximum buffer size.");
    if (instance_data_size > 0) {
        memcpy(m_mapped_instance_data[image_index], instances.data(), instance_data_size);
    }

    RenderStatistics stats{.instances = instances.size()};
    ImDrawData *draw_data{RenderImGUI(stats)}; // Returns a nullptr if ImGUI is disabled

    vkResetCommandBuffer(m_command_buffers[image_index], 0);
    RecordCommandBuffer(m_command_buffers[image_index], image_index, static_cast<u32>(instances.size()), draw_data);

    m_queue.Submit(m_command_buffers[image_index], m_current_frame, m_frame_fences[m_current_frame]);
    m_queue.Present(image_index, m_current_frame);

    m_current_frame = (m_current_frame + 1) % m_queue.GetMaxFramesInFlight();
}

void VulkanRenderer::SetupPipeline(const Mesh *mesh, std::string_view vertex_shader_path,
                                   std::string_view fragment_shader_filepath)
{
    p_vertex_shader = gouda::vk::CreateShaderModuleFromBinary(p_device->GetDevice(), vertex_shader_path);

    p_fragment_shader = gouda::vk::CreateShaderModuleFromBinary(p_device->GetDevice(), fragment_shader_filepath);

    p_pipeline =
        std::make_unique<GraphicsPipeline>(*this, p_window, p_render_pass, p_vertex_shader, p_fragment_shader, mesh,
                                           p_swapchain->GetImageCount(), m_uniform_buffers, sizeof(UniformData));
}

void VulkanRenderer::CreateFramebuffers()
{
    m_framebuffers.clear();
    m_framebuffers.resize(p_swapchain->GetImages().size());

    FrameBufferSize swapchain_size = p_swapchain->GetFramebufferSize();
    ENGINE_LOG_INFO("Framebuffer size from swapchain: {}x{}", swapchain_size.width, swapchain_size.height);

    ASSERT(m_framebuffer_size.area() > 0, "Framebuffer dimensions cannot be zero.");

    // Loop through each image in the swapchain to create a framebuffer
    for (uint i = 0; i < p_swapchain->GetImages().size(); i++) {
        std::vector<VkImageView> attachments;
        attachments.emplace_back(p_swapchain->GetImageViews()[i]);
        attachments.emplace_back(p_depth_resources->GetDepthImages()[i].p_view);

        // Define framebuffer creation settings
        VkFramebufferCreateInfo frame_buffer_create_info{};
        frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.renderPass = p_render_pass; // Attach to the given render pass
        frame_buffer_create_info.attachmentCount = static_cast<u32>(attachments.size());
        frame_buffer_create_info.pAttachments = attachments.data();
        frame_buffer_create_info.width = static_cast<u32>(swapchain_size.width);
        frame_buffer_create_info.height = static_cast<u32>(swapchain_size.height);
        frame_buffer_create_info.layers = 1;

        // Create the framebuffer and check for errors
        VkResult result{
            vkCreateFramebuffer(p_device->GetDevice(), &frame_buffer_create_info, nullptr, &m_framebuffers[i])};
        CHECK_VK_RESULT(result, "vkCreateFramebuffer\n");
    }

    ENGINE_LOG_DEBUG("Framebuffers created.");
}

void VulkanRenderer::DestroyFramebuffers()
{
    for (auto buffer : m_framebuffers) {
        vkDestroyFramebuffer(p_device->GetDevice(), buffer, nullptr);
    }

    m_framebuffers.clear();

    ENGINE_LOG_DEBUG("Framebuffers destroyed.");
}

void VulkanRenderer::CreateCommandBuffers()
{
    m_command_buffers.clear();
    m_command_buffers.resize(p_swapchain->GetImageCount());

    p_command_buffer_manager->AllocateBuffers(static_cast<u32>(m_command_buffers.size()), m_command_buffers.data());
}

VkShaderModule VulkanRenderer::CreateShaderModule(std::string_view filepath)
{
    VkShaderModule module{gouda::vk::CreateShaderModuleFromBinary(p_device->GetDevice(), filepath)};
    ENGINE_LOG_DEBUG("Shader module created from: {}", filepath);
    return module;
}

void VulkanRenderer::DestroyShaderModule(VkShaderModule module)
{
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(p_device->GetDevice(), module, nullptr);
        ENGINE_LOG_DEBUG("Shader module destroyed");
    }
}

void VulkanRenderer::CreateUniformBuffers(size_t data_size)
{
    m_uniform_buffers.clear();
    m_uniform_buffers.reserve(p_swapchain->GetImages().size());

    for (size_t i = 0; i < p_swapchain->GetImages().size(); ++i) {
        m_uniform_buffers.emplace_back(p_buffer_manager->CreateUniformBuffer(data_size));
    }
}

void VulkanRenderer::SetClearColour(const Colour<f32> &colour)
{
    m_clear_colour = {{colour.value[0], colour.value[1], colour.value[2], colour.value[3]}};
}

// Private ----------------------------------------------------------------------------------------
VkRenderPass VulkanRenderer::CreateRenderPass()
{
    ENGINE_PROFILE_SCOPE("Create render passes");

    VkAttachmentDescription colour_attachment_description{};
    colour_attachment_description.flags = 0;
    colour_attachment_description.format = p_swapchain->GetSurfaceFormat().format;
    colour_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_attachment_reference{.attachment = 0,
                                                      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkFormat depth_format{p_device->GetSelectedPhysicalDevice().m_depth_format};

    VkAttachmentDescription depth_attachment_description{};
    depth_attachment_description.format = depth_format;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{.attachment = 1,
                                                     .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    std::vector<VkSubpassDependency> subpass_dependencies{
        {.srcSubpass = VK_SUBPASS_EXTERNAL,
         .dstSubpass = 0,
         .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
         .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
         .srcAccessMask = 0,
         .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
         .dependencyFlags = 0}};

    VkSubpassDescription subpass_description{};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &colour_attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    std::vector<VkAttachmentDescription> attachment_descriptions;
    attachment_descriptions.push_back(colour_attachment_description);
    attachment_descriptions.push_back(depth_attachment_description);

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = static_cast<u32>(attachment_descriptions.size());
    render_pass_create_info.pAttachments = attachment_descriptions.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = static_cast<u32>(subpass_dependencies.size());
    render_pass_create_info.pDependencies = subpass_dependencies.data();

    VkRenderPass render_pass{VK_NULL_HANDLE};
    VkResult result{vkCreateRenderPass(p_device->GetDevice(), &render_pass_create_info, nullptr, &render_pass)};
    CHECK_VK_RESULT(result, "vkCreateRenderPass\n");

    ENGINE_LOG_DEBUG("Render pass created.");

    return render_pass;
}

void VulkanRenderer::CreateFences()
{
    const auto max_frames = m_queue.GetMaxFramesInFlight();
    m_frame_fences.resize(max_frames, VK_NULL_HANDLE);
    VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    for (u32 i = 0; i < max_frames; i++) {
        gouda::vk::CreateFence(p_device->GetDevice(), &fence_info, nullptr, &m_frame_fences[i]);
    }

    ENGINE_LOG_DEBUG("Frame fences created.");
}

void VulkanRenderer::CacheFrameBufferSize()
{
    glfwGetWindowSize(p_window, &m_framebuffer_size.width, &m_framebuffer_size.height);
}

void VulkanRenderer::ReCreateSwapchain()
{
    DestroyFramebuffers();

    p_swapchain->Recreate();
    m_queue.SetSwapchain(p_swapchain->GetHandle());
    CacheFrameBufferSize();
    p_depth_resources->Recreate();

    CreateFramebuffers();
}

void VulkanRenderer::CreateInstanceBuffers()
{
    VkDeviceSize max_instance_size{sizeof(InstanceData) * m_max_instances};
    m_instance_buffers.resize(p_swapchain->GetImageCount());

    for (auto &buffer : m_instance_buffers) {
        buffer = p_buffer_manager->CreateDynamicVertexBuffer(max_instance_size);
    }

    m_mapped_instance_data.resize(p_swapchain->GetImageCount());
    for (size_t i = 0; i < p_swapchain->GetImageCount(); ++i) {
        m_mapped_instance_data[i] = m_instance_buffers[i].MapPersistent(p_device->GetDevice());
    }
}

void VulkanRenderer::InitializeImGUI()
{

#ifdef USE_IMGUI

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io{ImGui::GetIO()};
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation
    io.IniFilename = "config/imgui_settings.ini";
    io.LogFilename = "debug/logs/imgui_log.txt";

    ImGui::StyleColorsDark();                                         // Set ImGui style
    ImGuiStyle &style{ImGui::GetStyle()};                             // Access ImGuiStyle for further customization
    style.WindowRounding = 5.0f;                                      // Round corners
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // Set custom window background color

    // const std::string font_filepath{"path/to/your/font.ttf"};
    //  ImFont *font{io.Fonts->AddFontFromFileTTF(font_filepath, 16.0f)};
    //  if (font == nullptr) {
    //   ENGINE_LOG_ERROR("Failed to load font '{}' for ImGUI", font_filepath);
    // }

    ImGui_ImplGlfw_InitForVulkan(p_window, false); // Initialize ImGui for Vulkan

    VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = static_cast<u32>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(p_device->GetDevice(), &pool_info, nullptr, &p_imgui_pool) != VK_SUCCESS) {
        ENGINE_THROW("Failed to create ImGui descriptor pool!");
    }

    VkQueue graphicsQueue{VK_NULL_HANDLE};
    vkGetDeviceQueue(GetDevice(), p_device->GetQueueFamily(), 0, &graphicsQueue);

    u32 min_image_count{p_device->GetSelectedPhysicalDevice().m_surface_capabilities.minImageCount};

    // Initialize ImGui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = p_instance->GetInstance();
    init_info.PhysicalDevice = p_device->GetPhysicalDevice();
    init_info.Device = p_device->GetDevice();
    init_info.QueueFamily = p_device->GetQueueFamily();
    init_info.Queue = graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = p_imgui_pool;
    init_info.RenderPass = p_render_pass;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = min_image_count;
    init_info.ImageCount = p_swapchain->GetImageCount();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    ENGINE_LOG_DEBUG("ImGUI initialized.");
#endif
}

void VulkanRenderer::DestroyImGUI()
{
#ifdef USE_IMGUI
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(p_device->GetDevice(), p_imgui_pool, nullptr);

    ENGINE_LOG_DEBUG("ImGUI destroyed.");
#endif
}

ImDrawData *VulkanRenderer::RenderImGUI(const RenderStatistics &stats)
{
#ifdef USE_IMGUI
    // Start new ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static int location = 0;
    static bool open{true};
    ImGuiIO &io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                    ImGuiWindowFlags_NoNav;
    if (location >= 0) {
        const float PAD = 10.0f;
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        // ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    else if (location == -2) {
        // Center window
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Stats for nerds", &open, window_flags)) {
        ImGui::Text("Stats for nerds.");
        ImGui::Separator();
        ImGui::Text("Renderer:");
        ImGui::Text("Instances: %d/%d", stats.instances, m_max_instances);
        ImGui::Separator();
        if (ImGui::IsMousePosValid())
            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text("Mouse Position: <invalid>");
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Custom", NULL, location == -1))
                location = -1;
            if (ImGui::MenuItem("Center", NULL, location == -2))
                location = -2;
            if (ImGui::MenuItem("Top-left", NULL, location == 0))
                location = 0;
            if (ImGui::MenuItem("Top-right", NULL, location == 1))
                location = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, location == 2))
                location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3))
                location = 3;
            if (open && ImGui::MenuItem("Close"))
                open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    // Render the ImGui frame (creates draw data)
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    return draw_data;

#else
    return nullptr;
#endif
}

} // namespace gouda::vk
