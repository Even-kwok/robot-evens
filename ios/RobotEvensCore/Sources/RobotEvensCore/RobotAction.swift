import Foundation

public enum ActionEndMode: String, Equatable, Sendable {
    case home
    case hold
}

public struct PoseKeyframe: Equatable, Sendable {
    public let timeMs: Int
    public let yaw: Int
    public let pitch: Int
    public let speed: Int

    public init(timeMs: Int, yaw: Int, pitch: Int, speed: Int) {
        self.timeMs = timeMs
        self.yaw = yaw
        self.pitch = pitch
        self.speed = speed
    }
}

public struct FaceEvent: Equatable, Sendable {
    public let timeMs: Int
    public let mood: String

    public init(timeMs: Int, mood: String) {
        self.timeMs = timeMs
        self.mood = mood
    }
}

public struct SoundEvent: Equatable, Sendable {
    public let timeMs: Int
    public let assetId: String

    public init(timeMs: Int, assetId: String) {
        self.timeMs = timeMs
        self.assetId = assetId
    }
}

public struct RobotAction: Equatable, Sendable {
    public let id: String
    public let title: String
    public let durationMs: Int
    public let startSoundAssetId: String?
    public let soundEvents: [SoundEvent]
    public let endMode: ActionEndMode
    public let poseKeyframes: [PoseKeyframe]
    public let faceEvents: [FaceEvent]

    public init(
        id: String,
        title: String,
        durationMs: Int,
        startSoundAssetId: String? = nil,
        soundEvents: [SoundEvent] = [],
        endMode: ActionEndMode,
        poseKeyframes: [PoseKeyframe],
        faceEvents: [FaceEvent]
    ) {
        self.id = id
        self.title = title
        self.durationMs = durationMs
        self.startSoundAssetId = startSoundAssetId
        self.soundEvents = soundEvents
        self.endMode = endMode
        self.poseKeyframes = poseKeyframes
        self.faceEvents = faceEvents
    }
}

public enum CompactActionCodec {
    public static func encode(_ action: RobotAction) throws -> Data {
        var object: [String: Any] = [
            "v": 1,
            "id": action.id,
            "title": action.title,
            "dur": action.durationMs,
            "end": action.endMode.rawValue,
            "poses": action.poseKeyframes.map { [$0.timeMs, $0.yaw, $0.pitch, $0.speed] },
            "faces": action.faceEvents.map { [$0.timeMs, $0.mood] }
        ]
        if let startSoundAssetId = action.startSoundAssetId, !startSoundAssetId.isEmpty {
            object["sound"] = startSoundAssetId
        }
        if !action.soundEvents.isEmpty {
            object["sounds"] = action.soundEvents.map { [$0.timeMs, $0.assetId] }
        }

        return try JSONSerialization.data(withJSONObject: object, options: [.sortedKeys])
    }
}

public enum RobotActionValidationError: Error, Equatable, Sendable {
    case invalidId
    case invalidTitle
    case durationOutOfRange
    case tooManyPoseKeyframes
    case tooManyFaceEvents
    case tooManySoundEvents
    case poseTimeOutOfRange
    case faceTimeOutOfRange
    case soundTimeOutOfRange
    case yawOutOfRange
    case pitchOutOfRange
    case speedOutOfRange
    case encodedSizeTooLarge
}

public enum RobotActionValidator {
    public static let maxDurationMs = 30_000
    public static let maxPoseKeyframes = 256
    public static let maxFaceEvents = 64
    public static let maxSoundEvents = 64
    public static let maxEncodedBytes = 32 * 1024

    public static func validate(_ action: RobotAction) throws {
        guard action.id.hasPrefix("action."), action.id.count > "action.".count else {
            throw RobotActionValidationError.invalidId
        }
        guard !action.title.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            throw RobotActionValidationError.invalidTitle
        }
        guard (1...maxDurationMs).contains(action.durationMs) else {
            throw RobotActionValidationError.durationOutOfRange
        }
        guard action.poseKeyframes.count <= maxPoseKeyframes else {
            throw RobotActionValidationError.tooManyPoseKeyframes
        }
        guard action.faceEvents.count <= maxFaceEvents else {
            throw RobotActionValidationError.tooManyFaceEvents
        }
        guard action.soundEvents.count <= maxSoundEvents else {
            throw RobotActionValidationError.tooManySoundEvents
        }

