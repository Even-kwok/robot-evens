import AVFoundation
import Foundation

enum PoseInputMode: String, CaseIterable, Identifiable {
    case motion
    case manual

    var id: String { rawValue }

    var title: String {
        switch self {
        case .motion: "陀螺仪"
        case .manual: "滑块"
        }
    }
}

enum TimelineSelection: Equatable {
    case clip(Int)
    case sound(Int)
}

enum AudioBeatLightMode: String, CaseIterable, Identifiable {
    case beatFlash
    case marquee
    case breathing
    case rainbow
    case custom

    var id: String { rawValue }

    var title: String {
        switch self {
        case .beatFlash: "随音乐闪烁"
        case .marquee: "跑马灯"
        case .breathing: "呼吸灯"
        case .rainbow: "彩虹"
        case .custom: "自定义"
        }
    }
}

enum AudioBeatLightColorPreset: String, CaseIterable, Identifiable {
    case cyan
    case green
    case pink
    case amber
    case blue
    case white

    var id: String { rawValue }

    var title: String {
        switch self {
        case .cyan: "青色"
        case .green: "绿色"
        case .pink: "粉色"
        case .amber: "橙色"
        case .blue: "蓝色"
        case .white: "白色"
        }
    }

    var hex: String {
        switch self {
        case .cyan: "#00f2ff"
        case .green: "#00ff66"
        case .pink: "#ff4fd8"
        case .amber: "#ffb000"
        case .blue: "#2f6bff"
        case .white: "#ffffff"
        }
    }
}

@MainActor
final class AppViewModel: ObservableObject {
    @Published var robotBaseURLString: String {
        didSet {
            userDefaults.set(robotBaseURLString, forKey: Self.robotBaseURLDefaultsKey)
        }
    }
    @Published var statusText = "未连接"
    @Published var isConnected = false
    @Published var isLiveControlEnabled = false
    @Published var isRecording = false
    @Published var isPaused = false
    @Published var selectedStartSoundAssetId = "sound.happy"
    @Published var savedActions: [RobotAction] = []
    @Published var poseInputMode: PoseInputMode = .motion
    @Published var servoSpeed = 700
    @Published var isPrecisionModeEnabled = true
    @Published var isLowLatencyStreamEnabled = true
    @Published var isMotionStreamAvailable = false
    @Published var currentPose = ServoPose(yaw: 0, pitch: 120, speed: 260)
    @Published var manualYaw = 0
    @Published var manualPitch = 120
    @Published var selectedActionClipId = BuiltInActionClipLibrary.clips.first?.id ?? ""
    @Published var timelineStartMs = 0
    @Published var timelineSpeedScale = 1.0
    @Published var selectedTimelineSoundAssetId = "sound.happy"
    @Published var timelineSoundTimeMs = 0
    @Published var timelineClips: [ActionTimelineClip] = []
    @Published var timelineSounds: [SoundEvent] = []
    @Published var selectedTimelineItem: TimelineSelection?
    @Published var rhythmTemplates = BuiltInRhythmTemplateLibrary.templates
    @Published var selectedRhythmTemplateId = BuiltInRhythmTemplateLibrary.templates.first?.id ?? ""
    @Published private(set) var rhythmActiveLayers: [RhythmActiveTemplate] = []
    @Published private(set) var activeRhythmTemplateIds: Set<String> = []
    @Published private(set) var isRhythmRecording = false
    @Published private(set) var rhythmRecordedPoseCount = 0
    @Published private(set) var isAudioBeatControlEnabled = false
    @Published private(set) var audioBeatLevel = 0.0
    @Published var audioBeatSensitivity = 0.65
    @Published var isAudioBeatPitchEnabled = true
    @Published var audioBeatPitchAmplitude = 120
    @Published var isAudioBeatPitchAmplitudeRandomized = false
    @Published var isAudioBeatYawEnabled = true
    @Published var audioBeatYawAmplitude = 70
    @Published var isAudioBeatYawAmplitudeRandomized = false
    @Published var audioBeatSpeed = 760
    @Published var isAudioBeatAdaptiveSpeedEnabled = true
    @Published var audioBeatSlowSpeed = 360
    @Published var audioBeatFastSpeed = 960
    @Published var audioBeatMotionCurve: RhythmMotionCurve = .linear
    @Published var isAudioBeatDriftAllowed = false
    @Published var isAudioBeatAutoReturnEnabled = true
    @Published var isAudioBeatStandbyPoseEnabled = true
    @Published var isAudioBeatLightEnabled = false
    @Published var audioBeatLightMode: AudioBeatLightMode = .beatFlash
    @Published var audioBeatLightBrightness = 180
    @Published var audioBeatLightPrimaryColor: AudioBeatLightColorPreset = .cyan
    @Published var audioBeatLightSecondaryColor: AudioBeatLightColorPreset = .pink
    @Published private(set) var isRealtimeVoiceStreaming = false
    @Published private(set) var isRealtimeVoiceFrameSending = false
    @Published private(set) var realtimeVoiceStatusText = "待机"
    @Published private(set) var realtimeVoiceReplyText = "-"
    @Published private(set) var realtimeVoiceAudioText = "-"
    @Published private(set) var realtimeVoiceFrameCount = 0
    @Published var isChatMotionEnabled = true
    @Published var chatMotionStyle: ChatMotionStyle = .natural
    @Published private(set) var currentChatMotionState: ChatMotionCallState = .settling
    @Published private(set) var chatMotionSummaryText = "等待实时语音"

    let expressionMoods = ["neutral", "happy", "love", "surprised", "thinking", "curious", "sad", "angry"]
    let actionClips = BuiltInActionClipLibrary.clips
    let startSoundChoices = [
        "sound.happy",
        "sound.shy",
        "sound.cute",
        "sound.proud",
        "sound.confused",
        "sound.surprised",
        "sound.celebrate"
    ]

    private var client: RobotClient?
    private var motionMapper = MotionMapper()
    private let motionStreamClient = MotionStreamUDPClient()
    private var manualPoseEditor = ManualPoseEditor()
    private var recorder = ActionRecorder()
    private var currentAttitude: PhoneAttitude = .zero
    private var lastPoseSentAtMs = 0
    private var recordingIndex = 1
    private var timelineActionIndex = 1
    private var rhythmActionIndex = 1
    private var previewTask: Task<Void, Never>?
    private var rhythmLoopTask: Task<Void, Never>?
    private var rhythmLoopStartedAtMs: Int?
    private var rhythmRecordingStartedAtMs: Int?
    private var rhythmRecordedPoses: [PoseKeyframe] = []
    private var audioLevelMonitor: AudioLevelMonitor?
    private var liveVoiceStreamer: VoiceLiveAudioStreamer?
    private var realtimeVoiceClient: VolcengineRealtimeDirectClient?
    private var realtimeVoiceRobotClient: RobotClient?
    private var realtimeVoiceReceiveTask: Task<Void, Never>?
    private var realtimeVoiceChunkQueue: [Data] = []
    private var chatMotionTask: Task<Void, Never>?
    private var latestRealtimeVoiceStage: String?
    private var audioBeatMapper = AudioBeatServoMapper()
    private var audioBeatLockedCenterPose: ServoPose?
    private var lastAudioBeatLightSentAtMs = 0
    private var audioBeatLightFlashToken = 0
    private var pendingLivePose: ServoPose?
    private var isSendingLivePose = false
    private var didEnableLiveControlForRhythm = false
    private var didEnableLiveControlForAudioBeat = false
    private var hasAttemptedInitialConnection = false
    private let userDefaults: UserDefaults

    private static let robotBaseURLDefaultsKey = "robotBaseURLString"
    private static let servoSpeedDefaultsKey = "servoSpeed"
    private static let precisionModeDefaultsKey = "precisionModeEnabled"
    private static let lowLatencyStreamDefaultsKey = "lowLatencyStreamEnabled"
    private static let chatMotionAssetId = "clip.chat_soft_idle"

    init(userDefaults: UserDefaults = .standard) {
        self.userDefaults = userDefaults
        robotBaseURLString = RobotConnectionDefaults.resolvedBaseURLString(
            stored: userDefaults.string(forKey: Self.robotBaseURLDefaultsKey)
        )
        servoSpeed = clampServoSpeed(userDefaults.integer(forKey: Self.servoSpeedDefaultsKey), fallback: 700)
        isPrecisionModeEnabled = userDefaults.object(forKey: Self.precisionModeDefaultsKey) as? Bool ?? true
        isLowLatencyStreamEnabled = userDefaults.object(forKey: Self.lowLatencyStreamDefaultsKey) as? Bool ?? true
        motionMapper = MotionMapper(configuration: makeMotionMapperConfiguration())
        currentPose = ServoPose(yaw: 0, pitch: 120, speed: servoSpeed)
    }

