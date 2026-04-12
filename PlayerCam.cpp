#include "PlayerCam.hpp"

#include <glm/gtx/quaternion.hpp>

#include "Context.hpp"
#include "Swapchain.hpp"

void PlayerCam::SetRot(const glm::vec3& rot) noexcept
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  mRot = rot;
  mDirPitch = std::asin(rot.z);
  mDirYaw = std::asin(rot.x / std::cos(mDirPitch));
  mDirPitch = glm::degrees(mDirPitch);
  mDirYaw = glm::degrees(mDirYaw);

  glm::vec3 planetNormal = glm::normalize(mPos);
  glm::quat alignment = glm::rotation(glm::vec3(0.f, 0.f, 1.f), planetNormal);
  mPlanetRot = alignment * mRot;
}

void PlayerCam::Update()
{
  if (mLastExtent.width != gSwapchain.GetExtent().width ||
      mLastExtent.height != gSwapchain.GetExtent().height)
  {
    CalculateProjMatrix(float(gSwapchain.GetExtent().width) /
                        gSwapchain.GetExtent().height);
    mLastExtent = gSwapchain.GetExtent();
  }

  ProcessInputEvents();

  if (!mFirstUpdate)
    mPrevMat = mMat;
  else
  {
    mPrevMat = mProjMat * CalculateViewMatrix();
    mFirstUpdate = false;
  }
  mMat = mProjMat * CalculateViewMatrix();
}

inline void PlayerCam::ProcessInputEvents()
{
  const auto window = gContext.GetWindow();

  // Rotation
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mFirstClick = true;
  }
  else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
  {
    ComputeMouseEvents();
  }

  glm::quat alignment = glm::rotation(glm::vec3(0.f, 0.f, 1.f), mPlanetUp);
  mPlanetRot = alignment * mRot;
  mPlanetRot = glm::normalize(mPlanetRot);

  // Position
  float speed = mSpeed * gContext.GetDeltaTime();
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
  {
    speed *= 10.f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
      speed *= 10.f;
    }
  }
  else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
  {
    speed *= 0.1f;
  }

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    mPos += mRot * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    mPos -= mRot * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    mPos += glm::cross(mPlanetRot, mPlanetUp) * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    mPos -= glm::cross(mPlanetRot, mPlanetUp) * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
  {
    mPos += mPlanetUp * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
  {
    mPos -= mPlanetUp * speed;
  }

  mPlanetUp = glm::normalize(mPos);
}

inline glm::mat4 PlayerCam::CalculateViewMatrix()
{
  return glm::lookAt(mPos, mPos + mPlanetRot, mPlanetUp);
  // return glm::lookAt(mPos, mPos + mRot, glm::vec3(0.f, 0.f, 1.f));
}

inline void PlayerCam::CalculateProjMatrix(float aspectRatio)
{
  mProjMat = glm::perspective(glm::radians(mFov), aspectRatio, mNear, mFar);
  mProjMat[1][1] *= -1;
}

inline void PlayerCam::GetMouseOffset(float& x_offset, float& y_offset)
{
  glfwSetInputMode(gContext.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double x_pos = 0.f, y_pos = 0.f;
  glfwGetCursorPos(gContext.GetWindow(), &x_pos, &y_pos);

  static double lastX, lastY;
  if (mFirstClick)
  {
    lastX = x_pos;
    lastY = y_pos;
    mFirstClick = false;
  }

  x_offset = x_pos - lastX;
  y_offset = lastY - y_pos;

  lastX = x_pos;
  lastY = y_pos;

  x_offset *= mSensitivity;
  y_offset *= mSensitivity;
}

inline void PlayerCam::ComputeMouseEvents()
{
  float x_offset;
  float y_offset;
  GetMouseOffset(x_offset, y_offset);

  mDirYaw += x_offset * gContext.GetDeltaTime();
  mDirPitch += y_offset * gContext.GetDeltaTime();
  mDirPitch = std::clamp(mDirPitch, -89.f, 89.f);

  UpdateRotation();
}

inline void PlayerCam::UpdateRotation()
{
  mRot.x = std::sin(glm::radians(mDirYaw)) * std::cos(glm::radians(mDirPitch));
  mRot.y = std::cos(glm::radians(mDirYaw)) * std::cos(glm::radians(mDirPitch));
  mRot.z = std::sin(glm::radians(mDirPitch));
  mRot = glm::normalize(mRot);
}
