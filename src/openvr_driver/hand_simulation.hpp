//============ Copyright (c) Valve Corporation, All rights reserved. ============
#pragma once

#include "openvr_driver.h"

// -1-1 values (1 fully curled)
// Represents a single joint on a glove
struct GloveFingerBend {
	float proximal;
	float distal;
};

// -1-1 values (1 fully curled)
struct GloveFingerCurls
{
	GloveFingerBend thumb;
	GloveFingerBend index;
	GloveFingerBend middle;
	GloveFingerBend ring;
	GloveFingerBend pinky;
};

//-1-1 values (1 fully to the left)
struct GloveFingerSplays
{
	float thumb;
	float index;
	float middle;
	float ring;
	float pinky;
};

enum HandSkeletonBone {
    kHandSkeletonBone_Root = 0,
    kHandSkeletonBone_Wrist,
    kHandSkeletonBone_Thumb0,
    kHandSkeletonBone_Thumb1,
    kHandSkeletonBone_Thumb2,
    kHandSkeletonBone_Thumb3,
    kHandSkeletonBone_IndexFinger0,
    kHandSkeletonBone_IndexFinger1,
    kHandSkeletonBone_IndexFinger2,
    kHandSkeletonBone_IndexFinger3,
    kHandSkeletonBone_IndexFinger4,
    kHandSkeletonBone_MiddleFinger0,
    kHandSkeletonBone_MiddleFinger1,
    kHandSkeletonBone_MiddleFinger2,
    kHandSkeletonBone_MiddleFinger3,
    kHandSkeletonBone_MiddleFinger4,
    kHandSkeletonBone_RingFinger0,
    kHandSkeletonBone_RingFinger1,
    kHandSkeletonBone_RingFinger2,
    kHandSkeletonBone_RingFinger3,
    kHandSkeletonBone_RingFinger4,
    kHandSkeletonBone_PinkyFinger0,
    kHandSkeletonBone_PinkyFinger1,
    kHandSkeletonBone_PinkyFinger2,
    kHandSkeletonBone_PinkyFinger3,
    kHandSkeletonBone_PinkyFinger4,
    kHandSkeletonBone_AuxThumb,
    kHandSkeletonBone_AuxIndexFinger,
    kHandSkeletonBone_AuxMiddleFinger,
    kHandSkeletonBone_AuxRingFinger,
    kHandSkeletonBone_AuxPinkyFinger,
    kHandSkeletonBone_Count
};

class GloveHandSimulation
{
public:
	void ComputeSkeletonTransforms(vr::ETrackedControllerRole role, const GloveFingerCurls& curls, const GloveFingerSplays& splays, vr::VRBoneTransform_t* out_transforms);
};