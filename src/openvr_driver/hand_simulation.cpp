//============ Copyright (c) Valve Corporation, All rights reserved. ============
// Inspired by Moshi Turner's code from Monado https://gitlab.freedesktop.org/monado/monado/-/blob/main/src/xrt/auxiliary/util/u_hand_simulation.c
#include "hand_simulation.hpp"
#include "maths.hpp"

struct HandSimSplayableJoint
{
    vr::HmdVector2_t swing = { 0.f, 0.f };
    float twist = 0.f;
};

struct HandSimJoint
{
    float rotation = 0.f;
};

struct HandSimThumb
{
    HandSimSplayableJoint metacarpal;	// bones in the palm
    HandSimSplayableJoint proximal;		// "root" bone
    HandSimJoint distal;				// "tip" of the finger
};

struct HandSimFinger
{
    HandSimSplayableJoint metacarpal;	// bones in the palm
    HandSimSplayableJoint proximal;		// "root" bone
    HandSimJoint intermediate;			// second joint
    HandSimJoint distal;				// "tip" of the finger
};

struct HandSimHand
{
    vr::ETrackedControllerRole role; // Whether we're left or right handed

    HandSimThumb thumb;

    // index, middle, ring, pinky (in that order)
    HandSimFinger fingers[4];
};

// rough finger lengths
static const float finger_joint_lengths[5][5] = {
    { 0.05f, 0.05f,  0.035f, 0.025f, 0.f },	    // thumb
    { 0.03f, 0.073f, 0.045f, 0.025f, 0.02f },   // index
    { 0.01f, 0.091f, 0.049f, 0.03f,  0.02f },   // middle
    { 0.02f, 0.073f, 0.045f, 0.03f,  0.03f },   // ring
    { 0.03f, 0.067f, 0.03f,  0.025f, 0.02f },   // pinky
};

//-----------------------------------------------------------------------------
// Purpose: Sets up a default open hand pose which can then be manipulated with curl and splay values.
// Much of this is very approximate.
//-----------------------------------------------------------------------------
static void InitHand(HandSimHand& out_hand)
{
    // Default curls for each of the fingers
    for (auto& finger : out_hand.fingers)
    {
        finger.metacarpal.swing.v[1] = 0.f;
        finger.metacarpal.twist = 0.f;

        finger.proximal.swing.v[1] = DegToRad(10.f);
        finger.intermediate.rotation = DegToRad(5.f);

        finger.intermediate.rotation = DegToRad(5.f);
        finger.distal.rotation = DegToRad(5.f);
    }

    // Curl, splay and "twist" the thumb into a default position
    out_hand.thumb.metacarpal.swing.v[0] = DegToRad(10.f);
    out_hand.thumb.metacarpal.swing.v[1] = DegToRad(40.f);
    out_hand.thumb.metacarpal.twist = DegToRad(70.f);

    out_hand.thumb.proximal.swing.v[0] = 0.f;
    out_hand.thumb.proximal.swing.v[1] = 0.f;
    out_hand.thumb.proximal.twist = 0.f;

    out_hand.thumb.distal.rotation = 0.f;

    // Metacarpal splays for each of the fingers. Do this to add some spread to the fingers' metacarpal (the joints that connect to the wrist).
    out_hand.fingers[0].metacarpal.swing.v[1] = DegToRad(13.f);
    out_hand.fingers[1].metacarpal.swing.v[1] = DegToRad(-0.f);
    out_hand.fingers[2].metacarpal.swing.v[1] = DegToRad(-15.f);
    out_hand.fingers[3].metacarpal.swing.v[1] = DegToRad(-27.f);

    // Proximal splays for each of the fingers. Do this to add some spread to the finger's proximal joints. These joints are also the ones that splay with the values provided.
    out_hand.fingers[0].proximal.swing.v[1] = DegToRad(3.f);
    out_hand.fingers[1].proximal.swing.v[1] = DegToRad(0.f);
    out_hand.fingers[2].proximal.swing.v[1] = DegToRad(-1.f);
    out_hand.fingers[3].proximal.swing.v[1] = DegToRad(-2.f);
}

//-----------------------------------------------------------------------------
// Purpose: All fingers (expect thumb) have basically the same properties in terms of curl and splay.
// You can splay only the proximal, and curl the proximal, intermediate and distal joints.
// This applies the curl and splay to these joints.
//-----------------------------------------------------------------------------
static void ApplyGenericFingerTransform(const GloveFingerBend& curl, const float splay, HandSimFinger& out_finger)
{
    out_finger.metacarpal.swing.v[0] += DegToRad(curl.proximal * 5.f); // the metacarpal only curls a little

    out_finger.proximal.swing.v[0] += DegToRad(curl.proximal * 90.f);
    out_finger.proximal.swing.v[1] += DegToRad(splay * 15.f);

    // Distal bone has a much higher influence in the intermediate bone's rotation
    const float curlIntermediate = (curl.proximal * 0.25f + curl.distal * 0.75f);
    out_finger.intermediate.rotation += DegToRad(curlIntermediate * 80.f);
    out_finger.distal.rotation += DegToRad(curl.distal * 80.f);
}


