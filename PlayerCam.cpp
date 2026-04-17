#include "PlayerCam.hpp"

#include "Context.hpp"
#include "Swapchain.hpp"

constexpr glm::vec3 localForward{1.f, 0.f, 0.f};
constexpr glm::vec3 localRight{0.f, -1.f, 0.f};

void PlayerCam::SetRot(const glm::vec3& rot) noexcept
{
  assert(glm::all(glm::epsilonEqual(rot, glm::normalize(rot), 1e-4f)) &&
         "Parameter \"rot\" needs to be normalized!");

  mYaw = glm::atan(rot.y, rot.x);
  mPitch = glm::atan(rot.z, glm::distance(rot.x, rot.y));
  UpdateLocalRotation();
}

glm::vec3 PlayerCam::GetRot() const noexcept
{
  return mPlanetRotation * localForward;
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
    MouseEvents();
  }

  KeyboardLookAround();

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

  const glm::vec3 forward = mPlanetRotation * localForward;
  const glm::vec3 right = mPlanetRotation * localRight;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    mPos += forward * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    mPos -= forward * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    mPos += right * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    mPos -= right * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
  {
    mPos += mPlanetUp * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
  {
    mPos -= mPlanetUp * speed;
  }

  glm::vec3 oldPlanetUp = mPlanetUp;
  mPlanetUp = glm::normalize(mPos);

  glm::quat upAlignment = glm::rotation(oldPlanetUp, mPlanetUp);

  mSurfaceBasis = upAlignment * mSurfaceBasis;
  mSurfaceBasis = glm::normalize(mSurfaceBasis);

  mPlanetRotation = mSurfaceBasis * mLocalRotation;
}

inline glm::mat4 PlayerCam::CalculateViewMatrix()
{
  return glm::lookAt(mPos, mPos + mPlanetRotation * localForward, mPlanetUp);
}

inline void PlayerCam::CalculateProjMatrix(float aspectRatio)
{
  mProjMat = glm::perspective(glm::radians(mFov), aspectRatio, mNear, mFar);
  mProjMat[1][1] *= -1;
}

void PlayerCam::GetMouseOffset(float& x_offset, float& y_offset)
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

inline void PlayerCam::UpdateLocalRotation() noexcept
{
  mYaw -= (mYaw >= glm::radians(360.f)) ? glm::radians(360.f) : 0.f;
  mYaw += (mYaw <= glm::radians(-360.f)) ? glm::radians(360.f) : 0.f;
  mPitch = glm::clamp(mPitch, glm::radians(-89.9f), glm::radians(89.9f));

  glm::quat yawQ = glm::angleAxis(mYaw, glm::vec3(0.f, 0.f, -1.f));
  glm::quat pitchQ = glm::angleAxis(mPitch, glm::vec3(0.f, -1.f, 0.f));
  mLocalRotation = yawQ * pitchQ;
}

void PlayerCam::Rotate(float yawOffset, float pitchOffset) noexcept
{
  float deltaTime = (float)gContext.GetDeltaTime();
  mYaw += glm::radians(yawOffset * deltaTime);
  mPitch += glm::radians(pitchOffset * deltaTime);

  UpdateLocalRotation();
}

inline void PlayerCam::KeyboardLookAround()
{
  float yawOffset = 0.f, pitchOffset = 0.f;
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_I) == GLFW_PRESS)
  {
    pitchOffset += mSensitivity;
  }
  else if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_K) == GLFW_PRESS)
  {
    pitchOffset -= mSensitivity;
  }
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_J) == GLFW_PRESS)
  {
    yawOffset -= mSensitivity;
  }
  else if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_L) == GLFW_PRESS)
  {
    yawOffset += mSensitivity;
  }
  if (yawOffset != 0.f || pitchOffset != 0.f)
    Rotate(yawOffset, pitchOffset);
}

inline void PlayerCam::MouseEvents()
{
  float yawOffset = 0.f, pitchOffset = 0.f;
  GetMouseOffset(yawOffset, pitchOffset);
  Rotate(yawOffset, pitchOffset);
}
