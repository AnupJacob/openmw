#ifndef OPENMW_MWRENDER_EFFECTMANAGER_H
#define OPENMW_MWRENDER_EFFECTMANAGER_H

#include <memory>
#include <string>
#include <vector>

#include <osg/ref_ptr>

namespace osg
{
    class Group;
    class Vec3f;
    class PositionAttitudeTransform;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    class EffectAnimationTime;

    // Note: effects attached to another object should be managed by MWRender::Animation::addEffect.
    // This class manages "free" effects, i.e. attached to a dedicated scene node in the world.
    class EffectManager
    {
    public:
        EffectManager(osg::ref_ptr<osg::Group> parent, Resource::ResourceSystem* resourceSystem);
        EffectManager(const EffectManager&) = delete;
        ~EffectManager();

        /// Add an effect. When it's finished playing, it will be removed automatically.
        void addEffect (const std::string& model, const std::string& textureOverride, const osg::Vec3f& worldPosition, float scale, bool isMagicVFX = true);

        void update(float dt);

        /// Remove all effects
        void clear();

    private:
        struct Effect
        {
            float mMaxControllerLength;
            std::shared_ptr<EffectAnimationTime> mAnimTime;
            osg::ref_ptr<osg::PositionAttitudeTransform> mTransform;
        };

        std::vector<Effect> mEffects;

        osg::ref_ptr<osg::Group> mParentNode;
        Resource::ResourceSystem* mResourceSystem;
    };

}

#endif
