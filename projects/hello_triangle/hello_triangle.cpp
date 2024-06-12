
#include "sample.hpp"

#include <iostream> // std::cout, std::endl

class HelloTriangle final : public Sample {
    public:
        HelloTriangle() : Sample("Hello Triangle") {
        }
        
        ~HelloTriangle() override {
        }
        
    private:
        void record_command_buffers() override {
        }
        
        void initialize_render_passes() override {
            // https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-objects
            // A render pass object represents a collection of attachments, subpasses, and dependencies between the subpasses
            // It describes how the attachments are used over the course of a set of subpasses
            
            VkRenderPassCreateInfo render_pass_ci { };
            render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            
            // An attachment description describes the properties of a render pass attachment
            // This render pass uses one color attachment
            VkAttachmentDescription color_attachment_description { };
        
            // format - specifies the format of the image that will be used for the attachment
            // Since this will be presented to the swapchain, use the format of the surface
            color_attachment_description.format = surface_format.format;
            
            // samples - number of samples to use, for multisampling support
            // This example does not use multisampling
            color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
        
            // loadOp - what to do with the attachment data before the render pass:
            //   - VK_ATTACHMENT_LOAD_OP_LOAD - preserve the existing contents of the attachment
            //   - VK_ATTACHMENT_LOAD_OP_CLEAR - clear the values to a constant at the start
            //   - VK_ATTACHMENT_LOAD_OP_DONT_CARE - existing contents are undefined
            // Clear the attachment to a uniform value (defined in VkRenderPassBeginInfo)
            color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        
            // storeOp - what to do with the attachment data after the render pass:
            //   - VK_ATTACHMENT_STORE_OP_STORE - rendered contents will be stored in memory and can be read later
            //   - VK_ATTACHMENT_STORE_OP_DONT_CARE - contents of the framebuffer will be undefined after the rendering operation
            // Store the color for presentation to the swapchain
            color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        
            // stencilLoadOp - what to do with the stencil buffer data before the render pass (same values as loadOp)
            // stencilStoreOp - what to do with the stencil buffer data after the render pass (same values as storeOp)
            // This example does not use a stencil buffer
            color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        
            // Render pass images need to be transitioned to specific layouts that are suitable for the next operation
        
            // The color attachment was previously used for presenting before being returned to the swapchain
            // It doesn't matter what the contents of the attachment are
            color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        
            // The color attachment should be transitioned to a format that is suitable to be presented to the swapchain at the end of the render pass
            color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            
            render_pass_ci.attachmentCount = 1;
            render_pass_ci.pAttachments = &color_attachment_description;
            
            // A subpass defines a phase of a render pass that reads from and writes to a subset of attachments
            
            // When a render pass is created, attachments are provided an initial layout and final layout, defined as initialLayout and finalLayout (respectively) on the attachment description above
            // Using the subpass description during subpass creation, color attachments are given a layout representing how the subpass uses the attachment
            // Vulkan performs implicit layout transitions for attachments at the following points:
            //   1. At the start of the render pass (vkCmdBeginRenderPass), each attachment is transitioned to its initial layout (attachment_description.initialLayout)
            //   2. Between subpasses, each used attachment is transitioned to the layout the subpass is expecting (subpass_description[i].color_attachment.layout)
            //   3. At the end of the render pass (vkCmdEndRenderPass), each attachment layout is transitioned to its final layout (attachment_description.finalLayout)
            
            VkSubpassDescription subpass_description { };
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            
            VkAttachmentReference color_attachment { };
        
            // attachment - index that is referenced by the layout(location = 0) out vec4 outColor line in the fragment shader
            // Note: this must also align with the first element in the attachment_descriptions array initialized above (order of attachment descriptions and attachment references matters), as this specifies which attachment is being referred to
            color_attachment.attachment = 0;
            
            // layout - specify the layout the attachment uses during the subpass
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment;
            
            subpass_description.pDepthStencilAttachment = nullptr; // Used for depth buffering
            subpass_description.pResolveAttachments = nullptr; // Used for resolving multisampled color attachments
            subpass_description.pInputAttachments = nullptr; // readonly attachments
            subpass_description.pPreserveAttachments = nullptr; // Attachments that are not used in this subpass but that should be preserved for later subpasses
        
            render_pass_ci.subpassCount = 1;
            render_pass_ci.pSubpasses = &subpass_description;
            
            // Define subpass dependencies
            // Subpass dependencies ensure that operations in one subpass complete before operations in another subpass that depend on the same resources begin (synchronization)
            // Subpass dependencies specify memory and execution dependencies between subpasses

            
            // srcSubpass - index of the subpass to depend on
            // dstSubpass - index of the current subpass (dstSubpass depends on srcSubpass)
            
            // srcStageMask - Vulkan stages that need to finish executing in the dependent subpass (srcSubpass) before we can execute the current subpass (dstSubpass)
            // dstStageMask - Vulkan stages in the current subpass (dstSubpass) that cannot be executed until after all stages specified in srcStageMask are completed
            
            // srcAccessMask - Vulkan memory access types used by the dependent subpass (srcSubpass)
            // dstAccessMask - Vulkan memory access types used by the current subpass (dstSubpass)
            
            
            // Vulkan defines a set of implicit subpass dependencies:
            //   1. Between external and initial subpass - all external operations before the start of the render pass (before the call to vkCmdBeginRenderPass) must complete before the start of the first subpass
            //
            //      Implicit subpass structure:
            //        srcStageMask = top of the render pipeline (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), the beginning of the render pass
            //        srcAccessMask = 0 (no access)
            //        dstStageMask = earliest specified stage in the first subpass
            //        dstAccessMask = memory access required by the first subpass
            //
            //      This subpass ensures that any image layout transitions necessary for the first subpass occur before the specified subpass stage
            //
            //   2. Between each pair of adjacent subpasses - all operations from one subpass must complete before the next subpass starts
            //
            //      Implicit subpass structure:
            //        srcStageMask = last stage of the previous subpass
            //        srcAccessMask = memory access required by the previous subpass
            //        dstStageMask = earliest stage of the next subpass
            //        dstAccessMask = memory access required by the next subpass
            //
            //   3. Between the final subpass and external - all operations from the final subpass must complete before any external operations (after the call to vkCmdEndRenderPass) begin
            //
            //      Implicit subpass structure:
            //        srcStageMask = last stage of the final subpass
            //        srcAccessMask = memory access required by the final subpass
            //        dstStageMask = bottom of the render pipeline (VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT), the end of the render pass
            //        dstAccessMask = 0 (no access)
            //
            //      This subpass ensures that any image layout transitions necessary after the final subpass occur before after the specified subpass stage
            //
            // Setting srcSubpass to VK_SUBPASS_EXTERNAL means creating a dependency on the first implicit subpass (commands that need to be completed are those that happened before the start of the render pass)
            // Setting dstSubpass to VK_SUBPASS_EXTERNAL means creating a dependency on the third implicit subpass (commands that need to be completed are those that will happen after the end of the render pass)
            
            VkSubpassDependency subpass_dependency { };

            // The first color attachment refers to an image retrieved from the swapchain (call to vkAcquireNextImageKHR)
            // We must ensure this image is available before any color operations (writes) happen
            // LOAD operations for color attachments happen in the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT pipeline stage, so this is a good stage to wait on to guarantee that the swapchain image is ready for color output
            //   ref: https://registry.khronos.org/vulkan/specs/1.2-khr-extensions/html/chap8.html#renderpass-load-operations
            
            // We need to be explicit about waiting on this stage, as otherwise the render pass may proceed without waiting for the swapchain image to be available (potentially resulting in writes to a swapchain image that is still being presented)
            // Keeping in mind the implicit subpass dependency automatically provided by Vulkan between the start of the render pass and the first subpass, this also means that somewhere before the color output stage the attachment will be transitioned into the layout specified by the first subpass
            
            subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            subpass_dependency.dstSubpass = 0;

            subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpass_dependency.srcAccessMask = 0;
        
            subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            
            render_pass_ci.dependencyCount = 1;
            render_pass_ci.pDependencies = &subpass_dependency;
        
            if (vkCreateRenderPass(device, &render_pass_ci, nullptr, &render_pass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        
        void render() override {
            // The base sample uses multiple frames in flight to avoid forcing the CPU to wait on the GPU to finish rendering the previous frame to start rendering a new one
            // With multiple frames in flight, the GPU can be rendering one frame while the CPU is recording commands for rendering another
            // This requires separate command buffers and synchronization primitives (semaphores, fences) per frame in flight to avoid any interference across two frames
            
            // The render function is kicked off once the resources associated with the current frame (frame_index) are no longer in use by the GPU
            // The sample is free to use the resources identified above at index frame_index to begin recording commands for rendering a new frame
            // The Sample base also takes care of presenting the finished frame to the screen
            
            VkCommandBuffer command_buffer = command_buffers[frame_index];
            VkSemaphore is_image_available = is_presentation_complete[frame_index];
            
            // Retrieve the index of the swapchain image to use for this frame
            // Note that this may differ from frame_index as this is controlled by swapchain internals
            unsigned image_index;
            VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<std::uint64_t>::max(), is_image_available, VK_NULL_HANDLE, &image_index);
            // TODO: handle different return values
    
            // Record command buffer(s)
            vkResetCommandBuffer(command_buffer, 0);
            
            // TODO:
            // record_command_buffers(image_index);
    
            VkSubmitInfo submit_info { };
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Ensure that the swapchain image is available before executing any color operations (writes) by waiting on the pipeline stage that writes to the color attachment (discussed in detail during render pass creation above)
            // Another approach that can be taken here is to wait on VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, which would ensure that no command buffers are executed before the image swapchain image is ready (vkAcquireNextImageKHR signals is_image_available, queue execution waits on is_image_available)
            // However, this is not the preferred approach - waiting on VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT will completely block the pipeline until the swapchain image is ready
            // Instead, waiting on the pipeline stage where writes are performed to the color attachment allows Vulkan to begin scheduling other work that happens before the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage is reached for execution (such as invoking the vertex shader)
            // This way, the implementation waits only the time that is absolutely necessary for coherent memory operations
            
            VkSemaphore wait_semaphores[] = { is_image_available }; // Semaphore(s) to wait on before the command buffers can begin execution
            VkPipelineStageFlags wait_stage_flags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // Note: wait_stage_flags and wait_semaphores have a 1:1 correlation, meaning it is possible to wait on and signal different semaphores at different pipeline stages
            
            // Waiting on the swapchain image to be ready (if not yet) when the pipeline is ready to perform writes to color attachments
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = wait_semaphores;
            submit_info.pWaitDstStageMask = wait_stage_flags;
            
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer; // Command buffer(s) to execute
    
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &is_rendering_complete[frame_index]; // Semaphore to signal when all command buffer(s) have finished executing
    
            // Submit
            if (vkQueueSubmit(queue, 1, &submit_info, is_frame_in_flight[frame_index]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }
        }
        
        
};

DEFINE_SAMPLE_MAIN(HelloTriangle);