        for pose in action.poseKeyframes {
            guard (0...action.durationMs).contains(pose.timeMs) else {
                throw RobotActionValidationError.poseTimeOutOfRange
            }
            guard (-1280...1280).contains(pose.yaw) else {
                throw RobotActionValidationError.yawOutOfRange
            }
            guard (0...900).contains(pose.pitch) else {
                throw RobotActionValidationError.pitchOutOfRange
            }
            guard (0...1000).contains(pose.speed) else {
                throw RobotActionValidationError.speedOutOfRange
            }
        }

        for faceEvent in action.faceEvents {
            guard (0...action.durationMs).contains(faceEvent.timeMs) else {
                throw RobotActionValidationError.faceTimeOutOfRange
            }
        }

        for soundEvent in action.soundEvents {
            guard (0...action.durationMs).contains(soundEvent.timeMs) else {
                throw RobotActionValidationError.soundTimeOutOfRange
            }
        }

        let encoded = try CompactActionCodec.encode(action)
        guard encoded.count <= maxEncodedBytes else {
            throw RobotActionValidationError.encodedSizeTooLarge
        }
    }
}

public struct ActionClip: Equatable, Sendable {
    public let id: String
    public let title: String
    public let durationMs: Int
    public let poseKeyframes: [PoseKeyframe]
    public let faceEvents: [FaceEvent]

    public init(
        id: String,
        title: String,
        durationMs: Int,
        poseKeyframes: [PoseKeyframe],
        faceEvents: [FaceEvent] = []
    ) {
        self.id = id
        self.title = title
        self.durationMs = durationMs
        self.poseKeyframes = poseKeyframes
        self.faceEvents = faceEvents
    }
}

