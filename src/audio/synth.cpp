#include "audio/synth.h"

#include <cmath>
#include <initializer_list>

namespace sisyphus::audio {

namespace {

constexpr float kTwoPi = 6.2831853f;
constexpr float kDt = 1.0f / static_cast<float>(kRate);
constexpr float kGlide = 1.0f / (0.5f * static_cast<float>(kRate));  // the steady sounds' glide, per sample

// A small deterministic noise source, so the one-off sounds render the same
// every time.
struct Rng {
    uint32_t s;
    float next() {  // -1..1
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<float>(s >> 8) / 8388608.0f - 1.0f;
    }
    float between(float a, float b) { return a + (b - a) * (next() * 0.5f + 0.5f); }
};

// One-pole low-pass filter coefficient for a cutoff in Hz.
float pole(float hz) { return 1.0f - std::exp(-kTwoPi * hz * kDt); }

int samples(float seconds) { return static_cast<int>(seconds * static_cast<float>(kRate)); }
float at(size_t i) { return static_cast<float>(i) * kDt; }
float attack(float t, float seconds) { return t < seconds ? t / seconds : 1.0f; }

// Soft tones starting `apart` seconds apart, each with a little of its octave.
std::vector<float> chime(std::initializer_list<float> notes, float apart, float tau, float seconds, float gain = 0.22f) {
    std::vector<float> out(static_cast<size_t>(samples(seconds)));
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(i);
        float s = 0, start = 0;
        for (float hz : notes) {
            if (t >= start) {
                const float u = t - start;
                s += attack(u, 0.005f) * std::exp(-u / tau) * (std::sin(kTwoPi * hz * u) + 0.3f * std::sin(kTwoPi * 2 * hz * u));
            }
            start += apart;
        }
        out[i] = gain * s;
    }
    return out;
}

// A struck bell: inharmonic partials, the higher ones dying first.
std::vector<float> bell(float hz, float seconds) {
    static const float ratio[] = {1.0f, 2.0f, 2.76f, 5.4f};
    static const float amp[] = {1.0f, 0.6f, 0.4f, 0.2f};
    static const float tau[] = {2.0f, 1.3f, 0.9f, 0.4f};
    std::vector<float> out(static_cast<size_t>(samples(seconds)));
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(i);
        float s = 0;
        for (int k = 0; k < 4; ++k) s += amp[k] * std::exp(-t / tau[k]) * std::sin(kTwoPi * hz * ratio[k] * t);
        out[i] = 0.25f * attack(t, 0.004f) * s;
    }
    return out;
}

// Short beeps, like a pager: `notes` one after another.
std::vector<float> beeps(std::initializer_list<float> notes, float each, float gap, float gain) {
    const float seconds = static_cast<float>(notes.size()) * (each + gap) + 0.05f;
    std::vector<float> out(static_cast<size_t>(samples(seconds)));
    float start = 0;
    for (float hz : notes) {
        for (size_t i = static_cast<size_t>(samples(start)); i < out.size(); ++i) {
            const float u = at(i) - start;
            if (u >= each) break;
            const float env = std::sin(3.14159265f * u / each);
            out[i] += gain * env * (std::sin(kTwoPi * hz * u) + 0.2f * std::sin(kTwoPi * 3 * hz * u));
        }
        start += each + gap;
    }
    return out;
}

// Low noise, its cutoff and its loudness falling over `seconds`: stone on stone.
std::vector<float> rumble(Rng& r, float seconds, float fromHz, float toHz, float gain) {
    std::vector<float> out(static_cast<size_t>(samples(seconds)));
    float lo = 0, lo2 = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(i), k = t / seconds;
        const float hz = fromHz + (toHz - fromHz) * k;
        lo += pole(hz) * (r.next() - lo);
        lo2 += pole(hz) * (lo - lo2);
        out[i] = gain * attack(t, 0.08f) * (1 - k) * lo2 * 6.0f;
    }
    return out;
}

std::vector<float> down(Rng& r) {
    // A low, beating thud: two tones a few hertz apart, and a knock.
    std::vector<float> out(static_cast<size_t>(samples(1.6f)));
    float lo = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(i);
        lo += pole(200) * (r.next() - lo);
        out[i] = 0.35f * std::exp(-t / 0.55f) * (std::sin(kTwoPi * 98 * t) + std::sin(kTwoPi * 104 * t)) +
                 1.5f * lo * std::exp(-t / 0.05f);
    }
    return out;
}

std::vector<float> patched(Rng& r) {
    // A click, then a small bright chime.
    std::vector<float> out = chime({1318.5f, 1760.0f}, 0.07f, 0.25f, 0.6f, 0.16f);
    for (size_t i = 0; i < out.size() && i < static_cast<size_t>(samples(0.02f)); ++i)
        out[i] += 0.5f * r.next() * std::exp(-at(i) / 0.004f);
    return out;
}

// A footstep on packed dirt: a soft, low thud with a faint scuff, quiet
// beside the wind.
std::vector<float> step(Rng& r) {
    std::vector<float> out(static_cast<size_t>(samples(0.14f)));
    float lo = 0, mid = 0, mid2 = 0;
    const float tone = r.between(0.85f, 1.15f);
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(i), n = r.next();
        lo += pole(180 * tone) * (n - lo);
        mid += pole(1400 * tone) * (n - mid);
        mid2 += pole(700 * tone) * (n - mid2);
        const float thud = 0.45f * lo + 0.03f * std::sin(kTwoPi * 70 * tone * t);
        const float scuff = 0.04f * (mid - mid2) * std::exp(-t / 0.012f);
        out[i] = (thud * std::exp(-t / 0.04f) + scuff) * attack(t, 0.006f);
    }
    return out;
}

