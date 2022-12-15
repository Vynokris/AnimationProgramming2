#include "Utilities/Includer.h"
#include <iostream>

#include "imgui.h"
#include "imconfig.h"
#include "imgui_internal.h"

#include "Core/Engine.h"
#include "Core/Simulation.h"

#include <algorithm>

#include "Maths/Maths.h"
using namespace Maths;


void CSimulation::Initialize()
{
	ImGui::SetCurrentContext((ImGuiContext*)GetImGUIContext());

	// Load animations.
	Animator& animator = skeleton.GetAnimator();
	animator.AddAnimation(WALK_ANIMATION);
	animator.AddAnimation(RUN_ANIMATION );
	animator.SetCurrentAnimation(WALK_ANIMATION);
	
	for (size_t i = 0; i < GetSkeletonBoneCount(); i++)
	{
		// Get the bone's name and index.
		const int   boneIndex = (int)i;
		const char* boneName  = GetSkeletonBoneName(boneIndex);

		// Create the bone.
		Bone* bone = new Bone(boneIndex, boneName, animator);

		// Set the skeleton root and discard inverse kinematics bones.
		if (boneIndex == 0) {
			skeleton.SetRootBone(bone);
		}
		if (bone->name.find("ik_") != std::string::npos) {
			// Note: This works because all IK bones are at the end of the bone array.
			//       If it wasn't the case, some bone indices would be different from their IDs,
			//       because removing bones from the middle of the array would modify the indices of the following elements.
			delete bone;
			continue;
		}

		// Add the bone to the skeleton.
		skeleton.AddBone(bone);
	}

	for (Bone* bone : skeleton.GetBones()) 
	{
		// Set the bone's parent.
		bone->parent = skeleton.GetBone(GetSkeletonBoneParentIndex(bone->index));

		// Find and set the bone's children.
		// O(n^2)... didn't find a better solution.
		for (Bone* child : skeleton.GetBones()) {
			if (GetSkeletonBoneParentIndex(child->index) == bone->index) {
				bone->children.push_back(child);
			}
		}
	}

	// Set default pose bone transforms.
	skeleton.GetRootBone()->SetChildrenDefaultTransform();
}

void CSimulation::Update(float deltaTime)
{
	skeleton.UpdateAnimation(deltaTime);
	
	DrawGizmo    ({  55,  0, 0 }, 50);
	skeleton.Draw({ -80, 0, 0 });

	ShowImGui(deltaTime);
}

void CSimulation::DrawGizmo(const Vector3& offset, const float& size) const
{
	DrawLine(offset.x, offset.y, offset.z, offset.x+size, offset.y,      offset.z,      1, 0, 0);
	DrawLine(offset.x, offset.y, offset.z, offset.x,      offset.y+size, offset.z,      0, 1, 0);
	DrawLine(offset.x, offset.y, offset.z, offset.x,      offset.y,      offset.z+size, 0, 0, 1);
}

void CSimulation::ShowImGui(const float& deltaTime)
{
	if (ImGui::Begin("Stats"))
	{
		ImGui::Text("FPS: %d", roundInt(1/deltaTime));
		ImGui::Text("Delta Time: %f", deltaTime);
	}
	ImGui::End();

	if (ImGui::Begin("Animations", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		Animator& animator = skeleton.GetAnimator();
		const Animation* curAnim = animator.GetCurrentAnimation();
		std::unordered_map<std::string, Animation*>& animations = animator.GetAllAnimations();

		// Current animation.
		ImGui::Text("Current animation:");
		if (ImGui::BeginCombo("##curAnimInput", curAnim->name.c_str()))
		{
			for (auto& [name, anim] : animations)
			{
				const bool isSelected = name == curAnim->name;
				if (ImGui::Selectable(name.c_str(), isSelected))
					animator.SetCurrentAnimation(name);

				if (isSelected)
                    ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::NewLine();
		
		for (auto& [name, anim] : animations)
		{
			if (ImGui::CollapsingHeader(name.c_str()))
			{
				// Current key frame.
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Current keyframe:");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(19);
				if (ImGui::DragInt("##currentKeyframeInput", &anim->curKeyframe, 0.1f, 0, anim->keyframeCount-1))
					anim->curKeyframe = std::clamp(anim->curKeyframe, 0, anim->keyframeCount-1);
				ImGui::SameLine();
				ImGui::Text("/%d", anim->keyframeCount);
				
				// Animation pause.
				ImGui::Checkbox("Paused", &anim->paused);
				
				// Animation reverse.
				ImGui::Checkbox("Reverse", &anim->reverse);

				// Animation speed.
				float animSpeed = 1 / anim->keyframeDuration;
				ImGui::SetNextItemWidth(19);
				if (ImGui::DragFloat("Speed", &animSpeed, 0.1f, 0.5f, 99.f, "%.0f")) {
					anim->keyframeDuration = 1 / clamp(animSpeed, 0.5f, 99.f);
				}
			}
		}
	}
	ImGui::End();
}

