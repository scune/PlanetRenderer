#pragma once

#include "Libs.hpp"

#include <glm/gtx/quaternion.hpp>

class PlayerCam
{
public:
  PlayerCam() = default;

  void Update();

  inline void SetPos(const glm::vec3& pos) noexcept { mPos = pos; }
  void SetRot(const glm::vec3& rot) noexcept;
  inline void SetSpeed(float speed) noexcept { mSpeed = speed; }

  inline const glm::mat4& GetMatrix() const noexcept { return mMat; }
  inline const glm::mat4& GetPrevMatrix() const noexcept { return mPrevMat; }
  inline const glm::vec3& GetPos() const noexcept { return mPos; }
  inline glm::vec3 GetRot() const noexcept
  {
    return mOrientation * glm::vec3(0.f, 0.f, -1.f);
  }
  inline float GetNear() const noexcept { return mNear; }
  inline float GetFar() const noexcept { return mFar; }

private:
  inline void ProcessInputEvents();

  inline glm::mat4 CalculateViewMatrix();
  inline void CalculateProjMatrix(float aspectRatio);
  glm::mat4 mMat{1.f};
  glm::mat4 mPrevMat{1.f};
  glm::mat4 mProjMat{1.f};
  const float mFov{90.f};
  const float mNear{0.01f};
  const float mFar{20000.f};

  glm::vec3 mPos;
  glm::vec3 mPlanetUp{0.f, 0.f, 1.f};
  float mSpeed{1.f};

  void GetMouseOffset(float& x_offset, float& y_offset);
  void Rotate(float yawOffset, float pitchOffset);
  inline void KeyboardLookAround();
  inline void MouseEvents();

  glm::quat mOrientation{glm::quat_identity<float, glm::defaultp>()};
  glm::quat mMovementOri{glm::quat_identity<float, glm::defaultp>()};
  glm::vec3 mLocalForward{-1.f, 0.f, 0.f};
  glm::vec3 mLocalRight{0.f, 1.f, 0.f};
  glm::vec3 mLocalUp{0.f, 0.f, 1.f};

  double mLastX;
  double mLastY;
  float mSensitivity{30.f};
  float mYaw;
  float mPitch;

  bool mFirstUpdate{true};
  bool mFirstClick;
  VkExtent2D mLastExtent;
};
