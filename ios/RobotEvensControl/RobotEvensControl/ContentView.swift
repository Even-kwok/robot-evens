import SwiftUI

struct ContentView: View {
    @StateObject private var viewModel = AppViewModel()
    private let poseSource = CoreMotionPoseSource()
    private let timelineTrackWidth: CGFloat = 840

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    connectionSection
                    creatorStudioSection
                    realtimeVoiceSection
                    motionSection
                    soundSection
                    audioBeatSection
                    expressionSection
                    rhythmSection
                    recordingSection
                    timelineSection
                    savedActionSection
                }
                .padding()
            }
            .navigationTitle("Robot Evens")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        viewModel.stopAll()
                    } label: {
                        Image(systemName: "stop.fill")
                    }
                    .accessibilityLabel("停止")
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color(.systemBackground))
        .task {
            await viewModel.connectToDefaultIfNeeded()
        }
        .onChange(of: viewModel.motionSourceToken) { _, _ in
            if viewModel.needsMotionUpdates {
                poseSource.start(updateInterval: viewModel.motionUpdateInterval) { attitude in
                    viewModel.handle(attitude: attitude)
                }
            } else {
                poseSource.stop()
            }
        }
    }

    private var connectionSection: some View {
        GroupBox("连接") {
            VStack(alignment: .leading, spacing: 12) {
                TextField("http://机器人IP", text: $viewModel.robotBaseURLString)
                    .textInputAutocapitalization(.never)
                    .autocorrectionDisabled()
                    .keyboardType(.URL)
                    .textFieldStyle(.roundedBorder)
                HStack {
                    Button {
                        Task { await viewModel.connect() }
                    } label: {
                        Label("连接", systemImage: "wifi")
                    }
                    Button {
                        viewModel.calibrate()
                    } label: {
                        Label("校准", systemImage: "scope")
                    }
                }
                Text(viewModel.statusText)
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }
        }
    }

    private var creatorStudioSection: some View {
        NavigationLink {
            CreatorStudioView(robotBaseURLString: $viewModel.robotBaseURLString)
        } label: {
            HStack(spacing: 14) {
                Image(systemName: "wand.and.stars")
                    .font(.title2)
                    .frame(width: 42, height: 42)
                    .background(.tint, in: RoundedRectangle(cornerRadius: 12))
                    .foregroundStyle(.white)
                VStack(alignment: .leading, spacing: 3) {
                    Text("创作工作室")
                        .font(.headline)
                    Text("生成情绪声音 · 捏眼形 · 参考图生成")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                Image(systemName: "chevron.right")
                    .foregroundStyle(.tertiary)
            }
            .padding(12)
            .background(Color(.secondarySystemBackground), in: RoundedRectangle(cornerRadius: 14))
        }
        .buttonStyle(.plain)
        .accessibilityIdentifier("creatorStudioLink")
    }

    private var realtimeVoiceSection: some View {
        GroupBox("实时语音") {
            VStack(alignment: .leading, spacing: 12) {
                Label("手机直连机器人", systemImage: "iphone.radiowaves.left.and.right")
                    .font(.footnote)
                    .foregroundStyle(.secondary)

                HStack {
                    Button {
                        viewModel.startRealtimeVoiceStreaming()
                    } label: {
                        Label("开始实时收音", systemImage: "mic.circle")
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(viewModel.isRealtimeVoiceStreaming || viewModel.isRealtimeVoiceFrameSending)

                    Button {
                        viewModel.stopRealtimeVoiceStreaming()
                    } label: {
                        Label("停止实时收音", systemImage: "stop.circle.fill")
                    }
                    .buttonStyle(.bordered)
                    .disabled(!viewModel.isRealtimeVoiceStreaming)
                }

                HStack {
                    Label(viewModel.realtimeVoiceStatusText, systemImage: "waveform")
                        .lineLimit(1)
                        .frame(minHeight: 20, alignment: .leading)
                        .frame(maxWidth: .infinity, alignment: .leading)
                    Spacer()
                    ProgressView()
                        .controlSize(.small)
                        .frame(width: 20, height: 20)
                        .opacity(viewModel.isRealtimeVoiceFrameSending ? 1 : 0)
                        .accessibilityHidden(!viewModel.isRealtimeVoiceFrameSending)
                }
                .font(.footnote)
                .foregroundStyle(.secondary)

                chatMotionControls

                VStack(alignment: .leading, spacing: 4) {
                    Label("已发送 \(viewModel.realtimeVoiceFrameCount) 帧", systemImage: "dot.radiowaves.left.and.right")
                        .lineLimit(1)
                    Label(viewModel.realtimeVoiceReplyText, systemImage: "text.bubble")
                        .lineLimit(2)
                    Label(viewModel.realtimeVoiceAudioText, systemImage: "speaker.wave.2")
                        .lineLimit(1)
                }
                .font(.footnote)
                .foregroundStyle(.secondary)
                .frame(minHeight: 54, alignment: .topLeading)
                .frame(maxWidth: .infinity, alignment: .topLeading)
            }
        }
        .transaction { transaction in
            transaction.animation = nil
        }
    }

    private var chatMotionControls: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Toggle(isOn: Binding(
                    get: { viewModel.isChatMotionEnabled },
                    set: { viewModel.setChatMotionEnabled($0) }
                )) {
                    Label("聊天动作", systemImage: "bubble.left.and.bubble.right")
                }
                Spacer()
                Label(viewModel.currentChatMotionState.title, systemImage: "sparkles")
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            }

            Picker("通话风格", selection: Binding(
                get: { viewModel.chatMotionStyle },
                set: { viewModel.setChatMotionStyle($0) }
            )) {
                ForEach(ChatMotionStyle.allCases) { style in
                    Text(style.title).tag(style)
                }
            }
            .pickerStyle(.segmented)

            Label(viewModel.chatMotionSummaryText, systemImage: "waveform.path.ecg")
                .font(.footnote)
                .foregroundStyle(.secondary)
                .lineLimit(1)
        }
        .padding(.vertical, 2)
        .accessibilityElement(children: .contain)
        .accessibilityLabel("聊天状态")
    }

    private var motionSection: some View {
        GroupBox("姿态") {
            VStack(alignment: .leading, spacing: 12) {
                Picker("输入", selection: Binding(
                    get: { viewModel.poseInputMode },
                    set: { viewModel.setPoseInputMode($0) }
                )) {
                    ForEach(PoseInputMode.allCases) { mode in
                        Text(mode.title).tag(mode)
                    }
                }
                .pickerStyle(.segmented)

                Toggle(isOn: Binding(
                    get: { viewModel.isPrecisionModeEnabled },
                    set: { viewModel.setPrecisionModeEnabled($0) }
                )) {
                    Label("精细模式", systemImage: "scope")
                }

                Toggle(isOn: Binding(
                    get: { viewModel.isLowLatencyStreamEnabled },
                    set: { viewModel.setLowLatencyStreamEnabled($0) }
                )) {
                    Label("低延迟流式", systemImage: "dot.radiowaves.left.and.right")
                }
                .disabled(!viewModel.isMotionStreamAvailable)

                Toggle(isOn: Binding(
                    get: { viewModel.isLiveControlEnabled },
                    set: { _ in viewModel.toggleLiveControl() }
                )) {
                    Label("实时控制", systemImage: "gyroscope")
                }

                sliderRow(
                    title: "速度",
                    value: Binding(
                        get: { Double(viewModel.servoSpeed) },
                        set: { viewModel.setServoSpeed(Int($0.rounded())) }
                    ),
                    range: 80...1000,
                    step: 10,
                    systemImage: "speedometer"
                )

                HStack {
                    Label("Yaw \(viewModel.currentPose.yaw)", systemImage: "arrow.left.and.right")
                    Spacer()
                    Label("Pitch \(viewModel.currentPose.pitch)", systemImage: "arrow.up.and.down")
                }
                .font(.footnote.monospacedDigit())

                if viewModel.poseInputMode == .manual {
                    manualKeyframeControls
                }
            }
        }
    }

    private var manualKeyframeControls: some View {
        VStack(alignment: .leading, spacing: 12) {
            sliderRow(
                title: "Yaw",
                value: Binding(
                    get: { Double(viewModel.manualYaw) },
                    set: { viewModel.setManualYaw(Int($0.rounded())) }
                ),
                range: -1280...1280,
                step: 1,
                systemImage: "arrow.left.and.right"
            )
            sliderRow(
                title: "Pitch",
                value: Binding(
                    get: { Double(viewModel.manualPitch) },
                    set: { viewModel.setManualPitch(Int($0.rounded())) }
                ),
                range: 0...900,
                step: 1,
                systemImage: "arrow.up.and.down"
            )
            HStack {
                Button {
                    viewModel.sendCurrentManualPose()
                } label: {
                    Label("发送姿态", systemImage: "paperplane")
                }

                Button {
                    viewModel.insertManualKeyframe()
                } label: {
                    Label("插入关键帧", systemImage: "plus.square.on.square")
                }
                .disabled(!viewModel.isRecording || viewModel.isPaused)
            }
        }
    }

    private func sliderRow(
        title: String,
        value: Binding<Double>,
        range: ClosedRange<Double>,
        step: Double,
        systemImage: String
    ) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Label(title, systemImage: systemImage)
                Spacer()
                Text("\(Int(value.wrappedValue.rounded()))")
                    .font(.footnote.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
            Slider(
                value: value,
                in: range,
                step: step
            )
        }
    }

    private func ratioSliderRow(
        title: String,
        value: Binding<Double>,
        range: ClosedRange<Double>,
        step: Double,
        systemImage: String
    ) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Label(title, systemImage: systemImage)
                Spacer()
                Text("\(value.wrappedValue.formatted(.number.precision(.fractionLength(2))))x")
                    .font(.footnote.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
            Slider(
                value: value,
                in: range,
                step: step
            )
        }
    }

    private func percentageSliderRow(
        title: String,
        value: Binding<Double>,
        range: ClosedRange<Double>,
        step: Double,
        systemImage: String
    ) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Label(title, systemImage: systemImage)
                Spacer()
                Text("\(Int((value.wrappedValue * 100).rounded()))%")
                    .font(.footnote.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
            Slider(
                value: value,
                in: range,
                step: step
            )
        }
    }

    private var soundSection: some View {
        GroupBox("开场音效") {
            VStack(alignment: .leading, spacing: 12) {
                Picker("音效", selection: $viewModel.selectedStartSoundAssetId) {
                    ForEach(viewModel.startSoundChoices, id: \.self) { sound in
                        Text(sound).tag(sound)
                    }
                }
                Button {
                    viewModel.playSelectedStartSound()
                } label: {
                    Label("试听", systemImage: "speaker.wave.2.fill")
                }
            }
        }
    }

    private var audioBeatSection: some View {
        GroupBox("音乐节拍同步") {
            VStack(alignment: .leading, spacing: 12) {
                HStack {
                    Label(viewModel.audioBeatSummaryText, systemImage: "waveform")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                    Spacer()
                    Button {
                        viewModel.toggleAudioBeatControl()
                    } label: {
                        Label(
                            viewModel.isAudioBeatControlEnabled ? "停止" : "开启",
                            systemImage: viewModel.isAudioBeatControlEnabled ? "stop.circle" : "waveform.circle"
                        )
                    }
                    .buttonStyle(.borderedProminent)
                    .tint(viewModel.isAudioBeatControlEnabled ? .red : .accentColor)
                }

                ProgressView(value: viewModel.audioBeatLevel)
                    .progressViewStyle(.linear)

                Toggle(isOn: Binding(
                    get: { viewModel.isAudioBeatPitchEnabled },
                    set: { viewModel.setAudioBeatPitchEnabled($0) }
                )) {
                    Label("点头", systemImage: "arrow.up.and.down")
                }

                if viewModel.isAudioBeatPitchEnabled {
                    sliderRow(
                        title: "点头幅度",
                        value: Binding(
                            get: { Double(viewModel.audioBeatPitchAmplitude) },
                            set: { viewModel.setAudioBeatPitchAmplitude(Int($0.rounded())) }
                        ),
                        range: 0...420,
                        step: 5,
                        systemImage: "arrow.up.and.down"
                    )

                    Toggle(isOn: Binding(
                        get: { viewModel.isAudioBeatPitchAmplitudeRandomized },
                        set: { viewModel.setAudioBeatPitchAmplitudeRandomized($0) }
                    )) {
                        Label("点头幅度随机变化", systemImage: "sparkles")
                    }
                }

                Toggle(isOn: Binding(
                    get: { viewModel.isAudioBeatYawEnabled },
                    set: { viewModel.setAudioBeatYawEnabled($0) }
                )) {
                    Label("摇头", systemImage: "arrow.left.and.right")
                }

                if viewModel.isAudioBeatYawEnabled {
                    sliderRow(
                        title: "摇头幅度",
                        value: Binding(
                            get: { Double(viewModel.audioBeatYawAmplitude) },
                            set: { viewModel.setAudioBeatYawAmplitude(Int($0.rounded())) }
                        ),
                        range: 0...420,
                        step: 5,
                        systemImage: "arrow.left.and.right"
                    )

                    Toggle(isOn: Binding(
                        get: { viewModel.isAudioBeatYawAmplitudeRandomized },
                        set: { viewModel.setAudioBeatYawAmplitudeRandomized($0) }
                    )) {
                        Label("摇头幅度随机变化", systemImage: "sparkles")
                    }
                }

                percentageSliderRow(
                    title: "灵敏度",
                    value: Binding(
                        get: { viewModel.audioBeatSensitivity },
                        set: { viewModel.setAudioBeatSensitivity($0) }
                    ),
                    range: 0...1,
                    step: 0.05,
                    systemImage: "waveform.badge.magnifyingglass"
                )

                sliderRow(
                    title: "基础速度",
                    value: Binding(
                        get: { Double(viewModel.audioBeatSpeed) },
                        set: { viewModel.setAudioBeatSpeed(Int($0.rounded())) }
                    ),
                    range: 80...1000,
                    step: 10,
                    systemImage: "speedometer"
                )

                Toggle(isOn: Binding(
                    get: { viewModel.isAudioBeatAdaptiveSpeedEnabled },
                    set: { viewModel.setAudioBeatAdaptiveSpeedEnabled($0) }
                )) {
                    Label("跟随节拍调速", systemImage: "metronome")
                }

                if viewModel.isAudioBeatAdaptiveSpeedEnabled {
                    sliderRow(
                        title: "慢拍速度",
                        value: Binding(
                            get: { Double(viewModel.audioBeatSlowSpeed) },
                            set: { viewModel.setAudioBeatSlowSpeed(Int($0.rounded())) }
                        ),
                        range: 80...1000,
                        step: 10,
                        systemImage: "speedometer"
                    )

                    sliderRow(
                        title: "快拍速度",
                        value: Binding(
                            get: { Double(viewModel.audioBeatFastSpeed) },
                            set: { viewModel.setAudioBeatFastSpeed(Int($0.rounded())) }
                        ),
                        range: 80...1000,
                        step: 10,
                        systemImage: "speedometer"
                    )
                }

                Picker("线性风格", selection: Binding(
                    get: { viewModel.audioBeatMotionCurve },
                    set: { viewModel.setAudioBeatMotionCurve($0) }
                )) {
                    ForEach(RhythmMotionCurve.allCases, id: \.self) { curve in
                        Text(curve.title).tag(curve)
                    }
                }
                .pickerStyle(.menu)

                Toggle(isOn: Binding(
                    get: { viewModel.isAudioBeatStandbyPoseEnabled },
                    set: { viewModel.setAudioBeatStandbyPoseEnabled($0) }
                )) {
                    Label("保持待机状态", systemImage: "power")
                }

                Toggle(isOn: Binding(
                    get: { viewModel.isAudioBeatDriftAllowed },
                    set: { viewModel.setAudioBeatDriftAllowed($0) }
                )) {
                    Label("允许漂移", systemImage: "point.topleft.down.curvedto.point.bottomright.up")
                }
                .disabled(viewModel.isAudioBeatStandbyPoseEnabled)

                Toggle(isOn: Binding(
                    get: { viewModel.isAudioBeatAutoReturnEnabled },
                    set: { viewModel.setAudioBeatAutoReturnEnabled($0) }
                )) {
                    Label("自动回中", systemImage: "scope")
                }

                Divider()

                audioBeatLightControls
            }
        }
    }

    private var audioBeatLightControls: some View {
        VStack(alignment: .leading, spacing: 12) {
            Toggle(isOn: Binding(
                get: { viewModel.isAudioBeatLightEnabled },
                set: { viewModel.setAudioBeatLightEnabled($0) }
            )) {
                Label("灯光随音乐", systemImage: "lightbulb.led")
            }

            if viewModel.isAudioBeatLightEnabled {
                Picker("灯光模式", selection: Binding(
                    get: { viewModel.audioBeatLightMode },
                    set: { viewModel.setAudioBeatLightMode($0) }
                )) {
                    ForEach(AudioBeatLightMode.allCases) { mode in
                        Text(mode.title).tag(mode)
                    }
                }
                .pickerStyle(.menu)

                sliderRow(
                    title: "灯光亮度",
                    value: Binding(
                        get: { Double(viewModel.audioBeatLightBrightness) },
                        set: { viewModel.setAudioBeatLightBrightness(Int($0.rounded())) }
                    ),
                    range: 0...255,
                    step: 5,
                    systemImage: "sun.max"
                )

                Picker("主色", selection: Binding(
                    get: { viewModel.audioBeatLightPrimaryColor },
                    set: { viewModel.setAudioBeatLightPrimaryColor($0) }
                )) {
                    ForEach(AudioBeatLightColorPreset.allCases) { color in
                        Text(color.title).tag(color)
                    }
                }
                .pickerStyle(.menu)

                if viewModel.audioBeatLightMode == .beatFlash || viewModel.audioBeatLightMode == .custom {
                    Picker("副色", selection: Binding(
                        get: { viewModel.audioBeatLightSecondaryColor },
                        set: { viewModel.setAudioBeatLightSecondaryColor($0) }
                    )) {
                        ForEach(AudioBeatLightColorPreset.allCases) { color in
                            Text(color.title).tag(color)
                        }
                    }
                    .pickerStyle(.menu)
                }
            }
        }
    }

    private var expressionSection: some View {
        GroupBox("表情") {
            LazyVGrid(columns: [GridItem(.adaptive(minimum: 112), spacing: 8)], spacing: 8) {
                ForEach(viewModel.expressionMoods, id: \.self) { mood in
                    Button {
                        viewModel.triggerFace(mood)
                    } label: {
                        Label(mood, systemImage: symbol(for: mood))
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
    }

    private var rhythmSection: some View {
        GroupBox("节奏编辑器") {
            VStack(alignment: .leading, spacing: 12) {
                Label(viewModel.rhythmSummaryText, systemImage: "metronome")
                    .font(.footnote)
                    .foregroundStyle(.secondary)

                LazyVGrid(columns: [GridItem(.adaptive(minimum: 150), spacing: 8)], spacing: 8) {
                    ForEach(viewModel.rhythmTemplates) { template in
                        let isActive = viewModel.activeRhythmTemplateIds.contains(template.id)
                        Button {
                            viewModel.toggleRhythmTemplate(template.id)
                        } label: {
                            VStack(alignment: .leading, spacing: 4) {
                                Label(template.title, systemImage: rhythmSymbol(for: template.id))
                                    .font(.caption)
                                    .fontWeight(.semibold)
                                    .lineLimit(1)
                                    .minimumScaleFactor(0.85)
                                Text("\(template.cycleMs)ms · Y\(template.yawAmplitude) P\(template.pitchAmplitude)")
                                    .font(.caption2.monospacedDigit())
                                    .foregroundStyle(.secondary)
                                    .lineLimit(1)
                            }
                            .frame(maxWidth: .infinity, minHeight: 54, alignment: .leading)
                        }
                        .buttonStyle(.bordered)
                        .tint(isActive ? .accentColor : .gray)
                    }
                }

                HStack {
                    Button {
                        viewModel.startRhythmRecording()
                    } label: {
                        Label("录制节奏", systemImage: "record.circle")
                    }
                    .disabled(viewModel.isRhythmRecording)

                    Button {
                        viewModel.stopRhythmRecording()
                    } label: {
                        Label("保存节奏", systemImage: "checkmark.circle")
                    }
                    .disabled(!viewModel.isRhythmRecording)

                    Button(role: .destructive) {
                        viewModel.stopRhythmTemplates()
                    } label: {
                        Label("停止模板", systemImage: "stop.circle")
                    }
                    .disabled(viewModel.activeRhythmTemplateIds.isEmpty)
                }

                Divider()

                Picker("编辑模板", selection: $viewModel.selectedRhythmTemplateId) {
                    ForEach(viewModel.rhythmTemplates) { template in
                        Text(template.title).tag(template.id)
                    }
                }

                rhythmTemplateEditor
            }
        }
    }

    private var rhythmTemplateEditor: some View {
        VStack(alignment: .leading, spacing: 12) {
            sliderRow(
                title: "Yaw 中心",
                value: Binding(
                    get: { Double(viewModel.selectedRhythmTemplate?.centerYaw ?? 0) },
                    set: { viewModel.updateSelectedRhythmCenterYaw(Int($0.rounded())) }
                ),
                range: -1280...1280,
                step: 1,
                systemImage: "scope"
            )
            sliderRow(
                title: "Pitch 中心",
                value: Binding(
                    get: { Double(viewModel.selectedRhythmTemplate?.centerPitch ?? 120) },
                    set: { viewModel.updateSelectedRhythmCenterPitch(Int($0.rounded())) }
                ),
                range: 0...900,
                step: 1,
                systemImage: "scope"
            )
            sliderRow(
                title: "Yaw 幅度",
                value: Binding(
                    get: { Double(viewModel.selectedRhythmTemplate?.yawAmplitude ?? 0) },
                    set: { viewModel.updateSelectedRhythmYawAmplitude(Int($0.rounded())) }
                ),
                range: 0...1280,
                step: 1,
                systemImage: "arrow.left.and.right"
            )
            sliderRow(
                title: "Pitch 幅度",
                value: Binding(
                    get: { Double(viewModel.selectedRhythmTemplate?.pitchAmplitude ?? 0) },
                    set: { viewModel.updateSelectedRhythmPitchAmplitude(Int($0.rounded())) }
                ),
                range: 0...900,
                step: 1,
                systemImage: "arrow.up.and.down"
            )
            sliderRow(
                title: "周期 ms",
                value: Binding(
                    get: { Double(viewModel.selectedRhythmTemplate?.cycleMs ?? 500) },
                    set: { viewModel.updateSelectedRhythmCycleMs(Int($0.rounded())) }
                ),
                range: 60...2500,
                step: 10,
                systemImage: "timer"
            )
            sliderRow(
                title: "Yaw 相位",
                value: Binding(
                    get: { viewModel.selectedRhythmTemplate?.yawPhaseDegrees ?? 0 },
                    set: { viewModel.updateSelectedRhythmYawPhase($0) }
                ),
                range: 0...360,
                step: 5,
                systemImage: "dial.low"
            )
            sliderRow(
                title: "Pitch 相位",
                value: Binding(
                    get: { viewModel.selectedRhythmTemplate?.pitchPhaseDegrees ?? 0 },
                    set: { viewModel.updateSelectedRhythmPitchPhase($0) }
                ),
                range: 0...360,
                step: 5,
                systemImage: "dial.high"
            )
            sliderRow(
                title: "模板速度",
                value: Binding(
                    get: { Double(viewModel.selectedRhythmTemplate?.speed ?? viewModel.servoSpeed) },
                    set: { viewModel.updateSelectedRhythmSpeed(Int($0.rounded())) }
                ),
                range: 0...1000,
                step: 10,
                systemImage: "speedometer"
            )

            Picker("曲线", selection: Binding(
                get: { viewModel.selectedRhythmTemplate?.curve ?? .easeInOut },
                set: { viewModel.updateSelectedRhythmCurve($0) }
            )) {
                ForEach(RhythmMotionCurve.allCases, id: \.self) { curve in
                    Text(curve.title).tag(curve)
                }
            }
            .pickerStyle(.segmented)

            Button {
                viewModel.resetSelectedRhythmTemplate()
            } label: {
                Label("重置当前模板", systemImage: "arrow.counterclockwise")
            }
        }
    }

    private var recordingSection: some View {
        GroupBox("录制") {
            HStack {
                Button {
                    viewModel.startRecording()
                } label: {
                    Label("录制", systemImage: "record.circle")
                }
                .disabled(viewModel.isRecording)

                Button {
                    viewModel.pauseOrResumeRecording()
                } label: {
                    Label(viewModel.isPaused ? "继续" : "暂停", systemImage: viewModel.isPaused ? "play.fill" : "pause.fill")
                }
                .disabled(!viewModel.isRecording)

                Button {
                    viewModel.stopRecording()
                } label: {
                    Label("保存", systemImage: "checkmark.circle")
                }
                .disabled(!viewModel.isRecording)
            }
        }
    }

    private var timelineSection: some View {
        GroupBox("组合动作") {
            VStack(alignment: .leading, spacing: 12) {
                Label("动作包", systemImage: "rectangle.stack.badge.plus")
                    .font(.subheadline)

                ratioSliderRow(
                    title: "默认倍率",
                    value: Binding(
                        get: { viewModel.timelineSpeedScale },
                        set: { viewModel.timelineSpeedScale = $0 }
                    ),
                    range: 0.25...4.0,
                    step: 0.25,
                    systemImage: "gauge.with.dots.needle.67percent"
                )

                actionClipPalette

                Divider()

                Picker("音效", selection: $viewModel.selectedTimelineSoundAssetId) {
                    ForEach(viewModel.startSoundChoices, id: \.self) { sound in
                        Text(sound).tag(sound)
                    }
                }

                sliderRow(
                    title: "音效 ms",
                    value: Binding(
                        get: { Double(viewModel.timelineSoundTimeMs) },
                        set: { viewModel.timelineSoundTimeMs = Int($0.rounded()) }
                    ),
                    range: 0...Double(ActionTimeline.maxDurationMs),
                    step: 100,
                    systemImage: "music.note"
                )

                Button {
                    viewModel.addSelectedSoundToTimeline()
                } label: {
                    Label("添加音效", systemImage: "speaker.wave.2")
                }

                timelineTrack
                selectedTimelineEditor

                HStack {
                    Button {
                        viewModel.previewTimelineAction()
                    } label: {
                        Label("预览组合", systemImage: "play.circle")
                    }
                    .disabled(viewModel.timelineClips.isEmpty && viewModel.timelineSounds.isEmpty)

                    Button {
                        viewModel.saveTimelineAction()
                    } label: {
                        Label("保存组合", systemImage: "square.and.arrow.down")
                    }
                    .disabled(viewModel.timelineClips.isEmpty && viewModel.timelineSounds.isEmpty)
                }
            }
        }
    }

    private var actionClipPalette: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(viewModel.actionClips, id: \.id) { clip in
                    Button {
                        viewModel.addActionClipToTimeline(clipId: clip.id)
                    } label: {
                        VStack(alignment: .leading, spacing: 4) {
                            Label(clip.title, systemImage: clipSymbol(for: clip.id))
                                .font(.caption)
                                .fontWeight(.semibold)
                                .lineLimit(1)
                            Text("\(clip.durationMs)ms")
                                .font(.caption2.monospacedDigit())
                                .foregroundStyle(.secondary)
                        }
                        .frame(width: 142, height: 48, alignment: .leading)
                    }
                    .buttonStyle(.bordered)
                }
            }
            .padding(.vertical, 2)
        }
    }

    private var timelineTrack: some View {
        VStack(alignment: .leading, spacing: 8) {
            Label(viewModel.timelineSummaryText, systemImage: "rectangle.stack.badge.play")
                .font(.footnote)
                .foregroundStyle(.secondary)

            ScrollView(.horizontal, showsIndicators: true) {
                VStack(alignment: .leading, spacing: 8) {
                    timelineRuler
                    timelineActionTrack
                    timelineSoundTrack
                }
                .frame(width: timelineTrackWidth, alignment: .leading)
                .padding(.vertical, 4)
            }
        }
    }

    private var timelineRuler: some View {
        ZStack(alignment: .topLeading) {
            Rectangle()
                .fill(Color(.separator))
                .frame(height: 1)
                .offset(y: 18)

            ForEach(0...15, id: \.self) { second in
                let x = timelineX(forMs: second * 1000)
                VStack(spacing: 2) {
                    Rectangle()
                        .fill(Color.secondary)
                        .frame(width: 1, height: second % 5 == 0 ? 14 : 8)
                    Text("\(second)s")
                        .font(.caption2.monospacedDigit())
                        .foregroundStyle(.secondary)
                }
                .frame(width: 34)
                .position(x: x, y: 18)
            }
        }
        .frame(width: timelineTrackWidth, height: 34)
    }

    private var timelineActionTrack: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("动作轨")
                .font(.caption)
                .foregroundStyle(.secondary)
            ZStack(alignment: .leading) {
                trackBackground(height: 54)

                ForEach(Array(viewModel.timelineClips.enumerated()), id: \.offset) { index, placement in
                    if let frame = viewModel.timelineFrame(for: placement) {
                        let width = max(56, timelineTrackWidth * CGFloat(frame.widthFraction))
                        let x = timelineX(forMs: placement.startTimeMs) + width / 2
                        Button {
                            viewModel.selectTimelineClip(at: index)
                        } label: {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(viewModel.title(forActionClipId: placement.clipId))
                                    .font(.caption)
                                    .fontWeight(.semibold)
                                    .lineLimit(1)
                                Text("\(placement.startTimeMs)ms · \(placement.speedScale.formatted(.number.precision(.fractionLength(2))))x")
                                    .font(.caption2.monospacedDigit())
                                    .lineLimit(1)
                            }
                            .foregroundStyle(.white)
                            .padding(.horizontal, 8)
                            .frame(width: width, height: 42, alignment: .leading)
                            .background(isSelectedClip(index) ? Color.teal : Color.blue)
                            .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
                        }
                        .buttonStyle(.plain)
                        .position(x: min(x, timelineTrackWidth - width / 2), y: 27)
                    }
                }
            }
            .frame(width: timelineTrackWidth, height: 54)
        }
    }

    private var timelineSoundTrack: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("音效轨")
                .font(.caption)
                .foregroundStyle(.secondary)
            ZStack(alignment: .leading) {
                trackBackground(height: 40)

                ForEach(Array(viewModel.timelineSounds.enumerated()), id: \.offset) { index, sound in
                    let markerWidth: CGFloat = 58
                    let x = timelineX(forMs: sound.timeMs) + markerWidth / 2
                    Button {
                        viewModel.selectTimelineSound(at: index)
                    } label: {
                        HStack(spacing: 4) {
                            Image(systemName: "waveform")
                            Text(shortSoundName(sound.assetId))
                                .lineLimit(1)
                        }
                        .font(.caption2)
                        .foregroundStyle(.white)
                        .frame(width: markerWidth, height: 28)
                        .background(isSelectedSound(index) ? Color.orange : Color.green)
                        .clipShape(RoundedRectangle(cornerRadius: 6, style: .continuous))
                    }
                    .buttonStyle(.plain)
                    .position(x: min(x, timelineTrackWidth - markerWidth / 2), y: 20)
                }
            }
            .frame(width: timelineTrackWidth, height: 40)
        }
    }

    private func trackBackground(height: CGFloat) -> some View {
        ZStack(alignment: .leading) {
            RoundedRectangle(cornerRadius: 6, style: .continuous)
                .fill(Color(.secondarySystemFill))
            ForEach(0...15, id: \.self) { second in
                Rectangle()
                    .fill(second % 5 == 0 ? Color(.separator) : Color(.separator).opacity(0.45))
                    .frame(width: 1, height: height)
                    .offset(x: timelineX(forMs: second * 1000))
            }
        }
        .frame(width: timelineTrackWidth, height: height)
    }

    private var selectedTimelineEditor: some View {
        Group {
            if let selected = viewModel.selectedTimelineClip {
                VStack(alignment: .leading, spacing: 10) {
                    Label(viewModel.title(forActionClipId: selected.clip.clipId), systemImage: "slider.horizontal.3")
                        .font(.subheadline)
                    if let frame = viewModel.timelineFrame(for: selected.clip) {
                        LazyVGrid(columns: [GridItem(.adaptive(minimum: 96), spacing: 8)], spacing: 8) {
                            timelineDetailBadge(
                                title: "开始",
                                value: "\(selected.clip.startTimeMs)ms",
                                systemImage: "arrow.right.to.line"
                            )
                            timelineDetailBadge(
                                title: "结束",
                                value: "\(selected.clip.startTimeMs + frame.durationMs)ms",
                                systemImage: "arrow.left.to.line"
                            )
                            timelineDetailBadge(
                                title: "时长",
                                value: "\(frame.durationMs)ms",
                                systemImage: "timer"
                            )
                        }
                    }
                    ratioSliderRow(
                        title: "片段倍率",
                        value: Binding(
                            get: { viewModel.timelineClips.indices.contains(selected.index) ? viewModel.timelineClips[selected.index].speedScale : selected.clip.speedScale },
                            set: { viewModel.setTimelineClipSpeed(at: selected.index, speedScale: $0) }
                        ),
                        range: 0.25...4.0,
                        step: 0.25,
                        systemImage: "speedometer"
                    )
                    Button(role: .destructive) {
                        viewModel.removeTimelineClip(at: selected.index)
                    } label: {
                        Label("删除片段", systemImage: "trash")
                    }
                }
            } else if let selected = viewModel.selectedTimelineSound {
                VStack(alignment: .leading, spacing: 10) {
                    Label(selected.sound.assetId, systemImage: "speaker.wave.2")
                        .font(.subheadline)
                    sliderRow(
                        title: "音效开始",
                        value: Binding(
                            get: { Double(viewModel.timelineSounds.indices.contains(selected.index) ? viewModel.timelineSounds[selected.index].timeMs : selected.sound.timeMs) },
                            set: { viewModel.setTimelineSoundTime(at: selected.index, timeMs: Int($0.rounded())) }
                        ),
                        range: 0...Double(ActionTimeline.maxDurationMs),
                        step: 50,
                        systemImage: "music.note"
                    )
                    Button(role: .destructive) {
                        viewModel.removeTimelineSound(at: selected.index)
                    } label: {
                        Label("删除音效", systemImage: "trash")
                    }
                }
            }
        }
    }

    private func timelineX(forMs timeMs: Int) -> CGFloat {
        timelineTrackWidth * CGFloat(ActionTimelineTrackMetrics.fraction(forTimeMs: timeMs))
    }

    private func isSelectedClip(_ index: Int) -> Bool {
        viewModel.selectedTimelineItem == .clip(index)
    }

    private func isSelectedSound(_ index: Int) -> Bool {
        viewModel.selectedTimelineItem == .sound(index)
    }

    private func shortSoundName(_ assetId: String) -> String {
        assetId.replacingOccurrences(of: "sound.", with: "")
    }

    private func rhythmSymbol(for templateId: String) -> String {
        switch templateId {
        case "rhythm.micro_fast_head_shake":
            return "arrow.left.and.right"
        case "rhythm.micro_flash_nod":
            return "bolt.fill"
        case "rhythm.rhythmic_vertical_nod":
            return "metronome"
        case "rhythm.rhythmic_side_nod":
            return "waveform.path"
        case "rhythm.normal_head_shake":
            return "arrow.triangle.2.circlepath"
        case "rhythm.normal_vertical_nod":
            return "arrow.up.and.down"
        default:
            return "music.quarternote.3"
        }
    }

    private func clipSymbol(for clipId: String) -> String {
        switch clipId {
        case "clip.chat_soft_idle":
            return "bubble.left.and.bubble.right"
        case "clip.micro_fast_head_shake":
            return "arrow.left.and.right"
        case "clip.micro_flash_nod":
            return "bolt.fill"
        case "clip.rhythmic_vertical_nod":
            return "metronome"
        case "clip.rhythmic_side_nod":
            return "waveform.path"
        case "clip.normal_head_shake":
            return "arrow.triangle.2.circlepath"
        case "clip.normal_vertical_nod":
            return "arrow.up.and.down"
        default:
            return "film"
        }
    }

    private func timelineDetailBadge(title: String, value: String, systemImage: String) -> some View {
        Label {
            VStack(alignment: .leading, spacing: 2) {
                Text(title)
                    .font(.caption2)
                    .foregroundStyle(.secondary)
                Text(value)
                    .font(.caption.monospacedDigit())
                    .fontWeight(.semibold)
            }
        } icon: {
            Image(systemName: systemImage)
                .foregroundStyle(.secondary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private var savedActionSection: some View {
        GroupBox("动作") {
            VStack(alignment: .leading, spacing: 12) {
                Label(viewModel.lastSavedActionTitle, systemImage: "film.stack")
                HStack {
                    Button {
                        viewModel.previewLastAction()
                    } label: {
                        Label("预览", systemImage: "play.circle")
                    }
                    Button {
                        viewModel.stopAll()
                    } label: {
                        Label("回正", systemImage: "house")
                    }
                }
            }
        }
    }

    private func symbol(for mood: String) -> String {
        switch mood {
        case "happy": "face.smiling"
        case "love": "heart.fill"
        case "surprised": "exclamationmark.circle"
        case "thinking": "brain.head.profile"
        case "curious": "eye"
        case "sad": "cloud.rain"
        case "angry": "flame"
        default: "circle"
        }
    }
}

#Preview {
    ContentView()
}
