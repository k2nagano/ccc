#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <ctime>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

// 現在時刻を取得し、ファイル名を生成
std::string getTimestampedFilename() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);

    std::ostringstream oss;
    oss << "oculus_"
        << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << "."
        << std::setw(3) << std::setfill('0') << now_ms.count()
        << ".mkv";
    return oss.str();
}

class SonarRecorder {
public:
    SonarRecorder(int width, int height, int fps, bool is16bit);
    ~SonarRecorder();
    void recordFrame(const uint8_t* pImage);

private:
    int width, height, fps;
    bool is16bit;
    std::string filename;
    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVStream* stream = nullptr;
    AVPacket* pkt = nullptr;

    void initFFmpeg();
    void encodeFrame(AVFrame* frame);
    void cleanup();
};

// コンストラクタ
SonarRecorder::SonarRecorder(int width, int height, int fps, bool is16bit)
    : width(width), height(height), fps(fps), is16bit(is16bit), filename(getTimestampedFilename()) {
    avformat_network_init();
    initFFmpeg();
}

// デストラクタ
SonarRecorder::~SonarRecorder() {
    cleanup();
}

// フレーム記録処理 (エラー原因の関数が欠落していたため修正)
void SonarRecorder::recordFrame(const uint8_t* pImage) {
    if (!pImage) return;

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Error: Failed to allocate AVFrame" << std::endl;
        return;
    }
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = width;
    frame->height = height;
    av_frame_get_buffer(frame, 32);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t intensity = is16bit ? pImage[2*(y*width + x)] / 256 : pImage[y*width + x];
            frame->data[0][y * frame->linesize[0] + x] = intensity;
        }
    }

    memset(frame->data[1], 128, frame->linesize[1] * height / 2);
    memset(frame->data[2], 128, frame->linesize[2] * height / 2);

    encodeFrame(frame);
    av_frame_free(&frame);
}

// FFmpeg 初期化処理
void SonarRecorder::initFFmpeg() {
    int ret = avformat_alloc_output_context2(&formatCtx, nullptr, "matroska", filename.c_str());
    if (ret < 0 || !formatCtx) {
        std::cerr << "Error: Failed to allocate output context (" << ret << ")" << std::endl;
        exit(1);
    }

    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Error: H264 encoder not found" << std::endl;
        exit(1);
    }

    stream = avformat_new_stream(formatCtx, codec);
    if (!stream) {
        std::cerr << "Error: Failed to create stream" << std::endl;
        exit(1);
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        std::cerr << "Error: Failed to allocate codec context" << std::endl;
        exit(1);
    }

    codecCtx->width = width;
    codecCtx->height = height;
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    codecCtx->time_base = {1, fps};
    codecCtx->framerate = {fps, 1};
    codecCtx->gop_size = 30;
    codecCtx->max_b_frames = 1;
    codecCtx->bit_rate = 1000000;
    codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if ((ret = avcodec_open2(codecCtx, codec, nullptr)) < 0) {
        std::cerr << "Error: Failed to open codec (" << ret << ")" << std::endl;
        exit(1);
    }

    ret = avcodec_parameters_from_context(stream->codecpar, codecCtx);
    if (ret < 0) {
        std::cerr << "Error: Failed to copy codec parameters (" << ret << ")" << std::endl;
        exit(1);
    }

    av_dump_format(formatCtx, 0, filename.c_str(), 1);

    if ((ret = avio_open(&formatCtx->pb, filename.c_str(), AVIO_FLAG_WRITE)) < 0) {
        std::cerr << "Error: Could not open output file (" << ret << ")" << std::endl;
        exit(1);
    }

    if ((ret = avformat_write_header(formatCtx, nullptr)) < 0) {
        std::cerr << "Error: Failed to write header (" << ret << ")" << std::endl;
        exit(1);
    }

    pkt = av_packet_alloc();
}

// フレームをエンコード
void SonarRecorder::encodeFrame(AVFrame* frame) {
    int ret = avcodec_send_frame(codecCtx, frame);
    if (ret < 0) {
        std::cerr << "Error: Failed to send frame for encoding (" << ret << ")" << std::endl;
        return;
    }

    while (avcodec_receive_packet(codecCtx, pkt) == 0) {
        av_interleaved_write_frame(formatCtx, pkt);
        av_packet_unref(pkt);
    }
}

// リソース解放
void SonarRecorder::cleanup() {
    if (formatCtx) {
        av_write_trailer(formatCtx);
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
        avformat_free_context(formatCtx);
    }
    if (pkt) {
        av_packet_free(&pkt);
    }
}

// メイン関数
int main() {
    SonarRecorder recorder(512, 256, 15, false);
    std::vector<uint8_t> dummyImage(512 * 256, 128);
    for (int i = 0; i < 300; ++i) {
        recorder.recordFrame(dummyImage.data());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 15));
    }
    std::cout << "Recording completed." << std::endl;
    return 0;
}
