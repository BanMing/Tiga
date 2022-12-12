#ifndef _JOINT_ANIMATION_TRACK_H_
#define _JOINT_ANIMATION_TRACK_H_

#include "keyframe.h"
#include <vector>
#include <string>
class AnimatedJoint
{
public:
    // std::vector<Keyframe> m_keyframes;
    std::string m_jointName;
    size_t m_index;
};
#endif //_JOINT_ANIMATION_TRACK_H_