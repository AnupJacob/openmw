#include "effectmanager.hpp"

#include <osg/PositionAttitudeTransform>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/controller.hpp>

#include "animation.hpp"
#include "vismask.hpp"
#include "util.hpp"

#include <algorithm>

namespace MWRender
{

EffectManager::EffectManager(osg::ref_ptr<osg::Group> parent, Resource::ResourceSystem* resourceSystem)
    : mParentNode(parent)
    , mResourceSystem(resourceSystem)
{
}

EffectManager::~EffectManager()
{
    clear();
}

void EffectManager::addEffect(const std::string &model, const std::string& textureOverride, const osg::Vec3f &worldPosition, float scale, bool isMagicVFX)
{
    osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->getInstance(model);

    node->setNodeMask(Mask_Effect);

    Effect effect;
    effect.mAnimTime = std::make_shared<EffectAnimationTime>();

    SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
    node->accept(findMaxLengthVisitor);
    effect.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();

    osg::ref_ptr<osg::PositionAttitudeTransform> trans = new osg::PositionAttitudeTransform;
    trans->setPosition(worldPosition);
    trans->setScale(osg::Vec3f(scale, scale, scale));
    trans->addChild(node);

    effect.mTransform = trans;

    SceneUtil::AssignControllerSourcesVisitor assignVisitor(effect.mAnimTime);
    node->accept(assignVisitor);

    if (isMagicVFX)
        overrideFirstRootTexture(textureOverride, mResourceSystem, node);
    else
        overrideTexture(textureOverride, mResourceSystem, node);

    mParentNode->addChild(trans);

    mEffects.push_back(std::move(effect));
}

void EffectManager::update(float dt)
{
    mEffects.erase(
        std::remove_if(
            mEffects.begin(), 
            mEffects.end(), 
            [dt](Effect& effect)
            {
                effect.mAnimTime->addTime(dt);
                return effect.mAnimTime->getTime() >= effect.mMaxControllerLength;
            }), 
        mEffects.end()
    );
}

void EffectManager::clear()
{
    for(const auto& effect : mEffects)
    {
        mParentNode->removeChild(effect.mTransform);
    }
    mEffects.clear();
}

}
