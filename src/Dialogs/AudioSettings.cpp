#include <Dialogs/AudioSettings.h>

#include <Core/Widgets.h>
#include <Editor/Music.h>

#include <cmath>
#include <string>

namespace Vortex {

DialogAudioSettings::DialogAudioSettings() {
    setTitle("AUDIO OFFSET");

    audioOffsetMs_ = gMusic->getAudioOffsetMs();
    sliderOffsetMs_ = audioOffsetMs_;

    myLayout.row().col(216);
    offsetLabel_ = myLayout.add<WgLabel>();
    updateOffsetLabel();

    myLayout.row().col(216);
    WgSlider* offset = myLayout.add<WgSlider>();
    offset->value.bind(&sliderOffsetMs_);
    offset->setRange(-1000, 1000);
    offset->onChange.bind(this, &DialogAudioSettings::onAction);
    offset->setTooltip("Adjust the playback offset from -1000 to +1000 ms");

    myLayout.row().col(76).col(24).col(24).col(76);
    myLayout.addBlank();
    WgButton* decrease = myLayout.add<WgButton>();
    decrease->text.set("-");
    decrease->onPress.bind(this, &DialogAudioSettings::onAdjust, -1);
    decrease->setTooltip("Decrease the offset by 1 ms");

    WgButton* increase = myLayout.add<WgButton>();
    increase->text.set("+");
    increase->onPress.bind(this, &DialogAudioSettings::onAdjust, 1);
    increase->setTooltip("Increase the offset by 1 ms");
    myLayout.addBlank();
}

void DialogAudioSettings::onTick() {
    audioOffsetMs_ = gMusic->getAudioOffsetMs();
    sliderOffsetMs_ = audioOffsetMs_;
    updateOffsetLabel();
    EditorDialog::onTick();
}

void DialogAudioSettings::onAction() {
    gMusic->setAudioOffsetMs(static_cast<int>(std::lround(sliderOffsetMs_)));
    audioOffsetMs_ = gMusic->getAudioOffsetMs();
    sliderOffsetMs_ = audioOffsetMs_;
    updateOffsetLabel();
}

void DialogAudioSettings::onAdjust(int deltaMs) {
    gMusic->setAudioOffsetMs(gMusic->getAudioOffsetMs() + deltaMs);
    audioOffsetMs_ = gMusic->getAudioOffsetMs();
    sliderOffsetMs_ = audioOffsetMs_;
    updateOffsetLabel();
}

void DialogAudioSettings::updateOffsetLabel() {
    std::string value = std::to_string(audioOffsetMs_);
    if (audioOffsetMs_ > 0) value.insert(value.begin(), '+');
    offsetLabel_->text.set(value + " ms");
}

};  // namespace Vortex