// A cricket: a few quick pulses of a high tone.
std::vector<float> chirp(Rng& r) {
    const float hz = r.between(4200, 4900);
    std::vector<float> out(static_cast<size_t>(samples(0.16f)));
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(i);
        const float pulse = std::sin(3.14159265f * std::fmod(t, 0.04f) / 0.04f);
        out[i] = 0.05f * pulse * pulse * std::sin(kTwoPi * hz * t);
    }
    return out;
}

}  // namespace

Synth::Synth() {
    Rng r{0x5157u};
    auto& c = cues_;
    c[static_cast<size_t>(Cue::Release)] = chime({523.3f, 659.3f, 784.0f}, 0.13f, 0.6f, 1.6f);
    std::vector<float> summit = bell(261.6f, 3.5f);
    const std::vector<float> rise = chime({523.3f, 659.3f, 784.0f, 1046.5f}, 0.18f, 0.9f, 2.6f);
    for (size_t i = 0; i < rise.size(); ++i) summit[i] += rise[i];
    c[static_cast<size_t>(Cue::Summit)] = summit;
    c[static_cast<size_t>(Cue::Fall)] = rumble(r, 3.0f, 260, 40, 0.5f);
    c[static_cast<size_t>(Cue::Incident)] = beeps({880, 660}, 0.09f, 0.05f, 0.12f);
    c[static_cast<size_t>(Cue::Advisory)] = beeps({1046.5f, 784, 1046.5f, 784}, 0.07f, 0.04f, 0.14f);
    c[static_cast<size_t>(Cue::Down)] = down(r);
    c[static_cast<size_t>(Cue::Patched)] = patched(r);
    c[static_cast<size_t>(Cue::Retired)] = chime({523.3f, 392.0f}, 0.16f, 0.35f, 0.9f, 0.18f);
    c[static_cast<size_t>(Cue::Slip)] = rumble(r, 0.9f, 500, 120, 0.6f);
    for (int k = 0; k < 6; ++k) steps_.push_back(step(r));
    for (int k = 0; k < 4; ++k) chirps_.push_back(chirp(r));
}

void Synth::play(Cue c) {
    const std::vector<float>& s = cues_[static_cast<size_t>(c)];
    if (!s.empty() && playing_.size() < 16) playing_.push_back({&s, 0});
}

float Synth::noise() {
    seed_ ^= seed_ << 13;
    seed_ ^= seed_ >> 17;
    seed_ ^= seed_ << 5;
    return static_cast<float>(seed_ >> 8) / 8388608.0f - 1.0f;
}

float Synth::next() {
    // Glide the steady sounds towards where they are heading.
    auto glide = [](float& v, float target) { v += (target - v) * kGlide * 8.0f; };
    glide(now_.wind, target_.wind);
    glide(now_.grind, target_.grind);
    glide(now_.strain, target_.strain);
    glide(now_.crickets, target_.crickets);
    glide(now_.town, target_.town);
    glide(now_.alarm, target_.alarm);
    now_.steps = target_.steps;
    time_ += kDt;
    const float t = static_cast<float>(std::fmod(time_, 1000.0));
    const float n = noise();

    // Wind: low noise, gusting slowly.
    windLow_ += pole(380) * (n - windLow_);
    windSlow_ += pole(0.15f) * (noise() - windSlow_);
    const float gust = 0.6f + 0.4f * std::sin(kTwoPi * 0.07f * t) + 2.0f * windSlow_;
    float s = 0.5f * now_.wind * windLow_ * gust;

    // The boulder: a deep grinding rumble, and the gravel under it.
    grindLow_ += pole(160) * (n - grindLow_);
    grindRumble_ += pole(45) * (grindLow_ - grindRumble_);
    s += now_.grind * (2.2f * grindRumble_ + 0.5f * grindLow_);

    // Straining: a low breath, in and out.
    const float breath = 0.5f + 0.5f * std::sin(kTwoPi * 0.55f * t);
    s += 0.25f * now_.strain * breath * grindLow_ + 0.03f * now_.strain * std::sin(kTwoPi * 82 * t) * breath;

    // The town: a far murmur.
    townLow_ += pole(260) * (noise() - townLow_);
    s += 0.25f * now_.town * townLow_;

    // An outage: a soft tone that pulses, so it is noticed without being shouted.
    alarmPhase_ += kDt;
    const float pulse = std::fmod(alarmPhase_, 1.2f) < 0.25f ? 1.0f : 0.0f;
    s += 0.035f * now_.alarm * pulse * std::sin(kTwoPi * 392 * t);

    // Footsteps and crickets, from their clocks.
    stepClock_ += std::fmin(now_.steps, 4.0f) * kDt;  // never faster than a run
    if (stepClock_ >= 1) {
        stepClock_ -= 1;
        playing_.push_back({&steps_[static_cast<size_t>(step_++ % static_cast<int>(steps_.size()))], 0});
    }
    if (now_.crickets > 0.05f) {
        chirpClock_ += (2.5f + 1.5f * noise()) * now_.crickets * kDt;
        if (chirpClock_ >= 1) {
            chirpClock_ -= 1;
            playing_.push_back({&chirps_[static_cast<size_t>(seed_ % chirps_.size())], 0});
        }
    }

    for (size_t i = 0; i < playing_.size();) {
        Playing& p = playing_[i];
        s += (*p.samples)[p.at++];
        if (p.at >= p.samples->size()) {
            playing_[i] = playing_.back();
            playing_.pop_back();
        } else {
            ++i;
        }
    }
    // A soft limit, so stacked sounds never clip.
    return std::tanh(s);
}

void Synth::render(float* out, int frames) {
    for (int i = 0; i < frames; ++i) out[i] = next();
}

}  // namespace sisyphus::audio
