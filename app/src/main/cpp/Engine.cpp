#include <jni.h>
#include <cstring>
#include <oboe/Oboe.h>
#include <oboe/FullDuplexStream.h>
#include <NAM/get_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include <stdexcept>
#include "Controls.h"

// All lifecycle calls are serialized by the Java executor. Only parameters cross
// into the callback, through lock-free atomics. Model ownership never changes live.
static_assert(std::atomic<float>::is_always_lock_free);
class Engine final : public oboe::FullDuplexStream, public oboe::AudioStreamErrorCallback {
public:
    static constexpr int block = 256;
    std::shared_ptr<oboe::AudioStream> in, out;
    std::unique_ptr<nam::DSP> model;
    std::atomic<float> inputDb{0}, outputDb{-12}, gateDb{-60};
    std::atomic<bool> bypass{false}, disconnected{false};
    std::array<float, block> input{}, output{};
    Controls controls;
    int rate = 0;
    ~Engine() { close(); }
    void close() {
        if (out) { out->requestStop(); out->close(); }
        if (in) { in->requestStop(); in->close(); }
        setInputStream(nullptr); setOutputStream(nullptr);
        out.reset(); in.reset();
    }
    void load(const std::string& path) {
        close();
        auto candidate = nam::get_dsp(std::filesystem::path(path));
        if (!candidate || candidate->NumInputChannels()!=1 || candidate->NumOutputChannels()!=1)
            throw std::runtime_error("Only mono NAM models are supported.");
        model = std::move(candidate);
    }
    void check(oboe::Result result) {
        if (result != oboe::Result::OK) throw std::runtime_error(oboe::convertToText(result));
    }
    void open() {
        close(); disconnected.store(false);
        try {
            oboe::AudioStreamBuilder b;
            b.setDirection(oboe::Direction::Output)->setFormat(oboe::AudioFormat::Float)
                ->setChannelCount(2)->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                ->setSharingMode(oboe::SharingMode::Exclusive)->setDataCallback(this)->setErrorCallback(this);
            // No resampler yet: explicitly request the model's training rate.
            if (model && model->GetExpectedSampleRate()>0)
                b.setSampleRate(static_cast<int>(model->GetExpectedSampleRate()));
            check(b.openStream(out));
            rate = out->getSampleRate();
            if (model && model->GetExpectedSampleRate()>0 && model->GetExpectedSampleRate()!=rate)
                throw std::runtime_error("Audio device rate does not match the model. Resampling is not implemented.");
            oboe::AudioStreamBuilder ib;
            ib.setDirection(oboe::Direction::Input)->setFormat(oboe::AudioFormat::Float)
                ->setChannelCount(1)->setSampleRate(rate)->setInputPreset(oboe::InputPreset::Unprocessed)
                ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                ->setSharingMode(oboe::SharingMode::Exclusive)
                ->setBufferCapacityInFrames(out->getBufferCapacityInFrames()*2)->setErrorCallback(this);
            check(ib.openStream(in));
            if (in->getSampleRate()!=rate) throw std::runtime_error("Input/output sample rates differ.");
            out->setBufferSizeInFrames(out->getFramesPerBurst()*2);
            controls = Controls{};
            if (model) model->ResetAndPrewarm(rate, block);
            setInputStream(in.get()); setOutputStream(out.get());
            check(start());
        } catch (...) { close(); throw; }
    }
    void onErrorAfterClose(oboe::AudioStream*, oboe::Result) override { disconnected.store(true); }
    oboe::DataCallbackResult onBothStreamsReady(const void* data, int available, void* dest, int frames) override {
        auto* src = static_cast<const float*>(data);
        auto* dst = static_cast<float*>(dest);
        const bool dry = bypass.load(std::memory_order_relaxed);
        const float gain = Controls::db(inputDb.load(std::memory_order_relaxed));
        const float level = Controls::db(outputDb.load(std::memory_order_relaxed));
        const float threshold = gateDb.load(std::memory_order_relaxed);
        for (int offset=0; offset<frames; offset+=block) {
            int n = std::min(block, frames-offset);
            for (int i=0;i<n;++i) {
                controls.gain += (gain-controls.gain)*0.002f;
                input[i] = controls.input(src && offset+i<available ? src[offset+i] : 0.f, threshold);
            }
            if (model && !dry) {
                float* ip=input.data(); float* op=output.data();
                model->process(&ip,&op,n);
            } else std::copy_n(input.data(),n,output.data());
            for (int i=0;i<n;++i) {
                controls.output += (level-controls.output)*0.002f;
                float sample=controls.finish(output[i]);
                dst[(offset+i)*2]=sample; dst[(offset+i)*2+1]=sample;
            }
        }
        return oboe::DataCallbackResult::Continue;
    }
};
static Engine engine;
static jstring message(JNIEnv* env, const std::string& s) { return env->NewStringUTF(s.c_str()); }
extern "C" JNIEXPORT jstring JNICALL Java_org_acoustixaudio_nam2_MainActivity_nativeStart(JNIEnv* env,jobject) {
    try { engine.open(); return message(env,"Running • "+std::to_string(engine.rate)+" Hz"); }
    catch (const std::exception& e) { return message(env,std::string("Error: ")+e.what()); }
}
extern "C" JNIEXPORT void JNICALL Java_org_acoustixaudio_nam2_MainActivity_nativeStop(JNIEnv*,jobject) { engine.close(); }
extern "C" JNIEXPORT jstring JNICALL Java_org_acoustixaudio_nam2_MainActivity_nativeLoad(JNIEnv* env,jobject,jstring path) {
    const char* chars=env->GetStringUTFChars(path,nullptr);
    if (!chars) return nullptr;
    std::string file(chars); env->ReleaseStringUTFChars(path,chars);
    try { engine.load(file); return message(env,"Model ready • "+std::to_string(static_cast<int>(engine.model->GetExpectedSampleRate()))+" Hz"); }
    catch (const std::exception& e) { return message(env,std::string("Error: ")+e.what()); }
}
extern "C" JNIEXPORT void JNICALL Java_org_acoustixaudio_nam2_MainActivity_nativeParameters(JNIEnv*,jobject,jfloat input,jfloat output,jfloat gate,jboolean bypass) {
    engine.inputDb.store(input); engine.outputDb.store(output); engine.gateDb.store(gate); engine.bypass.store(bypass);
}
extern "C" JNIEXPORT jboolean JNICALL Java_org_acoustixaudio_nam2_MainActivity_nativeDisconnected(JNIEnv*,jobject) { return engine.disconnected.load(); }
