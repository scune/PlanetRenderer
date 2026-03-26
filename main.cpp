#include "Context.hpp"
#include "PlanetRenderer.hpp"
#include "ResultCheck.hpp"
#include "Swapchain.hpp"

PlanetRenderer planetRenderer;

void Initialize(bool force_x11)
{
  gContext.Init(force_x11);
  gSwapchain.Init();
  gContext.CreateCmdBufferPreRec(gSwapchain.GetViews().size());
  planetRenderer.Init();
}

void Run()
{
  // Prerecord cmd buffer
  for (uint32_t i = 0; i < gSwapchain.GetViews().size(); i++)
  {
    gSwapchain.SetFrameImgIndex(i);
    gContext.BeginCmdBufferPreRec(i);
    planetRenderer.DrawGraphics(gContext.GetCmdBufferPreRec(i), i);
    gContext.EndCmdBufferPreRec(i);
  }
  gSwapchain.SetFrameImgIndex(0);

  while (!gContext.WindowShouldClose())
  {
    glfwPollEvents();

    IfNThrow(gContext.WaitForFence(gContext.GetFencePreRec()),
             "Failed to wait for fence!");

    if (gContext.FrameCap())
      continue;

    gContext.UpdateWindowTitle();

    gSwapchain.AcquireNextImage();
    IfNThrow(gContext.ResetFence(gContext.GetFencePreRec()),
             "Failed to reset fence!");

    planetRenderer.Update();

    gContext.SubmitWorkPreRec(gSwapchain.GetFrameImgIndex());
    gSwapchain.Present();
  }
}

void Destroy() noexcept
{
  if (gContext.GetDevice() != VK_NULL_HANDLE)
  {
    if (vkDeviceWaitIdle(gContext.GetDevice()) != VK_SUCCESS)
      return;
  }

  planetRenderer.Destroy();
  gSwapchain.Destroy();
  gContext.Destroy();
}

int main(int argc, char* argv[])
{
  bool force_x11 = false;
  for (int i = 0; i < argc; i++)
  {
    if (strcmp(argv[i], "-x11") == 0)
    {
      force_x11 = true;
      break;
    }
  }

  int exitStatus = EXIT_SUCCESS;

  try
  {
    Initialize(force_x11);

    Run();
  }
  catch (const std::exception& exception)
  {
    PrintErr(exception);
    exitStatus = EXIT_FAILURE;
  }

  Destroy();
  return exitStatus;
}
