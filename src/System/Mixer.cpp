#include <System/Mixer.h>
#include <System/Debug.h>

#include <SDL3/SDL.h>

#include <cstring>
#include <atomic>

namespace Vortex {
static const int MIXER_CHANNELS = 2;
static const int MIXER_BLOCK_FRAMES = 8192;
static const int MIXER_BLOCK_SIZE =
    sizeof(short) * MIXER_CHANNELS * MIXER_BLOCK_FRAMES;

struct MixerImpl : public Mixer {
    SDL_AudioStream* stream = nullptr;
    MixSource* source = nullptr;
    int sampleRate = 0;
    std::atomic<bool> isPaused{true};
    std::atomic<bool> isOpened{false};
    short mixBuffer[MIXER_CHANNELS * MIXER_BLOCK_FRAMES];

    ~MixerImpl() override { close(); }

    static void SDLCALL audioCallback(void* userdata, SDL_AudioStream* astream,
                                      int additional_amount, int total_amount) {
        auto* mixer = static_cast<MixerImpl*>(userdata);
        if (mixer->isPaused.load() || !mixer->source || additional_amount <= 0)
            return;

        int framesNeeded = additional_amount / (sizeof(short) * MIXER_CHANNELS);
        while (framesNeeded > 0) {
            int framesToWrite = (framesNeeded < MIXER_BLOCK_FRAMES)
                                    ? framesNeeded
                                    : MIXER_BLOCK_FRAMES;
            mixer->source->writeFrames(mixer->mixBuffer, framesToWrite);
            SDL_PutAudioStreamData(
                astream, mixer->mixBuffer,
                framesToWrite * sizeof(short) * MIXER_CHANNELS);
            framesNeeded -= framesToWrite;
        }
    }

    bool open(MixSource* src, int samplerate) override {
        if (isOpened.load()) close();

        source = src;
        sampleRate = samplerate;

        SDL_AudioSpec spec;
        spec.freq = samplerate;
        spec.format = SDL_AUDIO_S16;
        spec.channels = MIXER_CHANNELS;

        stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                           &spec, audioCallback, this);
        if (!stream) {
            HudError("Failed to open SDL3 audio: %s", SDL_GetError());
            return false;
        }

        isOpened.store(true);
        isPaused.store(true);
        return true;
    }

    void close() override {
        if (stream) {
            SDL_DestroyAudioStream(stream);
            stream = nullptr;
        }
        source = nullptr;
        isOpened.store(false);
        isPaused.store(true);
    }

    void pause() override {
        if (isOpened.load() && !isPaused.load()) {
            isPaused.store(true);
            if (stream) {
                SDL_PauseAudioStreamDevice(stream);
            }
        }
    }

    void resume() override {
        if (isOpened.load() && isPaused.load()) {
            isPaused.store(false);
            if (stream) {
                SDL_ResumeAudioStreamDevice(stream);
            }
        }
    }
};

Mixer* Mixer::create() { return new MixerImpl; }

Mixer::~Mixer() = default;

}  // namespace Vortex
