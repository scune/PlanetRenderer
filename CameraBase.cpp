#include "CameraBase.hpp"

#include "Swapchain.hpp"

void CameraBase::Update()
{
  if (mLastExtent.width != gSwapchain.GetExtent().width ||
      mLastExtent.height != gSwapchain.GetExtent().height)
  {
    CalculateProjMatrix(float(gSwapchain.GetExtent().width) /
                        gSwapchain.GetExtent().height);
    mLastExtent = gSwapchain.GetExtent();
  }

  ProcessInput();

  mMatIdx = !mMatIdx;
  mMat[mMatIdx] = mProjMat * CalculateViewMatrix();
}

void CameraBase::ProcessInput()
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

  // Speed
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

  // Position
  UpdateLocalBasis();

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    mPos += mForward * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    mPos -= mForward * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    mPos += mRight * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    mPos -= mRight * speed;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
  {
    mPos += mUp * speed;
  }
  else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
  {
    mPos -= mUp * speed;
  }
}

void CameraBase::CalculateProjMatrix(float aspectRatio)
{
  mProjMat = glm::perspective(glm::radians(mFov), aspectRatio, mNear, mFar);
  mProjMat[1][1] *= -1;
}

void CameraBase::KeyboardLookAround()
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

void CameraBase::MouseEvents()
{
  float yawOffset = 0.f, pitchOffset = 0.f;
  GetMouseOffset(yawOffset, pitchOffset);
  Rotate(yawOffset, pitchOffset);
}

void CameraBase::GetMouseOffset(float& xOffset, float& yOffset)
{
  glfwSetInputMode(gContext.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double xPos = 0.f, yPos = 0.f;
  glfwGetCursorPos(gContext.GetWindow(), &xPos, &yPos);

  if (mFirstClick)
  {
    mLastX = xPos;
    mLastY = yPos;
    mFirstClick = false;
  }

  xOffset = xPos - mLastX;
  yOffset = mLastY - yPos;

  mLastX = xPos;
  mLastY = yPos;

  xOffset *= mSensitivity;
  yOffset *= mSensitivity;
}

void CameraBase::Rotate(float yawOffset, float pitchOffset)
{
  float deltaTime = (float)gContext.GetDeltaTime();
  mYaw -= glm::radians(yawOffset * deltaTime);
  mPitch -= glm::radians(pitchOffset * deltaTime);

  UpdateLocalRotation();
}

void CameraBase::UpdateLocalRotation()
{
  mYaw -= (mYaw >= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mYaw += (mYaw <= glm::two_pi<float>()) ? glm::two_pi<float>() : 0.f;
  mPitch = glm::clamp(mPitch, glm::radians(-89.f), glm::radians(89.f));

  glm::quat yawQ = glm::angleAxis(mYaw, glm::vec3(0.f, 0.f, 1.f));
  glm::quat pitchQ = glm::angleAxis(mPitch, glm::vec3(0.f, 1.f, 0.f));
  mLocalRot = yawQ * pitchQ;
}
