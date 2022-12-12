#ifndef _POSE_H_
#define _POSE_H_

#include "../math/transform.h"
#include "../math/matrix4.h"
#include <vector>

class Pose
{
protected:
    std::vector<Transform> m_joints;
    std::vector<int> m_parents;

public:
    Pose();
    Pose(const Pose &_pose);
    Pose(size_t _jointsNum);
    Pose &operator=(const Pose &_pose);

    void resizeJoints(size_t _size);
    size_t getJointsSize() const;

    int getParent(size_t _index) const;
    void setParent(size_t _index, size_t _parent);

    Transform getLocalTransfrom(size_t _index) const;
    void setLocalTransfrom(size_t _index, const Transform &_transfrom);

    Transform getGlobalTransform(size_t _index) const;
    Transform operator[](size_t _index) const;

    void getMatrixPalette(std::vector<Matrix4> &_out) const;
};

#endif //_POSE_H_