//-----------------------------------------------------------------------------
// Purpose: Takes an orientation and length of a joint and converts it to a vr::VRBoneTransform_t
// We have to do convert the quaternion as we use vr::HmdQuaternion_t for our representation, but OpenVR wants vr::HmdQuaternionf_t
//-----------------------------------------------------------------------------
static void ComputeBoneTransform(const vr::ETrackedControllerRole role, const vr::HmdQuaternion_t& orientation, const vr::HmdVector3_t& position, vr::VRBoneTransform_t& out_transform)
{
    // Note that we use vr::HmdQuaternion_t but the Skeletal Input API needs vr::HmdQuaternionf_t
    out_transform.orientation.w = (float) orientation.w;
    out_transform.orientation.x = (float) orientation.x;
    out_transform.orientation.y = (float) orientation.y;
    out_transform.orientation.z = (float) orientation.z;

    // Fit the HmdVector3_t into the HmdVector4_t that the Skeletal Input API accepts
    out_transform.position.v[0] = position.v[0];
    out_transform.position.v[1] = position.v[1];
    out_transform.position.v[2] = position.v[2];
    out_transform.position.v[3] = 1.f;

    //"up" axis is flipped between hands, so inverse the joint length for the right hand, as we base all our computations of the left skeleton pose.
    if (role == vr::TrackedControllerRole_RightHand)
    {
        out_transform.position.v[0] *= -1.f;
    }
}


//-----------------------------------------------------------------------------
// Purpose: This is just a little helper function to make our calls when trying to compute each bone a little simpler by specifying just a float for joint length, instead of the whole vector
//-----------------------------------------------------------------------------
static void ComputeBoneTransform(const vr::ETrackedControllerRole role, const vr::HmdQuaternion_t& orientation, const float joint_length, vr::VRBoneTransform_t& out_transform)
{
    ComputeBoneTransform(role, orientation, { joint_length, 0.f, 0.f }, out_transform);
}

//-----------------------------------------------------------------------------
// Purpose: Takes an orientation and length of a joint and converts it to a vr::VRBoneTransform_t, but with the orientation applied to the offset
// We have to do convert the quaternion as we use vr::HmdQuaternion_t for our representation, but OpenVR wants vr::HmdQuaternionf_t
//-----------------------------------------------------------------------------
static void ComputeBoneTransformMetacarpal(const vr::ETrackedControllerRole role, const vr::HmdQuaternion_t& orientation, const float joint_length, vr::VRBoneTransform_t& out_transform)
{
    const vr::HmdVector3_t offset = { joint_length, 0.f, 0.f };

    /*
    The Skeletal Input API is designed to be used with common industry tools, such as Maya, to make it easier to move content from 3D editors into VR.
    The way that FBX handles conversion to a different coordinate system is to transform the root bone (wrist), then counter-transform the root's children to account for the root's change, but then
    leave the local coordinate systems of the remaining bones as-is This means that the metacarpals will be rotated 90 degrees from the wrist if trying to build a skeleton programmatically. So, we
    apply this extra rotation to the metacarpals to account for this.
    */
    vr::HmdQuaternion_t magic = { 0.5f, 0.5f, -0.5f, 0.5f };

    vr::HmdQuaternion_t bone_orientation = magic * orientation;

    // Rotate the offset vector by the orientation
    vr::HmdVector3_t bone_position = offset * bone_orientation;

    //"up" axis is flipped between hands, so we need to inverse the x and y axis for the right hand, as all our calculations are based on the left hand currently.
    if (role == vr::TrackedControllerRole_RightHand)
    {
        std::swap(bone_orientation.w, bone_orientation.x);
        std::swap(bone_orientation.y, bone_orientation.z);

        bone_orientation.x *= -1.f;
        bone_orientation.z *= -1.f;
    }

    // pass off to put the position and orientation we've calculated into the skeleton
    ComputeBoneTransform(role, bone_orientation, bone_position, out_transform);
}

//-----------------------------------------------------------------------------
// Purpose: Get the OpenVR bone index from a finger (index=0, middle=1...) and the bone position in the finger. Used in ComputeSkeletalTransforms
//-----------------------------------------------------------------------------
static int CalculateBoneTransformPositionFromFinger(int finger, int bone_in_finger)
{
    const int bone_transform_finger_start_offset = kHandSkeletonBone_IndexFinger0;

    const int result = bone_transform_finger_start_offset + finger * 5 + bone_in_finger;

    return result;
}

