#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

#include <filesystem>
#include <fstream>


class GuiTest : public tsl::Gui {
public:
    GuiTest() { }

    static bool compareIp(const u8* ip1, const u8* ip2) {
        for (int i = 0; i < 4; i++) {
            if (ip1[i] != ip2[i]) {
                return false;
            }
        }

        return true;
    }

    static void setIp(u8* ip1, const u8* ip2) {
        for (int i = 0; i < 4; i++) {
            ip1[i] = ip2[i];
        }
    }

    std::string ipToString(u8* ip) {
        std::string out = "";
        for (size_t i = 0; i < 3; i++) {
            out += std::to_string(unsigned(ip[i]));
            out += ".";
        }
        out += std::to_string(unsigned(ip[3]));
        return out;
    }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("tesla-lan-play", "v1.0.0");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        //Get current network info
        NifmNetworkProfileData profile;
        tsl::hlp::doWithSmSession([&profile]{
            nifmInitialize(NifmServiceType_User);
            nifmGetCurrentNetworkProfile(&profile);
            nifmExit();
        });

        //Lan play required settings
        u8 lan_subnet[] = {255,255,0,0};
        u8 lan_gateway[] = {10,13,37,1};

        tsl::hlp::doWithSDCardHandle([]{
            std::filesystem::create_directories("/config/tesla-lan-play");
        });
        
        //if any of these are false, lan play isn't on, it's just some other weird ip configuration
        bool is_lan_play_active =
            !profile.ip_setting_data.ip_address_setting.is_automatic &&
            profile.ip_setting_data.ip_address_setting.current_addr.addr[0] == 10 &&
            profile.ip_setting_data.ip_address_setting.current_addr.addr[1] == 13 &&
            compareIp(profile.ip_setting_data.ip_address_setting.subnet_mask.addr, lan_subnet) &&
            compareIp(profile.ip_setting_data.ip_address_setting.gateway.addr, lan_gateway);

        //lan_play Toggler
        auto lan_toggle = new tsl::elm::ToggleListItem("LAN Play", is_lan_play_active);
        lan_toggle->setStateChangedListener([lan_subnet, lan_gateway](bool state) {
            tsl::hlp::doWithSmSession([state,lan_subnet,lan_gateway]{
                nifmInitialize(NifmServiceType_Admin);
                NifmNetworkProfileData profile;
                nifmGetCurrentNetworkProfile(&profile);

                if (state) {
                    //Turn on lan play
                    u8 lan_addr[] = {10, 13, rand() % 256, rand() % 253 + 2};

                    if (!profile.ip_setting_data.ip_address_setting.is_automatic) {
                        //Store static ip info to binary file to avoid text encoding
                        tsl::hlp::doWithSDCardHandle([&profile]{
                            std::ofstream out("/config/tesla-lan-play/ip", std::ios::binary);
                            out.write((char*)profile.ip_setting_data.ip_address_setting.current_addr.addr, 4);
                            out.write((char*)profile.ip_setting_data.ip_address_setting.subnet_mask.addr, 4);
                            out.write((char*)profile.ip_setting_data.ip_address_setting.gateway.addr, 4);
                            out.close();
                        });
                    }

                    setIp(profile.ip_setting_data.ip_address_setting.current_addr.addr, lan_addr);
                    setIp(profile.ip_setting_data.ip_address_setting.subnet_mask.addr, lan_subnet);
                    setIp(profile.ip_setting_data.ip_address_setting.gateway.addr, lan_gateway);

                    profile.ip_setting_data.ip_address_setting.is_automatic = false;
                } else {
                    tsl::hlp::doWithSDCardHandle([&profile]{
                        if (std::filesystem::exists("/config/tesla-lan-play/ip")) {
                            //Restore IP config if it's stored
                            std::ifstream in("/config/tesla-lan-play/ip", std::ios::binary);
                            in.read((char*)profile.ip_setting_data.ip_address_setting.current_addr.addr, 4);
                            in.read((char*)profile.ip_setting_data.ip_address_setting.subnet_mask.addr, 4);
                            in.read((char*)profile.ip_setting_data.ip_address_setting.gateway.addr, 4);
                            in.close();
                            std::filesystem::remove("/config/tesla-lan-play/ip");

                            profile.ip_setting_data.ip_address_setting.is_automatic = false;
                        } else {
                            profile.ip_setting_data.ip_address_setting.is_automatic = true;
                        }
                    });
                }

                nifmSetNetworkProfile(&profile, &profile.uuid);
                nifmSetWirelessCommunicationEnabled(true); //dispite the name this does work on ethernet

                nifmExit();
                //sleep(3);
            });
        });
        list->addItem(lan_toggle);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);
        
        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        return false;   // Return true here to signal the inputs have been consumed
    }
};

class OverlayTest : public tsl::Overlay {
public:
                                             // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {}  // Called at the start to initialize all services necessary for this Overlay
    virtual void exitServices() override {}  // Callet at the end to clean up all services previously initialized

    virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<GuiTest>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}