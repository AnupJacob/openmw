#include "actionmanager.hpp"

#include <MyGUI_InputManager.h>

#include <SDL_keyboard.h>

#include <components/settings/settings.hpp>

#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwgui/messagebox.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"

namespace MWInput
{

    ActionManager::ActionManager(BindingsManager* bindingsManager,
            osgViewer::ScreenCaptureHandler::CaptureOperation* screenCaptureOperation,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler)
        : mBindingsManager(bindingsManager)
        , mViewer(viewer)
        , mScreenCaptureHandler(screenCaptureHandler)
        , mScreenCaptureOperation(screenCaptureOperation)
        , mAlwaysRunActive(Settings::Manager::getBool("always run", "Input"))
        , mSneaking(false)
        , mAttemptJump(false)
        , mTimeIdle(0.f)
    {
    }

    void ActionManager::update(float dt, bool triedToMove)
    {
        // Disable movement in Gui mode
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running)
        {
            mAttemptJump = false;
            return;
        }

        // Configure player movement according to keyboard input. Actual movement will
        // be done in the physics system.
        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            bool alwaysRunAllowed = false;

            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();

            if (mBindingsManager->actionIsActive(A_MoveLeft) != mBindingsManager->actionIsActive(A_MoveRight))
            {
                alwaysRunAllowed = true;
                triedToMove = true;
                player.setLeftRight(mBindingsManager->actionIsActive(A_MoveRight) ? 1 : -1);
            }

            if (mBindingsManager->actionIsActive(A_MoveForward) != mBindingsManager->actionIsActive(A_MoveBackward))
            {
                alwaysRunAllowed = true;
                triedToMove = true;
                player.setAutoMove (false);
                player.setForwardBackward(mBindingsManager->actionIsActive(A_MoveForward) ? 1 : -1);
            }

            if (player.getAutoMove())
            {
                alwaysRunAllowed = true;
                triedToMove = true;
                player.setForwardBackward (1);
            }

            if (mAttemptJump && MWBase::Environment::get().getInputManager()->getControlSwitch("playerjumping"))
            {
                player.setUpDown(1);
                triedToMove = true;
            }

