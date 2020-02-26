//
// Created by jack on 20-2-25.
//

#include "display.h"
#include "logic.h"
#include <OgreQuaternion.h>

namespace Escape {
    static void setColor(Ogre::Entity *ent, float r, float g, float b) {
        char name[32];
        sprintf(name, "Color(%d,%d,%d)", (int) (r * 255), (int) (g * 255), (int) (b * 255));
        auto material = Ogre::MaterialManager::getSingleton().getByName(name);
        if (!material) {
            material = Ogre::MaterialManager::getSingleton().getByName("white")->clone(name);
            material->setDiffuse(r, g, b, 1);
        }
        ent->setMaterial(material);
    }

    std::pair<Ogre::SceneNode *, Ogre::Entity *>
    DisplayOgre::newBox(float cx, float cy, float width, float height) {
        Ogre::SceneNode *cubeNode = rects->createChildSceneNode();
        cubeNode->setScale(width / 100, height / 100, 1.0 / 100);
        cubeNode->setPosition(cx, cy, 0);

        Ogre::Entity *cube = scnMgr->createEntity("Prefab_Cube");
        cube->setMaterialName("white");

        cubeNode->attachObject(cube);
        return std::make_pair(cubeNode, cube);
    }

    std::pair<Ogre::SceneNode *, Ogre::Entity *> DisplayOgre::newCircle(float cx, float cy, float radius) {
        Ogre::SceneNode *cubeNode = rects->createChildSceneNode();
        cubeNode->setScale(radius / 100, radius / 100, 1.0 / 100);
        cubeNode->setPosition(cx, cy, 0);

        Ogre::Entity *cube = scnMgr->createEntity("Prefab_Sphere");
        cube->setMaterialName("white");

        cubeNode->attachObject(cube);
        return std::make_pair(cubeNode, cube);
    }

    Ogre::Vector3 DisplayOgre::pickUp(unsigned int absoluteX, unsigned int absoluteY) {
        float width = (float) cam->getViewport()->getActualWidth();   // viewport width
        float height = (float) cam->getViewport()->getActualHeight(); // viewport height

        Ogre::Ray ray = cam->getCameraToViewportRay((float) absoluteX / width, (float) absoluteY / height);
        float t = ray.getOrigin().z / ray.getDirection().z;

        return ray.getOrigin() + ray.getDirection() * t;
    }

    void DisplayOgre::initialize() {
        Window::initialize();
        logic = findSystem<Logic>();
        world = logic->getWorld();
        rects = scnMgr->getRootSceneNode()->createChildSceneNode();
        Ogre::ResourceGroupManager::getSingleton().createResourceGroup("Popular");
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation("assets", "FileSystem", "Popular");
        Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Popular");
        Ogre::ResourceGroupManager::getSingleton().loadResourceGroup("Popular");
    }

    void DisplayOgre::processInput() {
        // ESC exit
        WindowOgre::processInput();
        auto player = logic->getPlayer();
        if (player == entt::null)
            return;
        if (input.mouse[OgreBites::BUTTON_LEFT]) {
            auto click = pickUp(input.mouse_x, input.mouse_y);
            double x = click.x, y = click.y;
            // std::cerr << "Cursor: " << x << " " << y << std::endl;
            assert(world->has<Position>(player));
            auto pos = world->get<Position>(player);
            float angle = atan2(y - pos.y, x - pos.x);
            logic->fire(player, angle);
        }

        Velocity vel(0, 0);
        if (input.keys['w'])
            vel.y += 1;
        if (input.keys['s'])
            vel.y += -1;
        if (input.keys['a'])
            vel.x += -1;
        if (input.keys['d'])
            vel.x += 1;
        float spd = glm::length(vel.unwrap());
        if (spd > 0) {
            if (spd > 1) {
                vel /= spd;
            }
            vel *= 6.0f;
        }
        logic->move(player, vel);

        if (input.keys['1'])
            logic->changeWeapon(player, WeaponType::HANDGUN);

        if (input.keys['2'])
            logic->changeWeapon(player, WeaponType::SHOTGUN);

        if (input.keys['3'])
            logic->changeWeapon(player, WeaponType::SMG);

        if (input.keys['4'])
            logic->changeWeapon(player, WeaponType::RIFLE);

        if (input.keys['p'])
            logic->agent_system->createAgent(
                    Position(logic->timeserver->random(-50, 50), logic->timeserver->random(-50, 50)));

        if (input.keys['o']) {
            try {
                std::cerr << "Writing map file" << std::endl;
                SerializationHelper helper("map.txt");
                helper.serialize_ptr(world);
            }
            catch (std::runtime_error &e) {
                std::cerr << "error " << e.what() << std::endl;
            }
            catch (std::exception &e) {
                std::cerr << "error " << e.what() << std::endl;
            }
        }
        if (input.keys['i']) {
            std::cerr << "Reading map file" << std::endl;
            SerializationHelper helper("map.txt");
            helper.deserialize_ptr(world);
        }
    }


    void DisplayOgre::postProcess() {
        rects->removeAndDestroyAllChildren();
    }


    void DisplayOgre::render() {
        // std::cerr << "Render " << logic->timeserver->getTick() << std::endl;
        world->view<AgentData, Position, Health>().each([&](auto ent, auto &agt, auto &pos, auto &health) {
            float percent = health.health / health.max_health;
            auto pair = newCircle(pos.x, pos.y, 2);
            setColor(pair.second, 1 - percent, percent, 0);
            if (world->has<Rotation>(ent)) {
                auto rotation = world->get<Rotation>(ent);
                pair.first->setOrientation(Ogre::Quaternion(Ogre::Radian(rotation.radian), Ogre::Vector3(0, 0, 1)));

            }
        });
        world->view<TerrainData, Position>().each([&](auto ent, auto &ter, auto &pos) {
            auto pair = newBox(pos.x, pos.y, ter.arguments[0], ter.arguments[1]);
            setColor(pair.second, .5, .5, .5);
            if (world->has<Rotation>(ent)) {
                auto rotation = world->get<Rotation>(ent);
                pair.first->setOrientation(Ogre::Quaternion(Ogre::Radian(rotation.radian), Ogre::Vector3(0, 0, 1)));
            }

        });
        world->view<BulletData, Position>().each([&](auto ent, auto &data, auto &pos) {
            newBox(pos.x, pos.y, .2, .2);
        });
    }

    void DisplayOgre::windowResized(int width, int height) {
        std::cerr << "Resized " << width << " " << height << std::endl;
    }

    DisplayOgre::DisplayOgre() : WindowOgre("Escape", 800, 600) {}

}
