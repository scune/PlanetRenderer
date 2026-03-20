#include "Camera.hpp"

#include "Context.hpp"
#include "Swapchain.hpp"

void Camera::Update()
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

inline void Camera::ProcessInputEvents()
{
  // Rotation
  if (glfwGetMouseButton(gContext.GetWindow(), GLFW_MOUSE_BUTTON_RIGHT) ==
      GLFW_RELEASE)
  {
    glfwSetInputMode(gContext.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mFirstClick = true;
  }
  else if (glfwGetMouseButton(gContext.GetWindow(), GLFW_MOUSE_BUTTON_RIGHT) ==
           GLFW_PRESS)
  {
    ComputeMouseEvents();
  }

  // Position
  float speed = mSpeed * gContext.GetDeltaTime();
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
  {
    speed *= 10.f;
    if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
      speed *= 4.f;
    }
  }
  else if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
  {
    speed *= 0.1f;
  }

  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
  {
    mPos += mRot * speed;
  }
  else if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
  {
    mPos -= mRot * speed;
  }
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
  {
    mPos += glm::normalize(glm::cross(mRot, glm::vec3(0.f, 0.f, 1.f))) * speed;
  }
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
  {
    mPos -= glm::normalize(glm::cross(mRot, glm::vec3(0.f, 0.f, 1.f))) * speed;
  }
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_E) == GLFW_PRESS)
  {
    mPos += glm::vec3(0.f, 0.f, 1.f) * speed;
  }
  else if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_Q) == GLFW_PRESS)
  {
    mPos -= glm::vec3(0.f, 0.f, 1.f) * speed;
  }
}

inline glm::mat4 Camera::CalculateViewMatrix()
{
  return glm::lookAt(mPos, mPos + mRot, glm::vec3(0.0f, 0.0f, 1.0f));
}

inline void Camera::CalculateProjMatrix(float aspectRatio)
{
  mProjMat = glm::perspective(glm::radians(mFov), aspectRatio, mNear, mFar);
  mProjMat[1][1] *= -1;
}

inline void Camera::GetMouseOffset(float& x_offset, float& y_offset)
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

inline void Camera::ComputeMouseEvents()
{
  float x_offset;
  float y_offset;
  GetMouseOffset(x_offset, y_offset);

  mDirYaw += x_offset * gContext.GetDeltaTime();
  mDirPitch += y_offset * gContext.GetDeltaTime();
  mDirPitch = std::clamp(mDirPitch, -89.f, 89.f);

  UpdateRotation();
}

inline void Camera::UpdateRotation()
{
  glm::vec3 direction;
  direction.x = std::sin(glm::radians(mDirYaw)) *
                std::cos(glm::radians(mDirPitch)); // Pitch // Zur anderen Seite
  direction.y = std::cos(glm::radians(mDirYaw)) *
                std::cos(glm::radians(mDirPitch)); // Yaw   // Zur Seite
  direction.z = std::sin(glm::radians(mDirPitch)); // Roll  // Hoch und Runter
  direction = glm::normalize(direction);

  mRot += direction - mLastDir;
  mLastDir = direction;
}