            // if player tried to start moving, but can't (due to being overencumbered), display a notification.
            if (triedToMove)
            {
                MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld ()->getPlayerPtr();
                if (playerPtr.getClass().getEncumbrance(playerPtr) > playerPtr.getClass().getCapacity(playerPtr))
                {
                    player.setAutoMove (false);
                    std::vector<MWGui::MessageBox*> msgboxs = MWBase::Environment::get().getWindowManager()->getActiveMessageBoxes();
                    const std::vector<MWGui::MessageBox*>::iterator it = std::find_if(msgboxs.begin(), msgboxs.end(), [](MWGui::MessageBox*& msgbox)
                    {
                        return (msgbox->getMessage() == "#{sNotifyMessage59}");
                    });

                    // if an overencumbered messagebox is already present, reset its expiry timer, otherwise create new one.
                    if (it != msgboxs.end())
                        (*it)->mCurrentTime = 0;
                    else
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage59}");
                }
            }

            if (triedToMove)
                MWBase::Environment::get().getInputManager()->resetIdleTime();

            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (!isToggleSneak)
            {
                if(!MWBase::Environment::get().getInputManager()->joystickLastUsed())
                    player.setSneak(mBindingsManager->actionIsActive(A_Sneak));
            }

            float xAxis = mBindingsManager->getActionValue(A_MoveLeftRight);
            float yAxis = mBindingsManager->getActionValue(A_MoveForwardBackward);
            bool isRunning = osg::Vec2f(xAxis * 2 - 1, yAxis * 2 - 1).length2() > 0.25f;
            if ((mAlwaysRunActive && alwaysRunAllowed) || isRunning)
                player.setRunState(!mBindingsManager->actionIsActive(A_Run));
            else
                player.setRunState(mBindingsManager->actionIsActive(A_Run));
        }

        if (mBindingsManager->actionIsActive(A_MoveForward) ||
            mBindingsManager->actionIsActive(A_MoveBackward) ||
            mBindingsManager->actionIsActive(A_MoveLeft) ||
            mBindingsManager->actionIsActive(A_MoveRight) ||
            mBindingsManager->actionIsActive(A_Jump) ||
            mBindingsManager->actionIsActive(A_Sneak) ||
            mBindingsManager->actionIsActive(A_TogglePOV) ||
            mBindingsManager->actionIsActive(A_ZoomIn) ||
            mBindingsManager->actionIsActive(A_ZoomOut))
        {
            resetIdleTime();
        }
        else
            mTimeIdle += dt;

        mAttemptJump = false;
    }

    void ActionManager::resetIdleTime()
    {
        mTimeIdle = 0.f;
    }

    void ActionManager::executeAction(int action)
    {
        MWBase::Environment::get().getLuaManager()->inputEvent({MWBase::LuaManager::InputEvent::Action, action});
        const auto inputManager = MWBase::Environment::get().getInputManager();
        const auto windowManager = MWBase::Environment::get().getWindowManager();
        // trigger action activated
        switch (action)
        {
        case A_GameMenu:
            toggleMainMenu ();
            break;
        case A_Screenshot:
            screenshot();
            break;
        case A_Inventory:
            toggleInventory ();
            break;
        case A_Console:
            toggleConsole ();
            break;
        case A_Activate:
            inputManager->resetIdleTime();
            activate();
            break;
        case A_MoveLeft:
        case A_MoveRight:
        case A_MoveForward:
        case A_MoveBackward:
            handleGuiArrowKey(action);
            break;
        case A_Journal:
            toggleJournal();
            break;
        case A_AutoMove:
            toggleAutoMove();
            break;
        case A_AlwaysRun:
            toggleWalking();
            break;
        case A_ToggleWeapon:
            toggleWeapon();
            break;
        case A_Rest:
            rest();
            break;
        case A_ToggleSpell:
            toggleSpell();
            break;
        case A_QuickKey1:
            quickKey(1);
            break;
        case A_QuickKey2:
            quickKey(2);
            break;
        case A_QuickKey3:
            quickKey(3);
            break;
        case A_QuickKey4:
            quickKey(4);
            break;
        case A_QuickKey5:
            quickKey(5);
            break;
        case A_QuickKey6:
            quickKey(6);
            break;
        case A_QuickKey7:
            quickKey(7);
            break;
        case A_QuickKey8:
            quickKey(8);
            break;
        case A_QuickKey9:
            quickKey(9);
            break;
        case A_QuickKey10:
            quickKey(10);
            break;
        case A_QuickKeysMenu:
            showQuickKeysMenu();
            break;
        case A_ToggleHUD:
            windowManager->toggleHud();
            break;
        case A_ToggleDebug:
            windowManager->toggleDebugWindow();
            break;
        case A_TogglePostProcessorHUD:
            windowManager->togglePostProcessorHud();
            break;
        case A_QuickSave:
            quickSave();
            break;
        case A_QuickLoad:
            quickLoad();
            break;
        case A_CycleSpellLeft:
            if (checkAllowedToUseItems() && windowManager->isAllowed(MWGui::GW_Magic))
                MWBase::Environment::get().getWindowManager()->cycleSpell(false);
            break;
        case A_CycleSpellRight:
            if (checkAllowedToUseItems() && windowManager->isAllowed(MWGui::GW_Magic))
                MWBase::Environment::get().getWindowManager()->cycleSpell(true);
            break;
        case A_CycleWeaponLeft:
            if (checkAllowedToUseItems() && windowManager->isAllowed(MWGui::GW_Inventory))
                MWBase::Environment::get().getWindowManager()->cycleWeapon(false);
            break;
        case A_CycleWeaponRight:
            if (checkAllowedToUseItems() && windowManager->isAllowed(MWGui::GW_Inventory))
                MWBase::Environment::get().getWindowManager()->cycleWeapon(true);
            break;
        case A_Sneak:
            static const bool isToggleSneak = Settings::Manager::getBool("toggle sneak", "Input");
            if (isToggleSneak)
            {
                toggleSneaking();
            }
            break;
        }
    }

    bool ActionManager::checkAllowedToUseItems() const
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            // Cannot use items or spells while in werewolf form
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return false;
        }
        return true;
    }

    void ActionManager::screenshot()
    {
        const std::string& settingStr = Settings::Manager::getString("screenshot type", "Video");
        bool regularScreenshot = settingStr.size() == 0 || settingStr.compare("regular") == 0;

        if (regularScreenshot)
        {
            mScreenCaptureHandler->setFramesToCapture(1);
            mScreenCaptureHandler->captureNextFrame(*mViewer);
        }
        else
        {
            osg::ref_ptr<osg::Image> screenshot (new osg::Image);

            if (MWBase::Environment::get().getWorld()->screenshot360(screenshot.get()))
            {
                (*mScreenCaptureOperation) (*(screenshot.get()), 0);
                // FIXME: mScreenCaptureHandler->getCaptureOperation() causes crash for some reason
            }
        }
    }

    void ActionManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance().isModalAny())
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            return;
        }

        if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
        {
            MWBase::Environment::get().getWindowManager()->toggleConsole();
            return;
        }

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode()) //No open GUIs, open up the MainMenu
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
        else //Close current GUI
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        }
    }

    void ActionManager::toggleSpell()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the magic window is accessible
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playermagic") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        if (!checkAllowedToUseItems())
            return;

        // Not allowed if no spell selected
        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        MWWorld::InventoryStore& inventory = player.getPlayer().getClass().getInventoryStore(player.getPlayer());
        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell().empty() &&
            inventory.getSelectedEnchantItem() == inventory.end())
            return;

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player.getPlayer()))
            return;

        MWMechanics::DrawState state = player.getDrawState();
        if (state == MWMechanics::DrawState::Weapon || state == MWMechanics::DrawState::Nothing)
            player.setDrawState(MWMechanics::DrawState::Spell);
        else
            player.setDrawState(MWMechanics::DrawState::Nothing);
    }

    void ActionManager::quickLoad()
    {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickLoad();
    }

    void ActionManager::quickSave()
    {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickSave();
    }

    void ActionManager::toggleWeapon()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the inventory window is accessible
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playerfighting") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        // We want to interrupt animation only if attack is preparing, but still is not triggered
        // Otherwise we will get a "speedshooting" exploit, when player can skip reload animation by hitting "Toggle Weapon" key twice
        if (MWBase::Environment::get().getMechanicsManager()->isAttackPreparing(player.getPlayer()))
            player.setAttackingOrSpell(false);
        else if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player.getPlayer()))
            return;

        MWMechanics::DrawState state = player.getDrawState();
        if (state == MWMechanics::DrawState::Spell || state == MWMechanics::DrawState::Nothing)
            player.setDrawState(MWMechanics::DrawState::Weapon);
        else
            player.setDrawState(MWMechanics::DrawState::Nothing);
    }

    void ActionManager::rest()
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        if (!MWBase::Environment::get().getWindowManager()->getRestEnabled() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Rest); //Open rest GUI
    }

    void ActionManager::toggleInventory()
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;

        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        if (MWBase::Environment::get().getWindowManager()->isConsoleMode())
            return;

        // Toggle between game mode and inventory mode
        if(!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Inventory);
        else
        {
            MWGui::GuiMode mode = MWBase::Environment::get().getWindowManager()->getMode();
            if(mode == MWGui::GM_Inventory || mode == MWGui::GM_Container)
                MWBase::Environment::get().getWindowManager()->popGuiMode();
        }

        // .. but don't touch any other mode, except container.
    }

    void ActionManager::toggleConsole()
    {
        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        MWBase::Environment::get().getWindowManager()->toggleConsole();
    }

    void ActionManager::toggleJournal()
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
            return;
        if (MyGUI::InputManager::getInstance ().isModalAny())
            return;

        MWBase::WindowManager* windowManager = MWBase::Environment::get().getWindowManager();
        if (windowManager->getMode() != MWGui::GM_Journal
                && windowManager->getMode() != MWGui::GM_MainMenu
                && windowManager->getMode() != MWGui::GM_Settings
                && windowManager->getJournalAllowed())
        {
            windowManager->pushGuiMode(MWGui::GM_Journal);
        }
        else if (windowManager->containsMode(MWGui::GM_Journal))
        {
            windowManager->removeGuiMode(MWGui::GM_Journal);
        }
    }

    void ActionManager::quickKey (int index)
    {
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playerfighting") || !MWBase::Environment::get().getInputManager()->getControlSwitch("playermagic"))
            return;
        if (!checkAllowedToUseItems())
            return;

        if (MWBase::Environment::get().getWorld()->getGlobalFloat ("chargenstate")!=-1)
            return;

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->activateQuickKey (index);
    }

    void ActionManager::showQuickKeysMenu()
    {
        if (MWBase::Environment::get().getWindowManager()->getMode () == MWGui::GM_QuickKeysMenu)
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
            return;
        }

        if (MWBase::Environment::get().getWorld()->getGlobalFloat ("chargenstate") != -1)
            return;

        if (!checkAllowedToUseItems())
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_QuickKeysMenu);
    }

    void ActionManager::activate()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            bool joystickUsed = MWBase::Environment::get().getInputManager()->joystickLastUsed();
            if (!SDL_IsTextInputActive() && !mBindingsManager->isLeftOrRightButton(A_Activate, joystickUsed))
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Return, 0, false);
        }
        else if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
            player.activate();
        }
    }

    void ActionManager::toggleAutoMove()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        if (MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols"))
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
            player.setAutoMove (!player.getAutoMove());
        }
    }

    void ActionManager::toggleWalking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode() || SDL_IsTextInputActive()) return;
        mAlwaysRunActive = !mAlwaysRunActive;

        Settings::Manager::setBool("always run", "Input", mAlwaysRunActive);
    }

    void ActionManager::toggleSneaking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;
        if (!MWBase::Environment::get().getInputManager()->getControlSwitch("playercontrols")) return;
        mSneaking = !mSneaking;
        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        player.setSneak(mSneaking);
    }

    void ActionManager::handleGuiArrowKey(int action)
    {
        bool joystickUsed = MWBase::Environment::get().getInputManager()->joystickLastUsed();
        // This is currently keyboard-specific code
        // TODO: see if GUI controls can be refactored into a single function
        if (joystickUsed)
            return;

        if (SDL_IsTextInputActive())
            return;

        if (mBindingsManager->isLeftOrRightButton(action, joystickUsed))
            return;

        MyGUI::KeyCode key;
        switch (action)
        {
        case A_MoveLeft:
            key = MyGUI::KeyCode::ArrowLeft;
            break;
        case A_MoveRight:
            key = MyGUI::KeyCode::ArrowRight;
            break;
        case A_MoveForward:
            key = MyGUI::KeyCode::ArrowUp;
            break;
        case A_MoveBackward:
        default:
            key = MyGUI::KeyCode::ArrowDown;
            break;
        }

        MWBase::Environment::get().getWindowManager()->injectKeyPress(key, 0, false);
    }
}