public enum BuiltInActionClipLibrary {
    public static let clips: [ActionClip] = [
        ActionClip(
            id: "clip.chat_soft_idle",
            title: "聊天轻点头摇头",
            durationMs: 2400,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 190),
                PoseKeyframe(timeMs: 360, yaw: 18, pitch: 128, speed: 190),
                PoseKeyframe(timeMs: 760, yaw: -22, pitch: 112, speed: 185),
                PoseKeyframe(timeMs: 1160, yaw: 14, pitch: 130, speed: 180),
                PoseKeyframe(timeMs: 1560, yaw: -14, pitch: 116, speed: 175),
                PoseKeyframe(timeMs: 1960, yaw: 8, pitch: 124, speed: 170),
                PoseKeyframe(timeMs: 2400, yaw: 0, pitch: 120, speed: 165)
            ]
        ),
        ActionClip(
            id: "clip.micro_fast_head_shake",
            title: "微小快速摇头",
            durationMs: 480,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 850),
                PoseKeyframe(timeMs: 80, yaw: 70, pitch: 120, speed: 850),
                PoseKeyframe(timeMs: 160, yaw: -70, pitch: 120, speed: 850),
                PoseKeyframe(timeMs: 240, yaw: 65, pitch: 120, speed: 850),
                PoseKeyframe(timeMs: 320, yaw: -55, pitch: 120, speed: 850),
                PoseKeyframe(timeMs: 480, yaw: 0, pitch: 120, speed: 850)
            ]
        ),
        ActionClip(
            id: "clip.micro_flash_nod",
            title: "微小左右快闪点头",
            durationMs: 420,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 900),
                PoseKeyframe(timeMs: 70, yaw: 40, pitch: 145, speed: 900),
                PoseKeyframe(timeMs: 140, yaw: -40, pitch: 105, speed: 900),
                PoseKeyframe(timeMs: 210, yaw: 35, pitch: 150, speed: 900),
                PoseKeyframe(timeMs: 280, yaw: -25, pitch: 110, speed: 900),
                PoseKeyframe(timeMs: 420, yaw: 0, pitch: 120, speed: 900)
            ]
        ),
        ActionClip(
            id: "clip.rhythmic_vertical_nod",
            title: "有节奏上下点头",
            durationMs: 1600,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 520),
                PoseKeyframe(timeMs: 220, yaw: 0, pitch: 245, speed: 520),
                PoseKeyframe(timeMs: 420, yaw: 0, pitch: 95, speed: 520),
                PoseKeyframe(timeMs: 680, yaw: 0, pitch: 230, speed: 520),
                PoseKeyframe(timeMs: 880, yaw: 0, pitch: 105, speed: 520),
                PoseKeyframe(timeMs: 1160, yaw: 0, pitch: 210, speed: 500),
                PoseKeyframe(timeMs: 1600, yaw: 0, pitch: 120, speed: 480)
            ],
            faceEvents: [FaceEvent(timeMs: 0, mood: "happy")]
        ),
        ActionClip(
            id: "clip.rhythmic_side_nod",
            title: "有节奏左右点头",
            durationMs: 1600,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 520),
                PoseKeyframe(timeMs: 220, yaw: 180, pitch: 135, speed: 520),
                PoseKeyframe(timeMs: 420, yaw: -180, pitch: 105, speed: 520),
                PoseKeyframe(timeMs: 680, yaw: 170, pitch: 135, speed: 520),
                PoseKeyframe(timeMs: 880, yaw: -160, pitch: 105, speed: 520),
                PoseKeyframe(timeMs: 1160, yaw: 120, pitch: 125, speed: 500),
                PoseKeyframe(timeMs: 1600, yaw: 0, pitch: 120, speed: 480)
            ],
            faceEvents: [FaceEvent(timeMs: 0, mood: "curious")]
        ),
        ActionClip(
            id: "clip.normal_head_shake",
            title: "正常左右摇头",
            durationMs: 1200,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 480),
                PoseKeyframe(timeMs: 220, yaw: 380, pitch: 120, speed: 520),
                PoseKeyframe(timeMs: 460, yaw: -380, pitch: 120, speed: 520),
                PoseKeyframe(timeMs: 720, yaw: 320, pitch: 120, speed: 500),
                PoseKeyframe(timeMs: 940, yaw: -240, pitch: 120, speed: 460),
                PoseKeyframe(timeMs: 1200, yaw: 0, pitch: 120, speed: 420)
            ],
            faceEvents: [FaceEvent(timeMs: 0, mood: "neutral")]
        ),
        ActionClip(
            id: "clip.normal_vertical_nod",
            title: "正常上下点头",
            durationMs: 1000,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 480),
                PoseKeyframe(timeMs: 200, yaw: 0, pitch: 330, speed: 520),
                PoseKeyframe(timeMs: 420, yaw: 0, pitch: 80, speed: 520),
                PoseKeyframe(timeMs: 660, yaw: 0, pitch: 260, speed: 480),
                PoseKeyframe(timeMs: 1000, yaw: 0, pitch: 120, speed: 420)
            ],
            faceEvents: [FaceEvent(timeMs: 0, mood: "happy")]
        )
    ]
}

public enum ChatMotionCallState: String, CaseIterable, Equatable, Sendable, Identifiable {
    case listening
    case thinking
    case speaking
    case settling

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .listening: "听你说"
        case .thinking: "思考"
        case .speaking: "回答中"
        case .settling: "停顿"
        }
    }
}

public enum ChatMotionStyle: String, CaseIterable, Equatable, Sendable, Identifiable {
    case natural
    case calm
    case engaged

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .natural: "自然"
        case .calm: "安静"
        case .engaged: "专注"
        }
    }
}

public struct ChatMotionCue: Equatable, Sendable {
    public let state: ChatMotionCallState
    public let clipId: String
    public let nextDelayMs: Int
    public let speedScale: Double

    public init(state: ChatMotionCallState, clipId: String, nextDelayMs: Int, speedScale: Double) {
        self.state = state
        self.clipId = clipId
        self.nextDelayMs = nextDelayMs
        self.speedScale = speedScale
    }
}

public enum ChatMotionPlanner {
    public static let softIdleClipId = "clip.chat_soft_idle"

