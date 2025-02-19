#ifndef GAME_RENDER_ACTORANIMATION_H
#define GAME_RENDER_ACTORANIMATION_H

#include <map>

#include <osg/ref_ptr>

#include "../mwworld/containerstore.hpp"

#include "animation.hpp"

namespace osg
{
    class Node;
}

namespace MWWorld
{
    class ConstPtr;
}

namespace SceneUtil
{
    class LightSource;
    class LightListCallback;
}

namespace MWRender
{

class ActorAnimation : public Animation, public MWWorld::ContainerStoreListener
{
    public:
        ActorAnimation(const MWWorld::Ptr &ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem);
        virtual ~ActorAnimation();

        void itemAdded(const MWWorld::ConstPtr& item, int count) override;
        void itemRemoved(const MWWorld::ConstPtr& item, int count) override;
        virtual bool isArrowAttached() const { return false; }
        bool useShieldAnimations() const override;
        bool updateCarriedLeftVisible(const int weaptype) const override;

        void removeFromScene() override;

    protected:
        osg::Group* getBoneByName(const std::string& boneName) const;
        virtual void updateHolsteredWeapon(bool showHolsteredWeapons);
        virtual void updateHolsteredShield(bool showCarriedLeft);
        virtual void updateQuiver();
        std::string getShieldMesh(const MWWorld::ConstPtr& shield, bool female) const;
        virtual std::string getSheathedShieldMesh(const MWWorld::ConstPtr& shield) const;
        virtual std::string getHolsteredWeaponBoneName(const MWWorld::ConstPtr& weapon);
        virtual PartHolderPtr attachMesh(const std::string& model, const std::string& bonename, bool enchantedGlow, osg::Vec4f* glowColor);
        virtual PartHolderPtr attachMesh(const std::string& model, const std::string& bonename)
        {
            osg::Vec4f stubColor = osg::Vec4f(0,0,0,0);
            return attachMesh(model, bonename, false, &stubColor);
        };
        osg::ref_ptr<osg::Node> attach(const std::string& model, const std::string& bonename, const std::string& bonefilter, bool isLight);

        PartHolderPtr mScabbard;
        PartHolderPtr mHolsteredShield;

    private:
        void addHiddenItemLight(const MWWorld::ConstPtr& item, const ESM::Light* esmLight);
        void removeHiddenItemLight(const MWWorld::ConstPtr& item);
        void resetControllers(osg::Node* node);

        typedef std::map<MWWorld::ConstPtr, osg::ref_ptr<SceneUtil::LightSource> > ItemLightMap;
        ItemLightMap mItemLights;
};

}

#endif