    var lastSavedActionTitle: String {
        savedActions.last?.title ?? "无"
    }

    var timelineSummaryText: String {
        "\(timelineClips.count) 段动作 · \(timelineSounds.count) 个音效"
    }

    var selectedRhythmTemplate: RhythmMotionTemplate? {
        rhythmTemplates.first { $0.id == selectedRhythmTemplateId }
    }

    var rhythmSummaryText: String {
        let activeText = rhythmActiveLayers.isEmpty ? "未运行" : "运行 \(rhythmActiveLayers.count) 个模板"
        let recordingText = isRhythmRecording ? " · 录制 \(rhythmRecordedPoseCount) 帧" : ""
        return activeText + recordingText
    }

    var audioBeatSummaryText: String {
        let levelText = "\(Int((audioBeatLevel * 100).rounded()))%"
        let axisText = [
            isAudioBeatPitchEnabled ? "点头" : nil,
            isAudioBeatYawEnabled ? "摇头" : nil
        ].compactMap { $0 }.joined(separator: "+")
        let centerText = isAudioBeatStandbyPoseEnabled ? "待机" : (isAudioBeatDriftAllowed ? "漂移" : "锁定")
        let randomText = (isAudioBeatPitchAmplitudeRandomized || isAudioBeatYawAmplitudeRandomized) ? " · 幅度随机" : ""
        let modeText = axisText.isEmpty ? "无动作" : axisText
        return isAudioBeatControlEnabled
            ? "手机麦克风 · \(modeText) · \(centerText)\(randomText) · \(levelText)"
            : "未开启"
    }

    var selectedTimelineClip: (index: Int, clip: ActionTimelineClip)? {
        guard case .clip(let index) = selectedTimelineItem, timelineClips.indices.contains(index) else {
            return nil
        }
        return (index, timelineClips[index])
    }

    var selectedTimelineSound: (index: Int, sound: SoundEvent)? {
        guard case .sound(let index) = selectedTimelineItem, timelineSounds.indices.contains(index) else {
            return nil
        }
        return (index, timelineSounds[index])
    }

    var needsMotionUpdates: Bool {
        poseInputMode == .motion && rhythmActiveLayers.isEmpty && !isAudioBeatControlEnabled && (isLiveControlEnabled || isRecording)
    }

    var motionTuning: MotionControlTuning {
        isPrecisionModeEnabled ? .precision : .standard
    }

    var motionUpdateInterval: TimeInterval {
        1.0 / Double(motionTuning.motionUpdateHz)
    }

    var motionSourceToken: String {
        "\(needsMotionUpdates)-\(motionTuning.motionUpdateHz)"
    }

    func connectToDefaultIfNeeded() async {
        guard !hasAttemptedInitialConnection else { return }
        hasAttemptedInitialConnection = true
        await connect()
    }

    func connect() async {
        let normalizedURLString = RobotConnectionDefaults.normalizedBaseURL(robotBaseURLString)
        guard let url = URL(string: normalizedURLString) else {
            statusText = "机器人地址无效"
            isConnected = false
            client = nil
            return
        }

        robotBaseURLString = normalizedURLString
        statusText = "正在连接 \(normalizedURLString)"
        let robotClient = RobotClient(baseURL: url)
        client = robotClient
        do {
            _ = try await robotClient.ping()
            await updateMotionStreamAvailability(using: robotClient, baseURL: url)
            if isMotionStreamAvailable {
                statusText = "已连接 \(url.absoluteString)，流式姿态可用"
            } else {
                statusText = "已连接 \(url.absoluteString)"
            }
            isConnected = true
        } catch {
            statusText = "连接失败，请确认同一 Wi-Fi"
            isConnected = false
            isMotionStreamAvailable = false
            motionStreamClient.close()
        }
    }

    func calibrate() {
        motionMapper.calibrate(currentAttitude)
        currentPose = ServoPose(yaw: 0, pitch: 120, speed: servoSpeed)
        syncManualPose(from: currentPose)
        statusText = "已校准"
    }

    func setPoseInputMode(_ mode: PoseInputMode) {
        guard mode != poseInputMode else { return }
        poseInputMode = mode
        if mode == .manual {
            syncManualPose(from: currentPose)
            statusText = "滑块 K 帧模式"
        } else {
            motionMapper.calibrate(currentAttitude)
            statusText = "陀螺仪模式"
        }
    }

    func setServoSpeed(_ speed: Int) {
        let clamped = clampServoSpeed(speed, fallback: 700)
        guard clamped != servoSpeed else { return }
        servoSpeed = clamped
        userDefaults.set(clamped, forKey: Self.servoSpeedDefaultsKey)
        motionMapper.setSpeed(clamped)
        manualPoseEditor.setSpeed(clamped)
        currentPose = ServoPose(yaw: currentPose.yaw, pitch: currentPose.pitch, speed: clamped)
        if poseInputMode == .manual {
            queueManualPoseIfLive()
        }
    }

    func setPrecisionModeEnabled(_ enabled: Bool) {
        guard enabled != isPrecisionModeEnabled else { return }
        isPrecisionModeEnabled = enabled
        userDefaults.set(enabled, forKey: Self.precisionModeDefaultsKey)
        motionMapper = MotionMapper(configuration: makeMotionMapperConfiguration())
        motionMapper.calibrate(currentAttitude)
        statusText = enabled ? "精细模式" : "标准模式"
    }

    func setLowLatencyStreamEnabled(_ enabled: Bool) {
        guard enabled != isLowLatencyStreamEnabled else { return }
        isLowLatencyStreamEnabled = enabled
        userDefaults.set(enabled, forKey: Self.lowLatencyStreamDefaultsKey)
        if enabled && !isMotionStreamAvailable {
            statusText = "固件未开启流式姿态"
        } else {
            statusText = enabled ? "低延迟流式已开启" : "低延迟流式已关闭"
        }
    }

    func setManualYaw(_ yaw: Int) {
        manualPoseEditor.setYaw(yaw)
        syncCurrentPoseFromManualEditor()
        queueManualPoseIfLive()
    }

    func setManualPitch(_ pitch: Int) {
        manualPoseEditor.setPitch(pitch)
        syncCurrentPoseFromManualEditor()
        queueManualPoseIfLive()
    }

    func sendCurrentManualPose() {
        guard poseInputMode == .manual else { return }
        let pose = currentPose
        Task {
            do {
                try await client?.setPose(pose)
                statusText = "已发送姿态"
            } catch {
                statusText = "姿态发送失败"
            }
        }
    }

    func insertManualKeyframe() {
        guard poseInputMode == .manual else { return }
        guard isRecording else {
            statusText = "请先开始录制"
            return
        }
        guard !isPaused else {
            statusText = "暂停中"
            return
        }
        recorder.recordPose(currentPose, atMs: monotonicMilliseconds())
        statusText = "已插入关键帧 Yaw \(currentPose.yaw) Pitch \(currentPose.pitch)"
    }

    func handle(attitude: PhoneAttitude) {
        guard poseInputMode == .motion else { return }
        currentAttitude = attitude
        var mapper = motionMapper
        let pose = mapper.map(attitude)
        motionMapper = mapper
        currentPose = pose
        syncManualPose(from: pose)

        let now = monotonicMilliseconds()
        if isRecording && !isPaused {
            recorder.recordPose(pose, atMs: now)
        }
        guard isLiveControlEnabled else { return }
        queueLivePose(pose)
    }

    func toggleLiveControl() {
        isLiveControlEnabled.toggle()
        if !isLiveControlEnabled {
            pendingLivePose = nil
        } else if poseInputMode == .manual {
            queueLivePose(currentPose)
        }
        statusText = isLiveControlEnabled ? "实时控制中" : "实时控制已关闭"
    }

    func startRecording() {
        let now = monotonicMilliseconds()
        let id = "action.iphone_\(recordingIndex)"
        recorder.start(id: id, title: "iPhone Action \(recordingIndex)", atMs: now)
        recorder.setStartSoundAssetId(selectedStartSoundAssetId)
        recorder.recordPose(currentPose, atMs: now)
        isRecording = true
        isPaused = false
        statusText = "录制中"
    }

    func pauseOrResumeRecording() {
        guard isRecording else { return }
        let now = monotonicMilliseconds()
        if isPaused {
            recorder.resume(atMs: now)
            isPaused = false
            statusText = "继续录制"
        } else {
            recorder.pause(atMs: now)
            isPaused = true
            statusText = "已暂停"
        }
    }