    public static func cue(
        isStreaming: Bool,
        stage: String?,
        style: ChatMotionStyle,
        randomUnit: Double
    ) -> ChatMotionCue? {
        guard isStreaming else { return nil }
        let state = callState(for: stage)
        let delayRange = delayRangeMs(for: state)
        let styleDelayScale = delayScale(for: style)
        let unit = rhythmClamp(randomUnit, low: 0, high: 1)
        let baseDelay = Double(delayRange.min) + (Double(delayRange.max - delayRange.min) * unit)
        let delayMs = max(1_200, Int((baseDelay * styleDelayScale).rounded()))
        let speedScale = rhythmClamp(baseSpeedScale(for: state) + speedOffset(for: style), low: 0.45, high: 1.25)
        return ChatMotionCue(
            state: state,
            clipId: softIdleClipId,
            nextDelayMs: delayMs,
            speedScale: speedScale
        )
    }

    public static func callState(for stage: String?) -> ChatMotionCallState {
        let normalized = (stage ?? "").lowercased()
        if normalized.contains("speak") || normalized.contains("answer") || normalized.contains("reply") || normalized.contains("play") {
            return .speaking
        }
        if normalized.contains("think") || normalized.contains("process") || normalized.contains("generat") || normalized.contains("api") {
            return .thinking
        }
        if normalized.contains("listen") || normalized.contains("mic") || normalized.contains("record") || normalized.contains("audio") {
            return .listening
        }
        return .settling
    }

    private static func delayRangeMs(for state: ChatMotionCallState) -> (min: Int, max: Int) {
        switch state {
        case .listening:
            return (3_600, 7_200)
        case .thinking:
            return (5_200, 8_200)
        case .speaking:
            return (2_600, 4_200)
        case .settling:
            return (6_200, 10_000)
        }
    }

    private static func baseSpeedScale(for state: ChatMotionCallState) -> Double {
        switch state {
        case .listening:
            return 0.78
        case .thinking:
            return 0.66
        case .speaking:
            return 0.92
        case .settling:
            return 0.62
        }
    }

    private static func delayScale(for style: ChatMotionStyle) -> Double {
        switch style {
        case .natural:
            return 1.0
        case .calm:
            return 1.35
        case .engaged:
            return 0.72
        }
    }

    private static func speedOffset(for style: ChatMotionStyle) -> Double {
        switch style {
        case .natural:
            return 0
        case .calm:
            return -0.08
        case .engaged:
            return 0.12
        }
    }
}

public enum RhythmMotionCurve: String, CaseIterable, Equatable, Sendable {
    case linear
    case easeInOut
    case bounce
    case beat

    public var title: String {
        switch self {
        case .linear: "线性"
        case .easeInOut: "缓入缓出"
        case .bounce: "弹跳"
        case .beat: "节拍型"
        }
    }

    public func value(at phase: Double) -> Double {
        let normalized = phase - floor(phase)
        switch self {
        case .linear:
            if normalized < 0.25 {
                return normalized * 4
            } else if normalized < 0.75 {
                return 2 - normalized * 4
            } else {
                return normalized * 4 - 4
            }
        case .easeInOut:
            return sin(normalized * 2 * .pi)
        case .bounce:
            let wave = sin(normalized * 2 * .pi)
            let rebound = 0.22 * sin(normalized * 6 * .pi)
            return rhythmClamp(wave + rebound, low: -1, high: 1)
        case .beat:
            if normalized < 0.12 {
                return normalized / 0.12
            } else if normalized < 0.24 {
                return 1 - ((normalized - 0.12) / 0.12)
            } else if normalized < 0.5 {
                return 0
            } else if normalized < 0.62 {
                return -((normalized - 0.5) / 0.12)
            } else if normalized < 0.74 {
                return -1 + ((normalized - 0.62) / 0.12)
            } else {
                return 0
            }
        }
    }

    public func releasePulse(at progress: Double) -> Double {
        let normalized = rhythmClamp(progress, low: 0, high: 1)
        switch self {
        case .linear:
            return 1 - normalized
        case .easeInOut:
            return (cos(normalized * .pi) + 1) / 2
        case .bounce:
            let base = 1 - normalized
            let rebound = 0.18 * sin(normalized * 4 * .pi) * base
            return rhythmClamp(base + rebound, low: 0, high: 1)
        case .beat:
            if normalized < 0.16 {
                return 1
            } else if normalized < 0.46 {
                return 1 - ((normalized - 0.16) / 0.3)
            } else if normalized < 0.62 {
                return 0.18
            } else {
                return rhythmClamp(0.18 * (1 - ((normalized - 0.62) / 0.38)), low: 0, high: 1)
            }
        }
    }
}

