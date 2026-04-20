#pragma once

#include "Libs.hpp"

class CameraBase
{
public:
  CameraBase() = default;
  virtual ~CameraBase() = default;

  void Update();

  void SetPos(const glm::vec3& pos) { mPos = pos; }
  void SetRot(const glm::vec3& rot);
  void SetSpeed(float speed) { mSpeed = speed; }

  const glm::mat4& GetMatrix() const { return mMat[mMatIdx]; }
  const glm::mat4& GetPrevMatrix() const { return mMat[!mMatIdx]; }
  const glm::vec3& GetPos() const { return mPos; }
  virtual glm::vec3 GetRot() const = 0;
  float GetSpeed() const { return mSpeed; }

protected:
  void ProcessInput();

  virtual glm::mat4 CalculateViewMatrix() = 0;
  void CalculateProjMatrix(float aspectRatio);
  glm::mat4 mMat[2]{glm::identity<glm::mat4>(), glm::identity<glm::mat4>()};
  glm::mat4 mProjMat{glm::identity<glm::mat4>()};
  bool mMatIdx;
  const float mFov{90.f};
  const float mNear{0.01f};
  const float mFar{20000.f};

  virtual void UpdateLocalBasis() = 0;
  glm::vec3 mForward;
  glm::vec3 mRight;
  glm::vec3 mUp{0.f, 0.f, 1.f};

  glm::vec3 mPos;
  float mSpeed{1.f};

  void KeyboardLookAround();
  void MouseEvents();
  void GetMouseOffset(float& xOffset, float& yOffset);
  void Rotate(float yawOffset, float pitchOffset);
  virtual void UpdateLocalRotation() = 0;
  double mLastX;
  double mLastY;
  float mSensitivity{30.f};
  float mYaw;
  float mPitch;

  bool mFirstClick;
  VkExtent2D mLastExtent;
};