    func stopRecording() {
        guard isRecording else { return }
        let now = monotonicMilliseconds()
        recorder.stop(atMs: now)
        do {
            let action = try recorder.buildAction(endMode: .home)
            savedActions.append(action)
            recordingIndex += 1
            statusText = "已保存 \(action.title)"
        } catch {
            statusText = "动作保存失败"
        }
        isRecording = false
        isPaused = false
    }

    func triggerFace(_ mood: String) {
        let now = monotonicMilliseconds()
        if isRecording && !isPaused {
            recorder.recordFaceEvent(mood: mood, atMs: now)
        }
        Task {
            do {
                try await client?.setFace(mood: mood)
            } catch {
                statusText = "表情发送失败"
            }
        }
    }

    func playSelectedStartSound() {
        Task {
            do {
                try await client?.invokeAsset(selectedStartSoundAssetId)
            } catch {
                statusText = "音效播放失败"
            }
        }
    }

    func startRealtimeVoiceStreaming() {
        guard !isRealtimeVoiceStreaming, !isRealtimeVoiceFrameSending else { return }
        if isAudioBeatControlEnabled {
            stopAudioBeatControl()
        }

        let normalizedURLString = RobotConnectionDefaults.normalizedBaseURL(robotBaseURLString)
        guard let url = URL(string: normalizedURLString) else {
            realtimeVoiceStatusText = "机器人地址无效"
            statusText = realtimeVoiceStatusText
            return
        }

        robotBaseURLString = normalizedURLString
        let apiClient = VolcengineRealtimeDirectClient(config: VolcengineRealtimeMobileConfig.defaultComputerConfig)
        let robotAudioClient = RobotClient(baseURL: url)
        realtimeVoiceClient = apiClient
        realtimeVoiceRobotClient = robotAudioClient
        realtimeVoiceChunkQueue = []
        realtimeVoiceFrameCount = 0
        latestRealtimeVoiceStage = "starting"
        realtimeVoiceStatusText = "准备收音"
        realtimeVoiceReplyText = "-"
        realtimeVoiceAudioText = "-"
        chatMotionSummaryText = isChatMotionEnabled ? "等待实时语音" : "已关闭"

        let streamer = VoiceLiveAudioStreamer { [weak self] chunk in
            Task { @MainActor in
                self?.enqueueRealtimeVoiceChunk(chunk)
            }
        }
        liveVoiceStreamer = streamer

        Task {
            await startRealtimeVoiceStreaming(
                with: streamer,
                apiClient: apiClient,
                robotClient: robotAudioClient
            )
        }
    }

    func stopRealtimeVoiceStreaming() {
        guard isRealtimeVoiceStreaming || liveVoiceStreamer != nil else { return }
        liveVoiceStreamer?.stop()
        liveVoiceStreamer = nil
        realtimeVoiceChunkQueue = []
        isRealtimeVoiceStreaming = false
        stopChatMotionLoop()
        realtimeVoiceStatusText = "正在停止"
        realtimeVoiceReceiveTask?.cancel()
        realtimeVoiceReceiveTask = nil
        let client = realtimeVoiceClient
        realtimeVoiceClient = nil
        realtimeVoiceRobotClient = nil

        Task {
            await client?.finish()
            realtimeVoiceStatusText = "已停止"
            statusText = realtimeVoiceStatusText
        }
    }

    func setChatMotionEnabled(_ isEnabled: Bool) {
        isChatMotionEnabled = isEnabled
        if isEnabled {
            chatMotionSummaryText = isRealtimeVoiceStreaming ? "等待下一次聊天动作" : "等待实时语音"
            startChatMotionLoopIfNeeded()
        } else {
            stopChatMotionLoop()
            chatMotionSummaryText = "已关闭"
        }
    }

    func setChatMotionStyle(_ style: ChatMotionStyle) {
        chatMotionStyle = style
        chatMotionSummaryText = isChatMotionEnabled ? "\(style.title) · \(currentChatMotionState.title)" : "已关闭"
    }

    func toggleAudioBeatControl() {
        if isAudioBeatControlEnabled {
            stopAudioBeatControl()
        } else {
            Task {
                await startAudioBeatControl()
            }
        }
    }

