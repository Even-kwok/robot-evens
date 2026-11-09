import AVFoundation
import SwiftUI
import UniformTypeIdentifiers

@MainActor
final class EmotionSoundStudioModel: ObservableObject {
    @Published var selectedSlot: EmotionSoundSlot = .happy
    @Published var settings = EmotionSoundSettings()
    @Published private(set) var waveform: [Double] = []
    @Published private(set) var rawData = Data()
    @Published private(set) var statusText = "选择情绪后生成，或录音/导入音频"
    @Published private(set) var sourceTitle = "内置合成器"
    @Published private(set) var isRecording = false
    @Published private(set) var isBusy = false

    private var sourceSamples: [Float] = []
    private var processedSamples: [Float] = []
    private var recorder: AVAudioRecorder?
    private var recordingStopTask: Task<Void, Never>?
    private var player: AVAudioPlayer?

    var durationText: String {
        String(format: "%.2f 秒 · %d bytes · raw-u8-16k-mono", Double(rawData.count) / 16_000, rawData.count)
    }

    func generate() {
        sourceSamples = EmotionSoundProcessor.synthesize(
            slot: selectedSlot,
            durationMs: settings.durationMs
        )
        sourceTitle = "内置合成 · \(selectedSlot.title)"
        rebuild()
    }

    func updateSettings(_ update: (inout EmotionSoundSettings) -> Void) {
        update(&settings)
        settings = EmotionSoundSettings(
            durationMs: settings.durationMs,
            speed: settings.speed,
            pitchSemitones: settings.pitchSemitones,
            robotAmount: settings.robotAmount,
            roughness: settings.roughness,
            gain: settings.gain
        )
        if sourceTitle.hasPrefix("内置合成") {
            sourceSamples = EmotionSoundProcessor.synthesize(slot: selectedSlot, durationMs: settings.durationMs)
        }
        rebuild()
    }

    func toggleRecording() {
        if isRecording {
            stopRecording()
        } else {
            Task { await startRecording() }
        }
    }

    func loadAudio(from url: URL) {
        isBusy = true
        defer { isBusy = false }
        let scoped = url.startAccessingSecurityScopedResource()
        defer { if scoped { url.stopAccessingSecurityScopedResource() } }
        do {
            let file = try AVAudioFile(forReading: url)
            let sampleLimit = Int64(file.processingFormat.sampleRate * 20)
            let frameCount = AVAudioFrameCount(min(file.length, sampleLimit))
            guard
                frameCount > 0,
                let buffer = AVAudioPCMBuffer(pcmFormat: file.processingFormat, frameCapacity: frameCount)
            else {
                throw StudioAudioError.unreadable
            }
            try file.read(into: buffer, frameCount: frameCount)
            guard let channels = buffer.floatChannelData else { throw StudioAudioError.unreadable }
            let channelCount = Int(buffer.format.channelCount)
            let count = Int(buffer.frameLength)
            var mono = [Float](repeating: 0, count: count)
            for frame in 0..<count {
                var total: Float = 0
                for channel in 0..<channelCount { total += channels[channel][frame] }
                mono[frame] = total / Float(max(1, channelCount))
            }
            sourceSamples = EmotionSoundProcessor.resample(
                mono,
                sourceRate: file.processingFormat.sampleRate
            )
            sourceTitle = "导入 · \(url.lastPathComponent)"
            rebuild()
        } catch {
            statusText = "音频读取失败：\(error.localizedDescription)"
        }
    }