//-----------------------------------------------------------------------------
// Purpose: Given the curls and splays, convert this to a vr::VRBoneTransform_t array
//-----------------------------------------------------------------------------
static void ComputeSkeletalTransforms(const HandSimHand& hand, vr::VRBoneTransform_t* out_transforms)
{
    // Do the thumb separately as it's special and not like the other fingers
    ComputeBoneTransformMetacarpal(
        hand.role, SwingTwistToQuaternion(hand.thumb.metacarpal.swing, hand.thumb.metacarpal.twist), finger_joint_lengths[0][0], out_transforms[kHandSkeletonBone_Thumb0]);
    ComputeBoneTransform(hand.role, SwingTwistToQuaternion(hand.thumb.proximal.swing, hand.thumb.metacarpal.twist), finger_joint_lengths[0][1], out_transforms[kHandSkeletonBone_Thumb1]);
    ComputeBoneTransform(hand.role, EulerToQuaternion(0.f, 0.f, hand.thumb.distal.rotation), finger_joint_lengths[0][2], out_transforms[kHandSkeletonBone_Thumb2]);
    ComputeBoneTransform(hand.role, HmdQuaternion_Identity, finger_joint_lengths[0][3], out_transforms[kHandSkeletonBone_Thumb3]);

    // index, middle, ring, pinky
    // We can do these all together as they all require the same calculations
    for (int finger = 0; finger < 4; finger++)
    {
        ComputeBoneTransformMetacarpal(hand.role, SwingTwistToQuaternion(hand.fingers[finger].metacarpal.swing, hand.fingers[finger].metacarpal.twist),
            finger_joint_lengths[finger + 1][0], out_transforms[CalculateBoneTransformPositionFromFinger(finger, 0)]);

        ComputeBoneTransform(hand.role, SwingTwistToQuaternion(hand.fingers[finger].proximal.swing, hand.fingers[finger].proximal.twist), finger_joint_lengths[finger + 1][1],
            out_transforms[CalculateBoneTransformPositionFromFinger(finger, 1)]);

        ComputeBoneTransform(hand.role, EulerToQuaternion(hand.fingers[finger].intermediate.rotation, 0.f, 0.f), finger_joint_lengths[finger + 1][2],
            out_transforms[CalculateBoneTransformPositionFromFinger(finger, 2)]);

        ComputeBoneTransform(hand.role, EulerToQuaternion(hand.fingers[finger].distal.rotation, 0.f, 0.f), finger_joint_lengths[finger + 1][3],
            out_transforms[CalculateBoneTransformPositionFromFinger(finger, 3)]);

        ComputeBoneTransform(hand.role, HmdQuaternion_Identity, finger_joint_lengths[finger + 1][4], out_transforms[CalculateBoneTransformPositionFromFinger(finger, 4)]);
    }
}

void GloveHandSimulation::ComputeSkeletonTransforms(vr::ETrackedControllerRole role, const GloveFingerCurls& curls, const GloveFingerSplays& splays, vr::VRBoneTransform_t* out_transforms)
{
    // This is where we store our internal representation of curls and splays for the hand.
    HandSimHand hand{};

    // Set the handed-ness of the current skeleton (left or right hand)
    hand.role = role;

    // root bone. This is just 0s. It's aligned to /pose/raw.
    out_transforms[0] = { { 0.000000f, 0.000000f, 0.000000f, 1.000000f }, { 1.000000f, -0.000000f, -0.000000f, 0.000000f } };

    // wrist bone. This was taken from the index controller pose.

    out_transforms[1] = { { -0.034038f, 0.036503f, 0.164722f, 1.000000f }, { -0.055147f, -0.078608f, -0.920279f, 0.379296f } };

    //"up" axis is flipped between hands so invert
    if (role == vr::TrackedControllerRole_RightHand)
    {
        out_transforms[1].position.v[0] *= -1.f;

        out_transforms[1].orientation.y *= -1.f;
        out_transforms[1].orientation.z *= -1.f;
    }

    // Initialize a default hand pose
    InitHand(hand);

    // We need to apply the curls and splays separately to the thumb as it's special
    hand.thumb.metacarpal.swing.v[0] += DegToRad(curls.thumb.proximal * 5.f);
    hand.thumb.metacarpal.swing.v[1] += DegToRad(splays.thumb * 5.f);
    hand.thumb.metacarpal.twist = 0.f;

    hand.thumb.proximal.swing.v[0] += DegToRad(curls.thumb.proximal * 90.f);
    hand.thumb.proximal.swing.v[1] += DegToRad(splays.thumb * 20.f);
    hand.thumb.proximal.twist = 0.f;

    hand.thumb.distal.rotation += DegToRad(curls.thumb.distal * 90.f);

    // But we can batch up the fingers with a generic apply function.
    ApplyGenericFingerTransform(curls.index, splays.index, hand.fingers[0]);
    ApplyGenericFingerTransform(curls.middle, splays.middle, hand.fingers[1]);
    ApplyGenericFingerTransform(curls.ring, splays.ring, hand.fingers[2]);
    ApplyGenericFingerTransform(curls.pinky, splays.pinky, hand.fingers[3]);

    // Now compute
    ComputeSkeletalTransforms(hand, out_transforms);
}