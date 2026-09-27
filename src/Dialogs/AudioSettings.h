#pragma once

#include <Dialogs/Dialog.h>

namespace Vortex {

class WgLabel;

class DialogAudioSettings : public EditorDialog {
   public:
    DialogAudioSettings();

    void onTick() override;
    void onAction();
    void onAdjust(int deltaMs);

   private:
    void updateOffsetLabel();

    int audioOffsetMs_ = 0;
    double sliderOffsetMs_ = 0.0;
    WgLabel* offsetLabel_ = nullptr;
};

};  // namespace Vortex
