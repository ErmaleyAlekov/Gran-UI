#ifndef GRAN_APP_INTERNAL_PROFILE_TRANSFER_HPP
#define GRAN_APP_INTERNAL_PROFILE_TRANSFER_HPP

#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    struct ProfileTransfer
    {
        ui::Node* child;
        ui::Node* parent;
        std::size_t slot = 0;
    };

    ui::Node* profileHost(ui::Node& parent, ui::Node& root);
    void prepareProfileSlots(std::vector<ProfileTransfer>& plan, AppWindow& next, const ui::Node* focus);
}
#endif