public struct RhythmMotionTemplate: Equatable, Sendable, Identifiable {
    public var id: String
    public var title: String
    public var centerYaw: Int
    public var centerPitch: Int
    public var yawAmplitude: Int
    public var pitchAmplitude: Int
    public var cycleMs: Int
    public var yawPhaseDegrees: Double
    public var pitchPhaseDegrees: Double
    public var speed: Int
    public var curve: RhythmMotionCurve

    public init(
        id: String,
        title: String,
        centerYaw: Int,
        centerPitch: Int,
        yawAmplitude: Int,
        pitchAmplitude: Int,
        cycleMs: Int,
        yawPhaseDegrees: Double = 0,
        pitchPhaseDegrees: Double = 0,
        speed: Int,
        curve: RhythmMotionCurve
    ) {
        self.id = id
        self.title = title
        self.centerYaw = rhythmClamp(centerYaw, low: -1280, high: 1280)
        self.centerPitch = rhythmClamp(centerPitch, low: 0, high: 900)
        self.yawAmplitude = rhythmClamp(yawAmplitude, low: 0, high: 1280)
        self.pitchAmplitude = rhythmClamp(pitchAmplitude, low: 0, high: 900)
        self.cycleMs = max(60, cycleMs)
        self.yawPhaseDegrees = yawPhaseDegrees
        self.pitchPhaseDegrees = pitchPhaseDegrees
        self.speed = rhythmClamp(speed, low: 0, high: 1000)
        self.curve = curve
    }

    public func pose(atElapsedMs elapsedMs: Int) -> ServoPose {
        let phase = Double(max(0, elapsedMs)) / Double(max(1, cycleMs))
        let yawOffset = curve.value(at: phase + yawPhaseDegrees / 360) * Double(yawAmplitude)
        let pitchOffset = curve.value(at: phase + pitchPhaseDegrees / 360) * Double(pitchAmplitude)
        return ServoPose(
            yaw: rhythmClamp(centerYaw + Int(yawOffset.rounded()), low: -1280, high: 1280),
            pitch: rhythmClamp(centerPitch + Int(pitchOffset.rounded()), low: 0, high: 900),
            speed: speed
        )
    }
}

public enum BuiltInRhythmTemplateLibrary {
    public static let templates: [RhythmMotionTemplate] = [
        RhythmMotionTemplate(
            id: "rhythm.micro_fast_head_shake",
            title: "微小快速摇头",
            centerYaw: 0,
            centerPitch: 120,
            yawAmplitude: 70,
            pitchAmplitude: 0,
            cycleMs: 320,
            speed: 850,
            curve: .easeInOut
        ),
        RhythmMotionTemplate(
            id: "rhythm.micro_flash_nod",
            title: "微小左右快闪点头",
            centerYaw: 0,
            centerPitch: 128,
            yawAmplitude: 40,
            pitchAmplitude: 28,
            cycleMs: 280,
            pitchPhaseDegrees: 90,
            speed: 900,
            curve: .beat
        ),
        RhythmMotionTemplate(
            id: "rhythm.rhythmic_vertical_nod",
            title: "有节奏上下点头",
            centerYaw: 0,
            centerPitch: 165,
            yawAmplitude: 0,
            pitchAmplitude: 86,
            cycleMs: 520,
            speed: 520,
            curve: .linear
        ),
        RhythmMotionTemplate(
            id: "rhythm.rhythmic_side_nod",
            title: "有节奏左右点头",
            centerYaw: 0,
            centerPitch: 120,
            yawAmplitude: 180,
            pitchAmplitude: 18,
            cycleMs: 520,
            pitchPhaseDegrees: 90,
            speed: 520,
            curve: .linear
        ),
        RhythmMotionTemplate(
            id: "rhythm.normal_head_shake",
            title: "正常左右摇头",
            centerYaw: 0,
            centerPitch: 120,
            yawAmplitude: 380,
            pitchAmplitude: 0,
            cycleMs: 900,
            speed: 520,
            curve: .easeInOut
        ),
        RhythmMotionTemplate(
            id: "rhythm.normal_vertical_nod",
            title: "正常上下点头",
            centerYaw: 0,
            centerPitch: 205,
            yawAmplitude: 0,
            pitchAmplitude: 125,
            cycleMs: 760,
            speed: 520,
            curve: .easeInOut
        )
    ]
}