    func setAudioBeatSensitivity(_ value: Double) {
        audioBeatSensitivity = min(max(value, 0), 1)
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatPitchEnabled(_ isEnabled: Bool) {
        isAudioBeatPitchEnabled = isEnabled
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatPitchAmplitude(_ value: Int) {
        audioBeatPitchAmplitude = min(max(value, 0), 420)
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatPitchAmplitudeRandomized(_ isEnabled: Bool) {
        isAudioBeatPitchAmplitudeRandomized = isEnabled
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatYawEnabled(_ isEnabled: Bool) {
        isAudioBeatYawEnabled = isEnabled
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatYawAmplitude(_ value: Int) {
        audioBeatYawAmplitude = min(max(value, 0), 420)
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatYawAmplitudeRandomized(_ isEnabled: Bool) {
        isAudioBeatYawAmplitudeRandomized = isEnabled
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatSpeed(_ value: Int) {
        audioBeatSpeed = min(max(value, 0), 1000)
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatAdaptiveSpeedEnabled(_ isEnabled: Bool) {
        isAudioBeatAdaptiveSpeedEnabled = isEnabled
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatSlowSpeed(_ value: Int) {
        audioBeatSlowSpeed = min(max(value, 0), 1000)
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatFastSpeed(_ value: Int) {
        audioBeatFastSpeed = min(max(value, 0), 1000)
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatMotionCurve(_ curve: RhythmMotionCurve) {
        audioBeatMotionCurve = curve
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatDriftAllowed(_ isAllowed: Bool) {
        isAudioBeatDriftAllowed = isAllowed
        if !isAllowed && audioBeatLockedCenterPose == nil {
            audioBeatLockedCenterPose = audioBeatCenterPose()
        }
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatAutoReturnEnabled(_ isEnabled: Bool) {
        isAudioBeatAutoReturnEnabled = isEnabled
    }

    func setAudioBeatStandbyPoseEnabled(_ isEnabled: Bool) {
        isAudioBeatStandbyPoseEnabled = isEnabled
        if isEnabled {
            audioBeatLockedCenterPose = audioBeatStandbyPose
        } else if !isAudioBeatDriftAllowed {
            audioBeatLockedCenterPose = currentPose
        }
        updateAudioBeatMapperConfiguration()
    }

    func setAudioBeatLightEnabled(_ isEnabled: Bool) {
        isAudioBeatLightEnabled = isEnabled
        audioBeatLightFlashToken += 1
        if isEnabled {
            applyAudioBeatLightMode()
        } else {
            restoreAudioBeatStandbyLight()
        }
        statusText = isEnabled ? "音乐灯光已开启" : "音乐灯光已关闭"
    }

    func setAudioBeatLightMode(_ mode: AudioBeatLightMode) {
        audioBeatLightMode = mode
        audioBeatLightFlashToken += 1
        applyAudioBeatLightMode()
    }

    func setAudioBeatLightBrightness(_ value: Int) {
        audioBeatLightBrightness = min(max(value, 0), 255)
        applyAudioBeatLightMode()
    }

    func setAudioBeatLightPrimaryColor(_ preset: AudioBeatLightColorPreset) {
        audioBeatLightPrimaryColor = preset
        applyAudioBeatLightMode()
    }

    func setAudioBeatLightSecondaryColor(_ preset: AudioBeatLightColorPreset) {
        audioBeatLightSecondaryColor = preset
        applyAudioBeatLightMode()
    }

    func toggleRhythmTemplate(_ templateId: String) {
        selectedRhythmTemplateId = templateId
        let now = monotonicMilliseconds()
        let loopStart = rhythmLoopStartedAtMs ?? now
        rhythmLoopStartedAtMs = loopStart

        if let activeIndex = rhythmActiveLayers.firstIndex(where: { $0.template.id == templateId }) {
            rhythmActiveLayers.remove(at: activeIndex)
            statusText = "已关闭 \(title(forRhythmTemplateId: templateId))"
        } else if let template = rhythmTemplate(for: templateId) {
            let startedAtMs = max(0, now - loopStart)
            rhythmActiveLayers.append(RhythmActiveTemplate(template: template, startedAtMs: startedAtMs))
            statusText = "已开启 \(template.title)"
        }

        syncActiveRhythmTemplateIds()
        startRhythmLoopIfNeeded()
        if rhythmActiveLayers.isEmpty && !isRhythmRecording {
            stopRhythmLoopIfIdle()
        }
    }

    func stopRhythmTemplates() {
        rhythmActiveLayers.removeAll()
        syncActiveRhythmTemplateIds()
        statusText = "节奏已停止"
        if !isRhythmRecording {
            stopRhythmLoopIfIdle()
        }
    }

    func startRhythmRecording() {
        guard !isRhythmRecording else { return }
        let now = monotonicMilliseconds()
        if rhythmLoopStartedAtMs == nil {
            rhythmLoopStartedAtMs = now
        }
        rhythmRecordingStartedAtMs = now
        rhythmRecordedPoses = []
        rhythmRecordedPoseCount = 0
        isRhythmRecording = true
        recordRhythmPose(currentPose, elapsedMs: 0)
        statusText = "节奏录制中"
        startRhythmLoopIfNeeded()
    }

    func stopRhythmRecording() {
        guard isRhythmRecording else { return }
        let now = monotonicMilliseconds()
        let durationMs = min(
            ActionTimeline.maxDurationMs,
            max(1, now - (rhythmRecordingStartedAtMs ?? now))
        )
        let currentPoseFrame = PoseKeyframe(
            timeMs: durationMs,
            yaw: currentPose.yaw,
            pitch: currentPose.pitch,
            speed: currentPose.speed
        )
        if rhythmRecordedPoses.last?.timeMs != durationMs {
            rhythmRecordedPoses.append(currentPoseFrame)
        }

        let poses = rhythmRecordedPoses
            .filter { (0...durationMs).contains($0.timeMs) }
            .prefix(RobotActionValidator.maxPoseKeyframes)
        let action = RobotAction(
            id: "action.rhythm_\(rhythmActionIndex)",
            title: "节奏动作 \(rhythmActionIndex)",
            durationMs: durationMs,
            endMode: .home,
            poseKeyframes: Array(poses),
            faceEvents: []
        )
        do {
            try RobotActionValidator.validate(action)
            savedActions.append(action)
            rhythmActionIndex += 1
            statusText = "已保存 \(action.title)"
        } catch {
            statusText = "节奏动作保存失败"
        }

        isRhythmRecording = false
        rhythmRecordingStartedAtMs = nil
        rhythmRecordedPoses = []
        rhythmRecordedPoseCount = 0
        if rhythmActiveLayers.isEmpty {
            stopRhythmLoopIfIdle()
        }
    }

    func title(forRhythmTemplateId templateId: String) -> String {
        rhythmTemplate(for: templateId)?.title ?? templateId
    }

    func updateSelectedRhythmCenterYaw(_ value: Int) {
        updateSelectedRhythmTemplate { $0.centerYaw = min(max(value, -1280), 1280) }
    }

    func updateSelectedRhythmCenterPitch(_ value: Int) {
        updateSelectedRhythmTemplate { $0.centerPitch = min(max(value, 0), 900) }
    }

    func updateSelectedRhythmYawAmplitude(_ value: Int) {
        updateSelectedRhythmTemplate { $0.yawAmplitude = min(max(value, 0), 1280) }
    }

    func updateSelectedRhythmPitchAmplitude(_ value: Int) {
        updateSelectedRhythmTemplate { $0.pitchAmplitude = min(max(value, 0), 900) }
    }

    func updateSelectedRhythmCycleMs(_ value: Int) {
        updateSelectedRhythmTemplate { $0.cycleMs = min(max(value, 60), 2_500) }
    }

    func updateSelectedRhythmYawPhase(_ value: Double) {
        updateSelectedRhythmTemplate { $0.yawPhaseDegrees = min(max(value, 0), 360) }
    }

    func updateSelectedRhythmPitchPhase(_ value: Double) {
        updateSelectedRhythmTemplate { $0.pitchPhaseDegrees = min(max(value, 0), 360) }
    }

    func updateSelectedRhythmSpeed(_ value: Int) {
        updateSelectedRhythmTemplate { $0.speed = min(max(value, 0), 1000) }
    }

    func updateSelectedRhythmCurve(_ curve: RhythmMotionCurve) {
        updateSelectedRhythmTemplate { $0.curve = curve }
    }

    func resetSelectedRhythmTemplate() {
        guard let builtIn = BuiltInRhythmTemplateLibrary.templates.first(where: { $0.id == selectedRhythmTemplateId }) else {
            return
        }
        updateRhythmTemplate(builtIn)
        statusText = "已重置 \(builtIn.title)"
    }

    func previewLastAction() {
        guard let action = savedActions.last else {
            statusText = "没有可预览动作"
            return
        }
        preview(action)
    }

    func addSelectedActionClipToTimeline() {
        addActionClipToTimeline(clipId: selectedActionClipId)
    }

    func addActionClipToTimeline(clipId: String) {
        let startTimeMs = ActionTimelineSequencer.nextStart(after: timelineClips, clipLibrary: actionClips)
        let placement = ActionTimelineClip(
            clipId: clipId,
            startTimeMs: startTimeMs,
            speedScale: timelineSpeedScale
        )
        do {
            let sequenced = try ActionTimelineSequencer.sequencedClips(
                timelineClips + [placement],
                clipLibrary: actionClips
            )
            let draft = ActionTimeline(title: "组合动作", clips: sequenced, sounds: timelineSounds)
            _ = try ActionTimelineComposer.compose(draft, clipLibrary: actionClips, id: "action.timeline_draft")
            timelineClips = sequenced
            selectedTimelineItem = .clip(timelineClips.count - 1)
            selectedActionClipId = clipId
            timelineStartMs = nextTimelineStart()
            statusText = "已添加 \(title(forActionClipId: placement.clipId))"
        } catch {
            statusText = "动作总时长不能超过 15 秒"
        }
    }

    func removeTimelineClip(at index: Int) {
        guard timelineClips.indices.contains(index) else { return }
        timelineClips.remove(at: index)
        timelineClips = (try? ActionTimelineSequencer.sequencedClips(timelineClips, clipLibrary: actionClips)) ?? timelineClips
        timelineStartMs = nextTimelineStart()
        selectedTimelineItem = nil
        statusText = "已删除动作片段"
    }

    func selectTimelineClip(at index: Int) {
        guard timelineClips.indices.contains(index) else { return }
        selectedTimelineItem = .clip(index)
    }

    func setTimelineClipStart(at index: Int, startTimeMs: Int) {
        guard timelineClips.indices.contains(index) else { return }
        let clampedStart = min(max(startTimeMs, 0), ActionTimeline.maxDurationMs)
        let current = timelineClips[index]
        let updated = ActionTimelineClip(
            clipId: current.clipId,
            startTimeMs: clampedStart,
            speedScale: current.speedScale
        )
        updateTimelineClip(at: index, with: updated, failureText: "该开始时间会超过 15 秒")
    }

    func setTimelineClipSpeed(at index: Int, speedScale: Double) {
        guard timelineClips.indices.contains(index) else { return }
        let clampedScale = min(max(speedScale, 0.25), 4.0)
        let current = timelineClips[index]
        let updated = ActionTimelineClip(
            clipId: current.clipId,
            startTimeMs: current.startTimeMs,
            speedScale: clampedScale
        )
        updateTimelineClip(at: index, with: updated, failureText: "该速度会超过 15 秒")
    }

    private func updateTimelineClip(at index: Int, with updated: ActionTimelineClip, failureText: String) {
        var draftClips = timelineClips
        draftClips[index] = updated
        do {
            let sequenced = try ActionTimelineSequencer.sequencedClips(draftClips, clipLibrary: actionClips)
            let draft = ActionTimeline(title: "组合动作", clips: sequenced, sounds: timelineSounds)
            _ = try ActionTimelineComposer.compose(draft, clipLibrary: actionClips, id: "action.timeline_draft")
            timelineClips = sequenced
            selectedTimelineItem = .clip(index)
            timelineStartMs = nextTimelineStart()
        } catch {
            statusText = failureText
        }
    }

    func addSelectedSoundToTimeline() {
        let sound = SoundEvent(timeMs: timelineSoundTimeMs, assetId: selectedTimelineSoundAssetId)
        let draft = ActionTimeline(title: "组合动作", clips: timelineClips, sounds: timelineSounds + [sound])
        do {
            _ = try ActionTimelineComposer.compose(draft, id: "action.timeline_draft")
            timelineSounds.append(sound)
            selectedTimelineItem = .sound(timelineSounds.count - 1)
            statusText = "已添加音效"
        } catch {
            statusText = "音效时间不能超过 15 秒"
        }
    }

    func removeTimelineSound(at index: Int) {
        guard timelineSounds.indices.contains(index) else { return }
        timelineSounds.remove(at: index)
        selectedTimelineItem = nil
        statusText = "已删除音效"
    }

    func selectTimelineSound(at index: Int) {
        guard timelineSounds.indices.contains(index) else { return }
        selectedTimelineItem = .sound(index)
    }

    func setTimelineSoundTime(at index: Int, timeMs: Int) {
        guard timelineSounds.indices.contains(index) else { return }
        let current = timelineSounds[index]
        let clampedTime = min(max(timeMs, 0), ActionTimeline.maxDurationMs)
        let updated = SoundEvent(timeMs: clampedTime, assetId: current.assetId)
        var draftSounds = timelineSounds
        draftSounds[index] = updated
        let draft = ActionTimeline(title: "组合动作", clips: timelineClips, sounds: draftSounds)
        do {
            _ = try ActionTimelineComposer.compose(draft, id: "action.timeline_draft")
            timelineSounds = draftSounds
            selectedTimelineItem = .sound(index)
        } catch {
            statusText = "音效时间不能超过 15 秒"
        }
    }

    func title(forActionClipId clipId: String) -> String {
        actionClips.first { $0.id == clipId }?.title ?? clipId
    }

    func actionClip(for clipId: String) -> ActionClip? {
        actionClips.first { $0.id == clipId }
    }

    func timelineFrame(for placement: ActionTimelineClip) -> ActionTimelineTrackFrame? {
        guard let clip = actionClip(for: placement.clipId) else { return nil }
        return ActionTimelineTrackMetrics.frame(for: placement, clip: clip)
    }

    func previewTimelineAction() {
        do {
            let action = try buildTimelineAction(id: "action.timeline_preview", title: "组合预览")
            preview(action)
        } catch {
            statusText = "组合动作无效"
        }
    }

    func saveTimelineAction() {
        do {
            let action = try buildTimelineAction(
                id: "action.timeline_\(timelineActionIndex)",
                title: "组合动作 \(timelineActionIndex)"
            )
            savedActions.append(action)
            timelineActionIndex += 1
            statusText = "已保存 \(action.title)"
        } catch {
            statusText = "组合动作保存失败"
        }
    }

    private func preview(_ action: RobotAction) {
        previewTask?.cancel()
        let commands = PlaybackScheduler.commands(for: action)
        previewTask = Task {
            let started = monotonicMilliseconds()
            for command in commands {
                if Task.isCancelled { return }
                let due = started + command.timeMs
                let delay = max(0, due - monotonicMilliseconds())
                if delay > 0 {
                    try? await Task.sleep(for: .milliseconds(delay))
                }
                if Task.isCancelled { return }
                do {
                    try await client?.send(command.request)
                } catch {
                    statusText = "预览发送失败"
                    return
                }
            }
            statusText = "预览完成"
        }
    }

    private func buildTimelineAction(id: String, title: String) throws -> RobotAction {
        guard !timelineClips.isEmpty || !timelineSounds.isEmpty else {
            throw ActionTimelineComposerError.timelineTooLong
        }
        let sequenced = try ActionTimelineSequencer.sequencedClips(timelineClips, clipLibrary: actionClips)
        let timeline = ActionTimeline(title: title, clips: sequenced, sounds: timelineSounds)
        return try ActionTimelineComposer.compose(timeline, clipLibrary: actionClips, id: id)
    }

    private func nextTimelineStart() -> Int {
        ActionTimelineSequencer.nextStart(after: timelineClips, clipLibrary: actionClips)
    }

    func stopAll() {
        previewTask?.cancel()
        previewTask = nil
        stopAudioBeatControl()
        cancelRealtimeVoiceStreaming()
        if isRhythmRecording {
            stopRhythmRecording()
        }
        stopRhythmTemplates()
        isLiveControlEnabled = false
        pendingLivePose = nil
        if isRecording {
            stopRecording()
        }
        Task {
            do {
                try await client?.home()
                statusText = "已回正"
            } catch {
                statusText = "回正失败"
            }
        }
    }

    private func startRealtimeVoiceStreaming(
        with streamer: VoiceLiveAudioStreamer,
        apiClient: VolcengineRealtimeDirectClient,
        robotClient: RobotClient
    ) async {
        do {
            latestRealtimeVoiceStage = "connecting"
            realtimeVoiceStatusText = "连接实时语音"
            try await apiClient.start()
            latestRealtimeVoiceStage = "microphone"
            realtimeVoiceStatusText = "准备麦克风"
            try await streamer.start()
            guard liveVoiceStreamer === streamer else {
                streamer.stop()
                await apiClient.finish()
                return
            }
            isRealtimeVoiceStreaming = true
            latestRealtimeVoiceStage = "listening"
            realtimeVoiceStatusText = "实时收音中"
            statusText = "手机直连实时语音中"
            startChatMotionLoopIfNeeded()
            realtimeVoiceReceiveTask = Task { [weak self] in
                await self?.receiveRealtimeVoiceResponses(
                    apiClient: apiClient,
                    robotClient: robotClient
                )
            }
        } catch {
            if liveVoiceStreamer === streamer {
                liveVoiceStreamer = nil
            }
            isRealtimeVoiceStreaming = false
            realtimeVoiceClient = nil
            realtimeVoiceRobotClient = nil
            await apiClient.finish()
            realtimeVoiceStatusText = realtimeVoiceErrorText(error)
            statusText = realtimeVoiceStatusText
        }
    }

    private func enqueueRealtimeVoiceChunk(_ chunk: Data) {
        guard isRealtimeVoiceStreaming, !chunk.isEmpty else { return }
        if realtimeVoiceChunkQueue.count > 24 {
            realtimeVoiceChunkQueue.removeFirst(realtimeVoiceChunkQueue.count - 12)
        }
        realtimeVoiceChunkQueue.append(chunk)
        guard !isRealtimeVoiceFrameSending else { return }
        isRealtimeVoiceFrameSending = true
        Task {
            await flushRealtimeVoiceChunks()
        }
    }

    private func flushRealtimeVoiceChunks() async {
        defer { isRealtimeVoiceFrameSending = false }
        guard let client = realtimeVoiceClient else { return }

        while isRealtimeVoiceStreaming, !realtimeVoiceChunkQueue.isEmpty {
            let chunk = realtimeVoiceChunkQueue.removeFirst()
            do {
                latestRealtimeVoiceStage = "listening"
                try await client.sendAudioFrame(chunk)
                realtimeVoiceFrameCount += 1
                realtimeVoiceStatusText = "实时收音中"
                realtimeVoiceAudioText = realtimeVoiceAudioSummary()
            } catch {
                realtimeVoiceStatusText = "实时语音发送失败"
                statusText = "实时语音发送失败"
                latestRealtimeVoiceStage = "settling"
                break
            }
        }
    }

    private func receiveRealtimeVoiceResponses(
        apiClient: VolcengineRealtimeDirectClient,
        robotClient: RobotClient
    ) async {
        while isRealtimeVoiceStreaming, realtimeVoiceClient === apiClient {
            do {
                latestRealtimeVoiceStage = "thinking"
                let response = try await apiClient.receiveResponse()
                guard isRealtimeVoiceStreaming else { return }
                realtimeVoiceReplyText = response.chatText.isEmpty ? "-" : response.chatText
                latestRealtimeVoiceStage = "speaking"
                realtimeVoiceStatusText = "API已返回，机器人播放中"
                let playback = try await robotClient.playRealtimeVoice(
                    response.audioPcm16,
                    sampleRate: response.audioSampleRate,
                    audioFormat: response.audioFormat
                )
                applyRealtimeVoiceResponse(response, playback: playback)
                try? await Task.sleep(for: .milliseconds(realtimeVoiceSpeakingDelayMs(for: response)))
                latestRealtimeVoiceStage = "settling"
            } catch {
                guard isRealtimeVoiceStreaming, realtimeVoiceClient === apiClient else { return }
                realtimeVoiceStatusText = realtimeVoiceErrorText(error)
                statusText = realtimeVoiceStatusText
                latestRealtimeVoiceStage = "settling"
                stopRealtimeVoiceStreaming()
                return
            }
        }
    }

    private func cancelRealtimeVoiceStreaming() {
        liveVoiceStreamer?.stop()
        liveVoiceStreamer = nil
        realtimeVoiceReceiveTask?.cancel()
        realtimeVoiceReceiveTask = nil
        let client = realtimeVoiceClient
        realtimeVoiceClient = nil
        realtimeVoiceRobotClient = nil
        realtimeVoiceChunkQueue = []
        isRealtimeVoiceStreaming = false
        isRealtimeVoiceFrameSending = false
        stopChatMotionLoop()
        Task {
            await client?.finish()
        }
    }

    private func applyRealtimeVoiceResponse(
        _ response: VolcengineRealtimeMobileResponse,
        playback: RobotVoicePlaybackResult
    ) {
        realtimeVoiceAudioText = realtimeVoiceAudioSummary(response, playback: playback)
        if playback.ok {
            realtimeVoiceStatusText = "机器人播放中"
        } else {
            realtimeVoiceStatusText = playback.error ?? "API已返回，机器人播放失败"
        }
        statusText = realtimeVoiceStatusText
    }

    private func realtimeVoiceAudioSummary(
        _ response: VolcengineRealtimeMobileResponse? = nil,
        playback: RobotVoicePlaybackResult? = nil
    ) -> String {
        var parts = ["发送 \(realtimeVoiceFrameCount) 帧"]
        if let response, !response.audioPcm16.isEmpty {
            parts.append("API \(response.audioPcm16.count) bytes")
        }
        if let playback {
            parts.append(playback.ok ? "机器人 \(playback.uploadedBytes) bytes" : "机器人失败")
        }
        return parts.joined(separator: " / ")
    }

    private func realtimeVoiceSpeakingDelayMs(for response: VolcengineRealtimeMobileResponse) -> Int {
        guard response.audioSampleRate > 0, !response.audioPcm16.isEmpty else {
            return 900
        }
        let bytesPerSecond = response.audioSampleRate * 2
        let durationMs = response.audioPcm16.count * 1_000 / bytesPerSecond
        return min(max(durationMs, 900), 6_000)
    }

    private func realtimeVoiceErrorText(_ error: Error) -> String {
        switch error as? VoiceLiveAudioStreamer.StreamerError {
        case .permissionDenied:
            return "请允许麦克风权限"
        case .inputUnavailable:
            return "麦克风不可用"
        case nil:
            if let realtimeError = error as? VolcengineRealtimeError {
                return realtimeError.message
            }
            return "实时语音失败"
        }
    }

    private func startChatMotionLoopIfNeeded() {
        guard isChatMotionEnabled, isRealtimeVoiceStreaming, chatMotionTask == nil else { return }
        chatMotionTask = Task { [weak self] in
            await self?.runChatMotionLoop()
        }
    }

    private func stopChatMotionLoop() {
        chatMotionTask?.cancel()
        chatMotionTask = nil
        currentChatMotionState = .settling
        if isChatMotionEnabled {
            chatMotionSummaryText = isRealtimeVoiceStreaming ? "等待下一次聊天动作" : "等待实时语音"
        }
    }

    private func runChatMotionLoop() async {
        defer {
            chatMotionTask = nil
        }

        while !Task.isCancelled, isRealtimeVoiceStreaming, isChatMotionEnabled {
            guard let cue = ChatMotionPlanner.cue(
                isStreaming: isRealtimeVoiceStreaming,
                stage: latestRealtimeVoiceStage,
                style: chatMotionStyle,
                randomUnit: Double.random(in: 0...1)
            ) else {
                chatMotionSummaryText = "等待实时语音"
                return
            }

            currentChatMotionState = cue.state
            chatMotionSummaryText = "\(chatMotionStyle.title) · \(cue.state.title) · \(title(forActionClipId: cue.clipId))"
            try? await Task.sleep(for: .milliseconds(cue.nextDelayMs))
            guard !Task.isCancelled, isRealtimeVoiceStreaming, isChatMotionEnabled else { return }
            guard chatMotionCanPlay else { continue }
            await playChatMotionCue(cue)
        }
    }

    private var chatMotionCanPlay: Bool {
        client != nil
            && !isAudioBeatControlEnabled
            && rhythmActiveLayers.isEmpty
            && !isRhythmRecording
            && !isRecording
            && !isLiveControlEnabled
    }

    private func playChatMotionCue(_ cue: ChatMotionCue) async {
        guard cue.clipId == Self.chatMotionAssetId else { return }
        let timeline = ActionTimeline(
            title: "聊天状态",
            clips: [ActionTimelineClip(clipId: cue.clipId, startTimeMs: 0, speedScale: cue.speedScale)],
            sounds: []
        )
        do {
            let action = try ActionTimelineComposer.compose(
                timeline,
                clipLibrary: actionClips,
                id: "action.chat_motion",
                endMode: .hold
            )
            let started = monotonicMilliseconds()
            for command in PlaybackScheduler.commands(for: action) {
                if Task.isCancelled { return }
                let due = started + command.timeMs
                let delay = max(0, due - monotonicMilliseconds())
                if delay > 0 {
                    try? await Task.sleep(for: .milliseconds(delay))
                }
                if Task.isCancelled { return }
                try await client?.send(command.request)
            }
        } catch {
            chatMotionSummaryText = "聊天动作发送失败"
        }
    }

    private func rhythmTemplate(for templateId: String) -> RhythmMotionTemplate? {
        rhythmTemplates.first { $0.id == templateId }
    }

    private func updateSelectedRhythmTemplate(_ update: (inout RhythmMotionTemplate) -> Void) {
        guard var template = selectedRhythmTemplate else { return }
        update(&template)
        updateRhythmTemplate(template)
    }

    private func updateRhythmTemplate(_ template: RhythmMotionTemplate) {
        if let index = rhythmTemplates.firstIndex(where: { $0.id == template.id }) {
            rhythmTemplates[index] = template
        }
        for index in rhythmActiveLayers.indices where rhythmActiveLayers[index].template.id == template.id {
            rhythmActiveLayers[index].template = template
        }
    }

    private func syncActiveRhythmTemplateIds() {
        activeRhythmTemplateIds = Set(rhythmActiveLayers.map { $0.template.id })
    }

    private func startRhythmLoopIfNeeded() {
        guard rhythmLoopTask == nil else { return }
        if rhythmLoopStartedAtMs == nil {
            rhythmLoopStartedAtMs = monotonicMilliseconds()
        }
        if !isLiveControlEnabled {
            isLiveControlEnabled = true
            didEnableLiveControlForRhythm = true
        }
        rhythmLoopTask = Task { [weak self] in
            await self?.runRhythmLoop()
        }
    }

    private func stopRhythmLoopIfIdle() {
        guard rhythmActiveLayers.isEmpty, !isRhythmRecording else { return }
        rhythmLoopTask?.cancel()
        rhythmLoopTask = nil
        rhythmLoopStartedAtMs = nil
        if didEnableLiveControlForRhythm {
            isLiveControlEnabled = false
            didEnableLiveControlForRhythm = false
        }
    }

    private func runRhythmLoop() async {
        while !Task.isCancelled && (!rhythmActiveLayers.isEmpty || isRhythmRecording) {
            let now = monotonicMilliseconds()
            let loopStart = rhythmLoopStartedAtMs ?? now
            rhythmLoopStartedAtMs = loopStart
            let elapsedMs = max(0, now - loopStart)
            let pose = RhythmTemplateMixer.pose(
                atElapsedMs: elapsedMs,
                activeLayers: rhythmActiveLayers,
                basePose: ServoPose(yaw: 0, pitch: 120, speed: servoSpeed)
            )
            currentPose = pose
            syncManualPose(from: pose)
            queueLivePose(pose)

            if isRhythmRecording {
                let recordingElapsedMs = min(
                    ActionTimeline.maxDurationMs,
                    max(0, now - (rhythmRecordingStartedAtMs ?? now))
                )
                recordRhythmPose(pose, elapsedMs: recordingElapsedMs)
                if recordingElapsedMs >= ActionTimeline.maxDurationMs {
                    stopRhythmRecording()
                }
            }

            try? await Task.sleep(for: .milliseconds(RhythmPerformanceRenderer.defaultSampleIntervalMs))
        }
        rhythmLoopTask = nil
        stopRhythmLoopIfIdle()
    }

    private func recordRhythmPose(_ pose: ServoPose, elapsedMs: Int) {
        let clampedElapsedMs = min(max(elapsedMs, 0), ActionTimeline.maxDurationMs)
        let frame = PoseKeyframe(
            timeMs: clampedElapsedMs,
            yaw: pose.yaw,
            pitch: pose.pitch,
            speed: pose.speed
        )
        if rhythmRecordedPoses.last?.timeMs == clampedElapsedMs {
            rhythmRecordedPoses[rhythmRecordedPoses.count - 1] = frame
        } else {
            rhythmRecordedPoses.append(frame)
        }
        rhythmRecordedPoseCount = rhythmRecordedPoses.count
    }

    private func startAudioBeatControl() async {
        if isRhythmRecording {
            stopRhythmRecording()
        }
        stopRhythmTemplates()
        audioBeatLockedCenterPose = audioBeatCenterPoseForStart()
        updateAudioBeatMapperConfiguration()
        audioBeatMapper.reset()

        let monitor = AudioLevelMonitor { [weak self] level in
            self?.handleAudioBeatLevel(level)
        }
        audioLevelMonitor = monitor

        do {
            try await monitor.start()
            beginAudioBeatControlStatus()
        } catch AudioLevelMonitor.MonitorError.permissionDenied {
            audioLevelMonitor = nil
            statusText = "请允许麦克风权限"
        } catch {
            audioLevelMonitor = nil
            statusText = "麦克风启动失败"
        }
    }

    private func stopAudioBeatControl() {
        let shouldSendStandbyPose = isAudioBeatAutoReturnEnabled || isAudioBeatStandbyPoseEnabled
        let shouldRestoreAudioBeatLight = isAudioBeatLightEnabled
        audioLevelMonitor?.stop()
        audioLevelMonitor = nil
        isAudioBeatControlEnabled = false
        audioBeatLevel = 0
        audioBeatMapper.reset()
        audioBeatLockedCenterPose = nil
        audioBeatLightFlashToken += 1
        if shouldSendStandbyPose {
            sendAudioBeatStandbyPose()
        }
        if shouldRestoreAudioBeatLight {
            restoreAudioBeatStandbyLight()
        }
        if didEnableLiveControlForAudioBeat {
            isLiveControlEnabled = false
            didEnableLiveControlForAudioBeat = false
        }
        statusText = shouldSendStandbyPose ? "音乐节拍已停止，已回待机" : "音乐节拍已停止"
    }

    private func beginAudioBeatControlStatus() {
        isAudioBeatControlEnabled = true
        if !isLiveControlEnabled {
            isLiveControlEnabled = true
            didEnableLiveControlForAudioBeat = true
        }
        applyAudioBeatLightMode()
        statusText = "手机麦克风同步中"
    }

    private func handleAudioBeatLevel(_ level: Double) {
        guard isAudioBeatControlEnabled else { return }
        audioBeatLevel = level
        updateAudioBeatMapperConfiguration()

        let now = monotonicMilliseconds()
        var mapper = audioBeatMapper
        let state = mapper.map(level: level, atMs: now)
        audioBeatMapper = mapper
        currentPose = state.pose
        syncManualPose(from: state.pose)
        handleAudioBeatLighting(state: state, atMs: now)

        if isRecording && !isPaused {
            recorder.recordPose(state.pose, atMs: now)
        }
        queueLivePose(state.pose)
    }

    private func updateAudioBeatMapperConfiguration() {
        audioBeatMapper.update(configuration: makeAudioBeatConfiguration())
    }

    private func makeAudioBeatConfiguration() -> AudioBeatServoConfiguration {
        let centerPose = audioBeatCenterPose()
        return AudioBeatServoConfiguration(
            centerYaw: centerPose.yaw,
            centerPitch: centerPose.pitch,
            pitchAmplitude: audioBeatPitchAmplitude,
            yawAmplitude: audioBeatYawAmplitude,
            isPitchEnabled: isAudioBeatPitchEnabled,
            isYawEnabled: isAudioBeatYawEnabled,
            isPitchAmplitudeRandomized: isAudioBeatPitchAmplitudeRandomized,
            isYawAmplitudeRandomized: isAudioBeatYawAmplitudeRandomized,
            motionCurve: audioBeatMotionCurve,
            speed: audioBeatSpeed,
            isAdaptiveSpeedEnabled: isAudioBeatAdaptiveSpeedEnabled,
            adaptiveSlowSpeed: audioBeatSlowSpeed,
            adaptiveFastSpeed: audioBeatFastSpeed,
            sensitivity: audioBeatSensitivity,
            releaseMs: 180,
            minimumBeatSpacingMs: 140
        )
    }

    private var audioBeatStandbyPose: ServoPose {
        ServoPose(yaw: 0, pitch: 120, speed: audioBeatSpeed)
    }

    private func audioBeatCenterPoseForStart() -> ServoPose {
        if isAudioBeatStandbyPoseEnabled {
            return audioBeatStandbyPose
        }
        return currentPose
    }

    private func audioBeatCenterPose() -> ServoPose {
        if isAudioBeatStandbyPoseEnabled {
            return audioBeatStandbyPose
        }
        if isAudioBeatDriftAllowed {
            return currentPose
        }
        return audioBeatLockedCenterPose ?? currentPose
    }

    private func sendAudioBeatStandbyPose() {
        let pose = audioBeatStandbyPose
        currentPose = pose
        syncManualPose(from: pose)
        if isLowLatencyStreamEnabled, isMotionStreamAvailable, motionStreamClient.send(pose) {
            return
        }
        Task {
            do {
                try await client?.setPose(pose)
            } catch {
                statusText = "回待机失败"
            }
        }
    }

    private func restoreAudioBeatStandbyLight() {
        Task {
            do {
                try await client?.restoreStandbyRGB()
            } catch {
                statusText = "待机灯光恢复失败"
            }
        }
    }

    private func applyAudioBeatLightMode() {
        guard isAudioBeatLightEnabled else { return }
        switch audioBeatLightMode {
        case .beatFlash:
            sendAudioBeatRGB(
                color: audioBeatLightSecondaryColor.hex,
                brightness: max(8, audioBeatLightBrightness / 6),
                breathing: false,
                marquee: false,
                rainbow: false,
                preset: "manual"
            )
        case .marquee:
            sendAudioBeatRGB(
                brightness: audioBeatLightBrightness,
                breathing: false,
                marquee: true,
                rainbow: false,
                preset: "rainbowMarquee"
            )
        case .breathing:
            sendAudioBeatRGB(
                color: audioBeatLightPrimaryColor.hex,
                brightness: audioBeatLightBrightness,
                breathing: true,
                marquee: false,
                rainbow: false,
                preset: "manual"
            )
        case .rainbow:
            sendAudioBeatRGB(
                brightness: audioBeatLightBrightness,
                breathing: false,
                marquee: false,
                rainbow: true,
                preset: "rainbowStatic"
            )
        case .custom:
            sendAudioBeatRGB(
                color: audioBeatLightPrimaryColor.hex,
                side: "left",
                brightness: audioBeatLightBrightness,
                breathing: false,
                marquee: false,
                rainbow: false,
                preset: "manual"
            )
            sendAudioBeatRGB(
                color: audioBeatLightSecondaryColor.hex,
                side: "right",
                brightness: audioBeatLightBrightness,
                breathing: false,
                marquee: false,
                rainbow: false,
                preset: "manual"
            )
        }
    }

    private func handleAudioBeatLighting(state: AudioBeatServoState, atMs now: Int) {
        guard isAudioBeatLightEnabled, audioBeatLightMode == .beatFlash, state.isBeat else { return }
        guard now - lastAudioBeatLightSentAtMs >= 90 else { return }
        lastAudioBeatLightSentAtMs = now
        audioBeatLightFlashToken += 1
        let token = audioBeatLightFlashToken
        let brightness = min(
            255,
            max(24, Int((Double(audioBeatLightBrightness) * max(state.normalizedLevel, 0.35)).rounded()))
        )
        sendAudioBeatRGB(
            color: audioBeatLightPrimaryColor.hex,
            brightness: brightness,
            breathing: false,
            marquee: false,
            rainbow: false,
            preset: "manual"
        )
        Task {
            try? await Task.sleep(for: .milliseconds(90))
            await MainActor.run {
                guard self.audioBeatLightFlashToken == token else { return }
                guard self.isAudioBeatLightEnabled, self.audioBeatLightMode == .beatFlash else { return }
                self.sendAudioBeatRGB(
                    color: self.audioBeatLightSecondaryColor.hex,
                    brightness: max(8, self.audioBeatLightBrightness / 6),
                    breathing: false,
                    marquee: false,
                    rainbow: false,
                    preset: "manual"
                )
            }
        }
    }

    private func sendAudioBeatRGB(
        color: String? = nil,
        side: String? = nil,
        brightness: Int? = nil,
        breathing: Bool? = nil,
        marquee: Bool? = nil,
        rainbow: Bool? = nil,
        preset: String? = nil
    ) {
        Task {
            do {
                try await client?.setRGB(
                    color: color,
                    side: side,
                    brightness: brightness,
                    breathing: breathing,
                    marquee: marquee,
                    rainbow: rainbow,
                    preset: preset,
                    temporary: true
                )
            } catch {
                statusText = "灯光发送失败"
            }
        }
    }

    private func queueLivePose(_ pose: ServoPose) {
        if isLowLatencyStreamEnabled, isMotionStreamAvailable, motionStreamClient.send(pose) {
            return
        }
        pendingLivePose = pose
        guard !isSendingLivePose else { return }
        isSendingLivePose = true
        Task {
            await drainLivePoseQueue()
        }
    }

    private func drainLivePoseQueue() async {
        while isLiveControlEnabled, let pose = pendingLivePose {
            pendingLivePose = nil
            let elapsedMs = monotonicMilliseconds() - lastPoseSentAtMs
            let waitMs = max(0, motionTuning.liveSendIntervalMs - elapsedMs)
            if waitMs > 0 {
                try? await Task.sleep(for: .milliseconds(waitMs))
            }

            guard isLiveControlEnabled else { break }
            lastPoseSentAtMs = monotonicMilliseconds()
            do {
                try await client?.setPose(pose)
            } catch {
                statusText = "姿态发送失败"
                pendingLivePose = nil
                break
            }
        }
        isSendingLivePose = false
        if isLiveControlEnabled, pendingLivePose != nil {
            queueLivePose(pendingLivePose!)
        }
    }

    private func syncManualPose(from pose: ServoPose) {
        manualPoseEditor = ManualPoseEditor(pose: ServoPose(yaw: pose.yaw, pitch: pose.pitch, speed: servoSpeed))
        manualYaw = manualPoseEditor.pose.yaw
        manualPitch = manualPoseEditor.pose.pitch
    }

    private func syncCurrentPoseFromManualEditor() {
        currentPose = manualPoseEditor.pose
        manualYaw = currentPose.yaw
        manualPitch = currentPose.pitch
    }

    private func queueManualPoseIfLive() {
        guard poseInputMode == .manual, isLiveControlEnabled else { return }
        queueLivePose(currentPose)
    }

    private func updateMotionStreamAvailability(using robotClient: RobotClient, baseURL: URL) async {
        isMotionStreamAvailable = false
        motionStreamClient.close()
        do {
            let data = try await robotClient.motionStreamStatus()
            let body = String(data: data, encoding: .utf8) ?? ""
            guard body.contains("\"ok\":true"), body.contains("\"port\":") else { return }
            motionStreamClient.configure(baseURL: baseURL)
            isMotionStreamAvailable = true
        } catch {
            motionStreamClient.close()
        }
    }

    private func makeMotionMapperConfiguration() -> MotionMapperConfiguration {
        let tuning = motionTuning
        return MotionMapperConfiguration(
            deadZoneRadians: tuning.deadZoneRadians,
            smoothingAlpha: tuning.smoothingAlpha,
            speed: servoSpeed
        )
    }
}

private final class VoiceLiveAudioStreamer {
    enum StreamerError: Error {
        case permissionDenied
        case inputUnavailable
    }

    private let engine = AVAudioEngine()
    private let chunker: VoiceLiveAudioChunker
    private let onChunk: @Sendable (Data) -> Void
    private let targetSampleRate = 16_000
    private var hasInstalledTap = false

    init(chunkDurationMs: Int = 120, onChunk: @escaping @Sendable (Data) -> Void) {
        let targetBytes = max(640, targetSampleRate * 2 * max(60, min(chunkDurationMs, 320)) / 1_000)
        self.chunker = VoiceLiveAudioChunker(targetByteCount: targetBytes)
        self.onChunk = onChunk
    }

    @MainActor
    func start() async throws {
        guard await requestMicrophonePermission() else {
            throw StreamerError.permissionDenied
        }

        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playAndRecord, mode: .spokenAudio, options: [.mixWithOthers, .defaultToSpeaker, .allowBluetoothHFP])
            try session.setActive(true)
            guard session.isInputAvailable else {
                throw StreamerError.inputUnavailable
            }

            let inputNode = engine.inputNode
            removeTapIfNeeded(from: inputNode)
            let inputFormat = inputNode.inputFormat(forBus: 0)
            chunker.reset()

            inputNode.installTap(
                onBus: 0,
                bufferSize: 4_096,
                format: nil,
                block: Self.makeTapBlock(
                    chunker: chunker,
                    sourceSampleRate: inputFormat.sampleRate > 0 ? inputFormat.sampleRate : session.sampleRate,
                    targetSampleRate: Double(targetSampleRate),
                    onChunk: onChunk
                )
            )
            hasInstalledTap = true

            engine.prepare()
            try engine.start()
        } catch {
            stopEngine()
            throw error
        }
    }

    @MainActor
    func stop() {
        stopEngine()
        chunker.reset()
    }

    @MainActor
    private func requestMicrophonePermission() async -> Bool {
        switch AVAudioApplication.shared.recordPermission {
        case .granted:
            return true
        case .denied:
            return false
        case .undetermined:
            return await withCheckedContinuation { continuation in
                AVAudioApplication.requestRecordPermission { granted in
                    continuation.resume(returning: granted)
                }
            }
        @unknown default:
            return false
        }
    }

    @MainActor
    private func stopEngine() {
        removeTapIfNeeded(from: engine.inputNode)
        engine.stop()
        try? AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
    }

    @MainActor
    private func removeTapIfNeeded(from inputNode: AVAudioInputNode) {
        if hasInstalledTap {
            inputNode.removeTap(onBus: 0)
            hasInstalledTap = false
        }
    }

    private nonisolated static func makeTapBlock(
        chunker: VoiceLiveAudioChunker,
        sourceSampleRate: Double,
        targetSampleRate: Double,
        onChunk: @escaping @Sendable (Data) -> Void
    ) -> AVAudioNodeTapBlock {
        { pcmBuffer, _ in
            let samples = Self.monoSamples(from: pcmBuffer)
            guard !samples.isEmpty else { return }
            let resampled = Self.resample(
                samples,
                sourceSampleRate: pcmBuffer.format.sampleRate > 0 ? pcmBuffer.format.sampleRate : sourceSampleRate,
                targetSampleRate: targetSampleRate
            )
            let pcm16 = Self.pcm16Data(from: resampled)
            for chunk in chunker.append(pcm16) {
                onChunk(chunk)
            }
        }
    }

    private nonisolated static func monoSamples(from buffer: AVAudioPCMBuffer) -> [Float] {
        guard let channels = buffer.floatChannelData else { return [] }
        let channelCount = Int(buffer.format.channelCount)
        let frameCount = Int(buffer.frameLength)
        guard channelCount > 0, frameCount > 0 else { return [] }

        var samples = Array(repeating: Float(0), count: frameCount)
        for channelIndex in 0..<channelCount {
            let channel = channels[channelIndex]
            for frameIndex in 0..<frameCount {
                samples[frameIndex] += channel[frameIndex]
            }
        }

        if channelCount > 1 {
            let divisor = Float(channelCount)
            for index in samples.indices {
                samples[index] /= divisor
            }
        }
        return samples
    }

    private nonisolated static func resample(
        _ samples: [Float],
        sourceSampleRate: Double,
        targetSampleRate: Double
    ) -> [Float] {
        guard !samples.isEmpty else { return [] }
        guard sourceSampleRate > 0, targetSampleRate > 0 else { return samples }
        guard abs(sourceSampleRate - targetSampleRate) > 0.5 else { return samples }

        let targetCount = max(1, Int((Double(samples.count) * targetSampleRate / sourceSampleRate).rounded()))
        guard samples.count > 1, targetCount > 1 else {
            return Array(samples.prefix(targetCount))
        }

        let ratio = sourceSampleRate / targetSampleRate
        var output: [Float] = []
        output.reserveCapacity(targetCount)
        for targetIndex in 0..<targetCount {
            let sourcePosition = Double(targetIndex) * ratio
            let lowerIndex = min(Int(sourcePosition), samples.count - 1)
            let upperIndex = min(lowerIndex + 1, samples.count - 1)
            let fraction = Float(sourcePosition - Double(lowerIndex))
            let lower = samples[lowerIndex]
            let upper = samples[upperIndex]
            output.append(lower + (upper - lower) * fraction)
        }
        return output
    }

    private nonisolated static func pcm16Data(from samples: [Float]) -> Data {
        var data = Data()
        data.reserveCapacity(samples.count * 2)
        for sample in samples {
            let clipped = min(max(sample, -1), 1)
            var value = Int16((clipped * Float(Int16.max)).rounded()).littleEndian
            withUnsafeBytes(of: &value) { bytes in
                data.append(contentsOf: bytes)
            }
        }
        return data
    }
}

private final class VoiceLiveAudioChunker: @unchecked Sendable {
    private let lock = NSLock()
    private var pending = Data()
    private let targetByteCount: Int

    init(targetByteCount: Int) {
        self.targetByteCount = max(640, targetByteCount)
    }

    func reset() {
        lock.lock()
        pending.removeAll(keepingCapacity: true)
        lock.unlock()
    }

    func append(_ bytes: Data) -> [Data] {
        lock.lock()
        pending.append(bytes)
        var chunks: [Data] = []
        while pending.count >= targetByteCount {
            chunks.append(Data(pending.prefix(targetByteCount)))
            pending.removeFirst(targetByteCount)
        }
        lock.unlock()
        return chunks
    }
}

private func monotonicMilliseconds() -> Int {
    Int((ProcessInfo.processInfo.systemUptime * 1000).rounded())
}

private func clampServoSpeed(_ speed: Int, fallback: Int) -> Int {
    let value = speed == 0 ? fallback : speed
    return min(max(value, 0), 1000)
}
