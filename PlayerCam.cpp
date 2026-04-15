#include "PlayerCam.hpp"

#include <glm/gtx/quaternion.hpp>

#include "Context.hpp"
#include "Swapchain.hpp"

void PlayerCam::SetRot(const glm::vec3& rot) noexcept
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");
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

  glm::vec3 moveForward = mMovementOri * glm::vec3(-1.f, 0.f, 0.f);
  glm::vec3 moveRight = mMovementOri * glm::vec3(0.f, 1.f, 0.f);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    mPos += moveForward * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    mPos -= moveForward * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    mPos += moveRight * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    mPos -= moveRight * speed;
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
  return glm::lookAt(mPos, mPos + mLocalForward, mLocalUp);
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

  if (mFirstClick)
  {
    mLastX = x_pos;
    mLastY = y_pos;
    mFirstClick = false;
  }

  x_offset = x_pos - mLastX;
  y_offset = mLastY - y_pos;

  mLastX = x_pos;
  mLastY = y_pos;

  x_offset *= mSensitivity;
  y_offset *= mSensitivity;
}

inline void PlayerCam::ComputeMouseEvents()
{
  float x_offset;
  float y_offset;
  GetMouseOffset(x_offset, y_offset);

  float deltaTime = (float)gContext.GetDeltaTime();
  mYaw -= glm::radians(x_offset * deltaTime);
  mYaw -= (mYaw >= glm::radians(360.f)) ? glm::radians(360.f) : 0.f;
  mYaw += (mYaw <= glm::radians(-360.f)) ? glm::radians(360.f) : 0.f;
  mPitch += glm::radians(y_offset * deltaTime);
  mPitch = std::clamp(mPitch, glm::radians(-89.9f), glm::radians(89.9f));

  glm::quat yawQ = glm::angleAxis(mYaw, glm::vec3(0.f, 0.f, 1.f));
  glm::quat pitchQ = glm::angleAxis(mPitch, glm::vec3(0.f, 1.f, 0.f));
  mOrientation = yawQ * pitchQ;

  glm::quat alignRotation = glm::rotation({0.f, 0.f, 1.f}, mPlanetUp);
  mOrientation = alignRotation * mOrientation;

  mMovementOri = mOrientation;

  mLocalForward = mOrientation * glm::vec3(-1.f, 0.f, 0.f);
  mLocalRight = mOrientation * glm::vec3(0.f, 1.f, 0.f);
  mLocalUp = mOrientation * glm::vec3(0.f, 0.f, 1.f);
}