public struct RhythmActiveTemplate: Equatable, Sendable {
    public var template: RhythmMotionTemplate
    public var startedAtMs: Int

    public init(template: RhythmMotionTemplate, startedAtMs: Int) {
        self.template = template
        self.startedAtMs = startedAtMs
    }
}

public enum RhythmTemplateMixer {
    public static let basePose = ServoPose(yaw: 0, pitch: 120, speed: 700)

    public static func pose(
        atElapsedMs elapsedMs: Int,
        activeLayers: [RhythmActiveTemplate],
        basePose: ServoPose = Self.basePose
    ) -> ServoPose {
        guard !activeLayers.isEmpty else { return basePose }

        var yaw = basePose.yaw
        var pitch = basePose.pitch
        var speed = basePose.speed

        for layer in activeLayers where elapsedMs >= layer.startedAtMs {
            let localPose = layer.template.pose(atElapsedMs: elapsedMs - layer.startedAtMs)
            yaw += localPose.yaw - layer.template.centerYaw
            pitch += localPose.pitch - layer.template.centerPitch
            speed = max(speed, localPose.speed)
        }

        return ServoPose(
            yaw: rhythmClamp(yaw, low: -1280, high: 1280),
            pitch: rhythmClamp(pitch, low: 0, high: 900),
            speed: rhythmClamp(speed, low: 0, high: 1000)
        )
    }
}

public struct RhythmTemplateEvent: Equatable, Sendable {
    public var timeMs: Int
    public var templateId: String
    public var isActive: Bool

    public init(timeMs: Int, templateId: String, isActive: Bool) {
        self.timeMs = max(0, timeMs)
        self.templateId = templateId
        self.isActive = isActive
    }
}

public enum RhythmPerformanceRenderer {
    public static let defaultSampleIntervalMs = 60

    public static func renderAction(
        id: String,
        title: String,
        durationMs: Int,
        events: [RhythmTemplateEvent],
        templates: [RhythmMotionTemplate],
        sampleIntervalMs: Int = defaultSampleIntervalMs,
        basePose: ServoPose = RhythmTemplateMixer.basePose,
        endMode: ActionEndMode = .home
    ) throws -> RobotAction {
        let clampedDurationMs = rhythmClamp(durationMs, low: 1, high: ActionTimeline.maxDurationMs)
        let intervalMs = max(1, sampleIntervalMs)
        let templatesById = Dictionary(uniqueKeysWithValues: templates.map { ($0.id, $0) })
        let sortedEvents = events
            .filter { (0...clampedDurationMs).contains($0.timeMs) }
            .sorted { lhs, rhs in
                if lhs.timeMs == rhs.timeMs {
                    return lhs.isActive && !rhs.isActive
                }
                return lhs.timeMs < rhs.timeMs
            }

        var eventIndex = 0
        var activeStarts: [String: Int] = [:]
        var poses: [PoseKeyframe] = []

        for sampleMs in sampleTimes(durationMs: clampedDurationMs, intervalMs: intervalMs) {
            while eventIndex < sortedEvents.count, sortedEvents[eventIndex].timeMs <= sampleMs {
                let event = sortedEvents[eventIndex]
                if event.isActive {
                    activeStarts[event.templateId] = event.timeMs
                } else {
                    activeStarts.removeValue(forKey: event.templateId)
                }
                eventIndex += 1
            }

            let activeLayers = activeStarts.compactMap { templateId, startedAtMs -> RhythmActiveTemplate? in
                guard let template = templatesById[templateId] else { return nil }
                return RhythmActiveTemplate(template: template, startedAtMs: startedAtMs)
            }
            let pose = RhythmTemplateMixer.pose(atElapsedMs: sampleMs, activeLayers: activeLayers, basePose: basePose)
            poses.append(PoseKeyframe(timeMs: sampleMs, yaw: pose.yaw, pitch: pose.pitch, speed: pose.speed))
        }

        let action = RobotAction(
            id: id,
            title: title,
            durationMs: clampedDurationMs,
            endMode: endMode,
            poseKeyframes: poses,
            faceEvents: []
        )
        try RobotActionValidator.validate(action)
        return action
    }

