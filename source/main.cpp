#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header


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
        auto frame = new tsl::elm::OverlayFrame("tesla-lan-play", "v0.5");

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

        bool is_auto = profile.ip_setting_data.ip_address_setting.is_automatic;
        bool is_lan_play_addr =
            profile.ip_setting_data.ip_address_setting.current_addr.addr[0] == 10 &&
            profile.ip_setting_data.ip_address_setting.current_addr.addr[1] == 13 &&
            compareIp(profile.ip_setting_data.ip_address_setting.subnet_mask.addr, lan_subnet) &&
            compareIp(profile.ip_setting_data.ip_address_setting.gateway.addr, lan_gateway);

        //lan_play Toggler
        auto lan_toggle = new tsl::elm::ToggleListItem("LAN Play", !is_auto && is_lan_play_addr);
        lan_toggle->setStateChangedListener([lan_subnet, lan_gateway](bool state) {
            tsl::hlp::doWithSmSession([state,lan_subnet,lan_gateway]{
                nifmInitialize(NifmServiceType_Admin);
                NifmNetworkProfileData profile;
                nifmGetCurrentNetworkProfile(&profile);

                if (state) {
                    u8 lan_addr[] = {10, 13, rand() % 256, rand() % 253 + 2};

                    setIp(profile.ip_setting_data.ip_address_setting.current_addr.addr, lan_addr);
                    setIp(profile.ip_setting_data.ip_address_setting.subnet_mask.addr, lan_subnet);
                    setIp(profile.ip_setting_data.ip_address_setting.gateway.addr, lan_gateway);

                    profile.ip_setting_data.ip_address_setting.is_automatic = false;
                } else {
                    profile.ip_setting_data.ip_address_setting.is_automatic = true;
                }

                nifmSetNetworkProfile(&profile, &profile.uuid);
                nifmSetWirelessCommunicationEnabled(true); //dispite the name this does work on ethernet

                nifmExit();
                //sleep(3);
            });
        });
        list->addItem(lan_toggle);

        //Debugging crap
        list->addItem(new tsl::elm::CategoryHeader("DEBUG INFO:", true));
        std::string auto_ip;
        if (profile.ip_setting_data.ip_address_setting.is_automatic) {
            auto_ip = "YES";
        } else {
            auto_ip = "NO";
        }
        list->addItem(new tsl::elm::ListItem("Automatic IP: ", auto_ip));
        list->addItem(new tsl::elm::ListItem("Current Address: ", ipToString(profile.ip_setting_data.ip_address_setting.current_addr.addr)));
        list->addItem(new tsl::elm::ListItem("Subnet Mask: ", ipToString(profile.ip_setting_data.ip_address_setting.subnet_mask.addr)));
        list->addItem(new tsl::elm::ListItem("Default Gateway: ", ipToString(profile.ip_setting_data.ip_address_setting.gateway.addr)));

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