    func preview() {
        guard !rawData.isEmpty else { return }
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playback, mode: .default, options: [.defaultToSpeaker])
            try session.setActive(true)
            player = try AVAudioPlayer(data: EmotionSoundProcessor.wavData(fromRawU8: rawData))
            player?.prepareToPlay()
            player?.play()
            statusText = "正在手机试听 \(selectedSlot.title)"
        } catch {
            statusText = "试听失败：\(error.localizedDescription)"
        }
    }

    func uploadAndPlay(robotBaseURLString: String) async {
        guard !rawData.isEmpty else { return }
        let normalized = RobotConnectionDefaults.normalizedBaseURL(robotBaseURLString)
        guard let url = URL(string: normalized) else {
            statusText = "机器人地址无效"
            return
        }
        isBusy = true
        statusText = "正在上传到 \(selectedSlot.title)…"
        defer { isBusy = false }
        do {
            let result = try await RobotClient(baseURL: url).uploadEmotionSoundSlot(
                selectedSlot,
                rawU8Audio: rawData,
                playAfterUpload: true
            )
            statusText = result.ok
                ? "已上传并在机器人试听；重启前有效"
                : "机器人拒绝：\(result.error ?? "未知错误")"
        } catch {
            statusText = "上传失败：\(error.localizedDescription)"
        }
    }

    private func startRecording() async {
        let granted = await withCheckedContinuation { continuation in
            AVAudioApplication.requestRecordPermission { allowed in
                continuation.resume(returning: allowed)
            }
        }
        guard granted else {
            statusText = "没有麦克风权限"
            return
        }
        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playAndRecord, mode: .default, options: [.defaultToSpeaker, .allowBluetoothHFP])
            try session.setActive(true)
            let url = FileManager.default.temporaryDirectory.appendingPathComponent("robot-emotion-recording.wav")
            recorder = try AVAudioRecorder(
                url: url,
                settings: [
                    AVFormatIDKey: Int(kAudioFormatLinearPCM),
                    AVSampleRateKey: 16_000,
                    AVNumberOfChannelsKey: 1,
                    AVLinearPCMBitDepthKey: 16,
                    AVLinearPCMIsFloatKey: false,
                    AVLinearPCMIsBigEndianKey: false
                ]
            )
            recorder?.record(forDuration: 20)
            isRecording = true
            statusText = "录音中，最长 20 秒"
            recordingStopTask?.cancel()
            recordingStopTask = Task { [weak self] in
                try? await Task.sleep(for: .seconds(20))
                guard !Task.isCancelled else { return }
                self?.stopRecording()
            }
        } catch {
            statusText = "录音启动失败：\(error.localizedDescription)"
        }
    }

    private func stopRecording() {
        guard let recorder else { return }
        recordingStopTask?.cancel()
        recordingStopTask = nil
        recorder.stop()
        self.recorder = nil
        isRecording = false
        loadAudio(from: recorder.url)
    }

    private func rebuild() {
        guard !sourceSamples.isEmpty else { return }
        processedSamples = EmotionSoundProcessor.process(sourceSamples, settings: settings)
        rawData = EmotionSoundProcessor.rawU8Data(from: processedSamples)
        waveform = EmotionSoundProcessor.waveform(processedSamples)
        statusText = "已生成 \(selectedSlot.title) · \(durationText)"
    }

    private enum StudioAudioError: LocalizedError {
        case unreadable
        var errorDescription: String? { "无法解码这个音频文件" }
    }
}