    private static func sampleTimes(durationMs: Int, intervalMs: Int) -> [Int] {
        var times = stride(from: 0, through: durationMs, by: intervalMs).map { $0 }
        if times.last != durationMs {
            times.append(durationMs)
        }
        return times
    }
}

public struct ActionTimelineClip: Equatable, Sendable {
    public let clipId: String
    public let startTimeMs: Int
    public let speedScale: Double

    public init(clipId: String, startTimeMs: Int, speedScale: Double = 1.0) {
        self.clipId = clipId
        self.startTimeMs = startTimeMs
        self.speedScale = speedScale
    }
}

public struct ActionTimeline: Equatable, Sendable {
    public static let maxDurationMs = 15_000

    public let title: String
    public let clips: [ActionTimelineClip]
    public let sounds: [SoundEvent]

    public init(title: String, clips: [ActionTimelineClip], sounds: [SoundEvent]) {
        self.title = title
        self.clips = clips
        self.sounds = sounds
    }
}

public struct ActionTimelineTrackFrame: Equatable, Sendable {
    public let startFraction: Double
    public let widthFraction: Double
    public let durationMs: Int

    public init(startFraction: Double, widthFraction: Double, durationMs: Int) {
        self.startFraction = startFraction
        self.widthFraction = widthFraction
        self.durationMs = durationMs
    }
}

public enum ActionTimelineTrackMetrics {
    public static func frame(for placement: ActionTimelineClip, clip: ActionClip) -> ActionTimelineTrackFrame {
        let durationMs = scaledTime(clip.durationMs, by: placement.speedScale)
        return ActionTimelineTrackFrame(
            startFraction: fraction(forTimeMs: placement.startTimeMs),
            widthFraction: fraction(forTimeMs: durationMs),
            durationMs: durationMs
        )
    }

    public static func fraction(forTimeMs timeMs: Int) -> Double {
        let clamped = min(max(timeMs, 0), ActionTimeline.maxDurationMs)
        return Double(clamped) / Double(ActionTimeline.maxDurationMs)
    }

    public static func scaledTime(_ timeMs: Int, by speedScale: Double) -> Int {
        guard speedScale > 0 else { return timeMs }
        return Int((Double(timeMs) / speedScale).rounded())
    }
}

public enum ActionTimelineComposerError: Error, Equatable, Sendable {
    case missingClip(String)
    case invalidSpeedScale
    case timelineTooLong
    case overlappingClips
}

public enum ActionTimelineSequencer {
    public static func duration(
        for placement: ActionTimelineClip,
        clipLibrary: [ActionClip] = BuiltInActionClipLibrary.clips
    ) throws -> Int {
        guard (0.25...4.0).contains(placement.speedScale) else {
            throw ActionTimelineComposerError.invalidSpeedScale
        }
        guard let clip = clipLibrary.first(where: { $0.id == placement.clipId }) else {
            throw ActionTimelineComposerError.missingClip(placement.clipId)
        }
        return ActionTimelineTrackMetrics.scaledTime(clip.durationMs, by: placement.speedScale)
    }

    public static func endTime(
        for placement: ActionTimelineClip,
        clipLibrary: [ActionClip] = BuiltInActionClipLibrary.clips
    ) throws -> Int {
        guard (0...ActionTimeline.maxDurationMs).contains(placement.startTimeMs) else {
            throw ActionTimelineComposerError.timelineTooLong
        }
        let endTimeMs = placement.startTimeMs + (try duration(for: placement, clipLibrary: clipLibrary))
        guard endTimeMs <= ActionTimeline.maxDurationMs else {
            throw ActionTimelineComposerError.timelineTooLong
        }
        return endTimeMs
    }

    public static func nextStart(
        after placements: [ActionTimelineClip],
        clipLibrary: [ActionClip] = BuiltInActionClipLibrary.clips
    ) -> Int {
        let endTimes = placements.compactMap { placement in
            try? endTime(for: placement, clipLibrary: clipLibrary)
        }
        return min(endTimes.max() ?? 0, ActionTimeline.maxDurationMs)
    }

