import Foundation

public struct ActionRecorderConfiguration: Equatable, Sendable {
    public let minPoseSpacingMs: Int
    public let minYawDelta: Int
    public let minPitchDelta: Int

    public init(
        minPoseSpacingMs: Int = 50,
        minYawDelta: Int = 8,
        minPitchDelta: Int = 8
    ) {
        self.minPoseSpacingMs = minPoseSpacingMs
        self.minYawDelta = minYawDelta
        self.minPitchDelta = minPitchDelta
    }
}

public enum ActionRecorderError: Error, Equatable, Sendable {
    case noActiveRecording
}

public struct ActionRecorder: Sendable {
    private struct TimedPose: Equatable, Sendable {
        let timeMs: Int
        let pose: ServoPose
    }

    private let configuration: ActionRecorderConfiguration
    private var id: String = ""
    private var title: String = ""
    private var startMs: Int?
    private var pausedAtMs: Int?
    private var pausedDurationMs: Int = 0
    private var stoppedElapsedMs: Int?
    private var startSoundAssetId: String?
    private var rawPoses: [TimedPose] = []
    private var faceEvents: [FaceEvent] = []

    public init(configuration: ActionRecorderConfiguration = ActionRecorderConfiguration()) {
        self.configuration = configuration
    }

    public mutating func start(id: String, title: String, atMs: Int) {
        self.id = id
        self.title = title
        self.startMs = atMs
        self.pausedAtMs = nil
        self.pausedDurationMs = 0
        self.stoppedElapsedMs = nil
        self.startSoundAssetId = nil
        self.rawPoses = []
        self.faceEvents = []
    }

    public mutating func setStartSoundAssetId(_ assetId: String?) {
        guard startMs != nil else { return }
        let trimmed = assetId?.trimmingCharacters(in: .whitespacesAndNewlines)
        startSoundAssetId = trimmed?.isEmpty == false ? trimmed : nil
    }

    public mutating func pause(atMs: Int) {
        guard startMs != nil, pausedAtMs == nil else { return }
        pausedAtMs = atMs
    }

    public mutating func resume(atMs: Int) {
        guard let pausedAtMs else { return }
        pausedDurationMs += max(0, atMs - pausedAtMs)
        self.pausedAtMs = nil
    }

    public mutating func stop(atMs: Int) {
        guard startMs != nil else { return }
        stoppedElapsedMs = elapsedMs(atMs: atMs)
    }

    public mutating func recordPose(_ pose: ServoPose, atMs: Int) {
        guard isAcceptingEvents else { return }
        rawPoses.append(TimedPose(timeMs: elapsedMs(atMs: atMs), pose: pose))
    }

    public mutating func recordFaceEvent(mood: String, atMs: Int) {
        guard isAcceptingEvents else { return }
        faceEvents.append(FaceEvent(timeMs: elapsedMs(atMs: atMs), mood: mood))
    }

    public func buildAction(endMode: ActionEndMode) throws -> RobotAction {
        guard startMs != nil else {
            throw ActionRecorderError.noActiveRecording
        }

        let durationMs = max(
            stoppedElapsedMs ?? 0,
            rawPoses.last?.timeMs ?? 0,
            faceEvents.last?.timeMs ?? 0,
            1
        )
        let action = RobotAction(
            id: id,
            title: title,
            durationMs: durationMs,
            startSoundAssetId: startSoundAssetId,
            endMode: endMode,
            poseKeyframes: compressedPoseKeyframes(),
            faceEvents: faceEvents
        )
        try RobotActionValidator.validate(action)
        return action
    }

    private var isAcceptingEvents: Bool {
        startMs != nil && pausedAtMs == nil && stoppedElapsedMs == nil
    }

    private func elapsedMs(atMs: Int) -> Int {
        guard let startMs else { return 0 }
        let effectiveAtMs = pausedAtMs ?? atMs
        return max(0, effectiveAtMs - startMs - pausedDurationMs)
    }

    private func compressedPoseKeyframes() -> [PoseKeyframe] {
        guard let first = rawPoses.first else { return [] }
        guard rawPoses.count > 1 else {
            return [keyframe(from: first)]
        }

        var kept: [TimedPose] = [first]
        let middle = rawPoses.dropFirst().dropLast()
        for sample in middle {
            guard let last = kept.last else { continue }
            let spacedEnough = sample.timeMs - last.timeMs >= configuration.minPoseSpacingMs
            let changedEnough =
                abs(sample.pose.yaw - last.pose.yaw) >= configuration.minYawDelta ||
                abs(sample.pose.pitch - last.pose.pitch) >= configuration.minPitchDelta ||
                sample.pose.speed != last.pose.speed

            if spacedEnough && changedEnough {
                kept.append(sample)
            }
        }

        if let last = rawPoses.last, kept.last != last {
            kept.append(last)
        }

        return kept.map(keyframe(from:))
    }

    private func keyframe(from timedPose: TimedPose) -> PoseKeyframe {
        PoseKeyframe(
            timeMs: timedPose.timeMs,
            yaw: timedPose.pose.yaw,
            pitch: timedPose.pose.pitch,
            speed: timedPose.pose.speed
        )
    }
}