struct EmotionSoundStudioView: View {
    @Binding var robotBaseURLString: String
    @StateObject private var model = EmotionSoundStudioModel()
    @State private var isImporting = false
    @State private var exportDocument: RawSoundDocument?
    @State private var isExporting = false

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                GroupBox("固件情绪槽") {
                    VStack(alignment: .leading, spacing: 12) {
                        Picker("目标槽", selection: $model.selectedSlot) {
                            ForEach(EmotionSoundSlot.allCases) { slot in
                                Text(slot.title).tag(slot)
                            }
                        }
                        .pickerStyle(.menu)

                        HStack {
                            Button {
                                model.generate()
                            } label: {
                                Label("生成情绪音", systemImage: "wand.and.stars")
                            }
                            .buttonStyle(.borderedProminent)

                            Button {
                                model.toggleRecording()
                            } label: {
                                Label(model.isRecording ? "停止录音" : "录音", systemImage: model.isRecording ? "stop.circle.fill" : "mic.circle")
                            }
                            .tint(model.isRecording ? .red : .accentColor)

                            Button {
                                isImporting = true
                            } label: {
                                Label("导入", systemImage: "square.and.arrow.down")
                            }
                        }
                        .buttonStyle(.bordered)

                        Text(model.sourceTitle)
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }

                GroupBox("声音塑形") {
                    VStack(alignment: .leading, spacing: 14) {
                        soundSlider(
                            title: "基础长度",
                            value: Binding(
                                get: { Double(model.settings.durationMs) },
                                set: { value in model.updateSettings { $0.durationMs = Int(value.rounded()) } }
                            ),
                            range: 120...4_000,
                            display: "\(model.settings.durationMs) ms"
                        )
                        soundSlider(
                            title: "速度",
                            value: Binding(
                                get: { model.settings.speed },
                                set: { value in model.updateSettings { $0.speed = value } }
                            ),
                            range: 0.5...2,
                            display: String(format: "%.2fx", model.settings.speed)
                        )
                        soundSlider(
                            title: "音高",
                            value: Binding(
                                get: { model.settings.pitchSemitones },
                                set: { value in model.updateSettings { $0.pitchSemitones = value } }
                            ),
                            range: -12...12,
                            display: String(format: "%+.1f 半音", model.settings.pitchSemitones)
                        )
                        soundSlider(
                            title: "机器感",
                            value: Binding(
                                get: { model.settings.robotAmount },
                                set: { value in model.updateSettings { $0.robotAmount = value } }
                            ),
                            range: 0...1,
                            display: "\(Int(model.settings.robotAmount * 100))%"
                        )
                        soundSlider(
                            title: "颗粒感",
                            value: Binding(
                                get: { model.settings.roughness },
                                set: { value in model.updateSettings { $0.roughness = value } }
                            ),
                            range: 0...1,
                            display: "\(Int(model.settings.roughness * 100))%"
                        )
                    }
                }

                GroupBox("预览与导出") {
                    VStack(alignment: .leading, spacing: 12) {
                        SoundWaveformView(values: model.waveform)
                            .frame(height: 110)
                            .background(Color.black.opacity(0.9), in: RoundedRectangle(cornerRadius: 12))

                        Text(model.durationText)
                            .font(.footnote.monospacedDigit())
                            .foregroundStyle(.secondary)

                        HStack {
                            Button {
                                model.preview()
                            } label: {
                                Label("手机试听", systemImage: "speaker.wave.2")
                            }
                            Button {
                                Task { await model.uploadAndPlay(robotBaseURLString: robotBaseURLString) }
                            } label: {
                                Label("上传机器人试听", systemImage: "antenna.radiowaves.left.and.right")
                            }
                            .disabled(model.isBusy || model.rawData.isEmpty)
                        }
                        .buttonStyle(.borderedProminent)

                        Button {
                            exportDocument = RawSoundDocument(data: model.rawData)
                            isExporting = true
                        } label: {
                            Label("导出 \(model.selectedSlot.rawValue).raw 给固件", systemImage: "square.and.arrow.up")
                        }
                        .buttonStyle(.bordered)
                        .disabled(model.rawData.isEmpty)

                        Text(model.statusText)
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }
            }
            .padding()
        }
        .onAppear { if model.rawData.isEmpty { model.generate() } }
        .onChange(of: model.selectedSlot) { _, _ in model.generate() }
        .fileImporter(isPresented: $isImporting, allowedContentTypes: [.audio]) { result in
            if case .success(let url) = result { model.loadAudio(from: url) }
        }
        .fileExporter(
            isPresented: $isExporting,
            document: exportDocument,
            contentType: .data,
            defaultFilename: "\(model.selectedSlot.rawValue).raw"
        ) { _ in }
    }

    private func soundSlider(
        title: String,
        value: Binding<Double>,
        range: ClosedRange<Double>,
        display: String
    ) -> some View {
        VStack(spacing: 5) {
            HStack {
                Text(title)
                Spacer()
                Text(display).font(.footnote.monospacedDigit()).foregroundStyle(.secondary)
            }
            Slider(value: value, in: range)
        }
    }
}

private struct SoundWaveformView: View {
    let values: [Double]

    var body: some View {
        Canvas { context, size in
            guard !values.isEmpty else { return }
            let middle = size.height / 2
            let width = size.width / CGFloat(values.count)
            var path = Path()
            for (index, value) in values.enumerated() {
                let x = CGFloat(index) * width + width / 2
                let amplitude = max(1, CGFloat(value) * size.height * 0.43)
                path.move(to: CGPoint(x: x, y: middle - amplitude))
                path.addLine(to: CGPoint(x: x, y: middle + amplitude))
            }
            context.stroke(path, with: .color(.cyan), lineWidth: max(1, width * 0.55))
        }
        .padding(8)
    }
}

private struct RawSoundDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.data] }
    let data: Data

    init(data: Data) { self.data = data }
    init(configuration: ReadConfiguration) throws { data = configuration.file.regularFileContents ?? Data() }
    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper { FileWrapper(regularFileWithContents: data) }
}