    public static func sequencedClips(
        _ placements: [ActionTimelineClip],
        clipLibrary: [ActionClip] = BuiltInActionClipLibrary.clips
    ) throws -> [ActionTimelineClip] {
        var cursor = 0
        var sequenced: [ActionTimelineClip] = []

        for placement in placements {
            let durationMs = try duration(for: placement, clipLibrary: clipLibrary)
            let endTimeMs = cursor + durationMs
            guard endTimeMs <= ActionTimeline.maxDurationMs else {
                throw ActionTimelineComposerError.timelineTooLong
            }
            sequenced.append(ActionTimelineClip(
                clipId: placement.clipId,
                startTimeMs: cursor,
                speedScale: placement.speedScale
            ))
            cursor = endTimeMs
        }

        return sequenced
    }

    public static func validateNoOverlaps(
        _ placements: [ActionTimelineClip],
        clipLibrary: [ActionClip] = BuiltInActionClipLibrary.clips
    ) throws {
        let intervals = try placements.map { placement in
            (
                start: placement.startTimeMs,
                end: try endTime(for: placement, clipLibrary: clipLibrary)
            )
        }
        let sorted = intervals.sorted { lhs, rhs in
            if lhs.start == rhs.start {
                return lhs.end < rhs.end
            }
            return lhs.start < rhs.start
        }

        for index in sorted.indices.dropLast() {
            if sorted[index].end > sorted[index + 1].start {
                throw ActionTimelineComposerError.overlappingClips
            }
        }
    }
}

public enum ActionTimelineComposer {
    public static func compose(
        _ timeline: ActionTimeline,
        clipLibrary: [ActionClip] = BuiltInActionClipLibrary.clips,
        id: String,
        endMode: ActionEndMode = .home
    ) throws -> RobotAction {
        let clipsById = Dictionary(uniqueKeysWithValues: clipLibrary.map { ($0.id, $0) })
        var poses: [PoseKeyframe] = []
        var faces: [FaceEvent] = []
        var sounds: [SoundEvent] = []
        var durationMs = 1

        for sound in timeline.sounds {
            guard (0...ActionTimeline.maxDurationMs).contains(sound.timeMs) else {
                throw ActionTimelineComposerError.timelineTooLong
            }
            sounds.append(sound)
            durationMs = max(durationMs, sound.timeMs)
        }

        try ActionTimelineSequencer.validateNoOverlaps(timeline.clips, clipLibrary: clipLibrary)

        for placement in timeline.clips {
            guard let clip = clipsById[placement.clipId] else {
                throw ActionTimelineComposerError.missingClip(placement.clipId)
            }

            let clipEnd = try ActionTimelineSequencer.endTime(for: placement, clipLibrary: clipLibrary)
            durationMs = max(durationMs, clipEnd)

            for pose in clip.poseKeyframes {
                poses.append(PoseKeyframe(
                    timeMs: placement.startTimeMs + ActionTimelineTrackMetrics.scaledTime(pose.timeMs, by: placement.speedScale),
                    yaw: pose.yaw,
                    pitch: pose.pitch,
                    speed: clamp(Int((Double(pose.speed) * placement.speedScale).rounded()), low: 0, high: 1000)
                ))
            }

            for face in clip.faceEvents {
                faces.append(FaceEvent(
                    timeMs: placement.startTimeMs + ActionTimelineTrackMetrics.scaledTime(face.timeMs, by: placement.speedScale),
                    mood: face.mood
                ))
            }
        }

        let action = RobotAction(
            id: id,
            title: timeline.title,
            durationMs: min(durationMs, ActionTimeline.maxDurationMs),
            soundEvents: sounds.sorted { $0.timeMs < $1.timeMs },
            endMode: endMode,
            poseKeyframes: poses.sorted { $0.timeMs < $1.timeMs },
            faceEvents: faces.sorted { $0.timeMs < $1.timeMs }
        )
        try RobotActionValidator.validate(action)
        return action
    }

    private static func clamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
        min(max(value, low), high)
    }
}

private func rhythmClamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
