// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "ComJoint.h"

BE_NAMESPACE_BEGIN

class LuaVM;

class ComCharacterJoint : public ComJoint {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComCharacterJoint);

    ComCharacterJoint();
    virtual ~ComCharacterJoint();

    virtual void            Init() override;

    virtual void            Start() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    const Vec3 &            GetAnchor() const;
    void                    SetAnchor(const Vec3 &anchor);

    const Angles            GetAngles() const;
    void                    SetAngles(const Angles &angles);

    const float             GetSwing1LowerLimit() const;
    void                    SetSwing1LowerLimit(const float limit);

    const float             GetSwing1UpperLimit() const;
    void                    SetSwing1UpperLimit(const float limit);

    const float             GetSwing1Stiffness() const;
    void                    SetSwing1Stiffness(const float stiffness);

    const float             GetSwing1Damping() const;
    void                    SetSwing1Damping(const float damping);

    const float             GetSwing2LowerLimit() const;
    void                    SetSwing2LowerLimit(const float limit);

    const float             GetSwing2UpperLimit() const;
    void                    SetSwing2UpperLimit(const float limit);

    const float             GetSwing2Stiffness() const;
    void                    SetSwing2Stiffness(const float stiffness);

    const float             GetSwing2Damping() const;
    void                    SetSwing2Damping(const float damping);

    const float             GetTwistLowerLimit() const;
    void                    SetTwistLowerLimit(const float limit);

    const float             GetTwistUpperLimit() const;
    void                    SetTwistUpperLimit(const float limit);

    const float             GetTwistStiffness() const;
    void                    SetTwistStiffness(const float stiffness);

    const float             GetTwistDamping() const;
    void                    SetTwistDamping(const float damping);

    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    anchor;
    Mat3                    axis;
    Vec3                    lowerLimit;
    Vec3                    upperLimit;
    Vec3                    stiffness;
    Vec3                    damping;
};

BE_NAMESPACE_END
